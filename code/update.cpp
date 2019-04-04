

// for panning VIEW mode
bool mouse_up_once_since_loading = false;
float clickMouseX;
float clickMouseY;
float clickRectX;
float clickRectY;


//
// button click handlers

bool tag_menu_open = false;  // is tag menu open in browsing mode?
void ToggleTagMenu() { tag_menu_open = !tag_menu_open; }
void ToggleTagBrowse(int tagindex) {
    // int tagindex = (u64)disguisedint;
    if (browse_tag_indices.has(tagindex)) {
        browse_tag_indices.remove(tagindex);
    } else {
        browse_tag_indices.add(tagindex);
    }

    // update display list here
    CreateDisplayListFromBrowseSelection();

    // do in loop for now, just before rendering (or, well, in the update portion)
    // ArrangeTilesForDisplayList(display_list, &tiles, master_desired_tile_width, g_cw); // requires resolutions to be set
}
void SelectBrowseTagsAll() { SelectAllBrowseTags(); CreateDisplayListFromBrowseSelection(); }
void SelectBrowseTagsNone() { DeselectAllBrowseTags(); CreateDisplayListFromBrowseSelection(); }

bool tag_select_open = false;  // is tag menu open in viewing mode?
void ToggleTagSelectMenu() { tag_select_open = !tag_select_open; }
// we assume we're affecting the open / "viewing" file here
void ToggleTagSelection(int tagindex) {
    // u64 tagindex = (u64)disguisedint;
    if (items[viewing_file_index].tags.has(tagindex)) {
        items[viewing_file_index].tags.remove(tagindex);
    } else
    {
        items[viewing_file_index].tags.add(tagindex);
    }
    SaveMetadataFile(); // keep close eye on this, if it gets too slow to do inline we can move to background thread
}
void SelectItemTagsAll() {
    if (filtered_view_tag_indices.count == 0) {
        items[viewing_file_index].tags.empty_out();
        for (int i = 0; i < tag_list.count; i++) {
            items[viewing_file_index].tags.add(i);
        }
    } else {
        // now just affect visible (filtered) tags
        for (int ft = 0; ft < filtered_view_tag_indices.count; ft++) {
            int tagi = filtered_view_tag_indices[ft];
            if (!items[viewing_file_index].tags.has(tagi))
                items[viewing_file_index].tags.add(tagi);
        }
    }
}
void SelectItemTagsNone() {
    if (filtered_view_tag_indices.count == 0) {
        items[viewing_file_index].tags.empty_out();
    } else {
        // now just affect visible (filtered) tags
        for (int ft = 0; ft < filtered_view_tag_indices.count; ft++) {
            int tagi = filtered_view_tag_indices[ft];
            if (items[viewing_file_index].tags.has(tagi))
                items[viewing_file_index].tags.remove(tagi);
        }
    }
}

void OpenFileToView(int item_index) {
    app_mode = VIEWING_FILE;

    viewing_file_index = item_index;

    // // note shallow copy, not pointer or deep copy
    // viewing_tile = tiles[viewing_file_index];

    viewing_tile = tile::CreateFromItem(items[viewing_file_index]);

    viewing_tile.needs_loading = true;
    viewing_tile.needs_unloading = false; // these are init to false in tile::Create,
    viewing_tile.is_media_loaded = false; // but just to make it explicit here..

    float aspect_ratio = item_resolutions[viewing_file_index].aspect_ratio();
    rect fit_to_screen = calc_pixel_letterbox_subrect(g_cw, g_ch, aspect_ratio);
    // note rect seems to be TLBR not XYWH
    viewing_tile.pos = {fit_to_screen.x, fit_to_screen.y};
    viewing_tile.size = {fit_to_screen.w-fit_to_screen.x, fit_to_screen.h-fit_to_screen.y};

    mouse_up_once_since_loading = false;

    // move all the tile buttons so we can't click on them
    // better way to do this?
    for (int i = 0; i < tiles.count; i++) {
        tiles[i].pos = {-1000,-1000};
        tiles[i].size = {0,0};
    }

}
void OpenFileToView(void *disguisedint) {
    u64 item_index = (u64)disguisedint;
    OpenFileToView(item_index);
}






// helper function for DrawTagMenu
// pass actually_draw_buttons = false to just test the layout
// will return if the layout overruns the screen
// todo: is this starting to smell a little?
bool DrawTagsWithXColumns(int totalcols,
                             bool render,
                             float_pool widths,
                             float hgap,
                             float vgap,
                             int cw, int ch,
                             void (*tagSelect)(int),
                             int_pool *selected_tags_pool,
                             int_pool filtered_tag_indices)
{
    // algorithm is a bit quirky
    // basically we put all the widths in place for one column
    // then find the biggest gap between two widths in the largest X items (X=max_overrun_count)
    // that's where we create the column cutoff point (with the larger items running into the next column)
    // also, the gap has to be at least Y big to count (to avoid tiny little overruns) (Y=min_overrun_amount)
    //
    // so you might have a natural break with the 3rd largest item,
    // but if the largest is so much bigger than the next two, that's what will be used
    //
    // todo: test extreme values (half done when debugging for filtering)
    // todo: could maybe improve the result of this if instead of looking at max gap,
    // we look at some kind of area calculation that looks at gap*number of overrun items
    //
    // todo: evaluate the final result of how this is working
    // (better way to do this? tweak the algo? or different approach?)
    // feels like there's frequently just 1 or 2 items dangling in the last column,
    // or a lot of space on the right end of the screen

    if (filtered_tag_indices.count == 0) {
        if (render) {
            float x = 0;
            float y = (0+1) * (UI_TEXT_SIZE+vgap);
            ui_text("*no tags matching filter*", {x,y}, UI_LEFT,UI_TOP, true, 0.66, 0xffc0ffee);
        }
        return false; // within screen
    }

    int max_rows_per_col = (filtered_tag_indices.count + totalcols-1) / totalcols; // basically round up
    assert(max_rows_per_col>0);
    int max_overrun_count = 3; // max number of items to allow to overrun per column
    if (max_overrun_count > max_rows_per_col-1) max_overrun_count=max_rows_per_col-1; // not sure if needed with new "never skip the smallest width" and "don't skip anything on 1 row columns" checks done below
    float min_overrun_amount = 10+hgap;//hgap*2; // how far into next col we need to be

    // list of column widths for columns created so far
    int_pool colwidths = int_pool::new_empty();
    colwidths.add(0); // first column (more added each new column)

    // width of every item in current column (and row number of it)
    intfloatpair_pool widthsincol = intfloatpair_pool::new_empty();

    // list of row indicies (eg col 2 second item is row=1)
    // to skip over on this column (because last column item overruns into this spot)
    // update: keep widths included here so we can calculate if we need to double-overrun an item
    intfloatpair_pool skiprows = intfloatpair_pool::new_empty();

    int col = 0;
    int row = 0;
    float thiscolX = 0;
    for (int ft = 0; ft < filtered_tag_indices.count; ft++, row++) { // note row++
        int t = filtered_tag_indices[ft];
        if (row >= max_rows_per_col) {  // end of a row

            // this is the heart of the algorithm,
            // the question is: where do we end the column, and what items do we skip next column?
            float largestgap = 0;
            int skipcount = 0;
            if (row != 1) { // don't skip anything if col is only 1 row deep
                sort_intfloatpair_pool_high_to_low(&widthsincol);

                // find largest gap in widths of the largest X items (X=max_overrun_count)
                // gap has to be at least Y big to count (Y=min_overrun_amount)
                for (int i = 0; i<max_overrun_count && i<widthsincol.count-1; i++) {
                    float thisgap = widthsincol[i].f - widthsincol[i+1].f;
                    // todo: or maybe we should just go all out and calculate the whitespace of the whole column
                    // and calculate the whitespace delta for each possible number of cutoff items
                    // // could try something like this.. weight the potential new space by the number of potential new items
                    // float thisgap = (widthsincol[i].f - widthsincol[i+1].f) * ((i+1)-skipcount); // kind of area of (new width)*(num of newly added)
                    if (
                        thisgap > largestgap &&  // ignore this to overrun as many items as possible
                        thisgap > min_overrun_amount) // don't treat as gap if not this big
                    {
                        largestgap = thisgap;
                        skipcount = i+1; // recall widths are sorted large->small
                    }
                }
                skiprows.empty_out(); // what row spots to skip for the next columns
                for (int i = 0; i < skipcount; i++) {
                    if (i != widthsincol.count-1) // never skip the smallest width
                        skiprows.add(widthsincol[i]);
                }
            }
            colwidths[col] = widthsincol[skipcount].f + hgap; // final width of that column

            // print debug info at bottom of each row
            // if (render)
            // {
            //     float x = thiscolX;
            //     float y = (row+1) * (UI_TEXT_SIZE+vgap);
            //     char buf[256];
            //     sprintf(buf, "%i", largestgapcount);
            //     // ui_button(buf, x,y, UI_LEFT,UI_TOP, 0, 0);
            //     ui_text(buf, {x,y}, UI_LEFT,UI_TOP, true, 0.66, 0xffc0ffee);
            //     sprintf(buf, "%0.f", largestgap);
            //     // ui_button(buf, x,y+UI_TEXT_SIZE, UI_LEFT,UI_TOP, 0, 0);
            //     ui_text(buf, {x,y+UI_TEXT_SIZE}, UI_LEFT,UI_TOP, true, 0.66, 0xffc0ffee);
            // }

            // prep for next column
            widthsincol = intfloatpair_pool::new_empty(); // empty widths for use in next column
            colwidths.add(0); // first column (more added each new column)

            row = 0;
            col++;

            thiscolX = 0;
            for (int c = 0; c < col; c++)
                thiscolX += colwidths[c]; // sum of all previous columns
        }

        // check for end of screen before potentially skipping this row (not sure if matters)
        if (thiscolX + widths[ft] > cw) {
            // we're bleeding off screen right,
            // we found our max allowable column count (1 before this one)

            // if we overrun screen in this case, something went wrong
            // (caller handles now)
            // assert(!render);

            // // debug print bb rect
            // {
            //     float w = (float)thiscolX+widths[ft];
            //     float h = (float)max_rows_per_col*UI_TEXT_SIZE;
            //     ui_rect({0,UI_TEXT_SIZE,w,h}, rand_col(w+h*100), .8);
            //     char buf[256];
            //     sprintf(buf, "%i->%i", totalcols, col);
            //     ui_text(buf, {(float)cw,h}, UI_RIGHT,UI_TOP, true, .5);
            // }

            colwidths.free_all();
            widthsincol.free_all();
            skiprows.free_all();
            return true;
        }

        // skip spots where there are overrun items in the last column
        int index_of_this_row_skip = intfloatpair_pool_has_int(skiprows, row);
        if (index_of_this_row_skip >= 0) {
            float widthinthiscol = skiprows[index_of_this_row_skip].f - colwidths[col-1]; // todo: test: [col-1] should be fine since col 0 doesn't have any skips
            widthsincol.add({row,widthinthiscol}); // in case double overrun
            ft--;// awkward! we need to stay on this tag index in the next iteration
            continue;
        }
        widthsincol.add({row,widths[ft]});


        // actually draw the buttons (skip this when searching for now many columns we need)
        if (render)
        {
            float x = thiscolX;
            float y = (row+1) * (UI_TEXT_SIZE+vgap);

            rect brect;
            if (ui_button_text(&tag_list[t], tag_list[t].ToUTF8Reusable(), {x,y}, UI_LEFT,UI_TOP, &brect)) {
                if (tagSelect) tagSelect(t);
            }

            if (selected_tags_pool->has(t)) {
                ui_rect(brect, 0xffff00ff, 0.3);
            }
        }

    } // iterated through last tag

    // // debug print bb rect
    // {
    //     float finalX = 0;
    //     for (int c = 0; c < col; c++)
    //         finalX += colwidths[c]; // sum of all previous columns
    //     float maxw = 0;
    //     for (int r = 0; r < widthsincol.count; r++)
    //         if (widthsincol[r].f > maxw) maxw = widthsincol[r].f;
    //     finalX+=maxw;
    //     float w = (float)finalX;
    //     float h = (float)max_rows_per_col*(UI_TEXT_SIZE+vgap);
    //     ui_rect({0,UI_TEXT_SIZE+vgap,w,h}, rand_col(w+h*100), .3);
    //     char buf[256];
    //     sprintf(buf, "%i->%i", totalcols, col);
    //     ui_text(buf, {w,h}, UI_RIGHT,UI_TOP, true, .5);
    // }

    colwidths.free_all();
    widthsincol.free_all();
    skiprows.free_all();
    return false;
}

// can use for tag menu in browse mode (selecting what to browse)
// and tag menu in view mode (for selecting an item's tags)
// pass in the click handlers for the buttons
// and a list of ints which are the indices of the tags that are marked "selected" (color pink)
// also pass in list of indices into tag_list which are the tags not filtered out
void DrawTagMenu(int cw, int ch,
                 void (*selectNone)(),
                 void (*selectAll)(),
                 void (*tagSelect)(int),
                 void (*menuToggle)(),
                 bool menu_open,
                 int_pool *selected_tags_pool,
                 int_pool filtered_tag_indices,
                 newstring *tagfilter)
{

    if (!menu_open) {
        // ui_button("show tags", cw/2, 0, UI_CENTER,UI_TOP, &ToggleTagMenu);
        if (ui_button_text(&menuToggle, "show tags", {0, 0}, UI_LEFT,UI_TOP, 0)) {
            if (menuToggle) menuToggle();
        }
        return; // don't draw any more
    }

    rect hider;
    if (ui_button_text(&menuToggle, "hide tags", {0, 0}, UI_LEFT,UI_TOP, &hider)) {
        if (menuToggle) menuToggle();
    }

    ui_text("filter: ", {(float)cw/2-50,0}, UI_RIGHT,UI_TOP, true, 0.66);
    ui_textbox(tagfilter, tagfilter, {(float)cw/2-50,0,100,UI_TEXT_SIZE}, g_dt);
    if (ui_button_text((void*)'x', "x", {(float)cw/2+50,0}, UI_LEFT,UI_TOP, 0)) {
        tagfilter->count = 0;
    }

    float hgap = 12;
    float vgap = 3;

    rect lastr;
    if (ui_button_text(&selectNone, "select none", {hider.w+hgap,0}, UI_LEFT,UI_TOP, &lastr)) {
        if(selectNone) selectNone();
    }
    if (ui_button_text(&selectAll, "select all", {hider.w+lastr.w+hgap*2,0}, UI_LEFT,UI_TOP, 0)) {
        if(selectAll) selectAll();
    }


    // first get size of all our tags
    // note we are only finding widths on filtered tags (so index into widths is filtered tag index (ft) not tag index (t)
    float_pool widths = float_pool::new_empty();
    for (int i = 0; i < filtered_tag_indices.count; i++) {
        int t = filtered_tag_indices[i];
        rect r = ui_text(tag_list[t].ToUTF8Reusable(), {0,0}, UI_LEFT,UI_TOP, false, 0);
        widths.add(r.w);
    }


    // ok, try dividing into X columns, starting at 1 and going up until we're out of space

    int final_desired_cols = filtered_tag_indices.count;

    // upperbound is 1 col for every tag (note <= not <)
    for (int totalcols = 1; totalcols <= filtered_tag_indices.count; totalcols++) {
        if (DrawTagsWithXColumns(
            totalcols,
            false, // don't draw actual buttons, this is jsut a test of the layout with this many columns
            widths,
            hgap,
            vgap,
            cw, ch,
            tagSelect,
            selected_tags_pool,
            filtered_tag_indices))
        {
            final_desired_cols = totalcols-1;
            break;
        }
    }

    bool finalmenutoobig = DrawTagsWithXColumns(
            final_desired_cols,
            true, // now we draw the buttons
            widths,
            hgap,
            vgap,
            cw, ch,
            tagSelect,
            selected_tags_pool,
            filtered_tag_indices);

    assert(!finalmenutoobig);

}

// old method, just packs the buttons like words in a paragraph
// void DrawTagParagraphMenu_DEPRECATED(int cw, int ch,
//                  void (*selectNone)(void*),
//                  void (*selectAll)(void*),
//                  void (*tagSelect)(void*),
//                  int_pool *selected_tags_pool)
// {
//     float hgap = 3;
//     float vgap = 3;

//     float x = hgap;
//     float y = UI_TEXT_SIZE + vgap*2;

//     // get total size
//     {
//         for (int i = 0; i < tag_list.count; i++) {
//             rect this_rect = ui_text(tag_list[i].ToUTF8Reusable(), x,y, UI_LEFT,UI_TOP, false);
//             this_rect.w+=hgap;
//             this_rect.h+=vgap;
//             if (x+this_rect.w > cw) { y+=this_rect.h; x=hgap; }
//             x += this_rect.w;
//             if (i == tag_list.count-1) y += this_rect.h; // \n for last row
//         }
//     }
//     ui_rect_solid({0,0,(float)cw,y+UI_TEXT_SIZE+vgap}, 0xffffffff, 0.5); // menu bg

//     rect lastr = ui_button("select none", hgap,vgap, UI_LEFT,UI_TOP, selectNone);
//     ui_button("select all", lastr.w+hgap*2,vgap, UI_LEFT,UI_TOP, selectAll);
//     // could also do something like this
//     // if (mode == BROWSING_THUMBS) {
//     //     rect lastr = ui_button("select none", hgap,vgap, UI_LEFT,UI_TOP, &SelectItemTagsNone);
//     //     ui_button("select all", lastr.w+hgap*2,vgap, UI_LEFT,UI_TOP, &SelectItemTagsAll);
//     // } else if (mode == VIEWING_FILE) {
//     //     rect lastr = ui_button("select none", 0,0, UI_LEFT,UI_TOP, &SelectBrowseTagsNone);
//     //     ui_button("select all", lastr.w+hgap,0, UI_LEFT,UI_TOP, &SelectBrowseTagsAll);
//     // }

//     x = hgap;
//     y = UI_TEXT_SIZE + vgap*2;
//     for (int i = 0; i < tag_list.count; i++) {
//         rect this_rect = ui_text(tag_list[i].ToUTF8Reusable(), x,y, UI_LEFT,UI_TOP, false);
//         this_rect.w+=hgap;
//         this_rect.h+=vgap;
//         if (x+this_rect.w > cw) { y+=this_rect.h; x=hgap; }
//         rect brect = ui_button(tag_list[i].ToUTF8Reusable(), x,y, UI_LEFT,UI_TOP, tagSelect, i);

//         // color selected tags...
//         {
//             // if (items[viewing_file_index].tags.has(i)) {
//             if (selected_tags_pool->has(i)) {
//                 ui_rect(brect, 0xffff00ff, 0.3);
//             }
//         }

//         x += this_rect.w;
//         if (i == tag_list.count-1) y += this_rect.h; // \n for hide tag button
//     }
// }



// todo: we could use global dt here instead of passing in, same with cw/ch..
// or would it be better to go the other way -- pass dt and cw/ch to all children and try and remove global usage?

void browse_tick(float actual_dt, int cw, int ch) {


    // --update--

    // todo: find the right home for these vars

    static float last_scroll_pos = 0;

    static int debug_info_tile_index_mouse_was_on = 0;


    int tiles_height; // declare out here so we can use in our scrollbar figuring
    // position the tiles
    {
        // arrange every frame, in case window size changed
        // todo: could just arrange if window resize / tag settigns change / etc

        int col_count = CalcColumnsNeededForDesiredWidth(master_desired_tile_width, cw);

        if (master_ctrl_scroll_delta > 0) { col_count++; master_desired_tile_width = CalcWidthToFitXColumns(col_count, cw); }
        if (master_ctrl_scroll_delta < 0) { col_count--; master_desired_tile_width = CalcWidthToFitXColumns(col_count, cw); }
        master_ctrl_scroll_delta = 0; // done using this in this frame

        tiles_height = ArrangeTilesForDisplayList(display_list, &tiles, master_desired_tile_width, g_cw); // requires resolutions to be set
        // tiles_height = ArrangeTilesInOrder(&tiles, master_desired_tile_width, cw); // requires resolutions to be set

        int scroll_pos = CalculateScrollPosition(last_scroll_pos, master_scroll_delta, input.down, ch, tiles_height);
        last_scroll_pos = (float)scroll_pos;
        master_scroll_delta = 0; // done using this in this frame

        ShiftTilesBasedOnScrollPosition(&tiles, last_scroll_pos);

        debug_info_tile_index_mouse_was_on = TileIndexMouseIsOver(tiles, input.current.mouseX, input.current.mouseY);
    }


    for (int i = 0; i < tiles.count; i++) {
        if (tiles[i].IsOnScreen(ch)) {   // tile is on screen
            tiles[i].needs_loading = true;    // could use one flag "is_on_screen" just as easily probably
            tiles[i].needs_unloading = false;
        } else {
            tiles[i].needs_loading = false;
            tiles[i].needs_unloading = true;
        }
    }

    if (input.down.tab)
        ToggleTagMenu();




    // --render--

    for (u64 tileI = 0; tileI < tiles.count; tileI++) {
        tile& t = tiles[tileI];
        if (t.skip_rendering) continue; // this is set when tile is not selected for display
        if (t.IsOnScreen(ch)) { // only render if on screen

            if (!t.display_quad_created) {
                t.display_quad.create(0,0,1,1);
                t.display_quad_created = true;
            }

            if (!t.display_quad_texture_sent_to_gpu || t.texture_updated_since_last_read) {
                bitmap img = t.GetImage(actual_dt); // the bitmap memory should be freed when getimage is called again
                t.display_quad.set_texture(img.data, img.w, img.h);
                t.display_quad_texture_sent_to_gpu = true;
            }
            else if (t.is_media_loaded) {
                if (!t.media.IsStaticImageBestGuess()) {
                    // if video, just force send new texture every frame
                    // todo: i think a bug in playback speed here
                    //       maybe b/c if video is offscreen, dt will get messed up?
                    bitmap img = t.GetImage(actual_dt); // the bitmap memory should be freed when getimage is called again
                    t.display_quad.set_texture(img.data, img.w, img.h);
                    t.display_quad_texture_sent_to_gpu = true;
                }
            }

            t.display_quad.set_verts(t.pos.x, t.pos.y, t.size.w, t.size.h);
            t.display_quad.render(1);

            // add a button for this item
            rect r = {t.pos.x,t.pos.y, t.size.w,t.size.h};
            if (ui_button_rect((void*)(tileI+1), r, 0xffffffff, 0)) { //+1 so we never use 0 as an id
                OpenFileToView(tileI);
            }
        }
    }


    // scroll bar
    {
        float SCROLL_WIDTH = 25;

        float amtDown = last_scroll_pos; // note this is set above

        float top_percent = amtDown / (float)tiles_height;
        float bot_percent = (amtDown+ch) / (float)tiles_height;

        ui_scrollbar(&last_scroll_pos,
                     {cw-SCROLL_WIDTH, 0, SCROLL_WIDTH, (float)ch},
                     top_percent, bot_percent,
                     &last_scroll_pos,
                     tiles_height);

    }

    // tag menu (browse)
    {
        filtered_browse_tag_indices.empty_out();
        for (int i = 0; i < tag_list.count; i++) {
            // strstr returns pointer to substring in string
            // looks like strstr will work fine on utf8 strings
            if (strstr(tag_list[i].ToUTF8Reusable(), browse_tag_filter.to_utf8_reusable()) != 0) {
                filtered_browse_tag_indices.add(i);
            }
        }
        DrawTagMenu(cw, ch,
                    &SelectBrowseTagsNone,
                    &SelectBrowseTagsAll,
                    &ToggleTagBrowse,
                    &ToggleTagMenu,
                    tag_menu_open,
                    &browse_tag_indices,
                    filtered_browse_tag_indices,
                    &browse_tag_filter);
    }

    if (ui_button_text("settings", "settings", {(float)cw,0}, UI_RIGHT,UI_TOP, 0)) {
        app_mode = SETTINGS;
        // todo: make app mode switching into function?
    }


    // debug display metrics
    if (show_debug_console)
    {
        UI_PRINTRESET();

        static int debug_pip_count = 0;
        debug_pip_count = (debug_pip_count+1) % 32;
        char debug_pip_string[32 +1/*null terminator*/] = {0};
        for (int i = 0; i < 32; i++) { debug_pip_string[i] = '.'; }
        debug_pip_string[debug_pip_count] = ':';
        UI_PRINT(debug_pip_string);

        UI_PRINT("dt: %f", actual_dt);

        UI_PRINT("max dt: %f", metric_max_dt);

        UI_PRINT("ms to first frame: %f", metric_time_to_first_frame);

        int metric_media_loaded = 0;
        for (int i = 0; i < tiles.count; i++) {
            if (tiles[i].media.loaded) metric_media_loaded++;
        }
        UI_PRINT("media loaded: %i", metric_media_loaded);

        UI_PRINT("tiles: %i", tiles.count);
        UI_PRINT("items: %i", items.count);

        UI_PRINT("mouse: %f, %f", input.current.mouseX, input.current.mouseY);
        // UI_PRINT("mouseY: %f", input.mouseY);


        // UI_PRINT("amt off: %i", state_amt_off_anchor);


        // for (int i = 0; i < tiles.count; i++) {
        //     rect tile_rect = {tiles[i].pos.x, tiles[i].pos.y, tiles[i].size.w, tiles[i].size.h};
        //     if (tile_rect.ContainsPoint(input.mouseX, input.mouseY)) {

                // can just used our value calc'd above now:
                int i = debug_info_tile_index_mouse_was_on;
                UI_PRINT("tile_index_mouse_was_on: %i", debug_info_tile_index_mouse_was_on);

                UI_PRINT("name: %s", tiles[i].name.ToUTF8Reusable());

                if (tiles[i].media.vfc && tiles[i].media.vfc->iformat)
                    UI_PRINT("format name: %s", (char*)tiles[i].media.vfc->iformat->name);

                UI_PRINT("fps: %f", (float)tiles[i].media.fps);

                // UI_PRINT("frames: %f", (float)tiles[i].media.durationSeconds);
                // UI_PRINT("frames: %f", (float)tiles[i].media.totalFrameCount);

                if (tiles[i].media.vfc && tiles[i].media.vfc->iformat)
                    UI_PRINT(tiles[i].media.IsStaticImageBestGuess() ? "image" : "video");

                // wc *directory = CopyJustParentDirectoryName(items[i].fullpath.chars);
                // string temp = string::KeepMemory(directory);
                // UI_PRINT("folder: %s", temp.ToUTF8Reusable());
                // free(directory);

                // UI_PRINT("tile size: %f, %f", tiles[i].size.w, tiles[i].size.h);

                // // items and tiles share a common index
                // if (items[i].tags.count>0) {
                //     int firsttagI = items[i].tags[0];
                //     if (tag_list.count>0) {
                //         UI_PRINT("tag0: %s", tag_list[firsttagI].ToUTF8Reusable());
                //     } else {
                //         UI_PRINT("tag0: [no master list?!]");
                //     }
                // } else {
                //     UI_PRINT("tag0: none");
                // }

            UI_PRINT("browse tags: %i", browse_tag_indices.count);
            UI_PRINT("display list: %i", display_list.count);

            // int skiprender_count = 0;
            // // for (int j = 0; j < stubRectCount; j++) {
            // for (int i = 0; i < tiles.count; i++) {
            //     if (tiles[i].skip_rendering)
            //         skiprender_count++;
            // }
            // UI_PRINT("skiprend: %i", skiprender_count);


        //     }
        // }

        // char buf[235];
        // sprintf(buf, "dt: %f\n", actual_dt);
        // OutputDebugString(buf);
    }
}



void view_tick(float actual_dt, int cw, int ch) {

    // --update--

    if (input.down.right || input.down.left) {
        // find position in display list
        // (seems a little awkward...)
        u64 display_index_of_view_item;
        for (int i = 0; i < display_list.count; i++) {
            if (viewing_file_index == display_list[i]) {
                display_index_of_view_item = i;
            }
        }

        // increment
        if (input.down.right) {
            display_index_of_view_item = (display_index_of_view_item+1) % display_list.count;
        }
        if (input.down.left) {
            display_index_of_view_item = (display_index_of_view_item-1+display_list.count) % display_list.count;
        }

        // set new item
        //viewing_file_index = display_list[display_index_of_view_item];
        OpenFileToView(display_list[display_index_of_view_item]);
    }

    if (input.down.mouseR) {
        app_mode = BROWSING_THUMBS;
        // todo: remove tile stuff from memory or gpu ?
    }
    if (input.down.tab)
        ToggleTagSelectMenu();


    // pan
    if (!input.current.mouseL) mouse_up_once_since_loading = true;
    if (input.down.mouseM || input.down.mouseL) {
        clickMouseX = input.current.mouseX;
        clickMouseY = input.current.mouseY;
        clickRectX = viewing_tile.pos.x;
        clickRectY = viewing_tile.pos.y;
    }
    if ((input.current.mouseM || input.current.mouseL) && mouse_up_once_since_loading) {
        float deltaX = input.current.mouseX - clickMouseX;
        float deltaY = input.current.mouseY - clickMouseY;
        viewing_tile.pos.x = clickRectX + deltaX;
        viewing_tile.pos.y = clickRectY + deltaY;
    }

    // zoom
    if (master_scroll_delta != 0) {

        float scaleFactor = 0.8f;
        if (master_scroll_delta < 0) scaleFactor = 1.2f;
        viewing_tile.size.w *= scaleFactor;
        viewing_tile.size.h *= scaleFactor;
        viewing_tile.pos.x -= (input.current.mouseX - viewing_tile.pos.x) * (scaleFactor - 1);
        viewing_tile.pos.y -= (input.current.mouseY - viewing_tile.pos.y) * (scaleFactor - 1);

        master_scroll_delta = 0; // done using this this frame (todo: should put mousewheel in Input class)
    }



    // --render--

    tile *t = &viewing_tile;

    // render tile
    {
        if (!t->display_quad_created) {
            t->display_quad.create(0,0,1,1);
            t->display_quad_created = true;
        }

        if (!t->display_quad_texture_sent_to_gpu || t->texture_updated_since_last_read) {
            bitmap img = t->GetImage(actual_dt); // the bitmap memory should be freed when getimage is called again
            t->display_quad.set_texture(img.data, img.w, img.h);
            t->display_quad_texture_sent_to_gpu = true;
        }
        else if (t->is_media_loaded) {
            if (!t->media.IsStaticImageBestGuess()) {
                // if video, just force send new texture every frame
                // todo: i think a bug in playback speed here
                //       maybe b/c if video is offscreen, dt will get messed up?
                bitmap img = t->GetImage(actual_dt); // the bitmap memory should be freed when getimage is called again
                t->display_quad.set_texture(img.data, img.w, img.h);
                t->display_quad_texture_sent_to_gpu = true;
            }
        }

        t->display_quad.set_verts(t->pos.x, t->pos.y, t->size.w, t->size.h);
        t->display_quad.render(1);
    }


    // select tag menu (view)
    {
        filtered_view_tag_indices.empty_out();
        for (int i = 0; i < tag_list.count; i++) {
            // strstr returns pointer to substring in string
            // todo: not sure if works on utf8 strings
            if (strstr(tag_list[i].ToUTF8Reusable(), view_tag_filter.to_utf8_reusable()) != 0) {
                filtered_view_tag_indices.add(i);
            }
        }
        DrawTagMenu(cw, ch,
                    &SelectItemTagsNone,
                    &SelectItemTagsAll,
                    &ToggleTagSelection,
                    &ToggleTagSelectMenu,
                    tag_select_open,
                    &items[viewing_file_index].tags,
                    filtered_view_tag_indices,
                    &view_tag_filter);
    }


    // close button
    if (ui_button_text("close X", "close X", {(float)cw,0}, UI_RIGHT,UI_TOP, 0)) {
        app_mode = BROWSING_THUMBS;
        // todo: make app mode switching into function?
    }


    if (show_debug_console) {

        // debug tag count on right
        char buf[256];
        sprintf(buf, "%i", items[viewing_file_index].tags.count);
        ui_text(buf, {(float)cw,(float)ch}, UI_RIGHT,UI_BOTTOM, true, 0.66);

        // debug tag list on left
        float x = 0;
        for (int i = 0; i < items[viewing_file_index].tags.count; i++) {
            rect lastrect = ui_text(tag_list[items[viewing_file_index].tags[i]].ToUTF8Reusable(), {x,(float)ch}, UI_LEFT,UI_BOTTOM, true, 0.66);
            x+=lastrect.w;
        }


        UI_PRINTRESET();
        UI_PRINT("dt: %f", actual_dt);
        UI_PRINT("max dt: %f", metric_max_dt);




    }
}


newstring proposed_master_path = newstring::allocate_new(256);

void settings_tick(float actual_dt, int cw, int ch) {

    float textboxwidth = 300;
    float x = (float)cw/2 - textboxwidth/2;
    float y = (float)ch/4;
    ui_text("directory: ", {x,y-UI_TEXT_SIZE}, UI_LEFT,UI_TOP, true, 0);
    ui_textbox(&proposed_master_path, &proposed_master_path, {x,y,textboxwidth,UI_TEXT_SIZE}, actual_dt);
    if (ui_button_text((void*)'x', "x", {x+textboxwidth,y}, UI_LEFT,UI_TOP, 0)) {
        proposed_master_path.count = 0;
    }
    if (ui_button_text("browse","browse", {x+textboxwidth+40,y}, UI_LEFT,UI_TOP, 0)) {
        win32_OpenFolderSelectDialog(g_hwnd, &proposed_master_path);
    }

    if (win32_PathExists(proposed_master_path)) {
        if (win32_IsDirectory(proposed_master_path)) {
            ui_text("directory found", {x,y+UI_TEXT_SIZE}, UI_LEFT,UI_TOP, true, 0, 0xff00ff00);
        } else {
            ui_text("path not directory", {x,y+UI_TEXT_SIZE}, UI_LEFT,UI_TOP, true, 0, 0xff0000ff);
        }
    } else {
        ui_text("path not found", {x,y+UI_TEXT_SIZE}, UI_LEFT,UI_TOP, true, 0, 0xff0000ff);
    }


    // ui_texti("media files: %i", all_items.count, cw/2, ch/2 + UI_TEXT_SIZE*line++, UI_CENTER,UI_CENTER);

    // line++;
    // ui_texti("items_without_matching_thumbs: %i", item_indices_without_thumbs.count, cw/2, ch/2 + UI_TEXT_SIZE*line++, UI_CENTER,UI_CENTER);

    // line++;
    // ui_texti("items_without_matching_metadata: %i", item_indices_without_metadata.count, cw/2, ch/2 + UI_TEXT_SIZE*line++, UI_CENTER,UI_CENTER);

    // line++;
    // ui_text(loading_status_msg, cw/2, ch/2 + UI_TEXT_SIZE*line++, UI_CENTER,UI_CENTER);
    // ui_texti("files: %i of %i", loading_reusable_count, loading_reusable_max, cw/2, ch/2 + UI_TEXT_SIZE*line++, UI_CENTER,UI_CENTER);


    // ui_render_elements(0, 0); // pass mouse pos for highlighting
    // ui_reset(); // call at the end or start of every frame so buttons don't carry over between frames



    if (ui_button_text("close x", "close x", {(float)cw,0}, UI_RIGHT,UI_TOP, 0)) {
        app_mode = BROWSING_THUMBS;
        // todo: make app mode switching into function?
    }

}




