


struct tile
{

    // position/shape in list/"world"
    v2 pos;
    v2 size;


    // file data
    string fullpath;
    string name;
    u64 modifiedTimeSinceLastEpoc;

    // thumbnail data
    string thumbpath;


    // media data
    v2 resolution;
    ffmpeg_media media;
    bool needs_loading = false;
    bool needs_unloading = false;
    bool is_media_loaded = false;

    // state for rendering
    bool texture_updated_since_last_read = false; // set true when changing texture, false when reading texture
    bool has_display_quad = false; // already assigned a display quad (set/unset based on on/offscreen at render time)
    int display_quad_index = 0; // only used if has_display_quad true

    // debug stuff
    u32 rand_color;
    u32 red_color = 0xffff0000;

    // for rendering
    opengl_quad display_quad;
    bool display_quad_created;
    bool display_quad_texture_sent_to_gpu = false;


    void LoadMedia() {
        if (is_media_loaded) { OutputDebugString("tile media already loaded!\n"); return; }
        if (fullpath.length < 1) { OutputDebugString("tile media path not set yet!\n"); return; }
        media.LoadFromFile(fullpath);
        if (media.loaded) {
            is_media_loaded = true;
            needs_loading = false;
            texture_updated_since_last_read = true;
        }
    }

    void UnloadMedia() {
        if (!is_media_loaded) { needs_unloading = false; return; } // already unloaded
        media.Unload();
        if (media.loaded == false) {
            is_media_loaded = false;
            needs_unloading = false;
            texture_updated_since_last_read = true; // not sure we really need this here
        }
    }

    bitmap GetImage(double dt) {
        texture_updated_since_last_read = false;
        if (is_media_loaded) {
            return media.GetFrame(dt);

            // bitmap test = media.GetFrame();
            // return { &red_color, 1, 1 };
        } else {
            // the color that will be used if tile is onscreen before media is done loading
            return { &rand_color, 1, 1 };
            // return { &red_color, 1, 1 };
        }
    }

    bool IsOnScreen(int client_height) {
        // return pos.y < client_height;
        return pos.y+size.h >= 0 && pos.y < client_height;  // bug
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

        newTile.thumbpath = CopyItemPathAndConvertToThumbPath(path);

        return newTile;
    };


    bool operator== (tile o) { return fullpath==o.fullpath; }


};




DEFINE_TYPE_POOL(tile);  // missing "tile_pile" opportunity here


// although our pools aren't intended to have any guaranteed static order, we put one in order here anyway
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

int ArrangeTilesInOrder(tile_pool *pool, rect destination_rect) {

    float MASTER_IMG_WIDTH = 200; // todo: pass as input, affected by zoom

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


    return total_height;

}


int state_amt_off_anchor = 0;
int state_anchor_item = 0;
void ShiftTilesBasedOnScrollPosition(tile_pool *pool, int *scroll_delta, Input keysDown, int ch, int tile_height) {

    if (*scroll_delta != 0) {
        state_amt_off_anchor += *scroll_delta;
        *scroll_delta = 0;
    }

    // convert anchor point / amt_off to pixels down the camera is
    // note we use ANCHOR_ITEM not dispalyList[ANCHOR_ITEM] because
    // we want to tie to the actual item, not it's position in the list (or we could change it)
    // tile_pool &pool_ref = *pool;
    // tile *anchor_tile = &pool_ref[state_anchor_item];
    // int MASTER_SCROLL = (anchor_tile->pos.y + state_amt_off_anchor);

static int MASTER_SCROLL = 0;
MASTER_SCROLL += state_amt_off_anchor;
state_amt_off_anchor = 0;

    // // input that might affect position
    // if (keysDown.pgDown) { MASTER_SCROLL += ch; }
    // if (keysDown.pgUp) { MASTER_SCROLL -= ch; }


    // // MOUSE   scrollbar
    // {
    //     // skip any proc if outside client area
    //     RECT clientRect; GetClientRect(g_hwnd, &clientRect);
    //     if (input.mouseX > 0 && input.mouseX <= clientRect.right-clientRect.left &&
    //         input.mouseY > 0 && input.mouseY <= clientRect.bottom-clientRect.top)
    //     {
    //         // enable drag
    //         if (input.mouseX > g_cw-SCROLL_WIDTH)// &&
    //             // input.mouseX < g_sw-GetSystemMetrics(SM_CXFRAME)) // don't register clicks in resize area
    //         {
    //             if (input.mouseL && !is_dragging_scrollbar) {
    //                 is_dragging_scrollbar = true;
    //             }
    //         }
    //         // disable drag
    //         if (!input.mouseL && is_dragging_scrollbar) {
    //             is_dragging_scrollbar = false;
    //         }
    //         // drag processing
    //         if (is_dragging_scrollbar) {
    //             float percent = (float)input.mouseY / (float)g_ch;
    //             MASTER_SCROLL = (percent*total_height);
    //         }
    //     }
    // } // end mouse handling


    // // limit camera to content area
    // if (MASTER_SCROLL+ch > tile_height) MASTER_SCROLL = tile_height-ch;
    // if (MASTER_SCROLL < 0) MASTER_SCROLL = 0;  // do this last so we default to "display from top"


    // adjust rects for camera position (basically)
    // for (int j = 0; j < stubRectCount; j++) {
    for (int j = 0; j < pool->count; j++) {
        pool->pool[j].pos.y -= MASTER_SCROLL;
    }

    // // set new anchor point / amt_off for next frame
    // float smallest_delta = ch; // just some big seed value
    // // for (int j = 0; j < stubRectCount; j++) {
    // for (int i = 0; i < pool_ref.count; i++) {
    //     float delta = fabs(pool_ref[i].pos.y);    // works because y==0 means top of screen (after we add camera offset)
    //     if (delta < smallest_delta) {
    //         smallest_delta = delta;
    //         state_anchor_item = i;
    //         state_amt_off_anchor = -pool_ref[i].pos.y;
    //     }
    // }
}

