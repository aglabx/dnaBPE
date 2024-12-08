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
#include "robin_hood.h"

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
    robin_hood::unordered_map<std::string, uint32_t> vocab;  // строка -> id токена
    std::vector<std::pair<uint32_t, uint32_t>> merges;  // пары id токенов для мерджей
    VectorLinkedList current_sequence;  // Заменяем vector<VectorLinkedList> на один список
    int max_vocab_size;  // Removed const qualifier
    PairPriorityQueue pair_queue;  // Add this member

    void initial_count_pairs() {
        // ScopedProfiler prof("initial_count_pairs");
        pair_queue.clear();
        
        // First pass: count all pairs in a map and save positions
        robin_hood::unordered_map<std::pair<uint32_t, uint32_t>, size_t> pair_counts;
        
        // Setup progress bar for counting
        const size_t total_nodes = current_sequence.size();
        size_t nodes_processed = 0;
        int bar_width = 50;
        int last_percent = -1;
        
        std::cerr << "Processing sequence nodes:" << std::endl;
        
        uint64_t current_pos = current_sequence.get_head();
        while (current_pos != VectorLinkedList::END_MARKER) {
            nodes_processed++;
            int current_percent = (nodes_processed * 100) / total_nodes;
            
            if (current_percent != last_percent) {
                float progress = static_cast<float>(nodes_processed) / total_nodes;
                int pos = static_cast<int>(bar_width * progress);
                
                std::cerr << "\r[";
                for (int i = 0; i < bar_width; ++i) {
                    if (i < pos) std::cerr << "=";
                    else if (i == pos) std::cerr << ">";
                    else std::cerr << " ";
                }
                std::cerr << "] " << current_percent << "% "
                         << "(" << nodes_processed << "/" << total_nodes << ")\r";
                std::cerr.flush();
                last_percent = current_percent;
            }

            const VectorNode& node = current_sequence.get_node(current_pos);
            
            if (node.next_offset != VectorLinkedList::END_MARKER) {
                uint64_t next_pos = current_pos + node.next_offset;
                const VectorNode& next = current_sequence.get_node(next_pos);
                if (node.token_id != SEP_TOKEN_ID && next.token_id != SEP_TOKEN_ID) {  // Changed from current to node
                    auto token_pair = std::make_pair(node.token_id, next.token_id);
                    pair_counts[token_pair]++;
                    pair_queue.add_pair_position(node.token_id, next.token_id, current_pos);
                    // std::cerr << "Found pair: (" << node.token_id << "," << next.token_id 
                    //          << ") at position " << current_pos << std::endl;
                }
            }
            
            current_pos = (node.next_offset == VectorLinkedList::END_MARKER) ? 
                          VectorLinkedList::END_MARKER : current_pos + node.next_offset;
        }
        std::cerr << "\nNode processing completed." << std::endl;

        // std::cerr << "Debug: Found " << pair_counts.size() << " unique pairs" << std::endl;
        // for (const auto& [pair, count] : pair_counts) {
        //     std::cerr << "Pair (" << pair.first << "," << pair.second << ") count: " << count << std::endl;
        // }

        // Second pass: initialize priority queue with collected frequencies
        size_t pairs_processed = 0;
        const size_t total_pairs = pair_counts.size();
        bar_width = 50;
        last_percent = -1;

        for (const auto& [pair, freq] : pair_counts) {
            pairs_processed++;
            int current_percent = (pairs_processed * 100) / total_pairs;
            
            if (current_percent != last_percent) {
                float progress = static_cast<float>(pairs_processed) / total_pairs;
                int pos = static_cast<int>(bar_width * progress);
                
                std::cerr << "\rInitializing priority queue: [";
                for (int i = 0; i < bar_width; ++i) {
                    if (i < pos) std::cerr << "=";
                    else if (i == pos) std::cerr << ">";
                    else std::cerr << " ";
                }
                std::cerr << "] " << current_percent << "% "
                         << "(" << pairs_processed << "/" << total_pairs << ")\r";
                std::cerr.flush();
                last_percent = current_percent;
            }
            
            pair_queue.add_pair(pair.first, pair.second, freq);
        }
        std::cerr << "\nPriority queue initialization completed." << std::endl;
    }
    
    void update_token_frequencies(uint32_t left, uint32_t right, uint32_t new_id, size_t pair_freq) {
        // No need to resize since we pre-allocated
        global_token_frequencies[new_id] = pair_freq;
        
        // Decrease frequencies of constituent tokens
        if (left < BASE_VOCAB_SIZE) global_token_frequencies[left] -= pair_freq;
        if (right < BASE_VOCAB_SIZE) global_token_frequencies[right] -= pair_freq;
    }

    void update_train_progress(size_t current_vocab_size, int step, size_t freq, 
                             const std::string& left_token, const std::string& right_token) {
        const int bar_width = 50;
        float progress = static_cast<float>(current_vocab_size - BASE_VOCAB_SIZE) / 
                        (max_vocab_size - BASE_VOCAB_SIZE);
        int pos = static_cast<int>(bar_width * progress);

        auto format_token = [](const std::string& token) -> std::string {
            return token.length() > 3 ? token.substr(0, 3) + "..." : token;
        };

        std::string left = format_token(left_token);
        std::string right = format_token(right_token);

        // Форматируем строку прогресса с фиксированной шириной
        std::string progress_str = "\rTraining vocabulary: [";
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) progress_str += "=";
            else if (i == pos) progress_str += ">";
            else progress_str += " ";
        }
        
        char buf[256];
        snprintf(buf, sizeof(buf),
                "] %3d%% Vocab size: %zu/%-5d | Step: %-4d | Merging: '%-7s'+'%-7s' (freq: %-6zu)",
                int(progress * 100.0),
                current_vocab_size, max_vocab_size,
                step,
                left.c_str(), right.c_str(),
                freq);
        
        progress_str += buf;
        progress_str += "\r";
        std::cerr << progress_str << std::flush;
    }

    

    void apply_merges_batch(uint32_t left, uint32_t right, uint32_t new_id) {
        // ScopedProfiler prof("apply_merges_batch");
        
        const auto& positions = pair_queue.get_pair_positions(left, right);
        std::vector<uint64_t> merge_positions;
        
        // Select non-overlapping positions
        std::vector<bool> used(current_sequence.size(), false);
        for (size_t pos : positions) {
            VectorNode& node = current_sequence.get_node(pos);
            uint64_t next_pos = current_sequence.get_next_positions(pos);
            if (!used[pos] && !used[next_pos]) {
                merge_positions.push_back(pos);
                used[pos] = true;
                used[next_pos] = true;
            }
        }

        std::map<std::pair<uint32_t, uint32_t>, int64_t> differences;
        
        for (auto it = merge_positions.rbegin(); it != merge_positions.rend(); ++it) {
            uint64_t current_pos = *it;
            VectorNode& current = current_sequence.get_node(current_pos);
            uint64_t next_pos = current_sequence.get_next_positions(current_pos);
            
            uint32_t prev_token = current_sequence.get_prev_token(current_pos);
            uint32_t next_token = current_sequence.get_next_token(next_pos);

            // Remove old pairs' positions
            pair_queue.remove_pair_position(left, right, current_pos);
            if (prev_token != SEP_TOKEN_ID) {
                uint64_t prev_pos = current_sequence.get_prev_positions(current_pos);
                pair_queue.remove_pair_position(prev_token, left, prev_pos);
            }
            if (next_token != SEP_TOKEN_ID) {
                pair_queue.remove_pair_position(right, next_token, next_pos);
            }

            // Add new pairs' positions
            if (prev_token != SEP_TOKEN_ID) {
                uint64_t prev_pos = current_sequence.get_prev_positions(current_pos);;
                pair_queue.add_pair_position(prev_token, new_id, prev_pos);
            }
            if (next_token != SEP_TOKEN_ID) {
                pair_queue.add_pair_position(new_id, next_token, current_pos);
            }

            // Update frequencies using differences map
            differences[{left, right}]--;
            if (prev_token != SEP_TOKEN_ID) {
                differences[{prev_token, left}]--;
                differences[{prev_token, new_id}]++;
            }
            if (next_token != SEP_TOKEN_ID) {
                differences[{right, next_token}]--;
                differences[{new_id, next_token}]++;
            }

            if (!current_sequence.merge_nodes(current_pos, new_id)) {
                throw std::runtime_error("Failed to merge nodes");
            }
        }

        // Apply frequency differences
        for (const auto& [pair, diff] : differences) {
            if (diff < 0) {
                pair_queue.decrease_frequency(pair.first, pair.second, -diff);
            } else if (diff > 0) {
                pair_queue.increase_frequency(pair.first, pair.second, diff);
            }
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

    // Add move assignment operator
    DNABPETokenizer& operator=(DNABPETokenizer&& other) noexcept {
        if (this != &other) {
            vocab_strings = std::move(other.vocab_strings);
            vocab = std::move(other.vocab);
            merges = std::move(other.merges);
            current_sequence = std::move(other.current_sequence);
            max_vocab_size = other.max_vocab_size;
            pair_queue = std::move(other.pair_queue);
        }
        return *this;
    }

    // Публичные методы для внешнего доступа к базовому словарю
    static bool is_valid_token(char c) { return is_nucleotide(c); }
    static uint32_t get_base_token_id(char c) { return char_to_token_id(c); }
    static constexpr size_t get_sep_token_id() { return SEP_TOKEN_ID; }

    void train(SequenceReader& reader, int num_merges) {
        // ScopedProfiler prof("train");
        current_sequence = reader.read_all_sequences();
        // current_sequence.print();
        std::cout << "Data size: " << current_sequence.size() << std::endl;
        std::cerr << "Starting vocabulary training...\r" << std::flush;
        initial_count_pairs();
        std::cerr << "Initial pair counts completed." << std::endl;
        
        // pair_queue.print_heap();
        // pair_queue.print_frequencies();
        // pair_queue.print_positions();
        std::cerr << "Starting vocabulary training...\r" << std::flush;
        
        for (int i = 0; i < num_merges && vocab_strings.size() < max_vocab_size; ++i) {
            uint32_t left, right;
            size_t freq;
            bool found = pair_queue.get_most_frequent(left, right, freq);
            if (!found) {
                std::cerr << "\rNo more pairs to merge. Stopping training.\n" << std::flush;
                break;
            }
            std::pair<uint32_t, uint32_t> current_pair{left, right};
            
            std::string new_token = vocab_strings[left] + vocab_strings[right];
            uint32_t new_id = vocab_strings.size();
            vocab_strings.push_back(new_token);
            vocab[new_token] = new_id;

            update_token_frequencies(left, right, new_id, freq);
            merges.push_back({left, right});
            
            std::cerr << "Merging pair (" << vocab_strings[left] << "," << vocab_strings[right] 
                     << ") into token '" << new_token << "' (id=" << new_id << ") with frequency " << freq << std::endl;
            
            auto start_time = std::chrono::high_resolution_clock::now();
            apply_merges_batch(left, right, new_id);
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            std::cerr << "Merge operation completed in " << duration.count() << " ms," << " a new sequence size is " << current_sequence.size() << std::endl;

            // pair_queue.print_heap();
            // pair_queue.print_frequencies();
            // pair_queue.print_positions();
            
            // Очищаем предыдущую строку перед выводом нового прогресса
            // std::cerr << "\r" << std::string(120, ' ') << "\r" << std::flush;  // 120 пробелов для очистки
            // update_train_progress(vocab_strings.size(), i + 1, freq,
            //                     vocab_strings[left], vocab_strings[right]);
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
            uint64_t current = list.get_head();
            while (current != VectorLinkedList::END_MARKER) {
                VectorNode& node = list.get_node(current);
                if (node.next_offset != VectorLinkedList::END_MARKER) {
                    uint64_t next_pos = current + node.next_offset;
                    const VectorNode& next = list.get_node(next_pos);
                    if (node.token_id == first && next.token_id == second) {
                        list.merge_nodes(current, vocab[vocab_strings[first] + vocab_strings[second]]);
                    }
                }
                current = (node.next_offset == VectorLinkedList::END_MARKER) ? 
                          VectorLinkedList::END_MARKER : current + node.next_offset;
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