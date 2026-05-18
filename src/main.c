#include <SDL2/SDL.h>
#include <stdio.h>

#include <cpu.h>
#include <display.h>
#include <audio.h>

int main(const int argc, char **argv) {
    char filename[100];

    if (argc < 2) {
        printf("Enter the name of the file to run: \n");
        const char *str = fgets(filename, sizeof(filename), stdin);
        if (str == nullptr) {
            fprintf(stderr, "Could not read your input.");
            return 1;
        }

        // Remove '\n' at the end
        int i = 0;
        while (str[i] != '\n' && str[i] != '\0') {
            i++;
        }

        filename[i] = '\0';
    } else {
        memcpy(filename, argv[1], sizeof(filename));
    }

    // Initialise the CPU
    CHIP8_CPU *cpu = malloc(sizeof(CHIP8_CPU));
    init_cpu(cpu);
    load_rom(cpu, filename);

    // Initialise the display
    CHIP8_DISPLAY * display = &(CHIP8_DISPLAY) {nullptr, nullptr, nullptr};

    char title[32 + sizeof(argv[1])];
    snprintf(title, sizeof(title), "Chip 8 Emulator - %s", filename);

    init_display(display, title);

    // Initialise audio
    init_audio(cpu);

    // Main loop
    bool running = true;
    SDL_Event e;

    while (running) {
        for (int i = 0; i < 16; i++) {
            cpu->keypad[i] &= 1;
            cpu->keypad[i] *= 0b11;
        }

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }

            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                const bool pressed = e.type == SDL_KEYDOWN;

                switch (e.key.keysym.sym) {
                    case SDLK_1: cpu->keypad[0x1] &= 0b10; cpu->keypad[0x1] += pressed; break;
                    case SDLK_2: cpu->keypad[0x2] &= 0b10; cpu->keypad[0x2] += pressed; break;
                    case SDLK_3: cpu->keypad[0x3] &= 0b10; cpu->keypad[0x3] += pressed; break;
                    case SDLK_4: cpu->keypad[0xC] &= 0b10; cpu->keypad[0xC] += pressed; break;
                    case SDLK_q: cpu->keypad[0x4] &= 0b10; cpu->keypad[0x4] += pressed; break;
                    case SDLK_w: cpu->keypad[0x5] &= 0b10; cpu->keypad[0x5] += pressed; break;
                    case SDLK_e: cpu->keypad[0x6] &= 0b10; cpu->keypad[0x6] += pressed; break;
                    case SDLK_r: cpu->keypad[0xD] &= 0b10; cpu->keypad[0xD] += pressed; break;
                    case SDLK_a: cpu->keypad[0x7] &= 0b10; cpu->keypad[0x7] += pressed; break;
                    case SDLK_s: cpu->keypad[0x8] &= 0b10; cpu->keypad[0x8] += pressed; break;
                    case SDLK_d: cpu->keypad[0x9] &= 0b10; cpu->keypad[0x9] += pressed; break;
                    case SDLK_f: cpu->keypad[0xE] &= 0b10; cpu->keypad[0xE] += pressed; break;
                    case SDLK_z: cpu->keypad[0xA] &= 0b10; cpu->keypad[0xA] += pressed; break;
                    case SDLK_x: cpu->keypad[0x0] &= 0b10; cpu->keypad[0x0] += pressed; break;
                    case SDLK_c: cpu->keypad[0xB] &= 0b10; cpu->keypad[0xB] += pressed; break;
                    case SDLK_v: cpu->keypad[0xF] &= 0b10; cpu->keypad[0xF] += pressed; break;
                    default: break;
                }
            }
        }

        cpu_cycle(cpu);
        display_draw(display, cpu);
        SDL_Delay(2);
    }

    // Clean up resources
    display_destroy(display);
    free(cpu);
    return 0;
}