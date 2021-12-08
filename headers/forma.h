#ifndef FORMA
#define FORMA

#ifdef USE_SDL
#include <SDL2/SDL.h>
#endif // USE_SDL
#ifndef USE_SDL
#include "mockSDL.h"
#endif // USE_SDL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>

typedef struct Sprite
{
    SDL_Texture* t;
    int w;
    int h;
    double x;
    double y;
    double theta;
    double dx;
    double dy;
    double omega;
}
Sprite;


#endif // FORMA
