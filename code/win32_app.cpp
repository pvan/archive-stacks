#include <windows.h>
#include <stdio.h>

#include <assert.h>
#include <math.h>

#include "types.h"
#include "v2.cpp"
#include "string.cpp"
#include "pools.cpp"

#include "win32_icon/icon.h"
#include "win32_opengl.cpp"
#include "win32_file.cpp"

#include "tile.cpp"

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


bool running = true;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CLOSE:
            running = false;
            break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

    RECT win_rect = {0,0,600,400};
    int cw = win_rect.right;
    int ch = win_rect.bottom;

    timeBeginPeriod(1); // set resolution of sleep

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


    float target_fps = 30;
    float target_ms_per_frame = (1.0f / target_fps) * 1000.0f;
    time_init();
    float last_time = time_now();


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


    tile_pool tiles = tile_pool::empty();
    for (int i = 0; i < files.count; i++) {
        tiles.add(tile::CreateFromFile(files[i]));
    }

    SortTilePoolByDate(&tiles);

    // wchar_t buf2[213];
    // swprintf(buf2, L"files: %i\n", tiles.count);
    // OutputDebugStringW(buf2);
    // for (int i = 0; i < tiles.count; i++) {
    //     swprintf(buf2, L"%ls\n", tiles[i].fullpath.chars);
    //     OutputDebugStringW(buf2);
    //     // MessageBoxW(0,buf2,0,0);
    // }

    ReadTileResolutions(&tiles);

    ArrangeTilesInOrder(&tiles); // requires resolutions to be set



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

        // static float t = 0; t+=0.016;
        // static float r = 0; r = (cos(t *2*3.1415)+1)/2;
        // static float g = 0; g = (sin(t *2*3.1415)+1)/2;
        // static float b = 0; b = (-cos(t *2*3.1415)+1)/2;


        RECT clientRect; GetClientRect(hwnd, &clientRect);
        int cw = clientRect.right-clientRect.left;
        int ch = clientRect.bottom-clientRect.top;
        opengl_resize_if_change(cw, ch);


        // main_loop(actual_dt);



        // render
        opengl_clear();

        for (int i = 0; i < tiles.count; i++) {
            quad.set_verts(tiles[i].pos.x, tiles[i].pos.y, tiles[i].size.w, tiles[i].size.h);
            quad.render(1);
        }

        // quad.set_pos(0,0);
        // quad.render(1);

        // quad.set_pos(600-quad.cached_w/2,10);
        // quad.render(0.5);

        opengl_swap();


        // Sleep(16); // we set frame rate above right? or should we here?
    }
    return 0;
}
