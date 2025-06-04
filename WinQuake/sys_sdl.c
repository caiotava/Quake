#include "quakedef.h"
#include <SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

static Uint64 sys_start_time;

int nostdout = 0;
char *basedir = ".";
char *cachedir = ".";
qboolean isDedicated = false;

// --------------------------------------------------------------------
// File IO support lifted from the null backend for portability
// --------------------------------------------------------------------

#define MAX_HANDLES 10
static FILE *sys_handles[MAX_HANDLES];

static int findhandle(void)
{
    for(int i = 1; i < MAX_HANDLES; i++)
        if(!sys_handles[i])
            return i;
    Sys_Error("out of handles");
    return -1;
}

static int filelength(FILE *f)
{
    int pos = ftell(f);
    fseek(f, 0, SEEK_END);
    int end = ftell(f);
    fseek(f, pos, SEEK_SET);
    return end;
}

int Sys_FileOpenRead(char *path, int *handle)
{
    FILE *f;
    int h = findhandle();
    f = fopen(path, "rb");
    if(!f)
    {
        *handle = -1;
        return -1;
    }
    sys_handles[h] = f;
    *handle = h;
    return filelength(f);
}

int Sys_FileOpenWrite(char *path)
{
    FILE *f;
    int h = findhandle();
    f = fopen(path, "wb");
    if(!f)
        Sys_Error("Error opening %s", path);
    sys_handles[h] = f;
    return h;
}

void Sys_FileClose(int handle)
{
    fclose(sys_handles[handle]);
    sys_handles[handle] = NULL;
}

void Sys_FileSeek(int handle, int position)
{
    fseek(sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead(int handle, void *dest, int count)
{
    return fread(dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite(int handle, void *data, int count)
{
    return fwrite(data, 1, count, sys_handles[handle]);
}

int Sys_FileTime(char *path)
{
    FILE *f = fopen(path, "rb");
    if(f)
    {
        fclose(f);
        return 1;
    }
    return -1;
}

void Sys_mkdir(char *path)
{
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path, 0777);
#endif
}

void Sys_Error(char *error, ...)
{
    va_list argptr;
    char text[1024];
    va_start(argptr, error);
    vsnprintf(text, sizeof(text), error, argptr);
    va_end(argptr);
    fprintf(stderr, "%s\n", text);
    SDL_Quit();
    exit(1);
}

void Sys_Printf(char *fmt, ...)
{
    va_list argptr;
    char text[1024];
    if(nostdout)
        return;
    va_start(argptr, fmt);
    vsnprintf(text, sizeof(text), fmt, argptr);
    va_end(argptr);
    printf("%s", text);
}

void Sys_Quit(void)
{
    SDL_Quit();
    exit(0);
}

double Sys_FloatTime(void)
{
    return (SDL_GetTicks64() - sys_start_time) / 1000.0;
}

char *Sys_ConsoleInput(void)
{
    return NULL;
}

void Sys_Sleep(void)
{
    SDL_Delay(1);
}

void Sys_SendKeyEvents(void);

void Sys_HighFPPrecision(void) {}
void Sys_LowFPPrecision(void) {}
void Sys_SetFPCW(void) {}

int main(int argc, char **argv)
{
    static quakeparms_t parms;
    parms.memsize = 16*1024*1024;
    parms.membase = malloc(parms.memsize);
    parms.basedir = basedir;
    parms.cachedir = cachedir;

    COM_InitArgv(argc, argv);
    parms.argc = com_argc;
    parms.argv = com_argv;
    isDedicated = (COM_CheckParm("-dedicated") != 0);

    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_AUDIO) < 0)
        Sys_Error("SDL_Init failed: %s", SDL_GetError());

    sys_start_time = SDL_GetTicks64();

    Host_Init(&parms);

    while(1)
    {
        Sys_SendKeyEvents();
        Host_Frame(0.01);
        SDL_Delay(1);
    }
}

