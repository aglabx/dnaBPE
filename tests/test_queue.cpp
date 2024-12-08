#include <gtest/gtest.h>
#include "../src/queue.hpp"

class PairPriorityQueueTest : public ::testing::Test {
protected:
    PairPriorityQueue queue;

    void SetUp() override {
        queue = PairPriorityQueue();
    }
};

TEST_F(PairPriorityQueueTest, InitializationTest) {
    EXPECT_TRUE(queue.empty());
}

TEST_F(PairPriorityQueueTest, BasicAddAndRetrieveTest) {
    queue.add_pair(1, 2, 3);  // Add pair (1,2) with frequency 3
    queue.add_pair(3, 4, 2);  // Add pair (3,4) with frequency 2
    
    uint32_t left, right;
    size_t freq;
    EXPECT_TRUE(queue.get_most_frequent(left, right, freq));
    EXPECT_EQ(left, 1);
    EXPECT_EQ(right, 2);
    EXPECT_EQ(freq, 3);
}

TEST_F(PairPriorityQueueTest, FrequencyUpdateTest) {
    queue.add_pair(1, 2, 2);
    queue.increase_frequency(1, 2, 3);  // Total should be 5
    queue.decrease_frequency(1, 2, 2);  // Total should be 3
    
    EXPECT_EQ(queue.get_frequency(1, 2), 3);
}

TEST_F(PairPriorityQueueTest, PositionTrackingTest) {
    queue.add_pair_position(1, 2, 10);
    queue.add_pair_position(1, 2, 20);
    
    auto positions = queue.get_pair_positions(1, 2);
    EXPECT_EQ(positions.size(), 2);
    EXPECT_TRUE(positions.find(10) != positions.end());
    EXPECT_TRUE(positions.find(20) != positions.end());
    
    queue.remove_pair_position(1, 2, 10);
    positions = queue.get_pair_positions(1, 2);
    EXPECT_EQ(positions.size(), 1);
    EXPECT_TRUE(positions.find(20) != positions.end());
}

TEST_F(PairPriorityQueueTest, FrequencyOrderingTest) {
    // Add pairs with different frequencies
    queue.add_pair(1, 2, 2);
    queue.add_pair(3, 4, 5);
    queue.add_pair(5, 6, 3);
    
    // Check ordering
    uint32_t left, right;
    size_t freq;
    
    EXPECT_TRUE(queue.get_most_frequent(left, right, freq));
    EXPECT_EQ(left, 3);
    EXPECT_EQ(right, 4);
    EXPECT_EQ(freq, 5);
}

TEST_F(PairPriorityQueueTest, CleanupTest) {
    queue.add_pair(1, 2, 2);
    queue.decrease_frequency(1, 2, 2);  // Frequency becomes 0
    queue.cleanup_zero_pairs();
    
    uint32_t left, right;
    size_t freq;
    EXPECT_FALSE(queue.get_most_frequent(left, right, freq));
}

TEST_F(PairPriorityQueueTest, ComplexUpdateTest) {
    // Add initial pairs
    queue.add_pair(1, 2, 3);
    queue.add_pair(3, 4, 2);
    queue.add_pair(5, 6, 4);
    
    // Update frequencies
    queue.increase_frequency(1, 2, 2);  // Now 5
    queue.decrease_frequency(5, 6, 1);  // Now 3
    
    // Check ordering after updates
    uint32_t left, right;
    size_t freq;
    
    EXPECT_TRUE(queue.get_most_frequent(left, right, freq));
    EXPECT_EQ(left, 1);
    EXPECT_EQ(right, 2);
    EXPECT_EQ(freq, 5);
    
    // Clear and verify
    queue.clear();
    EXPECT_TRUE(queue.empty());
}

TEST_F(PairPriorityQueueTest, FrequencyThresholdTest) {
    // Add pairs with frequency < 2
    queue.add_pair(1, 2, 1);
    
    uint32_t left, right;
    size_t freq;
    EXPECT_FALSE(queue.get_most_frequent(left, right, freq));
    
    // Increase to 2
    queue.increase_frequency(1, 2, 1);
    EXPECT_TRUE(queue.get_most_frequent(left, right, freq));
    EXPECT_EQ(freq, 2);
}

TEST_F(PairPriorityQueueTest, PositionSetOperationsTest) {
    // Test multiple position operations
    queue.add_pair_position(1, 2, 10);
    queue.add_pair_position(1, 2, 20);
    queue.add_pair_position(1, 2, 30);
    
    auto positions = queue.get_pair_positions(1, 2);
    EXPECT_EQ(positions.size(), 3);
    
    // Remove middle position
    queue.remove_pair_position(1, 2, 20);
    positions = queue.get_pair_positions(1, 2);
    EXPECT_EQ(positions.size(), 2);
    EXPECT_TRUE(positions.find(10) != positions.end());
    EXPECT_TRUE(positions.find(30) != positions.end());
    
    // Remove all positions
    queue.remove_pair_position(1, 2, 10);
    queue.remove_pair_position(1, 2, 30);
    positions = queue.get_pair_positions(1, 2);
    EXPECT_TRUE(positions.empty());
}
