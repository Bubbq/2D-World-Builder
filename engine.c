#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "2d-tile-editor.h"

#define CHAR_LIMIT 100

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#undef RAYGUI_IMPLEMENTATION

const int SCREEN_WIDTH = 992;
const int SCREEN_HEIGHT = 992;

// update path to world you have previously saved
const char* WORLD_PATH = "test.txt";
const char* SPAWN_PATH = "spawn.txt";
const char* PLAYER_PATH = "Assets/player.png";
const char* DEATH_PROMPT = "YOU DIED, RESPAWN?"; 

const int ANIMATION_SPEED = 20;
const int FPS = 60;

Vector2 mp;
World world;
Entity player;
Camera2D camera;
Status status = ALIVE;

void StartTimer(Timer *timer, double lifetime) // good
{
	timer->startTime = GetTime();
	timer->lifeTime = lifetime;
}

bool TimerDone(Timer timer) { return GetTime() - timer.startTime >= timer.lifeTime; } // good

void resizeEntities(EntityList* world_entities) // good
{
	world_entities->cap *= 2;
	world_entities->entities = realloc(world_entities->entities, world_entities->cap);
}

void resizeLayer(TileList* layer) // good
{
    layer->cap *= 2;
    layer->list = realloc(layer->list, layer->cap);
}

void addEntity(EntityList* world_entities, Entity entity) // good
{
	if(world_entities->size * sizeof(Entity) == world_entities->cap) resizeEntities(world_entities);
	world_entities->entities[world_entities->size++] = entity;
}

void addTile(TileList* layer, Tile tile) // good
{
    if(layer->size * sizeof(Tile) == layer->cap) resizeLayer(layer);
    layer->list[layer->size++] = tile;
}

// either adds a new texture to list of textures or returns the existing texture of the one we want
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
	bool anim;
	int fc;
	int frames;
	int anim_speed;

    while(fgets(line, sizeof(line), inFile))
    {
        src.x = atoi(strtok(line, ","));
        src.y = atoi(strtok(NULL, ","));
        sp.x = atoi(strtok(NULL, ","));
        sp.y = atoi(strtok(NULL, ","));
        tt = (enum Element)atoi(strtok(NULL, ","));
        fp = strtok(NULL, ",");
		anim = atoi(strtok(NULL, ","));
		fc = atoi(strtok(NULL, ","));
		frames = atoi(strtok(NULL, ","));
		anim_speed = atoi(strtok(NULL, "\n"));

        Tile tile = (Tile){src, sp, addTexture(&world->textures, fp), tt, "NULL",true, anim, fc, frames, anim_speed};
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

int animateTile(Tile* t) // good
{
	int frame_pos;

	t->fc++;
	frame_pos = (t->fc / t->anim_speed);
	if(frame_pos > t->frames) frame_pos = t->fc = 0;

	return frame_pos;
}

void drawLayer(TileList* layer)
{
    for(int i = 0; i < layer->size; i++)
    {
		int frame_pos = 0;
		Tile* tile = &layer->list[i];

		if(!tile->active) continue;

		// animate tiles
		if(tile->anim) frame_pos = animateTile(tile);

		DrawTexturePro(tile->tx, (Rectangle){tile->src.x + (frame_pos * TILE_SIZE), tile->src.y, TILE_SIZE, TILE_SIZE},
											(Rectangle){tile->sp.x, tile->sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, (Vector2){0,0}, 0, WHITE);
    }
}

TileList createTileList() { return (TileList){0, TILE_CAP, malloc(TILE_CAP)}; } // good

EntityList createEntityList(){ return (EntityList){0, ENTITY_CAP, malloc(ENTITY_CAP)}; } // good

void resetEntityList(EntityList* en) // good
{
	if(en->size > 0) free(en->entities);
	en->entities = malloc(ENTITY_CAP);
	en->cap = ENTITY_CAP;
	en->size = 0;
}

Vector2 getSpawnPoint() // good
{
	FILE* file = fopen(SPAWN_PATH, "r");
	
	if(file == NULL) return (Vector2){0, 0};

	Vector2 spawn;
	char info[CHAR_LIMIT];

	while(fgets(info, sizeof(info), file))
	{
		spawn.x = atof(strtok(info, ","));
		spawn.y = atof(strtok(NULL, "\n")); 
	}

	return spawn;
}	

Entity createPlayer() { return (Entity){"player", 0, 100.0f, 3.0f, getSpawnPoint(), true, false, false, LoadTexture(PLAYER_PATH), ANIMATION_SPEED}; } // good
Entity createEnemy(){ return (Entity){"mob", 0, 100, 1.0f, mp, true, false, true, addTexture(&world.textures, PLAYER_PATH), ANIMATION_SPEED, GetRandomValue(0, 1000000000)}; }

void init(World* world, Entity* player) // good
{
	// game settings
	SetTargetFPS(FPS);
	SetTraceLogLevel(LOG_ERROR);
	InitWindow(SCREEN_WIDTH,SCREEN_HEIGHT, "2D-Engine");
	
	world->textures.size = 0;

	// world inititalization
	world->walls = createTileList();
	world->floors = createTileList();
	world->doors = createTileList();
	world->health_buffs = createTileList();
	world->damage_buffs = createTileList();
	world->interactables = createTileList();
	world->entities = createEntityList();
    loadLayers(world, WORLD_PATH);

	// player instantiation
	*player = createPlayer();
	
	// camera setup
	camera.zoom = 1.0f;
	camera.target = player->pos;
	camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
}

void deinit(World *world) // good
{
	// free alloc mem from dynamic lists
	free(world->walls.list);
	free(world->floors.list);
	free(world->doors.list);
	free(world->health_buffs.list);
	free(world->damage_buffs.list);
	free(world->interactables.list);
	free(world->entities.entities);

	// unload all textures
	for(int i = 0; i < world->textures.size; i++) UnloadTexture(world->textures.better_textures[i].tx);

	CloseWindow();
}

float getAngle(Vector2 v1, Vector2 v2) // good
{
	float angle = (atan2f((v2.y - v1.y), (v2.x - v1.x)) * RAD2DEG);
	
	if(angle > 360) angle -= 360;
	else if (angle < 0) angle += 360;

	return angle;
}

int entityCollisionWorld(Vector2 pos, TileList* layer) // good
{
	Rectangle en_area = {pos.x, pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE};
	for(int i = 0; i < layer->size; i++) if(CheckCollisionRecs(en_area, (Rectangle){layer->list[i].sp.x, layer->list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}) && layer->list[i].active) return i;
	return -1;
}

// returns index of entity that an entity 'en' has collided with
int entityCollisionEntity(int en_id, Vector2 en_pos, EntityList* ents) // good
{
	for(int i = 0; i < ents->size; i++)
	{
		// prevent collision with itself
		if(en_id == ents->entities[i].id) continue;
		if(CheckCollisionPointRec(en_pos, (Rectangle){ents->entities[i].pos.x, ents->entities[i].pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE})) return i;
	}

	return -1;
}

void animateEntity(Entity* en) // good
{
	en->frame_count++;
	en->xfp = (en->frame_count / en->anim_speed);
	if(en->xfp > 3) en->frame_count = en->xfp = 0;
}

void moveEntity(TileList* walls, EntityList* entities, Entity* en, Vector2 mv, EntityType et) // good
{
	int wall_col = -1;
	int en_col = -1;

	en->move = true;

	// move en by movement vector
	en->pos = Vector2Add(en->pos, mv);

	// finding the index of the collided wall or entitys (if any)
	wall_col = entityCollisionWorld(en->pos, walls);
	en_col = entityCollisionEntity(en->id, (Vector2){en->pos.x + TILE_SIZE, en->pos.y + TILE_SIZE}, entities);
	
	// collision handling
	if(et == ENEMY)
	{
		if((wall_col >= 0) || (en_col >= 0)) en->pos = Vector2Subtract(en->pos, mv);
	}

	if(et == PLAYER) if((wall_col >= 0)) en->pos = Vector2Subtract(en->pos, mv);
}

void displayStatistic(Vector2 pos, int width, int height, float f) 
{
	DrawRectangle(pos.x, pos.y, width * (f / 100), height, RED);
	DrawRectangleLines(pos.x, pos.y, width, height, BLACK);
}

void updateEntities(EntityList* world_entities, Entity* player, World* world) // good
{
	for(int i = 0; i < world_entities->size; i++)
	{
		Entity* en = &world_entities->entities[i];
		
		// only update alive entites
		if(en->health <= 0) continue;

		// getting angluar speed relative to the player
		en->angle = getAngle(en->pos, player->pos);
		Vector2 dx = {(cos(en->angle * DEG2RAD) * en->speed), 0};
		Vector2 dy = {0, (sin(en->angle * DEG2RAD) * en->speed)};
		
		// animation
		animateEntity(en);

		// setting sprite direction based on angle
		if((en->angle > 45) && (en->angle < 135)) en->yfp = 0;
		else if((en->angle > 225) && (en->angle < 315)) en->yfp = 1;
		else if((en->angle > 135) && (en->angle < 215)) en->yfp = 3;
		else en->yfp = 2;

		// moving entity by components
		moveEntity(&world->walls, world_entities, en, dx, ENEMY);
		moveEntity(&world->walls, world_entities, en, dy, ENEMY);

		// displaying health bar
		displayStatistic((Vector2){en->pos.x + 5, en->pos.y - 7}, 20, 7, en->health);

		// only draw entity when in bounds
		if(CheckCollisionPointRec((Vector2){en->pos.x + SCREEN_TILE_SIZE, en->pos.y + SCREEN_TILE_SIZE}, world->area))
		{
			DrawTexturePro(en->tx, (Rectangle){en->xfp * TILE_SIZE, en->yfp * TILE_SIZE, TILE_SIZE, TILE_SIZE},
										(Rectangle){en->pos.x, en->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, (Vector2){0,0}, 0, WHITE);
		}
	}
}

void moveBtn(Entity* player, TileList* walls, EntityList* entities, KeyboardKey key, Vector2 mv, int fp)
{
	if(IsKeyDown(key))
	{
		moveEntity(walls, entities, player, mv, PLAYER);
		player->yfp = fp;
	}
}

void movement(TileList* world_walls, Entity* player, EntityList* entities)
{
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
            player->speed = (sqrt(8 * (pow(player->speed, 2))) / 4);
			player->adjsp = true;
        }
    }

	else
	{
        player->speed = 3;
		player->adjsp = false;
    }
	
	// movement controls
	moveBtn(player, world_walls, entities, KEY_W, (Vector2){0, -player->speed}, 1);
	moveBtn(player, world_walls, entities, KEY_A, (Vector2){-player->speed, 0}, 3);
	moveBtn(player, world_walls, entities, KEY_S, (Vector2){0, player->speed}, 0);
	moveBtn(player, world_walls, entities, KEY_D, (Vector2){player->speed, 0}, 2);
	
	// afk
	if(!player->move) player->xfp = player->yfp = 0;

	// resetting movement flag
	player->move = false;
}

void heal(TileList* world_heals, Entity* player)
{
	int col =  entityCollisionWorld(player->pos, world_heals);

	if(col >= 0)
	{
		DrawText("PRESSING H WILL HEAL", GetScreenWidth() - 310, GetScreenHeight() - 110, 20, GREEN);
		if((IsKeyPressed(KEY_H)) && (player->health < 100))
		{
			player->health += 20;
			if(player->health > 100) player->health = 100;
			world_heals->list[col].active = false;
		}
	}

	// health bar
	DrawText("HEALTH", GetScreenWidth() - 300, GetScreenHeight() - 80, 20, RED);
	displayStatistic((Vector2){GetScreenWidth() - 310, GetScreenHeight() - 60}, 300, 50, player->health);
}

void fight(Entity* player, EntityList* enemies)
{
		int mob_col = entityCollisionEntity(player->id, (Vector2){player->pos.x + TILE_SIZE, player->pos.y + TILE_SIZE}, enemies);
		if(mob_col >= 0)
		{
			Entity* fight_en = &enemies->entities[mob_col];
			
			// small window of invincibility after getting hit
			if(TimerDone(player->timer))
			{
				StartTimer(&player->timer, 1.5);
				player->health = (player->health - 20 < 0) ? 0 : player->health - 20;
				
				// when you die
				if(player->health == 0)
				{
					status = DEAD;
					// player->alive = false;
				}
			}

			// doing damage to mob
			if(IsKeyPressed(KEY_E))
			{
				fight_en->health = fight_en->health - 20 > 0 ? fight_en->health - 20 : 0;
				
				// killing an enemy
				if(fight_en->health == 0)
				{
					fight_en->alive = false;
					fight_en->pos = (Vector2){-1000000, -1000000};
					
					// gaining experience/leveling up
					player->exp += 20;
					if(player->exp >= 100)
					{
						player->exp = 0;
						player->level++;
					}
				}
			}
		}
		
		// drawing and updating players level
		DrawRectangle(10, GetScreenHeight() - 60, (300) * (player->exp / 100), 50, GREEN);
		DrawRectangleLines(10, GetScreenHeight() - 60, 300, 50, BLACK);
		
		char lvl_dsc[100]; sprintf(lvl_dsc, "LEVEL: %d", player->level);
		DrawText(lvl_dsc, 10, GetScreenHeight() - 80, 20, GREEN);
}

void mobCreation(Vector2 mp, World* world)
{
	if(CheckCollisionPointRec(mp, world->area) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
	{
		bool valid_spawn = true;
		// cant spawn mob in wall
		if(entityCollisionWorld(mp, &world->walls) >= 0) valid_spawn = false;
		// cant spawn mobs on top of each other
		if(entityCollisionEntity(0, (Vector2){mp.x + TILE_SIZE, mp.y + TILE_SIZE}, &world->entities) >= 0) valid_spawn = false;
		// add enemy if spawn is valid
		if(valid_spawn) addEntity(&world->entities, createEnemy());
	}
}

void updatePlayer(TileList* world_walls, Entity* player, EntityList* entities)
{
	// animation
	animateEntity(player);
	
	// movement
	movement(world_walls, player, entities);

	// drawing player
	DrawTexturePro(player->tx, (Rectangle){player->xfp * TILE_SIZE, player->yfp * TILE_SIZE, TILE_SIZE, TILE_SIZE},
										(Rectangle){player->pos.x, player->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, (Vector2){0,0}, 0, WHITE);
} 

int main()
{
	init(&world, &player);
	while((status != QUIT) && (!WindowShouldClose()))
    {
		camera.target = player.pos;
		mp = GetScreenToWorld2D(GetMousePosition(), camera);
		world.area = (Rectangle){camera.target.x - camera.offset.x, camera.target.y - camera.offset.y, SCREEN_WIDTH, SCREEN_HEIGHT};
		BeginDrawing();
			switch(status)
			{
				case ALIVE:
					ClearBackground(BLACK);
					BeginMode2D(camera);
						// drawing world tiles	
						drawLayer(&world.floors);
						drawLayer(&world.health_buffs);
						drawLayer(&world.damage_buffs);
						drawLayer(&world.doors);
						drawLayer(&world.interactables);
						drawLayer(&world.walls);
						// updating entities
						updateEntities(&world.entities, &player, &world);
						updatePlayer(&world.walls, &player, &world.entities);
					EndMode2D();
					mobCreation(mp, &world);
					fight(&player, &world.entities);
					heal(&world.health_buffs, &player);
					break;
				case DEAD:
					ClearBackground(GRAY);
					DrawText(DEATH_PROMPT, (GetScreenWidth() / 2.0f) - (MeasureText(DEATH_PROMPT, 30) / 2.0f),GetScreenHeight() / 2.0f, 30, RED);
					if(GuiButton((Rectangle){(GetScreenWidth() / 2.0f) - 125, (GetScreenHeight() / 2.0f) + 50, 100, 50}, "RESPAWN"))
					{
						status = ALIVE;
						player = createPlayer();
						resetEntityList(&world.entities);
					}
					if(GuiButton((Rectangle){(GetScreenWidth() / 2.0f) + 50, (GetScreenHeight() / 2.0f) + 50, 100, 50}, "QUIT")) status = QUIT;
					break;
				default:
					break;
			}
		DrawFPS(0, 0);
		EndDrawing();
    }

    deinit(&world);
    return 0;
}

// TODO - refactor BetterTexture struct and functions
// make world, player, and mouse position variable global