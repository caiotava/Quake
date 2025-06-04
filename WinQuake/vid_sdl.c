#include "quakedef.h"
#include <SDL.h>
#include <SDL_opengl.h>

float mouse_x, mouse_y;
static SDL_Window *window = NULL;
static SDL_GLContext glcontext = NULL;
static int mx, my;

viddef_t vid; // global video state
unsigned short d_8to16table[256];
unsigned d_8to24table[256];
unsigned char d_15to8table[65536];

float gldepthmin, gldepthmax;
qboolean is8bit = false;
qboolean isPermedia = false;
qboolean gl_mtexable = false;
int texture_mode = GL_LINEAR;
int texture_extension_number = 1;
const char *gl_vendor, *gl_renderer, *gl_version, *gl_extensions;
cvar_t gl_ztrick = {"gl_ztrick","1"};

void GL_Init(void)
{
    gl_vendor = (const char *)glGetString(GL_VENDOR);
    gl_renderer = (const char *)glGetString(GL_RENDERER);
    gl_version = (const char *)glGetString(GL_VERSION);
    gl_extensions = (const char *)glGetString(GL_EXTENSIONS);

    glClearColor(1.f, 0.f, 0.f, 0.f);
    glCullFace(GL_FRONT);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.666f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glShadeModel(GL_FLAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

void GL_BeginRendering(int *x, int *y, int *width, int *height)
{
    *x = *y = 0;
    *width = vid.width;
    *height = vid.height;
    glViewport(0, 0, vid.width, vid.height);
}

void GL_EndRendering(void)
{
    glFlush();
    SDL_GL_SwapWindow(window);
}

qboolean VID_Is8bit(void) { return is8bit; }
void VID_Init8bitPalette(void) {}

void VID_SetPalette(unsigned char *palette)
{
    unsigned char *pal = palette;
    unsigned *table = d_8to24table;
    for(int i=0; i<256; i++)
    {
        unsigned r = pal[0];
        unsigned g = pal[1];
        unsigned b = pal[2];
        pal += 3;
        *table = (255u<<24) | (r<<0) | (g<<8) | (b<<16);
        d_8to16table[i] = ((r>>3)<<11) | ((g>>2)<<5) | (b>>3);
        table++;
    }
    d_8to24table[255] &= 0xffffff;

    for(int i=0; i<(1<<15); i++)
    {
        int r = ((i & 0x1F) << 3) + 4;
        int g = ((i & 0x03E0) >> 2) + 4;
        int b = ((i & 0x7C00) >> 7) + 4;
        pal = (unsigned char *)d_8to24table;
        int bestdist = 0x7fffffff;
        int best = 0;
        for(int v=0; v<256; v++, pal+=4)
        {
            int dr = r - pal[0];
            int dg = g - pal[1];
            int db = b - pal[2];
            int dist = dr*dr + dg*dg + db*db;
            if(dist < bestdist)
            {
                bestdist = dist;
                best = v;
            }
        }
        d_15to8table[i] = best;
    }
}

void VID_ShiftPalette(unsigned char *palette)
{
    VID_SetPalette(palette);
}

void VID_Shutdown(void)
{
    if(glcontext)
        SDL_GL_DeleteContext(glcontext);
    if(window)
        SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void VID_Init(unsigned char *palette)
{
    if(SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
        Sys_Error("SDL video init failed: %s", SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    window = SDL_CreateWindow("Quake SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if(!window)
        Sys_Error("SDL_CreateWindow failed: %s", SDL_GetError());

    glcontext = SDL_GL_CreateContext(window);
    if(!glcontext)
        Sys_Error("SDL_GL_CreateContext failed: %s", SDL_GetError());

    SDL_GL_SetSwapInterval(1);

    vid.width = vid.conwidth = 640;
    vid.height = vid.conheight = 480;
    vid.numpages = 1;
    vid.maxwarpwidth = WARP_WIDTH;
    vid.maxwarpheight = WARP_HEIGHT;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong(*((int *)vid.colormap + 2048));

    GL_Init();
    VID_SetPalette(palette);
    VID_Init8bitPalette();
    vid.recalc_refdef = 1;
}

void VID_Update(vrect_t *rects)
{
    SDL_GL_SwapWindow(window);
}

void VID_HandlePause(qboolean pause)
{
    SDL_ShowCursor(pause ? SDL_ENABLE : SDL_DISABLE);
}

int VID_SetMode(int modenum, unsigned char *palette)
{
    // only one mode supported
    return true;
}

void Sys_SendKeyEvents(void)
{
    SDL_Event e;
    SDL_PumpEvents();
    while(SDL_PollEvent(&e))
    {
        switch(e.type)
        {
            case SDL_QUIT:
                Sys_Quit();
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                Key_Event(e.key.keysym.sym & 255, e.type == SDL_KEYDOWN);
                break;
            case SDL_MOUSEMOTION:
                mx += e.motion.xrel;
                my += e.motion.yrel;
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if(e.button.button == SDL_BUTTON_LEFT)
                    Key_Event(K_MOUSE1, e.type == SDL_MOUSEBUTTONDOWN);
                else if(e.button.button == SDL_BUTTON_RIGHT)
                    Key_Event(K_MOUSE2, e.type == SDL_MOUSEBUTTONDOWN);
                else if(e.button.button == SDL_BUTTON_MIDDLE)
                    Key_Event(K_MOUSE3, e.type == SDL_MOUSEBUTTONDOWN);
                break;
        }
    }
}

void IN_Init(void)
{
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

void IN_Shutdown(void)
{
    SDL_SetRelativeMouseMode(SDL_FALSE);
}

void IN_Commands(void)
{
}

void IN_Move(usercmd_t *cmd)
{
    mouse_x += mx;
    mouse_y += my;
    mx = my = 0;
}

