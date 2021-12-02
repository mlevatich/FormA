#ifndef FORMA
#define FORMA

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>

// External constants initialized in main.c
// Rendering, display, texture loading
extern SDL_Window* window;
extern SDL_Renderer* renderer;
SDL_Texture* loadTexture(const char* path);

typedef struct Sprite
{
    bool active;
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
