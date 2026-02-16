//
// Created by Paul Clavaud on 23/12/25.
//

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

    Coprocessor0();

    // updates bits 2-6 of the cause register
    void set_cause(ExceptionCode exception);

    // Returns bits 2-6 of the cause register
    [[nodiscard]] ExceptionCode read_cause() const;

    // THIS DOES THE ACTUAL EXCEPTION HANDLING; returns whether program should terminate
    bool handle_exception(CPU &state);
};

#endif //MIPS_COPROCESSOR0_H