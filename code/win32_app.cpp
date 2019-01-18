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


// end display quads



bool running = true;

bool loading = true;
bool need_init = true;

#include "background.cpp"


int master_scroll_delta = 0;


void init_app(item_pool all_items, int cw, int ch) {

    // init tiles
    {
        tiles = tile_pool::empty();
        for (int i = 0; i < all_items.count; i++) {
            // tiles.add(tile::CreateFromFile(all_items[i].fullpath));
            tiles.add(tile::CreateFromItem(all_items[i]));
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
        ReadTileResolutions(&tiles); // should be loading cached files that are created in background while loading now
        ArrangeTilesInOrder(&tiles, {0,0,(float)cw,(float)ch}); // requires resolutions to be set
    }


    display_quads_init();


    // constant-churning loop to load items on screen, and unlod those off the screen
    LaunchBackgroundLoadingLoop();
    LaunchBackgroundUnloadingLoop();

}

void load_tick(item_pool all_items, int cw, int ch) {

    // ui_progressbar(cw/2, ch2)

    opengl_clear();

    ui_text("media files: %f", all_items.count, cw/2, ch/2 + UI_TEXT_SIZE*-1);
    // ui_text("thumb files: %f", thumb_files.count, cw/2, ch/2 + UI_TEXT_SIZE*0);
    // ui_text("metadata files: %f", thumb_files.count, cw/2, ch/2 + UI_TEXT_SIZE*1);

    ui_text("items_without_matching_thumbs: %f", item_indices_without_thumbs.count, cw/2, ch/2 + UI_TEXT_SIZE*3);
    // ui_text("thumbs_without_matching_item: %f", thumbs_without_matching_item.count, cw/2, ch/2 + UI_TEXT_SIZE*3);

    ui_text("items_without_matching_metadata: %f", item_indices_without_metadata.count, cw/2, ch/2 + UI_TEXT_SIZE*5);

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

    // string s1 = string::Create(L"D:/Users/phil/Desktop/test archive/art/xwfymstga9b11.png");
    // string s2 = CopyItemPathAndConvertToThumbPath(s1);
    // OutputDebugStringW(s2.chars);
    // return 0;

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
    PopulateItemPaths(); // fill in thumb paths, metdata path, etc.


    // create work queue for loading thread
    item_indices_without_thumbs = int_pool::empty();
    item_indices_without_metadata = int_pool::empty();
    for (int i = 0; i < items.count; i++) {
        if (!win32_PathExists(items[i].thumbpath.chars)) { item_indices_without_thumbs.add(i); }
        if (!win32_PathExists(items[i].metadatapath.chars)) { item_indices_without_metadata.add(i); }
    }



    // // string master_path = string::Create(L"D:/Users/phil/Desktop/test archive");
    // string master_path = string::Create(L"E:\\inspiration test folder");
    // files_full = FindAllItemPaths(master_path);

    // for (int i = 0; i < files_full.count; i++) {
    //     files_thumb.add(ItemPathToSubfolderPath(files_full[i], L"~thumbs"));
    //     files_metadata.add(ItemPathToSubfolderPath(files_full[i], L"~metadata"));
    // }

    // existing_thumbs = FindAllSubfolderPaths(master_path, L"/~thumbs");
    // items_without_matching_thumbs = ItemsInFirstPoolButNotSecond(files_thumb, existing_thumbs);
    // thumbs_without_matching_item = ItemsInFirstPoolButNotSecond(existing_thumbs, files_thumb);

    // existing_metadata = FindAllSubfolderPaths(master_path, L"/~metadata");
    // items_without_matching_metadata = ItemsInFirstPoolButNotSecond(files_metadata, existing_metadata);
    // metadata_without_matching_item = ItemsInFirstPoolButNotSecond(existing_metadata, files_metadata);



    // DEBUGPRINT("items_without_matching_thumbs: (%i)\n", items_without_matching_thumbs.count);
    // for (int i = 0; i < items_without_matching_thumbs.count; i++) {
    //     DEBUGPRINT(items_without_matching_thumbs[i].ToUTF8Reusable());
    // }
    // DEBUGPRINT("thumbs_without_matching_item: (%i)\n", thumbs_without_matching_item.count);
    // for (int i = 0; i < thumbs_without_matching_item.count; i++) {
    //     DEBUGPRINT(thumbs_without_matching_item[i].ToUTF8Reusable());
    // }

    // DEBUGPRINT("items_without_matching_metadata: (%i)\n", items_without_matching_metadata.count);
    // for (int i = 0; i < items_without_matching_metadata.count; i++) {
    //     DEBUGPRINT(items_without_matching_metadata[i].ToUTF8Reusable());
    // }
    // DEBUGPRINT("metadata_without_matching_item: (%i)\n", metadata_without_matching_item.count);
    // for (int i = 0; i < metadata_without_matching_item.count; i++) {
    //     DEBUGPRINT(metadata_without_matching_item[i].ToUTF8Reusable());
    // }
    // return 0;


    LaunchBackgroundThumbnailLoop();
    // LaunchBackgroundUnloadingLoop();


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





        if (loading) {
            load_tick(items, cw, ch);
            continue;
        }

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
        {
            // arrange every frame, in case window size changed
            // todo: could just arrange if window resize / tag settigns change
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

                // bitmap img = t->GetImage(actual_dt); // the bitmap memory should be freed when getimage is called again
                // quad.set_texture(img.data, img.w, img.h);
                // quad.set_verts(t->pos.x, t->pos.y, t->size.w, t->size.h);
                // quad.render(1);

                ////

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


            // } else {
            //     if (t->display_quad_created) {
            //         t->display_quad_created = false;
            //         t->display_quad_texture_sent_to_gpu = false;
            //         t->display_quad.destroy();
            //     }



                ////


            //     // trying to use quad list here so we can reuse textures each frame


            //     if (!t->has_display_quad) {
            //         int newindex = display_quad_get_unused_index();
            //         t->has_display_quad = true;
            //         t->display_quad_index = newindex;
            //         t->texture_updated_since_last_read = true; // force new texture sent to gpu
            //     }


            //     opengl_quad *q = &display_quads[t->display_quad_index].quad;
            //     if (t->texture_updated_since_last_read) {
            //         bitmap img = t->GetImage(actual_dt); // the bitmap memory should be freed when getimage is called again
            //         q->set_texture(img.data, img.w, img.h);
            //     }
            //     else if (t->is_media_loaded) {
            //         if (!t->media.IsStaticImageBestGuess()) {
            //             // if video, just force send new texture every frame
            //             bitmap img = t->GetImage(actual_dt); // the bitmap memory should be freed when getimage is called again
            //             q->set_texture(img.data, img.w, img.h);
            //         }
            //     }
            //     q->set_verts(t->pos.x, t->pos.y, t->size.w, t->size.h);
            //     q->render(1);
            // }
            // else { // tile is offscreen
            //     if (t->has_display_quad) {
            //         display_quad_set_index_unused(t->display_quad_index);
            //         t->has_display_quad = false;
            //         t->display_quad_index = 0;
            //     }


                ////

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

            // int display_quad_count = 0;
            // for (int i = 0; i < display_quad_count; i++) {
            //     if (display_quads[i].is_used) display_quad_count++;
            // }
            // HUDPRINT("used display quads: %i", display_quad_count);
            // // HUDPRINT("unused display indices: %i", display_quad_unused_indices.count);

            HUDPRINT("amt off: %i", state_amt_off_anchor);


            for (int i = 0; i < tiles.count; i++) {
                rect tile_rect = {tiles[i].pos.x, tiles[i].pos.y, tiles[i].size.w, tiles[i].size.h};
                if (tile_rect.ContainsPoint(input.mouseX, input.mouseY)) {
                    HUDPRINT("name: %s", tiles[i].name.ToUTF8());

                    HUDPRINT("quad index: %i", tiles[i].display_quad_index);
                    HUDPRINT("has disp quad: %i", (int)tiles[i].has_display_quad);

                    if (tiles[i].media.vfc && tiles[i].media.vfc->iformat)
                        HUDPRINT("format name: %s", (char*)tiles[i].media.vfc->iformat->name);

                    HUDPRINT("fps: %f", (float)tiles[i].media.fps);

                    // HUDPRINT("frames: %f", (float)tiles[i].media.durationSeconds);
                    // HUDPRINT("frames: %f", (float)tiles[i].media.totalFrameCount);

                    if (tiles[i].media.vfc && tiles[i].media.vfc->iformat)
                        HUDPRINT("is image: %i", (int)tiles[i].media.IsStaticImageBestGuess());

                    wc *directory = CopyJustParentDirectoryName(tiles[i].paths.fullpath.chars);
                    string temp = string::Create(directory);
                    HUDPRINT("folder: %s", temp.ToUTF8Reusable());
                    free(directory);

                    HUDPRINT("tile index: %i", i);

                }
            }

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
