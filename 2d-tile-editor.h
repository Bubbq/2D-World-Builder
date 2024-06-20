#include <stdio.h>
#include <raylib.h>

enum Element
{
	WALL = 0,
	FLOOR = 1,
	DOOR = 2,
	HEALTH_BUFF = 3,
	DAMAGE_BUFF = 4,
	INTERACTABLE = 5,
	SPAWN = 6,
};

typedef enum
{
	PLAYER = 0,
	ENEMY = 1,
} EntityType;

typedef enum 
{
	QUIT = -1,
	ALIVE = 0, 
	DEAD = 1, 
} Status;

typedef struct
{
	Vector2 src;
	Vector2 sp;
	Texture2D tx;
	enum Element tt;
	char fp[512];
	bool active;
	bool anim;
	int fc;
	int frames;
	int anim_speed;
} Tile;

typedef struct
{
	double startTime;
	double lifeTime;
} Timer;

typedef struct
{
	char* name;
	int frame_count;
	float health;
	float speed;
	Vector2 pos;
	bool alive;
	// flag for diagonal speed adjustment
	bool adjsp;
	bool move;
	Texture2D tx;
	int anim_speed;
	int id;
	Timer timer;
	Timer moveTimer;
	Vector2 path;
	int xfp;
	int yfp;
	float exp;
	int level;
	float angle;
	float dx;
	float dy;
} Entity;

typedef struct
{
	int size;
	size_t cap;
	Tile* list;
} TileList;

typedef struct
{
	Texture2D tx;
	char fp[512];
} BetterTexture;

typedef struct
{
	BetterTexture better_textures[25];
	int size;
} Textures;

typedef struct
{
	int size;
	size_t cap;
	Entity* entities;
} EntityList;

typedef struct
{
	EntityList entities;
	TileList walls;
	TileList floors;
	TileList doors;
	TileList health_buffs;
	TileList damage_buffs;
	TileList interactables;
	Textures textures;
	Vector2 spawn;
	Rectangle area;
} World;

const size_t TILE_CAP = sizeof(Tile);
const size_t ENTITY_CAP = sizeof(Entity);

const int TILE_SIZE = 16;
const float SCALE = 2.0f;
const int SCREEN_TILE_SIZE = TILE_SIZE * SCALE;
