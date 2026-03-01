#ifndef MIPS_PROC_MEMORY_H
#define MIPS_PROC_MEMORY_H

#include "utils.h"
#include <vector>
#include <memory>

// Note: does not perform safety checks
struct MIPS::MemoryBlock {
    std::vector<Byte> data;

    MemoryBlock();

    Byte operator[](Word offset) const;

    Byte& operator[](Word offset);

    void reset();
};

// Note: does not perform safety checks
struct MIPS::MemorySegment {
    Word base_address;  // First byte in segment
    Word limit_address; // Last byte in segment (important: address space = limit+1-base)
    std::vector<std::unique_ptr<MemoryBlock>> table;

    MemorySegment(Word base_address, Word limit_address);

    // Returns whether address refers to location in segment
    [[nodiscard]] bool inSegment(Word addr) const;

    // Returns address relative to start of segment (does not check if in segment)
    [[nodiscard]] Word relAddr(Word addr) const;

    // Given a relative address, return its block index
    static Word blockIndex(Word rel_addr);

    // Given a relative address, return its location in a block
    static Word blockOffset(Word rel_addr);

    // Does not check range
    MemoryBlock& getBlock(Word rel_addr);

    // Returns value at absolute address; does not check range
    Byte readByte(Word addr);

    // Writes num_bytes many bytes from memory into buf
    void readN(Word addr, Word num_bytes, Byte *buf);

    // Writes num_bytes bytes from buf into memory
    void writeN(Word addr, Word num_bytes, const Byte *buf);

    // Writes value to absolute address
    void writeByte(Word addr, Byte data);

    // Reads four bytes from absolute address; does not check range or word boundary
    Word readWord(Word addr);

    // Writes four bytes to absolute address; does not check range or word boundary
    void writeWord(Word addr, Word data);

    // Reads from memory but does not initialize any, unlike readByte; does not check range
    Byte operator[](Word addr) const;

    Byte& operator[](Word addr);

    void reset() const;

};

struct MIPS::Memory {
    MemorySegment stack;
    MemorySegment heap;
    MemorySegment data;
    MemorySegment text;

    // Keep track of program length to avoid undefined behavior when program does not
    // exit on its own
    Word text_length;

    Memory();

    Byte readByte(Word addr);

    void readN(Word addr, Word num_bytes, Byte *buf);

    Word readWord(Word addr);

    void writeByte(Word addr, Byte byte);

    void writeN(Word addr, Word num_bytes, const Byte *buf);

    void writeWord(Word addr, Word word);

    // Does not initialize memory. readByte() is safer
    Byte operator[](Word addr) const;

    Byte& operator[](Word addr);

    void debugText(int size);

    void debugData(int size);

    void debugStack(int size);

    void reset() const;
};

#endif //MIPS_PROC_MEMORY_H