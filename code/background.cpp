



DWORD WINAPI RunBackgroundThumbnailThread( LPVOID lpParam ) {

    // for (int i = 0; i < items.count; i++) {
    //     if (!win32_PathExists(items[i].thumbpath128.chars)) {
    //         if (ffmpeg_can_open(items[i].thumbpath128)) {
    //             DownresFileAtPathToPath(items[i].fullpath, items[i].thumbpath128);
    //         } else  {
    //             CreateDummyThumb(items[i].fullpath, items[i].thumbpath128);
    //         }
    //     }
    // }
    // loading = false;

    for (int i = items_without_matching_thumbs.count-1; i >= 0; i--) {
        string thumbpath = string::Create(CopyItemPathAndConvertToThumbPath(items_without_matching_thumbs[i].chars));
        if (ffmpeg_can_open(thumbpath)) {
            DownresFileAtPathToPath(items_without_matching_thumbs[i], thumbpath);
        } else  {
            CreateDummyThumb(items_without_matching_thumbs[i], thumbpath);
        }
        existing_thumbs.add(thumbpath);
        items_without_matching_thumbs.count--; // should add .pop()
        free(thumbpath.chars); // should just use the same memory
    }
    loading = false;
    return 0;
}
void LaunchBackgroundThumbnailLoop() {
    CreateThread(0, 0, RunBackgroundThumbnailThread, 0, 0, 0);
}


DWORD WINAPI RunBackgroundLoadingThread( LPVOID lpParam ) {
    while (running) {
        for (int i = 0; i < tiles.count; i++) {
            if (tiles[i].needs_loading) {
                if (!tiles[i].is_media_loaded) {
                    // tiles[i].LoadMedia(tiles[i].fullpath);
                    tiles[i].LoadMedia(tiles[i].thumbpath);
                    // DEBUGPRINT("loading: %s\n", tiles[i].thumbpath.ToUTF8Reusable());
                }
            }
        }
        // static bool first_loop = true;
        // if (first_loop) {
        //     OutputDebugString("done loading\n");
        // }
        // first_loop = false;
        Sleep(16); // hmm
    }
    return 0;
}
void LaunchBackgroundLoadingLoop() {
    CreateThread(0, 0, RunBackgroundLoadingThread, 0, 0, 0);
}



DWORD WINAPI RunBackgroundUnloadingThread( LPVOID lpParam ) {
    while (running) {
        for (int i = 0; i < tiles.count; i++) {
            if (tiles[i].needs_unloading) {
                if (tiles[i].is_media_loaded) {
                    tiles[i].UnloadMedia();
                }
            }
        }
        Sleep(16); // hmm
    }
    return 0;
}
void LaunchBackgroundUnloadingLoop() {
    CreateThread(0, 0, RunBackgroundUnloadingThread, 0, 0, 0);
}

