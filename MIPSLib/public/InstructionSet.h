#ifndef MIPS_INSTRUCTION_H
#define MIPS_INSTRUCTION_H

#include "utils.h"

using namespace MIPS;

/* TO ADD AN INSTRUCTION
- add its function prototype below
- add it to the opcode or funct table in init_opcode_table()
- define the function in InstructionSet.cpp
*/

struct MIPS::Instruction {
    unsigned int opcode : 6{}; // 6-bit opcode
    unsigned int rs : 5{};     // 5-bit rs
    unsigned int rt : 5{};     // 5-bit rt
    unsigned int rd : 5{};     // 5-bit rd
    unsigned int shamt : 5{};  // 5-bit shift amount
    unsigned int funct : 6{};  // 6-bit function code
    unsigned short imm{};      // 16-bit immediate
    unsigned int addr : 26{};  // 26-bit jump address

    Instruction() = default;

    static Instruction decode_instr(const Word machine_code) {
        Instruction instr;
        instr.opcode = (machine_code >> 26) & 0x3F;
        instr.rs = (machine_code >> 21) & 0x1F;
        instr.rt = (machine_code >> 16) & 0x1F;
        instr.rd = (machine_code >> 11) & 0x1F;
        instr.shamt = (machine_code >> 6) & 0x1F;
        instr.funct = machine_code & 0x3F;
        instr.imm = machine_code & 0xFFFF;
        instr.addr = machine_code & 0x03FFFFFF;
        return instr;
    }
};

void init_opcode_table(OPHandler (&)[64], OPHandler (&)[64]);

bool op_undefined(CPU&, Memory&, Instruction);
bool op_special(CPU&, Memory&, Instruction);

/* === R-TYPE === */
bool op_add(CPU&, Memory&, Instruction);
bool op_addu(CPU&, Memory&, Instruction);
bool op_and(CPU&, Memory&, Instruction);
bool op_jr(CPU&, Memory&, Instruction);
bool op_nor(CPU&, Memory&, Instruction);
bool op_or(CPU&, Memory&, Instruction);
bool op_slt(CPU&, Memory&, Instruction);
bool op_sltu(CPU&, Memory&, Instruction);
bool op_sll(CPU&, Memory&, Instruction);
bool op_srl(CPU&, Memory&, Instruction);
bool op_sllv(CPU&, Memory&, Instruction);
bool op_srlv(CPU&, Memory&, Instruction);
bool op_srav(CPU&, Memory&, Instruction);
bool op_sub(CPU&, Memory&, Instruction);
bool op_subu(CPU&, Memory&, Instruction);
bool op_div(CPU&, Memory&, Instruction);
bool op_divu(CPU&, Memory&, Instruction);
bool op_mfhi(CPU&, Memory&, Instruction);
bool op_mflo(CPU&, Memory&, Instruction);
bool op_mult(CPU&, Memory&, Instruction);
bool op_multu(CPU&, Memory&, Instruction);
bool op_sra(CPU&, Memory&, Instruction);
bool op_xor(CPU&, Memory&, Instruction);
bool op_tge(CPU&, Memory&, Instruction);
bool op_tgeu(CPU&, Memory&, Instruction);
bool op_tlt(CPU&, Memory&, Instruction);
bool op_tltu(CPU&, Memory&, Instruction);
bool op_teq(CPU&, Memory&, Instruction);
bool op_tne(CPU&, Memory&, Instruction);
bool op_seleqz(CPU&, Memory&, Instruction);
bool op_selnez(CPU&, Memory&, Instruction);

/* === I-TYPE === */
bool op_addi(CPU&, Memory&, Instruction);
bool op_addiu(CPU&, Memory&, Instruction);
bool op_andi(CPU&, Memory&, Instruction);
bool op_beq(CPU&, Memory&, Instruction);
bool op_bne(CPU&, Memory&, Instruction);
bool op_blez(CPU&, Memory&, Instruction);
bool op_bgtz(CPU&, Memory&, Instruction);
bool op_lui(CPU&, Memory&, Instruction);
bool op_lw(CPU&, Memory&, Instruction);
bool op_ori(CPU&, Memory&, Instruction);
bool op_slti(CPU&, Memory&, Instruction);
bool op_sltiu(CPU&, Memory&, Instruction);
bool op_sb(CPU&, Memory&, Instruction);
bool op_sh(CPU&, Memory&, Instruction);
bool op_sw(CPU&, Memory&, Instruction);
bool op_swl(CPU&, Memory&, Instruction);
bool op_swr(CPU&, Memory&, Instruction);
bool op_lb(CPU&, Memory&, Instruction);
bool op_lbu(CPU&, Memory&, Instruction);
bool op_lh(CPU&, Memory&, Instruction);
bool op_lhu(CPU&, Memory&, Instruction);
bool op_lwl(CPU&, Memory&, Instruction);
bool op_lwr(CPU&, Memory&, Instruction);

/* === OTHER === */
bool op_syscall(CPU&, Memory&, Instruction); // syscalls aren't actually instructions and should instead raise a syscall exception but it's easier this way
bool op_break(CPU&, Memory&, Instruction); // syscalls aren't actually instructions and should instead raise a syscall exception but it's easier this way

/* === J-TYPE === */
bool op_j(CPU&, Memory&, Instruction);
bool op_jal(CPU&, Memory&, Instruction);


#endif //MIPS_INSTRUCTION_H