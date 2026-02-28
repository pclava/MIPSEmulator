#ifndef MIPS_PROC_UTILS_H
#define MIPS_PROC_UTILS_H
#include <fstream>
#include <iostream>

// bunch of randum stuff i couldn't put elsewhere

namespace MIPS {
    using Byte = unsigned char;
    using Half = unsigned short;
    using Word = unsigned int;
    using u32 = unsigned int;
    using s32 = signed int;

    enum ExceptionCode {
        INTERRUPT = 0,
        ADDRESS_ERROR_EXCEPTION_LOAD = 4,
        ADDRESS_ERROR_EXCEPTION_STORE = 5,
        SYSCALL_EXCEPTION = 8,
        ARITHMETIC_OVERFLOW_EXCEPTION = 12,
    };

    struct FileHeader {
        Word text_size;
        Word data_size;
        Word program_entry;
    };

    static constexpr size_t BLOCK_SIZE = 4096; // Number of bytes per memory block

    inline Word STACK_START = 0x7ff00000; // 1 MiB stack memory
    inline Word STACK_LIMIT = 0x7fffffff;
    inline Word HEAP_START =  0x10080000; // 1 MiB heap memory
    inline Word HEAP_LIMIT =  0x1017ffff;
    inline Word DATA_START =  0x10000000; // 1 MiB data memory
    inline Word DATA_LIMIT =  0x100fffff;
    inline Word TEXT_START =  0x00400000; // 1 MiB text memory
    inline Word TEXT_LIMIT =  0x004fffff;

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