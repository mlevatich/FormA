#include "../headers/constants.h"
#include "../headers/forma.h"
#include "../headers/asteroid.h"
#include <assert.h>

// Window, renderer, font
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;
Mix_Music* music = NULL;
Mix_Chunk** sfx = NULL;

// Game state is captured by this data structure
typedef struct State
{
	long long score;
    Sprite* ship;
	int laser_cooldown;
	bool thrust;
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

// Play a sound effect
void playSfx(int sfx_id)
{
    int ch = Mix_PlayChannel(-1, sfx[sfx_id], 0);
	Mix_ExpireChannel(ch, 1000);
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

// Add a new sprite to the head of the linked list of sprites
void addSprite(State* st, Sprite* s)
{
	SpriteList* head = malloc(sizeof(SpriteList));
	head->prev = NULL;
	head->next = st->sprites;
	head->sprite = s;
	if(st->sprites) st->sprites->prev = head;
	st->sprites = head;
}

// Create a new asteroid at a random position off the edge of the screen,
// with a random inward velocity
Sprite* spawnAsteroid(State* st)
{
    // Width and height of the asteroid
    int a_w = 84;
    int a_h = 83;

    // Weight the chances towards spawning an asteroid on the longer edge, to
    // even out the distribution of where they appear across the perimeter
    double ratio = (double) SCREEN_WIDTH / (double) SCREEN_HEIGHT;
    double weighted_chance = ratio * 0.5;

    // Values to fill
    double x = 0.0;
    double y = 0.0;
    double dx = min(((getRand() * 2.5) + 0.5) * (0.5 + st->score / 16000.0), 3);
    double dy = min(((getRand() * 2.5) + 0.5) * (0.5 + st->score / 16000.0), 3);

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
    a->omega = ((getRand() * 0.1) - 0.05) * st->score / 16000.0;
    return a;
}

void breakAsteroid(State* st, Sprite* a)
{
	int w = 45;
	int h = 44;
	for(int i = 0; i < 4; i++) {
		int x = a->x + 2 + (a->w / 2 - 2) * (i >= 2);
		int y = a->y + 2 + (a->h / 2 - 2) * (i > 0 && i < 3);
		Sprite* f = loadSprite(FRAGMENT, w, h, x, y, "graphics/fragment.bmp");
		f->dx = a->dx * (0.5 + getRand() * 0.2 - 0.1);
		f->dy = a->dy * (0.5 + getRand() * 0.2 - 0.1);
		f->theta = (i + getRand() * 0.4 - 0.2) * M_PI_2;
		f->omega = a->omega * (0.5 + getRand() * 0.2 - 0.1);
		addSprite(st, f);
	}
}

// If the linked list is empty, creates it and inserts an asteroid, to make
// sure there is never an empty linked list.
void ensureAsteroids(State* st)
{
    if(!st->sprites) addSprite(st, spawnAsteroid(st));
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

	// True random seed
    struct timeval tm;
    gettimeofday(&tm, NULL);
    srand(tm.tv_sec + tm.tv_usec * 1000000ul);

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

	// Initialize audio
    Mix_OpenAudio(SAMPLE_RATE, MIX_DEFAULT_FORMAT, NUM_CHANNELS, CHUNK_SIZE);

    // Load music and set volume
	double track_select = getRand();
	if(track_select < 0.5) music = Mix_LoadMUS("audio/bg1.wav");
	else                   music = Mix_LoadMUS("audio/bg2.wav");
	Mix_VolumeMusic(100);
	Mix_PlayMusic(music, -1);

    // Make space for sound effect list
    sfx = (Mix_Chunk**) malloc(sizeof(Mix_Chunk*) * NUM_SFX);

    // Menu navigation noises
    sfx[SFX_LASER] = Mix_LoadWAV("audio/laser.wav");
    sfx[SFX_CRASH] = Mix_LoadWAV("audio/crash.wav");

    // Initial state
    double c_x = (double) (SCREEN_WIDTH - ship_w) / 2;
    double c_y = (double) (SCREEN_HEIGHT - ship_h) / 2;
    st->ship = loadSprite(SHIP, ship_w, ship_h, c_x, c_y, "graphics/ship.bmp");
	st->score = 0;
	st->laser_cooldown = 0;
	st->thrust = false;

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

// Remove a sprite at the given position in
// the linked list, and return the next element in the linked list
SpriteList* unloadSpriteInPlace(State* st, SpriteList* a)
{
	if(a->next) a->next->prev = a->prev;
	if(a->prev) a->prev->next = a->next;
	else        st->sprites = a->next;
	unloadSprite(a->sprite);
	SpriteList* next = a->next;
	free(a);
	return next;
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

	// Free font elements
	TTF_CloseFont(font);
	TTF_Quit();

	// Free audio elements
	Mix_FreeMusic(music);
	for(int i = 0; i < NUM_SFX; i++) Mix_FreeChunk(sfx[i]);
    free(sfx);
	Mix_Quit();

    // Free state
    unloadSprite(st->ship);
    unloadSprites(st->sprites);

    // Free SDL
    SDL_Quit();
}

bool colliding(const Sprite* s1, const Sprite* s2)
{
	bool left  = s2->x + s2->w <= s1->x;
	bool right = s1->x + s1->w <= s2->x;
	bool below = s1->y + s1->h <= s2->y;
	bool above = s2->y + s2->h <= s1->y;
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

bool detectAllCollisions(State* st)
{
	// Hash map of sprites marked for deletion
	int len = 0;
	for(SpriteList* a = st->sprites; a != NULL; a = a->next, len++);
	bool* delete = calloc(len, sizeof(bool));

	// Detect any collisions and mark sprites for deletion
	int i = 0;
    for(SpriteList* a = st->sprites; a != NULL; a = a->next, i++) {
		Sprite* s1 = a->sprite;

		// N^2 check
		int j = i + 1;
		for(SpriteList* b = a->next; b != NULL; b = b->next, j++) {
			Sprite* s2 = b->sprite;

			// Rock-laser collisions break the rock and delete the laser
			if(((isRock(s1) && isLaser(s2)) || (isLaser(s1) && isRock(s2)))
			  && colliding(s1, s2) && !delete[i] && !delete[j]) {
		        delete[i] = true;
				delete[j] = true;
				playSfx(SFX_CRASH);
				st->score += 50;
			}
		}

		// Rock-ship collisions end the game
        if(isRock(s1) && colliding(st->ship, s1)) {
			free(delete);
			return true;
		}
    }

	// Delete marked sprites and exit
	int n = 0;
	for(SpriteList* a = st->sprites; a != NULL; n++) {
		if(delete[n]) {
			if(a->sprite->id == ASTER) breakAsteroid(st, a->sprite);
			a = unloadSpriteInPlace(st, a);
		}
		else {
			a = a->next;
		}
	}
	free(delete);
	return false;
}

// Move the ship through space according to our "laws" of physics each frame
void moveShip(State* st, const Uint8* keys)
{
    Sprite* s = st->ship;

    // Ship parameters
    double thrust = 0.08;
    double thrust_damp = 0.99;
    double torque = 0.004;
    double torque_damp = 0.95;

    // Damping force based on velocity (simulates friction / resistance)
    s->dx *= thrust_damp;
    s->dy *= thrust_damp;
    s->omega *= torque_damp;

    // Apply forces based on keystate
    if (keys[SDL_SCANCODE_UP]) {
		st->thrust = true;
        s->dx += thrust * cos(s->theta);
        s->dy -= thrust * sin(s->theta);
    }
	else {
		st->thrust = false;
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
    int n_ast = st->score / 1000 + 3;

    // Count asteroids
    double n = 0;
    for(SpriteList* a = st->sprites; a != NULL; a = a->next) {
		if(a->sprite->id == ASTER) n += 1.0;
		if(a->sprite->id == FRAGMENT) n += 0.25;
	}

    // If we're under capacity, chance to append new asteroid to list
    if(n < n_ast && getRand() < spawn_chance) addSprite(st, spawnAsteroid(st));

#ifdef CBMC
	// There should still be asteroids after the fact
	__CPROVER_postcondition(st->sprites != NULL, "Must be > 0 asteroids");
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

        // Check if the sprite is off screen and remove it
        Sprite* s = a->sprite;
        if(s->x > SCREEN_WIDTH  + radius || s->x + s->w < 0 - radius ||
           s->y > SCREEN_HEIGHT + radius || s->y + s->h < 0 - radius) {
			a = unloadSpriteInPlace(st, a);
        }
        else {
            a = a->next;
        }
    }

// TODO: Check asteroids are all out of bounds
#ifdef CBMC
#endif // CBMC

    // If there are no sprites left, force an asteroid to spawn
    // (linked list should never be empty)
    ensureAsteroids(st);
}

void fireLaser(State* st)
{
	// Laser data. Velocity is at least 4, but in general is a little higher
	// than the ship's velocity, so the ship can never outrun its own lasers
	Sprite* ship = st->ship;
	int l_w = 2;
	int l_h = 12;
	int l_v = max(6, 2 + sqrt(ship->dx * ship->dx + ship->dy * ship->dy));

	// Stupid bullshit to line up the position of the (rotated) laser with the
	// (rotated) nose of the ship. Don't ask how I derived this.
	int w = ship->w;
	int h = ship->h;
	double t = ship->theta;
	int l_x = ship->x + (w * (1 + cos(t)) + l_h * sin(t + M_PI_2)) / 2;
	int l_y = ship->y + (h - w * sin(t) - l_h * (1 - cos(t + M_PI_2))) / 2;

	// Spawn laser and set its direction and velocity
	Sprite* lz = loadSprite(LASER, l_w, l_h, l_x, l_y, "graphics/laser.bmp");
	lz->theta = t + M_PI_2;
	lz->dx = l_v *  cos(t);
	lz->dy = l_v * -sin(t);

	// Add laser to head of linked list of active sprites
	addSprite(st, lz);

	// Laser sound effect
	playSfx(SFX_LASER);
}

void updateLasers(State* st, const Uint8* keys)
{
	if(st->laser_cooldown > 0) st->laser_cooldown--;
	if(st->laser_cooldown == 0 && keys[SDL_SCANCODE_SPACE]) {
		st->laser_cooldown = 50 - st->score / 400;
		fireLaser(st);
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

// Render a bar representing the cooldown of the laser
void renderCooldown(int cd)
{
	SDL_Texture* line = loadTexture("graphics/laser.bmp");
	int w = 2;
	int h = 12;
	int y = 50;
	for(int i = 0; i < cd; i++) {
		int x = 20 + i * 2;
		SDL_Rect src = { 0, 0, w, h };
	    SDL_Rect dst = { x, y, w, h };
	    SDL_RenderCopyEx(renderer, line, &src, &dst, 180, NULL, SDL_FLIP_NONE);
	}
	SDL_DestroyTexture(line);
}

// Render a little flame behind the ship when it's accelerating
void renderThrust(const Sprite* ship)
{
	int th_w = 10;
	int th_h = 8;

	int w = ship->w;
	int h = ship->h;
	double t = ship->theta;
	int th_x = ship->x + w/2 + ((-w/2 - 4) * cos(t)) - th_w/2;
	int th_y = ship->y + h/2 - ((-w/2 - 4) * sin(t)) - th_h/2;

	SDL_Rect src = { 0, 0, th_w, th_h };
    SDL_Rect dst = { th_x, th_y, th_w, th_h };
    double rot = -t * (180.0 / M_PI);

	SDL_Texture* tex = loadTexture("graphics/thrust.bmp");
    SDL_RenderCopyEx(renderer, tex, &src, &dst, rot, NULL, SDL_FLIP_NONE);
	SDL_DestroyTexture(tex);
}

// Render the entire game state each frame
void renderGame(const State* st)
{
    // Ship
    renderSprite(st->ship);

	if(st->thrust) renderThrust(st->ship);

    // Sprites
    for(SpriteList* a = st->sprites; a; a = a->next) renderSprite(a->sprite);

	// Score
	renderScore(st->score);

	// Laser cooldown bar
	renderCooldown(st->laser_cooldown);
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
