



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


char *loading_status_msg = "Starting...";
int loading_reusable_count = 0;
int loading_reusable_max = 0;

DWORD WINAPI RunBackgroundStartupThread( LPVOID lpParam ) {

    // read paths
    for (int i = 0; i < items.count; i++) {
        loading_status_msg = "Creating cache paths...";

        // for now, special case for txt...
        // we need txt thumbs to be something other than txt so we can open them
        // with our ffmpeg code that specifically "ignores all .txt files" atm
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
        loading_status_msg = "Looking for existing caches...";
        loading_reusable_count = i;
        loading_reusable_max = items.count;

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
        loading_status_msg = "Creating thumbnails...";
        loading_reusable_count = i;
        loading_reusable_max = item_indices_without_thumbs.count;

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
        loading_status_msg = "Creating metadata caches...";
        loading_reusable_count = i;
        loading_reusable_max = item_indices_without_metadata.count;

        string fullpath = items[item_indices_without_metadata[i]].fullpath;
        string thumbpath = items[item_indices_without_metadata[i]].thumbpath;
        string metadatapath = items[item_indices_without_metadata[i]].metadatapath;
        DEBUGPRINT("creating metadata file for %s\n", fullpath.ToUTF8Reusable());
        CreateCachedMetadataFile(fullpath, thumbpath, metadatapath);
        item_indices_without_metadata.count--; // should add .pop()
    }


    // init tiles
    {
        tiles = tile_pool::empty();
        for (int i = 0; i < items.count; i++) {
            loading_status_msg = "Reading file timestamps...";
            loading_reusable_count = i;
            loading_reusable_max = items.count;

            // TODO: we should put timestamps in the metadata cache
            // that way we potentially dont have to touch the original files at all on startup
            // (would cut loading time on a cold hdd in half)
            tiles.add(tile::CreateFromItem(items[i])); // reads timestamps! = slow on cold hdd
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

        // ReadCachedTileResolutions(&tiles);
        {
            for (int i = 0; i < tiles.count; i++) {
                loading_status_msg = "Reading resolutions from cached metadata...";
                loading_reusable_count = i;
                loading_reusable_max = tiles.count;

                item_resolutions.add({0,0});
                v2& res = item_resolutions[i]; //tiles.pool[i];

                // if (GetCachedResolutionIfPossible(t.paths.metadatapath, &t.resolution)) {
                if (GetCachedResolutionIfPossible(items[i].metadatapath, &res)) {
                    // DEBUGPRINT("read res: %f, %f\n", t.resolution.x, t.resolution.y);
                    // successfully loaded cached resolution
                    continue;
                } else {
                    // consider: should we try reading orig files here for resolution?
                    // 1 - we should have metadata for any file now
                    //     (since they are creating during loading, before this is done)
                    // but 2 - we are doing this async during loading as well now,
                    //         so we should be okay that it might take a while
                    // for now: don't try to load.. should we maybe even assert(false)?
                    // DEBUGPRINT("Couldn't load cached resolution from metadata for item: %s", t.paths.fullpath.ToUTF8Reusable());
                    DEBUGPRINT("Couldn't load cached resolution from metadata for item: %s", items[i].fullpath.ToUTF8Reusable());
                    res = {7,7};
                    assert(false);
                }

            }
        }

        // for (int i = 0; i < tiles.count; i++) {
        //     DEBUGPRINT("%f, %f \n", tiles[i].resolution.x, tiles[i].resolution.y);
        // }

        // read cached tag list from metadata file (combine with reading cached resolution?)
        {
            loading_status_msg = "Reading item tags...";

            // first read list of master tags
            tag_list = ReadTagListFromFileOrSomethingUsableOtherwise(master_path);

            // DEBUGPRINT("should be....");
            // for (int i = 0 ; i < tag_list.count; i++) {
            //     DEBUGPRINT(tag_list[i].ToUTF8Reusable());
            // }
            // DEBUGPRINT("total %i\n", tag_list.count);


            for (int i = 0; i < items.count; i++) {
                loading_reusable_count = i;
                loading_reusable_max = items.count;

                if (PopulateTagsFromCachedFileIfPossible(&items[i])) {
                    DEBUGPRINT("cached tags read for %s\n", tiles[i].name.ToUTF8Reusable());
                } else {
                    // fallback to directory orig file is in
                    if (PopulateTagFromPath(&items[i])) {
                        // DEBUGPRINT("read tag from directory for %s\n", tiles[i].name.ToUTF8Reusable());
                    } else {
                        DEBUGPRINT("unable to read tag from directory for %s\n", tiles[i].name.ToUTF8Reusable());
                        assert(false);
                    }
                }
            }

            SaveTagList();
        }


    }

    loading_status_msg = "Done loading!";

    loading = false;
    return 0;
}
void LaunchBackgroundStartupLoop() {
    CreateThread(0, 0, RunBackgroundStartupThread, 0, 0, 0);
}


DWORD WINAPI RunBackgroundLoadingThread( LPVOID lpParam ) {
    while (running) {
        for (int i = 0; i < tiles.count; i++) {
            if (tiles[i].needs_loading) {
                if (!tiles[i].is_media_loaded) {
                    // tiles[i].LoadMedia(tiles[i].fullpath);
                    tiles[i].LoadMedia(items[i].thumbpath);
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

