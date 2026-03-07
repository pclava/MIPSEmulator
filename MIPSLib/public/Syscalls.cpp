#include "Syscalls.h"
#include "Processor.h"
#include "Memory.h"
#include <unistd.h>
#include <limits>
#include <iostream>

#define V0 cpu.RF[2]
#define A0 cpu.RF[4]
#define A1 cpu.RF[5]
#define A2 cpu.RF[6]

bool do_syscall(CPU &cpu, Memory &mem) {
    switch (V0) { // Read v0 for operation
        case SYSCALL_PRINT_INT:
            return sys_print_int(cpu, mem);
        case SYSCALL_PRINT_FLOAT:
            return sys_print_float(cpu, mem);
        case SYSCALL_PRINT_DOUBLE:
            return sys_print_double(cpu, mem);
        case SYSCALL_PRINT_STRING:
            return sys_print_str(cpu, mem);
        case SYSCALL_READ_INT:
            return sys_read_int(cpu, mem);
        case SYSCALL_READ_FLOAT:
            return sys_read_float(cpu, mem);
        case SYSCALL_READ_DOUBLE:
            return sys_read_double(cpu, mem);
        case SYSCALL_READ_STRING:
            return sys_read_str(cpu, mem);
        case SYSCALL_SBRK:
            return sys_sbrk(cpu, mem);
        case SYSCALL_EXIT:
            return sys_exit(cpu, mem);
        case SYSCALL_PRINT_CHAR:
            return sys_print_char(cpu, mem);
        case SYSCALL_READ_CHAR:
            return sys_read_char(cpu, mem);
        case SYSCALL_OPEN_FILE:
            return sys_open_file(cpu, mem);
        case SYSCALL_READ_FILE:
            return sys_read_file(cpu, mem);
        case SYSCALL_WRITE_FILE:
            return sys_write_file(cpu, mem);
        case SYSCALL_CLOSE_FILE:
            return sys_close_file(cpu, mem);
        case SYSCALL_EXIT2:
            return sys_exit2(cpu, mem);
        case SYSCALL_PRINT_HEX:
            return sys_print_hex(cpu, mem);
        case SYSCALL_PRINT_UNSIGNED:
            return sys_print_unsigned(cpu, mem);
        case SYSCALL_BRK:
            return sys_brk(cpu, mem);
        default:
            cpu.raise_exception(SYSCALL_EXCEPTION, Instruction::decode_instr(0xc));
            return false;
    }
}

// v0 = 1, a0 = value
bool sys_print_int(CPU &cpu, Memory &) {
    cpu.system.out << A0 << std::flush;
    return true;
}

bool sys_print_float(CPU &, Memory &) {return true;} // Not implemented

bool sys_print_double(CPU &, Memory &) {return true;} // Not implemented

// v0 = 4, a0 = address of NULL-TERMINATED string
bool sys_print_str(CPU &cpu, Memory &mem) {
    Word addr = static_cast<Word>(A0);
    char c;
    int i = 0;
    try {
        std::string str;
        while ((c = static_cast<char>(mem.readByte(addr+i))) != '\0') {
            str += c;
            i++;
        }
        cpu.system.out << str << std::flush;
    } catch (const std::out_of_range&) {
        cpu.c0->vaddr.set(static_cast<s32>(addr+i));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD, Instruction::decode_instr(0xc));
        return false;
    }

    return true;
}

// v0 = 5 (and integer read)
bool sys_read_int(CPU &cpu, Memory &) {
    cpu.system.in >> V0;
    // "flush" stdin until newline (flushing new line as well)
    cpu.system.in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return true;
}

bool sys_read_float(CPU &, Memory &) {return true;} // Not implemented

bool sys_read_double(CPU &, Memory &) {return true;}

// v0 = 8, a0 = address of buffer in memory, a1 = bytes to read
bool sys_read_str(CPU &cpu, Memory &mem) {
    const Word addr = static_cast<Word>(A0);
    const u32 bytes = static_cast<u32>(A1);
    u32 i = 0;
    try {
        for (i = 0; i < bytes; i++) {
            char c;
            cpu.system.in.get(c);
            if (c == '\n') {
                mem.writeByte(addr+i, 0);
                break;
            }
            mem.writeByte(addr+i, c);
        }
    } catch (const std::out_of_range&) {
        cpu.c0->vaddr.set(static_cast<s32>(addr+i));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_STORE, Instruction::decode_instr(0xc));
        return false;
    }
    return true;
}

// v0 = 9 (return address of allocated memory), a0 = number of bytes to allocate
bool sys_sbrk(CPU &cpu, Memory &mem) {
    Word ret = mem.heapAddress;
    // Move up program break
    Word bytes = static_cast<Word>(A0);
    Word new_pbrk = mem.heapAddress + bytes;
    if (new_pbrk % 4 != 0) new_pbrk += 4 - new_pbrk % 4;
    // note heapAddress points to the first unallocated byte. if the whole heap is allocated,
    // this is HEAP_LIMIT+1
    if (new_pbrk > DATA_LIMIT+1) {
        cpu.raise_exception(SYSCALL_EXCEPTION, Instruction::decode_instr(0xc));
        return false;
    }
    mem.heapAddress = new_pbrk;
    V0 = static_cast<s32>(ret);
    return true;
}

// v0 = 10
bool sys_exit(CPU &cpu, Memory &) {
    cpu.terminate(0);
    return false;
}

// v0 = 11, a0[7:0] = character
bool sys_print_char(CPU &cpu, Memory &) {
    cpu.system.out << static_cast<char>(A0) << std::flush;
    return true;
}

// v0 = 12 (return char read)
bool sys_read_char(CPU &cpu, Memory &) {
    char c;
    cpu.system.in.get(c);
    V0 = static_cast<unsigned char>(c);

    // Clear input until newline if using stdin
    if (&cpu.system.in == &std::cin) {
        while (c != '\n') cpu.system.in.get(c);
    }
    return true;
}

// v0 = 13 (return fildes), a0 = pointer to name, a1 = flag (0 read, 1 write), a2 = mode (ignored)
bool sys_open_file(CPU &cpu, Memory &mem) {
    // Get file name
    Word addr = static_cast<Word>(A0);
    std::string path;
    try {
        char c = static_cast<char>(mem.readByte(addr++));
        while (c != '\0') {
            path += c;
            c = static_cast<char>(mem.readByte(addr++));
        }
    }  catch (const std::out_of_range&) {
        cpu.c0->vaddr.set(static_cast<s32>(addr-1));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD, Instruction::decode_instr(0xc));
        return false;
    }

    Word flag = static_cast<Word>(A1);
    int ret;
    FILE *f;
    switch (flag) {
        case 0: // read
            f = fopen(path.c_str(), "rb");
            ret = fileno(f);
            break;
        case 1: // write
            f = fopen(path.c_str(), "wb");
            ret = fileno(f);
            break;
        default: // unrecognized
            ret = -1;
    }
    V0 = static_cast<s32>(ret);
    return true;
}

// v0 = 14 (return bytes read), a0 = filedes, a1 = input buffer, a2 = bytes to read
bool sys_read_file(CPU &cpu, Memory &mem) {
    s32 fd = A0;
    s32 addr = A1;
    u32 bytes = static_cast<u32>(A2);
    u32 i=0;
    try {
        for (; i < bytes; i++) {
            Byte c[1];
            ssize_t bytes_read = read(fd, c, 1);
            if (bytes_read <= 0) {
                if (bytes_read == -1) i = -1;
                break;
            }
            mem.writeByte(addr+i, c[0]);
        }
    }  catch (const std::out_of_range&) {
        cpu.c0->vaddr.set(static_cast<s32>(addr+i));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_STORE, Instruction::decode_instr(0xc));
        return false;
    }

    V0 = static_cast<s32>(i);
    return true;
}

// v0 = 15 (return bytes written), a0 = filedes, a1 = output buffer, a2 = bytes to write
bool sys_write_file(CPU &cpu, Memory &mem) {
    s32 fd = A0;
    s32 addr = A1;
    u32 bytes = static_cast<u32>(A2);
    u32 i=0;
    try {
        for (; i < bytes; i++) {
            Byte c[1] = {mem.readByte(addr+i)};
            ssize_t bytes_written = write(fd, c, 1);
            if (bytes_written <= 0) {
                i = bytes_written;
                break;
            }
        }
    }  catch (const std::out_of_range&) {
        cpu.c0->vaddr.set(static_cast<s32>(addr+i));
        cpu.raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD, Instruction::decode_instr(0xc));
        return false;
    }
    V0 = static_cast<s32>(i);
    return true;
}

// v0 = 16, a0 = filedes
bool sys_close_file(CPU &cpu, Memory &) {
    s32 filedes = static_cast<s32>(A0);
    close(filedes);
    return true;
}

// v0 = 17, a0 = code
bool sys_exit2(CPU &cpu, Memory &) {
    unsigned char code = A0;
    cpu.terminate(code);
    return false;
}

// v0 = 20, a0 = int
bool sys_print_hex(CPU &cpu, Memory &) {
    cpu.system.out << std::hex << A0 << std::dec << std::flush;
    return true;
}

// v0 = 22, a0 = number
bool sys_print_unsigned(CPU &cpu, Memory &) {
    cpu.system.out << static_cast<u32>(A0) << std::flush;
    return true;
}

// v0 = 23, a0 = address
bool sys_brk(CPU &cpu, Memory &mem) {
    Word addr = static_cast<Word>(A0);
    if (addr < DATA_START || addr > DATA_LIMIT) {
        cpu.raise_exception(SYSCALL_EXCEPTION, Instruction::decode_instr(0xc));
        return false;
    }
    mem.heapAddress = addr;
    return true;
}