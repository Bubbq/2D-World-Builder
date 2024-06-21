#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "worldbuilder.h"

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
const char* HEAL_PROMPT = "PRESSING H WILL HEAL";
const int ANIMATION_SPEED = 20;
const int FPS = 60;

const Entity DEAD_ENT = (Entity){0};

Vector2 mp;
World world;
Entity player;
Camera2D camera;
Status status = ALIVE;

float getAngle(Vector2 v1, Vector2 v2) 
{
	float angle = (atan2f((v2.y - v1.y), (v2.x - v1.x)) * RAD2DEG);
	
	if(angle > 360) angle -= 360;
	else if (angle < 0) angle += 360;

	return angle;
}

void StartTimer(Timer *timer, double lifetime) 
{
	timer->startTime = GetTime();
	timer->lifeTime = lifetime;
}

bool TimerDone(Timer timer) { return GetTime() - timer.startTime >= timer.lifeTime; } 

void resizeEntities(EntityList* el) 
{
	el->cap *= 2;
	el->entities = realloc(el->entities, el->cap);
}

void resizeLayer(TileList* tl) 
{
    tl->cap *= 2;
    tl->list = realloc(tl->list, tl->cap);
}

void addEntity(EntityList* el, Entity en) 
{
	if(el->size * sizeof(Entity) == el->cap) resizeEntities(el);
	el->entities[el->size++] = en;
}

void addTile(TileList* tl, Tile t) 
{
    if(tl->size * sizeof(Tile) == tl->cap) resizeLayer(tl);
    tl->list[tl->size++] = t;
}

Texture2D addTexture(Textures* textures, const char* FP)
{
	// returning non unique texture
	for(int i = 0; i < textures->size; i++) { if(strcmp(textures->better_textures[i].fp, FP) == 0) return textures->better_textures[i].tx; }

	// adding new texture
	textures->better_textures[textures->size] = (BetterTexture){LoadTexture(FP), ""};
	strcpy(textures->better_textures[textures->size].fp, FP);
	textures->size++;

	return textures->better_textures[textures->size - 1].tx;
}

void loadLayers(const char* FP)
{
    char line[CHAR_LIMIT];
    FILE* inFile = fopen(FP, "r");
    
	if(inFile == NULL) return;

    Vector2 src;
    Vector2 sp;
    enum Element tt;
    char fp[CHAR_LIMIT];
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
		strcpy(fp, strtok(NULL, ","));
		anim = atoi(strtok(NULL, ","));
		fc = atoi(strtok(NULL, ","));
		frames = atoi(strtok(NULL, ","));
		anim_speed = atoi(strtok(NULL, "\n"));

        Tile tile = (Tile){src, sp, addTexture(&world.textures, fp), tt, "NULL", true, anim, fc, frames, anim_speed};

        switch (tt)
		{
			case WALL: addTile(&world.walls, tile); break;
			case FLOOR: addTile(&world.floors, tile); break;
			case DOOR: addTile(&world.doors, tile); break;
			case HEALTH_BUFF: addTile(&world.health_buffs, tile); break;
			case DAMAGE_BUFF: addTile(&world.damage_buffs, tile); break;
			case INTERACTABLE: addTile(&world.interactables, tile); break;
			default:
				break;
		}
    }

    fclose(inFile);
}

int animateTile(Tile* t) 
{
	int frame_pos;

	t->fc++;
	frame_pos = (t->fc / t->anim_speed);
	if(frame_pos > t->frames) frame_pos = t->fc = 0;

	return frame_pos;
}

void animateEntity(Entity* en) 
{
	// afk case
	if(!en->move) en->xfp = en->yfp = 0;

	else 
	{
		en->frame_count++;
		en->xfp = (en->frame_count / en->anim_speed);
		if(en->xfp > 3) en->frame_count = en->xfp = 0;
	}
}

void drawLayer(TileList* tl)
{	
    for(int i = 0; i < tl->size; i++)
    {
		int frame_pos = 0;
		Tile* t = &tl->list[i];

		if(!t->active) continue;

		// animate tiles
		if(t->anim) frame_pos = animateTile(t);

		DrawTexturePro(t->tx, (Rectangle){t->src.x + (frame_pos * TILE_SIZE), t->src.y, TILE_SIZE, TILE_SIZE},
									(Rectangle){t->sp.x, t->sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, (Vector2){0,0}, 0, WHITE);
    }
}

TileList createTileList() { return (TileList){0, TILE_CAP, malloc(TILE_CAP)}; }

EntityList createEntityList(){ return (EntityList){0, ENTITY_CAP, malloc(ENTITY_CAP)}; } 

void resetEntityList(EntityList* el) 
{
	if(el->size > 0) free(el->entities);
	el->entities = malloc(ENTITY_CAP);
	el->cap = ENTITY_CAP;
	el->size = 0;
}

Vector2 getSpawnPoint() 
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

Entity createPlayer() { return (Entity){"player", 0, 100.0f, 3.0f, getSpawnPoint(), true, false, false, LoadTexture(PLAYER_PATH), ANIMATION_SPEED}; } 

Entity createEnemy(){ return (Entity){"mob", 0, 100, 1.0f, mp, true, false, true, addTexture(&world.textures, PLAYER_PATH), ANIMATION_SPEED, GetRandomValue(0, 1000000000)}; }

void init()
{
	// game settings
	SetTargetFPS(FPS);
	SetTraceLogLevel(LOG_ERROR);
	InitWindow(SCREEN_WIDTH,SCREEN_HEIGHT, "2D-Engine");
	
	world.textures.size = 0;

	// world inititalization
	world.walls = createTileList();
	world.floors = createTileList();
	world.doors = createTileList();
	world.health_buffs = createTileList();
	world.damage_buffs = createTileList();
	world.interactables = createTileList();
	world.entities = createEntityList();
    loadLayers(WORLD_PATH);

	// player instantiation
	player = createPlayer();
	
	// camera setup
	camera.zoom = 1.0f;
	camera.target = player.pos;
	camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
}

void deinit() 
{
	// free alloc mem from dynamic lists
	free(world.walls.list);
	free(world.floors.list);
	free(world.doors.list);
	free(world.health_buffs.list);
	free(world.damage_buffs.list);
	free(world.interactables.list);
	free(world.entities.entities);

	// unload all textures
	for(int i = 0; i < world.textures.size; i++) UnloadTexture(world.textures.better_textures[i].tx);

	CloseWindow();
}

int collisionLayer(Vector2 pos, TileList* layer) 
{
	Rectangle en_area = {pos.x, pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE};
	for(int i = 0; i < layer->size; i++) if(CheckCollisionRecs(en_area, (Rectangle){layer->list[i].sp.x, layer->list[i].sp.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}) && layer->list[i].active) return i;
	return -1;
}

int collisionEntity(int en_id, Vector2 en_pos, EntityList* ents) 
{
	for(int i = 0; i < ents->size; i++)
	{
		// prevent collision with itself
		if(en_id == ents->entities[i].id) continue;
		if(CheckCollisionPointRec(en_pos, (Rectangle){ents->entities[i].pos.x, ents->entities[i].pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE})) return i;
	}

	return -1;
}

void moveEntity(TileList* walls, EntityList* entities, Entity* en, Vector2 mv, EntityType et) 
{
	int wall_col = -1;
	int en_col = -1;

	// move en by movement vector
	en->pos = Vector2Add(en->pos, mv);

	// finding the index of the collided wall or entitys (if any)
	wall_col = collisionLayer(en->pos, walls);
	en_col = collisionEntity(en->id, (Vector2){en->pos.x + TILE_SIZE, en->pos.y + TILE_SIZE}, entities);
	
	// collision handling
	if(wall_col >= 0 || en_col >= 0) en->pos = Vector2Subtract(en->pos, mv);
}

void displayStatistic(Vector2 pos, int width, int height, float f, Color c) 
{
	DrawRectangle(pos.x, pos.y, width * (f / 100), height, c);
	DrawRectangleLines(pos.x, pos.y, width, height, BLACK);
}

void damagePlayer(Entity* en, Entity* player)
{
	Rectangle player_area = {player->pos.x, player->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE};
	Vector2 center_of_en = {en->pos.x + TILE_SIZE, en->pos.y + TILE_SIZE};
	if(CheckCollisionPointRec(center_of_en, player_area))
	{
		if(TimerDone(en->attack_speed))
		{
			player->health -= 20;
			if(player->health <= 0) status = DEAD;
			StartTimer(&en->attack_speed, 1.0f);
		}
	}
}

void updateEntities(EntityList* world_entities, Entity* player, World* world) 
{
	for(int i = 0; i < world_entities->size; i++)
	{
		Entity* en = world_entities->entities + i;

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

		// combat
		damagePlayer(en, player);

		// moving entity by components
		moveEntity(&world->walls, world_entities, en, dx, ENEMY);
		moveEntity(&world->walls, world_entities, en, dy, ENEMY);

		// displaying health bar
		displayStatistic((Vector2){en->pos.x + 5, en->pos.y - 7}, 20, 7, en->health, RED);

		// only draw entity when in bounds
		if(CheckCollisionPointRec((Vector2){en->pos.x + SCREEN_TILE_SIZE, en->pos.y + SCREEN_TILE_SIZE}, world->area))
		{
			DrawTexturePro(en->tx, (Rectangle){en->xfp * TILE_SIZE, en->yfp * TILE_SIZE, TILE_SIZE, TILE_SIZE},
										(Rectangle){en->pos.x, en->pos.y, SCREEN_TILE_SIZE, SCREEN_TILE_SIZE}, (Vector2){0,0}, 0, WHITE);
		}
	}
}

void movement(TileList* world_walls, Entity* player, EntityList* entities)
{
	player->move = (IsKeyDown(KEY_W) || 
					IsKeyDown(KEY_A) || 
					IsKeyDown(KEY_S) ||
					IsKeyDown(KEY_D));

    bool diagonal = (IsKeyDown(KEY_W) && IsKeyDown(KEY_D)) || 
                    (IsKeyDown(KEY_W) && IsKeyDown(KEY_A)) || 
					(IsKeyDown(KEY_A) && IsKeyDown(KEY_S)) || 
					(IsKeyDown(KEY_S) && IsKeyDown(KEY_D));

	// using pythag thrm to prevent speedup when moving diagonally
    if (diagonal && !player->adjsp)
    {
		player->speed = (sqrt(8 * (pow(player->speed, 2))) / 4);
		player->adjsp = true;
    }

	else
	{
        player->speed = 3.0f;
		player->adjsp = false;
    }

	// movement controls
	if(IsKeyDown(KEY_W))
	{
		player->yfp = 1;
		player->pos.y -= player->speed;
		if(collisionLayer(player->pos, world_walls) >= 0) player->pos.y += player->speed;
	}
	
	if(IsKeyDown(KEY_A))
	{
		player->yfp = 3;
		player->pos.x -= player->speed;
		if(collisionLayer(player->pos, world_walls) >= 0) player->pos.x += player->speed;
	}
	
	if(IsKeyDown(KEY_S))
	{
		player->yfp = 0;
		player->pos.y += player->speed;
		if(collisionLayer(player->pos, world_walls) >= 0) player->pos.y -= player->speed;
	}
	
	if(IsKeyDown(KEY_D))
	{
		player->pos.x += player->speed;
		player->yfp = 2;
		if(collisionLayer(player->pos, world_walls) >= 0) player->pos.y += player->speed;
	}
}

void heal(Entity* player, Tile* health_buff)
{
	player->health += 20;
	if(player->health > 100) player->health = 100;
	health_buff->active = false;
}

void encounterHeals(Entity* player, TileList* heals)
{
	int col = collisionLayer(player->pos, heals);
	if(col >=0)
	{	
		DrawText(HEAL_PROMPT, GetScreenWidth() - 310, GetScreenHeight() - 110, 20, RAYWHITE);
		if(IsKeyDown(KEY_H) && player->health < 100 && TimerDone(player->heal_speed)) 
		{
			heal(player, &heals->list[col]);
			StartTimer(&player->heal_speed, 0.5f);
		}
	}
}

void awardXP(Entity* player)
{
	player->exp += 20;
	if(player->exp >= 100)
	{
		player->exp = 0;
		player->level++;
	}
}

void combat(Entity* player, EntityList* enemies)
{
	int mob_col = collisionEntity(player->id, (Vector2){player->pos.x + TILE_SIZE, player->pos.y + TILE_SIZE}, enemies);
	if(mob_col >= 0)
	{
		Entity* en = enemies->entities + mob_col;
		if(IsKeyPressed(KEY_E))
		{
			en->health -= 20;
			if(en->health <= 0) {*en = DEAD_ENT; awardXP(player);}
		}
	}
}

void mobCreation(Vector2 mp, World* world)
{
	if(CheckCollisionPointRec(mp, world->area) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
	{
		bool valid_spawn = true;
		// cant spawn mob in wall
		if(collisionLayer(mp, &world->walls) >= 0) valid_spawn = false;
		// cant spawn mobs on top of each other
		if(collisionEntity(0, (Vector2){mp.x + TILE_SIZE, mp.y + TILE_SIZE}, &world->entities) >= 0) valid_spawn = false;
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

void showHealth(float health)
{
	DrawText("HEALTH", GetScreenWidth() - 300, GetScreenHeight() - 80, 20, RED);
	displayStatistic((Vector2){GetScreenWidth() - 310, GetScreenHeight() - 60}, 300, 50, health, RED);					
}

void showLevels(int level, float exp)
{
	char text[CHAR_LIMIT]; sprintf(text, "LEVEL: %d", level);
	DrawText(text, 10, GetScreenHeight() - 80, 20, GREEN);
	displayStatistic((Vector2){10, GetScreenHeight() - 60}, 300, 50, exp, GREEN);
}

void deathScreen()
{
	ClearBackground(GRAY);
	DrawText(DEATH_PROMPT, (GetScreenWidth() / 2.0f) - (MeasureText(DEATH_PROMPT, 30) / 2.0f),GetScreenHeight() / 2.0f, 30, RED);
	
	// respawn btn
	if(GuiButton((Rectangle){(GetScreenWidth() / 2.0f) - 125, (GetScreenHeight() / 2.0f) + 50, 100, 50}, "RESPAWN"))
	{
		status = ALIVE;
		player = createPlayer();
		resetEntityList(&world.entities);
	}

	// quit btn
	if(GuiButton((Rectangle){(GetScreenWidth() / 2.0f) + 50, (GetScreenHeight() / 2.0f) + 50, 100, 50}, "QUIT")) status = QUIT;
}

int main()
{
	init();
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
						// drawing world 	
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
					// game mechanics
					mobCreation(mp, &world);
					combat(&player, &world.entities);
					encounterHeals(&player, &world.health_buffs);
					// player statistics
					showHealth(player.health);
					showLevels(player.level, player.exp);
					break;
				case DEAD: deathScreen(); break;
				default:
					break;
			}
		DrawFPS(0, 0);
		EndDrawing();
    }
    deinit();
    return 0;
}
// TODO - refactor BetterTexture struct and functions
// make world, player, and mouse position variable global