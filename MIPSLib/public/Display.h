#ifndef MIPS_DISPLAY_H
#define MIPS_DISPLAY_H

#include "utils.h"
#include "SDL3/SDL.h"


struct MIPS::Display {
    MemorySegment &memory;
    SDL_Color palette[256];
    static constexpr Word DISPLAY_WIDTH = 320;
    static constexpr Word DISPLAY_HEIGHT = 200;
    Word numblocks;

    Display(MemorySegment &mem);

    void draw_screen(SDL_Renderer &renderer) const;

};

#endif //MIPS_DISPLAY_H