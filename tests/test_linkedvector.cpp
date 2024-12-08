#include <gtest/gtest.h>
#include "../src/linkedvector.hpp"

class VectorLinkedListTest : public ::testing::Test {
protected:
    VectorLinkedList list;

    void SetUp() override {
        list = VectorLinkedList();
        list.init(100);  // Reserve some space for testing
    }
};

TEST_F(VectorLinkedListTest, InitializationTest) {
    EXPECT_EQ(list.size(), 0);
    EXPECT_TRUE(list.validate_links());
}

TEST_F(VectorLinkedListTest, PushFrontTest) {
    uint32_t pos1 = list.push_front(1);
    EXPECT_EQ(list.size(), 1);
    EXPECT_EQ(list.get_node(pos1).token_id, 1);
    EXPECT_TRUE(list.validate_links());

    uint32_t pos2 = list.push_front(2);
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list.get_node(pos2).token_id, 2);
    EXPECT_EQ(list.get_node(pos2).next_offset, pos1 - pos2);
    EXPECT_TRUE(list.validate_links());
}

TEST_F(VectorLinkedListTest, AppendTest) {
    list.append(1);
    list.append(2);
    list.append(3);
    
    EXPECT_EQ(list.size(), 3);
    EXPECT_TRUE(list.validate_links());
    
    auto it = list.begin();
    EXPECT_EQ((*it).token_id, 1);
    ++it;
    EXPECT_EQ((*it).token_id, 2);
    ++it;
    EXPECT_EQ((*it).token_id, 3);
}

TEST_F(VectorLinkedListTest, MergeNodesTest) {
    // First, add 10 tokens (1-10)
    std::vector<uint32_t> initial_tokens = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    for (uint32_t token : initial_tokens) {
        list.append(token);
    }
    
    EXPECT_EQ(list.size(), 10);
    EXPECT_TRUE(list.validate_links());
    
    // Store positions for merging
    std::vector<uint32_t> positions;
    for (auto it = list.begin(); it != list.end(); ++it) {
        positions.push_back(it.position());
    }
    
    // Perform merges at positions 2,4,6,8
    // Merge pos[2]: tokens 3,4 -> 11
    EXPECT_TRUE(list.merge_nodes(positions[2], 11));
    EXPECT_EQ(list.size(), 9);
    EXPECT_TRUE(list.validate_links());
    
    // Merge pos[4]: tokens 5,6 -> 12
    EXPECT_TRUE(list.merge_nodes(positions[4], 12));
    EXPECT_EQ(list.size(), 8);
    EXPECT_TRUE(list.validate_links());
    
    // Merge pos[6]: tokens 7,8 -> 13
    EXPECT_TRUE(list.merge_nodes(positions[6], 13));
    EXPECT_EQ(list.size(), 7);
    EXPECT_TRUE(list.validate_links());
    
    // Merge pos[8]: tokens 9,10 -> 14
    EXPECT_TRUE(list.merge_nodes(positions[8], 14));
    EXPECT_EQ(list.size(), 6);
    EXPECT_TRUE(list.validate_links());
    
    // Verify final token sequence
    std::vector<uint32_t> expected = {1, 2, 11, 12, 13, 14};
    std::vector<uint32_t> actual;
    for (const auto& node : list) {
        actual.push_back(node.token_id);
    }
    EXPECT_EQ(actual, expected);
    
    // Verify bidirectional traversal
    std::vector<uint32_t> reverse_expected = {14, 13, 12, 11, 2, 1};
    std::vector<uint32_t> reverse_actual;
    for (auto it = list.rbegin(); it != list.rend(); ++it) {
        reverse_actual.push_back((*it).token_id);
    }
    EXPECT_EQ(reverse_actual, reverse_expected);
    
    // Check neighbor relationships
    for (auto it = list.begin(); it != list.end(); ++it) {
        uint64_t pos = it.position();
        uint32_t token = (*it).token_id;
        
        // Check next token except for last node
        if (token != 14) {  // not last
            EXPECT_NE(list.get_next_token(pos), TokenizerConstants::SEP_TOKEN_ID);
        } else {  // last node
            EXPECT_EQ(list.get_next_token(pos), TokenizerConstants::SEP_TOKEN_ID);
        }
        
        // Check previous token except for first node
        if (token != 1) {  // not first
            EXPECT_NE(list.get_prev_token(pos), TokenizerConstants::SEP_TOKEN_ID);
        } else {  // first node
            EXPECT_EQ(list.get_prev_token(pos), TokenizerConstants::SEP_TOKEN_ID);
        }
    }
}

TEST_F(VectorLinkedListTest, IteratorTest) {
    std::vector<uint32_t> expected = {1, 2, 3, 4};
    for (uint32_t token : expected) {
        list.append(token);
    }

    std::vector<uint32_t> actual;
    for (const auto& node : list) {
        actual.push_back(node.token_id);
    }

    EXPECT_EQ(actual, expected);
}

TEST_F(VectorLinkedListTest, ReverseIteratorTest) {
    std::vector<uint32_t> input = {1, 2, 3, 4};
    for (uint32_t token : input) {
        list.append(token);
    }

    std::vector<uint32_t> actual;
    for (auto it = list.rbegin(); it != list.rend(); ++it) {
        actual.push_back((*it).token_id);
    }

    std::vector<uint32_t> expected = {4, 3, 2, 1};
    EXPECT_EQ(actual, expected);
}

TEST_F(VectorLinkedListTest, GetNeighborTokensTest) {
    list.append(1);
    uint32_t pos = list.get_head();
    list.append(2);
    
    EXPECT_EQ(list.get_prev_token(pos + 1), 1);
    EXPECT_EQ(list.get_next_token(pos), 2);
    EXPECT_EQ(list.get_prev_token(pos), TokenizerConstants::SEP_TOKEN_ID);
    EXPECT_EQ(list.get_next_token(pos + 1), TokenizerConstants::SEP_TOKEN_ID);
}

TEST_F(VectorLinkedListTest, PushDirectTest) {
    std::vector<VectorNode> nodes = {
        VectorNode(1),
        VectorNode(2),
        VectorNode(3)
    };
    
    list.push_direct(nodes);
    EXPECT_EQ(list.size(), 3);
    EXPECT_TRUE(list.validate_links());

    std::vector<uint32_t> expected = {1, 2, 3};
    std::vector<uint32_t> actual;
    for (const auto& node : list) {
        actual.push_back(node.token_id);
    }
    
    EXPECT_EQ(actual, expected);
}
