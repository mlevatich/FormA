#include "../headers/constants.h"
#include "../headers/forma.h"
#include "../headers/asteroid.h"
#include <assert.h>

// Window, renderer, font
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

// Game state is captured by this data structure
typedef struct State
{
	long long score;
    Sprite* ship;
	int laser_cooldown;
	SpriteList* sprites; // linked list of active sprites
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
Sprite* loadSprite(int id, int w, int h, double x, double y, const char* path)
{
    Sprite* s = malloc(sizeof(Sprite));
	s->id = id;
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

// Create a new asteroid at a random position off the edge of the screen,
// with a random inward velocity
Sprite* spawnAsteroid()
{
    // Width and height of the asteroid
    int a_w = 92;
    int a_h = 89;

    // Weight the chances towards spawning an asteroid on the longer edge, to
    // even out the distribution of where they appear across the perimeter
    double ratio = (double) SCREEN_WIDTH / (double) SCREEN_HEIGHT;
    double weighted_chance = ratio * 0.5;

    // Values to fill
    double x = 0.0;
    double y = 0.0;
    double dx = getRand() * 2.0 + 1;
    double dy = getRand() * 2.0 + 1;

    // From the top of the screen, with downward velocity
    double where = getRand();
    if(where < weighted_chance / 2) {
        x = getRand() * (SCREEN_WIDTH - a_w);
        y = 0 - a_h;
        dx /= 2;
    }

    // From the bottom of the screen, with upward velocity
    else if(where < weighted_chance) {
        x = getRand() * (SCREEN_WIDTH - a_w);
        y = SCREEN_HEIGHT;
        dx /= 2;
        dy *= -1;
    }

    // From the left of the screen, with rightward velocity
    else if(where < weighted_chance + (1 - weighted_chance) / 2) {
        y = getRand() * (SCREEN_HEIGHT - a_h);
        x = 0 - a_w;
        dy /= 2;
    }

    // From the right of the screen, with leftward velofity
    else {
        y = getRand() * (SCREEN_HEIGHT - a_h);
        x = SCREEN_WIDTH;
        dy /= 2;
        dx *= -1;
    }

    // Load the sprite with the computed parameters
	Sprite* a = loadSprite(ASTER, a_w, a_h, x, y, "graphics/asteroid.bmp");
    a->dx = dx;
    a->dy = dy;
    a->omega = getRand() * 0.1 - 0.05;
    return a;
}

// If the linked list is empty, creates it and inserts an asteroid, to make
// sure there is never an empty linked list.
void ensureAsteroids(State* st)
{
    if(!st->sprites) {
        st->sprites = malloc(sizeof(SpriteList));
        st->sprites->next = NULL;
        st->sprites->prev = NULL;
        st->sprites->sprite = spawnAsteroid();
    }
#ifdef CBMC
	__CPROVER_assert(st->sprites != NULL, "There should always be asteroids");
#endif // CBMC

}

// Load SDL and initialize the window, renderer, audio, and data
bool loadGame(State* st)
{
    // Ship size
    int ship_w = 20;
    int ship_h = 20;

    // Initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return false;

    // Create window
    window = SDL_CreateWindow("FormA", 20, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if(!window) return false;

    // Create renderer for window
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer) return false;

    // Initialize renderer color and image loading
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);

	// Font library
	TTF_Init();
	font = TTF_OpenFont("graphics/basis33.ttf", 24);

    // True random seed
    struct timeval tm;
    gettimeofday(&tm, NULL);
    srand(tm.tv_sec + tm.tv_usec * 1000000ul);

    // Initial state
    double c_x = (double) (SCREEN_WIDTH - ship_w) / 2;
    double c_y = (double) (SCREEN_HEIGHT - ship_h) / 2;
    st->ship = loadSprite(SHIP, ship_w, ship_h, c_x, c_y, "graphics/ship.bmp");
	st->score = 0;

    // "seed" the linked list with one asteroid - we don't want it to be empty.
    st->sprites = NULL;
    ensureAsteroids(st);

    return true;
}

// Destroy a sprite
void unloadSprite(Sprite* s)
{
    SDL_DestroyTexture(s->t);
    free(s);
}

// Destroy asteroid linked list
void unloadSprites(SpriteList* a)
{
    while(a) {
        unloadSprite(a->sprite);
        SpriteList* p = a;
        a = a->next;
        free(p);
    }
}

// Free all resources and quit SDL
void quitGame(State* st)
{
    // Free renderer and window
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // Free state
    unloadSprite(st->ship);
    unloadSprites(st->sprites);

    // Free SDL
    SDL_Quit();
}

// Use keydown events to update the game state
// TODO: What is this?
void handleKeypress(State* st, int key)
{

}

bool colliding(const Sprite* s1, const Sprite* s2)
{
	bool left  = s2->x + s2->w < s1->x;
	bool right = s1->x + s1->w < s2->x;
	bool below = s1->y + s1->h < s2->y;
	bool above = s2->y + s2->h < s1->y;
	return !(left || right || below || above);
}

bool isLaser(const Sprite* s)
{
	return s->id == LASER;
}

bool isRock(const Sprite* s)
{
	return (s->id == ASTER || s->id == FRAGMENT);
}

bool detectAllCollisions(const State* st)
{
    for(SpriteList* a = st->sprites; a != NULL; a = a->next) {

		Sprite* s1 = a->sprite;
		for(SpriteList* b = a->next; b != NULL; b = b->next) {

			Sprite* s2 = b->sprite;
			if(isRock(s1) && isRock(s2) && colliding(s1, s2)) {
			    // Resolve rock-rock collision
			}
			if(((isRock(s1) && isLaser(s2)) || (isLaser(s1) && isRock(s2))) && colliding(s1, s2)) {
				// Resolve rock-laser collision
			}
		}

		// Resolve rock-ship collision
        if(isRock(s1) && colliding(st->ship, s1)) return true;
    }
	return false;
}

// Move the ship through space according to our "laws" of physics each frame
void moveShip(State* st, const Uint8* keys)
{
    Sprite* s = st->ship;

    // Ship parameters
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
    s->x += s->dx;
    s->y += s->dy;
    s->theta += s->omega;

    // Screen wrap
    if(s->x > SCREEN_WIDTH)  s->x = 0 - s->w;
    if(s->y > SCREEN_HEIGHT) s->y = 0 - s->h;
    if(s->x < 0 - s->w)      s->x = SCREEN_WIDTH;
    if(s->y < 0 - s->h)      s->y = SCREEN_HEIGHT;

#ifdef CBMC
	bool xOutOfBounds = 0 < s->x || SCREEN_WIDTH < s->x;
	bool yOutOfBounds = 0 < s->y || SCREEN_HEIGHT < s->y;
	__CPROVER_assert(!(xOutOfBounds || yOutOfBounds), "OOB");
#endif // CBMC
}

// Move the asteroids through space according to "laws" of physics each frame
// No forces are applied to asteroids, they just travel through space.
// They don't wrap around the screen either.
void moveSprites(State* st)
{
    for(SpriteList* a = st->sprites; a != NULL; a = a->next) {
        Sprite* s = a->sprite;
        s->x += s->dx;
        s->y += s->dy;
        s->theta += s->omega;
    }
}

// There is a chance of spawning a new asteroid each frame, randomly placed,
// if there are less than the maximum number of asteroids out already
void checkSpawnAsteroid(State* st)
{

#ifdef CBMC
	__CPROVER_precondition(st->sprites != NULL, "");
#endif //CBMC

    // Controls how many asteroids are on screen
    double spawn_chance = 0.05;
    int max_asteroids = 12;

    // Jump to end of list, counting (whole) asteroids seen along the way
    int n = 0;
    SpriteList* a = st->sprites;
    for(; a->next != NULL; a = a->next) if(a->sprite->id == ASTER) n++;
	if(a->sprite->id == ASTER) n++;

    // If we're under capacity, chance to append new asteroid to list
    if(n < max_asteroids && getRand() < spawn_chance) {
        a->next = malloc(sizeof(SpriteList));
        a->next->next = NULL;
        a->next->prev = a;
        a->next->sprite = spawnAsteroid();
    }

#ifdef CBMC
	// There should still be asteroids after the fact
	__CPROVER_postcondition(st->sprites != NULL, "There should still be asteroids");
#endif //CBMC
}

// Handles garbage collection of asteroids after they've left the screen
void checkDespawnSprites(State* st)
{
    // How far off screen a sprite should be before it is despawned
    int radius = 100;

    // Iterate over all sprites
    SpriteList* a = st->sprites;
    while(a) {

        // Check if the sprite is off screen
        Sprite* s = a->sprite;
        if(s->x > SCREEN_WIDTH  + radius || s->x + s->w < 0 - radius ||
           s->y > SCREEN_HEIGHT + radius || s->y + s->h < 0 - radius) {

            // If so, remove it from linked list and delete it
            if(a->next) a->next->prev = a->prev;
            if(a->prev) a->prev->next = a->next;
            else        st->sprites = a->next;
            unloadSprite(s);
            SpriteList* tmp = a;
            a = a->next;
            free(tmp);
        }
        else {
            a = a->next;
        }
    }

// TODO: Check asteroids are all out of bounds
#ifdef CBMC
#endif // CBMC

    // If there are no asteroids left, force one to spawn
    // (linked list should never be empty)
    ensureAsteroids(st);
}

// TODO
void fireLaser(Sprite* ship)
{
	// Get laser base position
	// loadSprite
	// Set velocity, theta
	// Add to linked list of active sprites
}

void updateLasers(State* st, const Uint8* keys)
{
	if(st->laser_cooldown > 0) st->laser_cooldown--;
	if(st->laser_cooldown == 0 && keys[SDL_SCANCODE_SPACE]) {
		st->laser_cooldown = 50;
		fireLaser(st->ship);
	}
}

// Use keyboard state to update the game state
bool updateGame(State* st, const Uint8* keys)
{
    // Ship moves
    moveShip(st, keys);

    // Asteroids and lasers move
    moveSprites(st);

    // Collision detection and resolution
	// Returns true if the ship hit an asteroid
	if(detectAllCollisions(st)) return true;

	// Laser cooldown, check if player wants to fire a laser
	updateLasers(st, keys);

    // When a sprite goes off screen, it is despawned
    checkDespawnSprites(st);

    // Each frame, a new asteroid might spawn
    checkSpawnAsteroid(st);

	// Score goes up by 1 per frame
	// TODO: increase max asteroids and
	// decrease laser cooldown as score goes up
	st->score++;
	return false;
}

// Render a single sprite
void renderSprite(const Sprite* s)
{
    SDL_Rect src = { 0, 0, s->w, s->h };
    SDL_Rect dst = { (int) s->x, (int) s->y, s->w, s->h };
    double rot = -s->theta * (180.0 / M_PI);
    SDL_RenderCopyEx(renderer, s->t, &src, &dst, rot, NULL, SDL_FLIP_NONE);
}

// Render the current score
void renderScore(long long score)
{
	char score_str[100];
	sprintf(score_str, "Score: %010llu", score);
	SDL_Color white = { 255, 255, 255 };
	SDL_Surface* sMsg = TTF_RenderText_Solid(font, score_str, white);
	SDL_Texture* tMsg = SDL_CreateTextureFromSurface(renderer, sMsg);
	SDL_Rect rMsg = { 20, 20, 200, 24 };
	SDL_RenderCopy(renderer, tMsg, NULL, &rMsg);
	SDL_FreeSurface(sMsg);
	SDL_DestroyTexture(tMsg);
}

// Render the entire game state each frame
void renderGame(const State* st)
{
    // Ship
    renderSprite(st->ship);

    // Sprites
    for(SpriteList* a = st->sprites; a; a = a->next) renderSprite(a->sprite);

	// Score
	renderScore(st->score);
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
    bool quit = false;
    while(!quit)
    {
        // Track how long this frame takes
        int start_time = SDL_GetTicks();

        // Check if the player quit the game
        SDL_Event e;
        while(SDL_PollEvent(&e) != 0) if(e.type == SDL_QUIT) quit = true;

        // Update the game state for this frame, based on current game state
        // and current keyboard state
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if(updateGame(&st, keys)) break;

        // Render changes to screen based on current game state
        SDL_RenderClear(renderer);
        renderGame(&st);
        SDL_RenderPresent(renderer);

        // Cap framerate at MAX_FPS
        double ms_per_frame = 1000.0 / MAX_FPS;
        int sleep_time = ms_per_frame - (SDL_GetTicks() - start_time);
        if(sleep_time > 0) SDL_Delay(sleep_time);
    }

    // Free all resources and exit game
	printf("Final score: %llu\n", st.score);
    quitGame(&st);
    return 0;
}
