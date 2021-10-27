/*
 Constants used by multiple modules
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

// Screen size
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

// Frame rate
#define MAX_FPS 60

// Cardinal directions
enum directions
{ LEFT, RIGHT, UP, DOWN };

// Get random number in [0, 1)
static inline double getRand() { return (double) rand() / (double) RAND_MAX; }

// External constants initialized in main.c
// Rendering, display, texture loading
extern SDL_Window* window;
extern SDL_Renderer* renderer;
SDL_Texture* loadTexture(const char* path);
