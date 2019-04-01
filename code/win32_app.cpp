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

    // move all the tile buttons so we can't click on them
    // better way to do this?
    for (int i = 0; i < tiles.count; i++) {
        tiles[i].pos = {-1000,-1000};
        tiles[i].size = {0,0};
    }

}


// try another layout method
void DrawTagMenu(int cw, int ch,
                 void (*selectNone)(int),
                 void (*selectAll)(int),
                 void (*tagSelect)(int),
                 void (*hideMenu)(int),
                 int_pool *selected_tags_pool)
{
    rect lastr = ui_button("select none", 0,0, UI_LEFT,UI_TOP, selectNone);
    ui_button("select all", lastr.w,0, UI_LEFT,UI_TOP, selectAll);


    // first get size of all our tags
    float_pool widths = float_pool::new_empty();
    for (int i = 0; i < tag_list.count; i++) {
        rect r = ui_text(tag_list[i].ToUTF8Reusable(), 0,0, UI_LEFT,UI_TOP, false);
        widths.add(r.w);
    }



    // ok, try dividing into X columns, starting at 1 and going up until we're out of space

    // upperbound is 1 col for every tag
    // for (int totalcols = 1; totalcols < tag_list.count; totalcols++) {
    int totalcols = 5;

        // algorithm is a bit quirky
        // basically we put all the widths in place for one column
        // then find the biggest gap in the largest X items (X=max_overrun_count)
        // that's where we create the column width (with the larger items running into the next column)
        // also, the gap has to be at least Y big to count (to avoid tiny little overruns) (Y=min_overrun_amount)
        //
        // so you might have a natural break with the 3rd largest item,
        // but if the largest is so much bigger than the next two, that's what will be sued
        //
        // todo: theoretically could bleed over into an extra column, i think (if we skip a lot each col)
        // also these vars not tested for extreme values

        int max_rows_per_col = (tag_list.count / totalcols) + 1; // round up
        int max_overrun_count = 3; // max number of items to allow to overrun per column
        float min_overrun_amount = 20; // how far into next col we need to be

        // list of column widths for columns created so far
        int_pool colwidths = int_pool::new_empty();
        colwidths.add(0); // first column (more added each new column)

        // width of every item in current column (and row number of it)
        intfloatpair_pool widthsincol = intfloatpair_pool::new_empty();

        // list of row indicies (eg col 2 second item is row=1)
        // to skip over on this column (because last column item overruns into this spot)
        int_pool skiprows = int_pool::new_empty();

        int col = 0;
        int row = 0;
        for (int t = 0; t < tag_list.count; t++, row++) { // note row++

            if (row >= max_rows_per_col) {  // end of a row

                sort_intfloatpair_pool_high_to_low(&widthsincol);

                // find largest gap in widths of the largest X items (X=max_overrun_count)
                // gap has to be at least Y big to count (Y=min_overrun_amount)
                float largestgap = 0;
                int largestgapcount = 0;
                for (int i = 0; i<max_overrun_count && i<widthsincol.count-1; i++) {
                    float thisgap = widthsincol[i].f - widthsincol[i+1].f;
                    if (thisgap > largestgap &&
                        thisgap > min_overrun_amount) // don't treat as gap if not this big
                    {
                        largestgap = thisgap;
                        largestgapcount = i+1;
                    }
                }
                colwidths[col] = widthsincol[largestgapcount].f; // final width of that column
                skiprows.empty_out(); // and what row spots to skip for the next columns
                for (int i = 0; i < largestgapcount; i++) {
                    skiprows.add(widthsincol[i].i);
                }

                // prep for next column
                widthsincol = intfloatpair_pool::new_empty(); // empty widths for use in next column
                colwidths.add(0); // first column (more added each new column)

                row = 0;
                col++;
            }

            // skip spots where there are overrun items in the last column
            if (skiprows.has(row))
                continue;

            widthsincol.add({row,widths[t]});

            // actually draw the buttons (skip this when searching for now many columns we need)
            {
                float x = 0;
                for (int c = 0; c < col; c++) x += colwidths[c]; // sum of all previous columns
                float y = row * UI_TEXT_SIZE + UI_TEXT_SIZE;

                rect brect = ui_button(tag_list[t].ToUTF8Reusable(), x,y, UI_LEFT,UI_TOP, tagSelect, t);

                if (selected_tags_pool->has(t)) {
                    ui_rect(brect, 0xffff00ff, 0.3);
                }
            }

        } // iterated through last tag
    // }

    colwidths.free_all();
    widthsincol.free_all();
    skiprows.free_all();

}

// can use for tag menu in browse mode (selecting what to browse)
// and tag menu in view mode (for selecting an item's tags)
// pass in the click handlers for the buttons
// and a list of ints which are the indices of the tags that are marked "selected" (color pink)
void DrawTagMenu0(int cw, int ch,
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


// todo: find the right home for these vars
float metric_max_dt;
float metric_time_to_first_frame;

bool show_debug_console = false;

#include "update.cpp"


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
    metric_time_to_first_frame = 0;
    float time_at_startup = time_now();
    bool measured_first_frame_time = false;

    // other time metrics
    metric_max_dt = 0;
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

        g_cw = cw; // used for a couple things, eg callbacks where not so easy to pass values in
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



        // common to all app modes
        if (keysDown.tilde) show_debug_console = !show_debug_console;


        if (app_mode == BROWSING_THUMBS) {
            browse_tick(actual_dt, cw,ch, input,keysDown,keysUp);
        }
        else if (app_mode == VIEWING_FILE) {
            view_tick(actual_dt, cw,ch, input,keysDown,keysUp);
        }


        // update ui in all app modes
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
