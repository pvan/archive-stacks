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

u64 viewing_file_index = 0; // what file do we have open if we're in VIEWING_FILE mode

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
        input = input_read(hwnd);



        // common to all app modes
        if (input.down.tilde) show_debug_console = !show_debug_console;


        if (app_mode == BROWSING_THUMBS) {
            browse_tick(actual_dt, cw,ch);
        }
        else if (app_mode == VIEWING_FILE) {
            view_tick(actual_dt, cw,ch);
        }


        // // update ui in all app modes
        // ui_update(cw,ch, input.current, input.down);
        // ui_reset(); // call at the end or start of every frame so buttons don't carry over between frames


        opengl_swap();


        if (!measured_first_frame_time) { metric_time_to_first_frame = time_now() - time_at_startup; measured_first_frame_time = true; }



        // memdebug_print();
        // memdebug_reset();


        // Sleep(16); // we set frame rate above right? or should we here?
    }
    // memdebug_print();
    return 0;
}
