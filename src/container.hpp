#ifndef CONTAINER_FILE_H
#define CONTAINER_FILE_H


#include <tuple>
#include <vector>
#include "tokens.hpp"
#include <unordered_map>
#include <iostream>
#include <unordered_set>
#include <chrono>
#include <queue>
#include <thread>
#include <mutex>
std::mutex cout_mutex;

typedef std::unordered_map<Kmer, size_t> Counter;

class Node {
public:
    size_t index;
    Kmer pair; // std::tuple<uint16_t, uint16_t>
    bool help_token;
    Node* prev;
    Node* next;
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

    SequenceContainer() : head(nullptr), tail(nullptr), size_(0) {}

    SequenceContainer(const std::vector<TokenType>& seq) : head(nullptr), tail(nullptr), size_(0) {
        for (size_t i = 0; i < seq.size() - 1; i++) {

            if (i && i % 1000000 == 0) {
                std::cout << "Processed " << 100 * i / seq.size() << "%% tokens from " << seq.size() << std::endl;
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

        for (const auto& [kmer, count] : counter) {
            max_heap.push(std::make_pair(count, kmer));
        }
    }

    static void process_chunk(size_t thread_id, const std::vector<TokenType>& seq, size_t start, size_t end, SequenceContainer& container) {

        size_t total = end - 1 - start;

        for (size_t i = start; i < end - 1; i++) {
            TokenType a = seq.at(i);
            TokenType b = seq.at(i + 1);
            Kmer pair = std::make_tuple(a, b);
            bool help_token = false;
            if (a <= N_HELP_TOKENS || b <= N_HELP_TOKENS) {
                help_token = true;
            }
            container.append(i, pair, help_token);
            if (i && i % 1000000 == 0) {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "Thread " << thread_id << ": Processed " << 100. * (i-start) / (end - 1 - start) << " tokens from " << (end - 1 - start) << std::endl;
            }
        }
    }

    SequenceContainer(const std::vector<TokenType>& seq, size_t num_threads) : head(nullptr), tail(nullptr), size_(0) {

        size_t chunk_size = seq.size() / num_threads;
        std::vector<std::thread> threads;
        std::vector<SequenceContainer> containers(num_threads, SequenceContainer());
        
        for (size_t i = 0; i < num_threads; i++) {
            size_t start = i * chunk_size;
            size_t end = (i == num_threads - 1) ? seq.size() : start + chunk_size;
            std::cout << "Adding thread " << i << ": Processing chunk from " << start << " to " << end << " " << seq.size() << std::endl;
            threads.emplace_back([&, i, start, end, seq = std::cref(seq), container = std::ref(containers[i])] {
                    process_chunk(i, seq, start, end, container);
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        std::cout << "All threads compelted." << std::endl;
        for (size_t i = 0; i < num_threads; i++) {
            merge(std::move(containers[i]));
        }

        std::cout << "Updating max heap..." << std::endl;
        for (const auto& [kmer, count] : counter) {
            max_heap.push(std::make_pair(count, kmer));
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

    void merge(SequenceContainer&& other) {

        std::cout << "Merging containers..." << std::endl;

        if (!other.head) {
            return;
        }

        if (!head) {
            head = other.head;
            tail = other.tail;
        } else {
            tail->next = other.head;
            other.head->prev = tail;
            tail = other.tail;
        }

        size_ += other.size_;

        for (const auto& [kmer, count] : other.counter) {
            counter[kmer] += count;
        }

        for (const auto& [kmer, set] : other.positions) {
            positions[kmer].insert(set.begin(), set.end());
        }

        for (const auto& [index, node] : other.indexMap) {
            indexMap[index] = node;
        }

        other.head = nullptr;
        other.tail = nullptr;
    }

    ~SequenceContainer() {
        while (head) {
            Node* temp = head;
            head = head->next;
            if (temp != nullptr) delete temp;
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
    

        std::unordered_set<Kmer> touched_kmers;

        Node* currentNode = indexMap[index];

        if (currentNode->pair != rep) {
            return;
        }

        Node* prevNode = currentNode->prev;

        if (prevNode != nullptr) {
            
            Kmer prev_kmer = prevNode->pair;
            Kmer left_kmer = std::make_tuple(std::get<0>(prevNode->pair), L);
                 
            if (!prevNode->help_token) {
            
                counter[prev_kmer]--;
                if (counter[prev_kmer] == 0) {
                    counter.erase(prev_kmer);
                }
                positions[prev_kmer].erase(prevNode->index);

                counter[left_kmer]++;
                positions[left_kmer].insert(prevNode->index);  

                touched_kmers.insert(prev_kmer);
                touched_kmers.insert(left_kmer);  
            }
            

            prevNode->pair = left_kmer;
        }

        Node* nextNode = currentNode->next;
        

        if (nextNode != nullptr) {
            Kmer next_kmer = nextNode->pair;
            Kmer right_kmer = std::make_tuple(L, std::get<1>(nextNode->pair));

            if (!nextNode->help_token) {

                counter[next_kmer]--;
                if (counter[next_kmer] == 0) {
                    counter.erase(next_kmer);
                }
                positions[next_kmer].erase(nextNode->index);

                counter[right_kmer]++;
                positions[right_kmer].insert(nextNode->index);
                
                touched_kmers.insert(next_kmer);
                touched_kmers.insert(right_kmer);
            }

            nextNode->pair = right_kmer;

        }

        removeAtIndex(index);

        touched_kmers.insert(rep);
        
        for (const auto& kmer : touched_kmers) {
            if (counter[kmer] > 0) {
                max_heap.push(std::make_pair(counter[kmer], kmer));
            }
        }
        
    }

    std::pair<Kmer, size_t> get_most_frequent_pair() {
        if (counter.empty()) {
            throw std::runtime_error("The container is empty, cannot find the most frequent pair.");
        }

        while (!max_heap.empty()) {
            auto top_entry = max_heap.top();
            size_t count = top_entry.first;
            Kmer kmer = top_entry.second;

            if (counter[kmer] == count) {
                return std::pair(kmer, count);
            }
            max_heap.pop();
        }

        throw std::runtime_error("Could not find the most frequent pair.");
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
    Node* head = nullptr;
    Node* tail = nullptr;
    
    Counter counter;
    std::unordered_map<Kmer, std::unordered_set<size_t>> positions;

    size_t size_ = 0;
    std::unordered_map<size_t, Node*> indexMap;
    std::priority_queue<std::pair<size_t, Kmer>> max_heap;

};


#endif