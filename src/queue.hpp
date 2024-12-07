#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <iostream>
#include <vector>
#include <unordered_map>
#include <set>
#include <fstream>
#include <bitset>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <nlohmann/json.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <unordered_set>
#include "robin_hood.h"

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
    // Используем pair<size_t, uint64_t> где:
    // - first это frequency (для сортировки)
    // - second это упакованный ключ (left << 32 | right)
    struct PairCompare {
        bool operator()(const std::pair<size_t, uint64_t>& a, 
                       const std::pair<size_t, uint64_t>& b) const {
            return a.first > b.first || (a.first == b.first && a.second < b.second);
        }
    };
    
    std::multiset<std::pair<size_t, uint64_t>, PairCompare> heap;
    robin_hood::unordered_flat_map<uint64_t, size_t> pair_frequencies;
    robin_hood::unordered_flat_map<uint64_t, std::set<size_t>> pair_positions;
    
    static uint64_t make_key(uint32_t left, uint32_t right) {
        return (static_cast<uint64_t>(left) << 32) | right;
    }

    uint32_t extract_left(uint64_t key) const {
        return static_cast<uint32_t>(key >> 32);
    }

    uint32_t extract_right(uint64_t key) const {
        return static_cast<uint32_t>(key & 0xFFFFFFFF);
    }
    
    void rebuild_heap() {
        heap.clear();
        for (const auto& [key, freq] : pair_frequencies) {
            if (freq >= 2) {
                heap.emplace(freq, key);
            }
        }
    }

public:
    PairPriorityQueue() {
        pair_frequencies.reserve(10000);
        pair_positions.reserve(10000);
    }

    void add_pair(uint32_t left, uint32_t right, size_t initial_freq = 1) {
        uint64_t key = make_key(left, right);
        pair_frequencies[key] += initial_freq;
        if (pair_frequencies[key] >= 2) {
            heap.emplace(pair_frequencies[key], key);
        }
    }
    
    void increase_frequency(uint32_t left, uint32_t right, size_t delta) {
        uint64_t key = make_key(left, right);
        size_t old_freq = pair_frequencies[key];
        size_t new_freq = old_freq + delta;
        pair_frequencies[key] = new_freq;
        
        if (old_freq >= 2) {
            // Удаляем старую запись
            heap.erase(heap.find({old_freq, key}));
        }
        if (new_freq >= 2) {
            // Добавляем новую запись
            heap.emplace(new_freq, key);
        }
    }
    
    void decrease_frequency(uint32_t left, uint32_t right, size_t delta) {
        uint64_t key = make_key(left, right);
        if (pair_frequencies[key] >= delta) {
            size_t old_freq = pair_frequencies[key];
            size_t new_freq = old_freq - delta;
            
            if (new_freq == 0) {
                pair_frequencies.erase(key);
            } else {
                pair_frequencies[key] = new_freq;
            }
            
            if (old_freq >= 2) {
                // Удаляем старую запись
                heap.erase(heap.find({old_freq, key}));
            }
            if (new_freq >= 2) {
                // Добавляем новую запись
                heap.emplace(new_freq, key);
            }
        }
    }

    // New method to cleanup zero frequency pairs
    void cleanup_zero_pairs() {
        for (auto it = pair_frequencies.begin(); it != pair_frequencies.end();) {
            if (it->second == 0) {
                it = pair_frequencies.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    bool get_most_frequent(uint32_t& left, uint32_t& right, size_t& freq) {
        if (heap.empty()) {
            return false;
        }
        
        auto it = heap.begin();
        auto [top_freq, key] = *it;
        
        // Проверяем, что частота все еще валидна
        if (pair_frequencies[key] == top_freq && pair_frequencies[key] >= 2) {
            left = extract_left(key);
            right = extract_right(key);
            freq = top_freq;
            return true;
        }
        
        // Если частота не валидна, удаляем запись и пробуем следующую
        heap.erase(it);
        return get_most_frequent(left, right, freq);
    }
    
    void clear() {
        heap.clear();
        pair_frequencies.clear();
        pair_positions.clear();
    }
    
    bool empty() const {
        return heap.empty();
    }
    
    size_t get_frequency(uint32_t left, uint32_t right) const {
        uint64_t key = make_key(left, right);
        return pair_frequencies.at(key);
    }

    void print_heap() const {
        std::cout << "Heap contents:\n";
        for (const auto& [freq, key] : heap) {
            uint32_t left = extract_left(key);
            uint32_t right = extract_right(key);
            std::cout << "(" << left << "," << right << ") -> " << freq << "\n";
        }
    }

    size_t print_frequencies() const {
        size_t total_pairs = 0;
        std::cout << "Frequency map contents:\n";
        for (const auto& [key, freq] : pair_frequencies) {
            uint32_t left = extract_left(key);
            uint32_t right = extract_right(key);
            std::cout << "(" << left << "," << right << ") -> " << freq << "\n";
            if (left == 4 || right == 4) {
                continue;
            }
            total_pairs += freq;
        }
        return total_pairs;
    }

    void add_pair_position(uint32_t left, uint32_t right, size_t position) {
        uint64_t key = make_key(left, right);
        pair_positions[key].insert(position);
    }

    void remove_pair_position(uint32_t left, uint32_t right, size_t position) {
        uint64_t key = make_key(left, right);
        auto it = pair_positions.find(key);
        if (it != pair_positions.end()) {
            it->second.erase(position);
            if (it->second.empty()) {
                pair_positions.erase(it);
            }
        }
    }

    const std::set<size_t>& get_pair_positions(uint32_t left, uint32_t right) const {
        uint64_t key = make_key(left, right);
        static const std::set<size_t> empty_set;
        auto it = pair_positions.find(key);
        return it != pair_positions.end() ? it->second : empty_set;
    }

    // Добавим метод для отладки позиций
    void print_positions() const {
        std::cout << "Pair positions:\n";
        for (const auto& [key, positions] : pair_positions) {
            uint32_t left = extract_left(key);
            uint32_t right = extract_right(key);
            std::cout << "(" << left << "," << right << ") at positions: ";
            for (size_t pos : positions) {
                std::cout << pos << " ";
            }
            std::cout << "\n";
        }
    }
};

#endif // QUEUE_HPP