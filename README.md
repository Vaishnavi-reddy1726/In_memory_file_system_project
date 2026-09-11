# **In-Memory File System with LRU/LFU Caching**

A C++17 in-memory file system that supports full CRUD operations backed by a pluggable, strictly O(1) caching layer using either an LRU (Least Recently Used) or LFU (Least Frequently Used) eviction policy.

## **Features**

### **1. Dual Caching Strategies**

`LRUCache<Key, Value>` and `LFUCache<Key, Value>` are generic templates implementing a shared `ICache<Key, Value>` interface, allowing the file system to be configured with either caching policy at construction time.

### **2. Strict O(1) Operations**

Both caches use a doubly-linked list paired with a hash map (`std::unordered_map`) so that `get`, `put`, and `remove` operations never scan the cache.

LFU additionally buckets nodes by access frequency (`freqMap_`) and tracks the minimum frequency so eviction remains O(1).

### **3. Smart Pointers**

Cache nodes are managed using `std::shared_ptr` and `std::weak_ptr` to avoid reference cycles in the linked list.

The active cache strategy is owned through:

```cpp
std::unique_ptr<ICache<...>>
```

inside `FileSystem`.

### **4. Full CRUD Operations**

The file system supports:

- `createFile`
- `readFile`
- `updateFile`
- `deleteFile`
- `listFiles`
- `exists`

### **5. Automatic Cache Invalidation**

Every `updateFile` and `deleteFile` operation invalidates the corresponding cache entry immediately.

`updateFile` then repopulates the cache with the fresh value, ensuring reads never return stale data.

### **6. Cache Hit/Miss Statistics**

`FileSystem::stats()` reports:

- Cache hits
- Cache misses
- Hit rate

This allows LRU and LFU behavior to be compared under different access patterns.

## **Project Layout**

```text
inmemory-fs/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── ICache.hpp        # Generic cache interface
│   ├── LRUCache.hpp      # O(1) LRU: doubly-linked list + hash map
│   ├── LFUCache.hpp      # O(1) LFU: frequency-bucketed linked lists + hash maps
│   └── FileSystem.hpp    # File system CRUD API
└── src/
    ├── FileSystem.cpp    # CRUD implementation + cache invalidation
    └── main.cpp          # Interactive CLI demo
```

## **
