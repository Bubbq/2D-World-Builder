#include "headers/raylib.h"
#include "headers/raymath.h"
#include "headers/entity.h"
#include <stdlib.h>

void resize_entitylist(EntityList* entitylist)
{
	entitylist->cap *= 2;
	entitylist->entities = realloc(entitylist->entities, entitylist->cap);
}

void add_entity(EntityList* entitylist, Entity entity)
{
	if(entitylist->size * sizeof(Entity) == entitylist->cap) 
        resize_entitylist(entitylist);

	entitylist->entities[entitylist->size++] = entity;
}

EntityList create_entitylist()
{
    EntityList entitylist;

    entitylist.size = 0;
    entitylist.cap = sizeof(Entity);
    entitylist.entities = malloc(entitylist.cap);

    return entitylist;
}

int check_collision_entity(int entity_id, Vector2 entity_position, EntityList* entitylist)
{
    for(int i = 0; i < entitylist->size; i++)
	{
		// forget self collision and dead entities
		if(entity_id == entitylist->entities[i].id || entitylist->entities[i].health <= 0) 
            continue;

        Rectangle entity_area = (Rectangle){ entitylist->entities[i].pos.x, entitylist->entities[i].pos.y, 32, 32 };
		
        if(CheckCollisionPointRec(entity_position, entity_area))
             return i;
	}

	return -1;
}

void clear_entitylist(EntityList* entitylist)
{
	if(entitylist->size > 0) 
        free(entitylist->entities);
    
    *entitylist = create_entitylist();
}

void draw_entities(EntityList* entitylist, Rectangle world_border, int entity_size, int sprite_sheet_size)
{
    for(int i = 0; i < entitylist->size; i++)
    {
        Entity* entity = (entitylist->entities + i);
        if(entity->health <= 0)
            continue;

        if(CheckCollisionPointRec((Vector2){ entity->pos.x + (entity_size / 2.0), entity->pos.y + (entity_size / 2.0) }, world_border))
		{
			// health bar
			DrawRectangle(entity->pos.x + 5, entity->pos.y - 7, 20 * (entity->health / 100), 7, RED);

            // entity sprite
			DrawTexturePro(entity->tx, (Rectangle){entity->animation.xfposition * sprite_sheet_size, entity->animation.yfposition * sprite_sheet_size, sprite_sheet_size, sprite_sheet_size}, (Rectangle){ entity->pos.x, entity->pos.y, entity_size, entity_size}, (Vector2){0,0}, 0, WHITE);
		}
    }
}

void deal_damage(Entity* entity, Entity* target, int entity_size)
{
    Rectangle entity_area = (Rectangle){ entity->pos.x, entity->pos.y, entity_size, entity_size };
    Rectangle target_area = (Rectangle){ target->pos.x, target->pos.y, entity_size, entity_size };

    if(CheckCollisionRecs(entity_area, target_area) && is_timer_done(entity->attack_speed))
    {
        target->health -= entity->damage;

        if(target->health <= 0)
            gain_exp(entity);

        start_timer(&entity->attack_speed, 1.0f);
    }
}

void gain_exp(Entity *entity)
{
    entity->exp += 20;

    // leveling up
    if(entity->exp >= (entity->level * 100))
    {
        entity->exp = 0;
        entity->level++;
    }
}

void dealloc_entitylist(EntityList* entitylist)
{
    free(entitylist->entities);
}

void heal(Entity* entity, int max_health, int heal_amount)
{
    entity->health += heal_amount;

    if(entity->health > max_health)
        entity->health = max_health;
}