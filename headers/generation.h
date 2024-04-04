#include <raylib.h>
#include <string>
#include <raylib.h>
#include <vector>

const int TILE_SIZE = 16;
const float SCALE = 2.0f;
const int SCREEN_TILE_SIZE = TILE_SIZE * SCALE;
static int DISPLAY_TILE_SIZE = SCREEN_TILE_SIZE;

enum Element{
	WALL = 0,
	FLOOR = 1,
	DOOR = 2,
	BUFF = 3,
	INTERACTABLE = 4,
	SPAWN = 5,
};

struct Tile
{
	Vector2 src;
	Vector2 sp;
	Texture2D tx;
	std::string fp;
	Element tt;
};

struct World
{
std::vector<Tile> walls;
std::vector<Tile> floors;
std::vector<Tile> doors;
std::vector<Tile> buffs;
std::vector<Tile> interactables;
};

void saveLayer(std::vector<Tile>& layer, std::string filePath);
void loadLayers(std::string filePath, Rectangle& worldArea, World& world, Vector2 spawn);
void drawLayer(std::vector<Tile>& layer, Color color, Rectangle worldArea, const char * dsc, bool showDsc);
void clearWorld(World& world);