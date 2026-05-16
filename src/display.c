#include <display.h>
#include <cpu.h>
#include <SDL2/SDL.h>

void init_display(CHIP8_DISPLAY *display, const char *title) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    display->window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        DISPLAY_W * SCALE_PER_PIXEL,
        DISPLAY_H * SCALE_PER_PIXEL,
        0
    );
    display->renderer = SDL_CreateRenderer(display->window, -1, SDL_RENDERER_ACCELERATED);
    display->texture = SDL_CreateTexture(
        display->renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        DISPLAY_W,
        DISPLAY_H
    );
}

void display_draw(const CHIP8_DISPLAY *display, const CHIP8_CPU *cpu) {
    uint32_t pixels[DISPLAY_W * DISPLAY_H];
    for (int i = 0; i < DISPLAY_W * DISPLAY_H; i++) {
        pixels[i] = cpu->display[i] ? 0xFFFFFFFF : 0x00000000;
    }

    SDL_UpdateTexture(display->texture, nullptr, pixels, DISPLAY_W * sizeof(uint32_t));
    SDL_RenderClear(display->renderer);
    SDL_RenderCopy(display->renderer, display->texture, nullptr, nullptr);
    SDL_RenderPresent(display->renderer);
}

void display_destroy(const CHIP8_DISPLAY *display) {
    SDL_DestroyTexture(display->texture);
    SDL_DestroyRenderer(display->renderer);
    SDL_DestroyWindow(display->window);
    SDL_Quit();
}
