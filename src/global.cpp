#include "global.hpp"

// Define the global token frequencies vector
std::vector<size_t> global_token_frequencies;

// Function definitions
void init_token_frequencies(size_t max_size) {
    global_token_frequencies.clear();
    global_token_frequencies.resize(max_size, 0);  // Allocate all at once
}

void increase_token_frequency(uint32_t token_id) {
    // No need to check size or resize since we pre-allocated
    global_token_frequencies[token_id]++;
}

size_t get_token_frequency(uint32_t token_id) {
    return global_token_frequencies[token_id];
}
