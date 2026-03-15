#include "utils.h"
#include "Processor.h"
#include "mof.h"
#include <cstring>
#include <ranges>
#include <sys/fcntl.h>

using namespace MIPS;

CPU::CPU(Coprocessor0 *coproc, std::istream& input, std::ostream& output) :
    PC(32, 0),
    HI(33, 0),
    LO(34, 0),
    system(input, output)
{
    c0 = coproc;
    init_opcode_table(opcode_table, funct_table);
}

void CPU::set_pc_entry(const Word entry) {
    PC.reset_value = static_cast<s32>(entry);
    PC.reset();
}

void CPU::update_pc() {
    PC.value = static_cast<s32>(newPC);
}

void CPU::queue_pc_update(Word addr) {
    newPC = addr;
}

Word CPU::Fetch(Memory &MEM) {
    Word ret;
    try {
        ret = MEM.readWord(PC.read());
    } catch (const std::out_of_range&) {
        c0->vaddr.set(PC.read());
        raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD, Instruction::decode_instr(0));
        return 0;
    }
    newPC = PC.read()+4;
    return ret;
}

Instruction CPU::Decode(const Word code) {
    return Instruction::decode_instr(code);
}

bool CPU::Execute(Memory &mem, const Instruction instr) {
    // THIS LINE EXECUTES THE INSTRUCTION
    bool success = opcode_table[instr.opcode](*this, mem, instr);
    RF[0] = 0; // reset 0 register
    update_pc();
    return success;
}

void CPU::terminate(unsigned char code) {
    exit = code;
    throw std::runtime_error("CPU terminated");
}

void CPU::raise_exception(const ExceptionCode exception, const Instruction instr) {
    c0->set_cause(exception); // set the cause register
    c0->epc.set(PC.read()); // set epc to instruction that caused exception
    c0->status.set(c0->status.read() | 0b10); // set status bit 1
    c0->bad.set((instr.opcode << 26) | (instr.addr)); // recover bad instruction
    if (c0->handle_exception(*this) == 0) terminate(255);
}

// Returns program entry
#ifdef _WIN32
#include "mman.h"
#else
#include <sys/mman.h>
#endif
void load_file(const char *path, mof_file &file) {
    FILE *f = fopen(path, "rb+");
    if (f == nullptr) throw std::runtime_error("Could not open file");

    // Read header
    if (mof_read_header(f, &file.hdr) == 0) {
        fclose(f);
        throw std::runtime_error("Could not read file header");
    }
    if (!mof_is_valid(&file.hdr)) {
        fclose(f);
        throw std::runtime_error("Invalid file format; expected MOF");
    }

    // Read file
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        throw std::runtime_error("Error reading file");
    }
    const long size = ftell(f); // get file size
    rewind(f);
    void *map = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fileno(f), 0);
    if (map == MAP_FAILED) {
        fclose(f);
        throw std::runtime_error("Error mapping file");
    }
    file.file = map;
    file.size = static_cast<uint32_t>(size);
    file.text = mof_text(file.file);
    file.data = mof_data(file.file, &file.hdr);
    file.relocs = mof_relocs(file.file, &file.hdr);
    file.syms = mof_symbols(file.file, &file.hdr);
    file.strings = mof_strtab(file.file, &file.hdr);

    fclose(f);
}

void load_text(Memory &mem, const mof_file &file) {
    for (u32 i = 0; i < file.hdr.text/4; i++) { // Text segment
        mem.writeWord(TEXT_START+4*i, file.text[i]);
    }
}

void load_data(Memory &mem, const mof_file &file) {
    for (u32 i = 0; i < file.hdr.data; i++) { // Data segment
        mem.writeByte(DATA_START+0x00010000+i, file.data[i]); // begin writing at 0x10010000
    }
}

// Returns sp
s32 load_stack(Memory &mem, const int argc, char **argv) {
    /* STACK ORGANIZATION
     * [ARGC][ARGV[0]][ARGV[1]]...[ARGV[ARGC-1]][ARGV[0]...]...[ARGV[ARGC-1]...\0] |
     * ^     ^                    ^             ^                               ^
     * |     |                    |             |                               |
     * $sp $sp+4            $sp+4+4(ARGC-1) $sp+4+4(ARGC)                  0x7fffffff
     *
     * starting $sp = 0x7fffffff - size of above + 1
     */

    u32 num_bytes = 4+4*argc; // one word for argc and each argv
    for (int i = 0; i < argc; i++) {
        num_bytes += strlen(argv[i])+1; // count size of each argument
    }
    while (num_bytes % 4 != 0) {
        num_bytes += 1;
    }
    const u32 sp = STACK_LIMIT-num_bytes+1;

    mem.writeWord(sp, argc); // Write argc

    // Write strings
    u32 write_addr = sp + 4 + 4*argc; // pointer to start of current argument
    for (int i = 0; i < argc; i++) {
        char *str = argv[i];
        // Save pointer
        mem.writeWord(sp+4 + 4*i, write_addr);
        // Write string
        for (u32 j = 0; j < strlen(str)+1; j++) {
            char c = str[j];
            mem.writeByte(write_addr, c);   // write character
            write_addr++;                   // increment address
        }
    }

    return static_cast<s32>(sp);
}

void CPU::load_executable(const char *path, const int argc, char **argv, Memory &mem) {
    mof_file file{};
    load_file(path, file);

    // Load into processor
    set_pc_entry(file.hdr.entry);
    load_text(mem, file);
    load_data(mem, file);
    const s32 sp = load_stack(mem, argc, argv);
    RF[29] = sp;

    // Close
    if (munmap(file.file, file.size) == -1) {
        throw std::runtime_error("Error unmapping file");
    }
}
