#ifndef CONSTANTS
#define CONSTANTS

#include <stdlib.h>
#include <math.h>

// Screen size
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

// Frame rate
#define MAX_FPS 60

// Pi
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif // M_PI
#ifndef M_PI_2
	#define M_PI_2 (M_PI / 2)
#endif // M_PI_2

// Audio-related constants
#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2
#define CHUNK_SIZE 2048
#define NUM_SFX 3

// List of sound effects
enum sound_effects
{ SFX_LASER, SFX_CRASH, SFX_THRUST };

// Mute the game's audio
void setMute(void);

// Start the game's main theme
void startMusic(void);

// Play a sound effect
void playSoundEffect(int sfx_id);

// Load audio elements
void loadSound(void);

// Free audio elements
void freeSound(void);


// Sprite ids
enum sprite_ids
{ ASTER, FRAGMENT, LASER, SHIP };

// Get random number in [0, 1)
static inline double getRand() { return (double) rand() / (double) RAND_MAX; }

// Max and min
static inline double max(double a, double b) { if(a > b) return a; return b; }
static inline double min(double a, double b) { if(a < b) return a; return b; }

#endif // CONSTANTS
