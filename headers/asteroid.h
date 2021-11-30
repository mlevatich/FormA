#ifndef ASTEROID
#define ASTEROID

#include "forma.h"

struct Asteroid
{
	struct Asteroid* prev;
	struct Asteroid* next;
	Sprite* sprite;
};
typedef struct Asteroid Asteroid;

#endif // ASTEROID
