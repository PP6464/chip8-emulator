#ifndef CHIP8_DISPLAY_H
#define CHIP8_DISPLAY_H

#include <SDL2/SDL.h>
#include <cpu.h>

#define SCALE_PER_PIXEL 10

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
} CHIP8_DISPLAY;

void init_display(CHIP8_DISPLAY *display, const char *title);
void display_draw(const CHIP8_DISPLAY *display, const CHIP8_CPU *cpu);
void display_destroy(const CHIP8_DISPLAY *display);

#endif //CHIP8_DISPLAY_H