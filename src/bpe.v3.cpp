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
    std::vector<bool> to_replace(seq.size(), 0);

    std::unordered_map<TokenType, std::string> alphabet_map;
    std::unordered_map<TokenType, size_t> alphabet_tf_map;
    // fill with alphabet
    for (const auto& element : alphabet) {
        alphabet_map[element.second] = element.first;
        alphabet_tf_map[element.second] = 0;
    }

    // precompute

    std::cout << "Filling to SequenceContainer" << std::endl;
    SequenceContainer container(seq);
    seq.clear(); seq.resize(0);


    int pos = 0;

    std::string status;
    Kmer rep;
    size_t tf;

    while (true) {

        // print priority_queue and freqs
        // std::cout << "Priority queue: ";
        // for (const auto &item_tf : priority_queue) {
        //     std::cout << alphabet_map.at(std::get<0>(item_tf)) << " " << alphabet_map.at(std::get<1>(item_tf)) << " " << freqs[item_tf] << std::endl;
        // }

        std::tie(rep, tf) = container.get_most_frequent_pair();

        if (tf < 2) {
            break;
        }
        
        // std::cin >> status;
        
        //// Fill resulting structures
        merged.push_back(rep);
        tokens[L] = rep;
        rev_tokens[rep] = L;
        TokenType a = std::get<0>(rep);
        TokenType b = std::get<1>(rep);
        std::string token_str = alphabet_map.at(a) + alphabet_map.at(b);
        alphabet_map[L] = token_str;
        alphabet_tf_map[L] = tf;
        token_to_length[L] = alphabet_map[L].size();

        std::cout << "Tokens " << L << " " << alphabet_map.at(std::get<0>(rep)) << " " << alphabet_map.at(std::get<1>(rep)) << " " << token_str << " " << tf << " : "<< "size: " << container.size() << std::endl;
        pos++;

        size_t prev_len = seq.size();

        auto positions = container.get_positions(rep);

        std::cout << "Positions: " << positions.size() << std::endl;

        std::cout << "Collapsing..." << std::endl;
        for (size_t index: positions) {
            // print parameters of call and rep and L
            // std::cout << "index: " << index << " rep: " << alphabet_map.at(std::get<0>(rep)) << " " << alphabet_map.at(std::get<1>(rep)) << " L: " << L << std::endl;
            container.collapse(rep, index, L);
            // container.display(alphabet_map);
            // std::cout << "Continue?" << std::endl;
            // std::cin >> status;
        }

        L += 1;

        std::cout << " new size: " << seq.size() << std::endl;

        if (snapshot_points.find(L) != snapshot_points.end()) {
            save_snapshot(tokens, merged, rev_tokens, seq, alphabet_map, alphabet_tf_map, output_prefix, std::to_string(L), false);
        }

        if (max_tokens && L > max_tokens) {
            break;
        }

        // container.print_counter(alphabet_map);

        // std::cout << "Continue?";
        // std::cin >> status;
    }

    std::vector<TokenType> raw_seq = container.get_as_vector();

    save_snapshot(tokens, merged, rev_tokens, raw_seq, alphabet_map, alphabet_tf_map, output_prefix, std::to_string(L), true);
    

    return 0;
}
