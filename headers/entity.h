#ifndef  ENTITY_H
#define ENTITY_H

#include <stdlib.h>

#include "raylib.h"
#include "animation.h"

typedef struct
{
	double startTime;
	double lifeTime;
} Timer;

void start_timer(Timer *timer, double lifetime); 
bool is_timer_done(Timer timer);

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
	Vector2 direction;
} Entity;

typedef struct
{
	int size;
	size_t cap;
	Entity* entities;
} EntityList;

void resize_entitylist(EntityList* entitylist);
void add_entity(EntityList* entitylist, Entity entity); 
void delete_entity(EntityList* entitylist, int entity_id);
EntityList create_entitylist();
int check_collision_entity(int entity_id, Vector2 entity_position, EntityList* entitylist);
void reset_entitylist(EntityList* entitylist);
void draw_entities(EntityList* entitylist, Rectangle world_border);
void deal_damage(Entity* entity, Entity* target);
void gain_exp(Entity* entity);
void dealloc_entitylist(EntityList* entitylist);

#endif