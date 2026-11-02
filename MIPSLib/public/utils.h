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

    enum DisplayMode {
        D_GRAPHICAL,
        D_TEXT,
    };

    struct TextFmt {
        unsigned int foreground : 4;
        unsigned int background : 4;
    };

    struct VGAChar {
        char c;
        TextFmt fmt;
    };

    enum ExceptionCode {
        INTERRUPT = 0,
        TLB_MOD = 1,                        // Store occurs on valid entry with dirty bit set to 0. Kernel will check if writes are allowed, and is so set the dirty bit to 1
        TLBL_MISS = 2,                      // Load occurs without a valid entry
        TLBS_MISS = 3,                      // Store occurs without a valid entry
        ADDRESS_ERROR_EXCEPTION_LOAD = 4,
        ADDRESS_ERROR_EXCEPTION_STORE = 5,
        SYSCALL_EXCEPTION = 8,
        BREAKPOINT_EXCEPTION = 9,
        ARITHMETIC_OVERFLOW_EXCEPTION = 12,
        TRAP_EXCEPTION = 13,
    };

    static constexpr size_t BLOCK_SIZE = 4096; // Number of bytes per memory block and page

    // Non-mapped
    inline constexpr Word KTEXT_START = 0x80000180; // 256 Mb kernel text
    inline constexpr Word KTEXT_LIMIT = 0x8fffffff; // TODO: is this too much?
    inline constexpr Word KDATA_START = 0x90000000; // 256 Mb kernel data
    inline constexpr Word KDATA_LIMIT = 0x9fffffff; // TODO: is this too much?

    inline constexpr Word DISPLAY_START =  0xffff0000;
    inline constexpr Word DISPLAY_LIMIT =  0xfffffa03; // last word for key registers

    inline constexpr Word DISK_START = 0xfffffa04; // 2 words and 2 bytes
    inline constexpr Word DISK_LIMIT =  0xfffffa0d;

    inline constexpr Word UNMAPPED_UMEM_ALIAS_DIFF  = 0x8f000000; // how much to add/subtract by to get mapping
    inline constexpr Word UNMAPPED_UMEM_ALIAS_START = 0x8f001000; // Addresses here map 1-1 to user memory without translation
    inline constexpr Word UNMAPPED_UMEM_ALIAS_LIMIT = 0x8f3fffff;

    inline constexpr Word EXC_VEC_PTR = KDATA_START;

    // Mapped
    inline constexpr Word USER_MEM_START = 0x00001000; // 4 Mb user memory
    inline constexpr Word USER_MEM_LIMIT = 0x003fffff;
    // inline constexpr Word STACK_START = 0x7fc00000; // 4 Mb stack memory
    // inline constexpr Word STACK_LIMIT = 0x7fffffff;
    // inline constexpr Word DATA_START =  0x10000000; // 4 Mb data memory
    // inline constexpr Word DATA_LIMIT =  0x103fffff;
    // inline constexpr Word TEXT_START =  0x00400000; // 4 Mb text memory
    // inline constexpr Word TEXT_LIMIT =  0x007fffff;
    // inline constexpr Word HEAP_START =  0x10080000; // starting heap pointer (heap can go below, down to DATA_START)


    struct Register;
    struct RegisterFile;

    struct Memory;
    struct MemorySegment;
    struct MemoryBlock;

    struct CPU;
    struct Coprocessor0;

    struct TLBEntry;
    class TLB;

    class Device;
    class Display;
    class Disk;

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