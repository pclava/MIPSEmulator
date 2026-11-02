#ifndef MIPS_DISPLAY_H
#define MIPS_DISPLAY_H

// NOTE: this is called display but it also does keyboard input
// don't complain. it's not confusing. i don't know what you mean

#include "Device.h"
#include "utils.h"
#include "SDL3/SDL.h"

class MIPS::Display : public Device {
private:
    unsigned char palette[256][3];
    static constexpr Word DISPLAY_WIDTH = 320;
    static constexpr Word DISPLAY_HEIGHT = 200;
    static constexpr Word KEY_BUFFER = 0xFFFFFA00;
    Word numblocks;

    SDL_Renderer *renderer = nullptr;
    SDL_Texture *term_canvas = nullptr;

public:
    DisplayMode mode;
    SDL_Texture *glyph_cache[128]{};

    explicit Display();

    void set_up_renderer(SDL_Renderer *renderer);

    void write_key(Byte scancode);

    // GRAPHICS

    [[nodiscard]] bool render_frame() const;

    // TEXT

    [[nodiscard]] bool render_text() const;

    [[nodiscard]] bool term_dirty() const;

    void clear_text();

    void draw_char(VGAChar c, Word x, Word y) const;

    // Device functions

    void signal(Word addr, Word val, size_t bytes) override;

    [[nodiscard]] Word query(Word addr, size_t bytes) override;

};

#endif //MIPS_DISPLAY_H