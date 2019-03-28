


void browse_tick(float actual_dt, int cw, int ch, Input input, Input keysDown, Input keysUp) {


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

        int scroll_pos = CalculateScrollPosition(last_scroll_pos, master_scroll_delta, keysDown, ch, tiles_height);
        last_scroll_pos = (float)scroll_pos;
        master_scroll_delta = 0; // done using this in this frame

        ShiftTilesBasedOnScrollPosition(&tiles, last_scroll_pos);

        debug_info_tile_index_mouse_was_on = TileIndexMouseIsOver(tiles, input.mouseX, input.mouseY);
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

    if (keysDown.space)
        ToggleTagMenu(0);




    // --render--

    opengl_clear();

    for (int tileI = 0; tileI < tiles.count; tileI++) {
        tile *t = &tiles[tileI];
        if (t->skip_rendering) continue; // this is set when tile is not selected for display
        if (t->IsOnScreen(ch)) { // only render if on screen

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
    }


    // make invisible button on top of mouse-overed tile
    // consider: what do you think about this, seems a little funny
    if (input.point_in_client_area(cw,ch)) { // todo: where to put this check, here or with all click handling?
        viewing_file_index = TileIndexMouseIsOver(tiles, input.mouseX, input.mouseY);
        // if (tiles[viewing_file_index].IsOnScreen(ch)) {
        rect r = {tiles[viewing_file_index].pos.x, tiles[viewing_file_index].pos.y,
                  tiles[viewing_file_index].size.w, tiles[viewing_file_index].size.h};
        ui_button_permanent_highlight(r, &OpenFileToView, viewing_file_index);

        // nevermind, still need some sort of ordering or we'll hl even when hovering over hud
        // // highlight here so HUD is drawn overtop highlight rect
        // ui_highlight(r);
    }

    // scroll bar
    {
        float SCROLL_WIDTH = 25;

        float amtDown = last_scroll_pos; // note this is set above

        float top_percent = amtDown / (float)tiles_height;
        float bot_percent = (amtDown+ch) / (float)tiles_height;

        ui_scrollbar({cw-SCROLL_WIDTH, 0, SCROLL_WIDTH, (float)ch},
                     top_percent, bot_percent,
                     &last_scroll_pos,
                     tiles_height,
                     0);

    }

    // tag menu
    {
        if (!tag_menu_open) {
            ui_button("show tags", cw/2, 0, UI_CENTER,UI_TOP, &ToggleTagMenu);
        } else {
            DrawTagMenu(cw, ch,
                        &SelectBrowseTagsNone,
                        &SelectBrowseTagsAll,
                        &ToggleTagBrowse,
                        &ToggleTagMenu,
                        &browse_tags);
        }
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

        UI_PRINT("mouse: %f, %f", input.mouseX, input.mouseY);
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

            UI_PRINT("browse tags: %i", browse_tags.count);
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



void view_tick(float actual_dt, int cw, int ch, Input input, Input keysDown, Input keysUp) {

    // --update--

    if (keysDown.right || keysDown.left) {
        // find position in display list
        // (seems a little awkward...)
        int display_index_of_view_item;
        for (int i = 0; i < display_list.count; i++) {
            if (viewing_file_index == display_list[i]) {
                display_index_of_view_item = i;
            }
        }

        // increment
        if (keysDown.right) {
            display_index_of_view_item = (display_index_of_view_item+1) % display_list.count;
        }
        if (keysDown.left) {
            display_index_of_view_item = (display_index_of_view_item-1+display_list.count) % display_list.count;
        }

        // set new item
        //viewing_file_index = display_list[display_index_of_view_item];
        OpenFileToView(display_list[display_index_of_view_item]);
    }

    if (keysDown.mouseR) {
        app_mode = BROWSING_THUMBS;
        // todo: remove tile stuff from memory or gpu ?
    }
    if (keysDown.space)
        ToggleTagSelectMenu(0);


    // pan
    if (!input.mouseL) mouse_up_once_since_loading = true;
    if (keysDown.mouseM || keysDown.mouseL) {
        clickMouseX = input.mouseX;
        clickMouseY = input.mouseY;
        clickRectX = viewing_tile.pos.x;
        clickRectY = viewing_tile.pos.y;
    }
    if ((input.mouseM || input.mouseL) && mouse_up_once_since_loading) {
        float deltaX = input.mouseX - clickMouseX;
        float deltaY = input.mouseY - clickMouseY;
        viewing_tile.pos.x = clickRectX + deltaX;
        viewing_tile.pos.y = clickRectY + deltaY;
    }

    // zoom
    if (master_scroll_delta != 0) {

        float scaleFactor = 0.8f;
        if (master_scroll_delta < 0) scaleFactor = 1.2f;
        viewing_tile.size.w *= scaleFactor;
        viewing_tile.size.h *= scaleFactor;
        viewing_tile.pos.x -= (input.mouseX - viewing_tile.pos.x) * (scaleFactor - 1);
        viewing_tile.pos.y -= (input.mouseY - viewing_tile.pos.y) * (scaleFactor - 1);

        master_scroll_delta = 0; // done using this this frame (todo: should put mousewheel in Input class)
    }



    // --render--

    opengl_clear();

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


    // select tag menu
    {

        if (!tag_select_open) {
            ui_button("show tags", cw/2, 0, UI_CENTER,UI_TOP, &ToggleTagSelectMenu);
        } else {
            DrawTagMenu(cw, ch,
                        &SelectItemTagsNone,
                        &SelectItemTagsAll,
                        &ToggleTagSelection,
                        &ToggleTagSelectMenu,
                        &items[viewing_file_index].tags);
        }
    }


    if (show_debug_console) {

        // debug tag count on right
        char buf[256];
        sprintf(buf, "%i", items[viewing_file_index].tags.count);
        ui_text(buf, cw,ch, UI_RIGHT,UI_BOTTOM);

        // debug tag list on left
        float x = 0;
        for (int i = 0; i < items[viewing_file_index].tags.count; i++) {
            rect lastrect = ui_text(tag_list[items[viewing_file_index].tags[i]].ToUTF8Reusable(), x,ch, UI_LEFT,UI_BOTTOM);
            x+=lastrect.w;
        }


        UI_PRINTRESET();
        UI_PRINT("dt: %f", actual_dt);
        UI_PRINT("max dt: %f", metric_max_dt);




    }
}