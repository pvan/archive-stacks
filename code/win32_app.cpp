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
void DEBUGPRINT(char *s, float f1, float f2) { sprintf(debugprintbuffer, s, f1, f2); OutputDebugString(debugprintbuffer); }

#include "v2.cpp"
#include "rect.cpp"
#include "string.cpp"
#include "bitmap.cpp"
#include "pools.cpp"

#include "win32_input.cpp"
#include "win32_icon/icon.h"
#include "win32_opengl.cpp"
#include "win32_file.cpp"
#include "win32_system.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../lib/stb_image_write.h"

#include "ffmpeg.cpp"
#include "data.cpp"
#include "tile.cpp"
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

#include "background.cpp"



int master_scroll_delta = 0;
int master_ctrl_scroll_delta = 0;

float master_desired_tile_width = 200;


// done once after startup loading is done
void init_app(item_pool all_items, int cw, int ch) {

    // SortTilePoolByDate(&tiles);
    ArrangeTilesInOrder(&tiles, master_desired_tile_width, cw); // requires resolutions to be set

    // constant-churning loop to load items on screen, and unlod those off the screen
    LaunchBackgroundLoadingLoop();
    LaunchBackgroundUnloadingLoop();
}

void load_tick(item_pool all_items, int cw, int ch) {

    // ui_progressbar(cw/2, ch2)

    opengl_clear();

    int line = -1;

    ui_text("media files: %.0f", all_items.count, cw/2, ch/2 + UI_TEXT_SIZE*line++);
    // ui_text("thumb files: %f", thumb_files.count, cw/2, ch/2 + UI_TEXT_SIZE*line++);
    // ui_text("metadata files: %f", thumb_files.count, cw/2, ch/2 + UI_TEXT_SIZE*line++);

    line++;
    ui_text("items_without_matching_thumbs: %.0f", item_indices_without_thumbs.count, cw/2, ch/2 + UI_TEXT_SIZE*line++);
    // ui_text("thumbs_without_matching_item: %f", thumbs_without_matching_item.count, cw/2, ch/2 + UI_TEXT_SIZE*line++);

    line++;
    ui_text("items_without_matching_metadata: %.0f", item_indices_without_metadata.count, cw/2, ch/2 + UI_TEXT_SIZE*line++);

    line++;
    ui_text(loading_status_msg, cw/2, ch/2 + UI_TEXT_SIZE*line++);
    ui_text("files: %.0f of %.0f", loading_reusable_count, loading_reusable_max, cw/2, ch/2 + UI_TEXT_SIZE*line++);

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


    ffmpeg_init();

    ttf_init();

    opengl_quad quad;
    quad.create(0,0,300,200);



    string master_path = string::Create(L"E:\\inspiration test folder");

    // create item list with fullpath populated
    // just adapt old method for now
    string_pool itempaths = FindAllItemPaths(master_path);
    items = item_pool::empty();
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

        // position the tiles
        static int tile_index_mouse_was_on = 0;
        {
            // arrange every frame, in case window size changed
            // todo: could just arrange if window resize / tag settigns change / etc

            int col_count = CalcColumnsNeededForDesiredWidth(master_desired_tile_width, cw);

            if (master_ctrl_scroll_delta > 0) { col_count++; master_desired_tile_width = CalcWidthToFitXColumns(col_count, cw); }
            if (master_ctrl_scroll_delta < 0) { col_count--; master_desired_tile_width = CalcWidthToFitXColumns(col_count, cw); }
            master_ctrl_scroll_delta = 0; // done using this in this frame


            int tiles_height = ArrangeTilesInOrder(&tiles, master_desired_tile_width, cw); // requires resolutions to be set

            static int last_scroll_pos = 0;
            int scroll_pos = CalculateScrollPosition(last_scroll_pos, master_scroll_delta, keysDown, ch, tiles_height);
            last_scroll_pos = scroll_pos;
            master_scroll_delta = 0; // done using this in this frame

            ShiftTilesBasedOnScrollPosition(&tiles, last_scroll_pos);

            tile_index_mouse_was_on = TileIndexMouseIsOver(tiles, input.mouseX, input.mouseY);
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
                        bitmap img = t->GetImage(actual_dt); // the bitmap memory should be freed when getimage is called again
                        t->display_quad.set_texture(img.data, img.w, img.h);
                        t->display_quad_texture_sent_to_gpu = true;
                    }
                }

                t->display_quad.set_verts(t->pos.x, t->pos.y, t->size.w, t->size.h);
                t->display_quad.render(1);

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

            HUDPRINT("tiles: %i", tiles.count);
            HUDPRINT("items: %i", items.count);


            // HUDPRINT("amt off: %i", state_amt_off_anchor);


            // for (int i = 0; i < tiles.count; i++) {
            //     rect tile_rect = {tiles[i].pos.x, tiles[i].pos.y, tiles[i].size.w, tiles[i].size.h};
            //     if (tile_rect.ContainsPoint(input.mouseX, input.mouseY)) {

                    // can just used our value calc'd above now:
                    int i = tile_index_mouse_was_on;
                    HUDPRINT("tile_index_mouse_was_on: %i", tile_index_mouse_was_on);

                    HUDPRINT("name: %s", tiles[i].name.ToUTF8());

                    if (tiles[i].media.vfc && tiles[i].media.vfc->iformat)
                        HUDPRINT("format name: %s", (char*)tiles[i].media.vfc->iformat->name);

                    HUDPRINT("fps: %f", (float)tiles[i].media.fps);

                    // HUDPRINT("frames: %f", (float)tiles[i].media.durationSeconds);
                    // HUDPRINT("frames: %f", (float)tiles[i].media.totalFrameCount);

                    if (tiles[i].media.vfc && tiles[i].media.vfc->iformat)
                        HUDPRINT(tiles[i].media.IsStaticImageBestGuess() ? "image" : "video");

                    wc *directory = CopyJustParentDirectoryName(tiles[i].paths.fullpath.chars);
                    string temp = string::Create(directory);
                    HUDPRINT("folder: %s", temp.ToUTF8Reusable());
                    free(directory);

                    HUDPRINT("tile size: %f, %f", tiles[i].size.w, tiles[i].size.h);


            //     }
            // }

            char buf[235];
            sprintf(buf, "dt: %f\n", actual_dt);
            OutputDebugString(buf);
        }


        opengl_swap();

        if (!measured_first_frame_time) { metric_time_to_first_frame = time_now() - time_at_startup; measured_first_frame_time = true; }


        // Sleep(16); // we set frame rate above right? or should we here?
    }
    return 0;
}
