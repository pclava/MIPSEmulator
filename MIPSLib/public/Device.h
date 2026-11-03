#ifndef MIPSEMULATOR_DEVICE_H
#define MIPSEMULATOR_DEVICE_H
#include "Memory.h"
#include "utils.h"

/* I'll write something more formal but for now
 * the idea is that to communicate with an IO device, we call signal() and query()
 * rather than actually writing to memory
 *
 * Why?
 * - More general; abstracts away the memory-interface
 * - A device that doesn't need actual memory doesn't need to read/write
 *      - caveat: for simplicity, every device gets a memory segment, which needs to be at least one block.
 *              this isn't too big of a penalty and hey now every device gets free memory, which you should
 *              not take for granted in this economy
 * - An upside is if the device doesn't need memory, we can avoid expensive writes and reads
 */

class MIPS::Device {

protected:
    MemorySegment memory;

    explicit Device(const Word base_address, const Word limit_address) : memory(base_address, limit_address) {
    }

public:
    virtual ~Device() = default;

    // Tell the device that a write has come in
    // bytes can be 1 (writeByte, val & 0xFF) or 4 (writeWord, val)
    // the mem argument is if the device needs to write to the main memory
    // such as in a disk read
    virtual void signal(Word addr, Word val, size_t bytes, Memory &mem) = 0;

    // Ask the device for data when a read comes in
    // bytes can be 1 (readByte, 0xFF) or 4 (readWord)
    [[nodiscard]] virtual Word query(Word addr, size_t bytes) = 0;

    [[nodiscard]] bool inMemory(Word addr) const {
        return memory.inSegment(addr);
    }

};

#endif //MIPSEMULATOR_DEVICE_H
