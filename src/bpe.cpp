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

    std::vector<Kmer> merged;
    uint k = 2;
    TokenType L = alphabet.size();
    std::unordered_map<TokenType, Kmer> tokens;
    std::unordered_map<Kmer, TokenType> rev_tokens;

    std::unordered_map<TokenType, size_t> token_to_length;

    std::vector<std::thread> threads;
    std::vector<TokenType> new_seq;
    std::vector<bool> to_replace(seq.size(), false);

    std::unordered_map<TokenType, std::string> alphabet_map;
    std::unordered_map<TokenType, size_t> alphabet_tf_map;
    // fill with alphabet
    for (const auto& element : alphabet) {
        alphabet_map[element.second] = element.first;
        alphabet_tf_map[element.second] = 0;
    }


    while (true) {
        std::cout << "Tokens " << L;
        
        std::vector<std::atomic_size_t> c(L * L);
        for (auto &elem : c) {
            elem.store(0);
        }

        if (seq.size() > 1000000) {
            std::cout << " par compute freqs";
            compute_freqs_par(seq, c, threads, n_threads, L);
        } else {
            std::cout << " seq compute freqs";
            compute_freqs_seq(seq, c, L);
        }

        std::cout << " find max";
        auto max_result = found_max(c, L);
        size_t tf = max_result.first;
        Kmer rep = max_result.second;
        if (tf == 1) {
            break;
        }

        merged.push_back(rep);
        tokens[L] = rep;
        rev_tokens[rep] = L;

        std::string token_str = alphabet_map.at(std::get<0>(rep)) + alphabet_map.at(std::get<1>(rep));
        alphabet_map[L] = token_str;
        alphabet_tf_map[L] = tf;

        std::cout << " transform data" << std::endl;
        transform_data(seq, merged, tokens, max_tokens, to_replace, rep, tf, new_seq, L, k);

        token_to_length[L] = alphabet_map[L].size();

        std::cout << alphabet_map.at(std::get<0>(rep)) << " " << alphabet_map.at(std::get<1>(rep)) << " " << tf << " : "<< "size: " << seq.size() << std::endl;

        L += 1;

        // check that L in snapshot_points and then save resutls, and provide correct n_tokens_suffix
        if (snapshot_points.find(L) != snapshot_points.end()) {
            save_snapshot(tokens, seq, alphabet_map, alphabet_tf_map, output_prefix, std::to_string(L), false);
        }

        if (max_tokens && L > max_tokens) {
            break;
        }
    }

    save_snapshot(tokens, seq, alphabet_map, alphabet_tf_map, output_prefix, n_tokens_suffix, true);

    return 0;
}
