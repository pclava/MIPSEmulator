#include "gtest/gtest.h"
#include "Processor.h"
#include "utils.h"

using namespace MIPS;

class SyscallTest : public testing::Test {
protected:
    std::stringstream testIn; // input and output streams for testing
    std::stringstream testOut;

    Memory mem;
    Coprocessor0 c0;
    CPU cpu;

    SyscallTest() : cpu(&c0, testIn, testOut) {
    }

    void SetUp() override {
        cpu.set_pc_entry(0x00400000);
        cpu.PC.reset();
    }

    void TearDown() override {}
};

#define R(x) cpu.RF[x]

#define EXPECT_FAIL(machine_code) \
    EXPECT_THROW(cpu.Execute(mem, CPU::Decode(machine_code)), std::runtime_error); \
    EXPECT_EQ(cpu.exit, 255);

TEST_F(SyscallTest, CanPrintInt) {
    s32 num = -1412567278;
    // Write integer to a register
    R(8) = num; // 0xabcdef12

    // Prepare SYSCALL_PRINT_INT
    R(2) = 1; // v0 = 1
    cpu.Execute(mem, CPU::Decode(0x00082021)); // addu a0 r0 r8

    // Execute and test
    cpu.Execute(mem, CPU::Decode(0xc)); // syscall
    EXPECT_EQ(testOut.str(), std::to_string(num));
}

TEST_F(SyscallTest, CanPrintString) {
    // Write a string to memory
    const Byte buf[15] = "Hello, World!\n";
    mem.writeN(0x10010004, 15, buf);

    // Prepare SYSCALL_PRINT_STR
    R(2) = 4; // v0 = 4 (SYSCALL_PRINT_STR)
    R(4) = 0x10010004; // a0 = buf

    // Execute and test
    cpu.Execute(mem, CPU::Decode(0xc)); // syscall
    EXPECT_EQ(testOut.str(), "Hello, World!\n");
}

TEST_F(SyscallTest, CanReadInt) {
    testIn << 5757;

    // Prepare SYSCALL_READ_INT
    R(2) = 5;

    cpu.Execute(mem, CPU::Decode(0xc));
    EXPECT_EQ(cpu.RF[2], 5757);
}

TEST_F(SyscallTest, CanReadStr) {
    testIn << "Hello, World!\n";
    const int size = 15;

    // Prepare SYSCALL_READ_STR
    R(2) = 8;
    R(4) = 0x10010000; // buffer addr
    R(5) = size; // buffer size

    cpu.Execute(mem, CPU::Decode(0xc));

    Byte buf[size];
    Byte expected[] = "Hello, World!";
    mem.readN(0x10010000, size, buf);
    for (int i = 0; i < size-1; i++) {
        EXPECT_EQ(expected[i], buf[i]);
    }

    // Try reading less
    testIn << "Hello, World!\n";
    const int size2 = 5;

    R(4) = 0x10010100;
    R(5) = size2;

    cpu.Execute(mem, CPU::Decode(0xc));
    Byte buf2[size2];
    Byte expected2[size2] = {'H', 'e', 'l', 'l', 'o'};
    mem.readN(0x10010100, size2, buf2);
    for (int i = 0; i < size2; i++) {
        EXPECT_EQ(expected2[i], buf2[i]);
    }
}

TEST_F(SyscallTest, CanPrintChar) {
    unsigned char c = 'H';
    R(4) = c; // a0 = c
    R(2) = 11; // SYSCALL_PRINT_CHAR
    cpu.Execute(mem, CPU::Decode(0xc));
    EXPECT_EQ(testOut.str()[0], c);
}

TEST_F(SyscallTest, CanReadChar) {
    testIn << 72;
    R(2) = 12;
    cpu.Execute(mem, CPU::Decode(0xc));
    EXPECT_EQ(R(2), '7');
    R(2) = 12;
    cpu.Execute(mem, CPU::Decode(0xc));
    EXPECT_EQ(R(2), '2');
}

TEST_F(SyscallTest, CanExitAndExit2) {
    // Try exiting with syscall 10 (SYSCALL_EXIT)
    R(2) = 10;

    // Tests that terminate() is called
    EXPECT_THROW(
        {
            try {
                cpu.Execute(mem, CPU::Decode(0xc));
            } catch (std::runtime_error& e) {
                EXPECT_STREQ("CPU terminated", e.what());
                throw;
            }
        }, std::runtime_error);

    // Try exiting with syscall 17 (SYSCALL_EXIT2)
    R(2) = 17;
    R(4) = 24; // Some error code

    // Tests that terminate() is called with the correct error code
    EXPECT_THROW(
        {
            try {
                cpu.Execute(mem, CPU::Decode(0xc));
            } catch (std::runtime_error& e) {
                EXPECT_STREQ("CPU terminated", e.what());
                throw;
            }
        }, std::runtime_error);
    EXPECT_EQ(cpu.exit, 24);
}

TEST_F(SyscallTest, BadSyscallCausesFatalException) {
    // Try a fake syscall
    R(2) = 100;
    EXPECT_THROW(cpu.Execute(mem, CPU::Decode(0xc)), std::runtime_error); \
    EXPECT_EQ(cpu.exit, 255);
}

TEST_F(SyscallTest, CanOpenAndCloseFile) {
    Byte path[] = "MIPSTest/src/test.txt";
    mem.writeN(0x10000000, sizeof(path), path);

    // Open file
    R(2) = 13;
    R(4) = 0x10000000;
    R(5) = 0;
    cpu.Execute(mem, CPU::Decode(0xc));
    const s32 fd = R(2);
    EXPECT_EQ(fd, 3);

    // Close file
    R(2) = 16;
    R(4) = fd;
    cpu.Execute(mem, CPU::Decode(0xc));
}

TEST_F(SyscallTest, CanReadFile) {
    Byte path[] = "MIPSTest/src/test.txt";
    mem.writeN(0x10000000, sizeof(path), path);

    // Open file
    R(2) = 13;
    R(4) = 0x10000000;
    R(5) = 0;
    cpu.Execute(mem, CPU::Decode(0xc));
    const s32 fd = R(2);

    // Read 1 byte from file
    R(4) = fd;          // file descriptor
    R(5) = 0x10010000;  // buffer address
    R(6) = 1;           // bytes to read
    R(2) = 14;
    cpu.Execute(mem, CPU::Decode(0xc));
    R(8) = mem.readByte(0x10010000);
    EXPECT_EQ(R(8), 'H');
    EXPECT_EQ(R(2), 1);

    // Read past EOF
    R(4) = fd;
    R(5) = 0x10010000;
    R(6) = 32;
    R(2) = 14;
    cpu.Execute(mem, CPU::Decode(0xc));
    EXPECT_EQ(R(2), 12);

    // Try reading after EOF
    R(4) = fd;
    R(5) = 0x10010000;
    R(6) = 32;
    R(2) = 14;
    cpu.Execute(mem, CPU::Decode(0xc));
    EXPECT_EQ(R(2), 0);

    Byte expected[] = "ello, World!";
    Byte real[sizeof(expected)];
    mem.readN(0x10010000, sizeof(real), real);
    for (unsigned int i = 0; i < sizeof(expected)-1; i++) {
        EXPECT_EQ(expected[i], real[i]);
    }

    // Close file
    R(2) = 16;
    R(4) = fd;
    cpu.Execute(mem, CPU::Decode(0xc));
}

TEST_F(SyscallTest, CanWriteFile) {
    Byte path[] = "MIPSTest/src/writetest.txt";
    mem.writeN(0x10000000, sizeof(path), path);

    // Open file
    R(2) = 13;
    R(4) = 0x10000000;
    R(5) = 1; // write mode
    cpu.Execute(mem, CPU::Decode(0xc));
    const s32 fd = R(2);
    EXPECT_EQ(fd, 3);

    // Write a string to file
    Byte expected[] = "This was written with a syscall!";
    mem.writeN(0x10010000, sizeof(expected), expected);
    R(2) = 15;
    R(4) = fd;
    R(5) = 0x10010000;
    R(6) = sizeof(expected)-1; // don't write null terminator
    cpu.Execute(mem, CPU::Decode(0xc));

    // Close file
    R(2) = 16;
    R(4) = fd;
    cpu.Execute(mem, CPU::Decode(0xc));

    // Open file to test
    char path_str[sizeof(path)];
    memcpy(path_str, path, sizeof(path));
    FILE *f = fopen(path_str, "r");
    char real[sizeof(expected)];
    fread(real, sizeof(real), 1, f);
    for (unsigned int i = 0; i < sizeof(expected)-1; i++) {
        EXPECT_EQ(expected[i], real[i]);
    }
    fclose(f);
}

TEST_F(SyscallTest, CanUseHeap) {
    EXPECT_EQ(mem.heapAddress, HEAP_START);
    // Allocate small amount of memory
    R(2) = 9;   // sys sbrk
    R(4) = 16;  // 16 bytes
    cpu.Execute(mem, CPU::Decode(0xc));
    EXPECT_EQ(mem.heapAddress, HEAP_START+16);
    EXPECT_EQ(R(2), HEAP_START);

    R(8) = R(2);    // save pointer

    // Free that memory
    R(2) = 23;      // sys brk
    R(4) = R(8);
    cpu.Execute(mem, CPU::Decode(0xc));
    EXPECT_EQ(mem.heapAddress, HEAP_START);

    // Allocate large block
    R(2) = 9;
    R(4) = 0xfffff; // one word less than the max
    cpu.Execute(mem, CPU::Decode(0xc));
    EXPECT_EQ(mem.heapAddress, HEAP_LIMIT+1);
    EXPECT_EQ(R(2), HEAP_START);

    // Allocate one more, expect failure
    R(2) = 9;
    R(4) = 1;
    EXPECT_FAIL(cpu.Execute(mem, CPU::Decode(0xc)));

    // Try setting program break outside of heap
    R(2) = 23;
    R(4) = 0x00400000;
    EXPECT_FAIL(cpu.Execute(mem, CPU::Decode(0xc)));
}