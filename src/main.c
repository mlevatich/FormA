#include "../headers/constants.h"
#include "../headers/forma.h"

// Window and renderer, used by all modules
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

struct Asteroid
{
	struct Asteroid* prev;
	struct Asteroid* next;
	Sprite* sprite;
};
typedef struct Asteroid Asteroid;

// Game state is captured by this data structure
typedef struct State
{
	long score;
    Sprite* ship;
	Asteroid* asteroids; // linked list of asteroids
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
Sprite* loadSprite(bool a, int w, int h, double x, double y, const char* path)
{
    Sprite* s = malloc(sizeof(Sprite));
    s->active = a;
    s->t = loadTexture(path);
    s->w = w;
    s->h = h;
    s->x = x;
    s->y = y;
    s->theta = M_PI_2;
    s->dx = 0;
    s->dy = 0;
    s->omega = 0;
    return s;
}

// Destroy sprite memory
void unloadSprite(Sprite* s)
{
    SDL_DestroyTexture(s->t);
    free(s);
}

// Load SDL and initialize the window, renderer, audio, and data
bool loadGame(State* st)
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
    double c_x = (double) SCREEN_WIDTH / 2 - 10;
    double c_y = (double) SCREEN_HEIGHT / 2 - 10;
    st->ship = loadSprite(true, 20, 20, c_x, c_y, "graphics/ship.bmp");
	st->asteroids = NULL;

    return true;
}

// Free all resources and quit SDL
void quitGame(State* st)
{
    // Free renderer and window
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // Free state
    unloadSprite(st->ship);

    // Free SDL
    SDL_Quit();
}

// Use keydown events to update the game state
// TODO: What is this?
void handleKeydown(State* st, int key)
{

}

bool planeCollided(const State* st)
{
	Asteroid* asteroid = st->asteroids;
	while (asteroid != NULL) {
		// TODO: collision logic
		asteroid  = asteroid->next;
	}
	return false;
}

// Use keyboard state to update the game state
void updateGame(State* st, const Uint8* keys)
{
    Sprite* s = st->ship;

    // Constants
    double thrust = 0.08;
    double thrust_damp = 0.99;
    double torque = 0.003;
    double torque_damp = 0.95;

    // Damping force based on velocity (simulates friction / resistance)
    s->dx *= thrust_damp;
    s->dy *= thrust_damp;
    s->omega *= torque_damp;

    // Apply forces based on keystate
    if (keys[SDL_SCANCODE_UP]) {
        s->dx += thrust * cos(s->theta);
        s->dy -= thrust * sin(s->theta);
    }
    if (keys[SDL_SCANCODE_LEFT]) {
        s->omega += torque;
    }
    if (keys[SDL_SCANCODE_RIGHT]) {
        s->omega -= torque;
    }

    // Propagate to derived quantities
	// TODO: Check Out Of Bounds
    s->x += s->dx;
    s->y += s->dy;
    s->theta += s->omega;

	// TODO: spawn asteroids

	if (planeCollided(st)) {
		// TODO: send gameOver signal
	}
}


// Render the entire game state each frame
void renderGame(const State* st)
{
    Sprite* s = st->ship;

    SDL_Rect src = { 0, 0, s->w, s->h };
    SDL_Rect dst = { (int) s->x, (int) s->y, s->w, s->h };
    double rot = -s->theta * (180.0 / M_PI);
    SDL_RenderCopyEx(renderer, s->t, &src, &dst, rot, NULL, SDL_FLIP_NONE);

}

int main(int argc, char** argv)
{
    // Parse command line arguments
    if(argc > 1)
    {
        if(!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))
        {
            printf("FormA 1.0.0\n");
            return 0;
        }
        else if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
        {
            printf("\nFormA 1.0.0\n\n");
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
    State st;
    if(!loadGame(&st))
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
                handleKeydown(&st, key);
            }
        }

        // Update the game state for this frame, based on current game state
        // and current keyboard state
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        updateGame(&st, keys);

        // Render changes to screen based on current game state
        SDL_RenderClear(renderer);
        renderGame(&st);
        SDL_RenderPresent(renderer);

        // Cap framerate at MAX_FPS
        double ms_per_frame = 1000.0 / MAX_FPS;
        int sleep_time = ms_per_frame - (SDL_GetTicks() - start_time);
        if(sleep_time > 0) SDL_Delay(sleep_time);
        frame++; // TODO: is this used anywhere?
    }

    // Free all resources and exit game
    quitGame(&st);
    return 0;
}
