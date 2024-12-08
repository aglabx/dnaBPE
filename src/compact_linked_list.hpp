#ifndef COMPACT_LINKED_LIST_HPP
#define COMPACT_LINKED_LIST_HPP

#include <vector>
#include <cstdint>
#include <stdexcept>
#include <iostream>

class CompactLinkedList {
private:
    std::vector<uint32_t> tokens;
    std::vector<int64_t> prev;
    std::vector<int64_t> next;
    int64_t head;
    int64_t tail;
    size_t size_;
    size_t capacity;

public:
    CompactLinkedList() : head(-1), tail(-1), size_(0) {}

    void init(size_t capacity) {
        tokens.reserve(capacity);
        prev.reserve(capacity);
        next.reserve(capacity);
        this->capacity = capacity;
    }

    void append(uint32_t token_id) {
        int64_t pos = (int64_t)tokens.size();
        tokens.push_back(token_id);
        prev.push_back(tail);
        next.push_back(-1);

        if (head == -1) {
            // First element
            head = pos;
        } else {
            next[tail] = pos;
        }
        tail = pos;
        size_++;
        capacity++;
    }

    // Merge two consecutive tokens at position `first_pos` and `second_pos` into one token `new_token_id`
    // After merge, the second node is effectively removed from the chain.
    bool merge_nodes(int64_t first_pos, uint32_t new_token_id) {
        if (first_pos < 0 || (size_t)first_pos >= tokens.size()) {
            return false;
        }
        int64_t second_pos = next[first_pos];
        if (second_pos == -1) {
            // No next node to merge with
            return false;
        }

        // Set the merged token ID
        tokens[first_pos] = new_token_id;

        int64_t next_pos = next[second_pos];

        // Link `first_pos` directly to `next_pos`
        next[first_pos] = next_pos;
        if (next_pos != -1) {
            prev[next_pos] = first_pos;
        } else {
            // second_pos was tail
            tail = first_pos;
        }

        // Mark the merged out node as removed if needed
        // (Not strictly necessary unless we need to ensure `tokens[second_pos]` is cleared)
        tokens[second_pos] = 0; 
        prev[second_pos] = -1;
        next[second_pos] = -1;

        size_--;
        return true;
    }

    void push_direct(const std::vector<uint32_t>& new_tokens) {
        size_t old_size = tokens.size();
        size_t add_size = new_tokens.size();
        
        // Reserve space and add new tokens
        tokens.insert(tokens.end(), new_tokens.begin(), new_tokens.end());
        prev.resize(old_size + add_size);
        next.resize(old_size + add_size);
        
        if (head == -1) {
            // First elements in empty list
            head = old_size;
            tail = old_size + add_size - 1;
            
            // Set up links for new nodes
            for (size_t i = old_size; i < old_size + add_size - 1; ++i) {
                prev[i] = i - 1;
                next[i] = i + 1;
            }
            // Handle first and last nodes specially
            prev[old_size] = -1;
            next[tail] = -1;
            prev[tail] = tail - 1;
        } else {
            // Link old tail to new nodes
            next[tail] = old_size;
            prev[old_size] = tail;
            
            // Set up links for new nodes
            for (size_t i = old_size; i < old_size + add_size - 1; ++i) {
                prev[i] = i - 1;
                next[i] = i + 1;
            }
            // Update last node and tail
            prev[old_size + add_size - 1] = old_size + add_size - 2;
            next[old_size + add_size - 1] = -1;
            tail = old_size + add_size - 1;
        }
        
        size_ += add_size;
        capacity += add_size;
    }

    int64_t get_head() const { return head; }
    size_t size() const { return size_; }

    const uint32_t& get_token(int64_t idx) const {
        return tokens[idx];
    }

    // Forward iteration
    class Iterator {
    private:
        const CompactLinkedList* list;
        int64_t current;
    public:
        Iterator(const CompactLinkedList* l, int64_t start) 
            : list(l), current(start) {}

        bool operator!=(const Iterator& other) const {
            return current != other.current;
        }

        Iterator& operator++() {
            if (current != -1) {
                current = list->next[current];
            }
            return *this;
        }

        const uint32_t& operator*() const {
            return list->tokens[current];
        }
    };

    Iterator begin() const { return Iterator(this, head); }
    Iterator end() const { return Iterator(this, -1); }

    int64_t get_next_pos(int64_t pos) const {
        return next[pos];
    }

    int64_t get_prev_pos(int64_t pos) const {
        return prev[pos];
    }

    uint32_t get_prev_token(int64_t pos) const {
        int64_t p = prev[pos];
        return (p == -1) ? (uint32_t)-1 : tokens[p];
    }

    uint32_t get_next_token(int64_t pos) const {
        int64_t n = next[pos];
        return (n == -1) ? (uint32_t)-1 : tokens[n];
    }

    void print_debug() const {
        std::cout << "List size: " << size_ << ", head: " << head << ", tail: " << tail << "\n";
        int64_t cur = head;
        while (cur != -1) {
            std::cout << "Pos: " << cur << ", token: " << tokens[cur]
                      << ", prev: " << prev[cur] 
                      << ", next: " << next[cur] << "\n";
            cur = next[cur];
        }
    }

    size_t get_capacity() const { return capacity; }

};

#endif // COMPACT_LINKED_LIST_HPP
