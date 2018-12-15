


struct tile
{

    v2 pos;

    v2 size;

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

void ReadTileResolutions(tile_pool *pool) {



}

void ArrangeTilesInOrder(tile_pool *pool) {

        // // COLS (W = 100)
        // float tagPanelW = 0;
        // float tagPanelH = 0;
        // Rect destPanel = {tagPanelW, 0, (float)cw-tagPanelW, (float)ch-tagPanelH};

        // float gap = 0;
        // float realColWidthWithGap;
        // int cols;
        // CalcWidthToFit(MASTER_IMG_WIDTH, destPanel.w, gap, &realColWidthWithGap, &cols);
        // REAL_WIDTH = realColWidthWithGap-gap;

        int cols = 3;

        float *colbottoms = (float*)malloc(cols*sizeof(float));
        for (int c = 0; c < cols; c++)
            colbottoms[c] = 0;

        // for (int j = 0; j < stubRectCount; j++) {
        for (int i = 0; i < pool->count; i++) {
            tile& thisTile = pool->pool[i];

            float aspect_ratio = thisTile.resolution.w / thisTile.resolution.h;

            thisTile.size.w = 100;
            thisTile.size.h = 100 / aspect_ratio;

            int shortestCol = 0;
            for (int c = 1; c < cols; c++)
                if (colbottoms[c] < colbottoms[shortestCol])
                    shortestCol = c;

            float x = shortestCol * 100;
            float y = colbottoms[shortestCol];

            // float x = (shortestCol * realColWidthWithGap) + gap;
            // float y = colbottoms[shortestCol] + gap;
            // float calcH = realColWidthWithGap / aspect_ratio;

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


