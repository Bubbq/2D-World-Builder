#include "tile_generation.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

const int SCREEN_WIDTH = 992;
const int SCREEN_HEIGHT = 992;

// update path to world you have previously saved
const char* WORLD_PATH = "world.txt";
const char* SPAWN_PATH = "spawn.txt";
const char* PLAYER_PATH = "Assets/player.png";

const int ANIMATION_SPEED = 20;
const int FPS = 60;

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
			break;
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

        Tile tile = (Tile){src, sp, addTexture(&world->textures, fp), tt, "NULL", true};
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
        if(layer->list[i].tx.id > 0 && layer->list[i].active)
        {
            DrawTexturePro(layer->list[i].tx, (Rectangle){layer->list[i].src.x, layer->list[i].src.y, TILE_SIZE, TILE_SIZE},
													 (Rectangle){layer->list[i].sp.x, layer->list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, (Vector2){0,0}, 0, WHITE);
        }
    }
}

void init(World* world, Entity* player)
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
	world->textures.size = 0;

	world->spawn_rate = 60;
	
	world->entities.entities = malloc(ENTITY_CAP);
	world->walls.list = malloc(TILE_CAP);
	world->floors.list = malloc(TILE_CAP);
	world->doors.list = malloc(TILE_CAP);
	world->health_buffs.list = malloc(TILE_CAP);
	world->damage_buffs.list = malloc(TILE_CAP);
	world->interactables.list = malloc(TILE_CAP);

	world->entities.cap = ENTITY_CAP;
    world->interactables.cap = TILE_CAP;
    world->walls.cap = TILE_CAP;
    world->floors.cap = TILE_CAP;
    world->doors.cap = TILE_CAP;
    world->health_buffs.cap = TILE_CAP;
    world->damage_buffs.cap = TILE_CAP;

    loadLayers(world, WORLD_PATH);
	
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

	player->name = "player";
	player->frame_count = 0;
	player->health = 100;
	player->speed = 5;
	player->pos = (Vector2){world->spawn.x, world->spawn.y};
	player->alive = true;
	player->adjsp = false;
	player->move = false;
	player->tx = addTexture(&world->textures, PLAYER_PATH);
	
	player->camera.target = player->pos;
	player->camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
	player->camera.zoom = 1.75f;

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

	for(int i = 0; i < world->textures.size; i++)
	{
		UnloadTexture(world->textures.better_textures[i].tx);
	}

	CloseWindow();
}

enum Direction updateCoordinate(World* world, Entity* en, Entity* player)
{
	switch (en->dir)
	{
		case UP_DOWN:
			if((int)en->pos.y < (int)player->pos.y)
			{
				en->yfp = 0;
				en->pos.y += en->speed;
				for(int i = 0; i < world->walls.size; i++)
				{
					if(CheckCollisionRecs((Rectangle){en->pos.x, en->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, 
					(Rectangle){world->walls.list[i].sp.x,world->walls.list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}) && world->entities.entities[i].id != en->id)
					{
						en->pos.y -= en->speed;
						return LEFT_RIGHT;
					}
				}
			}
			if((int)en->pos.y > (int)player->pos.y)
			{
				en->yfp = 1;
				en->pos.y -= en->speed;
				for(int i = 0; i < world->walls.size; i++)
				{
					if(CheckCollisionRecs((Rectangle){en->pos.x, en->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, 
					(Rectangle){world->walls.list[i].sp.x,world->walls.list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}) && world->entities.entities[i].id != en->id)
					{
						en->pos.y += en->speed;
						return LEFT_RIGHT;
					}
				}
			}
			if((int)en->pos.y == (int)player->pos.y)
			{
				return LEFT_RIGHT;
			}
			else
			{
				return UP_DOWN;
			}
			break;
		case LEFT_RIGHT:
			if((int)en->pos.x < (int)player->pos.x)
			{
				en->yfp = 2;
				en->pos.x += en->speed;
				for(int i = 0; i < world->walls.size; i++)
				{
					if(CheckCollisionRecs((Rectangle){en->pos.x, en->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, 
					(Rectangle){world->walls.list[i].sp.x,world->walls.list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}) && world->entities.entities[i].id != en->id)
					{
						en->pos.x -= en->speed;
						return UP_DOWN;
					}
				}
			}
			if((int)en->pos.x > (int)player->pos.x)
			{
				en->yfp = 3;
				en->pos.x -= en->speed;
				for(int i = 0; i < world->walls.size; i++)
				{
					if(CheckCollisionRecs((Rectangle){en->pos.x, en->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, 
					(Rectangle){world->walls.list[i].sp.x,world->walls.list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}) && world->entities.entities[i].id != en->id)
					{
						en->pos.x += en->speed;
						return UP_DOWN;
					}
				}
			}
			if((int)en->pos.x == (int)player->pos.x)
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

int entityCollisionWorld(Entity* en, TileList* layer)
{
	for(int i = 0; i < layer->size; i++)
	{
		if(CheckCollisionRecs((Rectangle){en->pos.x, en->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE},
							(Rectangle){layer->list[i].sp.x, layer->list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}) && layer->list[i].active)
		{
			return i;
		}
	}

	return -1;
}

int entityCollisionEntity(Entity* en, Entities* ents)
{
	for(int i = 0; i < ents->size; i++)
	{
		if(CheckCollisionRecs((Rectangle){en->pos.x, en->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE},
							(Rectangle){ents->entities[i].pos.x, ents->entities[i].pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}) && ents->entities[i].id != en->id)
		{
			return i;
		}
	}

	return -1;
}

void updateEntities(Entities* world_entities, Entity* player, World* world)
{
	int wall_col, en_col = -1;

	for(int i = 0; i < world_entities->size; i++)
	{
		Entity* en = &world_entities->entities[i];
		en->frame_count++;

		en->xfp = (en->frame_count / ANIMATION_SPEED);
		if(en->xfp > 3)
		{
			en->frame_count = 0;
			en->xfp = 0;
		}

		// if(TimerDone(en->timer))
		// {
		// 	StartTimer(&en->timer, 1.25);
		// 	en->dir = (enum Direction)GetRandomValue(1, 2);
		// }

		// updating the direction of the enemy
		// en->dir = updateCoordinate(world, en, player);

		if(en->pos.x < player->pos.x)
		{
			en->pos.x += en->speed;
			wall_col = entityCollisionWorld(en, &world->walls);
			en_col = entityCollisionEntity(en, world_entities);
			
			if(wall_col >= 0)
			{
				en->pos.x -= en->speed;
			}

			if(en_col >= 0)
			{
				en->pos.x -=en->speed;
			}
		}
		
		if(en->pos.x > player->pos.x)
		{
			en->pos.x -= en->speed;
			wall_col = entityCollisionWorld(en, &world->walls);
			en_col = entityCollisionEntity(en, world_entities);
			
			if(wall_col >= 0)
			{
				en->pos.x += en->speed;
			}

			if(en_col >= 0)
			{
				en->pos.x +=en->speed;
			}
		}

		if(en->pos.y < player->pos.y)
		{
			en->pos.y += en->speed;
			wall_col = entityCollisionWorld(en, &world->walls);
			en_col = entityCollisionEntity(en, world_entities);

			if(wall_col >= 0)
			{
				en->pos.y -= en->speed;
			}
			
			if(en_col >= 0)
			{
				en->pos.y -=en->speed;
			}
		}

		if(en->pos.y > player->pos.y)
		{
			en->pos.y -= en->speed;
			wall_col = entityCollisionWorld(en, &world->walls);
			en_col = entityCollisionEntity(en, world_entities);

			if(wall_col >= 0)
			{
				en->pos.y += en->speed;
			}

			if(en_col >= 0)
			{
				en->pos.y +=en->speed;
			}
		}

		// drawing rectangle showing their health
		DrawRectangle(en->pos.x + 5, en->pos.y - 7, 20 * (en->health / 100), 7, RED);
		DrawRectangleLines(en->pos.x + 5, en->pos.y - 7, 20, 7, BLACK);

		// only draw entity when in bounds
		if(CheckCollisionPointRec((Vector2){en->pos.x, en->pos.y}, world->area))
		{
			DrawTexturePro(en->tx, (Rectangle){en->xfp * TILE_SIZE, en->yfp * TILE_SIZE, TILE_SIZE, TILE_SIZE},
										(Rectangle){en->pos.x, en->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, (Vector2){0,0}, 0, WHITE);
		}
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

	// flag for diagonal movement
    bool diagonal = (IsKeyDown(KEY_W) && IsKeyDown(KEY_D)) || 
                      (IsKeyDown(KEY_W) && IsKeyDown(KEY_A)) || 
                      (IsKeyDown(KEY_A) && IsKeyDown(KEY_S)) || 
                      (IsKeyDown(KEY_S) && IsKeyDown(KEY_D));

    if (diagonal)
    {
        if (!player->adjsp)
        {
			// through pythag thm: 2x^2 - player's speed^2 = 0, solve for x to find diag speed movement to prevent moving faster diagonly
            player->speed = sqrt(4 * (2) * (pow(player->speed, 2))) / 4;
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
		if(entityCollisionWorld(player, &world->walls) > 0)
		{
			player->pos.y += player->speed;
		}
	}

	if(IsKeyDown(KEY_A))
	{
		player->pos.x -= player->speed;
		player->yfp = 3;
		player->move = true;
		if(entityCollisionWorld(player, &world->walls) > 0)
		{
			player->pos.x += player->speed;
		}
	}

	if(IsKeyDown(KEY_S))
	{
		player->pos.y += player->speed;
		player->yfp = 0;
		player->move = true;
		if(entityCollisionWorld(player, &world->walls) > 0)
		{
			player->pos.y -= player->speed;
		}
	}

	if(IsKeyDown(KEY_D))
	{
		player->pos.x += player->speed;
		player->yfp = 2;
		player->move = true;
		if(entityCollisionWorld(player, &world->walls) > 0)
		{
			player->pos.x -= player->speed;
		}
	}

	// setting afk position
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
	Entity player;
	Vector2 mp;
	init(&world, &player);

	while(!WindowShouldClose())
    {
		mp = GetScreenToWorld2D(GetMousePosition(), player.camera);
		player.camera.target = player.pos;
		world.area = (Rectangle){player.camera.target.x - player.camera.offset.x, player.camera.target.y - player.camera.offset.y,SCREEN_WIDTH,SCREEN_HEIGHT};
	   
	    BeginDrawing();
            ClearBackground(WHITE);
			BeginMode2D(player.camera);
				drawLayer(&world.floors);
				drawLayer(&world.health_buffs);
				drawLayer(&world.damage_buffs);
				drawLayer(&world.doors);
				drawLayer(&world.interactables);
				drawLayer(&world.walls);
				updatePlayer(&world, &player);
				updateEntities(&world.entities, &player, &world);
			EndMode2D();
			
			// --------------------------------------- HEALTH MECHANICS----------------------------------------//
			// using health buff
			int h_col =  entityCollisionWorld(&player, &world.health_buffs);
			if(h_col >= 0)
			{
				DrawText("PRESSING H WILL HEAL", GetScreenWidth() - 310, GetScreenHeight() - 110, 20, GREEN);
				if(IsKeyPressed(KEY_H) && player.health < 100)
				{
					player.health = player.health + 20 > 100 ? 100 : player.health + 20;
					world.health_buffs.list[h_col].active = false;
					// removeTile(&world.health_buffs, h_col);
				}
			}

			// drawing the player's health bar
			DrawRectangle(GetScreenWidth() - 310,GetScreenHeight() - 60, (300) * (player.health / 100),50, RED);
			DrawRectangleLines(GetScreenWidth() - 310, GetScreenHeight() - 60, 300,50, BLACK);
			DrawText("HEALTH", GetScreenWidth() - 300, GetScreenHeight() - 80, 20, RED);
			// --------------------------------------- HEALTH MECHANICS----------------------------------------//

			// --------------------------------------- FIGHT MECHANICS----------------------------------------//
			int mob_col = entityCollisionEntity(&player, &world.entities);
			if(mob_col >= 0)
			{
				Entity* fight_en = &world.entities.entities[mob_col];
				
				// small window of invincibility after getting hit
				if(TimerDone(player.timer))
				{
					StartTimer(&player.timer, 1.5);
					player.health = (player.health - 20 < 0) ? 0 : player.health - 20;
					// when you die
					if(player.health == 0)
					{}
				}

				// doing damage to mob
				if(IsKeyPressed(KEY_E))
				{
					fight_en->health = fight_en->health - 20 > 0 ? fight_en->health - 20 : 0;
					// killing an enemy
					if(fight_en->health == 0)
					{
						fight_en->alive = false;
						fight_en->pos = (Vector2){INT_MIN, INT_MIN};
						
						// gaining experience/leveling up
						player.exp += 20;
						if(player.exp >= 100)
						{
							player.exp = 0;
							player.level++;
						}
					}
				}
			}
			
			// drawing and updating players level
			DrawRectangle(10, GetScreenHeight() - 60, (300) * (player.exp / 100), 50, GREEN);
			DrawRectangleLines(10, GetScreenHeight() - 60, 300, 50, BLACK);
			
			char lvl_dsc[100]; sprintf(lvl_dsc, "LEVEL: %d", player.level);
			DrawText(lvl_dsc, 10, GetScreenHeight() - 80, 20, GREEN);
			// --------------------------------------- FIGHT MECHANICS----------------------------------------//

			// --------------------------------------- MOB MECHANICS----------------------------------------//
			// creation of dummy enemies
			if(CheckCollisionPointRec(mp, world.area) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
			{
				// cant spawn mob in wall
				bool valid_spawn = true;
				for(int i = 0; i < world.walls.size; i++)
				{
					if(CheckCollisionRecs((Rectangle){mp.x, mp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE},
										(Rectangle){world.walls.list[i].sp.x, world.walls.list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}))
					{
						valid_spawn = false;
					}
				}

				if(valid_spawn)
				{
					Entity new_en;
					new_en.name = "mob";
					new_en.health = 100;
					new_en.speed = 1;
					new_en.pos = mp;
					new_en.alive = true;
					new_en.tx = addTexture(&world.textures, PLAYER_PATH);
					new_en.id = GetRandomValue(0, INT_MAX);
					addEntity(&world.entities, new_en);
				}
			}
			// --------------------------------------- MOB MECHANICS----------------------------------------//

		DrawFPS(0, 0);			
		EndDrawing();
    }

    deinit(&world);
    return 0;
}