#include "Processor.h"
#include "utils.h"

using namespace MIPS;

int main(int argc, char *argv[]) {
    Memory mem;
    Coprocessor0 c0;
    CPU cpu{&c0};

    cpu.load_executable(argv[1], argc-1, &argv[1], mem);

    // THIS IS WHERE THE MAGIC HAPPENS
    while (true) {
        try {

            // FETCH INSTRUCTION AT PROGRAM COUNTER
            const Word machine_code = cpu.Fetch(mem);

            // DECODE INSTRUCTION
            const Instruction instr = CPU::Decode(machine_code);

            // EXECUTE INSTRUCTION
            cpu.Execute(mem, instr);

        } catch (std::runtime_error &) {
            // IF CPU TERMINATED, CATCH AND EXIT
            printf("\nProgram terminated with exit code %d\n", cpu.exit);
            break;
        }
    }
}
