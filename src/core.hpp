#ifndef CORE_FILE_H
#define CORE_FILE_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include "tokens.hpp"
#include <iostream>
#include "container.hpp"

typedef std::unordered_map<Kmer, size_t> Counter;


Counter count_pairs(const std::vector<TokenType> &bseq, std::unordered_map<Kmer, std::unordered_set<size_t>>& cache) {
    Counter c;
    for (size_t i = 0; i < bseq.size() - 1; i++) {
        TokenType a = bseq[i];
        TokenType b = bseq[i + 1];
        if (a <= N_HELP_TOKENS || b <= N_HELP_TOKENS) {
            continue;
        }
        Kmer kmer_ = std::make_tuple(a, b);

        if (!cache.contains(kmer_)) {
            cache[kmer_] = std::unordered_set<size_t>();
        }    
        cache[kmer_].insert(i);
        c[kmer_]++;
    }
    return c;
}


void replace(const Kmer&token, std::vector<TokenType> &bseq, const TokenType L, const std::unordered_map<Kmer, TokenType>& rev_tokens, Counter& positive_c, Counter& negative_c, std::vector<bool>& to_replace, std::unordered_map<TokenType, std::string>& alphabet_map, uint k=2) {

    std::fill(to_replace.begin(), to_replace.end(), false);
    for (size_t i = 0; i < bseq.size() - k + 1; i++) {
        if (bseq[i] > N_HELP_TOKENS && bseq[i+1] > N_HELP_TOKENS) {
            Kmer kmer_ = std::make_tuple(bseq[i], bseq[i+1]);
            if (kmer_ == token) {
                to_replace[i] = true;
            }
        }
    }

    std::vector<TokenType> new_bseq;
    size_t i = 0;

    TokenType current_token = rev_tokens.at(token);
    std::string current_token_str = alphabet_map.at(current_token);

    std::cout << "String to replace " << current_token_str;

    for (size_t i = 0; i < bseq.size();) {
        if (to_replace[i]) {
            
            if (i > 0) {
                if (new_bseq.back() == current_token) {
                    // Do nothing
                } else {
                    if (bseq[i-1] > N_HELP_TOKENS) {
                        Kmer prev_neg = std::make_tuple(bseq[i - 1], std::get<0>(token));
                        negative_c[prev_neg]++;
                    }
                    
                }
            }

            

            TokenType next_token = bseq[i + 2];

            if (i + 2 < bseq.size()) {
                if (next_token > N_HELP_TOKENS) {
                    Kmer next_neg = std::make_tuple(std::get<1>(token), next_token);
                    negative_c[next_neg]++;
                }
            }

            if (!new_bseq.empty()) {
                if (new_bseq.back() > N_HELP_TOKENS) {
                        
                    Kmer prev_pair = std::make_tuple(new_bseq.back(), current_token);
                    if (prev_pair != token) {
                        positive_c[prev_pair]++;
                    }
                }
            }

            new_bseq.push_back(current_token);
            negative_c[token]++;

            if (i + 3 < bseq.size() && std::make_tuple(next_token, bseq[i + 3]) == token) {
                // Do nothing
            } else if (i + 2 < bseq.size() && next_token > N_HELP_TOKENS ) {
                Kmer next_pair = std::make_tuple(current_token, next_token);
                positive_c[next_pair]++;
            }
            i += 2;
            
            
        } else {
            new_bseq.push_back(bseq[i]);
            i += 1;
        }
    }

    bseq = std::vector<TokenType>(new_bseq.begin(), new_bseq.end());
    std::cout << " done." << std::endl;
}

Counter count_pairs(const std::vector<TokenType> &bseq) {
    Counter c;
    for (size_t i = 0; i < bseq.size() - 1; i++) {
        TokenType a = bseq[i];
        TokenType b = bseq[i + 1];
        if (a <= N_HELP_TOKENS || b <= N_HELP_TOKENS) {
            continue;
        }
        Kmer kmer_ = std::make_tuple(a, b);

        c[kmer_]++;
    }
    return c;
}

void compute_freqs_seq(const std::vector<TokenType>& seq, std::vector<std::atomic_size_t>& c, TokenType L) {
    for (size_t i = 0; i < seq.size() - 1; ++i) {
        if (seq[i] > N_HELP_TOKENS && seq[i + 1] > N_HELP_TOKENS) {
            ++c[seq[i] * L + seq[i + 1]];
        }
    }
}

void compute_freqs_par(const std::vector<TokenType>& seq, std::vector<std::atomic_size_t>& c, std::vector<std::thread>& threads, size_t n_threads, TokenType L) {
    
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
    threads.clear();
}

std::pair<std::size_t, Kmer> found_max(const std::vector<std::atomic_size_t>& c, TokenType L) {
    size_t max_count = 0;
    Kmer rep;
    for (size_t i=0; i < c.size(); ++i) {
        size_t current_count = c[i].load();
        if (current_count > max_count) {
            max_count = current_count;
            rep = std::make_tuple(i / L, i % L);
        }
    }
    return std::make_pair(max_count, rep);
}

void transform_data(std::vector<TokenType> &seq, std::vector<Kmer> &merged, std::unordered_map<TokenType, Kmer> &tokens, size_t max_tokens, std::vector<int>& to_replace, Kmer& rep, size_t tf, std::vector<TokenType>& new_seq,  TokenType L, uint k=2) {
    
    

    std::fill(to_replace.begin(), to_replace.end(), 0);
    for (size_t i = 0; i < seq.size() - k + 1; i++) {
        if (seq[i] > N_HELP_TOKENS && seq[i+1] > N_HELP_TOKENS) {
            Kmer kmer_ = std::make_tuple(seq[i], seq[i+1]);
            if (kmer_ == rep) {
                to_replace[i] = 1;
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

#endif