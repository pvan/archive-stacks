#include <windows.h>
#include <stdio.h>

#include <assert.h>
#include <math.h>


#include "types.h"

#include "memdebug.h" // will slow down our free()s especially

char debugprintbuffer[256];
void DEBUGPRINT(int i) { sprintf(debugprintbuffer, "%i\n", i); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(u64 ui) { sprintf(debugprintbuffer, "%lli\n", ui); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(float f) { sprintf(debugprintbuffer, "%f\n", f); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(char *s) { sprintf(debugprintbuffer, "%s\n", s); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(char *s, int i) { sprintf(debugprintbuffer, s, i); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(char *s, u32 i) { sprintf(debugprintbuffer, s, i); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(char *s, int i1, int i2) { sprintf(debugprintbuffer, s, i1, i2); OutputDebugString(debugprintbuffer); }
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
float time_now() { // ms
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    int64_t ticks_elapsed = now.QuadPart - time_start.QuadPart;
    float dt = ((float)ticks_elapsed * 1.0f) / ((float)time_freq.QuadPart);
    return dt * 1000.0f; // return ms
}






bool running = true;
HWND g_hwnd;

#include "tile.cpp"
#include "data.cpp"

tile viewing_tile; // tile for our open file (created from fullpath rather than thumbpath like the "browsing" tiles are)

#include "background.cpp"


int master_scroll_delta = 0;
int master_ctrl_scroll_delta = 0;

float master_desired_tile_width = 200;

// basically looking at these all over, let's just make global at app level for now
// updated at start of every frame
int g_cw;
int g_ch;
float g_dt;



// todo: find the right home for these vars
float metric_max_dt;
float metric_time_to_first_frame;

bool show_debug_console = false;

#include "update.cpp"


// done once after start loading is done or new directory selected
void init_app(item_pool all_items, int cw, int ch) {

    SelectAllBrowseTags();

    CreateDisplayListFromBrowseSelection();

    ArrangeTilesForDisplayList(display_list, &tiles, master_desired_tile_width, cw); // requires resolutions to be set

    // SortTilePoolByDate(&tiles);
    // ArrangeTilesInOrder(&tiles, master_desired_tile_width, cw); // requires resolutions to be set

    // constant-churning loop to load items on screen, and unlod those off the screen
    LaunchBackgroundLoadingLoopIfNeeded();
    LaunchBackgroundUnloadingLoopIfNeeded();

    SaveMetadataFile();
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
    g_hwnd = hwnd;

    HDC hdc = GetDC(hwnd);


    opengl_init(hdc);
    opengl_resize_if_change(cw, ch);
    gpu_init(hdc);


    ffmpeg_init();


    tf_init(UI_TEXT_SIZE);   // where to set font size, specifically, though?


    ui_init();


    master_path = string::create_with_new_memory(L"E:\\inspiration test folder2");
    SelectNewMasterDirectory(master_path);


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
        g_dt = actual_dt;
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




        // // runs until background startup loading is done
        // if (loading) {
        //     load_tick(items, cw, ch);
        //     continue;
        // }

        // ran once when loading is done
        if (app_mode == INIT) {
            init_app(items, cw, ch);
            // need_init = false;
            app_mode = BROWSING_THUMBS;
            // memdebug_reset();
        }




        // main_loop(actual_dt);


        // calc input edge triggers
        input = input_read(hwnd);



        // common to all app modes
        if (input.down.tilde) show_debug_console = !show_debug_console;

        opengl_clear();

        if (app_mode == BROWSING_THUMBS) {
            browse_tick(actual_dt, cw,ch);
        }
        else if (app_mode == VIEWING_FILE) {
            view_tick(actual_dt, cw,ch);
        }
        else if (app_mode == SETTINGS) {
            settings_tick(actual_dt, cw,ch);
        }
        else if (app_mode == LOADING) {
            load_tick(actual_dt, cw,ch);
            // memdebug_reset();
        }

        // common to all app modes
        ui_update();

        opengl_swap();



        if (!measured_first_frame_time) { metric_time_to_first_frame = time_now() - time_at_startup; measured_first_frame_time = true; }



        // memdebug_print();
        // memdebug_reset();


        // Sleep(16); // we set frame rate above right? or should we here?
    }



    // little test to see if we are leaking memory anywhere

    // same as background thread prep for new directory
    {
        loading_status_msg = "Unloading previous media...";
        loading_reusable_max = tiles.count;
        // 1a. list contents
        for (int i = 0; i < tiles.count; i++) {
            loading_reusable_count = i;
            tiles[i].UnloadMedia();
            // loading_reusable_count = 100000+i;
            // if (tiles[i].name.chars) free(tiles[i].name.chars);
        }
        loading_status_msg = "Unloading previous tags...";
        loading_reusable_max = tag_list.count;
        for (int i = 0; i < tag_list.count; i++) {
            loading_reusable_count = i;
            if (tag_list.pool[i].list) free(tag_list.pool[i].list);
        }
        // we separated the tag lists out from item so we could tightly pack all item paths into one memory allocation eventually
        // because free()ing each was taking forever
        // turns out it was our mem debug wrapper around free that was slow
        // freeing ~5k-10k times here was super fast without memdebug enabled
        loading_status_msg = "Unloading previous tag lists...";
        loading_reusable_max = item_tags.count;
        for (int i = 0; i < item_tags.count; i++) {
            loading_reusable_count = i;
            item_tags[i].free_pool();
        }
        // the idea is we could replace this loop with a single alloc/free
        // if we use a memory pool for these lists (since they never change it should be easy)
        loading_status_msg = "Unloading previous item's paths...";
        loading_reusable_max = items.count;
        for (int i = 0; i < items.count; i++) {
            loading_reusable_count = i;
            items[i].free_all();
        }

        // 1b. list themselves
        loading_status_msg = "Unloading previous item lists...";
        items.free_pool();
        tiles.free_pool();
        tag_list.free_pool();
        item_tags.free_pool();

        loading_status_msg = "Unloading previous item metadata lists...";
        modifiedTimes.free_pool();
        item_resolutions.free_pool();
        item_resolutions_valid.free_pool();

        loading_status_msg = "Unloading previous item index lists...";
        display_list.free_pool();
        browse_tag_filter.free_all();
        view_tag_filter.free_all();

        // can just reuse this memory, but need to empty out
        filtered_browse_tag_indices.empty_out();
        filtered_view_tag_indices.empty_out();
    }

    // specific to settings window
    {
        bg_path_copy.free_all();
        free_all_item_pool_memory(&proposed_items);
        proposed_thumbs_found.free_pool();
    }

    // view mode
    {
        viewing_tile.UnloadMedia();
    }

    // everything else
    {
        filtered_browse_tag_indices.free_pool();
        filtered_view_tag_indices.free_pool();
        browse_tag_indices.free_pool();

        master_path.free_all();
        proposed_master_path.free_all();
        last_proposed_master_path.free_all();
        proposed_path_msg.free_all();
        bg_path_copy.free_all();

        laststr.free_all();

        // tf
        free(tf_file_buffer);
        free(tf_fontatlas.data);

    }

    DEBUGPRINT("\n\n--TOTAL LEAKS--\n");
    memdebug_print();


    return 0;
}
