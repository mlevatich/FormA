/*
Spoof of the interface to SDL2/SDL.h needed for FormA, so that we can model
check the game independently of SDL.
*/

// Values never used
enum {
    SDL_INIT_VIDEO, SDL_INIT_AUDIO,
    SDL_RENDERER_ACCELERATED,
    SDL_FLIP_NONE, SDL_QUIT, SDL_KEYDOWN
};

// Used as array indicies, must not exceed length of spoofKeystate
#define SDL_SCANCODE_UP    0
#define SDL_SCANCODE_LEFT  1
#define SDL_SCANCODE_RIGHT 1

// Only passed as pointers, never dereferenced; type could be anything
typedef int SDL_Window;
typedef int SDL_Renderer;
typedef int SDL_Texture;
typedef int SDL_Surface;
typedef int Uint8;

// SDL_Event and SDL_Rect are accessed directly by us,
// so they need actual definitions
typedef struct SSE { int sym; } SSE;
typedef struct SE { SSE keysym; } SE;
typedef struct SDL_Event { int type; SE key; } SDL_Event;
typedef struct SDL_Rect { int x; int y; int w; int h; } SDL_Rect;

// The "keyboard" that the SDL_SCANCODEs index into.
// These settings cause the ship to accelerate forward but not turn
static Uint8 spoofKeystate[2] = { 1, 0 };

// 0 indicates success
static inline int           SDL_Init(int a)                                                     { return 0; }
static inline int           SDL_GetTicks()                                                      { return 0; }

// SDL_PollEvent must return 0 and set the event type to
// an integer value which is not SDL_KEYDOWN or SDL_QUIT.
static inline int           SDL_PollEvent(SDL_Event* a)                                         { a->type = 100; return 0; }

// Returns the spoofed keyboard state
static inline const Uint8*  SDL_GetKeyboardState(void* a)                                       { return spoofKeystate; }

// The pointers returned are never dereferenced,
// but they are checked for non-NULL, so I made them 0x01
static inline SDL_Window*   SDL_CreateWindow(const char* a, int b, int c, int d, int e, int f)  { return (SDL_Window*) 0x01; }
static inline SDL_Renderer* SDL_CreateRenderer(SDL_Window* a, int b, int c)                     { return (SDL_Renderer*) 0x01; }

// Pointers returned from these are never dereferenced
static inline SDL_Surface*  SDL_LoadBMP(const char* a)                                          { return NULL; }
static inline SDL_Texture*  SDL_CreateTextureFromSurface(SDL_Renderer* a, SDL_Surface* b)       { return NULL; }

// No-op functions
static inline void          SDL_Quit()                                                          {}
static inline void          SDL_Delay(int a)                                                    {}
static inline void          SDL_SetRenderDrawColor(SDL_Renderer* a, int b, int c, int d, int e) {}
static inline void          SDL_FreeSurface(SDL_Surface* a)                                     {}
static inline void          SDL_DestroyTexture(SDL_Texture* a)                                  {}
static inline void          SDL_DestroyRenderer(SDL_Renderer* a)                                {}
static inline void          SDL_DestroyWindow(SDL_Window* a)                                    {}
static inline void          SDL_RenderClear(SDL_Renderer* a)                                    {}
static inline void          SDL_RenderPresent(SDL_Renderer* a)                                  {}
static inline void          SDL_RenderCopyEx(SDL_Renderer* a, SDL_Texture* b, SDL_Rect* c,
                                             SDL_Rect* d, double e, void* f, int g)             {}
