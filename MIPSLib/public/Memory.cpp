#include "Memory.h"
#include "utils.h"

using namespace MIPS;


// MEMORY BLOCK

#define TABLE_SIZE(s, l) ((l-s)/BLOCK_SIZE + ((l-s) % BLOCK_SIZE != 0))  // takes ceiling of difference divided by block size

MemoryBlock::MemoryBlock() : data(BLOCK_SIZE, 0) {}

Byte MemoryBlock::operator[](const Word offset) const {
    return data[offset];
}

Byte & MemoryBlock::operator[](const Word offset) {
    return data[offset];
}

void MemoryBlock::reset() {
    std::ranges::fill(data, 0);
}

// MEMORY SEGMENT

MemorySegment::MemorySegment(const Word base_address, const Word limit_address):
    base_address(base_address),
    limit_address(limit_address),
    table(TABLE_SIZE(base_address, limit_address+1)) {}

bool MemorySegment::inSegment(const Word addr) const {
    return addr >= base_address && addr <= limit_address;
}

Word MemorySegment::relAddr(const Word addr) const {
    return addr - base_address;
}

Word MemorySegment::blockIndex(const Word rel_addr) {
    return rel_addr / BLOCK_SIZE;
}

Word MemorySegment::blockOffset(const Word rel_addr) {
    return rel_addr % BLOCK_SIZE;
}

MemoryBlock& MemorySegment::getBlock(const Word rel_addr) {
    const Word index = blockIndex(rel_addr);
    // Check if block is allocated
    if (!table[index]) {
        table[index] = std::make_unique<MemoryBlock>();
    }
    // Return block
    return *table[index];
}

Byte MemorySegment::readByte(const Word addr) {
    const Word rel_addr = relAddr(addr);
    auto& block = getBlock(rel_addr);
    return block[blockOffset(rel_addr)];
}

void MemorySegment::writeByte(const Word addr, const Byte data) {
    this->operator[](addr) = data;
}

void MemorySegment::readN(const Word addr, const Word num_bytes, Byte *buf) {
    const Word rel_addr = relAddr(addr);
    auto& block = getBlock(rel_addr);
    for (Word i = 0; i < num_bytes; i++) {
        const Word new_addr = blockOffset(rel_addr) + i;
        buf[i] = static_cast<char>(block[new_addr]);
    }
}

void MemorySegment::writeN(const Word addr, const Word num_bytes, const Byte *buf) {
    const Word rel_addr = relAddr(addr);
    auto& block = getBlock(rel_addr);
    for (Word i = 0; i < num_bytes; i++) {
        block[blockOffset(rel_addr)+i] = buf[i];
    }
}

Word MemorySegment::readWord(const Word addr) {
    // Note: little endian implementation
    Word word = 0;
    word |= readByte(addr);
    word |= readByte(addr+1) << 8;
    word |= readByte(addr+2) << 16;
    word |= readByte(addr+3) << 24;
    return word;
}

void MemorySegment::writeWord(Word addr, Word data) {
    writeByte(addr+3, data >> 24 & 0xff);
    writeByte(addr+2, data >> 16 & 0xff);
    writeByte(addr+1, data >> 8 & 0xff);
    writeByte(addr, data & 0xff);
}

Byte MemorySegment::operator[](const Word addr) const {
    const Word rel_addr = relAddr(addr);
    Word index = blockIndex(addr);
    if (!table[index]) throw std::out_of_range("memory not initialized. use readByte()\n");
    MemoryBlock& block = *table[index];
    return block[blockOffset(rel_addr)];
}

Byte& MemorySegment::operator[](const Word addr) {
    const Word rel_addr = relAddr(addr);
    auto& block = getBlock(rel_addr);
    return block[blockOffset(rel_addr)];
}

void MemorySegment::reset() const {
    for (const auto & i : table) {
        if (i) i->reset();
    }
}

// MEMORY

Memory::Memory() :
    stack(STACK_START, STACK_LIMIT),
    data(DATA_START, DATA_LIMIT),
    text(TEXT_START, TEXT_LIMIT),
    heapAddress(HEAP_START) {}

Byte Memory::readByte(const Word addr) {
    if (stack.inSegment(addr)) {
        return stack.readByte(addr);
    }
    if (text.inSegment(addr)) {
        return text.readByte(addr);
    }
    if (data.inSegment(addr)) {
        return data.readByte(addr);
    }
    throw std::out_of_range("address out of range\n");
}

void Memory::readN(const Word addr, const Word num_bytes, Byte *buf) {
    if (stack.inSegment(addr)) {
        stack.readN(addr, num_bytes, buf);
        return;
    }
    if (text.inSegment(addr)) {
        text.readN(addr, num_bytes, buf);
        return;
    }
    if (data.inSegment(addr)) {
        data.readN(addr, num_bytes, buf);
        return;
    }
    throw std::out_of_range("address out of range\n");
}

// DOES NOT CHECK WORD BOUNDARY
Word Memory::readWord(const Word addr) {
    if (stack.inSegment(addr)) {
        return stack.readWord(addr);
    }
    if (text.inSegment(addr)) {
        return text.readWord(addr);
    }
    if (data.inSegment(addr)) {
        return data.readWord(addr);
    }
    throw std::out_of_range("address out of range\n");
}

void Memory::writeByte(const Word addr, Byte byte) {
    if (stack.inSegment(addr)) {
        stack.writeByte(addr, byte);
        return;
    }
    if (text.inSegment(addr)) {
        text.writeByte(addr, byte);
        return;
    }
    if (data.inSegment(addr)) {
        data.writeByte(addr, byte);
        return;
    }
    throw std::out_of_range("address out of range\n");
}

void Memory::writeN(const Word addr, const Word num_bytes, const Byte *buf) {
    if (stack.inSegment(addr)) {
        stack.writeN(addr, num_bytes, buf);
        return;
    }
    if (text.inSegment(addr)) {
        text.writeN(addr, num_bytes, buf);
        return;
    }
    if (data.inSegment(addr)) {
        data.writeN(addr, num_bytes, buf);
        return;
    }
    throw std::out_of_range("address out of range\n");
}

void Memory::writeWord(const Word addr, Word word) {
    if (stack.inSegment(addr)) {
        stack.writeWord(addr, word);
        return;
    }
    if (text.inSegment(addr)) {
        text.writeWord(addr, word);
        return;
    }
    if (data.inSegment(addr)) {
        data.writeWord(addr, word);
        return;
    }
    throw std::out_of_range("address out of range\n");
}

// Does not initialize memory. readByte() is safer
Byte Memory::operator[](const Word addr) const {
    if (stack.inSegment(addr)) {
        return stack[addr];
    }
    if (text.inSegment(addr)) {
        return text[addr];
    }
    if (data.inSegment(addr)) {
        return data[addr];
    }
    throw std::out_of_range("address out of range\n");
}

Byte& Memory::operator[](const Word addr) {
    if (stack.inSegment(addr)) {
        return stack[addr];
    }
    if (text.inSegment(addr)) {
        return text[addr];
    }
    if (data.inSegment(addr)) {
        return data[addr];
    }
    throw std::out_of_range("address out of range\n");
}

void Memory::debugText(const int size) {
    for (int i = 0; i < size; i++) {
        printf("0x%.8x\n", text.readWord(TEXT_START+4*i));
    }
}

void Memory::debugData(const int size) {
    for (int i = 0; i < size; i++) {
        Byte b = data.readByte(DATA_START+0x10000+i);
        printf("%.8x: 0x%.2x (%c) \n", DATA_START+0x10000+i, b, b);
    }
}

void Memory::debugStack(const int size) {
    for (int i = 0; i < size; i++) {
        Byte b = stack.readByte(STACK_LIMIT-i);
        printf("%.8x: 0x%.2x (%c) \n", STACK_LIMIT-i, b, b);
    }
}

void Memory::reset() const {
    stack.reset();
    text.reset();
    data.reset();
}