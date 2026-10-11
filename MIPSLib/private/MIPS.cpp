#include <cassert>
#include "Display.h"
#include "Processor.h"
#include "utils.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <queue>

using namespace MIPS;

#define TICK_SPEED 500 // tick length in milliseconds

// TODO: formalize
#define font_path "/Users/paulclavaud/Documents/Code/MIPSComputer/emulator/examples/font.ttf"

struct FrontendInterrupt {
    InterruptCode code;
    SDL_Scancode scancode;
};

struct Frontend {
    SDL_Window *window;
    SDL_Renderer *renderer;

    TTF_Font* font = nullptr;

    bool running = true;

    Uint64 last_frame = 0;
    Uint64 last_update = 0;

    Uint64 timer_cycles = 0; // how many cycles since last timer interrupt
    Uint64 frame_count = 0; // how many frames since last timer interrupt
    Uint64 frame_cycles = 0; // how many cycles since last frame
    Uint64 refresh_cycles = 0; // how many cycles since last refresh
    Uint64 refresh = 10000; // how many cycles should be executed before refresh; updates over time with frame rate

    static constexpr Uint64 frame_delay = 1000/60;

    std::queue<FrontendInterrupt> &interrupts;
    Memory &mem;
    Display &display;
    bool do_timer = true;

    // TODO: error checks
    explicit Frontend(std::queue<FrontendInterrupt> &interrupts, Memory &mem, Display &display) : interrupts(interrupts), mem(mem), display(display) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            assert(0 && "Failed to initialize SDL");
        }
        window = SDL_CreateWindow("(MIPSEmulator)", 320, 200, SDL_WINDOW_RESIZABLE);
        renderer = SDL_CreateRenderer(window, nullptr);
        SDL_SetDefaultTextureScaleMode(renderer, SDL_SCALEMODE_NEAREST); // nearest neighbor scaling
        SDL_SetRenderLogicalPresentation(renderer, 320, 200, SDL_LOGICAL_PRESENTATION_LETTERBOX);

        // Initialize SDL_ttf
        if (!TTF_Init()) {
            assert(0 && "Failed to initialize TTF");
        }

        font = TTF_OpenFont(font_path, 50.0f);
        if (!font) {
            assert(0 && "Failed to load font");
        }

        // Setup display
        display.set_up_renderer(renderer);

        // Initialize glyphs
        SDL_Color white = {255,255,255,255};
        char str[2] = {0, 0};
        for (int i = 32; i < 126; i++) { // Printable ASCII range
            str[0] = static_cast<char>(i);
            SDL_Surface* surf = TTF_RenderText_Solid(font, str, 0, white);
            display.glyph_cache[i] = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_DestroySurface(surf);
        }
    }

    ~Frontend() {
        TTF_Quit();
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

    void Run() {
        Uint64 current_time = SDL_GetTicks();

        // Issue timer interrupt
        if (current_time >= last_update + TICK_SPEED) {
            std::string title = "(MIPSEmulator) CPU speed: ";

            // Calculate clock speed and format string
            std::string hz = std::to_string(timer_cycles / TICK_SPEED * 1000);
            int64_t i = static_cast<int64_t>(hz.length()) - 3;
            while (i > 0) {
                hz.insert(i, ",");
                i -= 3;
            }
            title+= hz;
            title+= " hz";


            SDL_SetWindowTitle(window, title.c_str());

            frame_count=0;
            last_update = current_time;
            timer_cycles = 0;

            if (do_timer) interrupts.push({TIMER_INTERRUPT, SDL_SCANCODE_UNKNOWN});
        }

        // Determine whether we refresh screen
        Uint64 dt = current_time - last_frame;
        if (dt >= frame_delay) {

            float late = static_cast<float>(dt) / static_cast<float>(frame_delay);
            refresh = static_cast<Uint64>(static_cast<float>(frame_cycles) / late) / 8;

            bool draw = false;
            if (display.mode == GRAPHICAL) {
                draw = display.render_frame();
                interrupts.push({DISPLAY_REFRESH, SDL_SCANCODE_UNKNOWN});
            }
            else if (display.mode == TEXT) {
                draw = display.render_text();
            }
            if (draw) SDL_RenderPresent(renderer);
            frame_count++;
            last_frame = current_time;
            frame_cycles = 0;
        }
    }

    void Cycle() {
        /*
         * note: only refreshing on occasion is VERY important.
         * for some reason doing this increased cpu speed by 145x
         * in the frame drawing code above, we constantly update the refresh rate
         * to be 1/8 of the frame rate
         */
        if (refresh_cycles >= refresh) {
            Input();
            Run();
            refresh_cycles = 0;
        }
        timer_cycles++;
        refresh_cycles++;
        frame_cycles++;
    }

    DisplayMode setMode(DisplayMode mode) {
        DisplayMode old_mode = display.mode;
        display.mode = mode;

        if (mode == TEXT) {
            display.clear_text();
            SDL_SetRenderLogicalPresentation(renderer, 720, 350, SDL_LOGICAL_PRESENTATION_LETTERBOX);
            SDL_SetWindowSize(window, 720, 350);
        } else if (mode == GRAPHICAL) {
            SDL_SetRenderLogicalPresentation(renderer, 320, 200, SDL_LOGICAL_PRESENTATION_LETTERBOX);
            SDL_SetWindowSize(window, 320, 200);
        }

        return old_mode;
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
    if (argc <= 1 || argc > 3) {
        fprintf(stderr, "Error: please provide an executable\n");
        return 1;
    }

    bool use_peripherals = false;
    int arg = 1;
    int input_file = 1;

    if (argc == 3 && argv[1][0] == '-') {
        input_file = 2;
        if (strcmp(argv[arg], "-d") == 0) {
            use_peripherals = true;
        } else {
            fprintf(stderr, "Error: unrecognized option \"%s\"\n", argv[arg]);
            return 1;
        }
    }

    // Main computer
    Coprocessor0 c0{};
    CPU cpu{c0, std::cin, std::cout};
    Memory mem{c0};

    // Prepare for kernel boot
    c0.set_mode(KERNEL);
    c0.set_exc_level(true);
    c0.status.set(static_cast<s32>(static_cast<u32>(c0.status.read()) | 0x80000000)); // Turn on power bit
    cpu.load_kernel(argv[input_file], mem);
    c0.exc_vec = mem.readWord(EXC_VEC_PTR); // Exception vector MUST be the first word of the kernel data!

    // Peripherals
    std::unique_ptr<Display> display = nullptr;
    std::queue<FrontendInterrupt> interrupt_queue;
    std::unique_ptr<Frontend> frontend = nullptr;
    if (use_peripherals) {
        display = std::make_unique<Display>(mem.mmio);
        frontend = std::make_unique<Frontend>(interrupt_queue, mem, *display);
        frontend->setMode(TEXT); // Start in text mode!
    }

    // Main loop
    while (true) {

        // CPU
        if (cpu.powered) {
            try {
                // Check status bit 31 for power (NOTE: not a real MIPS thing)
                if (c0.status.read() > 0) {
                    throw std::runtime_error("power flag turned off");
                }

                // Fetch, decode, and execute next instruction
                cpu.cycle(mem);

                // If an interrupt is queued and interrupts are enabled, handle it.
                // This allows multiple interrupts to happen at the same time without
                // being ignored.
                if (use_peripherals) {
                    if (c0.get_interrupts_enabled()) {
                        frontend->do_timer = true;
                        if (!interrupt_queue.empty()) {
                            FrontendInterrupt x = interrupt_queue.front();
                            interrupt_queue.pop();

                            if (x.code == KEY_UP || x.code == KEY_DOWN) {
                                display->write_key(x.scancode);
                            }

                            cpu.raise_interrupt(x.code, mem);
                        }
                    } else {
                        frontend->do_timer = false;
                    }
                }
            } catch (std::runtime_error &) {
                // IF CPU TERMINATED, CATCH AND EXIT
                printf("\nKernel shutdown with code %d\n", cpu.RF[2]);
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