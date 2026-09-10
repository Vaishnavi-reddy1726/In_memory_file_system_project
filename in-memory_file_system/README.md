# In-Memory File System with LRU/LFU Caching

A C++17 in-memory file system that supports full CRUD operations backed by a
pluggable, strictly O(1) caching layer using either an **LRU** (Least
Recently Used) or **LFU** (Least Frequently Used) eviction policy.

## Features

- **Dual caching strategies** — `LRUCache<Key, Value>` and
  `LFUCache<Key, Value>` are generic templates implementing a shared
  `ICache<Key, Value>` interface, so the file system can be configured with
  either policy at construction time.
- **Strict O(1) operations** — both caches use a doubly-linked list paired
  with a hash map (`std::unordered_map`) so that `get`, `put`, and `remove`
  never scan the cache. LFU additionally buckets nodes by access frequency
  (`freqMap_`) and tracks the minimum frequency so eviction is also O(1).
- **Smart pointers** — cache nodes are managed with `std::shared_ptr` /
  `std::weak_ptr` (to avoid reference cycles in the linked list), and the
  active cache strategy is owned via `std::unique_ptr<ICache<...>>` inside
  `FileSystem`.
- **Full CRUD** — `createFile`, `readFile`, `updateFile`, `deleteFile`, plus
  `listFiles` / `exists` for introspection.
- **Automatic cache invalidation** — every `updateFile` and `deleteFile`
  call invalidates the corresponding cache entry immediately (and
  `updateFile` repopulates it with the fresh value), so reads can never
  return stale data.
- **Cache hit/miss stats** — `FileSystem::stats()` reports hits, misses,
  and hit rate so you can compare LRU vs LFU behavior under different
  access patterns.

## Project layout

```
inmemory-fs/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── ICache.hpp        # generic cache interface
│   ├── LRUCache.hpp      # O(1) LRU: doubly-linked list + hash map
│   ├── LFUCache.hpp      # O(1) LFU: frequency-bucketed linked lists + hash maps
│   └── FileSystem.hpp    # file system CRUD API
└── src/
    ├── FileSystem.cpp    # CRUD implementation + cache invalidation
    └── main.cpp          # interactive CLI demo
```

## Building

Requires CMake 3.10+ and a C++17 compiler.

```bash
mkdir build && cd build
cmake ..
cmake --build .
./fs_demo
```

Or compile directly without CMake:

```bash
g++ -std=c++17 -Iinclude src/main.cpp src/FileSystem.cpp -o fs_demo
./fs_demo
```

## Using the demo

On startup you'll be asked to pick a policy (LRU or LFU) and a cache
capacity, then you get a small shell:

```
> create /notes.txt hello world
created /notes.txt
> read /notes.txt
hello world
> update /notes.txt goodbye world
updated /notes.txt
> read /notes.txt
goodbye world
> delete /notes.txt
deleted /notes.txt
> read /notes.txt
error: /notes.txt not found
> stats
[LRU] disk files=0 cached=0 hits=1 misses=1 hitRate=50.0%
```

Create more files than the cache capacity and re-read older ones to watch
eviction behavior differ between LRU (evicts by recency) and LFU (evicts by
access frequency) — `stats` shows the resulting hit rate for each policy.

## Design notes

- **LRU** keeps a single doubly-linked list ordered by recency. Every
  `get`/`put` moves the touched node to the front in O(1); eviction simply
  drops the tail.
- **LFU** keeps one doubly-linked list *per frequency count* plus a
  `minFreq_` pointer. Touching a key removes it from its current frequency
  bucket and pushes it to the front of the next bucket, both O(1); eviction
  removes the tail of the `minFreq_` bucket.
- `FileSystem` treats `storage_` (an `unordered_map<string, string>`) as the
  authoritative "disk", and the cache as a pure performance optimization
  layered on top — it is never a second source of truth, which is what
  makes the invalidation-on-write/delete logic safe and simple.
