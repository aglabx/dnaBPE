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
    uint32_t prev_offset;  // Relative distance to previous node (current - prev_offset)
    uint32_t next_offset;  // Relative distance to next node (current + next_offset)
    
    VectorNode(uint32_t id = 0, uint32_t prev = UINT32_MAX, uint32_t next = UINT32_MAX) 
        : token_id(id), prev_offset(prev), next_offset(next) {}
};

class VectorLinkedList {
private:
    std::vector<VectorNode> nodes;
    uint64_t head_pos;     // Absolute position of head
    uint64_t tail_pos;     // Absolute position of tail
    uint64_t size_;

public:
    static constexpr uint32_t END_MARKER = UINT32_MAX;
    
    VectorLinkedList() : head_pos(0), tail_pos(0), size_(0) {}
    
    void init(size_t capacity) {
        nodes.reserve(capacity);
    }
    
    uint32_t push_front(uint32_t token_id) {
        uint64_t new_pos = nodes.size();
        std::cout << "\nPushing front token_id=" << token_id << " at pos=" << new_pos << std::endl;
        
        if (size_ == 0) {
            std::cout << "First node - initializing head and tail" << std::endl;
            nodes.emplace_back(token_id, END_MARKER, END_MARKER);
            head_pos = tail_pos = new_pos;
        } else {
            std::cout << "Adding node: current head=" << head_pos << std::endl;
            // Push new node and update links
            nodes.emplace_back(token_id, new_pos - tail_pos, END_MARKER);  // Next node is 1 position away
            nodes[head_pos].next_offset = new_pos - tail_pos;  // Previous node is 1 position away
            
            // Update tail position
            tail_pos = new_pos;
            
            std::cout << "Updated links:" << std::endl;
            std::cout << "  New head at " << head_pos << ": prev_offset=" 
                     << nodes[head_pos].prev_offset << ", next_offset=" 
                     << nodes[head_pos].next_offset << std::endl;
            
            uint64_t old_head = head_pos + nodes[head_pos].next_offset;
            if (old_head < nodes.size()) {
                std::cout << "  Old head at " << old_head 
                         << ": prev_offset=" << nodes[old_head].prev_offset 
                         << ", next_offset=" << nodes[old_head].next_offset << std::endl;
            }
        }
        size_++;
        return new_pos;
    }
    
    void append(uint32_t token_id) {
        if (size_ == 0) {
            push_front(token_id);
            return;
        }
        
        uint64_t new_pos = nodes.size();
        nodes.emplace_back(token_id, new_pos - tail_pos, END_MARKER);
        nodes[tail_pos].next_offset = new_pos - tail_pos;
        tail_pos = new_pos;
        size_++;
    }
    
    // Merge two consecutive nodes
    bool merge_nodes(uint64_t first_pos, uint32_t new_token_id) {
        // if (first_pos >= nodes.size() || 
        //     nodes[first_pos].next_offset == END_MARKER) {
        //     return false;
        // }
        
        uint64_t second_pos = first_pos + nodes[first_pos].next_offset;
        uint64_t next_pos = (nodes[second_pos].next_offset == END_MARKER) ? 
                           END_MARKER : second_pos + nodes[second_pos].next_offset;
        
        // Update token and links
        nodes[first_pos].token_id = new_token_id;
        if (next_pos != END_MARKER) {
            nodes[first_pos].next_offset = next_pos - first_pos;
            nodes[next_pos].prev_offset = next_pos - first_pos;
        } else {
            nodes[first_pos].next_offset = END_MARKER;
            tail_pos = first_pos;
        }

        // Clear the merged node
        nodes[second_pos].token_id = 0;
        nodes[second_pos].next_offset = END_MARKER;
        nodes[second_pos].prev_offset = END_MARKER;

        return true;
    }
    
    uint32_t get_head() const { return head_pos; }
    size_t size() const { return size_; }
    const VectorNode& get_node(uint32_t idx) const { return nodes[idx]; }
    VectorNode& get_node(uint32_t idx) { return nodes[idx]; }
    
    // Iterator support
    class Iterator {
    private:
        const VectorLinkedList* list;
        uint64_t current_pos;
        
    public:
        // Add iterator traits
        using iterator_category = std::forward_iterator_tag;
        using value_type = VectorNode;
        using difference_type = std::ptrdiff_t;
        using pointer = const VectorNode*;
        using reference = const VectorNode&;
        
        Iterator(const VectorLinkedList* l, uint64_t start) 
            : list(l), current_pos(start) {}
        
        // Add getter for current position
        uint64_t position() const { return current_pos; }

        bool operator!=(const Iterator& other) const {
            return current_pos != other.current_pos;
        }
        
        Iterator& operator++() {
            if (current_pos != END_MARKER) {
                current_pos = (list->nodes[current_pos].next_offset == END_MARKER) ?
                            END_MARKER : current_pos + list->nodes[current_pos].next_offset;
            }
            return *this;
        }
        
        const VectorNode& operator*() const {
            return list->nodes[current_pos];
        }
    };
    
    Iterator begin() const { return Iterator(this, head_pos); }
    Iterator end() const { return Iterator(this, END_MARKER); }

    // Add new direct push method
    void push_direct(const std::vector<VectorNode>& new_nodes) {
        size_t old_size = nodes.size();
        nodes.insert(nodes.end(), new_nodes.begin(), new_nodes.end());
        
        if (size_ == 0) {
            head_pos = old_size;
            tail_pos = nodes.size() - 1;
        } else {
            nodes[old_size].prev_offset = old_size - tail_pos;
            nodes[tail_pos].next_offset = old_size - tail_pos;
        }
        
        // Update links for new nodes
        for (size_t i = old_size; i < nodes.size() - 1; ++i) {
            nodes[i].next_offset = 1;
            nodes[i + 1].prev_offset = 1;
        }
        
        nodes.back().next_offset = END_MARKER;
        tail_pos = nodes.size() - 1;
        size_ += new_nodes.size();
    }

    // Add methods for getting neighboring tokens
    uint32_t get_prev_token(uint64_t pos) const {
        if (pos >= nodes.size() || nodes[pos].prev_offset == END_MARKER) {
            return TokenizerConstants::SEP_TOKEN_ID;
        }
        return nodes[pos - nodes[pos].prev_offset].token_id;
    }

    uint32_t get_next_token(uint64_t pos) const {
        if (pos >= nodes.size() || nodes[pos].next_offset == END_MARKER) {
            return TokenizerConstants::SEP_TOKEN_ID;
        }
        return nodes[pos + nodes[pos].next_offset].token_id;
    }

    size_t get_next_positions(uint64_t pos) const {
        if (pos >= nodes.size() || nodes[pos].next_offset == END_MARKER) {
            return 0;
        }
        return pos + nodes[pos].next_offset;
    }

    size_t get_prev_positions(uint64_t pos) const {
        if (pos >= nodes.size() || nodes[pos].prev_offset == END_MARKER) {
            return 0;
        }
        return pos - nodes[pos].prev_offset;
    }

    // Reverse iterator support needs to be added
    class ReverseIterator {
    private:
        const VectorLinkedList* list;
        uint64_t current_pos;
        
    public:
        ReverseIterator(const VectorLinkedList* l, uint64_t start) 
            : list(l), current_pos(start) {}
        
        bool operator!=(const ReverseIterator& other) const {
            return current_pos != other.current_pos;
        }
        
        ReverseIterator& operator++() {
            if (current_pos != END_MARKER) {
                uint64_t prev = list->nodes[current_pos].prev_offset;
                current_pos = (prev == END_MARKER) ? END_MARKER : current_pos - prev;
            }
            return *this;
        }
        
        const VectorNode& operator*() const {
            return list->nodes[current_pos];
        }
    };

    ReverseIterator rbegin() const { return ReverseIterator(this, tail_pos); }
    ReverseIterator rend() const { return ReverseIterator(this, END_MARKER); }

    // Additional helper method for debug/validation
    bool validate_links() const {
        std::cout << "\nValidating linked list..." << std::endl;
        std::cout << "List state: size=" << size_ << ", head=" << head_pos << ", tail=" << tail_pos << std::endl;
        std::cout << "Total nodes in vector: " << nodes.size() << std::endl;

        if (size_ == 0) {
            bool valid = (head_pos == 0 && tail_pos == 0);
            std::cout << "Empty list validation: " << (valid ? "PASS" : "FAIL") << std::endl;
            if (!valid) {
                std::cout << "Error: Empty list should have head=0 and tail=0" << std::endl;
            }
            return valid;
        }

        // Print all nodes and check for properly cleared nodes
        std::cout << "\nAll nodes in vector:" << std::endl;
        std::unordered_set<uint64_t> active_positions;
        uint64_t current = head_pos;
        
        // First collect all active positions by traversing the list
        while (current != END_MARKER) {
            active_positions.insert(current);
            current = (nodes[current].next_offset == END_MARKER) ? 
                      END_MARKER : current + nodes[current].next_offset;
        }

        // Check all nodes
        for (size_t i = 0; i < nodes.size(); i++) {
            std::cout << "Node[" << i << "]: token=" << nodes[i].token_id 
                     << ", prev=" << nodes[i].prev_offset 
                     << ", next=" << nodes[i].next_offset;
            
            // Check if node should be cleared
            if (active_positions.find(i) == active_positions.end()) {
                std::cout << " (should be cleared)";
                if (nodes[i].token_id != 0 || 
                    nodes[i].next_offset != END_MARKER || 
                    nodes[i].prev_offset != END_MARKER) {
                    std::cout << " ERROR: Node not properly cleared!" << std::endl;
                    return false;
                }
            }
            std::cout << std::endl;
        }

        uint32_t count = 0;
        current = head_pos;
        std::cout << "\nTraversing list forward..." << std::endl;

        while (current != END_MARKER) {
            std::cout << "\nChecking node at position " << current << ":" << std::endl;
            std::cout << "  token_id: " << nodes[current].token_id << std::endl;
            std::cout << "  prev_offset: " << nodes[current].prev_offset << std::endl;
            std::cout << "  next_offset: " << nodes[current].next_offset << std::endl;

            // Check forward link
            uint64_t next = (nodes[current].next_offset == END_MARKER) ? 
                            END_MARKER : current + nodes[current].next_offset;
            std::cout << "  Next node position: " << (next == END_MARKER ? "END" : std::to_string(next)) << std::endl;
            std::cout << "  Prev node position: " << (current == head_pos ? "HEAD" : std::to_string(current - nodes[current].prev_offset)) << std::endl;
            
            // Check backward link if not at head
            if (current != head_pos) {
                if (nodes[current].prev_offset == 0 || nodes[current].prev_offset == END_MARKER) {
                    std::cout << "ERROR: Invalid prev_offset at position " << current << std::endl;
                    std::cout << "  prev_offset=" << nodes[current].prev_offset << std::endl;
                    return false;
                }
                
                uint64_t prev_pos = current - nodes[current].prev_offset;
                std::cout << "  Checking backward link to " << prev_pos << std::endl;
                
                if (nodes[prev_pos].next_offset != nodes[current].prev_offset) {
                    std::cout << "ERROR: Mismatched bidirectional link at position " << current << std::endl;
                    std::cout << "  Current node prev_offset: " << nodes[current].prev_offset << std::endl;
                    std::cout << "  Previous node next_offset: " << nodes[prev_pos].next_offset << std::endl;
                    return false;
                }
            }
            
            // Check forward link if not at tail
            if (next != END_MARKER) {
                std::cout << "  Validating forward link..." << std::endl;
                if (nodes[next].prev_offset != next - current) {
                    std::cout << "ERROR: Forward link mismatch at position " << current << std::endl;
                    std::cout << "  Expected prev_offset: " << (next - current) << std::endl;
                    std::cout << "  Actual prev_offset: " << nodes[next].prev_offset << std::endl;
                    return false;
                }
                if (nodes[current].next_offset != next - current) {
                    std::cout << "ERROR: Forward link distance mismatch at position " << current << std::endl;
                    std::cout << "  Expected next_offset: " << (next - current) << std::endl;
                    std::cout << "  Actual next_offset: " << nodes[current].next_offset << std::endl;
                    return false;
                }
            }

            current = next;
            count++;
        }

        bool size_valid = (count == size_);
        if (!size_valid) {
            std::cout << "\nERROR: Size mismatch!" << std::endl;
            std::cout << "Node traversal path:" << std::endl;
            current = head_pos;
            while (current != END_MARKER) {
                std::cout << current << " -> ";
                current = (nodes[current].next_offset == END_MARKER) ? 
                          END_MARKER : current + nodes[current].next_offset;
            }
            std::cout << "END" << std::endl;
        }
        std::cout << "\nSize validation: " << (size_valid ? "PASS" : "FAIL") << std::endl;
        std::cout << "  Expected size: " << size_ << std::endl;
        std::cout << "  Actual count: " << count << std::endl;

        std::cout << "\nFinal validation result: " << (size_valid ? "PASS" : "FAIL") << std::endl;
        return size_valid;
    }

    void print() const {
        std::cout << "List size: " << size_ << ", head: " << head_pos << ", tail: " << tail_pos << std::endl;
        std::cout << "Idx\tToken\tPrev\tNext" << std::endl;
        std::cout << "--------------------------------" << std::endl;
        
        uint64_t current = head_pos;
        while (current != END_MARKER) {
            const VectorNode& node = nodes[current];
            std::cout << current << "\t" 
                     << node.token_id << "\t"
                     << (node.prev_offset == END_MARKER ? "END" : std::to_string(current - node.prev_offset)) << "\t"
                     << (node.next_offset == END_MARKER ? "END" : std::to_string(current + node.next_offset)) << std::endl;
            current = (node.next_offset == END_MARKER) ? END_MARKER : current + node.next_offset;
        }
        std::cout << "--------------------------------" << std::endl;
    }
};

#endif // LINKEDVECTOR_HPP