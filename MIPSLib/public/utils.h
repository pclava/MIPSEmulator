#ifndef MIPS_PROC_UTILS_H
#define MIPS_PROC_UTILS_H
#include <fstream>
#include <iostream>
#include <cstdint>

// bunch of random stuff i couldn't put elsewhere

namespace MIPS {
    using Byte = uint8_t;
    using Half = uint16_t;
    using Word = uint32_t;
    using u32 = uint32_t;
    using s32 = int32_t;

    enum MODE {
        USER,
        KERNEL
    };

    enum ExceptionCode {
        INTERRUPT = 0,
        ADDRESS_ERROR_EXCEPTION_LOAD = 4,
        ADDRESS_ERROR_EXCEPTION_STORE = 5,
        SYSCALL_EXCEPTION = 8,
        BREAKPOINT_EXCEPTION = 9,
        ARITHMETIC_OVERFLOW_EXCEPTION = 12,
        TRAP_EXCEPTION = 13,
    };

    static constexpr size_t BLOCK_SIZE = 4096; // Number of bytes per memory block

    /*
     * NOTE:
     * the real MIPS stack goes from 0x10040000 to 0x7fffffff
     * the real MIPS text segment goes from 0x00400000 to 0x0fffffff
     *
     * the emulator only simulates 4 Mb of these
     */
    inline Word KTEXT_START = 0x80000000; // 1 Mb kernel text
    inline Word KTEXT_LIMIT = 0x800fffff;
    inline Word KDATA_START = 0x90000000; // 1 Mb kernel data
    inline Word KDATA_LIMIT = 0x900fffff;
    inline Word STACK_START = 0x7fc00000; // 4 Mb stack memory
    inline Word STACK_LIMIT = 0x7fffffff;
    inline Word DATA_START =  0x10000000; // 4 Mb data memory
    inline Word DATA_LIMIT =  0x103fffff;
    inline Word TEXT_START =  0x00400000; // 4 Mb text memory
    inline Word TEXT_LIMIT =  0x007fffff;

    inline Word EXC_VECTOR = 0x80000180;  // Start of exception handler
    inline Word HEAP_START =  0x10080000; // starting heap pointer (heap can go below, down to DATA_START)

    struct Register;
    struct RegisterFile;

    struct Memory;
    struct MemorySegment;
    struct MemoryBlock;

    struct CPU;
    struct Coprocessor0;

    struct Instruction;
    using OPHandler = bool (*)(CPU&, Memory&, Instruction);

    struct System;

    inline Byte read_byte(std::ifstream& file) {
        Byte b;
        file.read( reinterpret_cast<char *>(&b), sizeof(b) );
        if (file.gcount() != sizeof(Byte)) throw std::runtime_error("error reading from file");
        return b;
    }
}

#endif //MIPS_PROC_UTILS_H