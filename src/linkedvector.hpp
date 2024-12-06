#ifndef LINKEDVECTOR_HPP
#define LINKEDVECTOR_HPP

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
#include "global.hpp"

// Add new structures at the top of the file
struct VectorNode {
    uint32_t token_id;
    uint32_t next_idx;
    uint32_t prev_idx;  // Add prev_idx for doubly linked list
    
    VectorNode(uint32_t id = 0, uint32_t next = UINT32_MAX, uint32_t prev = UINT32_MAX) 
        : token_id(id), next_idx(next), prev_idx(prev) {}
};

class VectorLinkedList {
private:
    std::vector<VectorNode> nodes;
    uint32_t head_idx;
    uint32_t tail_idx;  // Add tail_idx for faster operations
    uint32_t size_;

public:
    static constexpr uint32_t END_MARKER = UINT32_MAX;
    
    VectorLinkedList() : head_idx(END_MARKER), tail_idx(END_MARKER), size_(0) {}
    
    void init(size_t capacity) {
        nodes.reserve(capacity);
    }
    
    uint32_t push_front(uint32_t token_id) {
        uint32_t new_idx = nodes.size();
        if (head_idx == END_MARKER) {
            nodes.emplace_back(token_id, END_MARKER, END_MARKER);
            head_idx = tail_idx = new_idx;
        } else {
            nodes.emplace_back(token_id, head_idx, END_MARKER);
            nodes[head_idx].prev_idx = new_idx;
            head_idx = new_idx;
        }
        size_++;
        return new_idx;
    }
    
    void append(uint32_t token_id) {
        if (head_idx == END_MARKER) {
            push_front(token_id);
            return;
        }
        
        uint32_t new_idx = nodes.size();
        nodes.emplace_back(token_id, END_MARKER, tail_idx);
        nodes[tail_idx].next_idx = new_idx;
        tail_idx = new_idx;
        size_++;
    }
    
    // Merge two consecutive nodes
    bool merge_nodes(uint32_t first_idx, uint32_t new_token_id) {
        if (first_idx >= nodes.size() || 
            nodes[first_idx].next_idx == END_MARKER ||
            nodes[first_idx].next_idx >= nodes.size()) {
            return false;
        }
        
        uint32_t second_idx = nodes[first_idx].next_idx;
        uint32_t next_idx = nodes[second_idx].next_idx;
        uint32_t prev_idx = nodes[first_idx].prev_idx;  // Store prev_idx of first node

        // Update token and links
        nodes[first_idx].token_id = new_token_id;
        nodes[first_idx].next_idx = next_idx;
        
        // Update next node's prev link
        if (next_idx != END_MARKER) {
            nodes[next_idx].prev_idx = first_idx;
        } else {
            tail_idx = first_idx;
        }

        // Update prev node's next link if it exists
        if (prev_idx != END_MARKER) {
            nodes[prev_idx].next_idx = first_idx;
        } else {
            head_idx = first_idx;
        }
        
        size_--;
        return true;
    }
    
    uint32_t get_head() const { return head_idx; }
    size_t size() const { return size_; }
    const VectorNode& get_node(uint32_t idx) const { return nodes[idx]; }
    VectorNode& get_node(uint32_t idx) { return nodes[idx]; }
    
    // Iterator support
    class Iterator {
    private:
        const VectorLinkedList* list;
        uint32_t current_idx;
        
    public:
        Iterator(const VectorLinkedList* l, uint32_t start) 
            : list(l), current_idx(start) {}
        
        bool operator!=(const Iterator& other) const {
            return current_idx != other.current_idx;
        }
        
        Iterator& operator++() {
            if (current_idx != END_MARKER) {
                current_idx = list->nodes[current_idx].next_idx;
            }
            return *this;
        }
        
        const VectorNode& operator*() const {
            return list->nodes[current_idx];
        }
    };
    
    Iterator begin() const { return Iterator(this, head_idx); }
    Iterator end() const { return Iterator(this, END_MARKER); }

    // Add new direct push method
    void push_direct(const std::vector<VectorNode>& new_nodes) {
        size_t old_size = nodes.size();
        nodes.insert(nodes.end(), new_nodes.begin(), new_nodes.end());
        
        if (head_idx == END_MARKER) {
            head_idx = old_size;
            tail_idx = nodes.size() - 1;
        } else {
            nodes[old_size].prev_idx = tail_idx;
            nodes[tail_idx].next_idx = old_size;
        }
        
        // Update links for new nodes
        for (size_t i = old_size; i < nodes.size() - 1; ++i) {
            nodes[i].next_idx = i + 1;
            nodes[i + 1].prev_idx = i;
        }
        
        nodes.back().next_idx = END_MARKER;
        tail_idx = nodes.size() - 1;
        size_ += new_nodes.size();
    }

    // Add methods for getting neighboring tokens
    uint32_t get_prev_token(uint32_t idx) const {
        if (idx >= nodes.size() || nodes[idx].prev_idx == END_MARKER) {
            return TokenizerConstants::SEP_TOKEN_ID;
        }
        return nodes[nodes[idx].prev_idx].token_id;
    }

    uint32_t get_next_token(uint32_t idx) const {
        if (idx >= nodes.size() || nodes[idx].next_idx == END_MARKER) {
            return TokenizerConstants::SEP_TOKEN_ID;
        }
        return nodes[nodes[idx].next_idx].token_id;
    }

    // Reverse iterator support needs to be added
    class ReverseIterator {
    private:
        const VectorLinkedList* list;
        uint32_t current_idx;
        
    public:
        ReverseIterator(const VectorLinkedList* l, uint32_t start) 
            : list(l), current_idx(start) {}
        
        bool operator!=(const ReverseIterator& other) const {
            return current_idx != other.current_idx;
        }
        
        ReverseIterator& operator++() {
            if (current_idx != END_MARKER) {
                current_idx = list->nodes[current_idx].prev_idx;
            }
            return *this;
        }
        
        const VectorNode& operator*() const {
            return list->nodes[current_idx];
        }
    };

    ReverseIterator rbegin() const { return ReverseIterator(this, tail_idx); }
    ReverseIterator rend() const { return ReverseIterator(this, END_MARKER); }

    // Additional helper method for debug/validation
    bool validate_links() const {
        if (size_ == 0) {
            return head_idx == END_MARKER && tail_idx == END_MARKER;
        }

        // Check forward links
        uint32_t count = 0;
        uint32_t current = head_idx;
        uint32_t prev = END_MARKER;

        while (current != END_MARKER) {
            if (nodes[current].prev_idx != prev) return false;
            prev = current;
            current = nodes[current].next_idx;
            count++;
        }

        // Verify size and tail
        return count == size_ && prev == tail_idx;
    }
};

#endif // LINKEDVECTOR_HPP