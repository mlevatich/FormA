#include "../headers/constants.h"

// Window and renderer, used by all modules
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

typedef struct Sprite
{
    bool active;
    SDL_Texture* t;
    int w;
    int h;
    int x;
    int y;
    int theta;
}
Sprite;

// Game state is captured by this data structure
typedef struct State
{
    Sprite* ship;
    long long time;
}
State;

// Load an SDL texture from a BMP file
SDL_Texture* loadTexture(const char* path)
{
    // Create a surface from path to bitmap file
    SDL_Texture* newTexture = NULL;
    SDL_Surface* loaded = SDL_LoadBMP(path);

    // Create a texture from the surface
    newTexture = SDL_CreateTextureFromSurface(renderer, loaded);
    SDL_FreeSurface(loaded);
    return newTexture;
}

// Load new sprite into the game
Sprite* loadSprite(bool a, int w, int h, int x, int y, const char* path)
{
    Sprite* s = malloc(sizeof(Sprite));
    s->active = a;
    s->t = loadTexture(path);
    s->w = w; s->h = h;
    s->x = x; s->y = y;
    s->theta = 0;
    return s;
}

// Destroy sprite memory
void unloadSprite(Sprite* s)
{
    SDL_DestroyTexture(s->t);
    free(s);
}

// Load SDL and initialize the window, renderer, audio, and data
bool loadGame(State* s)
{
    // Initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return false;

    // Create window
    window = SDL_CreateWindow("FormA", 20, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if(!window) return false;

    // Create renderer for window
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer) return false;

    // Initialize renderer color and image loading
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    // Initial state
    int center_x = SCREEN_WIDTH / 2 - 10;
    int center_y = SCREEN_HEIGHT / 2 - 10;
    s->ship = loadSprite(true, 20, 20, center_x, center_y, "graphics/ship.bmp");
    s->time = 0;

    return true;
}

// Free all resources and quit SDL
void quitGame(State* s)
{
    // Free renderer and window
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // Free state
    unloadSprite(s->ship);

    // Free SDL
    SDL_Quit();
}

// Use keydown events to update the game state
void handleKeydown(State* s, int key)
{

}

// Use keyboard state to update the game state
void updateGame(State* s, const Uint8* keys)
{
    int ship_speed = 3;
    if (keys[SDL_SCANCODE_UP])    s->ship->y -= ship_speed;
    if (keys[SDL_SCANCODE_DOWN])  s->ship->y += ship_speed;
    if (keys[SDL_SCANCODE_LEFT])  s->ship->x -= ship_speed;
    if (keys[SDL_SCANCODE_RIGHT]) s->ship->x += ship_speed;
}

// Render the entire game state each frame
void renderGame(const State* s)
{
    SDL_Rect src = { 0, 0, s->ship->w, s->ship->h };
    SDL_Rect dst = { s->ship->x, s->ship->y, s->ship->w, s->ship->h };
    SDL_RenderCopy(renderer, s->ship->t, &src, &dst);
}

int main(int argc, char** argv)
{
    // Parse command line arguments
    if(argc > 1)
    {
        if(!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))
        {
            printf("CSEE6863 1.0.0\n");
            return 0;
        }
        else if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
        {
            printf("\nCSEE6863 1.0.0\n\n");
            printf("Options\n");
            printf("----------------\n");
            printf("-v, --version        print version information\n");
            printf("-h, --help           print help text\n\n");
            return 0;
        }
        else
        {
            printf("Unknown option: %s\n", argv[1]);
            printf("Use -h or --help to see a list of available options.\n");
            return 0;
        }
    }

    // Load game, make initial state
    State s;
    if(!loadGame(&s))
    {
        fprintf(stderr, "Error: Initialization Failed\n");
        return 1;
    }

    // Game loop
    long long frame = 0;
    bool quit = false;
    while(!quit)
    {
        // Track how long this frame takes
        int start_time = SDL_GetTicks();

        // Process any SDL events that have happened since last frame
        SDL_Event e;
        while(SDL_PollEvent(&e) != 0)
        {
            // No need to process further events if an exit signal was received
            if(e.type == SDL_QUIT) quit = true;

            // Process key presses
            if(e.type == SDL_KEYDOWN)
            {
                int key = e.key.keysym.sym;
                handleKeydown(&s, key);
            }
        }

        // Update the game state for this frame, based on current game state
        // and current keyboard state
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        updateGame(&s, keys);

        // Render changes to screen based on current game state
        SDL_RenderClear(renderer);
        renderGame(&s);
        SDL_RenderPresent(renderer);

        // Cap framerate at MAX_FPS
        double ms_per_frame = 1000.0 / MAX_FPS;
        int sleep_time = ms_per_frame - (SDL_GetTicks() - start_time);
        if(sleep_time > 0) SDL_Delay(sleep_time);
        frame++;
    }

    // Free all resources and exit game
    quitGame(&s);
    return 0;
}
