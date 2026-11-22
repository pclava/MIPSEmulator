#include <cassert>

#include "Display.h"
#include "Processor.h"
#include "utils.h"
#include <SDL3/SDL.h>

#include <queue>

using namespace MIPS;

struct FrontendInterrupt {
    InterruptCode code;
    SDL_Scancode scancode;
};

struct Frontend {
    SDL_Window *window;
    SDL_Renderer *renderer;

    bool running = true;
    Uint64 refresh_rate = 10000; // how many cycles per refresh
    Uint64 refresh_cycles = 0; // how many cycles since last refresh
    Uint64 frame_cycles = 0; // how many cycles since last frame
    Uint64 update_cycles = 0; // how many cycles since last title update
    Uint64 last_frame = 0; // time of last frame
    Uint64 last_update = 0; // time of last title update
    Uint64 frame_count = 0; // frames since last update
    static constexpr Uint64 frame_delay = 1000/60;

    std::queue<FrontendInterrupt> &interrupts;
    Memory &mem;
    Display &display;

    // TODO: error checks
    explicit Frontend(std::queue<FrontendInterrupt> &interrupts, Memory &mem, Display &display) : interrupts(interrupts), mem(mem), display(display) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            assert(0 && "Failed to initialize SDL");
        }
        window = SDL_CreateWindow("(MIPSEmulator)", 640, 400, SDL_WINDOW_RESIZABLE);
        renderer = SDL_CreateRenderer(window, nullptr);
        SDL_SetDefaultTextureScaleMode(renderer, SDL_SCALEMODE_NEAREST); // nearest neighbor scaling
        SDL_SetRenderLogicalPresentation(renderer, 320, 200, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    }

    ~Frontend() {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void Input() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.repeat == 0) {
                        interrupts.push({KEY_DOWN, event.key.scancode});
                    }
                    break;
                case SDL_EVENT_KEY_UP:
                    if (event.key.repeat == 0) {
                        interrupts.push({KEY_UP, event.key.scancode});
                    }
                    break;
                default:
                    break;
            }
        }
    }

    void Render() {
        Uint64 current_time = SDL_GetTicks();

        // Calculate fps and clock speed
        if (current_time >= last_update + 1000) {

            std::string title = "(MIPSEmulator) CPU speed: ";
            std::string hz = std::to_string(update_cycles);
            int64_t i = static_cast<int64_t>(hz.length()) - 3;
            while (i > 0) {
                hz.insert(i, ",");
                i -= 3;
            }
            title += hz;
            title += " hz | ";
            std::string fps = std::to_string(frame_count);
            title += fps;
            title += " fps";
            SDL_SetWindowTitle(window, title.c_str());

            frame_count = 0;
            update_cycles = 0;
            last_update = current_time;
        }

        // Determine whether we should draw the frame
        Uint64 dt = current_time - last_frame;
        if (dt >= frame_delay) {
            float late = static_cast<float>(dt) / static_cast<float>(frame_delay);
            refresh_rate = static_cast<Uint64>(static_cast<float>(frame_cycles) / late) / 8;

            if (display.draw_screen(*renderer)) {
                SDL_RenderPresent(renderer);
            }
            last_frame = current_time;
            frame_count++;
            frame_cycles = 0;
            interrupts.push({DISPLAY_REFRESH, SDL_SCANCODE_UNKNOWN});
        }
    }

    void Cycle() {
        if (refresh_cycles >= refresh_rate) {
            Input();
            Render();
            refresh_cycles = 0;
        }
        refresh_cycles++;
        frame_cycles++;
        update_cycles++;
    }
};

/*
 * EMULATOR ENTRY AND MAIN LOOP
 *
 * Usage:
 * MIPSEmulator <options> <executable> <arguments>
 *
 * Use the -d flag before the executable to enable peripherals like the display and keyboard
 */

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        fprintf(stderr, "Error: please provide an executable\n");
        return 1;
    }

    bool use_peripherals = false;
    int input_file = 1;
    while (argv[input_file][0] == '-') {
        if (strcmp(argv[input_file], "-d") == 0) {
            use_peripherals = true;
        } else {
            fprintf(stderr, "Error: unrecognized option \"%s\"\n", argv[input_file]);
            return 1;
        }
        input_file++;
        if (input_file == argc) {
            fprintf(stderr, "Error: please provide an executable\n");
            return 1;
        }
    }

    // Main computer
    Memory mem{};
    Coprocessor0 c0{};
    CPU cpu{c0, std::cin, std::cout};
    cpu.load_executable(argv[input_file], argc-input_file, &argv[input_file], mem);

    // Peripherals
    std::unique_ptr<Display> display = nullptr;
    std::queue<FrontendInterrupt> interrupt_queue;
    std::unique_ptr<Frontend> frontend = nullptr;
    if (use_peripherals) {
        display = std::make_unique<Display>(mem.mmio);
        frontend = std::make_unique<Frontend>(interrupt_queue, mem, *display);
    }

    // Main loop
    while (true) {

        // CPU
        if (cpu.powered) {
            try {
                // Fetch, decode, and execute next instruction
                cpu.cycle(mem);

                // If an interrupt is queued and interrupts are enabled, handle it
                // This allows multiple interrupts to happen at the same time without
                // being ignored.
                if (use_peripherals) {
                    if (c0.get_interrupts_enabled() && !interrupt_queue.empty()) {
                        printf("interrupt\n");
                        FrontendInterrupt x = interrupt_queue.front();
                        interrupt_queue.pop();
                        if (x.code == KEY_UP || x.code == KEY_DOWN) {
                            display->write_key(x.scancode);
                        }
                        cpu.raise_interrupt(x.code, mem);
                    }
                }
            } catch (std::runtime_error &) {
                // IF CPU TERMINATED, CATCH AND EXIT
                printf("\nProgram terminated with exit code %d\n", cpu.exit);
                break;
            }
        }

        // PERIPHERALS
        if (use_peripherals) {
            if (frontend->running) {
                frontend->Cycle();
            }
            else {
                printf("Program aborted by user\n");
                break;
            }
        }
    }
    return 0;
}
