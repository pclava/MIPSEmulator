#include "Processor.h"
#include "utils.h"

using namespace MIPS;

int main(int argc, char *argv[]) {
    Memory mem;
    Coprocessor0 c0{};
    CPU cpu{c0, std::cin, std::cout};

    if (argc <= 1) {
        fprintf(stderr, "Error: please provide an executable\n");
        return 1;
    }

    cpu.load_executable(argv[1], argc-1, &argv[1], mem);

    while (true) {
        try {
            if (cpu.powered) {
                cpu.cycle(mem); // fetch, decode, and execute
            }

            // ISSUE HARDWARE INTERRUPTS

        } catch (std::runtime_error &) {
            // IF CPU TERMINATED, CATCH AND EXIT
            printf("\nProgram terminated with exit code %d\n", cpu.exit);
            break;
        }
    }
}
