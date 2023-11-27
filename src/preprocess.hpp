#ifndef PREPROCESS_FILE_H
#define PREPROCESS_FILE_H

#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <iostream>

#include "tokens.hpp"


std::vector<TokenType> get_dataset(const std::vector<std::string>& seqs, const std::unordered_map<std::string, TokenType>& alphabet) {

    size_t N = 0;
    for (auto& s : seqs) {
        N += s.size();
    } 
    N += seqs.size() - 1; // for ~
    std::vector<TokenType> seq;
    seq.reserve(N); // Reserve space
    std::string temp(1, '\0');
    size_t i = 0;
    for (auto& s : seqs) {
        for (auto x : s) {
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
            if (i && i % 100000000 == 0) {
                std::cout << " ... " << i << "/" << N << std::endl;
            }
            ++i;
        }
        temp[0] = '~';
        auto it = alphabet.find(temp);
        seq.emplace_back(it->second);
    }
    return seq;

}

#endif
