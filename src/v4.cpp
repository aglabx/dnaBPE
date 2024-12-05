#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp>  // Include the nlohmann/json library

using json = nlohmann::json;

// Nucleotide encoding and decoding mappings
std::unordered_map<char, uint8_t> nucleotide_encoding = {
    {'A', 0b00},
    {'C', 0b01},
    {'G', 0b10},
    {'T', 0b11}
};

std::unordered_map<uint8_t, char> nucleotide_decoding = {
    {0b00, 'A'},
    {0b01, 'C'},
    {0b10, 'G'},
    {0b11, 'T'}
};

std::vector<std::string> preprocess_dna(const std::string& sequence) {
    // Split the sequence at ambiguous nucleotides (e.g., 'N')
    std::vector<std::string> sequences;
    std::string current_seq;
    for (char c : sequence) {
        if (c == 'N') {
            if (!current_seq.empty()) {
                sequences.push_back(current_seq);
                current_seq.clear();
            }
        } else {
            current_seq += c;
        }
    }
    if (!current_seq.empty()) {
        sequences.push_back(current_seq);
    }
    return sequences;
}

uint64_t dna_to_bitstring(const std::string& sequence) {
    uint64_t bitstring = 0;
    for (char nucleotide : sequence) {
        bitstring = (bitstring << 2) | nucleotide_encoding[nucleotide];
    }
    return bitstring;
}

// Custom hash function for std::pair
struct pair_hash {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1,T2>& pair) const {
        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
};

void generate_kmers(uint64_t bitstring, size_t length, size_t max_k,
                    std::vector<std::pair<std::pair<uint64_t, size_t>, size_t>>& kmers) {
    for (size_t k = 2; k <= max_k; ++k) {
        uint64_t mask = (1ULL << (2 * k)) - 1;  // Mask to extract k nucleotides
        for (size_t i = 0; i <= length - k; ++i) {
            size_t shift = 2 * (length - k - i);
            uint64_t kmer = (bitstring >> shift) & mask;
            kmers.push_back({{kmer, k}, 1});  // Store k-mer and its length with count 1
        }
    }
}

void count_kmers(const std::vector<std::pair<std::pair<uint64_t, size_t>, size_t>>& kmers,
                 std::unordered_map<std::pair<uint64_t, size_t>, size_t, pair_hash>& kmer_counts) {
    for (const auto& kmer_count : kmers) {
        kmer_counts[kmer_count.first] += kmer_count.second;
    }
}

void build_bpe_vocab(const std::unordered_map<std::pair<uint64_t, size_t>, size_t, pair_hash>& kmer_counts,
                     size_t vocab_size,
                     std::unordered_map<std::pair<uint64_t, size_t>, uint32_t, pair_hash>& bpe_vocab,
                     std::unordered_map<uint32_t, std::pair<uint64_t, size_t>>& token_to_kmer,
                     std::unordered_map<uint32_t, size_t>& token_frequencies) {
    // Convert map to vector for sorting
    std::vector<std::pair<std::pair<uint64_t, size_t>, size_t>> kmer_counts_vec(kmer_counts.begin(), kmer_counts.end());
    // Sort k-mers by frequency in descending order
    std::sort(kmer_counts_vec.begin(), kmer_counts_vec.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    uint32_t token_id = 4;  // Starting token ID after nucleotides
    size_t count = 0;

    for (const auto& kmer_count : kmer_counts_vec) {
        if (count >= vocab_size) {
            break;
        }
        bpe_vocab[kmer_count.first] = token_id;
        token_to_kmer[token_id] = kmer_count.first;
        token_frequencies[token_id] = kmer_count.second;
        ++token_id;
        ++count;
    }
}

void encode_sequence(uint64_t bitstring, size_t length,
                     const std::unordered_map<std::pair<uint64_t, size_t>, uint32_t, pair_hash>& bpe_vocab,
                     size_t max_k,
                     std::vector<std::pair<uint32_t, size_t>>& encoded_sequence,
                     std::unordered_map<uint32_t, size_t>& total_token_frequencies) {
    size_t i = 0;
    while (i < length) {
        bool match_found = false;
        // Try to match the longest possible k-mer
        for (size_t k = max_k; k >= 2; --k) {
            if (i + k <= length) {
                size_t shift = 2 * (length - k - i);
                uint64_t kmer = (bitstring >> shift) & ((1ULL << (2 * k)) - 1);
                auto key = std::make_pair(kmer, k);
                auto it = bpe_vocab.find(key);
                if (it != bpe_vocab.end()) {
                    uint32_t token_id = it->second;
                    encoded_sequence.push_back({token_id, k});
                    total_token_frequencies[token_id]++;
                    i += k;
                    match_found = true;
                    break;
                }
            }
        }
        if (!match_found) {
            // Encode single nucleotide
            size_t shift = 2 * (length - i - 1);
            uint8_t nucleotide = (bitstring >> shift) & 0b11;
            encoded_sequence.push_back({nucleotide, 1});
            total_token_frequencies[nucleotide]++;
            i += 1;
        }
    }
}

std::string kmer_int_to_sequence(uint64_t kmer_int, size_t kmer_length) {
    std::string sequence;
    for (size_t i = 0; i < kmer_length; ++i) {
        size_t shift = 2 * (kmer_length - i - 1);
        uint8_t nucleotide_code = (kmer_int >> shift) & 0b11;
        char nucleotide = nucleotide_decoding[nucleotide_code];
        sequence += nucleotide;
    }
    return sequence;
}

std::string decode_sequence(const std::vector<std::pair<uint32_t, size_t>>& encoded_sequence,
                            const std::unordered_map<uint32_t, std::pair<uint64_t, size_t>>& token_to_kmer) {
    std::string decoded_sequence;
    for (const auto& token : encoded_sequence) {
        uint32_t token_id = token.first;
        size_t token_length = token.second;
        if (token_id < 4) {
            // Single nucleotide
            char nucleotide = nucleotide_decoding[token_id];
            decoded_sequence += nucleotide;
        } else {
            auto it = token_to_kmer.find(token_id);
            if (it != token_to_kmer.end()) {
                uint64_t kmer_int = it->second.first;
                size_t kmer_length = it->second.second;
                std::string kmer_sequence = kmer_int_to_sequence(kmer_int, kmer_length);
                decoded_sequence += kmer_sequence;
            }
        }
    }
    return decoded_sequence;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: bpe_tokenizer <dna_sequences_file> <output_file> <vocab_size>\n";
        return 1;
    }

    std::string dna_sequences_file = argv[1];
    std::string output_file = argv[2];
    size_t vocab_size = std::stoul(argv[3]);

    size_t max_k = 4;  // Maximum k-mer size

    // Read DNA sequences from file
    std::vector<std::string> preprocessed_sequences;

    std::ifstream infile(dna_sequences_file);
    if (!infile) {
        std::cerr << "Error opening file: " << dna_sequences_file << "\n";
        return 1;
    }

    std::string line;
    while (std::getline(infile, line)) {
        // Remove whitespace
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
        // Preprocess each line (DNA sequence)
        std::vector<std::string> sequences = preprocess_dna(line);
        preprocessed_sequences.insert(preprocessed_sequences.end(), sequences.begin(), sequences.end());
    }
    infile.close();

    if (preprocessed_sequences.empty()) {
        std::cerr << "No valid DNA sequences found in the input file.\n";
        return 1;
    }

    // Store all k-mers and sequence data
    std::vector<std::pair<std::pair<uint64_t, size_t>, size_t>> all_kmers;
    std::vector<uint64_t> bitstrings;
    std::vector<size_t> lengths;
    std::vector<std::string> sequences;

    // Process sequences to generate k-mers
    for (const auto& seq : preprocessed_sequences) {
        size_t length = seq.length();
        uint64_t bitstring = dna_to_bitstring(seq);
        bitstrings.push_back(bitstring);
        lengths.push_back(length);
        sequences.push_back(seq);

        std::vector<std::pair<std::pair<uint64_t, size_t>, size_t>> kmers;
        generate_kmers(bitstring, length, max_k, kmers);
        all_kmers.insert(all_kmers.end(), kmers.begin(), kmers.end());
    }

    // Count k-mers
    std::unordered_map<std::pair<uint64_t, size_t>, size_t, pair_hash> kmer_counts;
    count_kmers(all_kmers, kmer_counts);

    // Build BPE vocabulary
    std::unordered_map<std::pair<uint64_t, size_t>, uint32_t, pair_hash> bpe_vocab;
    std::unordered_map<uint32_t, std::pair<uint64_t, size_t>> token_to_kmer;
    std::unordered_map<uint32_t, size_t> token_frequencies;

    build_bpe_vocab(kmer_counts, vocab_size, bpe_vocab, token_to_kmer, token_frequencies);

    // Build vocabulary mapping from token strings to IDs
    std::unordered_map<std::string, uint32_t> vocab;

    // Add nucleotides to vocab
    vocab["A"] = nucleotide_encoding['A'];
    vocab["C"] = nucleotide_encoding['C'];
    vocab["G"] = nucleotide_encoding['G'];
    vocab["T"] = nucleotide_encoding['T'];

    // Add k-mers to vocab
    for (const auto& pair : token_to_kmer) {
        uint32_t token_id = pair.first;
        uint64_t kmer_int = pair.second.first;
        size_t kmer_length = pair.second.second;
        std::string kmer_sequence = kmer_int_to_sequence(kmer_int, kmer_length);
        vocab[kmer_sequence] = token_id;
    }

    // Encode sequences
    std::vector<std::vector<std::pair<uint32_t, size_t>>> tokenized_sequences;
    std::vector<std::string> decoded_sequences;
    std::unordered_map<uint32_t, size_t> total_token_frequencies;

    for (size_t idx = 0; idx < sequences.size(); ++idx) {
        std::vector<std::pair<uint32_t, size_t>> encoded_seq;
        encode_sequence(bitstrings[idx], lengths[idx], bpe_vocab, max_k, encoded_seq, total_token_frequencies);
        tokenized_sequences.push_back(encoded_seq);
        // Decode to verify correctness
        std::string decoded_seq = decode_sequence(encoded_seq, token_to_kmer);
        decoded_sequences.push_back(decoded_seq);
    }

    // Build JSON tokenizer
    json tokenizer_json;
    tokenizer_json["version"] = "1.0";
    tokenizer_json["truncation"] = nullptr;
    tokenizer_json["padding"] = nullptr;
    tokenizer_json["added_tokens"] = json::array();
    tokenizer_json["normalizer"] = nullptr;
    tokenizer_json["pre_tokenizer"] = nullptr;
    tokenizer_json["post_processor"] = nullptr;
    tokenizer_json["decoder"] = nullptr;

    json model;
    model["type"] = "BPE";
    model["unk_token"] = nullptr;
    model["continuing_subword_prefix"] = nullptr;
    model["end_of_word_suffix"] = nullptr;
    model["fuse_unk"] = false;

    json vocab_json;
    for (const auto& pair : vocab) {
        const std::string& token = pair.first;
        uint32_t token_id = pair.second;
        vocab_json[token] = token_id;
    }

    model["vocab"] = vocab_json;
    model["merges"] = json::array();  // Empty merges array since we didn't perform standard BPE merges

    tokenizer_json["model"] = model;

    // Write the JSON to the output file
    std::ofstream outfile(output_file);
    if (!outfile) {
        std::cerr << "Error opening output file: " << output_file << "\n";
        return 1;
    }
    outfile << tokenizer_json.dump(4);  // Pretty print with 4 spaces indent
    outfile.close();

    return 0;
}
