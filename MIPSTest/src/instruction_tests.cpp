#include "gtest/gtest.h"
#include "Processor.h"
#include "InstructionSet.h"
#include "utils.h"

using namespace MIPS;

class InstructionTest : public testing::Test {
protected:
    Memory mem;
    Coprocessor0 c0{};
    CPU cpu{c0, std::cin, std::cout};

    void SetUp() override {
        cpu.set_mode(USER, mem);
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

#define EXPECT_PASS(machine_code) \
    EXPECT_NO_THROW(cpu.Execute(mem, CPU::Decode(machine_code))); \
    EXPECT_EQ(cpu.exit,0);

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

    cpu.exit = 0;

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

TEST_F(InstructionTest, TestVariableBitShifts) {
    R(8) = 271;
    R(9) = 982;
    R(10) = 9999;
    Word code = 0x01494004; // sllv t0, t1, t2
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(8), 0x01eb0000);

    R(9) = -982;
    R(10) = 9999;
    code = 0x01494006; // srlv t0, t1, t2
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(8), 0x0001ffff);

    R(9) = -982;
    R(10) = 9991;
    code = 0x01494007; // srav t0, t1, t2
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(8), 0xfffffff8);
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

TEST_F(InstructionTest, TestLh) {
    R(8) = 0x10010000; // t0
    R(9) = 641;
    mem.writeByte(R(8), R(9) & 0xFF);
    mem.writeByte(R(8)+1, (R(9) & 0xFF00) >> 8);
    Word code = 0x850a0000; // lh t2, 0(t0)
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(10), 0x281);

    // test sign extension
    R(9) = -454;
    mem.writeByte(R(8), R(9) & 0xFF);
    mem.writeByte(R(8)+1, (R(9) & 0xFF00) >> 8);
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(10), 0xfffffe3a);
}

TEST_F(InstructionTest, TestLhu) {
    R(8) = 0x10010000; // t0
    R(9) = 641;
    mem.writeByte(R(8), R(9) & 0xFF);
    mem.writeByte(R(8)+1, (R(9) & 0xFF00) >> 8);
    Word code = 0x950a0000; // lhu t2, 0(t0)
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(10), 0x281);

    // test zero extension
    R(9) = -454;
    mem.writeByte(R(8), R(9) & 0xFF);
    mem.writeByte(R(8)+1, (R(9) & 0xFF00) >> 8);
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(10), 0xfe3a);
}

TEST_F(InstructionTest, TestUnalignedLoad) {
    // Write some values into memory
    R(9) = 0x10010000;
    mem.writeByte(R(9), 0);
    mem.writeByte(R(9)+1, 1);
    mem.writeByte(R(9)+2, 2);
    mem.writeByte(R(9)+3, 3);
    mem.writeByte(R(9)+4, 4);
    mem.writeByte(R(9)+5, 5);
    mem.writeByte(R(9)+6, 6);
    mem.writeByte(R(9)+7, 7);

    R(8) = 0x72abe9df;
    Word code = 0x89280006; // lwl t0, 6(t1)
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(8), 0x060504df);

    code = 0x99280003; // lwr t0, 3(t1)
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(8), 0x06050403);

    // Test on word boundaries
    R(8) = 0x7edcba98;
    code = 0x89280004; // lwl t0, 4(t1)
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(8), 0x04dcba98);

    R(8) = 0x7edcba98;
    code = 0x99280004; // lwr t0, 4(t1)
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(8), 0x07060504);
}

TEST_F(InstructionTest, TestUnalignedStore) {
    R(9) = 0x10010000;
    R(8) = 0x747ab431;
    Word code = 0xa9280006; // swl t0, 6(t1)
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(mem.readWord(0x10010004), 0x00747ab4);

    code = 0xB9280003; // swr t0, 3(t1)
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(mem.readWord(0x10010003), R(8));

    // Test on word boundaries
    R(8) = 0x1a2b3c4d;
    code = 0xa9280007; // swl t0, 7(t1)
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(mem.readWord(0x10010004), R(8));

    R(8) = 0x01020304;
    code = 0xa9280004; // swl t0, 4(t1)
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(mem.readWord(0x10010004), 0x1a2b3c01);

    R(8) = 0x18423789;
    code = 0xB9280004; // swr t0, 4(t1)
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(mem.readWord(0x10010004), R(8));
}

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

TEST_F(InstructionTest, TestJalr) {
    cpu.PC.set(0x00400abc);
    R(8) = 0x00411111;
    R(9) = 0x2315;
    Word code = 0x01004809; // jalr t1, t0
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(cpu.PC.read(), 0x00411111);
    EXPECT_EQ(R(9), 0x00400ac0);
}

TEST_F(InstructionTest, TestConditionalMoves) {
    R(8) = 87623;
    R(9) = 0;
    R(10) = 43268;
    Word code = 0x0109500A; // movz t2, t0, t1
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(10), 87623);

    R(8) = 9123;
    R(9) = 0xabc;
    R(10) = 145167;
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(10), 145167);

    R(8) = 87623;
    R(9) = 0;
    R(10) = 43268;
    code = 0x109500B;
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(10), 43268);

    R(8) = 9123;
    R(9) = 0xabc;
    R(10) = 145167;
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(10), 9123);
}

TEST_F(InstructionTest, TestBreak) {
    const Word code = 0x0d;
    EXPECT_FAIL(code);
}

TEST_F(InstructionTest, TestTraps) {
    // TEQ
    Word code = 0x01090034; // teq t0, t1
    R(8) = 19;
    R(9) = 19;
    EXPECT_FAIL(code);
    cpu.exit = 0;
    R(8) = 56423;
    R(9) = 716;
    EXPECT_PASS(code);

    // TNE
    code = 0x01090036; // tne t0, t1
    EXPECT_FAIL(code);
    cpu.exit = 0;
    R(8) = 29;
    R(9) = 29;
    EXPECT_PASS(code);

    // TLT
    code = 0x01090032; // tlt t0, t1
    R(8) = -1;
    R(9) = 1;
    EXPECT_FAIL(code);
    cpu.exit = 0;
    R(8) = 2;
    R(9) = 2;
    EXPECT_PASS(code);

    // TLTU
    code = 0x01090033; // tltu t0, t1
    R(8) = 0;
    R(9) = 1;
    EXPECT_FAIL(code);
    cpu.exit = 0;
    R(8) = -1;
    R(9) = 1;
    EXPECT_PASS(code);

    // TGE
    code = 0x01090030; // tge t0, t1
    R(8) = 81;
    R(9) = -81;
    EXPECT_FAIL(code);
    cpu.exit = 0;
    R(8) = -81;
    R(9) = 81;
    EXPECT_PASS(code);

    // TGEU
    code = 0x01090031; // tgeu t0, t1
    R(8) = -81;
    R(9) = 81;
    EXPECT_FAIL(code);
    cpu.exit = 0;
    R(8) = 81;
    R(9) = -81;
    EXPECT_PASS(code);
}

TEST_F(InstructionTest, TestMthiMtlo) {
    R(8) = 8415;
    Word code = 0x01000011;
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(cpu.HI.read(), 8415);

    R(8) = 423;
    code = 0x01000013;
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(cpu.LO.read(), 423);
}

TEST_F(InstructionTest, TestMfc0) {
    cpu.set_mode(KERNEL, mem);
    R(26) = 4324;
    cpu.c0.status.set(34235);
    Word code = 0x401a6000; // mfc0 $k0, $12
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(26), 34235);

    // Sets register to zero if c0 register doesn't exist
    R(26) = 4861;
    code = 0x401a9000; // mfc0 $k0, $18
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(26), 0);
}

TEST_F(InstructionTest, TestMtc0) {
    cpu.set_mode(KERNEL, mem);
    R(26) = 245676;
    Word code = 0x409a6000; // mtc0 $k0, $12
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(cpu.c0.status.read(), 245676);
    EXPECT_EQ(R(26), 245676); // doesn't touch rt

    // ignores instruction if register doesn't exist
    code = 0x409a9000; // mtc0 $k0, $18
    EXPECT_PASS(0x409a9000);
    EXPECT_EQ(R(26), 245676); // doesn't touch rt
}

TEST_F(InstructionTest, TestInterruptsToggle) {
    cpu.set_mode(KERNEL, mem);
    R(26) = 4247;
    s32 v = c0.status.read();
    Word code = 0x417A6000; // di $k0
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(26), v);
    EXPECT_EQ(c0.get_interrupts_enabled(), false);

    v = c0.status.read();
    R(26) = 32323;
    code = 0x417A6020; // ei $k0
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(R(26), v);
    EXPECT_EQ(c0.get_interrupts_enabled(), false);
    cpu.set_mode(USER, mem);
    EXPECT_EQ(c0.get_interrupts_enabled(), true);
}

TEST_F(InstructionTest, TestEret) {
    cpu.set_mode(KERNEL, mem);
    c0.epc.set(0x2521);
    Word code = 0x42000018;
    cpu.Execute(mem, CPU::Decode(code));
    cpu.update_pc();
    EXPECT_EQ(cpu.PC.read(), 0x2521);
}

TEST_F(InstructionTest, TestWait) {
    EXPECT_EQ(cpu.powered, true);
    Word code = 0x42000020;
    cpu.Execute(mem, CPU::Decode(code));
    EXPECT_EQ(cpu.powered, false);

    c0.has_handler = true;
    cpu.set_mode(KERNEL, mem);
    mem.writeWord(EXC_VECTOR, 0x42000018); // put 'eret' in kernel text so exception immediately returns
    cpu.set_mode(USER, mem);
    cpu.raise_exception(INTERRUPT, CPU::Decode(0), mem);
    EXPECT_EQ(cpu.powered, true);
}