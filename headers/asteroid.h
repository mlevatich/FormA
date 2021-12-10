#ifndef ASTEROID
#define ASTEROID

#include "forma.h"

struct SpriteList
{
	struct SpriteList* prev;
	struct SpriteList* next;
	Sprite* sprite;
};
typedef struct SpriteList SpriteList;

#endif // ASTEROID
