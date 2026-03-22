#ifndef MIPS_PROC_PROCESSOR_H
#define MIPS_PROC_PROCESSOR_H

#include "Register.h"
#include "InstructionSet.h"
#include "utils.h"
#include "Memory.h"
#include "Coprocessor0.h"
#include "Syscalls.h"

struct MIPS::CPU {
    Register PC;
    Word newPC{}; // stores new value for PC after Execute(). Normally PC+4 but changed with queue_pc_update() by jumps and branches
    Register HI;
    Register LO;
    RegisterFile RF;

    OPHandler opcode_table[64]{};
    OPHandler funct_table[64]{};
    OPHandler cop0_table[17]{};

    Coprocessor0 &c0;
    System system; // contains file streams for input and output

    MODE mode = USER;
    unsigned char exit = 0; // exit code

    explicit CPU(Coprocessor0 &coproc, std::istream& input, std::ostream& output);

    void set_pc_entry(Word entry);

    void update_pc();

    // Sets 'newPC': at the end of Execute(), the PC is set to newPC.
    // this allows the normal incrementing of the PC and any branches/jumps to use the same function
    void queue_pc_update(Word addr);

    Word Fetch(Memory &MEM);

    static Instruction Decode(Word code);

    bool Execute(Memory &mem, Instruction instr);

    // Sets 'exit' to the code and throws a runtime error to be caught be the main program
    void terminate(unsigned char code);

    void raise_exception(ExceptionCode exception, Instruction instr);

    // Returns program entry
    void load_executable(const char* path, int argc, char **argv, Memory &mem);

    // Sets the mode and returns whether the mode was changed
    bool set_mode(MODE new_mode, Memory &mem);
};

#endif //MIPS_PROC_PROCESSOR_H