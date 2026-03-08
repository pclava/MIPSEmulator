#ifndef MIPS_SYSCALL_H
#define MIPS_SYSCALL_H

#include "utils.h"
using namespace MIPS;

/* TO ADD A SYSCALL
- add its code to the enum below
- add its function prototype below
- in syscall.cpp, add the condition to do_syscall()
- in syscall.cpp, define the function
*/

enum SyscallCode {
    // SPIM SYSCALLS
    SYSCALL_PRINT_INT = 1, // start from 1
    SYSCALL_PRINT_FLOAT, // not supported; ignored
    SYSCALL_PRINT_DOUBLE, // not supported; ignored
    SYSCALL_PRINT_STRING,
    SYSCALL_READ_INT,
    SYSCALL_READ_FLOAT, // not supported; ignored
    SYSCALL_READ_DOUBLE, // not supported; ignored
    SYSCALL_READ_STRING,
    SYSCALL_SBRK,
    SYSCALL_EXIT,
    SYSCALL_PRINT_CHAR,
    SYSCALL_READ_CHAR,
    SYSCALL_OPEN_FILE,
    SYSCALL_READ_FILE,
    SYSCALL_WRITE_FILE,
    SYSCALL_CLOSE_FILE,
    SYSCALL_EXIT2, // 17
    // END SPIM SYSCALLS

    // FOLLOWING SYSCALLS ARE MY ADDITIONS
    SYSCALL_PRINT_HEX = 20,     // 20
    SYSCALL_PRINT_BIN,          // 21
    SYSCALL_PRINT_UNSIGNED,     // 22
    SYSCALL_BRK,                // 23

};

// Contains output and input streams. CPU sets these to stdout and stdin by default
struct MIPS::System {
    std::ostream& out;
    std::istream& in;

    System(std::istream& input, std::ostream& output) : out(output), in(input) {};
};

bool do_syscall(CPU &, Memory &);

bool sys_print_int(CPU &, Memory &);
bool sys_print_float(CPU &, Memory &);
bool sys_print_double(CPU &, Memory &);
bool sys_print_str(CPU &, Memory &);
bool sys_read_int(CPU &, Memory &);
bool sys_read_float(CPU &, Memory &);
bool sys_read_double(CPU &, Memory &);
bool sys_read_str(CPU &, Memory &);
bool sys_sbrk(CPU &, Memory &);
bool sys_exit(CPU &, Memory &);
bool sys_print_char(CPU &, Memory &);
bool sys_read_char(CPU &, Memory &);
bool sys_open_file(CPU &, Memory &);
bool sys_read_file(CPU &, Memory &);
bool sys_write_file(CPU &, Memory &);
bool sys_close_file(CPU &, Memory &);
bool sys_exit2(CPU &, Memory &);
bool sys_print_hex(CPU &, Memory &);
bool sys_print_unsigned(CPU &, Memory &);
bool sys_brk(CPU &, Memory &);
#endif //MIPS_SYSCALL_H