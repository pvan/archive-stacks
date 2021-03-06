



// assuming a canvas of 0,0,dw,dh, returns subrect (t/l/r/b it seems) filling that space but with aspect_ratio (w/h)
rect calc_pixel_letterbox_subrect(int dw, int dh, float aspect_ratio)
{
    float calcW = (float)dh * aspect_ratio;
    float calcH = (float)dw / aspect_ratio;
    if (calcW > dw) calcW = dw;  // letterbox
    else calcH = dh;  // pillarbox
    float posX = ((float)dw - calcW) / 2.0;
    float posY = ((float)dh - calcH) / 2.0;
    return {posX, posY, posX+calcW, posY+calcH};
}




struct tile
{

    // position/shape in list/"world"
    v2 pos;
    v2 size;

    bool skip_rendering; // created for not drawing unselected tiles

    // file data
    // string fullpath;
    // item paths_just_for_comparing_tiles; // just used for == (for tile_pool)
    // string name; // just for printing i think
    // u64 modifiedTimeSinceLastEpoc;

    // thumbnail data
    // string thumbpath;


    // media data
    // v2 resolution;
    ffmpeg_media media;
    bool needs_loading = false;
    bool needs_unloading = false;
    bool is_media_loaded = false;

    // state for rendering
    bool texture_updated_since_last_read = false; // set true when changing texture, false when reading texture
    // bool has_display_quad = false; // already assigned a display quad (set/unset based on on/offscreen at render time)
    // int display_quad_index = 0; // only used if has_display_quad true

    // debug stuff
    u32 rand_color;
    u32 red_color = 0xffff0000;

    // for rendering
    opengl_quad display_quad;
    bool display_quad_created;
    bool display_quad_texture_sent_to_gpu = false;


    void LoadMedia(string frompath) {
        if (is_media_loaded) { OutputDebugString("tile media already loaded!\n"); return; }
        if (frompath.count < 1) { OutputDebugString("tile media path not set yet!\n"); return; }
        media.LoadFromFile(frompath);
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


    bool operator== (tile o) { return pos==o.pos && size==o.size;}// && name==o.name; }


};




DEFINE_TYPE_POOL(tile);  // missing "tile_pile" opportunity here




// todo: audit the usage of limits, do we need a max?
float MIN_TILE_WIDTH = 50;

float CalcWidthToFitXColumns(int col_count, int container_width) {
    if (col_count < 1) col_count = 1;
    if (col_count > container_width) col_count = container_width; // 1 pixel result.. hmm
    float width = (float)container_width / (float)col_count;
    width = fmax(width, MIN_TILE_WIDTH);
    return width;
}

// given a desired item width,
// return the number of columns needed to fill container_width
// also pass back the resulting actual width in *actual_width (if passed given a non-null pointer)
int CalcColumnsNeededForDesiredWidth(float desired_width, int container_width, float *actual_width = 0) {
    desired_width = fmax(desired_width, MIN_TILE_WIDTH);

    int cols = container_width / desired_width;
    if (cols < 1) cols = 1;
    if (actual_width) *actual_width = (float)(container_width) / (float)cols;
    return cols;
}

// int ArrangeTilesInOrder(tile_pool *pool, float desired_tile_width, int dest_width) {

//     desired_tile_width = fmax(desired_tile_width, MIN_TILE_WIDTH);
//     float realWidth;
//     int cols = CalcColumnsNeededForDesiredWidth(desired_tile_width, dest_width, &realWidth);

//     float *colbottoms = (float*)malloc(cols*sizeof(float));
//     for (int c = 0; c < cols; c++)
//         colbottoms[c] = 0;

//     // for (int j = 0; j < stubRectCount; j++) {
//     for (int i = 0; i < pool->count; i++) {
//         tile& thisTile = pool->pool[i];
//         v2& thisRes = item_resolutions[i];

//         // todo: should be set to min of 10 from trying to find resolution originally
//         // assert(thisTile.size.w >= 10 && thisTile.size.h >= 10);
//         if (thisRes.w < 10) thisRes.w = 10;
//         if (thisRes.h < 10) thisRes.h = 10;

//         float aspect_ratio = thisRes.w / thisRes.h;

//         thisTile.size.w = realWidth;
//         thisTile.size.h = realWidth / aspect_ratio;

//         // if (thisTile.size.w > 3000 || thisTile.size.h > 3000) {
//         //     assert(false);
//         // }

//         int shortestCol = 0;
//         for (int c = 1; c < cols; c++)
//             if (colbottoms[c] < colbottoms[shortestCol])
//                 shortestCol = c;

//         float x = shortestCol * realWidth;
//         float y = colbottoms[shortestCol];

//         thisTile.pos = {x, y};

//         colbottoms[shortestCol] += thisTile.size.h;
//     }



//     float total_height;
//     total_height = 0;
//     int longestCol = 0;
//     for (int c = 1; c < cols; c++)
//         if (colbottoms[c] > colbottoms[longestCol])
//             longestCol = c;
//     total_height = colbottoms[longestCol];

//     free(colbottoms);


//     return total_height;

// }



int TileIndexMouseIsOver(tile_pool ts, int mouseX, int mouseY) {
    for (int i = 0; i < ts.count; i++) {
        if (ts[i].skip_rendering) continue; // this is set when tile is not selected for display
        rect tile_rect = {ts[i].pos.x, ts[i].pos.y, ts[i].size.w, ts[i].size.h};
        if (tile_rect.ContainsPoint(mouseX, mouseY)) {
            return i;
        }
    }
    return 0; // mouse is not over a tile... any special handling needed? hmm
}

int CalculateScrollPosition(int last_scroll_pos, int mwheeldelta, input_keystate keysdown, int ch, int tiles_height) {

    int result = last_scroll_pos + mwheeldelta;

    // input that might affect position
    if (keysdown.pgDown) { result += ch; }
    if (keysdown.pgUp) { result -= ch; }

    // limit camera to content area
    if (result+ch > tiles_height) result = tiles_height-ch;
    if (result < 0) result = 0;  // do this last so we default to "display from top"

    return result;
}

// int state_amt_off_anchor = 0;
// int state_anchor_item = 0;
// void ShiftTilesBasedOnScrollPosition(tile_pool *pool, int *scroll_delta, Input keysDown, int ch, int tile_height) {

// consider: replace this with a single transform uniform in the shader?
void ShiftTilesBasedOnScrollPosition(tile_pool *pool, int scroll_position) {

    // adjust rects for camera position (basically)
    // for (int j = 0; j < stubRectCount; j++) {
    for (int j = 0; j < pool->count; j++) {
        pool->pool[j].pos.y -= scroll_position;
    }

//     if (*scroll_delta != 0) {
//         state_amt_off_anchor += *scroll_delta;
//         *scroll_delta = 0;
//     }

//     // convert anchor point / amt_off to pixels down the camera is
//     // note we use ANCHOR_ITEM not dispalyList[ANCHOR_ITEM] because
//     // we want to tie to the actual item, not it's position in the list (or we could change it)
//     // tile_pool &pool_ref = *pool;
//     // tile *anchor_tile = &pool_ref[state_anchor_item];
//     // int MASTER_SCROLL = (anchor_tile->pos.y + state_amt_off_anchor);

// static int MASTER_SCROLL = 0;
// MASTER_SCROLL += state_amt_off_anchor;
// state_amt_off_anchor = 0;

//     // // input that might affect position
//     // if (keysDown.pgDown) { MASTER_SCROLL += ch; }
//     // if (keysDown.pgUp) { MASTER_SCROLL -= ch; }


//     // // MOUSE   scrollbar
//     // {
//     //     // skip any proc if outside client area
//     //     RECT clientRect; GetClientRect(g_hwnd, &clientRect);
//     //     if (input.mouseX > 0 && input.mouseX <= clientRect.right-clientRect.left &&
//     //         input.mouseY > 0 && input.mouseY <= clientRect.bottom-clientRect.top)
//     //     {
//     //         // enable drag
//     //         if (input.mouseX > g_cw-SCROLL_WIDTH)// &&
//     //             // input.mouseX < g_sw-GetSystemMetrics(SM_CXFRAME)) // don't register clicks in resize area
//     //         {
//     //             if (input.mouseL && !is_dragging_scrollbar) {
//     //                 is_dragging_scrollbar = true;
//     //             }
//     //         }
//     //         // disable drag
//     //         if (!input.mouseL && is_dragging_scrollbar) {
//     //             is_dragging_scrollbar = false;
//     //         }
//     //         // drag processing
//     //         if (is_dragging_scrollbar) {
//     //             float percent = (float)input.mouseY / (float)g_ch;
//     //             MASTER_SCROLL = (percent*total_height);
//     //         }
//     //     }
//     // } // end mouse handling


//     // // limit camera to content area
//     // if (MASTER_SCROLL+ch > tile_height) MASTER_SCROLL = tile_height-ch;
//     // if (MASTER_SCROLL < 0) MASTER_SCROLL = 0;  // do this last so we default to "display from top"


//     // adjust rects for camera position (basically)
//     // for (int j = 0; j < stubRectCount; j++) {
//     for (int j = 0; j < pool->count; j++) {
//         pool->pool[j].pos.y -= MASTER_SCROLL;
//     }

//     // // set new anchor point / amt_off for next frame
//     // float smallest_delta = ch; // just some big seed value
//     // // for (int j = 0; j < stubRectCount; j++) {
//     // for (int i = 0; i < pool_ref.count; i++) {
//     //     float delta = fabs(pool_ref[i].pos.y);    // works because y==0 means top of screen (after we add camera offset)
//     //     if (delta < smallest_delta) {
//     //         smallest_delta = delta;
//     //         state_anchor_item = i;
//     //         state_amt_off_anchor = -pool_ref[i].pos.y;
//     //     }
//     // }
}


tile_pool tiles; // access from background thread as well



