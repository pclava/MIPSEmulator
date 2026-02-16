//
// Created by Paul Clavaud on 18/12/25.
//

#include "gtest/gtest.h"
#include "Register.h"
#include "utils.h"

using namespace MIPS;

class RegisterTests : public testing::Test {
protected:
    RegisterFile rf;

    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RegisterTests, CanReadAndWriteRegister) {
    Register r(23, 0x42);
    EXPECT_EQ(r.read(), 0x42);
    r.set(0x999);
    EXPECT_EQ(r.read(), 0x999);
}

TEST_F(RegisterTests, RegisterCanReset) {
    Register r(23, 0x42);
    r.set(0x235);
    r.reset();
    EXPECT_EQ(r.read(), 0x42);
}

TEST_F(RegisterTests, RegisterFileCanReset) {
    rf.write(23, 0x24);
    rf.write(29, 0x234);
    rf.reset();
    EXPECT_EQ(rf.read(23), 0);
    EXPECT_EQ(rf.read(29), STACK_LIMIT);
}