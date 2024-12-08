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
#include "compact_linked_list.hpp"
#include "global.hpp"

// First, define SequenceReader class (move it before DNABPETokenizer)
class SequenceReader {
protected:
    std::string file;
    uint64_t file_size;

public:
    SequenceReader(const std::string& filename) : file(filename), file_size(0) {}
    virtual ~SequenceReader() = default;
    
    // Change return type to CompactLinkedList instead of unique_ptr
    virtual CompactLinkedList read_all_sequences() = 0;
};

class SequenceReaderImpl : public SequenceReader {
private:
    std::ifstream file_stream;
    static constexpr size_t BUFFER_SIZE = 10 * 1024 * 1024; // Increase to 10MB buffer
    
    // Reusable buffers as class members
    std::vector<char> read_buffer;
    std::vector<uint32_t> node_buffer;

    void calculate_file_size() {
        file_stream.seekg(0, std::ios::end);
        file_size = file_stream.tellg();
        file_stream.seekg(0, std::ios::beg);
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
    SequenceReaderImpl(const std::string& filename) : SequenceReader(filename), file_stream(filename) {
        if (!file_stream) {
            throw std::runtime_error("Could not open file: " + filename);
        }
        calculate_file_size();
        
        // Pre-allocate buffers once
        read_buffer.reserve(BUFFER_SIZE);
        read_buffer.resize(BUFFER_SIZE);
        
        // Estimate node buffer size (worst case: one node per byte)
        node_buffer.reserve(BUFFER_SIZE);
    }

    CompactLinkedList read_all_sequences() override {
        CompactLinkedList list;
        list.init(file_size); // Pre-allocate for worst case        
        size_t total_bytes = 0;
        bool in_sequence = false;
        const size_t progress_interval = 10 * 1024 * 1024; // 10MB intervals
        size_t next_progress = progress_interval;

        std::cerr << "Starting to read sequences..." << std::endl;

        while (file_stream) {
            file_stream.read(read_buffer.data(), BUFFER_SIZE);
            std::streamsize bytes_read = file_stream.gcount();
            if (bytes_read <= 0) break;

            total_bytes += bytes_read;
            node_buffer.clear();

            for (std::streamsize i = 0; i < bytes_read; ++i) {
                char c = std::toupper(read_buffer[i]);
                
                if (c == 'A' || c == 'T' || c == 'G' || c == 'C') {
                    uint32_t token_id = TokenizerConstants::char_to_token_id(c);
                    node_buffer.emplace_back(token_id);
                    increase_token_frequency(token_id);
                    in_sequence = true;
                } else if (in_sequence) {
                    node_buffer.emplace_back(TokenizerConstants::SEP_TOKEN_ID);
                    increase_token_frequency(TokenizerConstants::SEP_TOKEN_ID);
                    in_sequence = false;
                }
            }

            if (!node_buffer.empty()) {
                list.push_direct(node_buffer);
            }

            if (total_bytes >= next_progress) {
                if (total_bytes - next_progress >= progress_interval) {
                    next_progress = total_bytes;
                }
                update_progress(total_bytes, file_size);
                next_progress += progress_interval;
            }
        }

        if (in_sequence) {
            node_buffer.clear();
            node_buffer.emplace_back(TokenizerConstants::SEP_TOKEN_ID);
            list.push_direct(node_buffer);
        }

        update_progress(file_size, file_size);
        std::cerr << "\nFinished reading " << list.size() << " tokens" << std::endl;

        return list;
    }

    size_t get_file_size() const { return file_size; }

    ~SequenceReaderImpl() {
        if (file_stream.is_open()) {
            file_stream.close();
        }
    }
};

#endif // DNA_BPE_READER_HPP
