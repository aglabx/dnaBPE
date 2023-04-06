#pragma once

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <unordered_map>
#include "tokens.hpp"


typedef std::tuple<TokenType, TokenType> kmer;


void compute_freqs(const std::vector<TokenType>& seq, std::vector<std::atomic_size_t>& c, std::vector<std::thread>& threads, size_t n_threads, TokenType L) {
    auto worker = [&](size_t start, size_t end) {
        for (size_t i = start; i < end; ++i) {
            if (seq[i] > N_HELP_TOKENS && seq[i + 1] > N_HELP_TOKENS) {
                ++c[seq[i] * L + seq[i + 1]];
            }
        }
    };
    size_t chunk_size = (seq.size() - 1) / n_threads;

    for (size_t i = 0; i < n_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i == n_threads - 1) ? seq.size() - 1 : start + chunk_size;
        threads.emplace_back(worker, start, end);
    }

    for (auto &t : threads) {
        t.join();
    }
}

std::pair<std::size_t, kmer> found_max(const std::vector<std::atomic_size_t>& c, TokenType L) {
    size_t max_count = 0;
    kmer rep;
    for (size_t i=0; i < c.size(); ++i) {
        size_t current_count = c[i].load();
        if (current_count > max_count) {
            max_count = current_count;
            rep = std::make_tuple(i / L, i % L);
        }
    }
    return std::make_pair(max_count, rep);
}

void transform_data(std::vector<TokenType> &seq, std::vector<kmer> &merged, std::map<TokenType, kmer> &tokens, std::unordered_map<kmer, TokenType, tuple_hash<TokenType>> &rev_tokens, size_t max_tokens, std::vector<bool>& to_replace, kmer& rep, size_t tf, std::vector<TokenType>& new_seq,  TokenType L, uint k=2) {
    
    

    std::fill(to_replace.begin(), to_replace.end(), false);
    for (size_t i = 0; i < seq.size() - k + 1; i++) {
        if (seq[i] > N_HELP_TOKENS && seq[i+1] > N_HELP_TOKENS) {
            kmer kmer_ = std::make_tuple(seq[i], seq[i+1]);
            if (kmer_ == rep) {
                to_replace[i] = true;
            }
        }
    }
    
    new_seq.clear();
    new_seq.reserve(seq.size());
    for (size_t i = 0; i < seq.size();) {
        if (to_replace[i]) {
            new_seq.push_back(L);
            i += 2;
        } else {
            new_seq.push_back(seq[i]);
            i += 1;
        }
    }

    seq = std::vector<TokenType>(new_seq.begin(), new_seq.end());
    
}