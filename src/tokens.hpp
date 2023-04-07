#ifndef TOKENS_FILE_H
#define TOKENS_FILE_H

#include <string>
#include <tuple>
#include <functional>
#include <map>
#include <numeric>

typedef std::uint16_t TokenType;

const uint N_HELP_TOKENS = 6;
const uint MAX_N_TOKENS = 65535;

typedef std::tuple<TokenType, TokenType> kmer;

namespace std {
    template<>
    struct hash<tuple<TokenType, TokenType>> {
        size_t operator() (const tuple<TokenType, TokenType> &t) const {
            return hash<TokenType>()(get<0>(t)) ^ hash<TokenType>()(get<1>(t));
        }
    };
}


std::unordered_map<std::string, TokenType> alphabet = {
    {"[UNK]", 0},
    {"[CLS]", 1},
    {"[SEP]", 2},
    {"[PAD]", 3},
    {"[MASK]", 4},
    {"~", 5},
    {"N", 6},
    {"A", 7},
    {"C", 8},
    {"G", 9},
    {"T", 10}
};

template<typename T>
struct tuple_compare {
    bool operator()(const std::tuple<T, T>& a, const std::tuple<T, T>& b) const {
        if (std::get<0>(a) == std::get<0>(b)) {
            return std::get<1>(a) < std::get<1>(b);
        }
        return std::get<0>(a) < std::get<0>(b);
    }
};


struct pair_hash {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2> &pair) const {
        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
};

// Custom hash function for std::tuple
template<typename T>
struct tuple_hash {
    template<typename Tuple, TokenType N>
    struct hasher {
        static void hash(Tuple const& t, TokenType& seed) {
            hasher<Tuple, N - 1>::hash(t, seed);
            seed ^= std::hash<typename std::tuple_element<N - 1, Tuple>::type>()(std::get<N - 1>(t)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
    };

    template<typename Tuple>
    struct hasher<Tuple, 1> {
        static void hash(Tuple const& t, TokenType& seed) {
            seed ^= std::hash<typename std::tuple_element<0, Tuple>::type>()(std::get<0>(t)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
    };

    template<typename... Args>
    TokenType operator()(std::tuple<Args...> const& t) const {
        TokenType seed = 0;
        hasher<std::tuple<Args...>, sizeof...(Args)>::hash(t, seed);
        return seed;
    }
};


// NB: this function is not used in the current implementation and incorrect
std::string token_type_to_string(TokenType token, const std::unordered_map<std::string, TokenType>& alphabet, const std::unordered_map<TokenType, kmer>& tokens) {
    
    // Check if the token is in the initial alphabet
    for (const auto& element : alphabet) {
        if (element.second == token) {
            return element.first;
        }
    }

    // Check if the token is in the tokens map
    auto token_iter = tokens.find(token);
    if (token_iter != tokens.end()) {
        kmer kmer_ = token_iter->second;
        TokenType token1 = std::get<0>(kmer_);
        TokenType token2 = std::get<1>(kmer_);
        return token_type_to_string(token1, alphabet, tokens) + token_type_to_string(token2, alphabet, tokens);
    }

    // If the token is not found in either map, return an empty string
    return "";
}

#endif