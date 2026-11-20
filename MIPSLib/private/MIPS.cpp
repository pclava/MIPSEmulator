#include <cassert>
#include "Display.h"
#include "Processor.h"
#include "utils.h"
#include "Disk.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <queue>
#include <format>

#include <sys/mman.h>
#include "mof.h"
#include "tlb.h"

using namespace MIPS;

#define TICK_SPEED 10 // tick length in milliseconds

#define PTSIZE_DEF 50.0f // default font size
extern const unsigned char font_ttf[];
extern const std::size_t font_ttf_size;

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

    Uint64 timer_cycles = 0; // how many cycles since last second
    Uint64 frame_count = 0; // how many frames since last second
    Uint64 frame_cycles = 0; // how many cycles since last frame
    Uint64 refresh_cycles = 0; // how many cycles since last refresh
    Uint64 refresh = 10000; // how many cycles should be executed before refresh; updates over time with frame rate
    Uint64 ticks = 0;       // counts ticks

    static constexpr Uint64 frame_delay = 1000/60;

    std::queue<FrontendInterrupt> &interrupts;
    Memory &mem;
    Display &display;

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

        SDL_IOStream *stream = SDL_IOFromConstMem(font_ttf, font_ttf_size);
        font = TTF_OpenFontIO(stream, true, 50.0f);
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
            interrupts.push({TIMER_INTERRUPT, SDL_SCANCODE_UNKNOWN});

            // Every second, update title
            if (ticks == 1000/TICK_SPEED) {
                std::string title = "(MIPSEmulator) CPU speed: ";

                // Calculate clock speed and format string
                std::string hz = std::to_string(timer_cycles);
                int64_t i = static_cast<int64_t>(hz.length()) - 3;
                while (i > 0) {
                    hz.insert(i, ",");
                    i -= 3;
                }
                title+= hz;
                title+= " hz | ";
                std::string fps = std::to_string(frame_count);
                title+= fps;
                title+= " fps";

                SDL_SetWindowTitle(window, title.c_str());

                ticks = 0;
                frame_count= 0;
                timer_cycles = 0;
            }

            last_update = current_time;
            ticks++;
        }

        // Determine whether we refresh screen
        Uint64 dt = current_time - last_frame;
        if (dt >= frame_delay) {

            float late = static_cast<float>(dt) / static_cast<float>(frame_delay);
            refresh = static_cast<Uint64>(static_cast<float>(frame_cycles) / late) / 8;

            bool draw = false;
            if (display.mode == D_GRAPHICAL) {
                draw = display.render_frame();
                interrupts.push({DISPLAY_REFRESH, SDL_SCANCODE_UNKNOWN});
            }
            else if (display.mode == D_TEXT) {
                draw = display.render_text();
            }
            if (draw) {SDL_RenderPresent(renderer);}
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

        if (mode == D_TEXT) {
            display.clear_text();
            SDL_SetRenderLogicalPresentation(renderer, 720, 350, SDL_LOGICAL_PRESENTATION_LETTERBOX);
            SDL_SetWindowSize(window, 720, 350);
        } else if (mode == D_GRAPHICAL) {
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
 * MIPSEmulator kernel filesystem
 *
 * Use the -d flag before the executable to enable peripherals like the display and keyboard
 */

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Error: please provide an executable and a file system\n");
        return 1;
    }

    int input_file = 1;

    // Peripherals and main computer
    Display display{};
    Disk disk{argv[2]}; // TODO: configurable disk size

    Coprocessor0 c0{};
    TLB tlb{&c0};
    CPU cpu{c0, tlb};
    Memory mem{c0, tlb, display, disk};

    // Prepare for kernel boot
    c0.set_mode(KERNEL);
    c0.set_exc_level(true);
    c0.status.set(static_cast<s32>(static_cast<u32>(c0.status.read()) | 0x80000000)); // Turn on power bit
    cpu.load_kernel(argv[input_file], mem);
    c0.exc_vec = mem.readWord(EXC_VEC_PTR); // Exception vector MUST be the first word of the kernel data!

    // Frontend
    std::queue<FrontendInterrupt> interrupt_queue; // allows multiple interrupts to arrive between handlings
    Frontend frontend{interrupt_queue, mem, display};
    frontend.setMode(D_TEXT); // Start in text mode!

    // Main loop
    while (true) {

        // CPU
        if (cpu.powered) {
            try {
                // Check status bit 31 for power (NOTE: not a real MIPS thing)
                if (c0.status.read() > 0) {
                    throw std::runtime_error(std::format("\nKernel shutdown with code {}\n", cpu.RF[2]));
                }

                // Fetch, decode, and execute next instruction
                cpu.cycle(mem);

            } catch (std::runtime_error &e) {
                // IF CPU TERMINATED, CATCH AND EXIT
                printf("%s", e.what());
                break;
            }
        }

        // HANDLE PERIPHERALS
        if (!interrupt_queue.empty()) {
            // Receive interrupt
            auto&[code, scancode] = interrupt_queue.front();
            interrupt_queue.pop();

            // If enabled, process interrupt
            if (c0.get_interrupts_enabled()) {
                if (code == KEY_UP || code == KEY_DOWN) {
                    display.write_key(scancode);
                }

                cpu.raise_interrupt(code, mem);
            }

            // Either way, wake up CPU
            cpu.powered = true;
        }

        // RUN FRONTEND
        if (frontend.running) {
            frontend.Cycle();
        }
        else {
            printf("Program aborted by user\n");
            break;
        }
    }

    return 0;
}