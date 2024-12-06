#ifndef DNA_BPE_READER_HPP
#define DNA_BPE_READER_HPP


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
#include "linkedvector.hpp"
#include "global.hpp"

// First, define SequenceReader class (move it before DNABPETokenizer)
class SequenceReader {
private:
    std::ifstream file;
    size_t file_size;
    static constexpr size_t BUFFER_SIZE = 10 * 1024 * 1024; // Increase to 10MB buffer
    
    // Reusable buffers as class members
    std::vector<char> read_buffer;
    std::vector<VectorNode> node_buffer;

    void calculate_file_size() {
        file.seekg(0, std::ios::end);
        file_size = file.tellg();
        file.seekg(0, std::ios::beg);
    }

    void update_progress(size_t current, size_t total) {
        static size_t last_update = 0;
        static const size_t min_update_interval = 100000; // 100KB minimum between updates
        
        if (current - last_update < min_update_interval && current != total) {
            return;
        }
        last_update = current;

        const int bar_width = 50;
        float progress = static_cast<float>(current) / total;
        int pos = static_cast<int>(bar_width * progress);

        std::cerr << "\rReading sequences: [";
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) std::cerr << "=";
            else if (i == pos) std::cerr << ">";
            else std::cerr << " ";
        }
        std::cerr << "] " << int(progress * 100.0) << "% ";
        
        // Only show detailed progress for final update
        if (current == total) {
            std::cerr << current << "/" << total << " bytes\n";
        }
    }

public:
    SequenceReader(const std::string& filename) : file(filename), file_size(0) {
        if (!file) {
            throw std::runtime_error("Could not open file: " + filename);
        }
        calculate_file_size();
        
        // Pre-allocate buffers once
        read_buffer.reserve(BUFFER_SIZE);
        read_buffer.resize(BUFFER_SIZE);
        
        // Estimate node buffer size (worst case: one node per byte)
        node_buffer.reserve(BUFFER_SIZE);
    }

    VectorLinkedList read_all_sequences() {
        VectorLinkedList list;
        list.init(file_size); // Pre-allocate for worst case        
        size_t total_bytes = 0;
        bool first_sequence = true;
        const size_t progress_interval = 10 * 1024 * 1024; // Increase to 10MB intervals
        size_t next_progress = progress_interval;

        std::cerr << "Starting to read sequences..." << std::endl;

        while (file) {
            // Use pre-allocated buffer
            file.read(read_buffer.data(), BUFFER_SIZE);
            std::streamsize bytes_read = file.gcount();
            if (bytes_read <= 0) break;

            total_bytes += bytes_read;
            node_buffer.clear();  // Reuse existing buffer

            for (std::streamsize i = 0; i < bytes_read; ++i) {
                char c = read_buffer[i];
                uint32_t token_id;
                
                if (c == '\n') {
                    if (!first_sequence) {
                        token_id = TokenizerConstants::SEP_TOKEN_ID;
                        node_buffer.emplace_back(token_id);
                        increase_token_frequency(token_id);
                    }
                    first_sequence = false;
                } else if (c != '\r') {
                    token_id = TokenizerConstants::char_to_token_id(c);
                    node_buffer.emplace_back(token_id);
                    increase_token_frequency(token_id);
                }
            }

            if (!node_buffer.empty()) {
                list.push_direct(node_buffer);
            }

            if (total_bytes >= next_progress) {
                if (total_bytes - next_progress >= progress_interval) {
                    // Skip intermediate updates if we're far behind
                    next_progress = total_bytes;
                }
                update_progress(total_bytes, file_size);
                next_progress += progress_interval;
            }
        }

        // Add final separator if needed
        if (!first_sequence) {
            node_buffer.clear();
            node_buffer.emplace_back(TokenizerConstants::SEP_TOKEN_ID);
            list.push_direct(node_buffer);
        }

        update_progress(file_size, file_size);
        std::cerr << "\nFinished reading " << list.size() << " tokens" << std::endl;

        return list;
    }

    size_t get_file_size() const { return file_size; }

    ~SequenceReader() {
        if (file.is_open()) {
            file.close();
        }
    }
};

#endif // DNA_BPE_READER_HPP
