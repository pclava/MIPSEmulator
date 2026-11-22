#include "Memory.h"

#include "Coprocessor0.h"
#include "tlb.h"
#include "utils.h"
#include "Display.h"
#include "Device.h"
#include "Disk.h"

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

#define blockIndex(rel_addr) (rel_addr / BLOCK_SIZE)
#define blockOffset(rel_addr) (rel_addr % BLOCK_SIZE)

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
    // const Word rel_addr = relAddr(addr);
    // auto& block = getBlock(rel_addr);
    for (Word i = 0; i < num_bytes; i++) {
        const Word rel_addr = relAddr(addr+i);
        auto& block = getBlock(rel_addr);
        block[blockOffset(rel_addr)] = buf[i];
    }
}

Word MemorySegment::readWord(const Word addr) {
    // Note: little endian implementation
    Word word = 0;
    const Word rel_addr = relAddr(addr);
    auto& block = getBlock(rel_addr);
    const Word blockoffset = blockOffset(rel_addr);
    word |= block[blockoffset];
    word |= block[blockoffset+1] << 8;
    word |= block[blockoffset+2] << 16;
    word |= block[blockoffset+3] << 24;
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

Memory::Memory(Coprocessor0 &c0, TLB &tlb, Display &display, Disk &disk) :
    umem(USER_MEM_START, USER_MEM_LIMIT),
    ktext(KTEXT_START, KTEXT_LIMIT),
    kdata(KDATA_START, KDATA_LIMIT),
    c0(c0),
    tlb(tlb),
    display(display),
    disk(disk),
    heapAddress(0) {}

bool Memory::full_access() const {
    return c0.in_kernel();
}

Byte Memory::readByte(Word addr) {
    // If in translated memory, check TLB, otherwise use unmapped address
    if (addr < 0x80000000) {
        // Look up address and get physical frame number. lookup raises TLBMiss so we don't have to
        Address a = TLB::split_address(addr);
        TLBEntry e = tlb.lookup(a.pagenumber);
        TLBPhysicalPage p{};
        if ((a.pagenumber >> (PAGE_OFFSET_BITS)) % 2 == 0)
            p = e.EntryLo0;
        else p = e.EntryLo1;

        a.pagenumber = p.PFN;

        return umem.readByte(TLB::combine_address(a));
    }

    // Map back to user memory, and check
    if (addr >= UNMAPPED_UMEM_ALIAS_START && addr <= UNMAPPED_UMEM_ALIAS_LIMIT)
        addr -= UNMAPPED_UMEM_ALIAS_DIFF;

    // Note that this condition would always be false if we didn't map aliased memory
    if (umem.inSegment(addr)) {
        return umem.readByte(addr);
    }

    if (ktext.inSegment(addr) && full_access()) {
        return ktext.readByte(addr);
    }
    if (kdata.inSegment(addr) && full_access()) {
        return kdata.readByte(addr);
    }

    // MMIO
    if (display.inMemory(addr)) {
        return display.query(addr, 1) & 0xFF;
    }
    if (disk.inMemory(addr)) {
        return disk.query(addr, 1) & 0xFF;
    }

    throw std::out_of_range("address out of range\n");
}

void Memory::readN(Word addr, const Word num_bytes, Byte *buf) {

    // If in user memory, check TLB, otherwise return unmapped address
    if (addr < 0x80000000) {
        // Look up address and get physical frame number. lookup raises TLBMiss so we don't have to
        Address a = TLB::split_address(addr);
        TLBEntry e = tlb.lookup(a.pagenumber);
        TLBPhysicalPage p{};
        if ((a.pagenumber >> (PAGE_OFFSET_BITS)) % 2 == 0)
            p = e.EntryLo0;
        else p = e.EntryLo1;

        a.pagenumber = p.PFN;

        return umem.readN(TLB::combine_address(a), num_bytes, buf);
    }

    // Map back to user memory, and check
    if (addr >= UNMAPPED_UMEM_ALIAS_START && addr <= UNMAPPED_UMEM_ALIAS_LIMIT)
        addr -= UNMAPPED_UMEM_ALIAS_DIFF;
    // Note that this condition would always be false if we didn't map aliased memory
    if (umem.inSegment(addr)) {
        return umem.readN(addr, num_bytes, buf);
    }

    if (ktext.inSegment(addr) && full_access()) {
        return ktext.readN(addr, num_bytes, buf);
    }
    if (kdata.inSegment(addr) && full_access()) {
        return kdata.readN(addr, num_bytes, buf);
    }

    // MMIO
    if (display.inMemory(addr) || disk.inMemory(addr)) {
        throw std::out_of_range("can't readN from device\n");
    }

    throw std::out_of_range("address out of range\n");
}

// DOES NOT CHECK WORD BOUNDARY
Word Memory::readWord(Word addr) {
    // If in user memory, check TLB, otherwise return unmapped address
    if (addr < 0x80000000) {
        // Look up address and get physical frame number. lookup raises TLBMiss so we don't have to
        Address a = TLB::split_address(addr);
        TLBEntry e = tlb.lookup(a.pagenumber);
        TLBPhysicalPage p{};
        if ((a.pagenumber >> (PAGE_OFFSET_BITS)) % 2 == 0)
            p = e.EntryLo0;
        else p = e.EntryLo1;

        a.pagenumber = p.PFN;

        return umem.readWord(TLB::combine_address(a));
    }

    // Map back to user memory, and check
    if (addr >= UNMAPPED_UMEM_ALIAS_START && addr <= UNMAPPED_UMEM_ALIAS_LIMIT)
        addr -= UNMAPPED_UMEM_ALIAS_DIFF;
    // Note that this condition would always be false if we didn't map aliased memory
    if (umem.inSegment(addr)) {
        return umem.readWord(addr);
    }

    if (ktext.inSegment(addr) && full_access()) {
        return ktext.readWord(addr);
    }
    if (kdata.inSegment(addr) && full_access()) {
        return kdata.readWord(addr);
    }

    // MMIO
    if (display.inMemory(addr)) {
        return display.query(addr, 4);
    }
    if (disk.inMemory(addr)) {
        return disk.query(addr, 4);
    }

    throw std::out_of_range("address out of range\n");
}

void Memory::writeByte(Word addr, Byte byte) {
    // If in user memory, check TLB, otherwise return unmapped address
    if (addr < 0x80000000) {
        // Look up address and get physical frame number. lookup raises TLBMiss so we don't have to
        Address a = TLB::split_address(addr);
        TLBEntry e = tlb.lookup(a.pagenumber);
        TLBPhysicalPage p{};
        if ((a.pagenumber >> (PAGE_OFFSET_BITS)) % 2 == 0)
            p = e.EntryLo0;
        else p = e.EntryLo1;

        a.pagenumber = p.PFN;

        // Storing on clean page throws TLB_MOD exception
        if (!p.dirty) {
            throw TLBModifyException();
        }

        return umem.writeByte(TLB::combine_address(a), byte);
    }

    // Map back to user memory, and check
    if (addr >= UNMAPPED_UMEM_ALIAS_START && addr <= UNMAPPED_UMEM_ALIAS_LIMIT)
        addr -= UNMAPPED_UMEM_ALIAS_DIFF;
    // Note that this condition would always be false if we didn't map aliased memory
    if (umem.inSegment(addr)) {
        return umem.writeByte(addr, byte);
    }

    if (ktext.inSegment(addr) && full_access()) {
        return ktext.writeByte(addr, byte);
    }
    if (kdata.inSegment(addr) && full_access()) {
        return kdata.writeByte(addr, byte);
    }

    // MMIO
    if (display.inMemory(addr)) {
        return display.signal(addr, byte, 1, *this);
    }
    if (disk.inMemory(addr)) {
        return disk.signal(addr, byte, 1, *this);
    }

    throw std::out_of_range("address out of range\n");
}

void Memory::writeN(Word addr, const Word num_bytes, const Byte *buf) {
    // If in user memory, check TLB, otherwise return unmapped address
    if (addr < 0x80000000) {
        // Look up address and get physical frame number. lookup raises TLBMiss so we don't have to
        Address a = TLB::split_address(addr);
        TLBEntry e = tlb.lookup(a.pagenumber);
        TLBPhysicalPage p{};
        if ((a.pagenumber >> (PAGE_OFFSET_BITS)) % 2 == 0)
            p = e.EntryLo0;
        else p = e.EntryLo1;

        a.pagenumber = p.PFN;

        // Storing on clean page throws TLB_MOD exception
        if (!p.dirty) {
            throw TLBModifyException();
        }

        return umem.writeN(TLB::combine_address(a), num_bytes, buf);
    }

    // Map back to user memory, and check
    if (addr >= UNMAPPED_UMEM_ALIAS_START && addr <= UNMAPPED_UMEM_ALIAS_LIMIT)
        addr -= UNMAPPED_UMEM_ALIAS_DIFF;
    // Note that this condition would always be false if we didn't map aliased memory
    if (umem.inSegment(addr)) {
        return umem.writeN(addr, num_bytes, buf);
    }

    if (ktext.inSegment(addr) && full_access()) {
        return ktext.writeN(addr, num_bytes, buf);
    }
    if (kdata.inSegment(addr) && full_access()) {
        return kdata.writeN(addr, num_bytes, buf);
    }

    // MMIO
    if (display.inMemory(addr) ||  disk.inMemory(addr)) {
        throw std::out_of_range("can't writeN to device\n");
    }

    throw std::out_of_range("address out of range\n");
}

void Memory::writeWord(Word addr, Word word) {
    // If in user memory, check TLB, otherwise return unmapped address
    if (addr < 0x80000000) {
        // Look up address and get physical frame number. lookup raises TLBMiss so we don't have to
        Address a = TLB::split_address(addr);
        TLBEntry e = tlb.lookup(a.pagenumber);
        TLBPhysicalPage p{};
        if ((a.pagenumber >> (PAGE_OFFSET_BITS)) % 2 == 0)
            p = e.EntryLo0;
        else p = e.EntryLo1;

        a.pagenumber = p.PFN;

        // Storing on clean page throws TLB_MOD exception
        if (!p.dirty) {
            throw TLBModifyException();
        }

        return umem.writeWord(TLB::combine_address(a), word);
    }

    // Map back to user memory, and check
    if (addr >= UNMAPPED_UMEM_ALIAS_START && addr <= UNMAPPED_UMEM_ALIAS_LIMIT)
        addr -= UNMAPPED_UMEM_ALIAS_DIFF;
    // Note that this condition would always be false if we didn't map aliased memory
    if (umem.inSegment(addr)) {
        return umem.writeWord(addr, word);
    }

    if (ktext.inSegment(addr) && full_access()) {
        return ktext.writeWord(addr, word);
    }
    if (kdata.inSegment(addr) && full_access()) {
        return kdata.writeWord(addr, word);
    }

    // MMIO
    if (display.inMemory(addr)) {
        return display.signal(addr, word, 4, *this);
    }
    if (disk.inMemory(addr)) {
        return disk.signal(addr, word, 4, *this);
    }

    throw std::out_of_range("address out of range\n");
}

void Memory::reset() const {
    umem.reset();
    kdata.reset();
    ktext.reset();
}