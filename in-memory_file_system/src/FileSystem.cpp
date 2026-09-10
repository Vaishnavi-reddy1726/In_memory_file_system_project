#include "FileSystem.hpp"
#include "LRUCache.hpp"
#include "LFUCache.hpp"

namespace {
std::unique_ptr<ICache<std::string, std::string>> makeCache(CachePolicy policy, size_t capacity) {
    if (policy == CachePolicy::LRU) {
        return std::make_unique<LRUCache<std::string, std::string>>(capacity);
    }
    return std::make_unique<LFUCache<std::string, std::string>>(capacity);
}
} // namespace

FileSystem::FileSystem(CachePolicy policy, size_t cacheCapacity)
    : policy_(policy), cache_(makeCache(policy, cacheCapacity)) {}

bool FileSystem::createFile(const std::string& path, const std::string& content) {
    if (storage_.find(path) != storage_.end()) return false; // already exists
    storage_[path] = content;
    cache_->put(path, content); // warm the cache on create
    return true;
}

std::optional<std::string> FileSystem::readFile(const std::string& path) {
    if (auto cached = cache_->get(path)) {
        return cached; // cache hit
    }
    auto it = storage_.find(path);
    if (it == storage_.end()) return std::nullopt; // doesn't exist at all

    cache_->put(path, it->second); // cache miss -> populate from disk
    return it->second;
}

bool FileSystem::updateFile(const std::string& path, const std::string& content) {
    auto it = storage_.find(path);
    if (it == storage_.end()) return false;

    cache_->remove(path);       // invalidate the stale cached value
    it->second = content;       // write-through to the backing store
    cache_->put(path, content); // repopulate cache with the fresh value
    return true;
}

bool FileSystem::deleteFile(const std::string& path) {
    auto it = storage_.find(path);
    if (it == storage_.end()) return false;

    cache_->remove(path); // invalidate on delete
    storage_.erase(it);
    return true;
}

bool FileSystem::exists(const std::string& path) const {
    return storage_.find(path) != storage_.end();
}

std::vector<std::string> FileSystem::listFiles() const {
    std::vector<std::string> files;
    files.reserve(storage_.size());
    for (const auto& entry : storage_) files.push_back(entry.first);
    return files;
}

CacheStats FileSystem::stats() const {
    CacheStats s;
    if (policy_ == CachePolicy::LRU) {
        auto* lru = dynamic_cast<LRUCache<std::string, std::string>*>(cache_.get());
        if (lru) { s.hits = lru->hits(); s.misses = lru->misses(); }
    } else {
        auto* lfu = dynamic_cast<LFUCache<std::string, std::string>*>(cache_.get());
        if (lfu) { s.hits = lfu->hits(); s.misses = lfu->misses(); }
    }
    return s;
}

std::string FileSystem::policyName() const {
    return policy_ == CachePolicy::LRU ? "LRU" : "LFU";
}

size_t FileSystem::diskFileCount() const { return storage_.size(); }
size_t FileSystem::cacheSize() const { return cache_->size(); }
