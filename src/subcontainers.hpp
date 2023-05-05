#ifndef SUBCONTAINER_FILE_H
#define SUBCONTAINER_FILE_H

#include <tuple>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <unordered_set>
#include <chrono>
#include <queue>
#include <thread>
#include <mutex>
#include <cstring>
#include <atomic>
#include <cmath>

#include "tokens.hpp"
#include "positions.hpp"

// typedef CounterType to uint32_t
typedef uint32_t CounterType;

class CounterContainer {
public:

    // default constructer
    CounterContainer() {
        counts = nullptr;
        flags = nullptr;
        positions = nullptr;
        size_ = 0;
        max_size = 402653184;
    }

    // Copy constructor
    CounterContainer(const CounterContainer& other) {
        size_.store(other.size_.load());
        max_size = other.max_size;

        counts = new std::atomic<CounterType>[max_size];
        for (size_t i = 0; i < max_size; i++) {
            counts[i].store(other.counts[i].load());
        }
        
        flags = new char[max_size];
        memcpy(flags, other.flags, max_size * sizeof(char));

        positions = new PositionsContainer*[max_size];
        for (size_t i = 0; i < max_size; i++) {
            if (other.positions[i]) {
                positions[i] = new PositionsContainer(*other.positions[i]);
            } else {
                positions[i] = nullptr;
            }
        }
    }

    // Copy assignment operator
    CounterContainer& operator=(const CounterContainer& other) {
        if (this != &other) {
            size_.store(other.size_.load());
            max_size = other.max_size;

            delete[] counts;
            counts = new std::atomic<CounterType>[max_size];
            for (size_t i = 0; i < max_size; i++) {
                counts[i].store(other.counts[i].load());
            }

            delete[] flags;
            flags = new char[max_size];
            memcpy(flags, other.flags, max_size * sizeof(char));

            if (positions != nullptr) {
                for (size_t i = 0; i < max_size; i++) {
                    delete positions[i];
                }
                delete[] positions;
            }
            positions = new PositionsContainer*[max_size];
            for (size_t i = 0; i < max_size; i++) {
                if (other.positions[i]) {
                    positions[i] = new PositionsContainer(*other.positions[i]);
                } else {
                    positions[i] = nullptr;
                }
            }
        }
        return *this;
    }

    CounterContainer(size_t starting_size) {
        counts = new std::atomic<CounterType>[max_size];
        for (CounterType i = 0; i < max_size; i++) {
            counts[i] = 0;
        }
        flags = new char[max_size];
        for (size_t i = 0; i < max_size; i++) {
            flags[i] = 0;
        }
        positions = new PositionsContainer*[max_size];
        for (size_t i = 0; i < max_size; i++) {
            positions[i] = nullptr;
        }
        size_ = starting_size;
    }

    ~CounterContainer() {
        if (counts != nullptr) delete[] counts;
        if (flags != nullptr) delete[] flags;
        if (positions != nullptr) {
            for (size_t i = 0; i < max_size; ++i) {
                if (positions[i] != nullptr) {
                    delete positions[i];
                }
            }
            delete[] positions;
        }
    }

    void init_positions(size_t kmer_id, size_t tf) {
        if (kmer_id >= max_size) {
            std::cout << "kmer_id >= max_size" << std::endl;
            exit(1);
        }
        size_.store(std::max(size_.load(), kmer_id + 1));
        if (size_ >= max_size) {
            extend_counts();
        }
        if (positions[kmer_id] == nullptr) {
            positions[kmer_id] = new PositionsContainer(tf);
            return;
        }
    }

    void set_token(size_t kmer_id, char is_help_token) {
        if (kmer_id >= max_size) {
            std::cout << "kmer_id >= max_size" << std::endl;
            exit(1);
        }
        flags[kmer_id] = is_help_token;
    }

    void decrease(size_t kmer_id) {
        if (kmer_id >= max_size) {
            std::cout << "kmer_id >= max_size" << std::endl;
            exit(1);
        }
        
        if (kmer_id < size_) {
            counts[kmer_id]--;    
            return;
        }
        std::cout << "overflow counter in decrease kmer_id > size_: " << kmer_id << " " << size_ << std::endl;
    }

    void increase(size_t kmer_id) {
        if (kmer_id >= max_size) {
            std::cout << "kmer_id >= max_size" << std::endl;
            exit(1);
        }
        if (kmer_id < size_) {
            counts[kmer_id]++;
            return;
        }
        std::cout << "overflow counter in increase kmer_id > size_: " << kmer_id << " " << size_ << std::endl;
    }

    bool is_helper_kmer(size_t kmer_id) {
        if (kmer_id >= max_size) {
            std::cout << "kmer_id >= max_size" << std::endl;
            exit(1);
        }
        if (kmer_id < size_) {
            return flags[kmer_id];
        }
        std::cout << "overflow helper counter kmer_id > size_: " << kmer_id << " " << size_ << std::endl;
        return false;
    }

    void remove(size_t kmer_id) {
        if (kmer_id >= max_size) {
            std::cout << "kmer_id >= max_size" << std::endl;
            exit(1);
        }
        if (kmer_id < size_) {
            counts[kmer_id] = 0;
            flags[kmer_id] = 0;
            positions[kmer_id]->clear();
            return;
        }
        std::cout << "overflow counter remove kmer_id > size_: " << kmer_id << " " << size_ << std::endl;
    }

    CounterType get(size_t kmer_id) {
        if (kmer_id >= max_size) {
            std::cout << "kmer_id >= max_size" << std::endl;
            exit(1);
        }
        return counts[kmer_id].load();
    }

    void add_position(size_t kmer_id, size_t position) {
        if (kmer_id >= max_size) {
            std::cout << "kmer_id >= max_size" << std::endl;
            exit(1);
        }
        positions[kmer_id]->set(position);
    }

    PositionsContainer& get_positions(size_t kmer_id) {
        if (kmer_id >= max_size) {
            std::cout << "kmer_id >= max_size" << std::endl;
            exit(1);
        }
        return *positions[kmer_id];
    }

    size_t size() const {
        return size_;
    }

    void print_counts() {
        for (size_t i = 0; i < size_; i++) {
            if (counts[i] > 0) {
                std::cout << i << " " << counts[i] << std::endl;
            }
        }
    }


    void extend_counts() {
        size_t new_size = max_size + increment;
        std::cout << "extend_counts to " << new_size << std::endl;
        std::atomic<CounterType>* new_counts = new std::atomic<CounterType>[new_size];
        char* new_flags = new char[new_size];
        PositionsContainer** new_positions = new PositionsContainer*[new_size];
        for (size_t i = 0; i < max_size; i++) {
            new_flags[i] = flags[i];
            new_positions[i] = positions[i];
        }
        for (CounterType i = 0; i < max_size; i++) {
            new_counts[i].store(counts[i].load());
        }

        for (size_t i = max_size; i < new_size; i++) {
            new_counts[i] = 0;
            new_flags[i] = 0;
            new_positions[i] = nullptr;
        }
        for (CounterType i = max_size; i < new_size; i++) {
            new_counts[i] = 0;
        }
        delete[] counts;
        delete[] flags;
        delete[] positions;
        counts = new_counts;
        flags = new_flags;
        positions = new_positions;
        max_size = new_size;
    }

    void diagnostic_print_of_state() {
        // print all variables
        std::cout << "COINTER" << std::endl;
        std::cout << "size_ " << size_ << std::endl;
        std::cout << "max_size " << max_size << std::endl;
        std::cout << "counts" << std::endl;
        for (size_t i = 0; i < size_; i++) {
            std::cout << i << " " << counts[i] << std::endl;
        }
        std::cout << "flags" << std::endl;
        for (size_t i = 0; i < size_; i++) {
            std::cout << i << " " << (int)flags[i] << std::endl;
        }
        std::cout << "positions" << std::endl;
        for (size_t i = 0; i < size_; i++) {
            std::cout << i << std::endl;
            if (positions[i] != nullptr) {
                positions[i]->diagnostic_print_of_state();
            } else {
                std::cout << " nullptr" << std::endl;
            }
        }
    }

private:
    std::atomic<CounterType> * counts;
    PositionsContainer** positions;
    char* flags;
    std::atomic<u_int64_t> size_ = 0;
    size_t max_size = 40265318; // 3Gb of space
    const size_t increment = 12653184;
    // size_t max_size = 20; // 3Gb of space
};

#endif