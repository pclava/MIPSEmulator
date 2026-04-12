#ifndef MIPS_COPROCESSOR0_H
#define MIPS_COPROCESSOR0_H

#include "Processor.h"
#include "Register.h"
#include "utils.h"

enum InterruptCode {
    NO_INTERRUPT =      0,
    DISPLAY_REFRESH =   0x400,  // IP2
    KEY_DOWN =          0x800,  // IP3
    KEY_UP =            0x1000, // IP4
};

struct MIPS::Coprocessor0 {
    Register vaddr;
    Register status;
    Register cause;
    Register epc;
    Register bad; // NOT PART OF MIPS: added by me to make debugging easier. holds machine code that raised exception

    bool has_handler = false;

    Coprocessor0();

    // updates bits 2-6 of the cause register
    void set_cause(ExceptionCode exception);

    // Returns bits 2-6 of the cause register
    [[nodiscard]] ExceptionCode read_cause() const;

    bool raise_exception(CPU &state, ExceptionCode exception, Instruction instr);

    // Default exception handler used if program does not provide one
    [[nodiscard]] bool handle_exception() const;

    Register &get_register(int num);

    // Returns the value of Status bit 1
    [[nodiscard]] MODE get_mode() const;

    // Sets the new mode, returns old mode
    MODE set_mode(MODE mode);

    // Sets value of Status bit 0, returns old value
    bool set_interrupts_enabled(bool value);

    // Returns value of Status bit 0
    [[nodiscard]] bool get_interrupts_enabled() const;

    void clear_interrupts();

    void set_interrupt(InterruptCode interrupt);

    [[nodiscard]] InterruptCode get_interrupt() const;

};

#endif //MIPS_COPROCESSOR0_H