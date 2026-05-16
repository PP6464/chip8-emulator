#ifndef CHIP8_AUDIO_H
#define CHIP8_AUDIO_H

#include <cpu.h>

#define SAMPLE_RATE         44100
#define AMPLITUDE           30000
#define FREQUENCY           440.0
#define SOUND_TIME_DEC_RATE 60

void init_audio(CHIP8_CPU *cpu);

#endif //CHIP8_AUDIO_H