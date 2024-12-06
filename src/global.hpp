#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <array>
#include <algorithm>
#include <vector>
#include <cstdint>

namespace TokenizerConstants {
    static constexpr size_t BASE_VOCAB_SIZE = 5;
    static constexpr std::array<const char*, BASE_VOCAB_SIZE> BASE_VOCAB = {
        "A", "C", "G", "T", "<SEP>"
    };
    static constexpr std::array<char, 4> NUCLEOTIDES = {'A', 'C', 'G', 'T'};
    static constexpr size_t SEP_TOKEN_ID = 4;

    static bool is_nucleotide(char c) {
        return std::find(NUCLEOTIDES.begin(), NUCLEOTIDES.end(), c) != NUCLEOTIDES.end();
    }

    static uint32_t char_to_token_id(char c) {
        switch(c) {
            case 'A': return 0;
            case 'C': return 1;
            case 'G': return 2;
            case 'T': return 3;
            default: return SEP_TOKEN_ID;
        }
    }
}

// Global token frequencies management - only declarations
extern std::vector<size_t> global_token_frequencies;

// Function declarations only
void init_token_frequencies(size_t max_size);
void increase_token_frequency(uint32_t token_id);
size_t get_token_frequency(uint32_t token_id);

#endif // GLOBAL_HPP