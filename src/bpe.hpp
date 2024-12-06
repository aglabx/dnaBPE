#ifndef BPE_HPP
#define BPE_HPP

#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <bitset>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <fstream>
#include <unordered_set>
#include "linkedvector.hpp"
#include "queue.hpp"
#include "reader.hpp"
#include "profiler.hpp"
#include "global.hpp"

class DNABPETokenizer {
private:
    static constexpr size_t BASE_VOCAB_SIZE = 5;
    static constexpr std::array<const char*, BASE_VOCAB_SIZE> BASE_VOCAB = {
        "A", "C", "G", "T", "<SEP>"
    };
    static constexpr std::array<char, 4> NUCLEOTIDES = {'A', 'C', 'G', 'T'};
    static constexpr size_t SEP_TOKEN_ID = 4;

    static bool is_nucleotide(char c) {
        return std::find(NUCLEOTIDES.begin(), NUCLEOTIDES.end(), c) != NUCLEOTIDES.end();
    }

    static uint32_t char_to_token_id(char c) {
        switch(c) {
            case 'A': return 0;
            case 'C': return 1;
            case 'G': return 2;
            case 'T': return 3;
            default: return SEP_TOKEN_ID;
        }
    }

    static char token_id_to_char(uint32_t id) {
        return id < 4 ? NUCLEOTIDES[id] : '\0';
    }

    struct IntPair {
        uint32_t first;
        uint32_t second;
        
        bool operator==(const IntPair& other) const {
            return first == other.first && second == other.second;
        }
    };
    
    struct IntPairHash {
        std::size_t operator()(const IntPair& pair) const {
            return std::hash<uint32_t>()(pair.first) ^ (std::hash<uint32_t>()(pair.second) << 1);
        }
    };

    std::vector<std::string> vocab_strings;  // индекс это id токена, значение - строка
    std::unordered_map<std::string, uint32_t> vocab;  // строка -> id токена
    std::vector<std::pair<uint32_t, uint32_t>> merges;  // пары id токенов для мерджей
    VectorLinkedList current_sequence;  // Заменяем vector<VectorLinkedList> на один список
    const int max_vocab_size;
    PairPriorityQueue pair_queue;  // Add this member

    void initial_count_pairs() {
        ScopedProfiler prof("initial_count_pairs");
        pair_queue.clear();
        
        // Use a temporary buffer for batch processing
        static constexpr size_t BATCH_SIZE = 10000;
        std::vector<std::pair<uint32_t, uint32_t>> pairs;
        pairs.reserve(BATCH_SIZE);
        
        // Count initial pairs
        for (auto it = current_sequence.begin(); it != current_sequence.end(); ++it) {
            const VectorNode& current = *it;
            if (current.next_idx != VectorLinkedList::END_MARKER) {
                const VectorNode& next = current_sequence.get_node(current.next_idx);
                if (current.token_id != SEP_TOKEN_ID && next.token_id != SEP_TOKEN_ID) {
                    pairs.emplace_back(current.token_id, next.token_id);
                    
                    // Process batch when full
                    if (pairs.size() >= BATCH_SIZE) {
                        for (const auto& [left, right] : pairs) {
                            pair_queue.add_pair(left, right);
                        }
                        pairs.clear();
                    }
                }
            }
        }
        
        // Process remaining pairs
        for (const auto& [left, right] : pairs) {
            pair_queue.add_pair(left, right);
        }
    }
    
    void update_token_frequencies(uint32_t left, uint32_t right, uint32_t new_id, size_t pair_freq) {
        // No need to resize since we pre-allocated
        global_token_frequencies[new_id] = pair_freq;
        
        // Decrease frequencies of constituent tokens
        if (left < BASE_VOCAB_SIZE) global_token_frequencies[left] -= pair_freq;
        if (right < BASE_VOCAB_SIZE) global_token_frequencies[right] -= pair_freq;
    }

    void update_pair_frequencies_after_merge(uint32_t current_idx, uint32_t new_token_id) {
        VectorNode& current = current_sequence.get_node(current_idx);
        uint32_t next_idx = current.next_idx;
        
        // Get tokens before and after the merged pair
        uint32_t prev_token = current_sequence.get_prev_token(current_idx);
        uint32_t next_token = current_sequence.get_next_token(next_idx);

        // Remove the original pair that was merged
        pair_queue.decrease_frequency(current.token_id, current_sequence.get_node(next_idx).token_id);
        
        // Update frequencies for the previous token's pairs
        if (prev_token != SEP_TOKEN_ID) {
            // Remove old pair (prev - first)
            pair_queue.decrease_frequency(prev_token, current.token_id);
            // Add new pair (prev - new)
            pair_queue.add_pair(prev_token, new_token_id);
        }

        // Update frequencies for the next token's pairs
        if (next_token != SEP_TOKEN_ID) {
            // Remove old pair (second - next)
            pair_queue.decrease_frequency(current_sequence.get_node(next_idx).token_id, next_token);
            // Add new pair (new - next)
            pair_queue.add_pair(new_token_id, next_token);
        }
    }

    void apply_merge_to_list(VectorLinkedList& list, const IntPair& merge_pair, uint32_t new_token_id) {
        uint32_t current = list.get_head();
        while (current != VectorLinkedList::END_MARKER) {
            VectorNode& node = list.get_node(current);
            if (node.next_idx != VectorLinkedList::END_MARKER) {
                const VectorNode& next = list.get_node(node.next_idx);
                // Пропускаем мерджи где участвует SEP токен
                if (node.token_id != 4 && next.token_id != 4 && 
                    node.token_id == merge_pair.first && 
                    next.token_id == merge_pair.second) {
                    list.merge_nodes(current, new_token_id);
                }
            }
            current = node.next_idx;
        }
    }

    void update_train_progress(size_t current_vocab_size) {
        const int bar_width = 50;
        float progress = static_cast<float>(current_vocab_size - BASE_VOCAB_SIZE) / 
                        (max_vocab_size - BASE_VOCAB_SIZE);
        int pos = static_cast<int>(bar_width * progress);

        std::cerr << "\rTraining vocabulary: [";
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) std::cerr << "=";
            else if (i == pos) std::cerr << ">";
            else std::cerr << " ";
        }
        std::cerr << "] " << int(progress * 100.0) << "% "
                 << "Vocab size: " << current_vocab_size << "/" << max_vocab_size << "\r";
        std::cerr.flush();
    }

    void apply_merges_batch(uint32_t left, uint32_t right, uint32_t new_id) {
        ScopedProfiler prof("apply_merges_batch");
        std::vector<uint32_t> merge_positions;
        uint32_t current = current_sequence.get_head();
        
        // First pass: collect all positions where we can merge
        while (current != VectorLinkedList::END_MARKER) {
            VectorNode& node = current_sequence.get_node(current);
            if (node.next_idx != VectorLinkedList::END_MARKER) {
                const VectorNode& next = current_sequence.get_node(node.next_idx);
                if (node.token_id == left && next.token_id == right) {
                    merge_positions.push_back(current);
                }
            }
            current = node.next_idx;
        }
        
        // Second pass: apply merges from back to front to avoid position shifts
        for (auto it = merge_positions.rbegin(); it != merge_positions.rend(); ++it) {
            uint32_t pos = *it;
            update_pair_frequencies_after_merge(pos, new_id);
            current_sequence.merge_nodes(pos, new_id);
        }
    }

public:

    

    DNABPETokenizer(int max_size = 1000) : max_vocab_size(max_size) {
        // Pre-allocate everything at initialization
        vocab_strings.reserve(max_size);
        init_token_frequencies(max_size);  // Pre-allocate all frequencies at once
        
        for (size_t i = 0; i < BASE_VOCAB_SIZE; ++i) {
            vocab_strings.push_back(BASE_VOCAB[i]);
            vocab[BASE_VOCAB[i]] = i;
        }
    }

    

    // Публичные методы для внешнего доступа к базовому словарю
    static bool is_valid_token(char c) { return is_nucleotide(c); }
    static uint32_t get_base_token_id(char c) { return char_to_token_id(c); }
    static constexpr size_t get_sep_token_id() { return SEP_TOKEN_ID; }

    void train(SequenceReader& reader, int num_merges) {
        ScopedProfiler prof("train");
        current_sequence = reader.read_all_sequences();
        std::cerr << "Starting vocabulary training..." << std::endl;
        update_train_progress(vocab_strings.size());
        
        initial_count_pairs();
        std::unordered_set<std::pair<uint32_t, uint32_t>> used_merges;
        
        for (int i = 0; i < num_merges && vocab_strings.size() < max_vocab_size; ++i) {
            uint32_t left, right;
            size_t freq;
            
            bool found_valid_pair = false;
            while (!found_valid_pair && pair_queue.get_most_frequent(left, right, freq)) {
                std::pair<uint32_t, uint32_t> current_pair{left, right};
                if (used_merges.count(current_pair) == 0) {
                    found_valid_pair = true;
                }
            }
            
            if (!found_valid_pair) break;

            used_merges.insert({left, right});
            
            std::string new_token = vocab_strings[left] + vocab_strings[right];
            uint32_t new_id = vocab_strings.size();
            vocab_strings.push_back(new_token);
            vocab[new_token] = new_id;
            update_token_frequencies(left, right, new_id, freq);  // Update frequencies here
            merges.push_back({left, right});

            // Apply merges and update frequencies
            apply_merges_batch(left, right, new_id);
            
            update_train_progress(vocab_strings.size());
        }
        
        std::cerr << "\nVocabulary training completed. Final size: " 
                  << vocab_strings.size() << " tokens" << std::endl;
    }

    std::vector<int> tokenize(const std::string& sequence) {
        VectorLinkedList list;
        list.init(sequence.length());
        
        // Convert to initial tokens
        for (char c : sequence) {
            list.append(vocab[std::string(1, c)]);
        }
        
        // Apply merges
        for (const auto& [first, second] : merges) {
            uint32_t current = list.get_head();
            while (current != VectorLinkedList::END_MARKER) {
                VectorNode& node = list.get_node(current);
                if (node.next_idx != VectorLinkedList::END_MARKER) {
                    const VectorNode& next = list.get_node(node.next_idx);
                    if (node.token_id == first && next.token_id == second) {
                        list.merge_nodes(current, vocab[vocab_strings[first] + vocab_strings[second]]);
                    }
                }
                current = node.next_idx;
            }
        }
        
        // Convert to vector
        std::vector<int> result;
        for (const auto& node : list) {
            result.push_back(node.token_id);
        }
        return result;
    }

    std::string decode(const std::vector<int>& token_ids) {
        std::string result;
        for (int id : token_ids) {
            if (id < vocab_strings.size()) {
                result += vocab_strings[id];
            }
        }
        return result;
    }

    const auto& get_vocab() const { return vocab; }
    const auto& get_merges() const { return merges; }
    size_t get_token_frequency(uint32_t token_id) const { 
        return ::get_token_frequency(token_id);  // Use global function
    }
    int get_vocab_size() const { return vocab_strings.size(); }
    
    // Add getter for token by id
    const std::string& get_token_by_id(int id) const { 
        return vocab_strings[id]; 
    }
};

#endif // BPE_HPP