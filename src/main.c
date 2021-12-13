#include "../headers/constants.h"
#include "../headers/forma.h"
#include "../headers/asteroid.h"
#include <assert.h>

// Window, renderer, font, music
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;
Mix_Music* music = NULL;
Mix_Chunk** sfx = NULL;
Mix_Chunk* thrust_sfx = NULL;
int thrust_ch = -1;
bool debug = false;

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
	Mix_ExpireChannel(Mix_PlayChannel(-1, sfx[sfx_id], 0), 300);
}

// Load new sprite into the game
Sprite* loadSprite(int id, int w, int h, double x, double y,
	               int nbb, SDL_Rect* bb, const char* path)
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
	s->nbb = nbb;
	s->bb = bb;
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
	int nbb = 5;
	SDL_Rect* bb = malloc(sizeof(SDL_Rect) * nbb);
    bb[0] = (SDL_Rect) { 41, 1, 29, 71 };
    bb[1] = (SDL_Rect) { 1, 18, 80, 23 };
	bb[2] = (SDL_Rect) { 16, 10, 34, 71 };
	bb[3] = (SDL_Rect) { 7, 42, 76, 15 };
	bb[4] = (SDL_Rect) { 73, 54, 6, 15 };
	const char* path = "graphics/asteroid.bmp";
	Sprite* a = loadSprite(ASTER, a_w, a_h, x, y, nbb, bb, path);
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
		int x = a->x + 2 + (1.3 * a->w / 2 - 2) * (i >= 2);
		int y = a->y + 2 + (1.3 * a->h / 2 - 2) * (i > 0 && i < 3);
		int nbb = 4;
		SDL_Rect* bb = malloc(sizeof(SDL_Rect) * nbb);
	    bb[0] = (SDL_Rect) { 5, 13, 33, 19 };
	    bb[1] = (SDL_Rect) { 1, 33, 38, 8 };
		bb[2] = (SDL_Rect) { 37, 2, 7, 19 };
		bb[3] = (SDL_Rect) { 19, 9, 19, 5 };
		const char* path = "graphics/fragment.bmp";
		Sprite* f = loadSprite(FRAGMENT, w, h, x, y, nbb, bb, path);
		f->dx = a->dx * (1 + getRand() * 0.2 - 0.1);
		f->dy = a->dy * (1 + getRand() * 0.2 - 0.1);
		if(i >= 2) {
			f->dx += 0.1;
		}
		else {
			f->dx -= 0.1;
		}
		if(i > 0 && i < 3) {
			f->dy += 0.1;
		}
		else {
			f->dy -= 0.1;
		}
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
	music = Mix_LoadMUS("audio/bg1.wav");
	Mix_VolumeMusic(100);
	Mix_PlayMusic(music, -1);

	// Make space for sound effect list
	sfx = (Mix_Chunk**) malloc(sizeof(Mix_Chunk*) * NUM_SFX);

	// Sound effects
	sfx[SFX_LASER] = Mix_LoadWAV("audio/laser.wav");
	sfx[SFX_CRASH] = Mix_LoadWAV("audio/crash.wav");
	sfx[SFX_THRUST] = Mix_LoadWAV("audio/thrust.wav");

	// Initial state
	double c_x = (double) (SCREEN_WIDTH - ship_w) / 2;
	double c_y = (double) (SCREEN_HEIGHT - ship_h) / 2;
	int nbb = 2;
	SDL_Rect* bb = malloc(sizeof(SDL_Rect) * nbb);
    bb[0] = (SDL_Rect) { 2, 2, 7, 16 };
    bb[1] = (SDL_Rect) { 4, 7, 16, 6 };
	const char* path = "graphics/ship.bmp";
	st->ship = loadSprite(SHIP, ship_w, ship_h, c_x, c_y, nbb, bb, path);
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
	free(s->bb);
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

// Precisely check if sprites are touching by
// comparing their arrays of bounding boxes
bool colliding(const Sprite* s1, const Sprite* s2)
{
    // Nested for loop to compare each bounding box pair
    SDL_Rect* b1 = s1->bb;
    SDL_Rect* b2 = s2->bb;
    for(int i = 0; i < s1->nbb; i++) {
        int x1 = b1[i].x + s1->x;
        int y1 = b1[i].y + s1->y;
        for(int j = 0; j < s2->nbb; j++) {
            int x2 = b2[j].x + s2->x;
            int y2 = b2[j].y + s2->y;
			bool x_aligned = (x1 < x2 + b2[j].w && x1 + b1[i].w > x2);
			bool y_aligned = (y1 < y2 + b2[j].h && y1 + b1[i].h > y2);
            if(x_aligned && y_aligned) return true;
        }
    }
    return false;
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

	bool delete[len];
	for (int i=0; i < len; i++) delete[i] = false;

	// Detect any collisions and mark sprites for deletion
	int i = 0;
	for(SpriteList* a = st->sprites; a != NULL; a = a->next, i++) {
		Sprite* s1 = a->sprite;

		// N^2 check
		int j = i + 1;
		for(SpriteList* b = a->next; b != NULL; b = b->next, j++) {
			Sprite* s2 = b->sprite;

			bool laserHit = (isRock(s1) && isLaser(s2))
			             || (isRock(s2) && isLaser(s1));
			bool asteroidsCollide = isRock(s1) && isRock(s2);

			if((laserHit || asteroidsCollide)
					&& colliding(s1, s2) && !delete[i] && !delete[j]) {
				delete[i] = true;
				delete[j] = true;
				// Laser hit sound
				if (laserHit) {
					playSfx(SFX_CRASH);
					st->score += 50;
				}
				else {
					playSfx(SFX_CRASH);
				}
			}
		}

		// Rock-ship collisions end the game
		if(isRock(s1) && colliding(st->ship, s1)) {
			return true;
		}
	}

	// Delete marked sprites and exit
	int n = 0;

	for(SpriteList* a = st->sprites; a != NULL; n++) {
#ifdef CBMC
		__CPROVER_assert(n <= len, "Array access out of bounds!");
#endif // CBMC
		if(delete[n]) {
			if(a->sprite->id == ASTER) breakAsteroid(st, a->sprite);
			a = unloadSpriteInPlace(st, a);
		}
		else {
			a = a->next;
		}
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
	double torque = 0.004;
	double torque_damp = 0.95;

	// Damping force based on velocity (simulates friction / resistance)
	s->dx *= thrust_damp;
	s->dy *= thrust_damp;
	s->omega *= torque_damp;

	// Apply forces based on keystate
	if (keys[SDL_SCANCODE_UP]) {
		st->thrust = true;
		if(thrust_ch == -1) {
			thrust_ch = Mix_PlayChannel(-1, sfx[SFX_THRUST], -1);
		}
		s->dx += thrust * cos(s->theta);
		s->dy -= thrust * sin(s->theta);
	}
	else {
		st->thrust = false;
		if(thrust_ch != -1) {
			Mix_HaltChannel(thrust_ch);
			thrust_ch = -1;
		}
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

	// TODO: Check asteroids are eventually all out of bounds
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
	int nbb = 1;
	SDL_Rect* bb = malloc(sizeof(SDL_Rect) * nbb);
    bb[0] = (SDL_Rect) { 0, 0, 2, 12 };
	const char* path = "graphics/laser.bmp";
	Sprite* lz = loadSprite(LASER, l_w, l_h, l_x, l_y, nbb, bb, path);
	lz->theta = t + M_PI_2;
	lz->dx = l_v *  cos(t);
	lz->dy = l_v * -sin(t);

	// Add laser to head of linked list of active sprites
	addSprite(st, lz);

	// Laser sound effect
	playSfx(SFX_LASER);

/* Ship is never faster than the laser. */
/* What a terrible engineering feat it would be if this were true! */
#ifdef CBMC
	__CPROVER_assert(abs(st->ship->dx) <= abs(lz->dx),
			"Ship dx faster than laser!");
	__CPROVER_assert(abs(st->ship->dy) <= abs(lz->dy),
			"Ship dy faster than laser!");
#endif // CBMC
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
	st->score++;


/* TODO: calculate that ship velocity is less than max velocity */
#ifdef CBMC
#endif // CBMC

	return false;
}

void renderBounds(const Sprite* s)
{
	// For each box, render 4 lines to create the rectangle

	SDL_Texture* tex = loadTexture("graphics/dbg.bmp");
    SDL_Rect* bb = s->bb;
    for(int i = 0; i < s->nbb; i++) {
        // Line 1
        SDL_Rect box = bb[i];
        SDL_Rect clip = { 0, 0, box.w, 1 };
        SDL_Rect quad = { s->x + box.x, s->y + box.y, box.w, 1 };
        SDL_RenderCopyEx(renderer, tex, &clip, &quad, 0, NULL, SDL_FLIP_NONE);

        // Line 2
        quad = (SDL_Rect) { s->x + box.x, s->y + box.y + box.h, box.w, 1 };
        SDL_RenderCopyEx(renderer, tex, &clip, &quad, 0, NULL, SDL_FLIP_NONE);

        // Line 3
        clip = (SDL_Rect) { 0, 0, 1, box.h };
        quad = (SDL_Rect) { s->x + box.x, s->y + box.y, 1, box.h };
        SDL_RenderCopyEx(renderer, tex, &clip, &quad, 0, NULL, SDL_FLIP_NONE);

        // Line 4
        quad = (SDL_Rect) { s->x + box.x + box.w, s->y + box.y, 1, box.h };
        SDL_RenderCopyEx(renderer, tex, &clip, &quad, 0, NULL, SDL_FLIP_NONE);
    }

	SDL_Rect clip = { 5, 5, 2, 2 };
	SDL_Rect quad = { s->x, s->y, 2, 2 };
	SDL_RenderCopyEx(renderer, tex, &clip, &quad, 0, NULL, SDL_FLIP_NONE);

	SDL_DestroyTexture(tex);
}

// Render a single sprite
void renderSprite(const Sprite* s)
{
	SDL_Rect src = { 0, 0, s->w, s->h };
	SDL_Rect dst = { (int) s->x, (int) s->y, s->w, s->h };
	double rot = -s->theta * (180.0 / M_PI);
	SDL_RenderCopyEx(renderer, s->t, &src, &dst, rot, NULL, SDL_FLIP_NONE);
	if(debug) renderBounds(s);
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
	if(argc > 1) {
		if(!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version")) {
			printf("FormA 1.0.0\n");
			return 0;
		}
		else if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
			printf("\nFormA 1.0.0\n\n");
			printf("Options\n");
			printf("----------------\n");
			printf("-v, --version        print version information\n");
			printf("-h, --help           print help text\n\n");
			return 0;
		}
		else if(!strcmp(argv[1], "-d") || !strcmp(argv[1], "--debug")) {
			debug = true;
		}
		else {
			printf("Unknown option: %s\n", argv[1]);
			printf("Use -h or --help to see a list of available options.\n");
			return 0;
		}
	}

	// Load game, make initial state
	State st;
	if(!loadGame(&st)) {
		fprintf(stderr, "Error: Initialization Failed\n");
		return 1;
	}

	// Game loop
	bool quit = false;
	while(!quit) {
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
		if(debug) ms_per_frame = 3000.0 / MAX_FPS;
		int sleep_time = ms_per_frame - (SDL_GetTicks() - start_time);
		if(sleep_time > 0) SDL_Delay(sleep_time);
	}

	// Free all resources and exit game
	printf("Final score: %llu\n", st.score);
	quitGame(&st);
	return 0;
}
