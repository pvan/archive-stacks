


// bool GetCachedResolutionIfPossible(string path, v2 *result) {
//     FILE* file = fopen (path.ToUTF8Reusable(), "r");
//     if (!file) { return false; }
//     fscanf (file, "%f%f", &result->x, &result->y);
//     fclose (file);
//     return true;
// }

void CreateCachedMetadataFile(string origpath, string metapath) {
    v2 res = {0,0};
    if (ffmpeg_can_open(origpath)) {
        res = ffmpeg_GetResolution(origpath);
    }
    // todo: should we set any other limitations here?
    // maybe if we can't load it, pull res from any thumbs that exist?
    if (res.w == 0) res.w = 10;
    if (res.h == 0) res.h = 10;
    CreateCachedResolution(metapath, res);
}

int_pool item_indices_without_thumbs;
int_pool item_indices_without_metadata;

DWORD WINAPI RunBackgroundThumbnailThread( LPVOID lpParam ) {

    for (int i = item_indices_without_metadata.count-1; i >= 0; i--) {
        // string metapath = ItemPathToSubfolderPath(items_without_matching_metadata[i], L"~metadata");
        string fullpath = items[item_indices_without_metadata[i]].fullpath;
        string metadatapath = items[item_indices_without_metadata[i]].metadatapath;
        CreateCachedMetadataFile(fullpath, metadatapath);
        item_indices_without_metadata.count--; // should add .pop()
        // free(metapath.chars); // should just use the same memory
    }

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

    for (int i = item_indices_without_thumbs.count-1; i >= 0; i--) {
        string fullpath = items[item_indices_without_thumbs[i]].fullpath;
        string thumbpath = items[item_indices_without_thumbs[i]].thumbpath;
        if (ffmpeg_can_open(thumbpath)) {
            DownresFileAtPathToPath(fullpath, thumbpath);
        } else  {
            CreateDummyThumb(fullpath, thumbpath);
        }
        item_indices_without_thumbs.count--; // should add .pop()
        // free(thumbpath.chars); // should just use the same memory
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

