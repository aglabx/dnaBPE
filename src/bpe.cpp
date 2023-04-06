#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

#include "../nlohmann/json.hpp"
#include "tokens_model.hpp"
#include "readers.hpp"
#include "tokens.hpp"
#include "preprocess.hpp"
#include "core.hpp"

using json = nlohmann::json;


std::vector<TokenType> get_data(std::string file_name, std::string format) {
    
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

    std::string output_model_file = output_prefix + ".json";
    std::string output_poses_file = output_prefix + ".poses";
    std::string output_bpe_encoding_file = output_prefix + ".bpe";

    if (max_tokens > MAX_N_TOKENS) {
        std::cout << "Max tokens must be less than 65535" << std::endl;
        return 1;
    }
    
    std::vector<TokenType> seq = get_data(file_name, format);

    std::vector<kmer> merged;
    uint k = 2;
    TokenType L = alphabet.size();
    std::map<TokenType, kmer> tokens;
    std::unordered_map<TokenType, size_t> token_to_length;

    std::vector<std::thread> threads;
    std::vector<TokenType> new_seq;
    std::vector<bool> to_replace(seq.size(), false);

    while (true) {
        std::cout << "Tokens " << L;
        
        std::vector<std::atomic_size_t> c(L * L);
        for (auto &elem : c) {
            elem.store(0);
        }

        std::cout << " compute freqs";
        compute_freqs(seq, c, threads, n_threads, L);

        std::cout << " find max";
        auto max_result = found_max(c, L);
        size_t tf = max_result.first;
        kmer rep = max_result.second;
        if (tf == 1) {
            break;
        }

        merged.push_back(rep);
        tokens[L] = rep;

        std::cout << " transform data" << std::endl;
        transform_data(seq, merged, tokens, max_tokens, to_replace, rep, tf, new_seq, L, k);

        std::string token1 = token_type_to_string(std::get<0>(rep), alphabet, tokens);
        std::string token2 = token_type_to_string(std::get<1>(rep), alphabet, tokens);

        token_to_length[L] = token1.size() + token2.size();

        std::cout << token1 << " " << token2 << " " << tf << " : "<< "replace: " << seq.size() << " -> " << new_seq.size() << std::endl;

        L += 1;
        if (max_tokens && L > max_tokens) {
            break;
        }
    }
    
    // compute tokens as strings
    std::map<TokenType, std::string> tokens_str_map;
    for (const auto& element : tokens) {
        std::string token_string = token_type_to_string(element.first, alphabet, tokens);
        tokens_str_map[element.first] = token_string;
    }
    for (const auto& element : alphabet) {
        tokens_str_map[element.second] = element.first;
    }

    
    std::map<std::string, std::vector<std::pair<size_t, size_t>>> kmer2poses;
    // init kmer2poses with empty vectors and keys from tokens_str_map
    for (const auto& element : tokens_str_map) {
        kmer2poses[element.second] = std::vector<std::pair<size_t, size_t>>();
    }
    
    
    std::ofstream out_file(output_bpe_encoding_file);
    size_t pos = 0;
    size_t seqid = 0;
    if (out_file.is_open()) {
        for (const auto& element : seq) {
            if (element == 5) {
                seqid += 1;
                pos = 0;
                out_file << "\n";
            } else {
                std::string token_string = tokens_str_map.at(element);
                kmer2poses[token_string].push_back(std::make_pair(seqid, pos));
                out_file << token_string << " ";
                pos += token_string.size();
            }
        }
        out_file << std::endl;
        out_file.close();
    }

    std::ofstream poses_file(output_poses_file);
    // write poses to file
    for (const auto& element : kmer2poses) {
        poses_file << element.first << " ";
        for (const auto& pos : element.second) {
            poses_file << pos.first << ":" << pos.second << " ";
        }
        poses_file << std::endl;
    }

    poses_file.close();

    nlohmann::ordered_json json_data = get_json(tokens_str_map, tokens);

    std::ofstream configFile(output_model_file);
    configFile << std::setw(2) << json_data << std::endl;
    configFile.close();

    std::cout << "Config saved to config.json" << std::endl;

    return 0;
}
