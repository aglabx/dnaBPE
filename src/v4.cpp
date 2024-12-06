#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <bitset>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <nlohmann/json.hpp>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <fstream>
#include <unordered_set>
#include "reader.hpp"
#include "profiler.hpp"
#include "bpe.hpp"
#include "global.hpp"

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

void print_usage() {
    std::cerr << "Usage: ./bpe.exe <input_file> <output_file_prefix> <max_tokens> <threads>\n";
    std::cerr << "Example: ./bpe.exe input.txt output 1000 4\n";
    std::cerr << "Input format: one DNA sequence per line\n";
}

robin_hood::unordered_flat_map<std::string, double> ScopedProfiler::timings;

int main(int argc, char* argv[]) {

    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    if (argc != 5) {
        print_usage();
        return 1;
    }

    try {
        ScopedProfiler total_time("Total");
        
        std::string input_file = argv[1];
        std::string output_prefix = argv[2];
        int max_tokens = std::stoi(argv[3]);
        int num_threads = std::stoi(argv[4]);

        // Initialize tokenizer with vocabulary size
        DNABPETokenizer tokenizer(max_tokens);
        std::cerr << "Initialized tokenizer with max vocab size: " << max_tokens << std::endl;

        // Read all sequences at once
        SequenceReader reader(input_file);
        std::cerr << "File size: " << reader.get_file_size() << " bytes" << std::endl;

        // Train tokenizer
        {
            ScopedProfiler train_time("Training");
            std::cerr << "Training tokenizer..." << std::endl;
            auto start_time = std::chrono::high_resolution_clock::now();
            
            tokenizer.train(reader, max_tokens);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                end_time - start_time).count();
            std::cerr << "Finished training in " << duration << " seconds" << std::endl;
        }

        // Process sequences and save results
        {
            ScopedProfiler process_time("Processing");
            std::cerr << "Processing sequences and saving results..." << std::endl;
            auto start_time = std::chrono::high_resolution_clock::now();

            // Read all sequences again for processing
            auto all_sequences = reader.read_all_sequences();
            std::string sequence;
            sequence.reserve(1024);

            std::ofstream tokens_file(output_prefix + ".tokens");
            std::ofstream decoded_file(output_prefix + ".decoded");

            if (!tokens_file || !decoded_file) {
                throw std::runtime_error("Failed to open output files");
            }

            // Process the entire sequence
            for (const auto& node : all_sequences) {
                if (node.token_id < 4) {
                    sequence += "ACGT"[node.token_id];
                } else {
                    if (!sequence.empty()) {
                        auto tokens = tokenizer.tokenize(sequence);
                        auto decoded = tokenizer.decode(tokens);

                        for (int token : tokens) {
                            tokens_file << token << ' ';
                        }
                        tokens_file << '\n';
                        decoded_file << decoded << '\n';
                        sequence.clear();
                    }
                }
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                end_time - start_time).count();
            std::cerr << "Finished processing in " << duration << " seconds" << std::endl;
        }

        // Save tokenizer configuration
        {
            std::cerr << "Saving tokenizer configuration..." << std::endl;
            
            nlohmann::ordered_json model_json = {
                {"version", "1.0"},
                {"truncation", true},
                {"padding", true},
                {"vocab", nlohmann::ordered_json::object()},
                {"merges", nlohmann::json::array()},
                {"frequencies", nlohmann::ordered_json::object()}
            };

            // Use get_token_by_id instead of direct access to reverse_vocab
            for (int id = 0; id < tokenizer.get_vocab_size(); ++id) {
                const auto& token = tokenizer.get_token_by_id(id);
                model_json["vocab"][token] = id;
                model_json["frequencies"][token] = tokenizer.get_token_frequency(id);
            }

            // Add merges in order they were created
            for (const auto& [first, second] : tokenizer.get_merges()) {
                model_json["merges"].push_back({
                    tokenizer.get_token_by_id(first),
                    tokenizer.get_token_by_id(second)
                });
            }

            std::ofstream config_file(output_prefix + ".json");
            if (!config_file) {
                throw std::runtime_error("Failed to open config file");
            }
            config_file << model_json.dump(4);
        }

        std::cerr << "All operations completed successfully" << std::endl;
        ScopedProfiler::printReport();
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
