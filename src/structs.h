static int score = 0;

struct Coordinate {
	int x;
	int y;
};

struct Plane {
	struct Coordinate coord;
};

static struct Plane plane;

struct Meteor {
	struct Coordinate coord;
};
