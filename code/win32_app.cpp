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
#include "ui.cpp"

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




// display quads
// todo: add bounds checking

struct display_quad {
    opengl_quad quad;
    bool is_used = false;
};

const int MAX_DISPLAY_QUADS = 100;
display_quad *display_quads;
// int display_quad_count = 0;

void display_quads_init() {
    display_quads = (display_quad*)malloc(sizeof(display_quad) * MAX_DISPLAY_QUADS);
    for (int i = 0; i < MAX_DISPLAY_QUADS; i++) {
        display_quads[i].quad.create(0,0,1,1);
        //     u32 col = rand() & 0xff;
        //     col |= (rand() & 0xff) << 8;
        //     col |= (rand() & 0xff) << 16;
        //     col |= (rand() & 0xff) << 24;
        // display_quads[i].quad.set_texture(&col, 1, 1);
        display_quads[i].is_used = false;
    }
    // display_quad_count = 0;
}

int display_quad_get_unused_index() {
    for (int i = 0; i < MAX_DISPLAY_QUADS; i++) {
        if (!display_quads[i].is_used) {
            display_quads[i].is_used = true;
            return i;
        }
    }
    OutputDebugString("ERROR: trying to use more display quads that we have, just allocate more here");
    assert(false);
    return 0;
}

void display_quad_set_index_unused(int i) {
    if (display_quads[i].is_used) {
        // display_quads[i].destroy(); // could remove texture from gpu here
        display_quads[i].is_used = false;
    }
}

// const int MAX_DISPLAY_QUADS = 100;
// // opengl_quad display_quads[MAX_DISPLAY_QUADS];
// opengl_quad *display_quads;
// int display_quad_count = 0;

// // int_queue display_quad_unused_indices;

// void display_quads_init() {
//     display_quads = (opengl_quad*)malloc(sizeof(opengl_quad) * MAX_DISPLAY_QUADS);
//     for (int i = 0; i < MAX_DISPLAY_QUADS; i++) {
//         display_quads[i].create(0,0,1,1);
//     }
//     display_quad_count = 0;

//     display_quad_unused_indices = display_quad_unused_indices.new_empty();
//     // display_quad_unused_indices = (int*)malloc(sizeof(int) * MAX_DISPLAY_QUADS);
//     // display_quad_unused_indices_count = 0;
// }

// int display_quad_get_unused_index() {
//     if (display_quad_unused_indices.count > 0) {
//         return display_quad_unused_indices.pop();
//     } else {
//         return display_quad_count++;
//     }
// }

// void display_quad_set_index_unused(int i) {
//     if (!display_quad_unused_indices.has(i))
//         display_quad_unused_indices.push(i);
// }

// end display quads



bool running = true;

#include "background.cpp"


int master_scroll_delta = 0;


void init_app(string_pool files, int cw, int ch) {

    // init tiles
    {
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
        // SortTilePoolByDate(&tiles);
        ReadTileResolutions(&tiles);
        ArrangeTilesInOrder(&tiles, {0,0,(float)cw,(float)ch}); // requires resolutions to be set
    }


    display_quads_init();


    // constant-churning loop to load items on screen, and unlod those off the screen
    LaunchBackgroundLoadingLoop();
    LaunchBackgroundUnloadingLoop();

}

bool load_tick_and_return_false_if_done(string_pool files, string_pool thumb_files, int cw, int ch) {

    // ui_progressbar(cw/2, ch2)

    opengl_clear();

    ui_text("media files: %f", files.count, cw/2, ch/2);
    ui_text("thumb files: %f", thumb_files.count, cw/2, ch/2 + UI_TEXT_SIZE);



    opengl_swap();
    return true;

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
    // u32 texture[4] = {0xc0ffeeff, 0xff00ffff, 0xff0077ff, 0xff0f00ff};
    // quad.set_texture(texture, 2, 2);


    string master_path = string::Create(L"D:/Users/phil/Desktop/test archive");
    string_pool top_folders = win32_GetAllFilesAndFoldersInDir(master_path);
    string_pool files = string_pool::empty();

    string THUMB_DIR_NAME = string::Create("~thumbs");
    for (int folderI = 0; folderI < top_folders.count; folderI++) {
        if (win32_IsDirectory(top_folders[folderI])) {
            string_pool subfiles = win32_GetAllFilesAndFoldersInDir(top_folders[folderI]);
            for (int fileI = 0; fileI < subfiles.count; fileI++) {
                if (!win32_IsDirectory(subfiles[fileI].chars)) {
                    // DEBUGPRINT(subfiles[fileI].ParentDirectoryNameReusable().ToUTF8Reusable());
                    if (subfiles[fileI].ParentDirectoryNameReusable() != THUMB_DIR_NAME)
                        files.add(subfiles[fileI]);
                } else {
                    // wchar_t tempbuf[123];
                    // swprintf(tempbuf, L"%s is dir!\n", subfiles[fileI].chars);
                    // // OutputDebugStringW(tempbuf);
                    // MessageBoxW(0,tempbuf,0,0);
                }
            }
        } else {
            // files in top folder, add?
        }
    }

    string thumb_path = master_path.Append(L"/~thumbs");
    string_pool thumb_folders = win32_GetAllFilesAndFoldersInDir(thumb_path);
    string_pool thumb_files = string_pool::empty();
    for (int folderI = 0; folderI < thumb_folders.count; folderI++) {
        if (win32_IsDirectory(thumb_folders[folderI])) {
            string_pool subfiles = win32_GetAllFilesAndFoldersInDir(thumb_folders[folderI]);
            for (int fileI = 0; fileI < subfiles.count; fileI++) {
                if (!win32_IsDirectory(subfiles[fileI].chars)) {
                    // DEBUGPRINT(subfiles[fileI].ParentDirectoryNameReusable().ToUTF8Reusable());
                    thumb_files.add(subfiles[fileI]);
                } else {
                    // wchar_t tempbuf[123];
                    // swprintf(tempbuf, L"%s is dir!\n", subfiles[fileI].chars);
                    // // OutputDebugStringW(tempbuf);
                    // MessageBoxW(0,tempbuf,0,0);
                }
            }
        } else {
            // files in top folder, add?
        }
    }

    // todo: string allocs are a bit of a mess atm

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



    bool loading = true;
    bool need_init = true;

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



        RECT clientRect; GetClientRect(hwnd, &clientRect);
        int cw = clientRect.right-clientRect.left;
        int ch = clientRect.bottom-clientRect.top;
        opengl_resize_if_change(cw, ch);





        if (loading) {
            loading = load_tick_and_return_false_if_done(files, thumb_files, cw, ch);
            continue;
        }

        if (need_init) {
            init_app(files, cw, ch);
        }




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
            tile *t = &tiles[tileI];
            if (t->IsOnScreen(ch)) { // only render if on screen

                // trying to use quad list here so we can reuse textures each frame


                if (!t->has_display_quad) {
                    int newindex = display_quad_get_unused_index();
                    t->has_display_quad = true;
                    t->display_quad_index = newindex;
                    t->texture_updated_since_last_read = true; // force new texture sent to gpu
                }


                opengl_quad *q = &display_quads[t->display_quad_index].quad;
                if (t->texture_updated_since_last_read) {
                    bitmap img = t->GetImage(actual_dt); // the bitmap memory should be freed when getimage is called again
                    q->set_texture(img.data, img.w, img.h);
                }
                else if (t->is_media_loaded) {
                    if (!t->media.IsStaticImageBestGuess()) {
                        // if video, just force send new texture every frame
                        bitmap img = t->GetImage(actual_dt); // the bitmap memory should be freed when getimage is called again
                        q->set_texture(img.data, img.w, img.h);
                    }
                }
                q->set_verts(t->pos.x, t->pos.y, t->size.w, t->size.h);
                q->render(1);
            }
            else { // tile is offscreen
                if (t->has_display_quad) {
                    display_quad_set_index_unused(t->display_quad_index);
                    t->has_display_quad = false;
                    t->display_quad_index = 0;
                }
            }


        }

        // debug display metrics
        static bool show_debug_console = true;
        if (keysDown.tilde) show_debug_console = !show_debug_console;
        if (show_debug_console)
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

            int display_quad_count = 0;
            for (int i = 0; i < display_quad_count; i++) {
                if (display_quads[i].is_used) display_quad_count++;
            }
            HUDPRINT("used display quads: %i", display_quad_count);
            // HUDPRINT("unused display indices: %i", display_quad_unused_indices.count);

            HUDPRINT("amt off: %i", state_amt_off_anchor);


            for (int i = 0; i < tiles.count; i++) {
                rect tile_rect = {tiles[i].pos.x, tiles[i].pos.y, tiles[i].size.w, tiles[i].size.h};
                if (tile_rect.ContainsPoint(input.mouseX, input.mouseY)) {
                    HUDPRINT("name: %s", tiles[i].name.ToUTF8());

                    HUDPRINT("quad index: %i", tiles[i].display_quad_index);
                    HUDPRINT("has disp quad: %i", (int)tiles[i].has_display_quad);

                    if (tiles[i].media.vfc)
                        HUDPRINT("format name: %s", (char*)tiles[i].media.vfc->iformat->name);

                    HUDPRINT("fps: %f", (float)tiles[i].media.fps);

                    // HUDPRINT("frames: %f", (float)tiles[i].media.durationSeconds);
                    // HUDPRINT("frames: %f", (float)tiles[i].media.totalFrameCount);

                    if (tiles[i].media.vfc)
                        HUDPRINT("is image: %i", (int)tiles[i].media.IsStaticImageBestGuess());

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
