#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>
#include "ICache.hpp"

enum class CachePolicy { LRU, LFU };

struct CacheStats {
    size_t hits = 0;
    size_t misses = 0;
    double hitRate() const {
        const size_t total = hits + misses;
        return total == 0 ? 0.0 : (static_cast<double>(hits) / total) * 100.0;
    }
};

// In-memory file system.
//
// storage_ is the authoritative "disk" (every file that exists lives here).
// cache_ is a bounded LRU or LFU front-end over storage_ that speeds up
// repeated reads. Every mutating operation (update/delete) invalidates the
// affected cache entry immediately so the cache can never serve stale data.
class FileSystem {
public:
    FileSystem(CachePolicy policy, size_t cacheCapacity);

    // --- CRUD ---
    bool createFile(const std::string& path, const std::string& content);
    std::optional<std::string> readFile(const std::string& path);
    bool updateFile(const std::string& path, const std::string& content);
    bool deleteFile(const std::string& path);

    bool exists(const std::string& path) const;
    std::vector<std::string> listFiles() const;

    CacheStats stats() const;
    std::string policyName() const;
    size_t diskFileCount() const;
    size_t cacheSize() const;

private:
    CachePolicy policy_;
    std::unordered_map<std::string, std::string> storage_;
    std::unique_ptr<ICache<std::string, std::string>> cache_;
};
