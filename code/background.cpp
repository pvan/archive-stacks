



DWORD WINAPI RunBackgroundThumbnailThread( LPVOID lpParam ) {
    // while (running && loading) {
        for (int i = items_without_matching_thumbs.count-1; i >= 0; i--) {
            string thumbpath = string::Create(CopyItemPathAndConvertToThumbPath(items_without_matching_thumbs[i].chars));
            DownresFileAtPathToPath(items_without_matching_thumbs[i], thumbpath);
            thumb_files.add(thumbpath);
            items_without_matching_thumbs.count--; // should add .pop()
            free(thumbpath.chars); // should just use the same memory
        }
        loading = false;
    // }
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
                    tiles[i].LoadMedia();
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

