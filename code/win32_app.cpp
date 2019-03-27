#include <windows.h>
#include <stdio.h>

#include <assert.h>
#include <math.h>


#include "types.h"

#include "memdebug.h"

char debugprintbuffer[256];
void DEBUGPRINT(int i) { sprintf(debugprintbuffer, "%i\n", i); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(u64 ui) { sprintf(debugprintbuffer, "%lli\n", ui); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(float f) { sprintf(debugprintbuffer, "%f\n", f); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(char *s) { sprintf(debugprintbuffer, "%s\n", s); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(char *s, int i) { sprintf(debugprintbuffer, s, i); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(char *s, u32 i) { sprintf(debugprintbuffer, s, i); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(char *s, char *s1) { sprintf(debugprintbuffer, s, s1); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(char *s, char *s1, char *s2) { sprintf(debugprintbuffer, s, s1, s2); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(char *s, float f) { sprintf(debugprintbuffer, s, f); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(char *s, float f1, float f2) { sprintf(debugprintbuffer, s, f1, f2); OutputDebugString(debugprintbuffer); }

#include "v2.cpp"
#include "rect.cpp"
#include "string.cpp"
#include "bitmap.cpp"
#include "pools.cpp"

#include "win32_input.cpp"
#include "win32_icon/icon.h"
#include "win32_opengl.cpp"
#include "gpu.cpp"
#include "win32_file.cpp"
#include "win32_system.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../lib/stb_image_write.h"

#include "ffmpeg.cpp"
#include "text.cpp"
#include "ui.cpp"




LARGE_INTEGER time_freq;
LARGE_INTEGER time_start;
void time_init() {
    timeBeginPeriod(1);
    QueryPerformanceFrequency(&time_freq);
    QueryPerformanceCounter(&time_start);
}
float time_now() {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    int64_t ticks_elapsed = now.QuadPart - time_start.QuadPart;
    float dt = ((float)ticks_elapsed * 1.0f) / ((float)time_freq.QuadPart);
    return dt * 1000.0f; // return ms
}






bool running = true;

bool loading = true;
bool need_init = true;

// could add loading/etc to this framework
const int BROWSING_THUMBS = 0;
const int VIEWING_FILE = 1;
int app_mode = BROWSING_THUMBS;

int viewing_file_index = 0; // what file do we have open if we're in VIEWING_FILE mode

string master_path;

#include "data.cpp"
#include "tile.cpp"

tile viewing_tile; // tile for our open file (created from fullpath rather than thumbpath like the "browsing" tiles are)

#include "background.cpp"


int master_scroll_delta = 0;
int master_ctrl_scroll_delta = 0;

float master_desired_tile_width = 200;

int g_cw;
int g_ch;

// for panning VIEW mode
bool mouse_up_once_since_loading = false;
float clickMouseX;
float clickMouseY;
float clickRectX;
float clickRectY;

//
// button click handlers

bool tag_menu_open = false;  // is tag menu open in browsing mode?
void ToggleTagMenu(int) { tag_menu_open = !tag_menu_open; }
void ToggleTagBrowse(int tagindex) {
    if (browse_tags.has(tagindex)) {
        browse_tags.remove(tagindex);
    } else {
        browse_tags.add(tagindex);
    }

    // update display list here
    CreateDisplayListFromBrowseSelection();

    // do in loop for now, just before rendering (or, well, in the update portion)
    // ArrangeTilesForDisplayList(display_list, &tiles, master_desired_tile_width, g_cw); // requires resolutions to be set
}
void SelectBrowseTagsAll(int) { SelectAllBrowseTags(); CreateDisplayListFromBrowseSelection(); }
void SelectBrowseTagsNone(int) { DeselectAllBrowseTags(); CreateDisplayListFromBrowseSelection(); }

bool tag_select_open = false;  // is tag menu open in viewing mode?
void ToggleTagSelectMenu(int) { tag_select_open = !tag_select_open; }
// we assume we're affecting the open / "viewing" file here
void ToggleTagSelection(int tagindex) {
    if (items[viewing_file_index].tags.has(tagindex)) {
        items[viewing_file_index].tags.remove(tagindex);
    } else
    {
        items[viewing_file_index].tags.add(tagindex);
    }
    SaveMetadataFile(); // keep close eye on this, if it gets too slow to do inline we can move to background thread
}
void SelectItemTagsAll(int) {
    items[viewing_file_index].tags.empty_out();
    for (int i = 0; i < tag_list.count; i++) {
        items[viewing_file_index].tags.add(i);
    }
}
void SelectItemTagsNone(int) {
    items[viewing_file_index].tags.empty_out();
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
}


// can use for tag menu in browse mode (selecting what to browse)
// and tag menu in view mode (for selecting an item's tags)
// pass in the click handlers for the buttons
// and a list of ints which are the indices of the tags that are marked "selected" (color pink)
void DrawTagMenu(int cw, int ch,
                 void (*selectNone)(int),
                 void (*selectAll)(int),
                 void (*tagSelect)(int),
                 void (*hideMenu)(int),
                 int_pool *selected_tags_pool)
{
    float hgap = 3;
    float vgap = 3;

    float x = hgap;
    float y = UI_TEXT_SIZE + vgap*2;

    // get total size
    {
        for (int i = 0; i < tag_list.count; i++) {
            rect this_rect = ui_text(tag_list[i].ToUTF8Reusable(), x,y, UI_LEFT,UI_TOP, false);
            this_rect.w+=hgap;
            this_rect.h+=vgap;
            if (x+this_rect.w > cw) { y+=this_rect.h; x=hgap; }
            x += this_rect.w;
            if (i == tag_list.count-1) y += this_rect.h; // \n for last row
        }
    }
    ui_rect_solid({0,0,(float)cw,y+UI_TEXT_SIZE+vgap}, 0xffffffff, 0.5); // menu bg

    rect lastr = ui_button("select none", hgap,vgap, UI_LEFT,UI_TOP, selectNone);
    ui_button("select all", lastr.w+hgap*2,vgap, UI_LEFT,UI_TOP, selectAll);
    // could also do something like this
    // if (mode == BROWSING_THUMBS) {
    //     rect lastr = ui_button("select none", hgap,vgap, UI_LEFT,UI_TOP, &SelectItemTagsNone);
    //     ui_button("select all", lastr.w+hgap*2,vgap, UI_LEFT,UI_TOP, &SelectItemTagsAll);
    // } else if (mode == VIEWING_FILE) {
    //     rect lastr = ui_button("select none", 0,0, UI_LEFT,UI_TOP, &SelectBrowseTagsNone);
    //     ui_button("select all", lastr.w+hgap,0, UI_LEFT,UI_TOP, &SelectBrowseTagsAll);
    // }

    x = hgap;
    y = UI_TEXT_SIZE + vgap*2;
    for (int i = 0; i < tag_list.count; i++) {
        rect this_rect = ui_text(tag_list[i].ToUTF8Reusable(), x,y, UI_LEFT,UI_TOP, false);
        this_rect.w+=hgap;
        this_rect.h+=vgap;
        if (x+this_rect.w > cw) { y+=this_rect.h; x=hgap; }
        rect brect = ui_button(tag_list[i].ToUTF8Reusable(), x,y, UI_LEFT,UI_TOP, tagSelect, i);

        // color selected tags...
        {
            // if (items[viewing_file_index].tags.has(i)) {
            if (selected_tags_pool->has(i)) {
                ui_rect(brect, 0xffff00ff, 0.3);
            }
        }

        x += this_rect.w;
        if (i == tag_list.count-1) y += this_rect.h; // \n for hide tag button
    }

    ui_button("hide tags", cw/2, y/*ch/2*/, UI_CENTER,UI_TOP, hideMenu);
}


// done once after startup loading is done
void init_app(item_pool all_items, int cw, int ch) {

    SelectAllBrowseTags();

    CreateDisplayListFromBrowseSelection();

    ArrangeTilesForDisplayList(display_list, &tiles, master_desired_tile_width, cw); // requires resolutions to be set

    // SortTilePoolByDate(&tiles);
    // ArrangeTilesInOrder(&tiles, master_desired_tile_width, cw); // requires resolutions to be set

    // constant-churning loop to load items on screen, and unlod those off the screen
    LaunchBackgroundLoadingLoop();
    LaunchBackgroundUnloadingLoop();

    SaveMetadataFile();
}

// done every frame while loading during startup
void load_tick(item_pool all_items, int cw, int ch) {

    // ui_progressbar(cw/2, ch2)

    opengl_clear();

    int line = -1;

    ui_texti("media files: %i", all_items.count, cw/2, ch/2 + UI_TEXT_SIZE*line++, UI_CENTER,UI_CENTER);

    line++;
    ui_texti("items_without_matching_thumbs: %i", item_indices_without_thumbs.count, cw/2, ch/2 + UI_TEXT_SIZE*line++, UI_CENTER,UI_CENTER);

    line++;
    ui_texti("items_without_matching_metadata: %i", item_indices_without_metadata.count, cw/2, ch/2 + UI_TEXT_SIZE*line++, UI_CENTER,UI_CENTER);

    line++;
    ui_text(loading_status_msg, cw/2, ch/2 + UI_TEXT_SIZE*line++, UI_CENTER,UI_CENTER);
    ui_texti("files: %i of %i", loading_reusable_count, loading_reusable_max, cw/2, ch/2 + UI_TEXT_SIZE*line++, UI_CENTER,UI_CENTER);


    ui_render_elements(0, 0); // pass mouse pos for highlighting
    ui_reset(); // call at the end or start of every frame so buttons don't carry over between frames


    opengl_swap();

}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CLOSE: {
            running = false;
        } break;

        case WM_MOUSEWHEEL: {
            float delta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL) {
                master_ctrl_scroll_delta -= (delta / 120) * 10;
            } else {
                // if (loadItem == -1) {
                    master_scroll_delta -= delta * 1;
                // }
                // else {
                //     float scaleFactor = 1.2f;
                //     if (delta < 0) scaleFactor = 0.8f;
                //     main_media_rect.w *= scaleFactor;
                //     main_media_rect.h *= scaleFactor;
                //     main_media_rect.x -= (input.mouseX - main_media_rect.x) * (scaleFactor - 1);
                //     main_media_rect.y -= (input.mouseY - main_media_rect.y) * (scaleFactor - 1);
                // }
            }
        } break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    RECT win_rect = {0,0,800,600};
    int cw = win_rect.right;
    int ch = win_rect.bottom;

    timeBeginPeriod(1); // set resolution of sleep

    // init timing vars
    // {
    const float target_fps = 30;
    float target_ms_per_frame = (1.0f / target_fps) * 1000.0f;
    time_init();
    float last_time = time_now();

    // startup timing metrics
    float metric_time_to_first_frame = 0;
    float time_at_startup = time_now();
    bool measured_first_frame_time = false;

    // other time metrics
    float metric_max_dt = 0;
    int metric_dt_history_index = 0;
    const int METRIC_DT_HISTORY_COUNT = target_fps * 2/*seconds of history*/;
    float metric_dt_history[METRIC_DT_HISTORY_COUNT];
    ZeroMemory(metric_dt_history, METRIC_DT_HISTORY_COUNT*sizeof(float));
    // }

    WNDCLASS wndc = {0};
    wndc.style = CS_HREDRAW | CS_VREDRAW;
    wndc.lpfnWndProc = WndProc;
    wndc.hInstance = hInstance;
    wndc.hCursor = LoadCursor(0, IDC_ARROW);
    wndc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(ID_ICON));
    wndc.lpszClassName = "the-stacks";
    if (!RegisterClass(&wndc)) { MessageBox(0, "RegisterClass failed", 0, 0); return 1; }

    DWORD win_style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    AdjustWindowRect(&win_rect, win_style, 0);

    HWND hwnd = CreateWindowEx(
        0, wndc.lpszClassName, "The Stacks",
        win_style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        win_rect.right-win_rect.left, win_rect.bottom-win_rect.top,
        0, 0, hInstance, 0);
    if (!hwnd) { MessageBox(0, "CreateWindowEx failed", 0, 0); return 1; }


    HDC hdc = GetDC(hwnd);


    opengl_init(hdc);
    opengl_resize_if_change(cw, ch);
    gpu_init(hdc);


    ffmpeg_init();


    tf_init(UI_TEXT_SIZE);   // where to set font size, specifically, though?


    ui_init();




    master_path = string::KeepMemory(L"E:\\inspiration test folder");

    // create item list with fullpath populated
    // just adapt old method for now
    string_pool itempaths = FindAllItemPaths(master_path);
    items = item_pool::new_empty();
    for (int i = 0; i < itempaths.count; i++) {
        item newitem = {0};
        newitem.fullpath = itempaths[i];
        items.add(newitem);
    }



    LaunchBackgroundStartupLoop();


    while(running)
    {

        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }


        // just see how fast we can get for now
        // // set frame rate
        // float frame_duration = time_now() - last_time;
        // if (frame_duration < target_ms_per_frame + 0.0001f) {
        //     DWORD sleep_ms = (DWORD)(target_ms_per_frame - frame_duration);
        //     if (sleep_ms > 0)
        //         Sleep(sleep_ms);  // should probably check if sleep has 1ms resolution first
        //     while (frame_duration < target_ms_per_frame)  // churn and burn the last few fractional ms if needed
        //         frame_duration = time_now() - last_time;
        // } else {
        //     // frame took longer than our target fps
        // }
        float actual_dt = time_now() - last_time;
        last_time = time_now();

        // if (measured_first_frame_time) { // don't count first frame
            metric_dt_history[metric_dt_history_index++] = actual_dt;
        // }
        if (metric_dt_history_index >= METRIC_DT_HISTORY_COUNT) metric_dt_history_index = 0;
        metric_max_dt = 0;
        for (int i = 0; i < METRIC_DT_HISTORY_COUNT; i++) {
            if (metric_dt_history[i] > metric_max_dt) {
                metric_max_dt = metric_dt_history[i];
            }
        }



        RECT clientRect; GetClientRect(hwnd, &clientRect);
        int cw = clientRect.right-clientRect.left;
        int ch = clientRect.bottom-clientRect.top;
        opengl_resize_if_change(cw, ch);

        g_cw = cw; // right now only used to arrange tiles when changing browsing tag selection
        g_ch = ch;




        // runs until background startup loading is done
        if (loading) {
            load_tick(items, cw, ch);
            continue;
        }

        // ran once when startup loading is done
        if (need_init) {
            init_app(items, cw, ch);
            need_init = false;
        }




        // main_loop(actual_dt);


        // calc input edge triggers
        static Input last_input = {0};
        Input input = ReadInput(hwnd);
        Input keysDown = input_keys_changed(input, last_input);
        Input keysUp = input_keys_changed(last_input, input);
        last_input = input;


        // --UPDATE--

        static bool show_debug_console = false;
        if (keysDown.tilde) show_debug_console = !show_debug_console;

        static int debug_info_tile_index_mouse_was_on = 0;

        static float last_scroll_pos = 0;
        int tiles_height; // declare out here so we can use in our scrollbar figuring
        if (app_mode == BROWSING_THUMBS) {
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


        }

        else if (app_mode == VIEWING_FILE) {
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


        }


        // --RENDER-- (any reason to keep update/render loops split out?)

        opengl_clear();


        if (app_mode == BROWSING_THUMBS) {
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

        else if (app_mode == VIEWING_FILE) {

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


        ui_update_draggables(input.mouseX, input.mouseY, input.mouseL);
        ui_update_clickables(input.mouseX, input.mouseY, keysDown.mouseL, cw, ch);
        ui_render_elements(input.mouseX, input.mouseY); // pass mouse pos for highlighting
        ui_reset(); // call at the end or start of every frame so buttons don't carry over between frames



        opengl_swap();

        if (!measured_first_frame_time) { metric_time_to_first_frame = time_now() - time_at_startup; measured_first_frame_time = true; }



        // memdebug_print();
        // memdebug_reset();


        // Sleep(16); // we set frame rate above right? or should we here?
    }
    // memdebug_print();
    return 0;
}
