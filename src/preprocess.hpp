#pragma once
#include <string>
#include <vector>
#include <sstream>
#include "tokens.hpp"


std::string get_dataset(const std::vector<std::string>& seqs) {
    std::stringstream ss;
    bool first = true;
    for (const auto& s : seqs) {
        if (!first) {
            ss << "~";
        }
        first = false;
        ss << s;
    }
    return ss.str();
}

// in our case maximal tokens for std::uint16_t is 65535
std::vector<TokenType> convert_to_vector(std::string& dataset, const std::map<std::string, TokenType>& alphabet) {
    std::vector<TokenType> seq;
    seq.reserve(dataset.size()); // Reserve space
    for (auto x : dataset) {
        if (x == '\n') {
            x = '~';
        }
        if (alphabet.find(std::string(1, x)) != alphabet.end()) {
            seq.push_back(alphabet.at(std::string(1, x)));
        } else {
            seq.push_back(alphabet.at("[UNK]"));
        }
    }
    return seq;
}
