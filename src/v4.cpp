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

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

namespace TokenizerConstants {
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
}

// Add new structures at the top of the file
struct VectorNode {
    uint32_t token_id;
    uint32_t next_idx;
    
    VectorNode(uint32_t id = 0, uint32_t next = UINT32_MAX) 
        : token_id(id), next_idx(next) {}
};

class VectorLinkedList {
private:
    std::vector<VectorNode> nodes;
    uint32_t head_idx;
    uint32_t size_;

public:
    static constexpr uint32_t END_MARKER = UINT32_MAX;
    
    VectorLinkedList() : head_idx(END_MARKER), size_(0) {}
    
    void init(size_t capacity) {
        nodes.reserve(capacity);
    }
    
    uint32_t push_front(uint32_t token_id) {
        uint32_t new_idx = nodes.size();
        nodes.emplace_back(token_id, head_idx);
        head_idx = new_idx;
        size_++;
        return new_idx;
    }
    
    void append(uint32_t token_id) {
        if (head_idx == END_MARKER) {
            push_front(token_id);
            return;
        }
        
        uint32_t current = head_idx;
        while (nodes[current].next_idx != END_MARKER) {
            current = nodes[current].next_idx;
        }
        
        uint32_t new_idx = nodes.size();
        nodes.emplace_back(token_id, END_MARKER);
        nodes[current].next_idx = new_idx;
        size_++;
    }
    
    // Merge two consecutive nodes
    bool merge_nodes(uint32_t first_idx, uint32_t new_token_id) {
        if (first_idx >= nodes.size() || 
            nodes[first_idx].next_idx == END_MARKER ||
            nodes[first_idx].next_idx >= nodes.size()) {
            return false;
        }
        
        uint32_t second_idx = nodes[first_idx].next_idx;
        nodes[first_idx].token_id = new_token_id;
        nodes[first_idx].next_idx = nodes[second_idx].next_idx;
        size_--;
        return true;
    }
    
    uint32_t get_head() const { return head_idx; }
    size_t size() const { return size_; }
    const VectorNode& get_node(uint32_t idx) const { return nodes[idx]; }
    VectorNode& get_node(uint32_t idx) { return nodes[idx]; }
    
    // Iterator support
    class Iterator {
    private:
        const VectorLinkedList* list;
        uint32_t current_idx;
        
    public:
        Iterator(const VectorLinkedList* l, uint32_t start) 
            : list(l), current_idx(start) {}
        
        bool operator!=(const Iterator& other) const {
            return current_idx != other.current_idx;
        }
        
        Iterator& operator++() {
            if (current_idx != END_MARKER) {
                current_idx = list->nodes[current_idx].next_idx;
            }
            return *this;
        }
        
        const VectorNode& operator*() const {
            return list->nodes[current_idx];
        }
    };
    
    Iterator begin() const { return Iterator(this, head_idx); }
    Iterator end() const { return Iterator(this, END_MARKER); }

    // Add new direct push method
    void push_direct(const std::vector<VectorNode>& new_nodes) {
        size_t old_size = nodes.size();
        nodes.insert(nodes.end(), new_nodes.begin(), new_nodes.end());
        
        if (head_idx == END_MARKER) {
            head_idx = old_size;
        } else {
            // Find last node and link it
            uint32_t current = head_idx;
            while (nodes[current].next_idx != END_MARKER) {
                current = nodes[current].next_idx;
            }
            nodes[current].next_idx = old_size;
        }
        
        // Link new nodes
        for (size_t i = old_size; i < nodes.size() - 1; ++i) {
            nodes[i].next_idx = i + 1;
        }
        nodes.back().next_idx = END_MARKER;
        size_ += new_nodes.size();
    }
};

class SequenceReader {
private:
    std::ifstream file;
    size_t file_size;
    static constexpr size_t BUFFER_SIZE = 1024 * 1024; // 1MB buffer

    void calculate_file_size() {
        file.seekg(0, std::ios::end);
        file_size = file.tellg();
        file.seekg(0, std::ios::beg);
    }

    void update_progress(size_t current, size_t total) {
        const int bar_width = 50;
        float progress = static_cast<float>(current) / total;
        int pos = static_cast<int>(bar_width * progress);

        std::cerr << "\rReading sequences: [";
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) std::cerr << "=";
            else if (i == pos) std::cerr << ">";
            else std::cerr << " ";
        }
        std::cerr << "] " << int(progress * 100.0) << "% "
                 << current << "/" << total << " bytes\r";
        std::cerr.flush();
    }

public:
    SequenceReader(const std::string& filename) : file(filename), file_size(0) {
        if (!file) {
            throw std::runtime_error("Could not open file: " + filename);
        }
        calculate_file_size();
    }

    VectorLinkedList read_all_sequences() {
        VectorLinkedList list;
        list.init(file_size); // Pre-allocate for worst case
        
        // Буфер для эффективного чтения
        std::vector<char> buffer(BUFFER_SIZE);
        std::vector<VectorNode> temp_nodes;
        temp_nodes.reserve(BUFFER_SIZE); // Предварительное выделение памяти
        
        size_t total_bytes = 0;
        bool first_sequence = true;
        const size_t progress_interval = 1024 * 1024; // 1MB
        size_t next_progress = progress_interval;

        std::cerr << "Starting to read sequences..." << std::endl;
        update_progress(0, file_size);

        while (file) {
            file.read(buffer.data(), BUFFER_SIZE);
            std::streamsize bytes_read = file.gcount();
            if (bytes_read <= 0) break;

            total_bytes += bytes_read;
            temp_nodes.clear();

            for (std::streamsize i = 0; i < bytes_read; ++i) {
                char c = buffer[i];
                if (c == '\n') {
                    if (!first_sequence) {
                        temp_nodes.emplace_back(TokenizerConstants::SEP_TOKEN_ID);
                    }
                    first_sequence = false;
                } else if (c != '\r') { // Skip carriage returns
                    if (TokenizerConstants::is_nucleotide(c)) {
                        temp_nodes.emplace_back(TokenizerConstants::char_to_token_id(c));
                    } else {
                        temp_nodes.emplace_back(TokenizerConstants::SEP_TOKEN_ID);
                    }
                }
            }

            if (!temp_nodes.empty()) {
                list.push_direct(temp_nodes);
            }

            if (total_bytes >= next_progress) {
                update_progress(total_bytes, file_size);
                next_progress = total_bytes + progress_interval;
            }
        }

        // Add final separator if needed
        if (!first_sequence) {
            temp_nodes.clear();
            temp_nodes.emplace_back(TokenizerConstants::SEP_TOKEN_ID);
            list.push_direct(temp_nodes);
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

class DNABPETokenizer {
private:
    // Базовый словарь как статические константы класса
    static constexpr size_t BASE_VOCAB_SIZE = 5;
    static constexpr std::array<const char*, BASE_VOCAB_SIZE> BASE_VOCAB = {
        "A", "C", "G", "T", "<SEP>"
    };
    static constexpr std::array<char, 4> NUCLEOTIDES = {'A', 'C', 'G', 'T'};
    static constexpr size_t SEP_TOKEN_ID = 4;

    // Статические методы для работы с базовым словарем
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
    std::vector<size_t> token_frequencies;  // частоты по id токена
    VectorLinkedList current_sequence;  // Заменяем vector<VectorLinkedList> на один список
    const int max_vocab_size;

    void count_pairs(std::unordered_map<IntPair, size_t, IntPairHash>& frequencies) {
        for (auto it = current_sequence.begin(); it != current_sequence.end(); ++it) {
            const VectorNode& current = *it;
            if (current.next_idx != VectorLinkedList::END_MARKER) {
                const VectorNode& next = current_sequence.get_node(current.next_idx);
                if (current.token_id != 4 && next.token_id != 4) {
                    frequencies[IntPair{current.token_id, next.token_id}]++;
                }
            }
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

public:
    DNABPETokenizer(int max_size = 1000) : max_vocab_size(max_size) {
        // Инициализация базового словаря
        vocab_strings.reserve(max_size);
        token_frequencies.reserve(max_size);
        
        for (size_t i = 0; i < BASE_VOCAB_SIZE; ++i) {
            vocab_strings.push_back(BASE_VOCAB[i]);
            vocab[BASE_VOCAB[i]] = i;
            token_frequencies.push_back(0);
        }
    }

    // Публичные методы для внешнего доступа к базовому словарю
    static bool is_valid_token(char c) { return is_nucleotide(c); }
    static uint32_t get_base_token_id(char c) { return char_to_token_id(c); }
    static constexpr size_t get_sep_token_id() { return SEP_TOKEN_ID; }

    void train(SequenceReader& reader, int num_merges) {
        current_sequence = reader.read_all_sequences();
        std::cerr << "Starting vocabulary training..." << std::endl;
        update_train_progress(vocab_strings.size());
        
        for (int i = 0; i < num_merges && vocab_strings.size() < max_vocab_size; ++i) {
            std::unordered_map<IntPair, size_t, IntPairHash> frequencies;
            count_pairs(frequencies);
            
            auto most_frequent = std::max_element(
                frequencies.begin(), frequencies.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; }
            );
            
            if (most_frequent == frequencies.end() || most_frequent->second < 2) 
                break;

            std::string new_token = vocab_strings[most_frequent->first.first] + 
                                  vocab_strings[most_frequent->first.second];
            uint32_t new_id = vocab_strings.size();
            vocab_strings.push_back(new_token);
            vocab[new_token] = new_id;
            token_frequencies.push_back(most_frequent->second);
            merges.push_back({most_frequent->first.first, most_frequent->first.second});

            apply_merge_to_list(current_sequence, most_frequent->first, new_id);
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
        return token_frequencies[token_id];
    }
    int get_vocab_size() const { return vocab_strings.size(); }
    
    // Add getter for token by id
    const std::string& get_token_by_id(int id) const { 
        return vocab_strings[id]; 
    }
};

void print_usage() {
    std::cerr << "Usage: ./bpe.exe <input_file> <output_file_prefix> <max_tokens> <threads>\n";
    std::cerr << "Example: ./bpe.exe input.txt output 1000 4\n";
    std::cerr << "Input format: one DNA sequence per line\n";
}

// Update main function to use new SequenceReader implementation
int main(int argc, char* argv[]) {
    if (argc != 5) {
        print_usage();
        return 1;
    }

    try {
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
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
