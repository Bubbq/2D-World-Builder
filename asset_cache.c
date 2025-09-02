#include "asset_cache.h"

#include "utils.h"

AssetEntry* asset_entry_init(const char* asset_path)
{
    if (!valid_string(asset_path))
        return NULL;
    
    const unsigned long int hash_id = hash_string(asset_path);
    if (hash_id == 0)
        return NULL;

    const Texture texture = LoadTexture(asset_path);
    if (!IsTextureReady(texture))
        return NULL;

    char* path = strdup(asset_path);
    if (!path) {
        UnloadTexture(texture);
        return NULL;
    }

    AssetEntry* entry = malloc(sizeof(AssetEntry));
    if (!entry) {
        UnloadTexture(texture);
        free(path); path = NULL;
        return NULL;
    }

    entry->path = path;
    entry->id = hash_id;
    entry->texture = texture;

    return entry;
}

void asset_entry_free(AssetEntry* entry)
{
    if (!entry)
        return;

    entry->id = 0;

    if (IsTextureReady(entry->texture))
        UnloadTexture(entry->texture);

    if (entry->path) {
        free(entry->path); entry->path = NULL;
    }

    free(entry); entry = NULL;
}

bool asset_entry_is_ready(const AssetEntry* entry)
{
    return (entry) && (entry->id != 0) && (IsTextureReady(entry->texture)) && (valid_string(entry->path));
}

void asset_cache_add(AssetCache* cache, AssetEntry* entry)
{
    if (asset_entry_is_ready(entry))
        HASH_ADD_INT((*cache), id, entry);
}

void asset_cache_remove(AssetCache* cache, AssetEntry* entry)
{
    if (!cache || !entry)
        return;

    HASH_DEL((*cache), entry);

    asset_entry_free(entry);
}

void asset_cache_free(AssetCache* cache)
{
    if (!cache)
        return;

    AssetEntry* current, *tmp;

    HASH_ITER(hh, (*cache), current, tmp) 
        asset_cache_remove(cache, current);
}

AssetEntry* asset_cache_find(AssetCache* cache, const unsigned long int id)
{
    AssetEntry* found = NULL;

    HASH_FIND_INT((*cache), &id, found);

    return found;
}