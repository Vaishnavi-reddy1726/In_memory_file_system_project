#pragma once
#include <unordered_map>
#include <memory>
#include <optional>
#include "ICache.hpp"

// LFU (Least Frequently Used) cache, O(1) get/put/remove.
//
// Structure: keyMap_ maps key -> node (value + access frequency).
// freqMap_ maps a frequency count -> a doubly-linked list of all nodes
// currently at that frequency (most-recently-touched at the head, so ties
// within a frequency break LRU-style). minFreq_ tracks the smallest
// frequency bucket so eviction always pops that bucket's tail in O(1).
template <typename Key, typename Value>
class LFUCache : public ICache<Key, Value> {
private:
    struct Node {
        Key key;
        Value value;
        int freq = 1;
        std::shared_ptr<Node> next;
        std::weak_ptr<Node> prev;
        Node(const Key& k, const Value& v) : key(k), value(v) {}
    };

    struct FreqList {
        std::shared_ptr<Node> head;
        std::shared_ptr<Node> tail;
        size_t count = 0;

        void pushFront(const std::shared_ptr<Node>& node) {
            node->next = head;
            node->prev.reset();
            if (head) head->prev = node;
            head = node;
            if (!tail) tail = node;
            ++count;
        }

        void remove(const std::shared_ptr<Node>& node) {
            auto prev = node->prev.lock();
            auto next = node->next;
            if (prev) prev->next = next; else head = next;
            if (next) next->prev = prev; else tail = prev;
            node->next.reset();
            node->prev.reset();
            --count;
        }

        bool empty() const { return count == 0; }
    };

    size_t capacity_;
    size_t minFreq_ = 0;
    std::unordered_map<Key, std::shared_ptr<Node>> keyMap_;
    std::unordered_map<int, std::shared_ptr<FreqList>> freqMap_;
    size_t hits_ = 0;
    size_t misses_ = 0;

    void bumpFrequency(const std::shared_ptr<Node>& node) {
        const int oldFreq = node->freq;
        auto oldList = freqMap_[oldFreq];
        oldList->remove(node);
        if (oldList->empty()) {
            freqMap_.erase(oldFreq);
            if (minFreq_ == static_cast<size_t>(oldFreq)) ++minFreq_;
        }
        node->freq++;
        auto& newList = freqMap_[node->freq];
        if (!newList) newList = std::make_shared<FreqList>();
        newList->pushFront(node);
    }

public:
    explicit LFUCache(size_t capacity) : capacity_(capacity == 0 ? 1 : capacity) {}

    std::optional<Value> get(const Key& key) override {
        auto it = keyMap_.find(key);
        if (it == keyMap_.end()) {
            ++misses_;
            return std::nullopt;
        }
        ++hits_;
        bumpFrequency(it->second);
        return it->second->value;
    }

    void put(const Key& key, const Value& value) override {
        auto it = keyMap_.find(key);
        if (it != keyMap_.end()) {
            it->second->value = value;
            bumpFrequency(it->second);
            return;
        }

        if (keyMap_.size() >= capacity_) {
            auto minList = freqMap_[static_cast<int>(minFreq_)];
            auto victim = minList->tail;
            minList->remove(victim);
            if (minList->empty()) freqMap_.erase(static_cast<int>(minFreq_));
            keyMap_.erase(victim->key);
        }

        auto node = std::make_shared<Node>(key, value);
        keyMap_[key] = node;
        minFreq_ = 1;
        auto& list1 = freqMap_[1];
        if (!list1) list1 = std::make_shared<FreqList>();
        list1->pushFront(node);
    }

    void remove(const Key& key) override {
        auto it = keyMap_.find(key);
        if (it == keyMap_.end()) return;
        auto node = it->second;
        auto list = freqMap_[node->freq];
        list->remove(node);
        if (list->empty()) freqMap_.erase(node->freq);
        keyMap_.erase(it);
    }

    bool contains(const Key& key) const override {
        return keyMap_.find(key) != keyMap_.end();
    }

    size_t size() const override { return keyMap_.size(); }

    size_t hits() const { return hits_; }
    size_t misses() const { return misses_; }
};
