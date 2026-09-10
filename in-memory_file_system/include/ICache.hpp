#pragma once
#include <optional>
#include <cstddef>

// Generic cache contract implemented by both LRUCache and LFUCache.
// Using a common interface lets FileSystem swap eviction strategies
// at construction time without changing any CRUD logic.
template <typename Key, typename Value>
class ICache {
public:
    virtual std::optional<Value> get(const Key& key) = 0;
    virtual void put(const Key& key, const Value& value) = 0;
    virtual void remove(const Key& key) = 0;
    virtual bool contains(const Key& key) const = 0;
    virtual size_t size() const = 0;
    virtual ~ICache() = default;
};
