#include "gtest/gtest.h"
#include "Processor.h"
#include "InstructionSet.h"
#include "utils.h"

using namespace MIPS;

class InstructionTest : public testing::Test {
protected:
    Memory mem;
    Coprocessor0 c0;
    CPU cpu{&c0};

    void SetUp() override {
        cpu.set_pc_entry(0x00400000);
        cpu.PC.reset();
    }

    void TearDown() override {}
};

/* R-TYPE TESTS */

#define R(x) cpu.RF[x]

// Tests if the instruction causes a fatal exception
// Checks that terminate() was called with error code 255 (fatal exception)
#define EXPECT_FAIL(machine_code) \
    EXPECT_THROW(cpu.Execute(mem, CPU::Decode(machine_code)), std::runtime_error); \
    EXPECT_EQ(cpu.exit, 255);

TEST_F(InstructionTest, TestAdd) {
    R(8) = 99;
    R(9) = 2335;
    R(10) = 1242;
    Word code = 0x012a4020; // add $t0 $t1 $t2 (r8, r9, r10)
    Instruction instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), 3577);

    R(8) = 0;

    code = 0x012a4021; // addu $t0 $t1 $t2 (r8, r9, r10)
    instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), 3577);

    // Addu shouldn't cause arithmetic overflow
    R(9) = 0x7fffffff;
    R(10) = 0x7fffffff;
    code = 0x012a4021; // addu $t0 $t1 $t2 (r8, r9, r10)
    instr = CPU::Decode(code);
    bool success = cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), 0xfffffffe);
    EXPECT_EQ(success, true);
    EXPECT_EQ(cpu.exit, false);
}

TEST_F(InstructionTest, AddAndSubCauseArithmeticExceptionOverflow) {
    R(8) = 0;
    R(9) = 0x7fffffff;
    R(10) = 0x7fffffff;
    Word code = 0x012a4020; // add r8 r9 r10
    EXPECT_FAIL(code);

    cpu.exit = false;

    R(9) = 2147483647;
    R(10) = -1;
    code = 0x012a4022; // sub $t0 $t1 $t2
    EXPECT_FAIL(code);
}

TEST_F(InstructionTest, TestBitwiseRTypeOperations) {
    R(8) = 45;
    R(9) = 42313;
    R(10) = 9872;

    Word code = 0x012a4024; // and $t0 $t1 $t2
    Instruction instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), 9216);

    code = 0x012a4027; // nor $t0 $t1 $t2
    instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), -42970);

    code = 0x012a4025; // or $t0 $t1 $t2
    instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), 42969);
}

TEST_F(InstructionTest, TestJr) {
    R(31) = 12882;
    Word code = 0x03e00008; // jr $ra
    Instruction instr = CPU::Decode(code);
    cpu.Execute(mem, instr);

    EXPECT_EQ(cpu.PC.read(), 12882);
}

TEST_F(InstructionTest, TestSlt) {
    R(8) = 12882;
    R(9) = 56436;
    R(10) = 56437;
    R(11) = 56436;

    Word code = 0x012a402a; // slt $t0 $t1 $t2
    Instruction instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), 1);

    code = 0x012b402a; // slt $t0 $t1 $t3
    instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), 0);

    R(16) = -232;
    R(17) = 244;

    code = 0x0211902a; // slt $s2 $s0 $s1
    instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(18), 1);
}

TEST_F(InstructionTest, TestSltu) {
    R(8) = 12882;
    R(9) = 56436;
    R(10) = 56437;
    R(11) = 56436;

    Word code = 0x012a402b; // sltu $t0 $t1 $t2
    Instruction instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), 1);

    code = 0x012b402b; // sltu $t0 $t1 $t3
    instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), 0);

    R(16) = -232;
    R(17) = 244;

    code = 0x0211902b; // sltu $s2 $s0 $s1
    instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(18), 0);
}

TEST_F(InstructionTest, TestBitShifts) {
    R(8) = 432;
    R(9) = 213;

    cpu.newPC = cpu.PC.reset_value;

    Word code = 0x00094140; // sll $t0 $t1 5
    Instruction instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), 6816);

    R(9) = 0x0FFFFFFF;
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), -32);

    R(9) = 213;
    code = 0x00094142; // srl $t0 $t1 5
    instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), 6);

    R(9) = static_cast<s32>(0xFFFF0000);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), 134215680);

    code = 0x00094143; // sra $t0 $t1 5
    instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), -2048);

    R(9) = 213;
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), 6);
}

TEST_F(InstructionTest, TestSub) {
    R(8) = 23235;
    R(9) = 8643;
    R(10) = 92984;

    Word code = 0x012a4022; // sub $t0 $t1 $t2
    Instruction instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), -84341);

    R(8) = 0;

    code = 0x012a4023; // sub $t0 $t1 $t2
    instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.RF.read(8), -84341);
}

TEST_F(InstructionTest, TestCompleteDivisionOperation) {
    R(8) = static_cast<s32>(0xfffff241); // -3519 or 4,294,963,777
    R(9) = 0x838; // 2104

    Instruction instr = CPU::Decode(0x0109001a); // div r8 r9
    cpu.Execute(mem, instr);
    instr = CPU::Decode(0x00008010); // mfhi r16
    cpu.Execute(mem, instr);
    instr = CPU::Decode(0x00008812); // mflo r17
    cpu.Execute(mem, instr);
    instr = CPU::Decode(0x0109001b); // divu r8 r9
    cpu.Execute(mem, instr);
    instr = CPU::Decode(0x00009010); // mfhi r18
    cpu.Execute(mem, instr);
    instr = CPU::Decode(0x00009812); // mflo r19
    cpu.Execute(mem, instr);

    EXPECT_EQ(cpu.RF.read(16), -1415);
    EXPECT_EQ(cpu.RF.read(17), -1);
    EXPECT_EQ(cpu.RF.read(18), 1249);
    EXPECT_EQ(cpu.RF.read(19), 2041332);
}

TEST_F(InstructionTest, TestCompleteMultiplicationOperation) {
    R(8) = static_cast<s32>(0xf0008a45); // -268,400,059 or 4,026,567,237
    R(9) = 0x838; // 2104

    Instruction instr = CPU::Decode(0x01090018); // mult r8 r9
    cpu.Execute(mem, instr);
    instr = CPU::Decode(0x00008010); // mfhi r16
    cpu.Execute(mem, instr);
    instr = CPU::Decode(0x00008812); // mflo r17
    cpu.Execute(mem, instr);
    instr = CPU::Decode(0x01090019); // multu r8 r9
    cpu.Execute(mem, instr);
    instr = CPU::Decode(0x00009010); // mfhi r18
    cpu.Execute(mem, instr);
    instr = CPU::Decode(0x00009812); // mflo r19
    cpu.Execute(mem, instr);

    EXPECT_EQ(cpu.RF.read(16), -132);
    EXPECT_EQ(cpu.RF.read(17), -2073008360);
    EXPECT_EQ(cpu.RF.read(18), 1972);
    EXPECT_EQ(cpu.RF.read(19), -2073008360);
}

TEST_F(InstructionTest, TestMul) {
    R(8) = 59301;
    R(9) = 9876;
    R(10) = 873431;
    Word code = 0x712a4002; // mul r8 r9 r10
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(8), 0x0226624c); // 36069964
}

/* I-TYPE TESTS */

TEST_F(InstructionTest, TestAddi) {
    R(8) = 2344;
    R(9) = 5634;
    Word code = 0x2128259f; // addi $t0 $t1 9631 (r8, r9, 0x0000259f)
    Instruction instr = CPU::Decode(code);
    cpu.Execute(mem, instr);

    EXPECT_EQ(cpu.RF.read(8), 15265);

    R(16) = 236;
    R(17) = 82;
    code = 0x2230fffc; // addi $s0 $s1 -4 (r16, r17, 0xfffffffc)
    instr = CPU::Decode(code);
    cpu.Execute(mem, instr);

    EXPECT_EQ(cpu.RF.read(16), 78);

    R(8) = 2334;
    R(9) = 5634;
    cpu.Execute(mem, CPU::Decode(0x2528259f)); // addiu $t0 $t1 9631
    EXPECT_EQ(cpu.RF.read(8), 15265);
}

TEST_F(InstructionTest, TestAndi) {
    R(8) = 2334;
    R(9) = 5634;
    Word code = 0x3128259f; // andi $t0 $t1 9631
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(cpu.RF.read(8), 1026);
}

TEST_F(InstructionTest, TestOri) {
    R(8) = 2334;
    R(9) = 5634;
    Word code = 0x352886e4;
    cpu.Execute(mem, CPU::Decode(code)); // ori r8 r9 0x86e4
    EXPECT_EQ(cpu.RF.read(8), 0x96e6);
}

TEST_F(InstructionTest, TestBeq) {
    cpu.PC.set(0x00400008);
    cpu.newPC = 0x0040000c;
    R(8) = 2443;
    R(9) = 2443;
    Word code = 0x11090009; // beq $t0 $t1 0x0009
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(cpu.PC.read(), 0x00400030); // 48 = 8 + 4 + 4*9

    cpu.PC.set(0x00400008);
    cpu.newPC = 0x0040000c;
    R(8) = 2443;
    R(9) = 2444;
    cpu.Execute(mem, CPU::Decode(code)); // shouldn't branch
    EXPECT_EQ(cpu.PC.read(), 0x0040000c); // 12 = 8 + 4
}

TEST_F(InstructionTest, TestBne) {
    cpu.PC.set(0x00400008);
    cpu.newPC = 0x0040000c; // pc + 4
    R(8) = 2443;
    R(9) = 2443;
    Word code = 0x15090009; // bne $t0 $t1 0x0009
    cpu.Execute(mem, CPU::Decode(code)); // shouldn't branch
    EXPECT_EQ(cpu.PC.read(), 0x0040000c); // 12 = 8 + 4

    cpu.PC.set(0x00400008);
    cpu.newPC = 0x0040000c;
    R(8) = 2443;
    R(9) = 2444;
    cpu.Execute(mem, CPU::Decode(code)); // should branch
    EXPECT_EQ(cpu.PC.read(), 0x00400030); // 48 = 8 + 4 + 4*9
}

TEST_F(InstructionTest, TestLui) {
    R(8) = 0x4231;
    Word code = 0x3c08ff12;
    cpu.Execute(mem, CPU::Decode(code)); // ori r8 0xff12
    EXPECT_EQ(cpu.RF.read(8), 0xff120000);
}

TEST_F(InstructionTest, TestLw) {
    // Write some values to data memory
    mem.writeWord(0x1001244c, 0xffff7fff);
    mem.writeWord(0x10011114, 0x10101010);

    // Write base address to register
    R(8) = 0x10010000;

    // Load values to r16 and r17
    Word code1 = 0x8d10244c; // lw r16 0x244c(r8)
    Word code2 = 0x8d111114; // lw r17 0x1114(r8)
    cpu.Execute(mem, CPU::Decode(code1));
    cpu.Execute(mem, CPU::Decode(code2));

    // Check registers
    EXPECT_EQ(R(16), 0xffff7fff);
    EXPECT_EQ(R(17), 0x10101010);

}

TEST_F(InstructionTest, TestSlti) {
    R(8) = static_cast<s32>(0xffffff24);
    Word code = 0x2910075b; // slti r16 r8 0x075b
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(16), 1);
}

TEST_F(InstructionTest, TestSltiu) {
    R(8) = static_cast<s32>(0xffffff24);
    Word code = 0x2d10075b; // slti r16 r8 0x075b
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(16), 0);
}

TEST_F(InstructionTest, TestSb) {
    // Store some bytes, read as one word

    R(8) = 0x13521455;
    R(9) = static_cast<s32>(0xfff452c9);
    R(10) = 0x000000ba;
    R(11) = 0x55555592;

    R(16) = 0x10010000; // Base address

    Word code1 = 0xa2080000; // sb r8 0(r16)
    Word code2 = 0xa2090001; // sb r9 1(r16)
    Word code3 = 0xa20a0002; // sb r10 2(r16)
    Word code4 = 0xa20b0003; // sb r11 3(r16)
    cpu.Execute(mem, CPU::Decode(code1));
    cpu.Execute(mem, CPU::Decode(code2));
    cpu.Execute(mem, CPU::Decode(code3));
    cpu.Execute(mem, CPU::Decode(code4));

    EXPECT_EQ(mem.readWord(0x10010000), 0x92bac955);
}

TEST_F(InstructionTest, TestSh) {
    R(8) = 0x1252c955;
    R(9) = 0x424f92ba;

    R(16) = 0x10010000;

    Word code1 = 0xa6080000; // sh r8 0(r16)
    Word code2 = 0xa6090002; // sh r9 2(r16)
    cpu.Execute(mem, CPU::Decode(code1));
    cpu.Execute(mem, CPU::Decode(code2));

    EXPECT_EQ(mem.readWord(0x10010000), 0x92bac955);
}

TEST_F(InstructionTest, TestSw) {
    R(8) = static_cast<s32>(0x92bac955);
    R(16) = 0x10010000;
    Word code = 0xae080000;
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(mem.readWord(0x10010000), 0x92bac955);
}

TEST_F(InstructionTest, AddressLoadErrorOccurs) {
    // Address out of range
    Word code = 0x8c080000; // lw r8 0(r0)
    EXPECT_FAIL(code);

    // Address not on word boundary
    R(16) = 0x10010000;
    code = 0x8e080001; // lw r8 1(r16)
    EXPECT_FAIL(code);
}

TEST_F(InstructionTest, AddressStoreErrorOccurs) {
    // Address out of range
    R(16) = 0;
    EXPECT_FAIL(0xae080000); // sw r8 0(r16)
    EXPECT_FAIL(0xa6080000); // sh r8 0(r16)
    EXPECT_FAIL(0xa2080000); // sb r8 0(r16)

    // Address not on boundary
    R(16) = 0x10010000;
    EXPECT_FAIL(0xae080003); // sw r8 3(r16)
    EXPECT_FAIL(0xa6080003); // sh r8 3(r16)
}

TEST_F(InstructionTest, TestNop) {
    // Set some registers to make sure nop doesn't do anything
    R(2) = 35;
    R(4) = 1;
    R(5) = 982425;
    R(9) = 9911;
    R(13) = 2325;
    R(16) = 3782;
    R(20) = 88888;
    R(31) = 9876;
    cpu.Execute(mem, CPU::Decode(0));
    EXPECT_EQ(R(2), 35);
    EXPECT_EQ(R(4), 1);
    EXPECT_EQ(R(5), 982425);
    EXPECT_EQ(R(9), 9911);
    EXPECT_EQ(R(13), 2325);
    EXPECT_EQ(R(16), 3782);
    EXPECT_EQ(R(20), 88888);
    EXPECT_EQ(R(31), 9876);
}

TEST_F(InstructionTest, TestLb) {
    R(8) = 0x10010000;
    R(9) = 0x12345678;
    mem.writeWord(R(8), R(9)); // sw r9 0(r8)
    Word code = 0x810a0000; // lb r10 0(r8)
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(10), 0x78);

    // Test sign extension
    R(9) = 0x12345688;
    mem.writeWord(R(8), R(9)); // sw r9 0(r8)
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(10), 0xffffff88);
}

TEST_F(InstructionTest, TestLbu) {
    R(8) = 0x10010000;
    R(9) = 0x12345678;
    mem.writeWord(R(8), R(9));
    Word code = 0x910a0000;
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(10), 0x78);

    // Test zero extension
    R(9) = 0x12345688;
    mem.writeWord(R(8), R(9));
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(10), 0x88);
}

/* J-TYPE TESTS */

TEST_F(InstructionTest, TestJ) {
    Word code = 0x08100008; // j 0x00400020
    Instruction instr = CPU::Decode(code);
    cpu.Execute(mem, instr);
    EXPECT_EQ(cpu.PC.read(), 0x00400020);
}

TEST_F(InstructionTest, TestJal) {
    R(31) = 0x12345678;
    cpu.PC.set(0x00400abc); // start at addr abc

    // jaddr(real) = real[27:0] >> 2
    const Word code = 0x0c10032f; // jal 0x00400cbc
    cpu.Execute(mem, CPU::Decode(code));

    EXPECT_EQ(cpu.PC.read(), 0x00400cbc); // new pc should be at addr cbc
    EXPECT_EQ(R(31), 0x00400ac0); // $ra should be set to original PC+4
}