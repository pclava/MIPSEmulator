#include "Processor.h"
#include "InstructionSet.h"
#include "utils.h"
#include <climits>

#include "tlb.h"

using namespace MIPS;

void init_opcode_table(OPHandler (&op)[64], OPHandler (&funct)[64], OPHandler (&cop0)[17]) {
    for (int i = 0; i < 64; i++) {
        op[i] = op_undefined;
        funct[i] = op_undefined;
        if (i < 17) cop0[i] = op_undefined;
    }

    // Indexed by opcode
    op[0x00] = op_special;
    op[0x01] = op_regimm;
    op[0x02] = op_j;
    op[0x03] = op_jal;
    op[0x04] = op_beq;
    op[0x05] = op_bne;
    op[0x06] = op_blez;
    op[0x07] = op_bgtz;
    op[0x08] = op_addi;
    op[0x09] = op_addiu;
    op[0x0a] = op_slti;
    op[0x0b] = op_sltiu;
    op[0x0c] = op_andi;
    op[0x0d] = op_ori;
    op[0x0e] = op_xori;
    op[0x0f] = op_lui;
    op[0x10] = op_cop0;
    op[0x1c] = op_special2;
    op[0x1f] = op_special3;
    op[0x20] = op_lb;
    op[0x21] = op_lh;
    op[0x22] = op_lwl;
    op[0x23] = op_lw;
    op[0x24] = op_lbu;
    op[0x25] = op_lhu;
    op[0x26] = op_lwr;
    op[0x28] = op_sb;
    op[0x29] = op_sh;
    op[0x2a] = op_swl;
    op[0x2b] = op_sw;
    op[0x2e] = op_swr;

    // Indexed by funct code
    funct[0x00] = op_sll;
    funct[0x02] = op_srl;
    funct[0x03] = op_sra;
    funct[0x04] = op_sllv;
    funct[0x06] = op_srlv;
    funct[0x07] = op_srav;
    funct[0x08] = op_jr;
    funct[0x09] = op_jalr;
    funct[0x0a] = op_movz;
    funct[0x0b] = op_movn;
    funct[0x0c] = op_syscall;
    funct[0x0d] = op_break;
    funct[0x10] = op_mfhi;
    funct[0x11] = op_mthi;
    funct[0x12] = op_mflo;
    funct[0x13] = op_mtlo;
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
    funct[0x30] = op_tge;
    funct[0x31] = op_tgeu;
    funct[0x32] = op_tlt;
    funct[0x33] = op_tltu;
    funct[0x34] = op_teq;
    funct[0x36] = op_tne;

    // Indexed by RS
    cop0[0x00] = op_mfc0;
    cop0[0x04] = op_mtc0;
    cop0[0x0b] = op_interrupts;
    cop0[0x10] = op_c0;
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

bool op_undefined(CPU &cpu, Memory&mem, Instruction instruction) {
    printf("PC: 0x%.8x | opcode: 0x%.2x | instruction: 0x%.8x\n", cpu.PC.read(), instruction.opcode, mem.readWord(cpu.PC.read()));
    throw std::runtime_error("undefined opcode");
}

// Opcode zero (SPECIAL encoding)
bool op_special(CPU & cpu, Memory & mem, const Instruction instruction) {
    return cpu.funct_table[instruction.funct](cpu, mem, instruction);
}

// Opcode 17 (COP0 encoding)
bool op_cop0(CPU & cpu, Memory & mem, const Instruction instruction) {
    return cpu.cop0_table[instruction.rs](cpu, mem, instruction);
}

// macro for instructions of the form rd = rs (operator) rt
#define rfunc(operation) cpu.RF[instruction.rd] = cpu.RF[instruction.rs] operation cpu.RF[instruction.rt];

// macro for simpler register accesses
#define R(reg) cpu.RF[instruction.reg]

// macros for simpler casts
#define toU32(x) static_cast<u32>(x)
#define toS32(x) static_cast<s32>(x)
#define toS64(x) static_cast<int64_t>(x)
#define toU64(x) static_cast<uint64_t>(toU32(x)) // need to cast to unsigned first, then to long

// R TYPE

bool op_add(CPU &cpu, Memory &mem, const Instruction instruction) {
    rfunc(+);
    // detect overflow
    if (R(rt) > 0 ? INT_MAX-R(rt) < R(rs) : INT_MIN-R(rt) > R(rs)) {
        cpu.raise_exception(ARITHMETIC_OVERFLOW_EXCEPTION, instruction, mem);
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

bool op_jalr(CPU &cpu, Memory &, const Instruction instruction) {
    R(rd) = cpu.PC.read() + 4;
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

bool op_sub(CPU &cpu, Memory &mem, const Instruction instruction) {
    rfunc(-);
    if ( R(rs) > 0 ? R(rt) < 0 && R(rd) < 0 : R(rt) > 0 && R(rd) > 0 ) {
        cpu.raise_exception(ARITHMETIC_OVERFLOW_EXCEPTION, instruction, mem);
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

bool op_mthi(CPU &cpu, Memory &, const Instruction instruction) {
    cpu.HI.set(R(rs));
    return true;
}

bool op_mtlo(CPU &cpu, Memory &, const Instruction instruction) {
    cpu.LO.set(R(rs));
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
    R(rd) = R(rt) >> shamt; // registers store s32, so C++ performs arithmetic shift by default
    return true;
}

bool op_sllv(CPU &cpu, Memory &, const Instruction instruction) {
    // Get low-order 5 bits of rs and shift by that amount
    unsigned char shamt = R(rs) & 0x1f;
    R(rd) = R(rt) << shamt;
    return true;
}

bool op_srlv(CPU &cpu, Memory &, const Instruction instruction) {
    // Get low-order 5 bits of rs and shift by that amount
    unsigned char shamt = R(rs) & 0x1f;
    // first perform on unsigned to guarantee logical shift
    u32 res = toU32(R(rt)) >> shamt;
    R(rd) = toS32(res);
    return true;
}

bool op_srav(CPU &cpu, Memory &, const Instruction instruction) {
    // Get low-order 5 bits of rs and shift by that amount
    unsigned char shamt = R(rs) & 0x1f;
    R(rd) = R(rt) >> shamt; // registers store s32, so C++ performs arithmetic shift by default
    return true;
}

bool op_syscall(CPU &cpu, Memory &mem, const Instruction instr) {
    cpu.raise_exception(SYSCALL_EXCEPTION, instr, mem);
    return true;
}

bool op_break(CPU &cpu, Memory &, const Instruction) {
    printf("BREAK!\n$v0 = 0x%.8x\n"
            "Cause = 0x%.8x\n"
            "ExcCode = %d\n"
            "Status = 0x%.8x\n"
            "EPC = 0x%.8x\n"
            "Bad = 0x%.8x\n"
            "PC = 0x%.8x\n"
            "ASID = %d\n",
           cpu.RF[2], cpu.c0.cause.read(), (cpu.c0.cause.read() & 0b1111100) >> 2, cpu.c0.status.read(), cpu.c0.epc.read(), cpu.c0.bad.read(), cpu.PC.read(), cpu.c0.get_asid());
    // cpu.raise_exception(BREAKPOINT_EXCEPTION, instruction, mem);
    return true;
}

bool op_xor(CPU &cpu, Memory &, const Instruction instruction) {
    rfunc(xor);
    return true;
}

bool op_tge(CPU &cpu, Memory &mem, const Instruction instruction) {
    if (R(rs) >= R(rt)) cpu.raise_exception(TRAP_EXCEPTION, instruction, mem);
    return true;
}

bool op_tgeu(CPU &cpu, Memory &mem, const Instruction instruction) {
    if (toU32(R(rs)) >= toU32(R(rt))) cpu.raise_exception(TRAP_EXCEPTION, instruction, mem);
    return true;
}

bool op_tlt(CPU &cpu, Memory &mem, const Instruction instruction) {
    if (R(rs) < R(rt)) cpu.raise_exception(TRAP_EXCEPTION, instruction, mem);
    return true;
}

bool op_tltu(CPU &cpu, Memory &mem, const Instruction instruction) {
    if (toU32(R(rs)) < toU32(R(rt))) cpu.raise_exception(TRAP_EXCEPTION, instruction, mem);
    return true;
}

bool op_teq(CPU &cpu, Memory &mem, const Instruction instruction) {
    if (R(rs) == R(rt)) cpu.raise_exception(TRAP_EXCEPTION, instruction, mem);
    return true;
}

bool op_tne(CPU &cpu, Memory &mem, const Instruction instruction) {
    if (R(rs) != R(rt)) cpu.raise_exception(TRAP_EXCEPTION, instruction, mem);
    return true;
}

bool op_movz(CPU &cpu, Memory &, const Instruction instruction) {
    if (R(rt) == 0) R(rd) = R(rs);
    return true;
}

bool op_movn(CPU &cpu, Memory &, const Instruction instruction) {
    if (R(rt) != 0) R(rd) = R(rs);
    return true;
}

// I TYPE

bool op_addi(CPU &cpu, Memory &mem, const Instruction instruction) {
    const s32 imm = signExtend(instruction.imm);
    R(rt) = R(rs) + imm;
    if (R(rs) > 0 ? INT_MAX-R(rs) < imm : INT_MIN-R(rs) > imm) {
        cpu.raise_exception(ARITHMETIC_OVERFLOW_EXCEPTION, instruction, mem);
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

bool op_blez(CPU &cpu, Memory &, const Instruction instruction) {
    // note PC was incremented in the fetch stage
    if (R(rs) <= 0) {
        const s32 imm = static_cast<signed short>(instruction.imm) << 2;
        cpu.queue_pc_update(cpu.PC.read() + 4 + imm);
    }
    return true;
}

bool op_bgtz(CPU &cpu, Memory &, const Instruction instruction) {
    // note PC was incremented in the fetch stage
    if (R(rs) >= 0) {
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
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD, instruction, mem);
        return false;
    }
    Word value;
    try {
        value = mem.readWord(addr);
    } catch (const std::out_of_range&) {
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD, instruction, mem);
        return false;
    } catch (const TLBMissException&) {
        cpu.raise_tlbl(mem, toS32(addr), instruction);
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

bool op_xori(CPU &cpu, Memory &, const Instruction instruction) {
    const s32 imm = zeroExtend(instruction.imm);
    R(rt) = R(rs) ^ imm;
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
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_STORE, instruction, mem);
        return false;
    } catch (const TLBMissException&) {
        cpu.raise_tlbs(mem, toS32(addr), instruction);
        return false;
    } catch (const TLBModifyException&) {
        cpu.raise_tlbmod(mem, toS32(addr), instruction);
        return false;
    }

    return true;
}

bool op_sh(CPU &cpu, Memory &mem, const Instruction instruction) {
    const s32 imm = signExtend(instruction.imm);
    const Word addr = R(rs) + imm;
    if (addr % 2 != 0) {
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_STORE, instruction, mem);
        return false;
    }
    const unsigned short b0 = R(rt) & 0xFF;
    const unsigned short b1 = R(rt) >> 8 & 0xFF;
    try {
        mem.writeByte(addr, b0);
        mem.writeByte(addr+1, b1);
    } catch (const std::out_of_range&) {
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_STORE, instruction, mem);
        return false;
    } catch (const TLBMissException&) {
        cpu.raise_tlbs(mem, toS32(addr), instruction);
        return false;
    } catch (const TLBModifyException&) {
        cpu.raise_tlbmod(mem, toS32(addr), instruction);
        return false;
    }
    return true;
}

bool op_sw(CPU &cpu, Memory &mem, const Instruction instruction) {
    const s32 imm = signExtend(instruction.imm);
    const Word addr = R(rs) + imm;
    if (addr % 4 != 0) {
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_STORE, instruction, mem);
        return false;
    }
    try {
        mem.writeWord(addr, R(rt));
    } catch (const std::out_of_range&) {
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_STORE, instruction, mem);
        return false;
    } catch (const TLBMissException&) {
        cpu.raise_tlbs(mem, toS32(addr), instruction);
        return false;
    } catch (const TLBModifyException&) {
        cpu.raise_tlbmod(mem, toS32(addr), instruction);
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
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD, instruction, mem);
        return false;
    } catch (const TLBMissException&) {
        cpu.raise_tlbl(mem, toS32(addr), instruction);
        return false;
    }
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
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD, instruction, mem);
        return false;
    } catch (const TLBMissException&) {
        cpu.raise_tlbl(mem, toS32(addr), instruction);
        return false;
    }
    R(rt) = zeroExtend(value);
    return true;
}

bool op_lh(CPU &cpu, Memory &mem, const Instruction instruction) {
    const s32 imm = signExtend(instruction.imm);
    const Word addr = R(rs) + imm;
    if (addr % 2 != 0) {
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD, instruction, mem);
        return false;
    }
    Half value = 0;
    try {
        value |= mem.readByte(addr);
        value |= mem.readByte(addr+1) << 8;
    } catch (const std::out_of_range&) {
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD, instruction, mem);
        return false;
    } catch (const TLBMissException&) {
        cpu.raise_tlbl(mem, toS32(addr), instruction);
        return false;
    }
    R(rt) = signExtend(value);
    return true;
}

bool op_lhu(CPU &cpu, Memory &mem, const Instruction instruction) {
    const s32 imm = signExtend(instruction.imm);
    const Word addr = R(rs) + imm;
    if (addr % 2 != 0) {
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD, instruction, mem);
        return false;
    }
    Half value = 0;
    try {
        value |= mem.readByte(addr);
        value |= mem.readByte(addr+1) << 8;
    } catch (const std::out_of_range&) {
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD, instruction, mem);
        return false;
    } catch (const TLBMissException&) {
        cpu.raise_tlbl(mem, toS32(addr), instruction);
        return false;
    }
    R(rt) = zeroExtend(value);
    return true;
}

bool op_lwl(CPU &cpu, Memory &mem, const Instruction instruction) {
    const s32 imm = signExtend(instruction.imm);
    const Word addr = R(rs) + imm; // address of most significant byte of the word
    Word value = 0;
    const Word offset = addr % 4; // how far left (in bytes) the word boundary is
    unsigned char j = 24; // start at most significant byte
    try {
        for (Word i = 0; i <= offset; i++) {
            value |= mem.readByte(addr - i) << j;
            j -= 8;
        }
    } catch (const std::out_of_range&) {
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD, instruction, mem);
        return false;
    } catch (const TLBMissException&) {
        cpu.raise_tlbl(mem, toS32(addr), instruction);
        return false;
    }
    // Preserve LSBs of rt
    // j+8 represents number of bits that were not read from memory
    // extract the lower j+8 bits of rt and OR with value
    R(rt) = toS32(value) | (R(rt) & ((1 << (j+8))-1));
    return true;
}

bool op_lwr(CPU &cpu, Memory &mem, const Instruction instruction) {
    const s32 imm = signExtend(instruction.imm);
    const Word addr = R(rs) + imm; // address of least significant byte of the word
    Word value = 0;
    const Word offset = 4-(addr % 4); // how far right (in bytes) the word boundary is
    unsigned char j = 0; // start at least significant byte
    try {
        for (Word i = 0; i < offset; i++) {
            value |= mem.readByte(addr + i) << j;
            j += 8;
        }
    } catch (const std::out_of_range&) {
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD, instruction, mem);
        return false;
    } catch (const TLBMissException&) {
        cpu.raise_tlbl(mem, toS32(addr), instruction);
        return false;
    }
    // Preserve MSBs of rt
    // j represents number of bits that were read from memory
    // extract upper 32-j bits of rt
    s32 mask = (1 << (32-j)) - 1;
    mask <<= j;
    R(rt) = (R(rt) & mask) | (toS32(value));
    return true;
}

bool op_swl(CPU &cpu, Memory &mem, const Instruction instruction) {
    const s32 imm = signExtend(instruction.imm);
    const Word addr = R(rs) + imm; // address of most significant byte of the word
    const Word offset = addr % 4; // how far left (in bytes) the word boundary is
    unsigned char j = 24; // start at most significant byte
    try {
        for (Word i = 0; i <= offset; i++) {
            // Extract j'th byte from register
            Byte x = R(rt) >> j & 0xFF;
            // Store at addr-i
            mem.writeByte(addr-i, x);
            j-=8;
        }
    } catch (const std::out_of_range&) {
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_STORE, instruction, mem);
        return false;
    } catch (const TLBMissException&) {
        cpu.raise_tlbs(mem, toS32(addr), instruction);
        return false;
    } catch (const TLBModifyException&) {
        cpu.raise_tlbmod(mem, toS32(addr), instruction);
        return false;
    }
    return true;
}

bool op_swr(CPU &cpu, Memory &mem, const Instruction instruction) {
    const s32 imm = signExtend(instruction.imm);
    const Word addr = R(rs) + imm; // address of least significant byte of the word
    const Word offset = 4-(addr % 4); // how far right (in bytes) the word boundary is
    unsigned char j = 0; // start at most significant byte
    try {
        for (Word i = 0; i < offset; i++) {
            // Extract j'th byte from register
            Byte x = R(rt) >> j & 0xFF;
            // Store at addr-i
            mem.writeByte(addr+i, x);
            j+=8;
        }
    } catch (const std::out_of_range&) {
        cpu.c0.vaddr.set(toS32(addr));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD, instruction, mem);
        return false;
    } catch (const TLBMissException&) {
        cpu.raise_tlbs(mem, toS32(addr), instruction);
        return false;
    } catch (const TLBModifyException&) {
        cpu.raise_tlbmod(mem, toS32(addr), instruction);
        return false;
    }
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

// REGIMM
bool op_regimm(CPU & cpu, Memory & mem, const Instruction instruction) {
    switch (instruction.rt) {
        case 0b0:
            return op_bltz(cpu, mem, instruction);
        case 0b1:
            return op_bgez(cpu, mem, instruction);
        default:
            return false;
    }
}

bool op_bltz(CPU &cpu, Memory &, const Instruction instruction) {
    if (R(rs) < 0) {
        const s32 imm = static_cast<signed short>(instruction.imm) << 2;
        cpu.queue_pc_update(cpu.PC.read() + 4 + imm);
    }
    return true;
}

bool op_bgez(CPU &cpu, Memory &, const Instruction instruction) {
    if (R(rs) >= 0) {
        const s32 imm = static_cast<signed short>(instruction.imm) << 2;
        cpu.queue_pc_update(cpu.PC.read() + 4 + imm);
    }
    return true;
}

// SPECIAL2
bool op_special2(CPU &cpu, Memory &mem, const Instruction instruction) {
    switch (instruction.funct) {
        case 0x02:
            return op_mul(cpu, mem, instruction);
        default:
            return false;
    }
}

bool op_mul(CPU &cpu, Memory &, const Instruction instruction) {
    R(rd) = R(rs) * R(rt);
    return true;
}

// SPECIAL3
bool op_special3(CPU &cpu, Memory &mem, const Instruction instruction) {
    switch (instruction.funct) {
        case 0x00:
            return op_ext(cpu, mem, instruction);
        case 0x04:
            return op_ins(cpu, mem, instruction);
        case 0x20:
            return op_bshfl(cpu, mem, instruction);
        default:
            return false;
    }
}

bool op_ext(CPU &cpu, Memory &, const Instruction instruction) {
    const u32 size = instruction.rd % 32;
    const u32 pos = instruction.shamt % 32;
    if (pos+size > 32) return false;

    // Mask out bits [pos+size-1 : pos] of RS
    const u32 mask = (1u << size) - 1;
    u32 v = R(rs) >> pos;
    v &= mask;
    R(rt) = v;

    return true;
}

bool op_ins(CPU &cpu, Memory &, const Instruction instruction) {
    const u32 size = instruction.rd % 32;
    const u32 pos = instruction.shamt % 32;
    if (pos+size > 32) return false;

    // Extract lower size bits of RS
    u32 mask = (1u << size) - 1;
    u32 v = R(rs) & mask;
    v <<= pos;
    mask <<= pos;
    R(rt) &= ~(mask);
    R(rt) |= v;

    return true;
}

bool op_bshfl(CPU &cpu, Memory &mem, const Instruction instruction) {
    switch (instruction.shamt) {
        case 0b10000:
            return op_seb(cpu, mem, instruction);
        case 0b11000:
            return op_seh(cpu, mem, instruction);
        default:
            return false;
    }
}

bool op_seb(CPU &cpu, Memory &, const Instruction instruction) {
    R(rd) = static_cast<s32>(static_cast<int8_t>(R(rt)));
    return true;
}

bool op_seh(CPU &cpu, Memory &, const Instruction instruction) {
    R(rd) = static_cast<s32>(static_cast<int16_t>(R(rt)));
    return true;
}

// COP0

bool op_mfc0(CPU &cpu, Memory&, const Instruction instruction) {
    // Puts the value of the c0 register rd into gpr rt
    // Get c0 register. if it doesn't exist, return 0
    if (!cpu.c0.in_kernel()) return true; // ignore if in user mode
    try {
        Register &rd = cpu.c0.get_register(instruction.rd);
        R(rt) = rd.read();
    } catch (const std::out_of_range&) {
        R(rt) = 0;
    }
    return true;
}

bool op_mtc0(CPU &cpu, Memory&, const Instruction instruction) {
    // Puts the value of the gpr register rt into c0 register rd
    // Get the c0 register. if it doesn't exist, ignore the instruction
    if (!cpu.c0.in_kernel()) return true; // ignore if in user mode
    try {
        Register &rd = cpu.c0.get_register(instruction.rd);
        rd.set(R(rt));
        // printf("Setting c0 register %u to %u", rd.number, R(rt));
    } catch (const std::out_of_range&) {
    }
    return true;
}

bool op_interrupts(CPU &cpu, Memory&, const Instruction instruction) {
    switch (instruction.funct) {
        case 0:
            R(rt) = cpu.c0.status.read();
            cpu.c0.set_interrupts_enabled(false);
            break;
        case 32:
            R(rt) = cpu.c0.status.read();
            cpu.c0.set_interrupts_enabled(true);
            break;
        default:
            return true;
    }
    return true;
}

bool op_c0(CPU &cpu, Memory &mem, const Instruction instruction) {
    switch (instruction.funct) {
        case 0x01:
            return op_tlbr(cpu, mem, instruction);
        case 0x02:
            return op_tlbwi(cpu, mem, instruction);
        case 0x08:
            return op_tlbp(cpu, mem, instruction);
        case 0x18:
            return op_eret(cpu, mem, instruction);
        case 0x20:
            return op_wait(cpu, mem, instruction);
        default:
            return false;
    }
}

// Sets the mode, returns PC to user space
bool op_eret(CPU &cpu, Memory &, Instruction) {
    if (!cpu.c0.in_kernel()) return true; // ignore if in user mode
    cpu.c0.set_exc_level(false); // turn off exclevel
    cpu.queue_pc_update(cpu.c0.epc.read());
    return true;
}

bool op_wait(CPU &cpu, Memory &, Instruction) {
    // power off cpu until an exception/interrupt occurs
    // cpu won't execute any instructions until then
    cpu.powered = false;
    return true;
}

bool op_tlbr(CPU &cpu, Memory &, Instruction) {
    // Read index
    const s32 index = cpu.c0.index.read();

    // Get TLB entry
    TLBEntry entry = cpu.tlb.get_entry(index);

    // Write EntryHi
    cpu.c0.set_vpn2(entry.VPN);

    // Write EntryLo
    cpu.c0.set_entrylo(false, entry.EntryLo0);
    cpu.c0.set_entrylo(true, entry.EntryLo1);

    return true;
}

bool op_tlbwi(CPU &cpu, Memory &, Instruction) {
    // Get index
    const s32 index = cpu.c0.index.read();

    // Create entry
    TLBEntry entry{};

    // Get VPN
    entry.VPN = cpu.c0.get_vpn2();

    // Get physical pages
    entry.EntryLo0 = cpu.c0.get_entrylo(false);
    entry.EntryLo1 = cpu.c0.get_entrylo(true);

    entry.ASID = cpu.c0.get_asid();

    // Write entry
    cpu.tlb.set_entry(index, entry);

    return true;
}

bool op_tlbp(CPU &cpu, Memory &, Instruction) {
    // Check if entry at index is the same VPN and ASID, if so put index, otherwise put 0x80000000
    uint32_t vpn = cpu.c0.get_vpn2();
    uint32_t number = (vpn >> PAGE_OFFSET_BITS) >> 1;
    uint32_t index = cpu.tlb.get_index(vpn);
    TLBEntry entry = cpu.tlb.get_entry(index);

    if (entry.VPN == number && entry.ASID == cpu.c0.get_asid()) {
        cpu.c0.index.set(static_cast<s32>(index));
        return true;
    }

    cpu.c0.index.set(static_cast<s32>(0x80000000));
    return true;
}
