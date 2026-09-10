#pragma once
#include <unordered_map>
#include <memory>
#include <optional>
#include "ICache.hpp"

// LRU (Least Recently Used) cache.
//
// Structure: an intrusive doubly-linked list (most-recently-used at head,
// least-recently-used at tail) paired with a hash map from key -> node.
// Every operation only ever touches a constant number of pointers, so
// get/put/remove are all strict O(1).
//
// Node ownership uses shared_ptr for "next" and weak_ptr for "prev" to
// avoid reference cycles while still allowing O(1) unlink from anywhere
// in the list.
template <typename Key, typename Value>
class LRUCache : public ICache<Key, Value> {
private:
    struct Node {
        Key key;
        Value value;
        std::shared_ptr<Node> next;
        std::weak_ptr<Node> prev;
        Node(const Key& k, const Value& v) : key(k), value(v) {}
    };

    size_t capacity_;
    std::unordered_map<Key, std::shared_ptr<Node>> map_;
    std::shared_ptr<Node> head_; // most recently used
    std::shared_ptr<Node> tail_; // least recently used
    size_t hits_ = 0;
    size_t misses_ = 0;

    void detach(const std::shared_ptr<Node>& node) {
        auto prev = node->prev.lock();
        auto next = node->next;
        if (prev) prev->next = next; else head_ = next;
        if (next) next->prev = prev; else tail_ = prev;
        node->next.reset();
        node->prev.reset();
    }

    void attachFront(const std::shared_ptr<Node>& node) {
        node->next = head_;
        node->prev.reset();
        if (head_) head_->prev = node;
        head_ = node;
        if (!tail_) tail_ = node;
    }

    void evictIfNeeded() {
        while (map_.size() > capacity_ && tail_) {
            const Key evictKey = tail_->key;
            auto prev = tail_->prev.lock();
            tail_ = prev;
            if (tail_) tail_->next.reset(); else head_ = nullptr;
            map_.erase(evictKey);
        }
    }

public:
    explicit LRUCache(size_t capacity) : capacity_(capacity == 0 ? 1 : capacity) {}

    std::optional<Value> get(const Key& key) override {
        auto it = map_.find(key);
        if (it == map_.end()) {
            ++misses_;
            return std::nullopt;
        }
        ++hits_;
        auto node = it->second;
        detach(node);
        attachFront(node);
        return node->value;
    }

    void put(const Key& key, const Value& value) override {
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second->value = value;
            detach(it->second);
            attachFront(it->second);
            return;
        }
        auto node = std::make_shared<Node>(key, value);
        map_[key] = node;
        attachFront(node);
        evictIfNeeded();
    }

    void remove(const Key& key) override {
        auto it = map_.find(key);
        if (it == map_.end()) return;
        detach(it->second);
        map_.erase(it);
    }

    bool contains(const Key& key) const override {
        return map_.find(key) != map_.end();
    }

    size_t size() const override { return map_.size(); }

    size_t hits() const { return hits_; }
    size_t misses() const { return misses_; }
};
