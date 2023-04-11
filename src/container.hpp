#ifndef CONTAINER_FILE_H
#define CONTAINER_FILE_H

#include <stdexcept>
#include <tuple>
#include <vector>
#include <string>
#include "tokens.hpp"
#include <unordered_map>
#include <iostream>
#include <unordered_set>
#include <chrono>
#include <queue>
#include <thread>
#include <mutex>
#include "subcontainers.hpp"

std::mutex cout_mutex;
std::mutex hash_mutex;


std::string input;

class SequenceContainer {
public:

    SequenceContainer() {
        array_of_tokens = nullptr;
        array_of_prevs = nullptr;
        array_of_nexts = nullptr;
        container_size_ = 0;
        size_ = 0;
    }

    // Copy constructor
    SequenceContainer(const SequenceContainer& other) {
        container_size_ = other.container_size_;
        size_ = other.size_;
        counter = other.counter;
        merge_count = other.merge_count;
        max_heap = other.max_heap;

        array_of_tokens = new size_t[container_size_];
        memcpy(array_of_tokens, other.array_of_tokens, container_size_ * sizeof(size_t));

        array_of_prevs = new size_t[container_size_];
        memcpy(array_of_prevs, other.array_of_prevs, container_size_ * sizeof(size_t));

        array_of_nexts = new size_t[container_size_];
        memcpy(array_of_nexts, other.array_of_nexts, container_size_ * sizeof(size_t));
    }

    // Copy assignment operator
    SequenceContainer& operator=(const SequenceContainer& other) {
        if (this != &other) {
            container_size_ = other.container_size_;
            size_ = other.size_;
            counter = other.counter;
            merge_count = other.merge_count;
            max_heap = other.max_heap;

            delete[] array_of_tokens;
            array_of_tokens = new size_t[container_size_];
            memcpy(array_of_tokens, other.array_of_tokens, container_size_ * sizeof(size_t));

            delete[] array_of_prevs;
            array_of_prevs = new size_t[container_size_];
            memcpy(array_of_prevs, other.array_of_prevs, container_size_ * sizeof(size_t));

            delete[] array_of_nexts;
            array_of_nexts = new size_t[container_size_];
            memcpy(array_of_nexts, other.array_of_nexts, container_size_ * sizeof(size_t));
        }
        return *this;
    }

    SequenceContainer(const std::vector<TokenType>& seq, std::unordered_map<Kmer, size_t, TupleHash>& kmer2kmer_id, std::unordered_map<size_t, Kmer>& kmer_id2kmer) {
        
        std::cout << "Initializing container" << std::endl;
        size_ = 0;
        array_of_tokens = new size_t[seq.size()];
        array_of_prevs = new size_t[seq.size()];
        array_of_nexts = new size_t[seq.size()];
        container_size_ = seq.size();

        for (size_t i = 0; i < container_size_; i++) {
            array_of_tokens[i] = 0; // 1-based, zero is reserved for empty
            array_of_prevs[i] = 0; // 0-base, head as index == prevs
            array_of_nexts[i] = 0; // 0-base, tail as next == total size
        }
        // positions also 1-based
        std::cout << "Done" << std::endl;

        counter = CounterContainer(kmer_id2kmer.size());

        init_in_single_thread(seq, kmer2kmer_id, kmer_id2kmer); 
    }

    void init_in_single_thread(const std::vector<TokenType>& seq, std::unordered_map<Kmer, size_t, TupleHash>& kmer2kmer_id, std::unordered_map<size_t, Kmer>& kmer_id2kmer) {
        
        counter.set_token(0, 1);

        for (size_t i = 0; i < container_size_ - 1; i++) {

            if (i && i % 1000000 == 0) {
                std::cout << "Processed " << 100 * i / container_size_ << "%% tokens from " << container_size_ << std::endl;
            }

            TokenType a = seq[i];
            TokenType b = seq[i + 1];
            Kmer pair = std::make_tuple(a, b);

            char help_token = 0;
            if (a <= N_HELP_TOKENS || b <= N_HELP_TOKENS) {
                help_token = 1;
            }

            if (kmer2kmer_id.find(pair) == kmer2kmer_id.end()) {
                kmer2kmer_id[pair] = kmer2kmer_id.size();
                kmer_id2kmer[kmer_id2kmer.size()] = pair;
                if (!help_token) {
                    counter.init_positions(kmer2kmer_id[pair], 1000000);
                }
            }

            size_t kmer_id = kmer2kmer_id[pair];

            array_of_tokens[i] = kmer_id;
            if (i == 0) {
                array_of_prevs[i] = i;
            } else {
                array_of_prevs[i] = i - 1;
            }
            if (i == seq.size() - 2) {
                array_of_nexts[i] = container_size_;
            } else {
                array_of_nexts[i] = i + 1;
            }
            

            counter.set_token(kmer_id, help_token);
            if (!help_token) {
                counter.increase(kmer_id);
                counter.add_position(kmer_id, i);
            }
            size_++;
        }

        for (size_t i = 1; i < counter.size(); i++) {
            if (counter.is_helper_kmer(i) || counter.get(i) == 0) {
                continue;
            }
            max_heap.push(std::make_pair(counter.get(i), i));
        }
    }

    SequenceContainer(const std::vector<TokenType>& seq, std::unordered_map<Kmer, size_t, TupleHash>& kmer2kmer_id, std::unordered_map<size_t, Kmer>& kmer_id2kmer, size_t num_threads) : SequenceContainer(seq, kmer2kmer_id, kmer_id2kmer) {
        ;
    }

    void display(std::unordered_map<TokenType, std::string>& alphabet_map, std::unordered_map<size_t, Kmer>& kmer_id2kmer) {
        
        for (size_t i=0; i < container_size_; i++) {
            if (array_of_tokens[i] == 0) {
                continue;
            }
            Kmer kmer = kmer_id2kmer[array_of_tokens[i]];
            std::cout << alphabet_map.at(std::get<0>(kmer)) << "|" << alphabet_map.at(std::get<1>(kmer)) << " ";
        }
        std::cout << std::endl;
    }

    void print_queue(std::unordered_map<TokenType, std::string>& alphabet_map, std::unordered_map<size_t, Kmer>& kmer_id2kmer) {
        
        // iter and print max_heap
        std::priority_queue<std::pair<size_t, size_t>> temp = max_heap;
        while (!temp.empty()) {
            Kmer kmer = kmer_id2kmer[temp.top().second];
            std::cout << temp.top().first << " " << alphabet_map.at(std::get<0>(kmer)) << "|" << alphabet_map.at(std::get<1>(kmer)) << std::endl;
            temp.pop();
        }
    }

    bool removeAtIndex(size_t index) {
        size_t kmer_id = array_of_tokens[index];
        array_of_tokens[index] = 0;
        if (index == array_of_prevs[index]) {
            /// START A|B ...
            array_of_prevs[array_of_nexts[index]] = array_of_nexts[index];
        } else if (container_size_ == array_of_nexts[index]) {
            /// ...   A|B END
            array_of_nexts[array_of_prevs[index]] = container_size_;
        } else {
            /// ... A|B ...
            array_of_nexts[array_of_prevs[index]] = array_of_nexts[index];
            array_of_prevs[array_of_nexts[index]] = array_of_prevs[index];
        }
        counter.decrease(kmer_id);
        size_--;
        return true;
    }


    void collapse(size_t kmer_id, TokenType L, std::unordered_map<Kmer, size_t, TupleHash>& kmer2kmer_id, std::unordered_map<size_t, Kmer>& kmer_id2kmer, std::unordered_map<TokenType, std::string>& alphabet_map) {
    
        // counter.print_counts();

        std::unordered_set<size_t> touched_kmers;
        PositionsContainer& positions = counter.get_positions(kmer_id);

        for (size_t i = 0; i < positions.size(); i++) {
            if (positions.get_plus_one_position(i) == 0) {
                continue;
            }
            if (i && i % 10000000 == 0) {
                std::cout << "Processed " << i << " positions." << std::endl;
            }
            size_t index = positions.get_plus_one_position(i);
            if (index > 0 && kmer_id == array_of_tokens[index-1]) {
                index -= 1;
                

                size_t prev_index = array_of_prevs[index]; 

                size_t prev_kmer_id = array_of_tokens[prev_index];
                char is_helper = counter.is_helper_kmer(prev_kmer_id);

                if (index != array_of_prevs[index] && !is_helper) {
                    
                    // new kmer
                    Kmer prevKmer = kmer_id2kmer[prev_kmer_id];
                    Kmer left_kmer = std::make_tuple(std::get<0>(prevKmer), L);
                    if (kmer2kmer_id.find(left_kmer) == kmer2kmer_id.end()) {
                        kmer2kmer_id[left_kmer] = kmer2kmer_id.size();
                        kmer_id2kmer[kmer_id2kmer.size()] = left_kmer;
                        counter.set_token(kmer2kmer_id[left_kmer], 0);
                    }
                    size_t left_kmer_id = kmer2kmer_id[left_kmer];
                


                    counter.decrease(prev_kmer_id);
                    counter.increase(left_kmer_id);
                    counter.init_positions(left_kmer_id, 1000);
                    counter.add_position(left_kmer_id, prev_index);       
                    array_of_tokens[prev_index] = left_kmer_id;

                    touched_kmers.insert(prev_kmer_id);
                    touched_kmers.insert(left_kmer_id);
                }

                size_t next_index = array_of_nexts[index];
                size_t next_kmer_id = array_of_tokens[next_index];
                char is_helper_R = counter.is_helper_kmer(next_kmer_id);
                
                if (container_size_ != array_of_nexts[index]  && !is_helper_R) {

                    Kmer next_kmer = kmer_id2kmer[next_kmer_id];
                    Kmer right_kmer = std::make_tuple(L, std::get<1>(next_kmer));
                    if (kmer2kmer_id.find(right_kmer) == kmer2kmer_id.end()) {
                        kmer2kmer_id[right_kmer] = kmer2kmer_id.size();
                        kmer_id2kmer[kmer_id2kmer.size()] = right_kmer;
                        counter.set_token(kmer2kmer_id[right_kmer], 0);
                    }
                    size_t right_kmer_id = kmer2kmer_id[right_kmer];

                    counter.decrease(next_kmer_id);
                    counter.increase(right_kmer_id);
                    counter.init_positions(right_kmer_id, 1000);
                    counter.add_position(right_kmer_id, next_index); 
            
                    array_of_tokens[next_index] = right_kmer_id;

                    touched_kmers.insert(next_kmer_id);
                    touched_kmers.insert(right_kmer_id);
                }

                removeAtIndex(index);

                touched_kmers.insert(kmer_id);                

            }
        }

        
        

        for (const auto& kmer_id : touched_kmers) {
            if (counter.get(kmer_id) > 0 && !counter.is_helper_kmer(kmer_id)) {

                // print kmer id freq and alphabet_map
                Kmer kmer = kmer_id2kmer[kmer_id];
                // std::cout << "Kmer: " << alphabet_map[std::get<0>(kmer)] << alphabet_map[std::get<1>(kmer)] << " " << kmer_id << " " << counter.get(kmer_id) << std::endl;

                max_heap.push(std::make_pair(counter.get(kmer_id), kmer_id));
            } else {
                if (!counter.is_helper_kmer(kmer_id)) {
                    counter.remove(kmer_id);
                }
            }
        }

    }
    
    std::pair<size_t, size_t> get_most_frequent_pair() {
        
        while (!max_heap.empty()) {
            auto top_entry = max_heap.top();
            size_t count = top_entry.first;
            size_t kmer_id = top_entry.second;
            if (counter.get(kmer_id) == count) {
                return std::pair(kmer_id, count);
            }
            max_heap.pop();
        }
        throw std::runtime_error("Could not find the most frequent pair.");
    }

    size_t size() {
        return size_;
    }

    
    std::vector<TokenType> get_as_vector(std::unordered_map<size_t, Kmer>& kmer_id2kmer) {
        std::vector<TokenType> token_vector;
        token_vector.reserve(size_);

        for (size_t i=0; i < container_size_; ++i) {
            if (array_of_tokens[i] != 0) {
                size_t kmer_id = array_of_tokens[i];
                Kmer kmer = kmer_id2kmer.at(kmer_id);
                token_vector.emplace_back(std::get<0>(kmer));
            }
            if (i == container_size_ - 1) {
                size_t kmer_id = array_of_tokens[i];
                Kmer kmer = kmer_id2kmer.at(kmer_id);
                token_vector.emplace_back(std::get<1>(kmer));
            }
        }
        return token_vector;
    }
    
    ~SequenceContainer() {
        if (array_of_tokens != nullptr) {
            delete[] array_of_tokens;
        }
        if (array_of_prevs != nullptr) {
            delete[] array_of_prevs;
        }
        if (array_of_nexts != nullptr) {
            delete[] array_of_nexts;
        }
    }

private:


    size_t container_size_ = 0;
    size_t* array_of_tokens = nullptr;
    size_t* array_of_prevs = nullptr;
    size_t* array_of_nexts = nullptr;
    CounterContainer counter;
    size_t size_ = 0;
    std::priority_queue<std::pair<size_t, size_t>> max_heap;
    uint merge_count = 0;
};

#endif