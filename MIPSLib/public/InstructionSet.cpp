#include "Processor.h"
#include "InstructionSet.h"
#include "utils.h"
#include "Syscalls.h"
#include <climits>

using namespace MIPS;

void init_opcode_table(OPHandler (&op)[64], OPHandler (&funct)[64], OPHandler (&spec_2)[64]) {
    for (int i = 0; i < 64; i++) {
        op[i] = op_undefined;
        funct[i] = op_undefined;
        spec_2[i] = op_undefined;
    }

    op[0] = op_zero;
    op[0x02] = op_j;
    op[0x03] = op_jal;
    op[0x04] = op_beq;
    op[0x05] = op_bne;
    op[0x08] = op_addi;
    op[0x09] = op_addiu;
    op[0x0a] = op_slti;
    op[0x0b] = op_sltiu;
    op[0x0c] = op_andi;
    op[0x0d] = op_ori;
    op[0x0f] = op_lui;
    op[0x1c] = op_1c;
    op[0x20] = op_lb;
    op[0x23] = op_lw;
    op[0x24] = op_lbu;
    op[0x28] = op_sb;
    op[0x29] = op_sh;
    op[0x2b] = op_sw;

    funct[0x00] = op_sll;
    funct[0x02] = op_srl;
    funct[0x03] = op_sra;
    funct[0x08] = op_jr;
    funct[0x0c] = op_syscall;
    funct[0x10] = op_mfhi;
    funct[0x12] = op_mflo;
    funct[0x18] = op_mult;
    funct[0x19] = op_multu;
    funct[0x1a] = op_div;
    funct[0x1b] = op_divu;
    funct[0x20] = op_add;
    funct[0x21] = op_addu;
    funct[0x22] = op_sub;
    funct[0x23] = op_subu;
    funct[0x24] = op_and;
    funct[0x25] = op_or;
    funct[0x26] = op_xor;
    funct[0x27] = op_nor;
    funct[0x2a] = op_slt;
    funct[0x2b] = op_sltu;

    spec_2[0x02] = op_mul;
}

s32 signExtend(const unsigned short imm) {
    return static_cast<s32>(static_cast<signed short>(imm)); // make signed, then expand to 32
}

s32 signExtend(const Byte imm) {
    return static_cast<s32>(static_cast<signed char>(imm)); // make signed, then expand to 32
}

s32 zeroExtend(const unsigned short imm) {
    return static_cast<s32>(imm);
}

s32 zeroExtend(const Byte imm) {
    return static_cast<s32>(imm);
}

bool op_undefined(CPU &, Memory&, Instruction) {
    throw std::runtime_error("undefined opcode");
}

// Opcode zero (SPECIAL encoding)
bool op_zero(CPU & cpu, Memory & mem, const Instruction instruction) {
    return cpu.funct_table[instruction.funct](cpu, mem, instruction);
}

// Opcode 28 (SPECIAL2 encoding)
bool op_1c(CPU &cpu, Memory &mem, const Instruction instruction) {
    return cpu.spec2_table[instruction.funct](cpu, mem, instruction);
}

// macro for instructions of the form rd = f(rs, rt)
#define rfunc(operation) cpu.RF[instruction.rd] = cpu.RF[instruction.rs] operation cpu.RF[instruction.rt];

// macro for simpler register accesses
#define R(reg) cpu.RF[instruction.reg]

// macros for simpler casts
#define toU32(x) static_cast<u32>(x)
#define toS32(x) static_cast<s32>(x)
#define toS64(x) static_cast<int64_t>(x)
#define toU64(x) static_cast<uint64_t>(toU32(x)) // need to cast to unsigned first, then to long

// R TYPE

bool op_add(CPU &cpu, Memory &, const Instruction instruction) {
    rfunc(+);
    // detect overflow
    if (R(rt) > 0 ? INT_MAX-R(rt) < R(rs) : INT_MIN-R(rt) > R(rs)) {
        cpu.raise_exception(ARITHMETIC_OVERFLOW_EXCEPTION);
        return false;
    }
    return true;
}

bool op_addu(CPU &cpu, Memory &, const Instruction instruction) {
    rfunc(+);
    return true;
}

bool op_and(CPU &cpu, Memory &, const Instruction instruction) {
    rfunc(&);
    return true;
}

bool op_jr(CPU &cpu, Memory &, const Instruction instruction) {
    cpu.queue_pc_update(R(rs));
    return true;
}

bool op_nor(CPU &cpu, Memory &, const Instruction instruction) {
    R(rd) = ~(R(rs) | R(rt));
    return true;
}

bool op_or(CPU &cpu, Memory &, const Instruction instruction) {
    rfunc(|);
    return true;
}

bool op_slt(CPU &cpu, Memory &, const Instruction instruction) {
    rfunc(<);
    return true;
}

bool op_sltu(CPU &cpu, Memory &, const Instruction instruction) {
    R(rd) = toU32(R(rs)) < toU32(R(rt));
    return true;
}

// Also counts as 'nop'
bool op_sll(CPU &cpu, Memory &, const Instruction instruction) {

    // Get shamt mod 32
    unsigned char shamt = (instruction.shamt % 32 + 32) % 32;
    R(rd) = R(rt) << shamt;
    return true;
}

bool op_srl(CPU &cpu, Memory &, const Instruction instruction) {
    // get shamt mod 32
    unsigned char shamt = (instruction.shamt % 32 + 32) % 32;
    // first perform on unsigned to guarantee logical shift
    u32 res = toU32(R(rt)) >> shamt;
    R(rd) = toS32(res);
    return true;
}

bool op_sub(CPU &cpu, Memory &, const Instruction instruction) {
    rfunc(-);
    if ( R(rs) > 0 ? R(rt) < 0 && R(rd) < 0 : R(rt) > 0 && R(rd) > 0 ) {
        cpu.raise_exception(ARITHMETIC_OVERFLOW_EXCEPTION);
        return false;
    }
    return true;
}

bool op_subu(CPU &cpu, Memory &, const Instruction instruction) {
    rfunc(-);
    return true;
}

bool op_div(CPU &cpu, Memory &, const Instruction instruction) {
    cpu.LO.set(R(rs) / R(rt));
    cpu.HI.set(R(rs) % R(rt));
    return true;
}

bool op_divu(CPU &cpu, Memory &, const Instruction instruction) {
    u32 div_res = toU32(R(rs)) / toU32(R(rt));
    u32 mod_res = toU32(R(rs)) % toU32(R(rt));
    cpu.LO.set(toS32(div_res));
    cpu.HI.set(toS32(mod_res));
    return true;
}

bool op_mfhi(CPU &cpu, Memory &, const Instruction instruction) {
    R(rd) = cpu.HI.read();
    return true;
}

bool op_mflo(CPU &cpu, Memory &, const Instruction instruction) {
    R(rd) = cpu.LO.read();
    return true;
}

bool op_mult(CPU &cpu, Memory &, const Instruction instruction) {
    const int64_t full = toS64(R(rs)) * toS64(R(rt));
    cpu.LO.set(toS32(full & 0x00000000FFFFFFFF));
    cpu.HI.set(toS32(full >> 32));
    return true;
}

bool op_multu(CPU &cpu, Memory &, const Instruction instruction) {
    const uint64_t full = toU64(R(rs)) * toU64(R(rt));
    cpu.LO.set(toS32(full & 0x00000000FFFFFFFF));
    cpu.HI.set(toS32(full >> 32));
    return true;
}

bool op_sra(CPU &cpu, Memory &, const Instruction instruction) {
    // get shamt mod 32
    unsigned char shamt = (instruction.shamt % 32 + 32) % 32;
    R(rd) = R(rt) >> shamt; // registers stored s32, so C++ performs arithmetic shift by default
    return true;
}

bool op_mul(CPU &cpu, Memory &, const Instruction instruction) {
    const int64_t full = toS64(R(rs)) * toS64(R(rt));
    R(rd) = toS32(full & 0x00000000FFFFFFFF);
    return true;
}

bool op_syscall(CPU &cpu, Memory &mem, const Instruction) {
    return do_syscall(cpu, mem);
}

bool op_xor(CPU &cpu, Memory &, const Instruction instruction) {
    rfunc(xor);
    return true;
}

// I TYPE

bool op_addi(CPU &cpu, Memory &, const Instruction instruction) {
    const s32 imm = signExtend(instruction.imm);
    R(rt) = R(rs) + imm;
    if (R(rs) > 0 ? INT_MAX-R(rs) < imm : INT_MIN-R(rs) > imm) {
        cpu.raise_exception(ARITHMETIC_OVERFLOW_EXCEPTION);
        return false;
    }
    return true;
}

bool op_addiu(CPU &cpu, Memory &, const Instruction instruction) {
    R(rt) = R(rs) + signExtend(instruction.imm);
    return true;
}

bool op_andi(CPU &cpu, Memory &, const Instruction instruction) {
    const s32 imm = zeroExtend(instruction.imm);
    R(rt) = R(rs) & imm;
    return true;
}

bool op_beq(CPU &cpu, Memory &, const Instruction instruction) {
    // note PC was incremented in the fetch stage
    if (R(rs) == R(rt)) {
        const s32 imm = static_cast<signed short>(instruction.imm) << 2;
        cpu.queue_pc_update(cpu.PC.read() + 4 + imm);
    }
    return true;
}

bool op_bne(CPU &cpu, Memory &, const Instruction instruction) {
    // note PC was incremented in the fetch stage
    if (R(rs) != R(rt)) {
        const s32 imm = static_cast<signed short>(instruction.imm) << 2;
        cpu.queue_pc_update(cpu.PC.read() + 4 + imm);
    }
    return true;
}

bool op_lui(CPU &cpu, Memory &, const Instruction instruction) {
    const s32 imm = instruction.imm << 16;
    R(rt) = imm;
    return true;
}

bool op_lw(CPU &cpu, Memory &mem, const Instruction instruction) {
    const s32 imm = signExtend(instruction.imm);
    const Word addr = R(rs) + imm;
    if (addr % 4 != 0) {
        cpu.c0->vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD);
        return false;
    }
    Word value;
    try {
        value = mem.readWord(addr);
    } catch (const std::out_of_range&) {
        cpu.c0->vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD);
        return false;
    }
    R(rt) = toS32(value);
    return true;
}

bool op_ori(CPU &cpu, Memory &, const Instruction instruction) {
    const s32 imm = zeroExtend(instruction.imm);
    R(rt) = R(rs) | imm;
    return true;
}

bool op_slti(CPU &cpu, Memory &, const Instruction instruction) {
    s32 imm = signExtend(instruction.imm);
    R(rt) = R(rs) < imm;
    return true;
}

bool op_sltiu(CPU &cpu, Memory &, const Instruction instruction) {
    s32 imm = signExtend(instruction.imm);
    R(rt) = toU32(R(rs)) < toU32(imm);
    // TODO: check if this is right. sign extending an unsigned value feels weird
    return true;
}

bool op_sb(CPU &cpu, Memory &mem, const Instruction instruction) {
    const s32 imm = signExtend(instruction.imm);
    const Word addr = R(rs) + imm;
    const Byte b = R(rt) & 0xFF;
    try {
        mem.writeByte(addr, b);
    } catch (const std::out_of_range&) {
        cpu.c0->vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_STORE);
        return false;
    }

    return true;
}

bool op_sh(CPU &cpu, Memory &mem, const Instruction instruction) {
    const s32 imm = signExtend(instruction.imm);
    const Word addr = R(rs) + imm;
    if (addr % 2 != 0) {
        cpu.c0->vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_STORE);
        return false;
    }
    const unsigned short b0 = R(rt) & 0xFF;
    const unsigned short b1 = R(rt) >> 8 & 0xFF;
    try {
        mem.writeByte(addr, b0);
        mem.writeByte(addr+1, b1);
    } catch (const std::out_of_range&) {
        cpu.c0->vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_STORE);
        return false;
    }
    return true;
}

bool op_sw(CPU &cpu, Memory &mem, const Instruction instruction) {
    const s32 imm = signExtend(instruction.imm);
    const Word addr = R(rs) + imm;
    if (addr % 4 != 0) {
        cpu.c0->vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD);
        return false;
    }
    try {
        mem.writeWord(addr, R(rt));
    } catch (const std::out_of_range&) {
        cpu.c0->vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_STORE);
        return false;
    }
    return true;
}

bool op_lb(CPU &cpu, Memory &mem, const Instruction instruction) {
    const s32 imm = signExtend(instruction.imm);
    const Word addr = R(rs) + imm;
    Byte value;
    try {
        value = mem.readByte(addr);
    } catch (const std::out_of_range&) {
        cpu.c0->vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD);
        return false;
    }
    // static_cast<s32>(static_cast<signed short>(imm));
    R(rt) = signExtend(value);
    return true;
}

bool op_lbu(CPU &cpu, Memory &mem, const Instruction instruction) {
    const s32 imm = signExtend(instruction.imm);
    const Word addr = R(rs) + imm;
    Byte value;
    try {
        value = mem.readByte(addr);
    } catch (const std::out_of_range&) {
        cpu.c0->vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD);
        return false;
    }
    R(rt) = zeroExtend(value);
    return true;
}

// J TYPE

bool op_j(CPU &cpu, Memory &, const Instruction instruction) {
    const Word jaddr = ((cpu.PC.read()+4) & 0xF0000000) | (instruction.addr << 2 & 0xFFFFFFFC);
    cpu.queue_pc_update(toS32(jaddr));
    return true;
}

bool op_jal(CPU &cpu, Memory &, const Instruction instruction) {
    const Word jaddr = ((cpu.PC.read()+4) & 0xF0000000) | (instruction.addr << 2 & 0xFFFFFFFC);
    cpu.RF[31] = cpu.PC.read() + 4;
    cpu.queue_pc_update(toS32(jaddr));
    return true;
}