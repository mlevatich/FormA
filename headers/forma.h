#ifndef FORMA
#define FORMA

#ifdef USE_SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
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
    int id;
    SDL_Texture* t;
    int w;
    int h;
    double x;
    double y;
    double theta;
    double dx;
    double dy;
    double omega;
    SDL_Rect* bb;
    int nbb;
}
Sprite;

struct SpriteList
{
	struct SpriteList* prev;
	struct SpriteList* next;
	Sprite* sprite;
};
typedef struct SpriteList SpriteList;

#endif // FORMA
