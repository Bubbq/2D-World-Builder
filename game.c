#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <raylib.h>
#include <stdlib.h>
#include <string.h>

const int SCREEN_WIDTH = 992;
const int SCREEN_HEIGHT = 992;
const int PANEL_HEIGHT = 24;
const int TILE_SIZE = 16;
const float SCALE = 2.0f;
const int SCREEN_TILE_SIZE = TILE_SIZE * SCALE;

// update path to world you have previously saved
const char* WORLD_PATH = "world.txt";
const char* SPAWN_PATH = "spawn.txt";
const char* PLAYER_PATH = "Assets/player.png";

const int ANIMATION_SPEED = 20;

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

typedef struct
{
	Vector2 src;
	Vector2 sp;
	Texture2D tx;
	enum Element tt;
	char fp[512];
} Tile;

const int INIT_CAP = (25 * sizeof(Tile));
const int TEXTURE_CAP = 25;

typedef struct
{
	int size;
	size_t cap;
	Tile* list;
} TileList;

typedef struct
{
	Texture2D txts[25];
	int size;
	size_t cap;
} Textures;

typedef struct
{
	const char* name;
	int frame_count;
	float health;
	float speed;
	Vector2 pos;
	bool alive;
	// flag for diagonal speed adjustment
	bool adjsp;
	bool move;
	Texture2D tx;
} Entity;

typedef struct
{
	Entity* entities;
	int size;
	size_t cap;
} Entities;

typedef struct
{
	Entities entities;
	TileList walls;
	TileList floors;
	TileList doors;
	TileList health_buffs;
	TileList damage_buffs;
	TileList interactables;
	Vector2 spawn;
} World;

void resizeEntities(Entities* world_entities)
{
	world_entities->cap *= 2;
	world_entities->entities = realloc(world_entities->entities, world_entities->cap);
	if(world_entities->entities == NULL)
	{
		printf("ERROR RESZING ENTITIES \n");
		exit(1);
	}
}

void addEntity(Entities* world_entities, Entity entity)
{
	if(world_entities->size * sizeof(Entity) == world_entities->cap)
	{
		printf("need to resize \n");
		resizeEntities(world_entities);
		printf("resized sucessfully \n");
	}

	world_entities->entities[world_entities->size++] = entity;
}

void removeEntity(Entities* world_entities, int pos)
{
	if(world_entities->size == 0)
	{
		return;
	}

	for(int i = 0; i < world_entities->size; i++)
	{
		world_entities->entities[i] = world_entities->entities[i + 1];
	}

	world_entities->size--;

	if(world_entities->size == 0)
	{
		free(world_entities->entities);
		world_entities->size = 0;
		world_entities->entities = malloc(3 * sizeof(Entity));
		world_entities->cap = 3 * sizeof(Entity);
	}
}

void resizeLayer(TileList* layer)
{
    layer->cap *= 2;
    layer->list = realloc(layer->list, layer->cap);
    
    if(layer == NULL)
    {
        printf("ERROR RESIZING \n");
        exit(1);
    }
}

void addTile(TileList* layer, Tile tile)
{
    if(layer->size * sizeof(Tile) == layer->cap)
    {
        resizeLayer(layer);
    }

    layer->list[layer->size++] = tile;
}

void removeTile(TileList* layer, int pos)
{
	if(layer->size == 0)
	{
		return;
	}

	for(int i = pos; i < layer->size; i++)
	{
		layer->list[i] = layer->list[i + 1];
	}

	layer->size--;

	if(layer->size == 0)
	{
		free(layer->list);
		layer->size = 0;
		layer->list = malloc(INIT_CAP);
		layer->cap = INIT_CAP;
	}
}

void loadLayers(World* world, Textures* textures, const char* filePath)
{
    FILE* inFile = fopen(filePath, "r");
    char line[512];
    if(inFile == NULL)
    {
        printf("ERROR LOADING WORLD \n");
        return;
    }

    Vector2 src;
    Vector2 sp;
    enum Element tt;
    char* fp;
	char lfp[512];

    while(fgets(line, sizeof(line), inFile))
    {
        src.x = atoi(strtok(line, ","));
        src.y = atoi(strtok(NULL, ","));
        sp.x = atoi(strtok(NULL, ","));
        sp.y = atoi(strtok(NULL, ","));
        tt = (enum Element)atoi(strtok(NULL, ","));
        fp = strtok(NULL, ",");

		if(strcmp(lfp, fp) != 0)
		{
			textures->txts[textures->size++] = LoadTexture(fp);
			strcpy(lfp, fp);
		}

        Tile tile = (Tile){src, sp, textures->txts[textures->size - 1], tt, "NULL"};
        strcpy(tile.fp, fp);

        switch (tt)
		{
			case WALL:
				addTile(&world->walls, tile);
				break;
			case FLOOR:
				addTile(&world->floors, tile);
				break;
			case DOOR:
				addTile(&world->doors, tile);
				break;
			case HEALTH_BUFF:
				addTile(&world->health_buffs, tile);
				break;
			case DAMAGE_BUFF:
				addTile(&world->damage_buffs, tile);
				break;
			case INTERACTABLE:
				addTile(&world->interactables, tile);
				break;
			default:
				break;
		}
    }

    fclose(inFile);
}

void drawLayer(TileList* layer)
{
    for(int i = 0; i < layer->size; i++)
    {
        if(layer->list[i].tx.id != 0)
        {
            DrawTexturePro(layer->list[i].tx, (Rectangle){layer->list[i].src.x, layer->list[i].src.y, TILE_SIZE, TILE_SIZE}, (Rectangle){layer->list[i].sp.x, layer->list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, (Vector2){0,0}, 0, WHITE);
        }
    }
}

void init(World* world, Textures* textures)
{
	SetTraceLogLevel(LOG_ERROR);
	InitWindow(SCREEN_WIDTH,SCREEN_HEIGHT, "RPG-GAME");
	SetTargetFPS(60);

	// getting spawn point
	FILE* inFile = fopen(SPAWN_PATH, "r");
	if(inFile == NULL)
	{
		printf("NO SPAWN POINT SAVED \n");
		world->spawn = (Vector2){0, 0};
	}

	else
	{
		char line[512];
		while(fgets(line, sizeof(line), inFile))
		{
			world->spawn.x = atoi(strtok(line, " "));
			world->spawn.y = atoi(strtok(NULL, "\n"));
		}
	}

	fclose(inFile);

	textures->size = 0;

	world->walls.size = 0;
	world->floors.size = 0;
	world->doors.size = 0;
	world->health_buffs.size = 0;
	world->damage_buffs.size = 0;
	world->interactables.size = 0;
	world->entities.size = 0;
	
	world->entities.entities = malloc(3 * sizeof(Entity));
	world->walls.list = malloc(INIT_CAP);
	world->floors.list = malloc(INIT_CAP);
	world->doors.list = malloc(INIT_CAP);
	world->health_buffs.list = malloc(INIT_CAP);
	world->damage_buffs.list = malloc(INIT_CAP);
	world->interactables.list = malloc(INIT_CAP);

	world->entities.cap = 3 * sizeof(Entity);
    world->interactables.cap = (INIT_CAP);
    world->walls.cap = (INIT_CAP);
    world->floors.cap = (INIT_CAP);
    world->doors.cap = (INIT_CAP);
    world->health_buffs.cap = (INIT_CAP);
    world->damage_buffs.cap = (INIT_CAP);
}

void deinit(World *world, Textures* textures)
{
	free(world->walls.list);
	free(world->floors.list);
	free(world->doors.list);
	free(world->health_buffs.list);
	free(world->damage_buffs.list);
	free(world->interactables.list);
	free(world->entities.entities);

	for(int i = 0; i < TEXTURE_CAP; i++)
	{
		if(textures->txts[i].id != 0)
		{
			UnloadTexture(textures->txts[i]);
		}
	}

	CloseWindow();
}

void updatePlayer(World* world, Entity* player)
{
	// animation
	int xfp = 0;
	int yfp = 0;
	player->frame_count++;

	xfp = (player->frame_count / ANIMATION_SPEED);
	if(xfp > 3)
	{
		xfp = 0;
		player->frame_count = 0;
	}

	// movement
    bool diagonal = (IsKeyDown(KEY_W) && IsKeyDown(KEY_D)) || 
                      (IsKeyDown(KEY_W) && IsKeyDown(KEY_A)) || 
                      (IsKeyDown(KEY_A) && IsKeyDown(KEY_S)) || 
                      (IsKeyDown(KEY_S) && IsKeyDown(KEY_D));

    if (diagonal)
    {
        if (!player->adjsp)
        {
			// pythag thm: 2x^2 - player's_peed^2 = 0, solve for x to find diag speed movement to prevent moving faster diagonly
            float ns = sqrt(4 * (2) * (pow(player->speed, 2))) / 4;
            player->speed = ns;
			player->adjsp = true;
        }
    } 
				
	else
    {
        player->speed = 3;
		player->adjsp = false;
    }

	if(IsKeyDown(KEY_W))
	{
		player->pos.y -= player->speed;
		yfp = 1;
		player->move = true;
		for(int i = 0; i < world->walls.size; i++)
		{
			if(CheckCollisionRecs((Rectangle){player->pos.x, player->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE},
								 (Rectangle){world->walls.list[i].sp.x, world->walls.list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}))
			{
				player->pos.y += player->speed;
			}
		}
	}

	if(IsKeyDown(KEY_A))
	{
		player->pos.x -= player->speed;
		yfp = 3;
		player->move = true;
		for(int i = 0; i < world->walls.size; i++)
		{
			if(CheckCollisionRecs((Rectangle){player->pos.x, player->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE},
								 (Rectangle){world->walls.list[i].sp.x, world->walls.list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}))
			{
				player->pos.x += player->speed;
			}
		}
	}

	if(IsKeyDown(KEY_S))
	{
		player->pos.y += player->speed;
		yfp = 0;
		player->move = true;
		for(int i = 0; i < world->walls.size; i++)
		{
			if(CheckCollisionRecs((Rectangle){player->pos.x, player->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE},
								 (Rectangle){world->walls.list[i].sp.x, world->walls.list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}))
			{
				player->pos.y -= player->speed;
			}
		}
	}

	if(IsKeyDown(KEY_D))
	{
		player->pos.x += player->speed;
		yfp = 2;
		player->move = true;
		for(int i = 0; i < world->walls.size; i++)
		{
			if(CheckCollisionRecs((Rectangle){player->pos.x, player->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE},
								 (Rectangle){world->walls.list[i].sp.x, world->walls.list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}))
			{
				player->pos.x -= player->speed;
			}
		}
	}

	// when afk, have sprite look forward facing user
	if(!player->move)
	{
		xfp = yfp = 0;
	}

	// drawing player
	DrawTexturePro(player->tx, (Rectangle){xfp * TILE_SIZE, yfp * TILE_SIZE, TILE_SIZE, TILE_SIZE},
										 (Rectangle){player->pos.x, player->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, (Vector2){0,0}, 0, WHITE);
	
	// resetting movement flag
	player->move = false;
}

int main()
{
    World world;
	Textures textures;
	init(&world, &textures);
    loadLayers(&world, &textures, WORLD_PATH);
	Entity player = (Entity){"player", 0, 100, 5, (Vector2){world.spawn.x, world.spawn.y}, true, false, false, LoadTexture(PLAYER_PATH)};
	
	Camera2D camera = {0};
	camera.target = player.pos;
	camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
	camera.zoom = 1.75f;

	while(!WindowShouldClose())
    {
		camera.target = player.pos;
		camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

	    BeginDrawing();

            ClearBackground(WHITE);

			BeginMode2D(camera);
				drawLayer(&world.floors);
				drawLayer(&world.health_buffs);
				drawLayer(&world.damage_buffs);
				drawLayer(&world.doors);
				drawLayer(&world.interactables);
				drawLayer(&world.walls);
				updatePlayer(&world, &player);
			EndMode2D();
			
			// --------------------------------------- SIMULATING GETTING HIT----------------------------------------//
			for(int i = 0; i < world.health_buffs.size; i++)
			{
				if(CheckCollisionRecs((Rectangle){player.pos.x, player.pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, 
									(Rectangle){world.health_buffs.list[i].sp.x, world.health_buffs.list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}))
				{
					if(IsKeyPressed(KEY_E) && player.health < 100)
					{
						player.health = player.health + 20 > 100 ? 100 : player.health + 20;
						removeTile(&world.health_buffs, i);
					}
				}
			}

			if(IsKeyPressed(KEY_L))
			{
				player.health = player.health - 20 < 0 ? 0 : player.health - 20;
			}

			// players health bar
			DrawRectangle(GetScreenWidth() - 310,GetScreenHeight() - 60, (300) * (player.health / 100),50, RED);
			DrawRectangleLines(GetScreenWidth() - 310, GetScreenHeight() - 60, 300,50, BLACK);
			DrawText("HEALTH", GetScreenWidth() - 300, GetScreenHeight() - 80, 20, RED);
			DrawFPS(0, 0);
			// --------------------------------------- SIMULATING GETTING HIT----------------------------------------//

        EndDrawing();
    }

    deinit(&world, &textures);
    return 0;
}