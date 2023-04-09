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
std::mutex hash_mutex;


class Node {
public:
    size_t index;
    size_t kmer_id; // kmer_id to std::tuple<uint16_t, uint16_t>
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

    SequenceContainer(const std::vector<TokenType>& seq, std::unordered_map<Kmer, size_t, TupleHash>& kmer2kmer_id, std::unordered_map<size_t, Kmer>& kmer_id2kmer) : head(nullptr), tail(nullptr), size_(0) {

        for (size_t i = 0; i < seq.size() - 1; i++) {

            if (i && i % 1000000 == 0) {
                std::cout << "Processed " << 100 * i / seq.size() << "%% tokens from " << seq.size() << std::endl;
            }

            TokenType a = seq[i];
            TokenType b = seq[i + 1];
            Kmer pair = std::make_tuple(a, b);

            if (kmer2kmer_id.find(pair) == kmer2kmer_id.end()) {
                kmer2kmer_id[pair] = kmer2kmer_id.size();
                kmer_id2kmer[kmer_id2kmer.size()] = pair;
            }
            size_t kmer_id = kmer2kmer_id[pair];

            bool help_token = false;
            if (a <= N_HELP_TOKENS || b <= N_HELP_TOKENS) {
                help_token = true;
            }
            append(i, kmer_id, help_token);
        }

        for (const auto& [kmer_id, count] : counter) {
            max_heap.push(std::make_pair(count, kmer_id));
        }
    }

    static void process_chunk(size_t thread_id, const std::vector<TokenType>& seq, size_t start, size_t end, SequenceContainer& container, std::unordered_map<Kmer, size_t, TupleHash>& kmer2kmer_id, std::unordered_map<size_t, Kmer>& kmer_id2kmer) {

        for (size_t i = start; i < end - 1; i++) {
            TokenType a = seq.at(i);
            TokenType b = seq.at(i + 1);
            Kmer pair = std::make_tuple(a, b);
            bool help_token = false;
            if (a <= N_HELP_TOKENS || b <= N_HELP_TOKENS) {
                help_token = true;
            }

            if (kmer2kmer_id.find(pair) == kmer2kmer_id.end()) {
                std::lock_guard<std::mutex> lock(hash_mutex);
                kmer2kmer_id[pair] = kmer2kmer_id.size();
                kmer_id2kmer[kmer_id2kmer.size()] = pair;
            }
            size_t kmer_id = kmer2kmer_id.at(pair);

            container.append_simple(i, kmer_id, help_token);
            if (i && i % 1000000 == 0) {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "Thread " << thread_id << ": Processed " << 100. * (i-start) / (end - 1 - start) << " tokens from " << (end - 1 - start) << std::endl;
            }
        }
    }

    SequenceContainer(const std::vector<TokenType>& seq, std::unordered_map<Kmer, size_t, TupleHash>& kmer2kmer_id, std::unordered_map<size_t, Kmer>& kmer_id2kmer, size_t num_threads) : head(nullptr), tail(nullptr), size_(0) {

        size_t chunk_size = seq.size() / num_threads;
        std::vector<std::thread> threads;
        std::vector<SequenceContainer> containers(num_threads, SequenceContainer());
        
        for (size_t i = 0; i < num_threads; i++) {
            size_t start = i * chunk_size;
            size_t end = (i == num_threads - 1) ? seq.size() : start + chunk_size;
            std::cout << "Adding thread " << i << ": Processing chunk from " << start << " to " << end << " " << seq.size() << std::endl;
            threads.emplace_back([&, i, start, end, seq = std::cref(seq), container = std::ref(containers[i])] {
                    process_chunk(i,  seq, start, end, container, kmer2kmer_id, kmer_id2kmer);
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        std::cout << "All threads compeleted." << std::endl;
        for (size_t i = 0; i < num_threads; i++) {
            merge(std::move(containers[i]), kmer2kmer_id, kmer_id2kmer);
        }

        // recompute counter and positions
        std::cout << "Recomputing counter and positions..." << std::endl;
        for (const auto& [kmer_id, count] : counter) {
            positions[kmer_id].clear();
        }
        counter.clear();
        for (auto it = begin(); it != end(); ++it) {
            if (!it->help_token) {
                counter[it->kmer_id]++;
                positions[it->kmer_id].insert(it->index);
            }
        }

        std::cout << "Updating max heap..." << std::endl;
        for (const auto& [kmer_id, count] : counter) {
            max_heap.push(std::make_pair(count, kmer_id));
        }
        std::cout << "Max heap updated." << std::endl;
    }

    void append(size_t index, size_t kmer, bool help_token) {
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
            positions[kmer].insert(index);
        }
        indexMap[index] = newNode;
        size_++;
    }

    void append_simple(size_t index, size_t kmer, bool help_token) {
        Node* newNode = new Node{index, kmer, help_token, nullptr, nullptr};
        if (!head) {
            head = tail = newNode;
        } else {
            newNode->prev = tail;
            tail->next = newNode;
            tail = newNode;
        }
        indexMap[index] = newNode;
        size_++;
    }

    void merge(SequenceContainer&& other, std::unordered_map<Kmer, size_t, TupleHash>& kmer2kmer_id, std::unordered_map<size_t, Kmer>& kmer_id2kmer) {
        ++merge_count;
        std::cout << "Merging containers (" << merge_count << ")..." << std::endl;

        if (!other.head) {
            return;
        }

        if (!head) {
            head = other.head;
            tail = other.tail;
        } else {
            // Handle border nodes between parts
            TokenType a = std::get<1>(kmer_id2kmer[tail->kmer_id]);
            TokenType b = std::get<0>(kmer_id2kmer[other.head->kmer_id]);
            Kmer border_pair = std::make_tuple(a, b);

            if (kmer2kmer_id.find(border_pair) == kmer2kmer_id.end()) {
                kmer2kmer_id[border_pair] = kmer2kmer_id.size();
                kmer_id2kmer[kmer_id2kmer.size()] = border_pair;
            }
            size_t border_kmer_id = kmer2kmer_id[border_pair];

            tail->kmer_id = border_kmer_id;

            tail->next = other.head;
            other.head->prev = tail;
            tail = other.tail;
        }

        size_ += other.size_;

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

    void display(std::unordered_map<TokenType, std::string>& alphabet_map, std::unordered_map<size_t, Kmer>& kmer_id2kmer) {
        Node* temp = head;

        while (temp) {

            Kmer kmer = kmer_id2kmer[temp->kmer_id];

            std::cout << alphabet_map.at(std::get<0>(kmer)) << "|" << alphabet_map.at(std::get<1>(kmer)) << " ";
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
        counter[currentNode->kmer_id]--;
        size_--;

        delete currentNode;
        return true;
    }


    void collapse(size_t kmer_id, size_t index, TokenType L, std::unordered_map<Kmer, size_t, TupleHash>& kmer2kmer_id, std::unordered_map<size_t, Kmer>& kmer_id2kmer) {
    

        std::unordered_set<size_t> touched_kmers;

        Node* currentNode = indexMap[index];

        if (currentNode->kmer_id != kmer_id) {
            return;
        }

        Node* prevNode = currentNode->prev;

        if (prevNode != nullptr) {
            
            size_t prev_kmer_id = prevNode->kmer_id;

            // new kmer
            Kmer prevKmer = kmer_id2kmer[prev_kmer_id];
            Kmer left_kmer = std::make_tuple(std::get<0>(prevKmer), L);
            if (kmer2kmer_id.find(left_kmer) == kmer2kmer_id.end()) {
                kmer2kmer_id[left_kmer] = kmer2kmer_id.size();
                kmer_id2kmer[kmer_id2kmer.size()] = left_kmer;
            }
            size_t left_kmer_id = kmer2kmer_id[left_kmer];
                 

            if (!prevNode->help_token) {
                counter[prev_kmer_id]--;
                counter[left_kmer_id]++;
                positions[left_kmer_id].insert(prevNode->index);
                touched_kmers.insert(prev_kmer_id);
                touched_kmers.insert(left_kmer_id);
            }

            prevNode->kmer_id = left_kmer_id;
        }

        Node* nextNode = currentNode->next;
        
        if (nextNode != nullptr) {

            size_t next_kmer_id = nextNode->kmer_id;


            Kmer next_kmer = kmer_id2kmer[next_kmer_id];
            Kmer right_kmer = std::make_tuple(L, std::get<1>(next_kmer));
            if (kmer2kmer_id.find(right_kmer) == kmer2kmer_id.end()) {
                kmer2kmer_id[right_kmer] = kmer2kmer_id.size();
                kmer_id2kmer[kmer_id2kmer.size()] = right_kmer;
            }
            size_t right_kmer_id = kmer2kmer_id[right_kmer];

            if (!nextNode->help_token) {

                counter[next_kmer_id]--;
                counter[right_kmer_id]++;
                positions[right_kmer_id].insert(nextNode->index);
                touched_kmers.insert(next_kmer_id);
                touched_kmers.insert(right_kmer_id);
            }

            nextNode->kmer_id = right_kmer_id;

        }

        removeAtIndex(index);

        touched_kmers.insert(kmer_id);
        
        for (const auto& kmer_id : touched_kmers) {
            if (counter[kmer_id] > 0) {
                max_heap.push(std::make_pair(counter[kmer_id], kmer_id));
            } else {
                counter.erase(kmer_id);
                positions.erase(kmer_id);
            }
        }
        
    }

    std::pair<size_t, size_t> get_most_frequent_pair() {
        if (counter.empty()) {
            throw std::runtime_error("The container is empty, cannot find the most frequent pair.");
        }

        while (!max_heap.empty()) {
            auto top_entry = max_heap.top();
            size_t count = top_entry.first;
            size_t kmer_id = top_entry.second;

            if (counter[kmer_id] == count) {
                return std::pair(kmer_id, count);
            }
            max_heap.pop();
        }

        throw std::runtime_error("Could not find the most frequent pair.");
    }

    size_t size() {
        return size_;
    }

    void process_repeat(size_t kmer_id, TokenType L, std::unordered_map<Kmer, size_t, TupleHash>& kmer2kmer_id, std::unordered_map<size_t, Kmer>& kmer_id2kmer) {

        // std::cout << "Collapsing..." << std::endl;
        for (size_t index: get_positions(kmer_id)) {
            collapse(kmer_id, index, L, kmer2kmer_id, kmer_id2kmer);
        }
        // std::cout << "Done." << std::endl;
    }

    std::vector<size_t> get_positions(size_t rep) {
        const std::unordered_set<size_t>& position_set = positions.at(rep);
        std::vector<size_t> position_vector(position_set.begin(), position_set.end());  
        std::sort(position_vector.begin(), position_vector.end());
        return position_vector;
    }

    // print counter
    void print_counter(std::unordered_map<TokenType, std::string>& alphabet_map, std::unordered_map<size_t, Kmer>& kmer_id2kmer) {
        for (const auto& entry : counter) {

            Kmer kmer = kmer_id2kmer.at(entry.first);

            std::cout << alphabet_map.at(std::get<0>(kmer)) << " " << alphabet_map.at(std::get<1>(kmer)) << " " << entry.second << std::endl;
        }
        // print get_most_frequent_pair

        Kmer kmer = kmer_id2kmer.at(get_most_frequent_pair().first);

        std::cout << "Most frequent pair: " << alphabet_map.at(std::get<0>(kmer)) << " " << alphabet_map.at(std::get<1>(kmer)) << " " << get_most_frequent_pair().second << std::endl;
    }


    iterator begin() const { return iterator(head); }
    iterator end() const { return iterator(nullptr); }

    std::vector<TokenType> get_as_vector(std::unordered_map<size_t, Kmer>& kmer_id2kmer) {
        std::vector<TokenType> token_vector;
        token_vector.reserve(size_);

        for (const auto& node : *this) {

            Kmer kmer = kmer_id2kmer.at(node.kmer_id);
            token_vector.emplace_back(std::get<0>(kmer));
        }

        // Add the last token from the last pair
        if (tail) {
            Kmer kmer = kmer_id2kmer.at(tail->kmer_id);
            token_vector.emplace_back(std::get<1>(kmer));
        }

        return token_vector;
    }
    

private:
    Node* head = nullptr;
    Node* tail = nullptr;

    std::unordered_map<size_t, size_t> kmer2id;
    std::unordered_map<size_t, size_t> id2kmer;
    
    std::unordered_map<size_t, size_t>  counter;
    std::unordered_map<size_t, std::unordered_set<size_t>> positions;

    size_t size_ = 0;
    std::unordered_map<size_t, Node*> indexMap;
    std::priority_queue<std::pair<size_t, size_t>> max_heap;

    uint merge_count = 0;

};


#endif