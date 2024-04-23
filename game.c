#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <raylib.h>
#include<raymath.h>
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
const int FPS = 60;

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

enum Direction
{
	LEFT_RIGHT = 1,
	UP_DOWN = 2,
	DIAGONAL = 3,
};

typedef struct
{
	Vector2 src;
	Vector2 sp;
	Texture2D tx;
	enum Element tt;
	char fp[512];
} Tile;

typedef struct
{
	double startTime;
	double lifeTime;
} Timer;

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
	enum Direction dir;
	Timer timer;
	int xfp;
	int yfp;
 	// Timer direction_timer;
} Entity;

const size_t INIT_TILE_CAP = (25 * sizeof(Tile));
const size_t INIT_ENTITY_CAP = (3 * sizeof(Entity));
const int TEXTURE_CAP = 25;

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
	Textures textures;
	Vector2 spawn;
	int spawn_rate;
} World;


Timer timer;

void StartTimer(Timer *timer, double lifetime)
{
	timer->startTime = GetTime();
	timer->lifeTime = lifetime;
}

bool TimerDone(Timer timer)
{
	return GetTime() - timer.startTime >= timer.lifeTime;
}

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
		resizeEntities(world_entities);
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
		world_entities->entities = malloc(INIT_ENTITY_CAP);
		world_entities->cap = INIT_ENTITY_CAP;
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
		layer->list = malloc(INIT_TILE_CAP);
		layer->cap = INIT_TILE_CAP;
	}
}

// returns the texture from a list of textures specified by filepath
Texture2D addTexture(Textures* textures, const char* filePath)
{
	int ctxi = -1;
	bool unique = true;
	for(int i = 0; i < textures->size; i++)
	{
		// if we alr have that texture, load it from the ith position
		if(strcmp(textures->better_textures[i].fp, filePath) == 0)
		{
			unique = false;
			ctxi = i;
		}
	}

	if(unique)
	{
		textures->better_textures[textures->size] = (BetterTexture){LoadTexture(filePath), "NULL"};
		strcpy(textures->better_textures[textures->size].fp, filePath);
		textures->size++;
	}

	// if we already have this tile, use it at that index, if not, the newest texture should hold the correct one to load
	ctxi = (ctxi == -1) ? (textures->size - 1) : ctxi;

	// return the texture of new or recurring element
	return textures->better_textures[ctxi].tx;
}

void loadLayers(World* world, const char* filePath)
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
	// the position of the texture in Textures array to load for a tile

    while(fgets(line, sizeof(line), inFile))
    {
        src.x = atoi(strtok(line, ","));
        src.y = atoi(strtok(NULL, ","));
        sp.x = atoi(strtok(NULL, ","));
        sp.y = atoi(strtok(NULL, ","));
        tt = (enum Element)atoi(strtok(NULL, ","));
        fp = strtok(NULL, ",");

        Tile tile = (Tile){src, sp, addTexture(&world->textures, fp), tt, "NULL"};
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

void init(World* world)
{
	SetTraceLogLevel(LOG_ERROR);
	InitWindow(SCREEN_WIDTH,SCREEN_HEIGHT, "2D-Engine");
	SetTargetFPS(FPS);

	world->walls.size = 0;
	world->floors.size = 0;
	world->doors.size = 0;
	world->health_buffs.size = 0;
	world->damage_buffs.size = 0;
	world->interactables.size = 0;
	world->entities.size = 0;

	world->spawn_rate = 60;
	
	world->entities.entities = malloc(INIT_ENTITY_CAP);
	world->walls.list = malloc(INIT_TILE_CAP);
	world->floors.list = malloc(INIT_TILE_CAP);
	world->doors.list = malloc(INIT_TILE_CAP);
	world->health_buffs.list = malloc(INIT_TILE_CAP);
	world->damage_buffs.list = malloc(INIT_TILE_CAP);
	world->interactables.list = malloc(INIT_TILE_CAP);

	world->entities.cap = INIT_ENTITY_CAP;
    world->interactables.cap = (INIT_TILE_CAP);
    world->walls.cap = (INIT_TILE_CAP);
    world->floors.cap = (INIT_TILE_CAP);
    world->doors.cap = (INIT_TILE_CAP);
    world->health_buffs.cap = (INIT_TILE_CAP);
    world->damage_buffs.cap = (INIT_TILE_CAP);
	
	// getting spawn point
	FILE* inFile = fopen(SPAWN_PATH, "r");
	if(inFile == NULL)
	{
		printf("NO SPAWN POINT SAVED \n");
		world->spawn = (Vector2){0, 0};
		return;
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
}

void deinit(World *world)
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
		UnloadTexture(world->textures.better_textures[i].tx);
	}

	CloseWindow();
}

enum Direction updateCoordinate(Vector2* mp, Vector2* pp, enum Direction dir, float sp, Timer* direction_timer, int* yfp)
{
	switch (dir)
	{
		case UP_DOWN:
			if((int)mp->y < (int)pp->y)
			{
				*yfp = 0;
				mp->y += sp;
			}
			if((int)mp->y > (int)pp->y)
			{
				*yfp = 1;
				mp->y -= sp;
			}
			if((int)mp->y == (int)pp->y)
			{
				return LEFT_RIGHT;
			}
			else
			{
				return UP_DOWN;
			}
			break;
		case LEFT_RIGHT:
			if((int)mp->x < (int)pp->x)
			{
				*yfp = 2;
				mp->x += sp;
			}
			if((int)mp->x > (int)pp->x)
			{
				*yfp = 3;
				mp->x -= sp;
			}
			if((int)mp->x == (int)pp->x)
			{
				return UP_DOWN;
			}
			else
			{
				return LEFT_RIGHT;
			}
			break;
		default:
			break;
	}

	return -1;
}

void updateEntities(Entities* world_entities, Entity* player)
{
	for(int i = 0; i < world_entities->size; i++)
	{
		world_entities->entities[i].frame_count++;

		world_entities->entities[i].xfp = (world_entities->entities[i].frame_count / ANIMATION_SPEED);
		if(world_entities->entities[i].xfp > 3)
		{
			world_entities->entities[i].frame_count = 0;
			world_entities->entities[i].xfp = 0;
			// xmv = true;
		}

		if(TimerDone(world_entities->entities[i].timer))
		{
			StartTimer(&world_entities->entities[i].timer, 1.25);
			world_entities->entities[i].dir = (enum Direction)GetRandomValue(1, 2);
		}

		// updating the direction of the enemy
		world_entities->entities[i].dir = updateCoordinate(&world_entities->entities[i].pos, &player->pos, world_entities->entities[i].dir, 
											world_entities->entities[i].speed, &world_entities->entities[i].timer, &world_entities->entities[i].yfp);

		// drawing rectangle showing their health
		DrawRectangle(world_entities->entities[i].pos.x + 5, world_entities->entities[i].pos.y - 7, 20 * (world_entities->entities[i].health / 100), 7, RED);
		// health border
		DrawRectangleLines(world_entities->entities[i].pos.x + 5, world_entities->entities[i].pos.y - 7, 20, 7, BLACK);

		DrawTexturePro(world_entities->entities[i].tx, (Rectangle){world_entities->entities[i].xfp * TILE_SIZE, world_entities->entities[i].yfp * TILE_SIZE, TILE_SIZE, TILE_SIZE},
							(Rectangle){world_entities->entities[i].pos.x, world_entities->entities[i].pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, (Vector2){0,0}, 0, WHITE);
	}
}

void updatePlayer(World* world, Entity* player)
{
	// animation
	player->frame_count++;

	player->xfp = (player->frame_count / ANIMATION_SPEED);
	if(player->xfp > 3)
	{
		player->xfp = 0;
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
		player->yfp = 1;
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
		player->yfp = 3;
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
		player->yfp = 0;
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
		player->yfp = 2;
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
		player->xfp = player->yfp = 0;
	}

	// drawing player
		DrawTexturePro(player->tx, (Rectangle){player->xfp * TILE_SIZE, player->yfp * TILE_SIZE, TILE_SIZE, TILE_SIZE},
										 (Rectangle){player->pos.x, player->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, (Vector2){0,0}, 0, WHITE);
	
	// resetting movement flag
	player->move = false;
}

int main()
{
    World world;
	world.textures.size = 0;
	init(&world);
    loadLayers(&world, WORLD_PATH);
	Entity player = (Entity){"player", 0, 100, 5, (Vector2){world.spawn.x, world.spawn.y}, true, false, false, addTexture(&world.textures, PLAYER_PATH)};
	Vector2 mp;
	Rectangle world_rectangle;
	Camera2D camera = {0};
	camera.target = player.pos;
	camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
	int itr = 1;
	camera.zoom = 1.75f;

	while(!WindowShouldClose())
    {
		itr++;
		mp = GetScreenToWorld2D(GetMousePosition(), camera);
		camera.target = player.pos;
		camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

	    BeginDrawing();

            ClearBackground(WHITE);

			BeginMode2D(camera);
				world_rectangle = (Rectangle){camera.target.x - camera.offset.x, camera.target.y - camera.offset.y,SCREEN_WIDTH,SCREEN_HEIGHT};
				drawLayer(&world.floors);
				drawLayer(&world.health_buffs);
				drawLayer(&world.damage_buffs);
				drawLayer(&world.doors);
				drawLayer(&world.interactables);
				drawLayer(&world.walls);
				updateEntities(&world.entities, &player);
				updatePlayer(&world, &player);
			EndMode2D();
			
			// --------------------------------------- HEALTH MECHANICS----------------------------------------//
			// using health buff
			for(int i = 0; i < world.health_buffs.size; i++)
			{
				if(CheckCollisionRecs((Rectangle){player.pos.x, player.pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, 
									(Rectangle){world.health_buffs.list[i].sp.x, world.health_buffs.list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}))
				{
					DrawText("PRESSING H WILL HEAL", GetScreenWidth() - 310, GetScreenHeight() - 110, 20, GREEN);
					if(IsKeyPressed(KEY_H) && player.health < 100)
					{
						player.health = player.health + 20 > 100 ? 100 : player.health + 20;
						removeTile(&world.health_buffs, i);
					}
				}
			}

			// drawing the player's health bar
			DrawRectangle(GetScreenWidth() - 310,GetScreenHeight() - 60, (300) * (player.health / 100),50, RED);
			DrawRectangleLines(GetScreenWidth() - 310, GetScreenHeight() - 60, 300,50, BLACK);
			DrawText("HEALTH", GetScreenWidth() - 300, GetScreenHeight() - 80, 20, RED);
			DrawFPS(0, 0);
			// --------------------------------------- HEALTH MECHANICS----------------------------------------//

			// --------------------------------------- FIGHT MECHANICS----------------------------------------//
			// pressing E near an enemy will simulate doing damage
			for(int i = 0; i < world.entities.size; i++)
			{
				if(CheckCollisionRecs((Rectangle){player.pos.x, player.pos.y, SCREEN_TILE_SIZE,SCREEN_TILE_SIZE},
									 (Rectangle){world.entities.entities[i].pos.x, world.entities.entities[i].pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}))
				{
					// small window of invincibility after getting hit
					if(TimerDone(player.timer))
					{
						StartTimer(&player.timer, 1.5);
						player.health = (player.health - 20 < 0) ? 0 : player.health - 20;
					}

					if(IsKeyPressed(KEY_E))
					{
						world.entities.entities[i].health = world.entities.entities[i].health - 20 > 0 ? world.entities.entities[i].health - 20 : 0;
						if(world.entities.entities[i].health <= 0)
						{
							removeEntity(&world.entities, i);
						}
					}
				}
			}

			if(!TimerDone(player.timer))
			{
				if(itr % 30)
				{
					
				}
				
			}

			// --------------------------------------- MOB MECHANICS----------------------------------------//
			if(CheckCollisionPointRec(mp, world_rectangle) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				Timer ct;
				StartTimer(&ct, 1);
				addEntity(&world.entities, (Entity){"mob", 0, 100, 1, mp, true, false, false, addTexture(&world.textures, PLAYER_PATH), LEFT_RIGHT, ct, 0, 0});
			}
			// --------------------------------------- MOB MECHANICS----------------------------------------//
        
		EndDrawing();
    }

    deinit(&world);
    return 0;
}