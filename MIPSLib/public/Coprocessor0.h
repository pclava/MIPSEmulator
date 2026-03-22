#ifndef MIPS_COPROCESSOR0_H
#define MIPS_COPROCESSOR0_H

#include "Processor.h"
#include "Register.h"
#include "utils.h"

struct MIPS::Coprocessor0 {
    Register vaddr;
    Register status;
    Register cause;
    Register epc;
    Register bad; // NOT PART OF MIPS: added by me to make debugging easier. holds instruction that raised exception

    Coprocessor0();

    // updates bits 2-6 of the cause register
    void set_cause(ExceptionCode exception);

    // Returns bits 2-6 of the cause register
    [[nodiscard]] ExceptionCode read_cause() const;

    // THIS DOES THE ACTUAL EXCEPTION HANDLING; returns whether program should terminate
    bool handle_exception(CPU &state);

    Register &get_register(int num);
};

#endif //MIPS_COPROCESSOR0_H