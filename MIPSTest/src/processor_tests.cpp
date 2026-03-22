#include "gtest/gtest.h"
#include "Processor.h"
#include "InstructionSet.h"
#include "utils.h"

using namespace MIPS;

class ProcessorTest : public testing::Test {
protected:

    Memory mem;
    Coprocessor0 c0;
    CPU cpu{c0, std::cin, std::cout};
    Word entry = 0;

    void SetUp() override {
        cpu.set_pc_entry(0x00400000);
        entry = cpu.PC.reset_value;
        cpu.PC.reset();
    }

    void TearDown() override {}
};

TEST_F(ProcessorTest, CanFetch) {
    EXPECT_EQ(cpu.PC.read(), entry);       // PC starts in correct location
    Word instr = cpu.Fetch(mem);
    EXPECT_EQ(instr, mem.readWord(entry)); // PC fetches correct instruction

    cpu.update_pc();

    // Supports successive fetches
    instr = cpu.Fetch(mem);
    EXPECT_EQ(instr, mem.readWord(entry+4));
    EXPECT_EQ(cpu.PC.read(), entry+4);
}

TEST_F(ProcessorTest, CanDecode) {
    constexpr Word r_type = 0x013d4020; // add $t0 $t1 $sp (add $8 $9 $29)
    const Instruction instr = CPU::Decode(r_type);
    EXPECT_EQ(instr.opcode, 0);
    EXPECT_EQ(instr.rs, 9);
    EXPECT_EQ(instr.rt, 29);
    EXPECT_EQ(instr.rd, 8);
    EXPECT_EQ(instr.shamt, 0);
    EXPECT_EQ(instr.funct, 0x20);

    constexpr Word i_type = 0x8db40138; // lw $s4 312($t5) (lw $20 312($13)
    const Instruction instr2 = CPU::Decode(i_type);
    EXPECT_EQ(instr2.opcode, 0x23);
    EXPECT_EQ(instr2.rs, 13);
    EXPECT_EQ(instr2.rt, 20);
    EXPECT_EQ(instr2.imm, 312);

    constexpr Word j_type = 0x0C100000; // jal (0x0100000)
    const Instruction instr3 = CPU::Decode(j_type);
    EXPECT_EQ(instr3.opcode, 0x3);
    EXPECT_EQ(instr3.addr, 0x0100000);
}

TEST_F(ProcessorTest, ExceptionHandlerReturnsControl) {
    // test if after a non-fatal exception, control is returned to cpu at the following instruction
    cpu.PC.set(0x324); // exception occured at 324
    EXPECT_NO_THROW(cpu.raise_exception(INTERRUPT, CPU::Decode(0)));
    EXPECT_EQ(cpu.newPC, 0x328);
    EXPECT_EQ(cpu.c0.cause.read(), 0);
    EXPECT_EQ(cpu.c0.status.read() & 1, 1); // if last bit is 1
}

TEST_F(ProcessorTest, ZeroRegisterPreserved) {
    cpu.RF[9] = 352;
    cpu.RF[10] = 865;
    cpu.Execute(mem, CPU::Decode(0x012a0020));
    EXPECT_EQ(cpu.RF[0], 0);
}