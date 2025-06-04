#include "quakedef.h"
#include <SDL.h>

static SDL_AudioDeviceID audio_dev;

qboolean SNDDMA_Init(void)
{
    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = 1024;
    desired.callback = NULL;

    audio_dev = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    if(!audio_dev)
        return false;

    shm = &sn;
    shm->splitbuffer = 0;
    shm->samplebits = (obtained.format == AUDIO_S16SYS) ? 16 : 8;
    shm->speed = obtained.freq;
    shm->channels = obtained.channels;
    shm->samples = obtained.samples * shm->channels;
    shm->submission_chunk = 1;
    shm->buffer = Hunk_AllocName(shm->samples*(shm->samplebits/8), "sndbuf");
    shm->samplepos = 0;

    SDL_PauseAudioDevice(audio_dev, 0);
    return true;
}

int SNDDMA_GetDMAPos(void)
{
    return shm ? shm->samplepos : 0;
}

void SNDDMA_Shutdown(void)
{
    if(audio_dev)
        SDL_CloseAudioDevice(audio_dev);
    audio_dev = 0;
}

void SNDDMA_Submit(void)
{
}

