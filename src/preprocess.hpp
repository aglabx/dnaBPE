#ifndef PREPROCESS_FILE_H
#define PREPROCESS_FILE_H

#include <string>
#include <vector>
#include <sstream>
#include "tokens.hpp"


std::string get_dataset(const std::vector<std::string>& seqs) {
    std::stringstream ss;
    bool first = true;
    for (auto& s : seqs) {
        if (!first) {
            ss << "~";
        }
        first = false;
        ss << s;
    }
    return ss.str();
}

// in our case maximal tokens for std::uint16_t is 65535
std::vector<TokenType> convert_to_vector(const std::string& dataset, const std::map<std::string, TokenType>& alphabet) {
    std::vector<TokenType> seq;
    seq.reserve(dataset.size()); // Reserve space

    std::string temp(1, '\0'); // Temporary string for lookup
    for (auto x : dataset) {
        if (x == '\n') {
            x = '~';
        }
        temp[0] = x;
        auto it = alphabet.find(temp);
        if (it != alphabet.end()) {
            seq.emplace_back(it->second);
        } else {
            seq.emplace_back(alphabet.at("[UNK]"));
        }
    }
    return seq;
}

#endif
