//
// Created by Paul Clavaud on 18/12/25.
//

#include "gtest/gtest.h"
#include "Memory.h"
#include "utils.h"

using namespace MIPS;

class MemoryTest : public testing::Test {
protected:
    Memory memory;
    MemoryBlock block;
    MemorySegment segment{0, BLOCK_SIZE*5-1};

    void SetUp() override {
        memory.reset();
        block.reset();
        segment.reset();
    }

    void TearDown() override {}
};

TEST_F(MemoryTest, BlockInitAndReadShouldBeZero) {
    ASSERT_EQ(block[32], 0);
}

TEST_F(MemoryTest, BlockWrite) {
    block[0x24] = 4;
    block[BLOCK_SIZE-1] = 54;
    block[0] = 255;
    block[0x322] = 99;
    EXPECT_EQ(block.data[0x24], 4);
    EXPECT_EQ(block.data[BLOCK_SIZE-1], 54);
    EXPECT_EQ(block.data[0], 255);
    EXPECT_EQ(block.data[0x322], 99);
}

TEST_F(MemoryTest, BlockReset) {
    block[0x24] = 4; // given
    block.reset(); // when
    EXPECT_EQ(block.data[0x24], 0); // then
}

TEST_F(MemoryTest, SegmentInitializesCorrectNumberOfBlocks) {
    EXPECT_EQ(segment.table.size(), 5);
    MemorySegment s1{0, BLOCK_SIZE*5};
    EXPECT_EQ(s1.table.size(), 6);
    MemorySegment s2{BLOCK_SIZE, BLOCK_SIZE*5-1};
    EXPECT_EQ(segment.table.size(), 5);
}

TEST_F(MemoryTest, InSegmentCorrectlyDetectsBounds) {
    EXPECT_EQ(segment.inSegment(-1), false);
    EXPECT_EQ(segment.inSegment(BLOCK_SIZE*5-1), true);
    EXPECT_EQ(segment.inSegment(BLOCK_SIZE*5), false);
    EXPECT_EQ(segment.inSegment(BLOCK_SIZE*2), true);
}

TEST_F(MemoryTest, getBlockInitializesBlock) {
    EXPECT_FALSE(segment.table[5]);
    EXPECT_EQ(segment.readByte(2*BLOCK_SIZE+3), 0);
    EXPECT_TRUE(segment.table[2]);
}

TEST_F(MemoryTest, SegmentReadWord) {
    segment[0] = 0x21;
    segment[1] = 0x26;
    segment[2] = 0x99;
    segment[3] = 0x00;
    EXPECT_EQ(segment.readWord(0), 0x00992621);
}

TEST_F(MemoryTest, SegmentWriteWord) {
    segment.writeWord(0, 0x00992621);
    EXPECT_EQ(segment.readWord(0), 0x00992621);
}

TEST_F(MemoryTest, SegmentReadN) {
    segment[3] = 0x21;
    segment[4] = 0x26;
    segment[5] = 0x99;
    segment[6] = 0x00;
    segment[7] = 0x01;
    Byte buf[5];
    const Byte expected[] = {0x21, 0x26, 0x99, 0x00, 0x01};
    segment.readN(3, 5, buf);
    EXPECT_EQ(1,1);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(expected[i], buf[i]);
    }
}

TEST_F(MemoryTest, SegmentWriteN) {
    const Byte buf[5] = {0xaa, 0xab, 0xbc, 0xcd, 0xde};
    segment.writeN(12, 5, buf);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(segment.readByte(12+i), buf[i]);
    }
}

TEST_F(MemoryTest, SegmentSupportsLargeWrites) {
    for (size_t i = 0; i < BLOCK_SIZE*5; i++) { // Write to every address in the segment
        segment.writeByte(i, static_cast<Byte>(i % BLOCK_SIZE % 255));
    }
    EXPECT_EQ(segment[BLOCK_SIZE*3+521], 521 % 255);
    EXPECT_EQ(segment[BLOCK_SIZE*5-1], (BLOCK_SIZE-1) % 255);
}