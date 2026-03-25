#include "Display.h"
#include "Processor.h"
#include "utils.h"
#include "SDL3/SDL.h"

using namespace MIPS;

struct Application {
    SDL_Window *window;
    SDL_Renderer *renderer;
    bool running = true;
    float fps = 0;
    Uint64 last_frame = 0;
    static constexpr Uint64 frame_delay = 16; // 62.5 fps

    // TODO: error checks
    Application() {
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("MIPSEmulator", 320, 200, SDL_WINDOW_RESIZABLE);
        renderer = SDL_CreateRenderer(window, nullptr);
        SDL_SetRenderLogicalPresentation(renderer, 320, 200, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    }

    ~Application() {
        SDL_Quit();
    }

    void Input() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                default:
                    break;
            }
        }
    }

    void Render(Display &display) {
        Uint64 current_time = SDL_GetTicks();
        Uint64 dt = current_time -last_frame;
        if (dt >= frame_delay) {
            display.draw_screen(*renderer);
            SDL_RenderPresent(renderer);
            last_frame = current_time;
        }
    }
};

int main(int argc, char *argv[]) {
    Memory mem{};
    Coprocessor0 c0{};
    CPU cpu{c0, std::cin, std::cout};
    Display display{mem.mmio};

    Application sdl_context{};

    if (argc <= 1) {
        fprintf(stderr, "Error: please provide an executable\n");
        return 1;
    }

    cpu.load_executable(argv[1], argc-1, &argv[1], mem);

    while (true) {
        if (cpu.powered) {
            try {
                // FETCH, DECODE, EXECUTE NEXT INSTRUCTION
                cpu.cycle(mem);
            } catch (std::runtime_error &) {
                // IF CPU TERMINATED, CATCH AND EXIT
                printf("\nProgram terminated with exit code %d\n", cpu.exit);
                break;
            }
        }

        // HANDLE HARDWARE
        if (sdl_context.running) {

            // Handle events
            sdl_context.Input();

            // Draw frame, if needed
            sdl_context.Render(display);

        }
        // Exit if user closed screen
        else {
            printf("Program aborted by user\n");
            break;
        }
    }
}
