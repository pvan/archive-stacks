



void CreateCachedMetadataFile(string origpath, string thumbpath, string metapath) {
    v2 res = {0,0};
    if (ffmpeg_can_open(origpath)) {
        res = ffmpeg_GetResolution(origpath);
    }
    // fallback to using thumbnail resolution
    else if (ffmpeg_can_open(thumbpath)) {
        res = ffmpeg_GetResolution(thumbpath);
    }
    // if can't get resoution from either, hmm...
    else {
        res = {10, 10};
    }
    CreateCachedResolution(metapath, res);
}

int_pool item_indices_without_thumbs;
int_pool item_indices_without_metadata;

DWORD WINAPI RunBackgroundThumbnailThread( LPVOID lpParam ) {

    // read paths
    for (int i = 0; i < items.count; i++) {
        // items[i].thumbpath = ItemPathToSubfolderPath(items[i].fullpath, L"~thumbs", L".bmp");
        // items[i].metadatapath = ItemPathToSubfolderPath(items[i].fullpath, L"~metadata", L".txt");

        // for now, special case for txt...
        // we need txt thumbs to be something other than txt so we can open them
        // with our "ignore all .txt files" ffmpeg code
        // but we need most thumbs to have original extensions to (for example) animate correctly
        if (StringEndsWith(items[i].fullpath.chars, L".txt")) {
            items[i].thumbpath = ItemPathToSubfolderPath(items[i].fullpath, L"~thumbs", L".bmp");
        } else {
            items[i].thumbpath = ItemPathToSubfolderPath(items[i].fullpath, L"~thumbs", L"");
        }
        items[i].metadatapath = ItemPathToSubfolderPath(items[i].fullpath, L"~metadata", L"");
    }

    // create work queue for rest of loading thread (do after reading paths)
    item_indices_without_thumbs = int_pool::empty();
    item_indices_without_metadata = int_pool::empty();
    for (int i = 0; i < items.count; i++) {
        if (!win32_PathExists(items[i].thumbpath)) {
            item_indices_without_thumbs.add(i);
            DEBUGPRINT("this doesn't exist? %s\n", items[i].thumbpath.ToUTF8Reusable());
        }
        if (!win32_PathExists(items[i].metadatapath)) {
            item_indices_without_metadata.add(i);
            DEBUGPRINT("this doesn't exist? %s\n", items[i].metadatapath.ToUTF8Reusable());
        }
    }

    // // force re-create thumbnails
    // // todo: temp dev-only, toggle as needed
    // {
    //     for (int i = 0; i < items.count; i++) {
    //         if (!ffmpeg_can_open(items[i].fullpath)) { item_indices_without_thumbs.add(i); }
    //     }
    // }

    // create cached thumbnail files
    for (int i = item_indices_without_thumbs.count-1; i >= 0; i--) {
        // item it = items[item_indices_without_thumbs[i]];
        string fullpath = items[item_indices_without_thumbs[i]].fullpath;
        string thumbpath = items[item_indices_without_thumbs[i]].thumbpath;
        string justname = items[item_indices_without_thumbs[i]].justname();
        if (ffmpeg_can_open(fullpath)) {
            DEBUGPRINT("creating media thumb for %s\n", fullpath.ToUTF8Reusable());
            DownresFileAtPathToPath(fullpath, thumbpath);
        } else  {
            DEBUGPRINT("creating dummy thumb for %s\n", fullpath.ToUTF8Reusable());
            CreateDummyThumb(fullpath, thumbpath, justname);
        }
        item_indices_without_thumbs.count--; // should add .pop()
    }

    // create cached metadata files
    // do metadata second because it uses thumbnails as a fallback
    for (int i = item_indices_without_metadata.count-1; i >= 0; i--) {
        string fullpath = items[item_indices_without_metadata[i]].fullpath;
        string thumbpath = items[item_indices_without_metadata[i]].thumbpath;
        string metadatapath = items[item_indices_without_metadata[i]].metadatapath;
        DEBUGPRINT("creating metadata file for %s\n", fullpath.ToUTF8Reusable());
        CreateCachedMetadataFile(fullpath, thumbpath, metadatapath);
        item_indices_without_metadata.count--; // should add .pop()
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
                    tiles[i].LoadMedia(tiles[i].paths.thumbpath);
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

