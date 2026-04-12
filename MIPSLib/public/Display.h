#ifndef MIPS_DISPLAY_H
#define MIPS_DISPLAY_H

#include "utils.h"
#include "SDL3/SDL.h"

struct MIPS::Display {
    MemorySegment &memory;
    unsigned char palette[256][3];
    static constexpr Word DISPLAY_WIDTH = 320;
    static constexpr Word DISPLAY_HEIGHT = 200;
    static constexpr Word KEY_BUFFER = 0xFFFFFA00;
    Word numblocks;

    explicit Display(MemorySegment &mem);

    void draw_screen(SDL_Renderer &renderer) const;

    void write_key(Byte scancode) const;

};

#endif //MIPS_DISPLAY_H