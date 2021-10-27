#include "../headers/constants.h"

// Window and renderer, used by all modules
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

// Game state is captured by this data structure
typedef struct State
{
    SDL_Texture* box_texture;
    int box_w;
    int box_h;
    int box_x;
    int box_y;
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

// Load SDL and initialize the window, renderer, audio, and data
bool loadGame(State* s)
{
    // Initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return false;

    // Create window
    window = SDL_CreateWindow("CSEE6863", 20, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if(!window) return false;

    // Create renderer for window
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer) return false;

    // Initialize renderer color and image loading
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    // Initial state
    s->box_w = 16;
    s->box_h = 16;
    s->box_x = SCREEN_WIDTH / 2 - (s->box_w / 2);
    s->box_y = SCREEN_HEIGHT / 2 - (s->box_h / 2);
    s->box_texture = loadTexture("graphics/box.bmp");

    return true;
}

// Free all resources and quit SDL
void quitGame(State* s)
{
    // Free renderer and window
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // Free state
    SDL_DestroyTexture(s->box_texture);

    // Free SDL
    SDL_Quit();
}

// Use keydown events to update the game state
void handleKeydown(State* s, int key)
{

}

// Use keyboard state to update the game state
void updateGame(State* s)
{
    int box_speed = 3;
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_UP])    s->box_y -= box_speed;
    if (keys[SDL_SCANCODE_DOWN])  s->box_y += box_speed;
    if (keys[SDL_SCANCODE_LEFT])  s->box_x -= box_speed;
    if (keys[SDL_SCANCODE_RIGHT]) s->box_x += box_speed;
}

// Render the entire game state each frame
void renderGame(const State* s)
{
    SDL_Rect src = { 0, 0, s->box_w, s->box_h };
    SDL_Rect dst = { s->box_x, s->box_y, s->box_w, s->box_h };
    SDL_RenderCopy(renderer, s->box_texture, &src, &dst);
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
        updateGame(&s);

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
