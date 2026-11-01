//
// Created by Paul Clavaud on 10/10/26.
//

#ifndef MIPSEMULATOR_TLB_H
#define MIPSEMULATOR_TLB_H

#include "utils.h"

typedef uint32_t page_number;

struct TLBPhysicalPage {
    page_number PFN;
    bool dirty;
    bool valid;
};

struct MIPS::TLBEntry {
    uint8_t         ASID;       // Process number
    page_number     VPN;        // Virtual page number (divided by 2, 18 bits)
    TLBPhysicalPage EntryLo0;   // Even-numbered virtual page
    TLBPhysicalPage EntryLo1;   // Odd-numbered virtual page
};

struct Address {
    page_number     pagenumber; // Stores raw page number (not right-shifted)
    uint32_t        pageoffset;
};

// Occurs when entry is invalid or there is no matching entry
class TLBMissException final : public std::exception {
public:

    explicit TLBMissException() = default;
};

// Occurs when a store happens on non-dirty entry
class TLBModifyException final : public std::exception {
public:
    explicit TLBModifyException() = default;

};

#define TLB_SIZE 128
#define PAGE_OFFSET_BITS 12
#define PAGE_NUMBER_BITS 19

class MIPS::TLB {
    // Note: entries are indexed by their VPN % 128. I don't believe that's
    // actually how it happens and I don't care!
    TLBEntry tlb[TLB_SIZE] = {};
    Coprocessor0 *c0;
public:

    explicit TLB(Coprocessor0 *c0);

    // Returns the physical page number, or -1 on miss
    // Note: expects raw page number; that is, not shifted to the right
    [[nodiscard]] TLBEntry lookup(page_number VPN) const;

    uint32_t get_index(page_number VPN) const;

    static Address split_address(uint32_t addr);

    static uint32_t combine_address(Address addr);

    [[nodiscard]] TLBEntry get_entry(unsigned int index) const;

    void set_entry(unsigned int index, const TLBEntry &entry);
};

#endif //MIPSEMULATOR_TLB_H