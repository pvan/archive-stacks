#include <windows.h>
#include <stdio.h>

#include <assert.h>
#include <math.h>


#include "types.h"

char debugprintbuffer[256];
void DEBUGPRINT(int i) { sprintf(debugprintbuffer, "%i\n", i); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(u64 ui) { sprintf(debugprintbuffer, "%lli\n", ui); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(float f) { sprintf(debugprintbuffer, "%f\n", f); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(char *s) { sprintf(debugprintbuffer, "%s\n", s); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(char *s, int i) { sprintf(debugprintbuffer, s, i); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(char *s, char *s2) { sprintf(debugprintbuffer, s, s2); OutputDebugString(debugprintbuffer); }
void DEBUGPRINT(char *s, float f) { sprintf(debugprintbuffer, s, f); OutputDebugString(debugprintbuffer); }

#include "v2.cpp"
#include "rect.cpp"
#include "string.cpp"
#include "bitmap.cpp"
#include "pools.cpp"

#include "win32_input.cpp"
#include "win32_icon/icon.h"
#include "win32_opengl.cpp"
#include "win32_file.cpp"

#include "ffmpeg.cpp"
#include "tile.cpp"
#include "text.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image.h"



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

tile_pool tiles; // access from background thread as well

bool running = true;

#include "background.cpp"


int master_scroll_delta = 0;


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

    RECT win_rect = {0,0,600,400};
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

    WNDCLASS wc = {0};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(ID_ICON));
    wc.lpszClassName = "the-stacks";
    if (!RegisterClass(&wc)) { MessageBox(0, "RegisterClass failed", 0, 0); return 1; }

    DWORD win_style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    AdjustWindowRect(&win_rect, win_style, 0);

    HWND hwnd = CreateWindowEx(
        0, wc.lpszClassName, "The Stacks",
        win_style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        win_rect.right-win_rect.left, win_rect.bottom-win_rect.top,
        0, 0, hInstance, 0);
    if (!hwnd) { MessageBox(0, "CreateWindowEx failed", 0, 0); return 1; }


    HDC hdc = GetDC(hwnd);

    opengl_init(hdc);
    opengl_resize_if_change(cw, ch);


    ffmpeg_init();

    ttf_init();

    opengl_quad quad;
    quad.create(0,0,300,200);

    // int width, height, channels;
    // stbi_set_flip_vertically_on_load(true);
    // u8 *data = stbi_load("D:\\~phil\\projects\\endless-drive\\art\\car.jpg", &width, &height, &channels, 4);
    // if (!data) { assert(false); }
    // quad.set_texture((u32*)data, width, height);

    // u8 texture[4*4*3] = {//0xc0ffeeff, 0xff00ffff, 0xff0077ff, 0xff0f00ff};
    //     0x00,0x00,0xff,  0x00,0xff,0x00,  0x00,0x00,0xff,  0x00,0xff,0x00,
    //     0xff,0x00,0x00,  0x00,0xff,0xff,  0xff,0x00,0x00,  0x00,0xff,0xff,
    //     0x00,0x00,0xff,  0x00,0xff,0x00,  0x00,0x00,0xff,  0x00,0xff,0x00,
    //     0xff,0x00,0x00,  0x00,0xff,0xff,  0xff,0x00,0x00,  0x00,0xff,0xff,
    // };
    u32 texture[4] = {0xc0ffeeff, 0xff00ffff, 0xff0077ff, 0xff0f00ff};
    quad.set_texture(texture, 2, 2);


    string master_path = string::Create(L"D:/Users/phil/Desktop/test archive");
    string_pool top_folders = win32_GetAllFilesAndFoldersInDir(master_path);
    string_pool files = string_pool::empty();

    for (int folderI = 0; folderI < top_folders.count; folderI++) {
        if (win32_IsDirectory(top_folders[folderI])) {
            string_pool subfiles = win32_GetAllFilesAndFoldersInDir(top_folders[folderI]);
            for (int fileI = 0; fileI < subfiles.count; fileI++) {
                if (!win32_IsDirectory(subfiles[fileI].chars)) {
                    files.add(subfiles[fileI]);
                } else {
                    // wchar_t tempbuf[123];
                    // swprintf(tempbuf, L"%s is dir!\n", subfiles[fileI].chars);
                    // // OutputDebugStringW(tempbuf);
                    // MessageBoxW(0,tempbuf,0,0);
                }
            }
        }
    }

    // if (win32_IsDirectory(L"D:/Users/phil/Desktop/test archive")) MessageBox(0,"1",0,0);
    // if (win32_IsDirectory(L"D:/Users/phil/Desktop/test archive/egs")) MessageBox(0,"2",0,0);
    // if (win32_IsDirectory(L"D:/Users/phil/Desktop/test archive/egs/")) MessageBox(0,"3",0,0);

    // // string master_path = string::Create(L"D:/Users/phil/Desktop/test archive/egs");
    // // string_pool files = win32io_GetAllFilesAndFoldersInDir(master_path);
    // wchar_t buf[213];
    // swprintf(buf, L"files: %i\n", files.count);
    // OutputDebugStringW(buf);
    // for (int i = 0; i < files.count; i++) {
    //     swprintf(buf, L"%ls\n", files[i].chars);
    //     OutputDebugStringW(buf);
    //     // MessageBoxW(0,buf,0,0);
    // }


    tiles = tile_pool::empty();
    for (int i = 0; i < files.count; i++) {
        tiles.add(tile::CreateFromFile(files[i]));
    }
    // AddRandomColorToTilePool
    {
        for (int i = 0; i < tiles.count; i++) {
            u32 col = rand() & 0xff;
            col |= (rand() & 0xff) << 8;
            col |= (rand() & 0xff) << 16;
            col |= (rand() & 0xff) << 24;
            tiles[i].rand_color = col;//rand();
        }
    }
    SortTilePoolByDate(&tiles);
    ReadTileResolutions(&tiles);
    ArrangeTilesInOrder(&tiles, {0,0,(float)cw,(float)ch}); // requires resolutions to be set


    int display_quad_count = 0;
    const int MAX_DISPLAY_QUADS = 100;
    opengl_quad display_quads[MAX_DISPLAY_QUADS];
    int display_quad_tile_index[MAX_DISPLAY_QUADS]; // tile[display_quad_tile_index[i]] goes with display_quads[i]
    for (int i = 0; i < MAX_DISPLAY_QUADS; i++) {
        display_quads[i].create(0,0,1,1);
    }


    // constant-churning loop to load items on screen, and unlod those off the screen
    LaunchBackgroundLoadingLoop();
    LaunchBackgroundUnloadingLoop();

    while(running)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }


        // set frame rate
        float frame_duration = time_now() - last_time;
        if (frame_duration < target_ms_per_frame + 0.0001f) {
            DWORD sleep_ms = (DWORD)(target_ms_per_frame - frame_duration);
            if (sleep_ms > 0)
                Sleep(sleep_ms);  // should probably check if sleep has 1ms resolution first
            while (frame_duration < target_ms_per_frame)  // churn and burn the last few fractional ms if needed
                frame_duration = time_now() - last_time;
        } else {
            // frame took longer than our target fps
        }
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

        // if (measured_first_frame_time) { // don't count first frame
        //     if (actual_dt > metric_max_dt) {
        //         metric_max_dt = actual_dt;
        //     }
        // }

        // static float t = 0; t+=0.016;
        // static float r = 0; r = (cos(t *2*3.1415)+1)/2;
        // static float g = 0; g = (sin(t *2*3.1415)+1)/2;
        // static float b = 0; b = (-cos(t *2*3.1415)+1)/2;


        RECT clientRect; GetClientRect(hwnd, &clientRect);
        int cw = clientRect.right-clientRect.left;
        int ch = clientRect.bottom-clientRect.top;
        opengl_resize_if_change(cw, ch);


        // main_loop(actual_dt);


        // calc input edge triggers
        static Input last_input = {0};
        Input input = ReadInput(hwnd);
        Input keysDown = input_keys_changed(input, last_input);
        Input keysUp = input_keys_changed(last_input, input);
        last_input = input;


        // position the tiles
        {
            // arrange every frame, in case window size changed
            int tile_height = ArrangeTilesInOrder(&tiles, {0,0,(float)cw,(float)ch}); // requires resolutions to be set

            ShiftTilesBasedOnScrollPosition(&tiles, &master_scroll_delta, keysDown, ch, tile_height);
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


        // render
        opengl_clear();


        for (int tileI = 0; tileI < tiles.count; tileI++) {
            if (tiles[tileI].IsOnScreen(ch)) { // only render if on screen

                // trying to use quad list here so we can reuse textures each frame

                int existing_quad_index = -1;
                for (int displayI = 0; displayI < display_quad_count; displayI++) {
                    if (display_quad_tile_index[displayI] == tileI) { // tile already has a quad
                        existing_quad_index = displayI;
                        break;
                    }
                }
                if (existing_quad_index != -1) { // tile already has a quad
                    display_quads[existing_quad_index].set_verts(tiles[tileI].pos.x, tiles[tileI].pos.y, tiles[tileI].size.w, tiles[tileI].size.h);
                    if (tiles[tileI].texture_updated_since_last_read) {
                        bitmap img = tiles[tileI].GetImage(); // check if change before sending to gpu?
                        display_quads[existing_quad_index].set_texture(img.data, img.w, img.h);
                    }
                    display_quads[existing_quad_index].render(1);
                }
                else { // new quad needed, create here
                    int new_quad_index = display_quad_count;
                    display_quad_count++;
                    display_quad_tile_index[new_quad_index] = tileI;

                    display_quads[new_quad_index].set_verts(tiles[tileI].pos.x, tiles[tileI].pos.y, tiles[tileI].size.w, tiles[tileI].size.h);
                    bitmap img = tiles[tileI].GetImage(); // check if change before sending to gpu?
                    display_quads[new_quad_index].set_texture(img.data, img.w, img.h);
                    display_quads[new_quad_index].render(1);
                }


                // quad.set_verts(tiles[tileI].pos.x, tiles[tileI].pos.y, tiles[tileI].size.w, tiles[tileI].size.h);
                // bitmap img = tiles[tileI].GetImage(); // check if change before sending to gpu?
                // quad.set_texture(img.data, img.w, img.h); // looks like passing this every frame isn't going to cut it
                // // quad.set_texture(img.data, 1, 1);
                // quad.render(1);

            }
        }

        // look through display quads to see if any are offscreen
        // we loop through this list rather than (1) checking the tiles or
        // (2) embedding this check in the existing tile loop
        // because typically there will be less display quads than tiles
        int display_quad_indices_to_remove[100];
        int display_quad_remove_count = 0;
        for (int displayI = 0; displayI < display_quad_count; displayI++) {
            if (!tiles[display_quad_tile_index[displayI]].IsOnScreen(ch)) { // check if this display quad tile is offscreen
                display_quad_indices_to_remove[display_quad_remove_count++] = displayI;
            }
        }
        for (int i = display_quad_remove_count-1; i >= 0; i--) {
            // replace deleted with last item (note this still works if we are removing the last item
            display_quads[display_quad_indices_to_remove[i]] = display_quads[display_quad_count-1];
            display_quad_count--;
        }


        // debug display metrics
        {
            HUDPRINTRESET();

            static int debug_pip_count = 0;
            debug_pip_count = (debug_pip_count+1) % 32;
            char debug_pip_string[32 +1/*null terminator*/] = {0};
            for (int i = 0; i < 32; i++) { debug_pip_string[i] = '.'; }
            debug_pip_string[debug_pip_count] = ':';
            HUDPRINT(debug_pip_string);

            HUDPRINT("dt: %f", actual_dt);

            HUDPRINT("max dt: %f", metric_max_dt);

            HUDPRINT("ms to first frame: %f", metric_time_to_first_frame);

            int metric_media_loaded = 0;
            for (int i = 0; i < tiles.count; i++) {
                if (tiles[i].media.loaded) metric_media_loaded++;
            }
            HUDPRINT("media loaded: %i", metric_media_loaded);

            HUDPRINT("used display quads: %i", display_quad_count);

            HUDPRINT("amt off: %i", state_amt_off_anchor);


            for (int i = 0; i < tiles.count; i++) {
                rect tile_rect = {tiles[i].pos.x, tiles[i].pos.y, tiles[i].size.w, tiles[i].size.h};
                if (tile_rect.ContainsPoint(input.mouseX, input.mouseY)) {
                    HUDPRINT(tiles[i].name.ToUTF8());

                    for (int displayI = 0; displayI < display_quad_count; displayI++) {
                        if (
                            tiles[display_quad_tile_index[displayI]].pos.x == tiles[i].pos.x &&
                            tiles[display_quad_tile_index[displayI]].pos.y == tiles[i].pos.y &&
                            tiles[display_quad_tile_index[displayI]].size.w == tiles[i].size.w &&
                            tiles[display_quad_tile_index[displayI]].size.h == tiles[i].size.h
                        ) {
                            HUDPRINT(displayI);
                        }
                    }

                }
            }


            // char buf[235];
            // sprintf(buf, "dt: %f\n", actual_dt);
            // OutputDebugString(buf);
        }

        // quad.set_pos(0,0);
        // quad.render(1);

        // quad.set_pos(600-quad.cached_w/2,10);
        // quad.render(0.5);

        opengl_swap();

        if (!measured_first_frame_time) { metric_time_to_first_frame = time_now() - time_at_startup; measured_first_frame_time = true; }

        // Sleep(16); // we set frame rate above right? or should we here?
    }
    return 0;
}
