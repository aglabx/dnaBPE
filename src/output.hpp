#ifndef OUTPUT_FILE_H
#define OUTPUT_FILE_H

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>

#include "tokens.hpp"
#include "tokens_model.hpp"

#include "../nlohmann/json.hpp"

using json = nlohmann::json;

void save_snapshot(const std::map<TokenType, kmer>& tokens, const std::vector<TokenType>& seq, const std::map<TokenType, std::string>& alphabet_map, std::map<TokenType, size_t>& alphabet_tf_map, const std::string& output_prefix, std::string n_tokens_suffix,  bool save_seq=false) {

    std::string output_bpe_encoding_file = output_prefix + n_tokens_suffix + ".bpe";
    std::string output_poses_file = output_prefix + n_tokens_suffix + ".poses";
    std::string output_model_file = output_prefix + n_tokens_suffix + ".json";

    std::map<std::string, std::vector<std::pair<size_t, size_t>>> kmer2poses;
    std::map<std::string, size_t> kmer2tf;
    // init kmer2poses with empty vectors and keys from alphabet_map
    for (const auto& element : alphabet_map) {
        kmer2poses[element.second] = std::vector<std::pair<size_t, size_t>>();
        kmer2tf[element.second] = 0;
    }

    size_t pos = 0;
        size_t seqid = 0;
        for (const auto& element : seq) {
            if (element == 5) {
                seqid += 1;
                pos = 0;
            } else {
                std::string token_string = alphabet_map.at(element);
                kmer2poses.at(token_string).push_back(std::make_pair(seqid, pos));
                kmer2tf.at(token_string) += 1;
                pos += token_string.size();
            }
        }
    
    if (save_seq) {
        std::ofstream out_file(output_bpe_encoding_file);
        if (out_file.is_open()) {
            for (const auto& element : seq) {
                if (element == 5) {
                    out_file << "\n";
                } else {
                    std::string token_string = alphabet_map.at(element);
                    out_file << token_string << " ";
                    pos += token_string.size();
                }
            }
            out_file << std::endl;
            out_file.close();
        }
    } 

    std::ofstream poses_file(output_poses_file);
    // write poses to file and add the secone argument tf from kmer2tf
    for (const auto& [token, kmer] : alphabet_map) {
        poses_file << kmer << "\t" << alphabet_tf_map.at(token) << "\t" << kmer2tf.at(kmer) << "\t";
        for (const auto& pos : kmer2poses.at(kmer)) {
            poses_file << pos.first << ":" << pos.second << " ";
        }
        poses_file << std::endl;
    }
    poses_file.close();

    nlohmann::ordered_json json_data = get_json(alphabet_map, tokens);

    std::ofstream configFile(output_model_file);
    configFile << std::setw(2) << json_data << std::endl;
    configFile.close();

    std::cout << "Config saved to config.json" << std::endl;
}

#endif