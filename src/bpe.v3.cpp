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
#include "container.hpp"
#include <chrono>


std::vector<TokenType> get_data(std::string& file_name, std::string& format, const std::unordered_map<std::string, TokenType>& alphabet) {
    
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
    return get_dataset(seqs, alphabet);
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
        // 512,
        // 1024,
        // 2048,
        // 4096,
        // 8192,
        // 16384,
        // 32768,
    };

    if (max_tokens > MAX_N_TOKENS) {
        std::cout << "Max tokens must be less than 65535" << std::endl;
        return 1;
    }
    
    std::vector<TokenType> seq = get_data(file_name, format, alphabet);

    // we keep kmer only in merged, in other places we use kmer_id
    std::vector<Kmer> merged;
    std::unordered_map<Kmer, size_t, TupleHash> kmer2kmer_id;
    std::unordered_map<size_t, Kmer> kmer_id2kmer;
    // set zero for start to mark collapsed nodes
    kmer2kmer_id[std::make_tuple(0, 0)] = 0;
    kmer_id2kmer[0] = std::make_tuple(0, 0);


    TokenType L = alphabet.size();

    std::unordered_map<TokenType, size_t> tokens;
    std::unordered_map<size_t, TokenType> rev_tokens;

    std::unordered_map<TokenType, size_t> token_to_length;

    std::vector<std::thread> threads;

    std::unordered_map<TokenType, std::string> alphabet_map;
    std::unordered_map<TokenType, size_t> alphabet_tf_map;
    // fill with alphabet
    for (const auto& element : alphabet) {
        alphabet_map[element.second] = element.first;
        alphabet_tf_map[element.second] = 0;
    }

    // precompute
    auto start_time = std::chrono::high_resolution_clock::now();
    std::cout << "Filling to SequenceContainer" << std::endl;    
    SequenceContainer container(seq, kmer2kmer_id, kmer_id2kmer, n_threads);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "Filling to SequenceContainer took " << duration << " ms" << std::endl;

    seq.clear(); seq.resize(0);

    int pos = 0;
    std::string status;
    size_t rep;
    size_t tf;
    
    while (true) {

        // std::cout << "Priority queue: ";
        // container.print_queue(alphabet_map, kmer_id2kmer);
        // container.display(alphabet_map, kmer_id2kmer);


        std::tie(rep, tf) = container.get_most_frequent_pair();

        if (tf < 2) {
            break;
        }
        if (max_tokens && L > max_tokens) {
            break;
        }
        if (L >= MAX_N_TOKENS) {
            break;
        }
        
        // std::cin >> status;
        
        //// Fill resulting structures
        Kmer rep_kmer = kmer_id2kmer.at(rep);
        merged.push_back(rep_kmer);
        tokens[L] = rep;
        rev_tokens[rep] = L;
        TokenType a = std::get<0>(rep_kmer);
        TokenType b = std::get<1>(rep_kmer);
        std::string token_str = alphabet_map.at(a) + alphabet_map.at(b);
        alphabet_map[L] = token_str;
        alphabet_tf_map[L] = tf;
        token_to_length[L] = alphabet_map[L].size();

        if (L < 1000 || (L < 50000 && L % 1000 == 0) || (L < 100000 && L % 10000 == 0) || (L < 1000000 && L % 100000 == 0) || (L < 10000000 && L % 1000000 == 0) || (L < 100000000 && L % 10000000 == 0) || (L % 100000000 == 0)) {
            
            end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

            std::cout << "Tokens " << L << " " << alphabet_map.at(std::get<0>(rep_kmer)) << " " << alphabet_map.at(std::get<1>(rep_kmer)) << " " << token_str << " " << tf << " : "<< "size: " << container.size() << " execution time: " << duration << " milliseconds" << std::endl;
            pos++;
            start_time = std::chrono::high_resolution_clock::now();
        } 

        container.collapse(rep, L, kmer2kmer_id, kmer_id2kmer, alphabet_map);

        

        L += 1;

        // std::cout << " new size: " << seq.size() << std::endl;

        // if (snapshot_points.find(L) != snapshot_points.end()) {
        //     save_snapshot(tokens, merged, kmer2kmer_id, rev_tokens, seq, alphabet_map, alphabet_tf_map, output_prefix, std::to_string(L), false);
        // }

        

        // container.print_counter(alphabet_map);

        // std::cout << "Continue?";
        // std::cin >> status;
    }


    std::vector<TokenType> raw_seq = container.get_as_vector(kmer_id2kmer);
    
    save_snapshot(tokens, merged, kmer2kmer_id, rev_tokens, raw_seq, alphabet_map, alphabet_tf_map, output_prefix, std::to_string(L), true);
    
    // container.diagnostic_print_of_state();

    std::string output_bpe_encoding_file = output_prefix + "." + std::to_string(L) + ".bpe";
    container.save_bpe_to_file(output_bpe_encoding_file, alphabet_map, kmer_id2kmer);

    std::cout << "Saving DONE" << std::endl;
    return 0;
}
