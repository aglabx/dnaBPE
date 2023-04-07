#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include "tokens_model.hpp"
#include "readers.hpp"
#include "tokens.hpp"
#include "preprocess.hpp"
#include "core.hpp"
#include "output.hpp"


std::vector<TokenType> get_data(std::string& file_name, std::string& format) {
    
    std::vector<std::string> seqs;

    std::cout << "Read data" << std::endl;
    if (format == "reads") {
        get_sequences_reads(file_name, seqs);
    } else if (format == "trf") {
        get_sequences_trf(file_name, seqs);
    } else if (format == "fasta") {
        get_sequences_fasta(file_name, seqs);
    } else {
        std::cout << "Format must be either reads or fasta or trf" << std::endl;
        exit(1);
    }
    std::cout << "Get dataset" << std::endl;
    std::string dataset = get_dataset(seqs);
    
    std::cout << "Convert to vector" << std::endl;
    return convert_to_vector(dataset, alphabet);
}

int main(int argc, char* argv[]) {

    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file_prefix> <format: reads, fasta, trf> <max_tokens> <threads>" << std::endl;
        return 1;
    }

    std::string file_name = argv[1];
    std::string output_prefix = argv[2];
    std::string format = argv[3];
    size_t max_tokens = std::stoul(argv[4]);
    size_t n_threads = std::stoul(argv[5]);
    std::string n_tokens_suffix = argv[4];


    const std::set<size_t> snapshot_points = {
        512,
        1024,
        2048,
        4096,
        8192,
        16384,
        32768,
    };

    if (max_tokens > MAX_N_TOKENS) {
        std::cout << "Max tokens must be less than 65535" << std::endl;
        return 1;
    }
    
    std::vector<TokenType> seq = get_data(file_name, format);

    std::vector<kmer> merged;
    uint k = 2;
    TokenType L = alphabet.size();
    std::unordered_map<TokenType, kmer> tokens;
    std::unordered_map<kmer, TokenType> rev_tokens;

    std::unordered_map<TokenType, size_t> token_to_length;

    std::vector<std::thread> threads;
    std::vector<TokenType> new_seq;
    std::vector<bool> to_replace(seq.size(), 0);

    std::unordered_map<TokenType, std::string> alphabet_map;
    std::unordered_map<TokenType, size_t> alphabet_tf_map;
    // fill with alphabet
    for (const auto& element : alphabet) {
        alphabet_map[element.second] = element.first;
        alphabet_tf_map[element.second] = 0;
    }

    std::vector<kmer> priority_queue;
    std::unordered_map<kmer, int> freqs;


    // precompute

    std::unordered_map<kmer, std::unordered_set<size_t>> cache;

    Counter c = count_pairs(seq, cache);
    for (const auto &item_tf : c) {
        priority_queue.push_back(item_tf.first);
        freqs[item_tf.first] = item_tf.second;
    }
    sort(priority_queue.begin(), priority_queue.end(), [&](const auto &a, const auto &b) {
        return freqs[a] < freqs[b];
    });
    int current_cutoff = freqs.at(priority_queue.front()) - 1; 
    int i = priority_queue.size() - 1;
    int pos = 0;

    std::string status;
    

    while (i >= 0) {

        // print priority_queue and freqs
        // std::cout << "Priority queue: ";
        // for (const auto &item_tf : priority_queue) {
        //     std::cout << alphabet_map.at(std::get<0>(item_tf)) << " " << alphabet_map.at(std::get<1>(item_tf)) << " " << freqs[item_tf] << std::endl;
        // }

        kmer rep = priority_queue.at(i);
        int tf = freqs.at(rep);
        if (tf < 2) {
            break;
        }
        
        
        // std::cin >> status;
        
        merged.push_back(rep);
        tokens[L] = rep;
        rev_tokens[rep] = L;

        TokenType a = std::get<0>(rep);
        TokenType b = std::get<1>(rep);

        std::string token_str = alphabet_map.at(a) + alphabet_map.at(b);
        alphabet_map[L] = token_str;
        alphabet_tf_map[L] = tf;
        token_to_length[L] = alphabet_map[L].size();

        std::cout << "Tokens " << L << " " << alphabet_map.at(std::get<0>(rep)) << " " << alphabet_map.at(std::get<1>(rep)) << " " << token_str << " " << tf << " : "<< "size: " << seq.size() << " cutoff: " << current_cutoff << " queue size: " << priority_queue.size() << std::endl;
        
        pos++;

        size_t prev_len = seq.size();

        Counter positive_c;
        Counter negative_c;

        replace(rep, seq, L, rev_tokens, positive_c, negative_c, to_replace, alphabet_map, cache);

        
        for (const auto& [pair, tf] : positive_c) {
            // print with freqs before and after
            // std::cout << "Positive " << alphabet_map.at(std::get<0>(pair)) << " " << alphabet_map.at(std::get<1>(pair)) << " " << tf << " " << freqs[pair] << std::endl;

            if (freqs.count(pair) == 0) {
                priority_queue.emplace_back(pair);
                freqs[pair] = 0;
            } 
            freqs.at(pair) += tf;          
        }

        for (const auto& [pair, tf] : negative_c) {
            // print with freqs before and after
            // std::cout << "Negative " << alphabet_map.at(std::get<0>(pair)) << " " << alphabet_map.at(std::get<1>(pair)) << " " << tf << " " << freqs.at(pair) << std::endl;
            if (freqs.count(pair) == 0) {
                freqs[pair] = 0;
            }
            freqs.at(pair) -= tf;
        }

        sort(priority_queue.begin(), priority_queue.end(), [&](const auto &a, const auto &b) {
            return freqs[a] < freqs[b];
        });

        priority_queue.erase(remove_if(priority_queue.begin(), priority_queue.end(), [&](const auto &x) {
            return freqs[x] <= current_cutoff;
        }), priority_queue.end());

        i = priority_queue.size() - 1;
        L += 1;

        std::cout << " new size: " << seq.size() << std::endl;

        if (snapshot_points.find(L) != snapshot_points.end()) {
            save_snapshot(tokens, seq, alphabet_map, alphabet_tf_map, output_prefix, std::to_string(L), false);
        }

        if (max_tokens && L > max_tokens) {
            break;
        }

        if (i == 0 || !priority_queue.size()) {
            // recompute
            priority_queue.clear();
            freqs.clear();
            std::cout << "Recomputing" << std::endl;
            Counter c = count_pairs(seq, cache);
            for (const auto &item_tf : c) {
                priority_queue.push_back(item_tf.first);
                freqs[item_tf.first] = item_tf.second;
            }
            sort(priority_queue.begin(), priority_queue.end(), [&](const auto &a, const auto &b) {
                return freqs[a] < freqs[b];
            });
            
            int num_items = std::min(1000, static_cast<int>(priority_queue.size()));
            int num_to_remove = priority_queue.size() - num_items;
            if (num_to_remove > 0) {
                priority_queue.erase(priority_queue.begin(), priority_queue.begin() + num_to_remove);
            }

            current_cutoff = freqs.at(priority_queue.front()) - 1;
            i = priority_queue.size() - 1;
        }

        // std::cin >> status;
    }

    save_snapshot(tokens, seq, alphabet_map, alphabet_tf_map, output_prefix, n_tokens_suffix, true);
    

    return 0;
}
