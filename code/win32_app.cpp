#include <windows.h>
#include <stdio.h>

#include <assert.h>
#include <math.h>

#include "types.h"

// #define DEBUG_MEM_ENABLED
#ifdef DEBUG_MEM_ENABLED
#include "memdebug.h" // will slow down our free()s especially
#endif

char debugprintbuffer[1024];
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
void DEBUGPRINT(wc *s) { sprintf(debugprintbuffer, "%ls\n", s); OutputDebugString(debugprintbuffer); }

#include "v2.cpp"
#include "rect.cpp"
#include "string.cpp"
void DEBUGPRINT(char *msg, string s) {
    wc *temp = s.to_wc_new_memory(__FILE__, __LINE__);
    sprintf(debugprintbuffer, msg, temp);
    free(temp);
    OutputDebugString(debugprintbuffer);
}
void DEBUGPRINT(string s) {
    wc *temp = s.to_wc_new_memory(__FILE__, __LINE__);
    sprintf(debugprintbuffer, "%ls\n", temp);
    free(temp);
    OutputDebugString(debugprintbuffer);
}
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

string running_keyboard_input = string::new_empty();
// string running_keyboard_input_last_match = string::new_empty();
int running_keyboard_last_common_chars = 0;
float ms_since_last_key = 0;
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

// basically looking at these all over, let's just make global at app level for now
// updated at start of every frame
HWND g_hwnd;
int g_cw;
int g_ch;
float g_dt; // still pass some of these around in tick()s etc though we could just use these instead

#include "tile.cpp"
#include "data.cpp"
#include "background.cpp"
#include "update.cpp"


// done once after start loading is done or new directory selected
void init_app(item_pool all_items, int cw, int ch) {

    SelectAllBrowseTags();

    CreateDisplayListFromBrowseSelection();

    ArrangeTilesForDisplayList(display_list, &tiles, master_desired_tile_width, cw); // requires resolutions to be set

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
    wndc.lpszClassName = "archive-stacks";
    if (!RegisterClass(&wndc)) { MessageBox(0, "RegisterClass failed", 0, 0); return 1; }

    DWORD win_style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    AdjustWindowRect(&win_rect, win_style, 0);

    HWND hwnd = CreateWindowEx(
        0, wndc.lpszClassName, "The Archive Stacks",
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


    // start these right away now
    // the should properly adapt to whatever app_mode we are in
    // constant-churning loop to load items on screen, and unlod those off the screen
    LaunchBackgroundLoadingLoopIfNeeded();
    LaunchBackgroundUnloadingLoopIfNeeded();


    // master_path = string::create_with_new_memory(L"E:\\inspiration test folder2");
    master_path = win32_load_string_from_appdata_into_new_memory(appdata_subpath_for_master_directory);
    if (master_path.count == 0) {
        master_path = string::create_with_new_memory(L"**no path selected**", __FILE__, __LINE__); // default path
    }

    if (!win32_IsDirectory(master_path)) {
        app_change_mode(SETTINGS); // launch in settings view if no valid path
    } else {
        OpenNewMasterDirectory(master_path);
    }


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

        // DEBUGPRINT(actual_dt);

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



        // track keyboard input globally so we can select the closest button in ui
        {
            if (ms_since_last_key < 1000) {
                ms_since_last_key += actual_dt;
            }
            if (input.mouse_moved()) {
                running_keyboard_input.empty_out();
            }
            running_keyboard_last_common_chars = 0; // reset each frame

            int precount = running_keyboard_input.count;
            if (!input.current.shift) {
                if (input.down.q) { running_keyboard_input.append(L'q'); }
                if (input.down.w) { running_keyboard_input.append(L'w'); }
                if (input.down.e) { running_keyboard_input.append(L'e'); }
                if (input.down.r) { running_keyboard_input.append(L'r'); }
                if (input.down.t) { running_keyboard_input.append(L't'); }
                if (input.down.y) { running_keyboard_input.append(L'y'); }
                if (input.down.u) { running_keyboard_input.append(L'u'); }
                if (input.down.i) { running_keyboard_input.append(L'i'); }
                if (input.down.o) { running_keyboard_input.append(L'o'); }
                if (input.down.p) { running_keyboard_input.append(L'p'); }
                if (input.down.a) { running_keyboard_input.append(L'a'); }
                if (input.down.s) { running_keyboard_input.append(L's'); }
                if (input.down.d) { running_keyboard_input.append(L'd'); }
                if (input.down.f) { running_keyboard_input.append(L'f'); }
                if (input.down.g) { running_keyboard_input.append(L'g'); }
                if (input.down.h) { running_keyboard_input.append(L'h'); }
                if (input.down.j) { running_keyboard_input.append(L'j'); }
                if (input.down.k) { running_keyboard_input.append(L'k'); }
                if (input.down.l) { running_keyboard_input.append(L'l'); }
                if (input.down.z) { running_keyboard_input.append(L'z'); }
                if (input.down.x) { running_keyboard_input.append(L'x'); }
                if (input.down.c) { running_keyboard_input.append(L'c'); }
                if (input.down.v) { running_keyboard_input.append(L'v'); }
                if (input.down.b) { running_keyboard_input.append(L'b'); }
                if (input.down.n) { running_keyboard_input.append(L'n'); }
                if (input.down.m) { running_keyboard_input.append(L'm'); }
                if (input.down.squareL   ) { running_keyboard_input.append(L'['); }
                if (input.down.squareR   ) { running_keyboard_input.append(L']'); }
                if (input.down.bslash    ) { running_keyboard_input.append(L'\\'); }
                if (input.down.semicolon ) { running_keyboard_input.append(L';'); }
                if (input.down.apostrophe) { running_keyboard_input.append(L'\''); }
                if (input.down.comma     ) { running_keyboard_input.append(L','); }
                if (input.down.period    ) { running_keyboard_input.append(L'.'); }
                if (input.down.fslash    ) { running_keyboard_input.append(L'/'); }
                if (input.down.space     ) { running_keyboard_input.append(L' '); }
                if (input.down.tilde     ) { running_keyboard_input.append(L'`'); }
                if (input.down.row[1] ) { running_keyboard_input.append(L'1'); }
                if (input.down.row[2] ) { running_keyboard_input.append(L'2'); }
                if (input.down.row[3] ) { running_keyboard_input.append(L'3'); }
                if (input.down.row[4] ) { running_keyboard_input.append(L'4'); }
                if (input.down.row[5] ) { running_keyboard_input.append(L'5'); }
                if (input.down.row[6] ) { running_keyboard_input.append(L'6'); }
                if (input.down.row[7] ) { running_keyboard_input.append(L'7'); }
                if (input.down.row[8] ) { running_keyboard_input.append(L'8'); }
                if (input.down.row[9] ) { running_keyboard_input.append(L'9'); }
                if (input.down.row[10]) { running_keyboard_input.append(L'0'); }
                if (input.down.row[11]) { running_keyboard_input.append(L'-'); }
                if (input.down.row[12]) { running_keyboard_input.append(L'='); }
            } else {
                if (input.down.q) { running_keyboard_input.append(L'Q'); }
                if (input.down.w) { running_keyboard_input.append(L'W'); }
                if (input.down.e) { running_keyboard_input.append(L'E'); }
                if (input.down.r) { running_keyboard_input.append(L'R'); }
                if (input.down.t) { running_keyboard_input.append(L'T'); }
                if (input.down.y) { running_keyboard_input.append(L'Y'); }
                if (input.down.u) { running_keyboard_input.append(L'U'); }
                if (input.down.i) { running_keyboard_input.append(L'I'); }
                if (input.down.o) { running_keyboard_input.append(L'O'); }
                if (input.down.p) { running_keyboard_input.append(L'P'); }
                if (input.down.a) { running_keyboard_input.append(L'A'); }
                if (input.down.s) { running_keyboard_input.append(L'S'); }
                if (input.down.d) { running_keyboard_input.append(L'D'); }
                if (input.down.f) { running_keyboard_input.append(L'F'); }
                if (input.down.g) { running_keyboard_input.append(L'G'); }
                if (input.down.h) { running_keyboard_input.append(L'H'); }
                if (input.down.j) { running_keyboard_input.append(L'J'); }
                if (input.down.k) { running_keyboard_input.append(L'K'); }
                if (input.down.l) { running_keyboard_input.append(L'L'); }
                if (input.down.z) { running_keyboard_input.append(L'Z'); }
                if (input.down.x) { running_keyboard_input.append(L'X'); }
                if (input.down.c) { running_keyboard_input.append(L'C'); }
                if (input.down.v) { running_keyboard_input.append(L'V'); }
                if (input.down.b) { running_keyboard_input.append(L'B'); }
                if (input.down.n) { running_keyboard_input.append(L'N'); }
                if (input.down.m) { running_keyboard_input.append(L'M'); }
                if (input.down.squareL   ) { running_keyboard_input.append(L'{'); }
                if (input.down.squareR   ) { running_keyboard_input.append(L'}'); }
                if (input.down.bslash    ) { running_keyboard_input.append(L'|'); }
                if (input.down.semicolon ) { running_keyboard_input.append(L':'); }
                if (input.down.apostrophe) { running_keyboard_input.append(L'"'); }
                if (input.down.comma     ) { running_keyboard_input.append(L'<'); }
                if (input.down.period    ) { running_keyboard_input.append(L'>'); }
                if (input.down.fslash    ) { running_keyboard_input.append(L'?'); }
                if (input.down.space     ) { running_keyboard_input.append(L' '); }
                if (input.down.tilde     ) { running_keyboard_input.append(L'~'); }
                if (input.down.row[1] ) { running_keyboard_input.append(L'!'); }
                if (input.down.row[2] ) { running_keyboard_input.append(L'@'); }
                if (input.down.row[3] ) { running_keyboard_input.append(L'#'); }
                if (input.down.row[4] ) { running_keyboard_input.append(L'$'); }
                if (input.down.row[5] ) { running_keyboard_input.append(L'%'); }
                if (input.down.row[6] ) { running_keyboard_input.append(L'^'); }
                if (input.down.row[7] ) { running_keyboard_input.append(L'&'); }
                if (input.down.row[8] ) { running_keyboard_input.append(L'*'); }
                if (input.down.row[9] ) { running_keyboard_input.append(L'('); }
                if (input.down.row[10]) { running_keyboard_input.append(L')'); }
                if (input.down.row[11]) { running_keyboard_input.append(L'_'); }
                if (input.down.row[12]) { running_keyboard_input.append(L'+'); }
            }
            // same regardless of shift key (thought could check for numlock state)
            if (input.down.num0)       { running_keyboard_input.append(L'0'); }
            if (input.down.num1)       { running_keyboard_input.append(L'1'); }
            if (input.down.num2)       { running_keyboard_input.append(L'2'); }
            if (input.down.num3)       { running_keyboard_input.append(L'3'); }
            if (input.down.num4)       { running_keyboard_input.append(L'4'); }
            if (input.down.num5)       { running_keyboard_input.append(L'5'); }
            if (input.down.num6)       { running_keyboard_input.append(L'6'); }
            if (input.down.num7)       { running_keyboard_input.append(L'7'); }
            if (input.down.num8)       { running_keyboard_input.append(L'8'); }
            if (input.down.num9)       { running_keyboard_input.append(L'9'); }
            if (input.down.numDiv)     { running_keyboard_input.append(L'/'); }
            if (input.down.numMul)     { running_keyboard_input.append(L'*'); }
            if (input.down.numSub)     { running_keyboard_input.append(L'-'); }
            if (input.down.numAdd)     { running_keyboard_input.append(L'+'); }
            if (input.down.numDecimal) { running_keyboard_input.append(L'.'); }

            // make sure .up or we could clear our ui focus with .down and then .up selection wont work
            if (input.up.backspace) { running_keyboard_input.empty_out(); }
            if (input.up.enter) { running_keyboard_input.empty_out(); }

            if (precount < running_keyboard_input.count) {
                ms_since_last_key = 0;
            }
        }


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
    // can remove this in release
#ifdef DEBUG_MEM_ENABLED
    {
        // wait for bg threads to finish before trying to free all memory
        while(background_loading_thread_launched);
        while(background_unloading_thread_launched);
        while(background_startup_thread_launched);

        FreeAllAppMemory(true);
        DEBUGPRINT("\n\n--TOTAL LEAKS--\n");
        memdebug_print();
    }
#endif

    return 0;
}
