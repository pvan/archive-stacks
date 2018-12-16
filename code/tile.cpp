


struct tile
{

    // position/shape in list
    v2 pos;
    v2 size;


    // file data
    string fullpath;
    string name;
    u64 modifiedTimeSinceLastEpoc;


    // media data
    v2 resolution;
    ffmpeg_media media;
    bool needs_loading = false;
    bool needs_unloading = false;
    bool is_media_loaded = false;


    // debug stuff
    u32 rand_color;
    u32 red_color = 0xff0000ff;

    void LoadMedia() {
        if (is_media_loaded) { OutputDebugString("tile media already loaded!\n"); return; }
        if (fullpath.length < 1) { OutputDebugString("tile media path not set yet!\n"); return; }
        media.LoadFromFile(fullpath);
        if (media.loaded) {
            is_media_loaded = true;
            needs_loading = false;
        }
    }

    void UnloadMedia() {
        if (!is_media_loaded) { needs_unloading = false; return; } // already unloaded
        media.Unload();
        if (media.loaded == false) {
            is_media_loaded = false;
            needs_unloading = false;
        }
    }

    bitmap GetImage() {
        if (is_media_loaded) {
            return media.GetFrame();
            // return { &red_color, 1, 1 };
        } else {
            return { &rand_color, 1, 1 };
        }
    }


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


void ReadTileResolutions(tile_pool *pool)
{
    for (int i = 0; i < pool->count; i++) {
        tile& t = pool->pool[i];
        t.resolution = ffmpeg_GetResolution(t.fullpath);
    }
}


void CalcWidthToFit(float desired_image_width, int container_width,
                    float *real_width, int *number_of_columns) {
    float img_width = desired_image_width;
    if (img_width < 1) img_width = 1;
    if (img_width > 2000) img_width = 2000;

    int cols = container_width / img_width;
    if (cols < 1) cols = 1;
    *real_width = (float)(container_width) / (float)cols;
    *number_of_columns = cols;
}

void ArrangeTilesInOrder(tile_pool *pool, rect destination_rect) {

        float MASTER_IMG_WIDTH = 100; // todo: pass as input, affected by zoom

        float realWidth;
        int cols;
        CalcWidthToFit(MASTER_IMG_WIDTH, destination_rect.w, &realWidth, &cols);

        float *colbottoms = (float*)malloc(cols*sizeof(float));
        for (int c = 0; c < cols; c++)
            colbottoms[c] = 0;

        // for (int j = 0; j < stubRectCount; j++) {
        for (int i = 0; i < pool->count; i++) {
            tile& thisTile = pool->pool[i];

            float aspect_ratio = thisTile.resolution.w / thisTile.resolution.h;

            thisTile.size.w = realWidth;
            thisTile.size.h = realWidth / aspect_ratio;

            int shortestCol = 0;
            for (int c = 1; c < cols; c++)
                if (colbottoms[c] < colbottoms[shortestCol])
                    shortestCol = c;

            float x = shortestCol * realWidth;
            float y = colbottoms[shortestCol];

            thisTile.pos = {x, y};

            colbottoms[shortestCol] += thisTile.size.h;
        }



        float total_height;
        total_height = 0;
        int longestCol = 0;
        for (int c = 1; c < cols; c++)
            if (colbottoms[c] > colbottoms[longestCol])
                longestCol = c;
        total_height = colbottoms[longestCol];

        free(colbottoms);



        // if (INCREMENT_SCROLL != 0) {
        //     amt_off_anchor += INCREMENT_SCROLL;
        //     INCREMENT_SCROLL = 0;
        // }

        // // convert anchor point / amt_off to pixels down the camera is
        // // note we use ANCHOR_ITEM not dispalyList[ANCHOR_ITEM] because
        // // we want to tie to the actual item, not it's position in the list (or we could change it)
        // MASTER_SCROLL = (item_rects[displayList.indices[ANCHOR_ITEM]].y + amt_off_anchor);


        // // input that might affect position
        // if (keysDown.pgDown) { MASTER_SCROLL += g_ch; }
        // if (keysDown.pgUp) { MASTER_SCROLL -= g_ch; }



}


