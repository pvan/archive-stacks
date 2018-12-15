


struct tile
{

    v2 pos;

    v2 resolution;


    string fullpath;

    string name;

    u64 modifiedTimeSinceLastEpoc;


    static tile CreateFromFile(string path) {
        tile newTile = {0};
        assert(win32_PathExists(path));
        assert(!win32_IsDirectory(path));
        // if (!win32_PathExists(path)) return newTile; // i feel like these are just the error down the line
        // if (win32_IsDirectory(path)) return newTile;

        newTile.fullpath = path.Copy();
        newTile.name = string::CopyJustFilename(path);

        newTile.modifiedTimeSinceLastEpoc = win32_GetModifiedTimeSinceEpoc(path);

        return newTile;
    };


    bool operator== (tile o) { return fullpath==o.fullpath; }


};




DEFINE_TYPE_POOL(tile);  // missing "tile_pile" opportunity here


// although pools don't really have any guaranteed static order, we put one in order here anyway
void SortTilePoolByDate(tile_pool *pool)
{
    for (int d = 0; d < pool->count-1; d++) {
        int anyswap = false;

        for (int j = 0; j < pool->count-1; j++) {
            if (pool->pool[j].modifiedTimeSinceLastEpoc > pool->pool[j+1].modifiedTimeSinceLastEpoc) {

                // swap
                tile temp = pool->pool[j];
                pool->pool[j] = pool->pool[j+1];
                pool->pool[j+1] = temp;

                anyswap = true;
            }
        }
        if (!anyswap) break;
    }
}
