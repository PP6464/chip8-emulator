#ifndef CHIP8_CPU_H
#define CHIP8_CPU_H

#include <stdint.h>
#define MEMORY_SIZE    4096
#define NUM_REGS       16
#define STACK_SIZE     16
#define DISPLAY_W      64
#define DISPLAY_H      32
#define ROM_START      0x200
#define FONT_SET_START 0x050

typedef struct {
    uint8_t memory[MEMORY_SIZE];
    uint8_t registers[NUM_REGS];             // The registers are labelled V0...VF
    uint16_t index;
    uint16_t rip;
    uint8_t rsp;
    uint16_t stack[STACK_SIZE];
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint8_t display[DISPLAY_H * DISPLAY_W];
    uint8_t keypad[16];
    int sample_index;                       // This is for the audio player
} CHIP8_CPU;

void init_cpu(CHIP8_CPU *cpu);
void load_rom(CHIP8_CPU *cpu, const char *rom_file);
void cpu_cycle(CHIP8_CPU *cpu);

#endif //CHIP8_CPU_H