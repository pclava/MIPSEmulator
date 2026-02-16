//
// Created by Paul Clavaud on 17/12/25.
//

#ifndef MIPS_PROC_REGISTER_H
#define MIPS_PROC_REGISTER_H

#include "utils.h"

struct MIPS::Register {
    s32 value;
    s32 reset_value;
    unsigned int number;

    Register(unsigned int number, s32 reset_value);

    void reset();

    void set(s32 v);

    [[nodiscard]] s32 read() const;
};

struct MIPS::RegisterFile {
    Register registers[32] = {
        Register(0, 0),  // $zero
        Register(1, 0),              // $at
        Register(2, 0),              // $v0
        Register(3, 0),              // $v1
        Register(4, 0),              // $a0
        Register(5, 0),              // $a1
        Register(6, 0),              // $a2
        Register(7, 0),              // $a3
        Register(8, 0),              // $t0
        Register(9, 0),              // $t1
        Register(10, 0),             // $t2
        Register(11, 0),             // $t3
        Register(12, 0),             // $t4
        Register(13, 0),             // $t5
        Register(14, 0),             // $t6
        Register(15, 0),             // $t7
        Register(16, 0),             // $s0
        Register(17, 0),             // $s1
        Register(18, 0),             // $s2
        Register(19, 0),             // $s3
        Register(20, 0),             // $s4
        Register(21, 0),             // $s5
        Register(22, 0),             // $s6
        Register(23, 0),             // $s7
        Register(24, 0),             // $t8
        Register(25, 0),             // $t9
        Register(26, 0),             // $k0
        Register(27, 0),             // $k1
        Register(28, static_cast<s32>(HEAP_START)),  // $gp
        Register(29, static_cast<s32>(STACK_LIMIT)), // $sp
        Register(30, static_cast<s32>(STACK_LIMIT)), // $fp
        Register(31, 0),             // $ra
    };

    void reset();

    void write(unsigned int index, s32 v);

    [[nodiscard]] s32 read(unsigned int index) const;

    s32 operator[](unsigned int index) const;

    s32& operator[](unsigned int index);

};

#endif //MIPS_PROC_REGISTER_H