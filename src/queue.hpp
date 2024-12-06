#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <bitset>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <nlohmann/json.hpp>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <fstream>
#include <unordered_set>
#include "robin_hood.h"

// Add this right after includes, before any other code
namespace std {
    template<>
    struct hash<pair<uint32_t, uint32_t>> {
        using argument_type = pair<uint32_t, uint32_t>;
        using result_type = size_t;
        
        result_type operator()(const argument_type& p) const noexcept {
            return hash<uint64_t>{}((static_cast<uint64_t>(p.first) << 32) | p.second);
        }
    };
}

class PairPriorityQueue {
private:
    struct PairInfo {
        uint32_t left;
        uint32_t right;
        size_t frequency;
        
        PairInfo(uint32_t l, uint32_t r, size_t f) 
            : left(l), right(r), frequency(f) {}
            
        bool operator<(const PairInfo& other) const {
            // Reverse comparison for max heap
            return frequency < other.frequency;
        }
    };
    
    std::priority_queue<PairInfo> heap;
    robin_hood::unordered_flat_map<uint64_t, size_t> pair_frequencies;  // Changed to robin_hood map
    size_t invalid_entries = 0;
    static constexpr size_t REBUILD_THRESHOLD = 1000; // Adjust this value based on your needs
    
    static uint64_t make_key(uint32_t left, uint32_t right) {
        return (static_cast<uint64_t>(left) << 32) | right;
    }
    
    void rebuild_heap() {
        std::priority_queue<PairInfo> new_heap;
        for (const auto& [key, freq] : pair_frequencies) {
            if (freq >= 2) {
                uint32_t left = key >> 32;
                uint32_t right = key & 0xFFFFFFFF;
                new_heap.emplace(left, right, freq);
            }
        }
        heap = std::move(new_heap);
    }

    void rebuild_heap_if_needed() {
        if (invalid_entries >= REBUILD_THRESHOLD && !heap.empty()) {
            rebuild_heap();
            invalid_entries = 0;
        }
    }

public:
    PairPriorityQueue() {
        // Pre-allocate space for common case
        pair_frequencies.reserve(10000);
    }

    void add_pair(uint32_t left, uint32_t right, size_t initial_freq = 1) {
        uint64_t key = make_key(left, right);
        pair_frequencies[key] += initial_freq;
        if (pair_frequencies[key] >= 2) {
            heap.emplace(left, right, pair_frequencies[key]);
        }
    }
    
    void increase_frequency(uint32_t left, uint32_t right) {
        uint64_t key = make_key(left, right);
        size_t new_freq = ++pair_frequencies[key];
        if (new_freq == 2) {
            heap.emplace(left, right, new_freq);
        }
    }
    
    void decrease_frequency(uint32_t left, uint32_t right) {
        uint64_t key = make_key(left, right);
        if (pair_frequencies[key] > 0) {
            if (--pair_frequencies[key] < 2) {
                invalid_entries++;
                rebuild_heap_if_needed();
            }
        }
    }
    
    bool get_most_frequent(uint32_t& left, uint32_t& right, size_t& freq) {
        while (!heap.empty()) {
            const PairInfo& top = heap.top();
            uint64_t key = make_key(top.left, top.right);
            
            // Check if the frequency is still valid
            if (pair_frequencies[key] == top.frequency && pair_frequencies[key] >= 2) {
                left = top.left;
                right = top.right;
                freq = top.frequency;
                return true;
            }
            
            heap.pop();  // Remove outdated entry
            invalid_entries--;  // Removed an invalid entry
        }
        return false;
    }
    
    void clear() {
        while (!heap.empty()) heap.pop();
        pair_frequencies.clear();
        invalid_entries = 0;
    }
    
    bool empty() const {
        return heap.empty();
    }
    
    size_t get_frequency(uint32_t left, uint32_t right) const {
        uint64_t key = make_key(left, right);
        return pair_frequencies.at(key);
    }
};

#endif // QUEUE_HPP