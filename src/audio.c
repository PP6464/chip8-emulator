#include <audio.h>
#include <SDL2/SDL.h>

void audio_callback(void *userdata, Uint8 *stream, const int len) {
    CHIP8_CPU *cpu = userdata;
    const auto buf = (Sint16 *) stream;
    const int samples = len / 2;  // 2 bytes per Sint16

    for (int i = 0; i < samples; i++) {
        if (cpu->sound_timer > 0) {
            const double t = (double) cpu->sample_index / SAMPLE_RATE;
            buf[i] = (Sint16) (AMPLITUDE * sin(2.0 * M_PI * FREQUENCY * t));
            cpu->sample_index++;
        } else {
            buf[i] = 0;
            cpu->sample_index = 0;
        }

        // Decrement the sound timer and delay timer at 60Hz
        if (cpu->sample_index % (SAMPLE_RATE / SOUND_TIME_DEC_RATE) == 0) {
            if (cpu->sound_timer) cpu->sound_timer--;
            if (cpu->delay_timer) cpu->delay_timer--;
        }
    }
}

void init_audio(CHIP8_CPU *cpu) {
    const SDL_AudioSpec want = {
        .freq     = SAMPLE_RATE,
        .format   = AUDIO_S16SYS,
        .channels = 1,
        .samples  = 512,
        .callback = audio_callback,
        .userdata = cpu,
    };
    SDL_AudioSpec got;
    const SDL_AudioDeviceID audio_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &got, 0);
    SDL_PauseAudioDevice(audio_dev, 0);  // start the audio thread
}
