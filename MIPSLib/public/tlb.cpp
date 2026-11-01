
#include "tlb.h"

#include "Coprocessor0.h"

MIPS::TLB::TLB(Coprocessor0 *c0) : c0(c0) {
    for (auto & i : tlb) {
        i.EntryLo0.dirty = i.EntryLo1.dirty = false;
        i.EntryLo0.valid = i.EntryLo1.valid = false;
    }
}

MIPS::TLBEntry MIPS::TLB::lookup(const page_number VPN) const {
    uint32_t number = VPN >> PAGE_OFFSET_BITS;
    uint32_t VPN2 = VPN & 0xFFFFE000;    // remove bottom bit of VPN
    uint32_t index = get_index(VPN);
    TLBEntry entry = tlb[index];

    // Check if invalid or incorrect mapping
    if (VPN2 != entry.VPN || entry.ASID != c0->get_asid()) {
        throw TLBMissException();
    }
    if (number % 2 == 0 && !entry.EntryLo0.valid) throw TLBMissException();
    if (number % 2 == 1 && !entry.EntryLo1.valid) throw TLBMissException();

    return entry;
}

uint32_t MIPS::TLB::get_index(page_number VPN) const {
    // "hashing" function to determine TLB index.
    // first remove page offset bits and the last, since the TLB only cares about the vpn/2
    // add 3 * current user process
    // multiply by big number
    return  ((((VPN >> PAGE_OFFSET_BITS) / 2) + (3 * c0->get_asid())) * 2654435769u) % TLB_SIZE;
}

Address MIPS::TLB::split_address(uint32_t addr) {
    uint32_t mask = (1 << PAGE_OFFSET_BITS) - 1;
    Address x{};
    x.pageoffset = addr & mask;
    x.pagenumber = addr & (~mask);
    return x;
}

uint32_t MIPS::TLB::combine_address(Address addr) {
    return addr.pagenumber | addr.pageoffset;
}

MIPS::TLBEntry MIPS::TLB::get_entry(const unsigned int index) const {
    return tlb[index % TLB_SIZE];
}

void MIPS::TLB::set_entry(const unsigned int index, const TLBEntry &entry) {
    tlb[index % TLB_SIZE] = entry;
}
