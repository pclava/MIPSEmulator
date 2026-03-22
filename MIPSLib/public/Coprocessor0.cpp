#include "utils.h"
#include "Processor.h"

Coprocessor0::Coprocessor0() : vaddr(8, 0), status(12, 0xff11), cause(13, 0), epc(14, 0), bad(0,0) {}

// updates bits 2-6 of the cause register
void Coprocessor0::set_cause(const ExceptionCode exception) {
    auto code = static_cast<unsigned char>(exception);
    cause.set(cause.read() & ~(0x1F << 2)); // clear cause
    code &= 0x1F; // extract lower 5 bits
    code <<= 2; // move up two bits
    cause.set(cause.read() | code);
}

// Returns bits 2-6 of the cause register
[[nodiscard]] ExceptionCode Coprocessor0::read_cause() const {
    return static_cast<ExceptionCode>((cause.read() & 0b1111100) >> 2);
}

bool Coprocessor0::raise_exception(CPU &state, const ExceptionCode exception, const Instruction instr) {
    // Disable interrupts
    set_interrupts(false);

    // Set cause, EPC, and bad
    set_cause(exception);
    epc.set(state.PC.read()); // set epc to instruction that caused exception (or in the case of interrupts, the instruction after_
    bad.set((instr.opcode << 26) | (instr.addr)); // recover bad instruction

    // Check if using special exception handler. If not, handle ourselves
    if (!has_handler) return handle_exception(state);

    // Go to kernel text
    state.queue_pc_update(EXC_VECTOR);
    return true;
}

// Default exception handler when the program does not have one
bool Coprocessor0::handle_exception(CPU &state) {
    state.queue_pc_update(state.PC.read()+4); // resume at next instruction
    if (bad.read() == 0) fprintf(stderr, "\nRuntime exception at 0x%.8x:\n ", epc.read());
    else fprintf(stderr, "\nRuntime exception at 0x%.8x (instruction: 0x%.8x):\n ", epc.read(), bad.read());
    bool ret = false;
    switch (read_cause()) {
        case INTERRUPT:
            fprintf(stderr, "hardware interrupt\n");
            ret = true;
            state.queue_pc_update(epc.read()); // interrupts resume at instruction that caused exception
            break;
        case ARITHMETIC_OVERFLOW_EXCEPTION:
            fprintf(stderr, "arithmetic overflow\n");
            break;
        case ADDRESS_ERROR_EXCEPTION_LOAD:
            fprintf(stderr, "address load exception at address 0x%.8x\n", vaddr.read());
            break;
        case ADDRESS_ERROR_EXCEPTION_STORE:
            fprintf(stderr, "address store exception at address 0x%.8x\n", vaddr.read());
            break;
        case SYSCALL_EXCEPTION:
            fprintf(stderr, "syscall exception\n");
            break;
        case BREAKPOINT_EXCEPTION:
            fprintf(stderr, "breakpoint exception\n");
            break;
        case TRAP_EXCEPTION:
            fprintf(stderr, "trap exception\n");
            break;
        default:
            fprintf(stderr, "unknown exception\n");
            break;
    }
    if (ret) {
        cause.set(0); // clear cause register
        set_interrupts(true);
        set_mode(USER); // return control
    }
    return ret; // return control to user
}

Register & Coprocessor0::get_register(int num) {
    switch (num) {
        case 8:
            return vaddr;
        case 12:
            return status;
        case 13:
            return cause;
        case 14:
            return epc;
        case 0:
            return bad;
        default:
            throw std::out_of_range("Invalid coprocessor 0 register\n");
    }
}

[[nodiscard]] MODE Coprocessor0::get_mode() const {
    s32 v = (status.read() >> 1) & 0x1;
    return v ? KERNEL : USER;
}

MODE Coprocessor0::set_mode(const MODE mode) {
    MODE old = get_mode();
    s32 v = status.read();
    v &= ~(0b10);       // set bit to 0
    v |= (mode << 1);   // OR with new mode
    status.set(v);
    return old;
}

// Sets value of Status bit 0, returns old value
bool Coprocessor0::set_interrupts(bool value) {
    bool old = status.read() % 2 == 1;
    value ? status.set(status.read() | 0b1) : status.set(status.read() & ~0b1);
    return old;
}

// Returns value of Status bit 0
bool Coprocessor0::get_interrupts() const {
    return status.read() & 0b1;
}