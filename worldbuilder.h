#include <stdio.h>
#include <raylib.h>
#define CHAR_LIMIT 1024

typedef enum
{
	EDIT = 0,
	SPAWN = 1,
	FREE = 2,
} EditState;

typedef enum 
{
	UNDF = -1,
	WALL = 0,
	FLOOR = 1,
	DOOR = 2,
	HEALTH_BUFF = 3,
	DAMAGE_BUFF = 4,
	INTERACTABLE = 5,
} TileType;

typedef enum 
{
	QUIT = -1,
	ALIVE = 0, 
	DEAD = 1, 
} Status;

typedef struct
{
	int frames;
	int count;
	int speed;
	int xfp;
	int yfp;
} Animation;

typedef struct
{
	Vector2 src;
	Vector2 sp;
	Texture2D tx;
	TileType tt;
	char fp[CHAR_LIMIT];
	bool animated;
	Animation animtaion;
} Tile;

typedef struct
{
	double startTime;
	double lifeTime;
} Timer;

typedef struct
{
	char* name;
	float health;
	float speed;
	Vector2 pos;
	bool adjsp;
	bool move;
	Texture2D tx;
	Animation animation;
	int id;
	int damage;
	Timer attack_speed;
	Timer heal_speed;
	float exp;
	int level;
	float angle;
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
	char fp[CHAR_LIMIT];
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
	TileList walls;
	TileList doors;
	TileList floors;
	TileList health_buffs;
	TileList damage_buffs;
	TileList interactables;
	EntityList entities;
	Textures textures;
	Rectangle area;
	Vector2 spawn;
} World;

const size_t TILE_CAP = sizeof(Tile);
const size_t ENTITY_CAP = sizeof(Entity);

const int TILE_SIZE = 16;
const float SCALE = 2.0f;
const int SCREEN_TILE_SIZE = TILE_SIZE * SCALE;
