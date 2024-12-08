#include <gtest/gtest.h>
#include "../src/bpe.hpp"
#include "../src/reader.hpp"

class DNABPETokenizerTest : public ::testing::Test {
protected:
    std::unique_ptr<DNABPETokenizer> tokenizer;
    std::vector<std::string> test_sequences;

    void SetUp() override {
        tokenizer = std::make_unique<DNABPETokenizer>(100);  // Max vocab size of 100
        test_sequences = {
            "ACGT",
            "ACGT",
            "ACGT",
            "ACTG",
            "ACTG",
            "GTAC"
        };
    }

    // Helper method to create a mock reader
    class MockSequenceReader : public SequenceReader {
    private:
        std::vector<std::string> sequences;
    public:
        // Add copy constructor and assignment operator
        MockSequenceReader(const MockSequenceReader& other) 
            : SequenceReader(""), sequences(other.sequences) {}
            
        MockSequenceReader& operator=(const MockSequenceReader& other) {
            if (this != &other) {
                sequences = other.sequences;
            }
            return *this;
        }

        MockSequenceReader(const std::vector<std::string>& seqs) 
            : SequenceReader(""), sequences(seqs) {}

        VectorLinkedList read_all_sequences() override {
            VectorLinkedList list;
            list.init(1000);
            
            for (const auto& seq : sequences) {
                for (char c : seq) {
                    if (DNABPETokenizer::is_valid_token(c)) {
                        list.append(DNABPETokenizer::get_base_token_id(c));
                    }
                }
                list.append(DNABPETokenizer::get_sep_token_id());
            }
            return list;
        }
    };
};

TEST_F(DNABPETokenizerTest, InitializationTest) {
    EXPECT_EQ(tokenizer->get_vocab_size(), 5);  // A,C,G,T,<SEP>
}

TEST_F(DNABPETokenizerTest, BasicTokenizationTest) {
    std::string sequence = "ACGT";
    auto tokens = tokenizer->tokenize(sequence);
    std::vector<int> expected = {0, 1, 2, 3};  // A=0, C=1, G=2, T=3
    EXPECT_EQ(tokens, expected);
}

TEST_F(DNABPETokenizerTest, BasicTrainingTest) {
    MockSequenceReader reader(test_sequences);
    tokenizer->train(reader, 3);  // Train with 3 merges
    
    EXPECT_GT(tokenizer->get_vocab_size(), 5);  // Should have more tokens than initial vocab
    
    // Test the most frequent pair was merged
    auto tokens = tokenizer->tokenize("ACGT");
    EXPECT_LT(tokens.size(), 4);  // Should have fewer tokens after merging
}

TEST_F(DNABPETokenizerTest, TokenDecodingTest) {
    std::string sequence = "ACGT";
    auto tokens = tokenizer->tokenize(sequence);
    std::string decoded = tokenizer->decode(tokens);
    EXPECT_EQ(decoded, sequence);
}

TEST_F(DNABPETokenizerTest, ComplexTrainingTest) {
    MockSequenceReader reader(test_sequences);
    tokenizer->train(reader, 5);  // More merges
    
    // Test different sequences
    std::vector<std::string> test_cases = {
        "ACGT",
        "ACTG",
        "GTAC"
    };
    
    for (const auto& seq : test_cases) {
        auto tokens = tokenizer->tokenize(seq);
        std::string decoded = tokenizer->decode(tokens);
        EXPECT_EQ(decoded, seq);
    }
}

TEST_F(DNABPETokenizerTest, VocabSizeLimitTest) {
    // Create tokenizer with small vocab limit
    DNABPETokenizer small_tokenizer(7);  // Only 2 merges possible (5 base tokens + 2 new)
    MockSequenceReader reader(test_sequences);
    small_tokenizer.train(reader, 10);  // Try to do 10 merges
    
    EXPECT_EQ(small_tokenizer.get_vocab_size(), 7);  // Should not exceed limit
}

TEST_F(DNABPETokenizerTest, InvalidSequenceTest) {
    std::string invalid_sequence = "ACGTX";  // X is invalid
    auto tokens = tokenizer->tokenize(invalid_sequence);
    
    // Should only tokenize valid nucleotides
    std::vector<int> expected = {0, 1, 2, 3};  // A=0, C=1, G=2, T=3
    EXPECT_EQ(tokens, expected);
}

TEST_F(DNABPETokenizerTest, FrequencyTrackingTest) {
    MockSequenceReader reader({"ACGT", "ACGT", "ACGT"});
    tokenizer->train(reader, 1);
    
    // Check if most frequent pair was merged
    auto tokens = tokenizer->tokenize("ACGT");
    int merged_token = tokens[0];
    EXPECT_GE(merged_token, 5);  // New token ID should be >= 5
    
    // Frequency of merged token should be tracked
    EXPECT_GT(tokenizer->get_token_frequency(merged_token), 0);
}

TEST_F(DNABPETokenizerTest, MergeConsistencyTest) {
    MockSequenceReader reader(test_sequences);
    tokenizer->train(reader, 3);
    
    // Tokenize same sequence multiple times
    auto tokens1 = tokenizer->tokenize("ACGT");
    auto tokens2 = tokenizer->tokenize("ACGT");
    EXPECT_EQ(tokens1, tokens2);
    
    // Different but similar sequences should be tokenized consistently
    auto tokens_acgt = tokenizer->tokenize("ACGT");
    auto tokens_actg = tokenizer->tokenize("ACTG");
    
    // Common substrings should be tokenized the same way
    if (tokens_acgt.size() > 1 && tokens_actg.size() > 1) {
        EXPECT_EQ(tokens_acgt[0], tokens_actg[0]);  // "AC" should be tokenized the same
    }
}
