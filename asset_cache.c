#include "asset_cache.h"

#include "utils.h"

bool asset_init(Asset* dest, const Texture texture, const char* path)
{
    if (!dest || !IsTextureReady(texture) || !valid_string(path))
        return false;
    
    dest->texture = texture;
    dest->path = strdup(path);
    
    return asset_is_ready(dest);
}

void asset_free(Asset* asset)
{
    if (!asset)
        return;
    
    if (IsTextureReady(asset->texture))
        UnloadTexture(asset->texture);
    
    if (asset->path) {
        free(asset->path); asset->path = NULL;
    }
}

bool asset_is_ready(const Asset* asset)
{
    return (asset) && (IsTextureReady(asset->texture)) && (valid_string(asset->path));
}

AssetEntry* asset_entry_init(const Texture texture, const unsigned long int id, const char* path)
{
    if (!IsTextureReady(texture) || !valid_string(path))
        return NULL;

    AssetEntry* entry = malloc(sizeof(AssetEntry));
    if (!entry) 
        return NULL;

    entry->id = id;

    entry->asset = (Asset){0};
    if (!asset_init( &entry->asset, texture, path)) {
        asset_free(&entry->asset);
        free(entry); entry = NULL;
    }

    return entry;
}

void asset_entry_free(AssetEntry* entry)
{
    if (!entry)
        return;

    asset_free(&entry->asset);

    free(entry); entry = NULL;
}

bool asset_entry_is_ready(const AssetEntry* entry)
{
    return (entry) && (entry->id != 0) && (asset_is_ready(&entry->asset));
}

void asset_cache_add(AssetCache* cache, AssetEntry* entry)
{
    if (asset_entry_is_ready(entry) && !asset_cache_find(cache, entry->id))
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