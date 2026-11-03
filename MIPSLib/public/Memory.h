#ifndef MIPS_PROC_MEMORY_H
#define MIPS_PROC_MEMORY_H

#include "utils.h"
#include <vector>
#include <memory>

/*
* Memory is divided into disjoint MemorySegments.
* Each MemorySegment is an array of pointers to MemoryBlocks
* A MemoryBlock, when initialized, is an array of 4096 Bytes
* The emulator only initializes a MemoryBlock when an address
* inside it is referenced, in order to be efficient with space.
* A 4 Mb MemorySegment has 1024 blocks. Therefore, a program
* that does not use any emulated memory will only
* use up enough memory to allocate the arrays of segment pointers,
* and enough 4096-byte chunks to store the program's contents
* in the text segment.
*/

#define TABLE_SIZE(s, l) ((l-s)/BLOCK_SIZE + ((l-s) % BLOCK_SIZE != 0))  // takes ceiling of difference divided by block size

// Note: does not perform safety checks
struct MIPS::MemoryBlock {
    std::vector<Byte> data;
    bool dirty = false;

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
    MemorySegment umem;
    MemorySegment ktext;
    MemorySegment kdata;

    // Coprocessor 0 to check mode
    Coprocessor0 &c0;

    // TLB for tlbing
    TLB &tlb;

    // Devices
    Display &display;
    Disk &disk;

    Word heapAddress; // address of first unallocated byte in heap TODO: delete

    explicit Memory(Coprocessor0 &c0, TLB &tlb, Display &display, Disk &disk);

    Byte readByte(Word addr);

    void readN(Word addr, Word num_bytes, Byte *buf);

    Word readWord(Word addr);



    void writeByte(Word addr, Byte byte);

    void writeN(Word addr, Word num_bytes, const Byte *buf);

    void writeWord(Word addr, Word word);

    // Whether kernel memory is accessible
    [[nodiscard]] bool full_access() const;

    void reset() const;
};

#endif //MIPS_PROC_MEMORY_H