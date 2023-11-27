#ifndef POSITIONS_FILE_H
#define POSITIONS_FILE_H

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

// typedef CounterType to uint32_t
typedef uint32_t CounterType;

class PositionsContainer {
public:

    PositionsContainer() {
        max_size_ = 100;
        positions = nullptr;
        size_ = 0;   
    }

    PositionsContainer(size_t size) {
        
        positions = new size_t[size];
        for (size_t i = 0; i < size; i++) {
            positions[i] = 0;
        }
        size_ = 0;
        max_size_ = size;
    }

    // the copy constructor
    PositionsContainer(const PositionsContainer& other)
    : max_size_(other.max_size_) {
        size_.store(other.size_.load());
        positions = new size_t[max_size_];
        for (size_t i = 0; i < max_size_; i++) {
            positions[i] = other.positions[i];
        }
    }
    
    // the copy assignment operator
    PositionsContainer& operator=(const PositionsContainer& other) {
        if (this != &other) {
            // Free the existing memory if necessary
            if (positions != nullptr) {
                delete[] positions;
            }

            // Copy the size and max_size values
            size_.store(other.size_.load());
            max_size_ = other.max_size_;

            // Allocate new memory for positions and copy the elements from the other object
            positions = new size_t[max_size_];
            for (size_t i = 0; i < max_size_; i++) {
                positions[i] = other.positions[i];
            }
        }
        return *this;
    }

    // Move constructor
    PositionsContainer(PositionsContainer&& other) noexcept
        : positions(other.positions), max_size_(other.max_size_) {
        size_.store(other.size_.load());
        // Nullify the pointers in the other object to avoid double deletion
        other.positions = nullptr;
        other.size_ = 0;
        other.max_size_ = 0;
    }

    // Move assignment operator
    PositionsContainer& operator=(PositionsContainer&& other) noexcept {
        if (this != &other) {
            if (positions != nullptr) delete[] positions;

            positions = other.positions;
            size_.store(other.size_.load());
            max_size_ = other.max_size_;

            other.positions = nullptr;
            other.size_ = 0;
            other.max_size_ = 0;
        }
        return *this;
    }


    ~PositionsContainer() {
        if (positions != nullptr) {
            delete[] positions;
        }
    }

    void set(size_t index) {
        if (size_.load() >= max_size_) {
            extend_counts();
        }
        positions[size_.fetch_add(1)] = index + 1;
    }

    size_t get(size_t index) {
        if (index >= size_) {
            std::cout << "index >= size_" << std::endl;
            exit(1);
        }
        return positions[index] - 1;
    }

    size_t get_plus_one_position(size_t index) {
        if (index >= size_) {
            std::cout << "index >= size_" << std::endl;
            exit(1);
        }
        return positions[index];
    }

    void clear() {
        if (positions != nullptr) {
            delete[] positions;
            positions = nullptr;
        }
    }
    u_int64_t size() const {
        return size_.load();
    }

    void extend_counts() {
        size_t* new_positions = new size_t[max_size_ * 2];
        for (size_t i = 0; i < max_size_; i++) {
            new_positions[i] = positions[i];
        }
        for (size_t i = max_size_; i < max_size_ * 2; i++) {
            new_positions[i] = 0;
        }
        delete[] positions;
        positions = new_positions;
        max_size_ *= 2;
    }

    void diagnostic_print_of_state() {
        // print all variables
        std::cout << "POSITIONS" << std::endl;
        std::cout << "size_: " << size_ << std::endl;
        std::cout << "max_size_: " << max_size_ << std::endl;
        std::cout << "positions: " << std::endl;
        if (positions == nullptr) {
            std::cout << "nullptr" << std::endl;
            return;
        }
        for (size_t i = 0; i < size_; i++) {
            std::cout << positions[i] << " ";
        }
        std::cout << std::endl;
    }


private:
    size_t* positions = nullptr;
    std::atomic<u_int64_t> size_ = 0;
    size_t max_size_ = 1000;
};

#endif