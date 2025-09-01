#ifndef TEXTURE_CACHE_H
#define TEXTURE_CACHE_H

#include "raylib.h"
#include "uthash.h"

typedef struct
{
    char id [64];
    UT_hash_handle hh;
    Texture texture;
} TextureCacheEntry;

typedef TextureCacheEntry* TextureCache;

TextureCacheEntry* texture_cache_entry_init(const Texture texture, const char* id);
void texture_cache_entry_free(TextureCacheEntry* entry);
bool texture_cache_entry_is_ready(TextureCacheEntry* entry);
void texture_cache_add_entry(TextureCache* texture_cache, TextureCacheEntry* entry);
void texture_cache_remove_entry(TextureCache* texture_cache, TextureCacheEntry* entry);
void texture_cache_free(TextureCache* texture_cache);
TextureCacheEntry* texture_cache_find_entry(TextureCache* texture_cache, const char* id);

#endif