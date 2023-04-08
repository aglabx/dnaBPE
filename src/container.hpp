#ifndef CONTAINER_FILE_H
#define CONTAINER_FILE_H


#include <tuple>
#include <vector>
#include "tokens.hpp"
#include <unordered_map>
#include <iostream>
#include <unordered_set>


typedef std::unordered_map<Kmer, size_t> Counter;


class Node {
public:
    size_t index;
    Kmer pair;
    bool help_token;
    Node* prev;
    Node* next;
    

    void update(Kmer pair, bool help_token) {
        this->pair = pair;
        this->help_token = help_token;
    }
};


class SequenceContainer {
public:

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Node;
        using pointer = Node*;
        using reference = Node&;

        iterator(Node* node) : current_node(node) {}

        reference operator*() const { return *current_node; }
        pointer operator->() const { return current_node; }

        iterator& operator++() {
            current_node = current_node->next;
            return *this;
        }

        iterator operator++(int) {
            iterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const iterator& other) const { return current_node == other.current_node; }
        bool operator!=(const iterator& other) const { return !(*this == other); }

    private:
        Node* current_node;
    };

    SequenceContainer() : head(nullptr), tail(nullptr) {}

    SequenceContainer(const std::vector<TokenType>& seq) : head(nullptr), tail(nullptr), size_(0) {
        for (size_t i = 0; i < seq.size() - 1; i++) {

            if (i && i % 1000000 == 0) {
                std::cout << "Processed " << i << " tokens from " << seq.size() << std::endl;
            }

            TokenType a = seq[i];
            TokenType b = seq[i + 1];
            Kmer pair = std::make_tuple(a, b);
            bool help_token = false;
            if (a <= N_HELP_TOKENS || b <= N_HELP_TOKENS) {
                help_token = true;
            }
            append(i, pair, help_token);
        }
    }

    void append(size_t index, Kmer kmer, bool help_token) {

        Node* newNode = new Node{index, kmer, help_token, nullptr, nullptr};

        if (!head) {
            head = tail = newNode;
        } else {
            newNode->prev = tail;
            tail->next = newNode;
            tail = newNode;
        }
        if (!help_token) {
            counter[kmer]++;
            // update positions
            if (!positions.contains(kmer)) {
                positions[kmer] = std::unordered_set<size_t>();
            }
            positions[kmer].insert(index);
        }
        indexMap[index] = newNode;
        size_++;
    }

    ~SequenceContainer() {
        while (head) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
    }

    Node* operator[](size_t index) {
        if (index < size_) {
            return indexMap[index];
        }
        return nullptr;
    }

    void display(std::unordered_map<TokenType, std::string>& alphabet_map) {
        Node* temp = head;

        while (temp) {
            std::cout << alphabet_map.at(std::get<0>(temp->pair)) << "|" << alphabet_map.at(std::get<1>(temp->pair)) << " ";
            temp = temp->next;
        }
        std::cout << std::endl;
    }

    bool removeAtIndex(size_t index) {
        
        Node* currentNode = indexMap[index];

        if (currentNode->prev == nullptr) {
            head = currentNode->next;
        } else {
            currentNode->prev->next = currentNode->next;
        }

        if (currentNode->next == nullptr) {
            tail = currentNode->prev;
        } else {
            currentNode->next->prev = currentNode->prev;
        }

        // Update indexVector
        indexMap.erase(index);

        // Update counter and positions
        counter[currentNode->pair]--;
        if (counter[currentNode->pair] == 0) {
            counter.erase(currentNode->pair);
        }
        positions.at(currentNode->pair).erase(index);
        size_--;

        delete currentNode;
        return true;
    }


    void collapse(Kmer rep, size_t index, TokenType L) {
        
        //  A   T   A   C   T
        // A|T T|A A|C C|T T|G
        // AC 
        // A|T T|AC -- AC|T T|G
        //  A   T       AC   T

        // A|~ ~|A A|C C|T T|G


        // A|T T|A A|C C|~ ~|G

        //  T    A    C   A   C   T
        // T|A  A|C  C|A A|C C|T T|... 
        //  T    AC   A   C   T
        // T|AC AC|A A|C A|T T|...

        // 5
        // // T|T T|T T|T T|T T|T T|C
        // T|T - 3 = 2
        // // T|TT TT|T T|T T|T T|C
        // T|TT 1
        // TT|T 1
        // T|T 2
        // // T|TT TT|TT  TT|T T|C
        // T|TT 1 
        // TT|T 1 - 1 + 1
        // T|T 2 -1 -1
        // TT|TT + 1

        Node* currentNode = indexMap[index];

        if (currentNode->pair != rep) {
            // std::cout << "Error: collapse called on wrong node" << std::endl;
            // print counter value
            return;
        }

        Node* prevNode = currentNode->prev;

        if (prevNode != nullptr) {
            
            Kmer prev_kmer = prevNode->pair;
            Kmer left_kmer = std::make_tuple(std::get<0>(prevNode->pair), L);
                 
            if (!prevNode->help_token) {
                
                // std::cout << "Decreasing counter for prev_kmer " << std::get<0>(prev_kmer) << " " << std::get<1>(prev_kmer) << std::endl;

                counter[prev_kmer]--;
                if (counter[prev_kmer] == 0) {
                    counter.erase(prev_kmer);
                }
                positions[prev_kmer].erase(prevNode->index);

                counter[left_kmer]++;
                positions[left_kmer].insert(prevNode->index);    
            }
            

            prevNode->pair = left_kmer;
        }

        Node* nextNode = currentNode->next;

        if (nextNode != nullptr) {
            Kmer next_kmer = nextNode->pair;
            Kmer right_kmer = std::make_tuple(L, std::get<1>(nextNode->pair));

            if (!nextNode->help_token) {

                // std::cout << "Decreasing counter for next_kmer " << std::get<0>(next_kmer) << " " << std::get<1>(next_kmer) << std::endl;


                counter[next_kmer]--;
                if (counter[next_kmer] == 0) {
                    counter.erase(next_kmer);
                }
                positions[next_kmer].erase(nextNode->index);

                counter[right_kmer]++;
                positions[right_kmer].insert(nextNode->index);
            }

            nextNode->pair = right_kmer;

        }

        // Remove the current node
        // std::cout << "Removing node at index: " << index << std::endl;
        removeAtIndex(index);
        // std::cout << "Counter size for rep is " << counter[rep] << std::endl;
        
    }

    std::pair<Kmer, size_t> get_most_frequent_pair() {
        if (counter.empty()) {
            throw std::runtime_error("The container is empty, cannot find the most frequent pair.");
        }
        Kmer most_frequent_pair;
        size_t max_count = 0;
        for (const auto& entry : counter) {
            if (entry.second > max_count) {
                max_count = entry.second;
                most_frequent_pair = entry.first;
            }
        }
        return std::pair(most_frequent_pair, max_count);
    }

    size_t size() {
        return size_;
    }

    std::vector<size_t> get_positions(Kmer rep) {
        const std::unordered_set<size_t>& position_set = positions.at(rep);
        std::vector<size_t> position_vector(position_set.begin(), position_set.end());  
        std::sort(position_vector.begin(), position_vector.end());
        return position_vector;
    }

    // print counter
    void print_counter(std::unordered_map<TokenType, std::string>& alphabet_map) {
        for (const auto& entry : counter) {
            std::cout << alphabet_map.at(std::get<0>(entry.first)) << " " << alphabet_map.at(std::get<1>(entry.first)) << " " << entry.second << std::endl;
        }
        // print get_most_frequent_pair
        std::cout << "Most frequent pair: " << alphabet_map.at(std::get<0>(get_most_frequent_pair().first)) << " " << alphabet_map.at(std::get<1>(get_most_frequent_pair().first)) << " " << get_most_frequent_pair().second << std::endl;
    }


    iterator begin() const { return iterator(head); }
    iterator end() const { return iterator(nullptr); }

    std::vector<TokenType> get_as_vector() {
        std::vector<TokenType> token_vector;
        token_vector.reserve(size_);

        for (const auto& node : *this) {
            token_vector.push_back(std::get<0>(node.pair));
        }

        // Add the last token from the last pair
        if (tail) {
            token_vector.push_back(std::get<1>(tail->pair));
        }

        return token_vector;
    }
    

private:
    Node* head;
    Node* tail;
    
    Counter  counter;
    std::unordered_map<Kmer, std::unordered_set<size_t>> positions;

    size_t size_;
    std::unordered_map<size_t, Node*> indexMap;

};


#endif