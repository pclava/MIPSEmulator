#include "utils.h"
#include "Processor.h"

Coprocessor0::Coprocessor0() : vaddr(8, 0), status(12, 0xff11), cause(13, 0), epc(14, 0), bad(0,0) {}

// updates bits 2-6 of the cause register
void Coprocessor0::set_cause(const ExceptionCode exception) {
    auto code = static_cast<unsigned char>(exception);
    code &= 0x1F; // mask out upper bits
    code <<= 2; // move up two bits
    cause.set(cause.read() | code);
}

// Returns bits 2-6 of the cause register
[[nodiscard]] ExceptionCode Coprocessor0::read_cause() const {
    return static_cast<ExceptionCode>((cause.read() & 0b1111100) >> 2);
}

// Built-in error handler
// One day I'll implement custom error handlers in the kernel text
bool Coprocessor0::handle_exception(CPU &state) {
    status.set(status.read() & ~0b1); // disable interrupts
    state.queue_pc_update(state.PC.read()+4); // resume at next instruction
    if (bad.read() == 0) fprintf(stderr, "\nRuntime exception at 0x%.8x:\n ", epc.read());
    else fprintf(stderr, "\nRuntime exception at 0x%.8x (instruction: 0x%.8x):\n ", epc.read(), bad.read());
    bool ret = false;
    switch (read_cause()) {
        case INTERRUPT:
            fprintf(stderr, "hardware interrupt\n");
            ret = true;
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
    cause.set(0); // clear cause register
    status.set(status.read() | 1); // enable interrupts
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