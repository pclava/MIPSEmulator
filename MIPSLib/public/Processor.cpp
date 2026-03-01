#include "utils.h"
#include "Processor.h"
#include <ranges>

using namespace MIPS;

CPU::CPU(Coprocessor0 *coproc, std::istream& input, std::ostream& output) :
    PC(32, 0),
    HI(33, 0),
    LO(34, 0),
    system(input, output)
{
    c0 = coproc;
    init_opcode_table(opcode_table, funct_table, spec2_table);
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
        raise_exception(ADDRESS_ERROR_EXCEPTION_LOAD);
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

void CPU::raise_exception(const ExceptionCode exception) {
    c0->set_cause(exception); // set the cause register
    c0->epc.set(PC.read()); // set epc to instruction that caused exception
    c0->status.set(c0->status.read() | 0b10); // set status bit 1
    if (c0->handle_exception(*this) == 0) terminate(255);
}

// Returns program entry
void CPU::load_executable(const std::string& path, const int argc, char **argv, Memory &mem) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Could not open file");

    // Read header
    FileHeader header{};
    file.read( reinterpret_cast<char *>(&header), sizeof(header));
    if (file.gcount() != sizeof(FileHeader)) throw std::runtime_error("error reading from file");
    set_pc_entry(header.program_entry);

    // Load text segment
    for (Word i = 0; i < header.text_size; i++) {
        mem.writeByte(TEXT_START+i, read_byte( file));
    }

    mem.text_length = header.text_size;

    // Load text segment
    for (Word i = 0; i < header.data_size; i++) {
        mem.writeByte(DATA_START+0x10000+i, read_byte( file)); // start writing at 0x10010000
    }

    /* STACK ORGANIZATION
     * [ARGC][ARGV[0]][ARGV[1]]...[ARGV[ARGC-1]][ARGV[0]...]...[ARGV[ARGC-1]...\0] |
     * ^     ^                    ^             ^                               ^
     * |     |                    |             |                               |
     * $sp $sp+4            $sp+4+4(ARGC-1) $sp+4+4(ARGC)                  0x7fffffff
     *
     * starting $sp = 0x7fffffff - size of above + 1
     */

    // Load arguments to stack
    u32 num_bytes = 4 + 4*argc; // one word for argc, one for each argv
    for (int i = 0; i < argc; i++) {
        num_bytes += strlen(argv[i])+1; // Get total number of bytes for every string
    }
    // Get stack pointer on word boundary
    while (num_bytes % 4 != 0) {
        num_bytes++;
    }
    const u32 sp = STACK_LIMIT-num_bytes+1;
    RF[29] = static_cast<s32>(sp);
    mem.writeWord(sp, argc);

    // Write strings to memory
    u32 write_addr = sp + 4 + 4*argc;
    for (int i = 0; i < argc; i++) {
        char *str = argv[i];
        // Save string address
        mem.writeWord(sp+4+4*i, write_addr);
        // Write strings
        for (u32 j = 0; j < strlen(str)+1; j++) {
            // Write byte
            char c = str[j];
            mem.writeByte(write_addr, c);
            // Increment write_addr
            write_addr++;
        }
    }

    file.close();
}