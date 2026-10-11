#include "Memory.h"

#include "Coprocessor0.h"
#include "utils.h"

using namespace MIPS;


// MEMORY BLOCK


MemoryBlock::MemoryBlock() : data(BLOCK_SIZE, 0) {}

// Read
Byte MemoryBlock::operator[](const Word offset) const {
    return data[offset];
}

// Write
Byte & MemoryBlock::operator[](const Word offset) {
    dirty = true;
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

Memory::Memory(Coprocessor0 &c0) :
    stack(STACK_START, STACK_LIMIT),
    udata(DATA_START, DATA_LIMIT),
    utext(TEXT_START, TEXT_LIMIT),
    ktext(KTEXT_START, KTEXT_LIMIT),
    kdata(KDATA_START, KDATA_LIMIT),
    mmio(MMIO_START, MMIO_LIMIT),
    c0(c0),
    heapAddress(HEAP_START) {}

bool Memory::full_access() const {
    return c0.in_kernel();
}

Byte Memory::readByte(const Word addr) {
    if (stack.inSegment(addr)) {
        return stack.readByte(addr);
    }
    if (utext.inSegment(addr)) {
        return utext.readByte(addr);
    }
    if (ktext.inSegment(addr) && full_access()) {
        return ktext.readByte(addr);
    }
    if (udata.inSegment(addr)) {
        return udata.readByte(addr);
    }
    if (kdata.inSegment(addr) && full_access()) {
        return kdata.readByte(addr);
    }
    if (mmio.inSegment(addr)) {
        return mmio.readByte(addr);
    }
    throw std::out_of_range("address out of range\n");
}

void Memory::readN(const Word addr, const Word num_bytes, Byte *buf) {
    if (stack.inSegment(addr)) {
        return stack.readN(addr, num_bytes, buf);
    }
    if (utext.inSegment(addr)) {
        return utext.readN(addr, num_bytes, buf);
    }
    if (ktext.inSegment(addr) && full_access()) {
        return ktext.readN(addr, num_bytes, buf);
    }
    if (udata.inSegment(addr)) {
        return udata.readN(addr, num_bytes, buf);
    }
    if (kdata.inSegment(addr) && full_access()) {
        return kdata.readN(addr, num_bytes, buf);
    }
    if (mmio.inSegment(addr)) {
        return mmio.readN(addr, num_bytes, buf);
    }
    throw std::out_of_range("address out of range\n");
}

// DOES NOT CHECK WORD BOUNDARY
Word Memory::readWord(const Word addr) {
    if (stack.inSegment(addr)) {
        return stack.readWord(addr);
    }
    if (utext.inSegment(addr)) {
        return utext.readWord(addr);
    }
    if (ktext.inSegment(addr) && full_access()) {
        return ktext.readWord(addr);
    }
    if (udata.inSegment(addr)) {
        return udata.readWord(addr);
    }
    if (kdata.inSegment(addr) && full_access()) {
        return kdata.readWord(addr);
    }
    if (mmio.inSegment(addr)) {
        return mmio.readWord(addr);
    }
    throw std::out_of_range("address out of range\n");
}

void Memory::writeByte(const Word addr, Byte byte) {
    if (stack.inSegment(addr)) {
        stack.writeByte(addr, byte);
        return;
    }
    if (utext.inSegment(addr)) {
        utext.writeByte(addr, byte);
        return;
    }
    if (ktext.inSegment(addr) && full_access()) {
        ktext.writeByte(addr, byte);
        return;
    }
    if (udata.inSegment(addr)) {
        udata.writeByte(addr, byte);
        return;
    }
    if (kdata.inSegment(addr) && full_access()) {
        kdata.writeByte(addr, byte);
        return;
    }
    if (mmio.inSegment(addr)) {
        mmio.writeByte(addr, byte);
        return;
    }
    throw std::out_of_range("address out of range\n");
}

void Memory::writeN(const Word addr, const Word num_bytes, const Byte *buf) {
    if (stack.inSegment(addr)) {
        stack.writeN(addr, num_bytes, buf);
        return;
    }
    if (utext.inSegment(addr)) {
        utext.writeN(addr, num_bytes, buf);
        return;
    }
    if (ktext.inSegment(addr) && full_access()) {
        ktext.writeN(addr, num_bytes, buf);
        return;
    }
    if (udata.inSegment(addr)) {
        udata.writeN(addr, num_bytes, buf);
        return;
    }
    if (kdata.inSegment(addr) && full_access()) {
        kdata.writeN(addr, num_bytes, buf);
        return;
    }
    if (mmio.inSegment(addr)) {
        mmio.writeN(addr, num_bytes, buf);
        return;
    }
    throw std::out_of_range("address out of range\n");
}

void Memory::writeWord(const Word addr, Word word) {
    if (stack.inSegment(addr)) {
        stack.writeWord(addr, word);
        return;
    }
    if (utext.inSegment(addr)) {
        utext.writeWord(addr, word);
        return;
    }
    if (ktext.inSegment(addr) && full_access()) {
        ktext.writeWord(addr, word);
        return;
    }
    if (udata.inSegment(addr)) {
        udata.writeWord(addr, word);
        return;
    }
    if (kdata.inSegment(addr) && full_access()) {
        kdata.writeWord(addr, word);
        return;
    }
    if (mmio.inSegment(addr)) {
        mmio.writeWord(addr, word);
        return;
    }
    throw std::out_of_range("address out of range\n");
}

void Memory::debugText(const int size) {
    for (int i = 0; i < size; i++) {
        printf("0x%.8x\n", utext.readWord(TEXT_START+4*i));
    }
}

void Memory::debugData(const int size) {
    for (int i = 0; i < size; i++) {
        Byte b = udata.readByte(DATA_START+0x10000+i);
        printf("%.8x: 0x%.2x (%c) \n", DATA_START+0x10000+i, b, b);
    }
}

void Memory::debugKdata(const int size) {
    for (int i = 0; i < size; i++) {
        Byte b = kdata.readByte(KDATA_START+i);
        printf("%.8x: 0x%.2x (%c) \n", KDATA_START+i, b, b);
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
    utext.reset();
    udata.reset();
    kdata.reset();
    ktext.reset();
}