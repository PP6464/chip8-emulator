#include <cpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>

static const uint8_t font_set[] = {
    0xF0,0x90,0x90,0x90,0xF0, // 0
    0x20,0x60,0x20,0x20,0x70, // 1
    0xF0,0x10,0xF0,0x80,0xF0, // 2
    0xF0,0x10,0xF0,0x10,0xF0, // 3
    0x90,0x90,0xF0,0x10,0x10, // 4
    0xF0,0x80,0xF0,0x10,0xF0, // 5
    0xF0,0x80,0xF0,0x90,0xF0, // 6
    0xF0,0x10,0x20,0x40,0x40, // 7
    0xF0,0x90,0xF0,0x90,0xF0, // 8
    0xF0,0x90,0xF0,0x10,0xF0, // 9
    0xF0,0x90,0xF0,0x90,0x90, // A
    0xE0,0x90,0xE0,0x90,0xE0, // B
    0xF0,0x80,0x80,0x80,0xF0, // C
    0xE0,0x90,0x90,0x90,0xE0, // D
    0xF0,0x80,0xF0,0x80,0xF0, // E
    0xF0,0x80,0xF0,0x80,0x80, // F
};

void init_cpu(CHIP8_CPU *cpu) {
    memset(cpu, 0, sizeof(CHIP8_CPU)); // Clear all the memory first
    cpu->rip = ROM_START;                  // Programs load at address 0x200 (ROM_START) conventionally
    memcpy(cpu->memory + FONT_SET_START, font_set, sizeof(font_set));
    cpu->sample_index = 0;
}

void load_rom(CHIP8_CPU *cpu, const char *rom_file) {
    char path[16 + strlen(rom_file)];
    snprintf(path, 16 + strlen(rom_file), "rom/%s.ch8", rom_file);

    FILE *rom = fopen(path, "rb");
    if (!rom) {
        fprintf(stderr, "Could not open ROM from path %s", path);
        exit(1);  // Erroneous exit
    }
    fread(cpu->memory + ROM_START, 1, MEMORY_SIZE - ROM_START, rom);
    const int result = fclose(rom);

    if (result != 0) {
        fprintf(stderr, "Error closing file %s", path);
        exit(1);
    }
}

void cpu_cycle(CHIP8_CPU *cpu) {
    // Get the opcode
    const uint8_t byte1 = cpu->memory[cpu->rip];
    const uint8_t byte2 = cpu->memory[cpu->rip + 1];
    const uint16_t opcode = byte1 << 8 | byte2;

    // For the following switch statements, some pieces of code may want to set rip to a specific address.
    // But as we increment rip by 2 at the end regardless, we have to start by subtracting 2 beforehand,
    // hence why there are the desired addresses -2 in some places.

    // This corresponds to the first hex digit
    switch ((opcode & 0xF000) >> 12) {
        case 0x0: {
            if (opcode == 0x00E0) {
                memset(cpu->display, 0, sizeof(cpu->display));
            }

            if (opcode == 0x00EE) {
                cpu->rsp--;
                cpu->rip = cpu->stack[cpu->rsp] - 2;
            }

            break;
        }
        case 0x1: {
            cpu->rip = (opcode & 0x0FFF) - 2;

            break;
        }
        case 0x2: {
            cpu->stack[cpu->rsp] = cpu->rip + 2; // Return address (post-increment)
            cpu->rsp++;
            cpu->rip = (opcode & 0x0FFF) - 2;    // Go to the code loaded at the address

            break;
        }
        case 0x3: {
            if (cpu->registers[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
                cpu->rip += 2;
            }

            break;
        }
        case 0x4: {
            if (cpu->registers[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
                cpu->rip += 2;
            }

            break;
        }
        case 0x5: {
            if ((opcode & 0x000F) == 0 && cpu->registers[(opcode & 0x0F00) >> 8] == cpu->registers[(opcode & 0x00F0) >> 4]) {
                cpu->rip += 2;
            }

            break;
        }
        case 0x6: {
            cpu->registers[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;

            break;
        }
        case 0x7: {
            cpu->registers[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;

            break;
        }
        case 0x8: {
            const uint8_t x = (opcode & 0x0F00) >> 8;
            const uint8_t y = (opcode & 0x00F0) >> 4;

            switch (opcode & 0x000F) {
                case 0x0: {
                    cpu->registers[x] = cpu->registers[y];

                    break;
                }
                case 0x1: {
                    cpu->registers[x] |= cpu->registers[y];
                    cpu->registers[0xF] = 0;

                    break;
                }
                case 0x2: {
                    cpu->registers[x] &= cpu->registers[y];
                    cpu->registers[0xF] = 0;

                    break;
                }
                case 0x3: {
                    cpu->registers[x] ^= cpu->registers[y];
                    cpu->registers[0xF] = 0;

                    break;
                }
                case 0x4: {
                    const uint16_t res = (uint16_t) cpu->registers[x] + (uint16_t) cpu->registers[y];

                    cpu->registers[x] = res & 0xFF;
                    cpu->registers[0xF] = res > 0xFF;

                    break;
                }
                case 0x5: {
                    const uint8_t res = cpu->registers[x] - cpu->registers[y];

                    cpu->registers[0xF] = cpu->registers[x] >= cpu->registers[y];
                    cpu->registers[x] = res;

                    break;
                }
                case 0x6: {
                    cpu->registers[0xF] = cpu->registers[x] & 0x1;
                    cpu->registers[x] >>= 1;

                    break;
                }
                case 0x7: {
                    const uint8_t res = cpu->registers[y] - cpu->registers[x];

                    cpu->registers[0xF] = cpu->registers[y] >= cpu->registers[x];
                    cpu->registers[x] = res;

                    break;
                }
                case 0xE: {
                    cpu->registers[0xF] = cpu->registers[x] >> 7;
                    cpu->registers[x] <<= 1;

                    break;
                }
                default: break;
            }

            break;
        }
        case 0x9: {
            if ((opcode & 0x000F) == 0 && cpu->registers[(opcode & 0x0F00) >> 8] != cpu->registers[(opcode & 0x00F0) >> 4]) {
                cpu->rip += 2;
            }

            break;
        }
        case 0xA: {
            cpu->index = opcode & 0x0FFF;

            break;
        }
        case 0xB: {
            cpu->rip = (opcode & 0x0FFF) + cpu->registers[0] - 2;

            break;
        }
        case 0xC: {
            uint8_t random[1];
            getrandom(random, sizeof(random), 0);

            cpu->registers[(opcode & 0x0F00) >> 8] = random[0] & (opcode & 0x00FF);

            break;
        }
        case 0xD: {
            const uint8_t vx = cpu->registers[(opcode & 0x0F00) >> 8];
            const uint8_t vy = cpu->registers[(opcode & 0x00F0) >> 4];
            const uint8_t n = opcode & 0x000F;

            cpu->registers[0xF] = 0;

            uint8_t sprite_data[n];
            memcpy(sprite_data, cpu->memory + cpu->index, n);

            for (int row = 0; row < n; row++) {
                const uint8_t byte = sprite_data[row];

                for (int col = 0; col < 8; col++) {
                    if (!(byte & 0x80 >> col)) {
                        continue;
                    }

                    const uint8_t x_pos = (vx + col) % DISPLAY_W;
                    const uint8_t y_pos = (vy + row) % DISPLAY_H;
                    const uint16_t display_addr = y_pos * DISPLAY_W + x_pos;

                    cpu->registers[0xF] |= cpu->display[display_addr];
                    cpu->display[display_addr] ^= 1;
                }
            }

            break;
        }
        case 0xE: {
            const int x = (opcode & 0x0F00) >> 8;
            const bool pressed = cpu->keypad[cpu->registers[x]] & 1;

            switch (opcode & 0x00FF) {
                case 0x9E: {
                    if (pressed) {
                        cpu->rip += 2;
                    }

                    break;
                }
                case 0xA1: {
                    if (!pressed) {
                        cpu->rip += 2;
                    }

                    break;
                }
                default: break;
            }

            break;
        }
        case 0xF: {
            const int x = (opcode & 0x0F00) >> 8;

            switch (opcode & 0x00FF) {
                case 0x7: {
                    cpu->registers[x] = cpu->delay_timer;

                    break;
                }
                case 0xA: {
                    bool key_released = false;
                    int index = 0;

                    for (int i = 0; i < 16; i++) {
                        if ((cpu->keypad[i] & 3) == 2) {
                            key_released = true;
                            index = i;
                            break;
                        }
                    }

                    if (!key_released) {
                        cpu->rip -= 2;
                    } else {
                        cpu->registers[x] = index;
                    }

                    break;
                }
                case 0x15: {
                    cpu->delay_timer = cpu->registers[x];

                    break;
                }
                case 0x18: {
                    cpu->sound_timer = cpu->registers[x];

                    break;
                }
                case 0x1E: {
                    cpu->index += cpu->registers[x];

                    break;
                }
                case 0x29: {
                    if (cpu->registers[x] <= 0xF) {
                        cpu->index = FONT_SET_START + cpu->registers[x] * 5;
                    }

                    break;
                }
                case 0x33: {
                    const uint8_t value = cpu->registers[x];

                    cpu->memory[cpu->index] = value / 100;
                    cpu->memory[cpu->index + 1] = value % 100 / 10;
                    cpu->memory[cpu->index + 2] = value % 10;

                    break;
                }
                case 0x55: {
                    memcpy(cpu->memory + cpu->index, cpu->registers, sizeof(uint8_t) * (x + 1));

                    cpu->index += x + 1;

                    break;
                }
                case 0x65: {
                    memcpy(cpu->registers, cpu->memory + cpu->index, sizeof(uint8_t) * (x + 1));

                    cpu->index += x + 1;

                    break;
                }
                default: break;
            }

            break;
        }
        default: break;
    }

    cpu->rip += 2;
}
