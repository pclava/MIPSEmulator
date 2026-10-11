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
    DisplayMode mode;

    SDL_Texture *glyph_cache[128]{};
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *term_canvas = nullptr;

    explicit Display(MemorySegment &mem);

    void set_up_renderer(SDL_Renderer *renderer);

    void write_key(Byte scancode) const;

    // GRAPHICS

    bool render_frame() const;

    // TEXT

    bool render_text() const;

    [[nodiscard]] bool term_dirty() const;

    void clear_text() const;

    void draw_char(VGAChar c, Word x, Word y) const;

};

#endif //MIPS_DISPLAY_H