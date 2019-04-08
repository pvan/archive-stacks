



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

int_pool item_indices_without_thumbs; // todo: move to data tab? or keep bg related stuff in bg?
// int_pool item_indices_without_metadata;


char *loading_status_msg = "Starting...";
int loading_reusable_count = 0;
int loading_reusable_max = 0;

bool background_startup_thread_launched = false;
DWORD WINAPI RunBackgroundStartupThread( LPVOID lpParam ) {


    // 1. free anything already created...

    // todo: some of these we probably don't have to free and can just reuse the memory

    loading_status_msg = "Unloading previous media...";
    loading_reusable_max = tiles.count;
    // 1a. list contents
    for (int i = 0; i < tiles.count; i++) {
        loading_reusable_count = i;
        tiles[i].UnloadMedia();
        // loading_reusable_count = 100000+i;
        // if (tiles[i].name.chars) free(tiles[i].name.chars);
    }
    loading_status_msg = "Unloading previous tags...";
    loading_reusable_max = tag_list.count;
    for (int i = 0; i < tag_list.count; i++) {
        loading_reusable_count = i;
        if (tag_list.pool[i].chars) free(tag_list.pool[i].chars);
    }
    // we separated the tag lists out from item so we could tightly pack all item paths into one memory allocation eventually
    // because free()ing each was taking forever
    // turns out it was our mem debug wrapper around free that was slow
    // freeing ~5k-10k times here was super fast without memdebug enabled
    loading_status_msg = "Unloading previous tag lists...";
    loading_reusable_max = item_tags.count;
    for (int i = 0; i < item_tags.count; i++) {
        loading_reusable_count = i;
        item_tags[i].free_pool();
    }
    // the idea is we could replace this loop with a single alloc/free
    // if we use a memory pool for these lists (since they never change it should be easy)
    loading_status_msg = "Unloading previous item's paths...";
    loading_reusable_max = items.count;
    for (int i = 0; i < items.count; i++) {
        loading_reusable_count = i;
        items[i].free_all();
    }

    // 1b. list themselves
    loading_status_msg = "Unloading previous item lists...";
    items.free_pool();
    tiles.free_pool();
    tag_list.free_pool();
    item_tags.free_pool();

    loading_status_msg = "Unloading previous item metadata lists...";
    modifiedTimes.free_pool();
    item_resolutions.free_pool();
    item_resolutions_valid.free_pool();

    loading_status_msg = "Unloading previous item index lists...";
    display_list.free_pool();
    browse_tag_filter.free_all();
    view_tag_filter.free_all();


    memdebug_print();
    memdebug_reset();

    // 2. load everything for new path...

    // create item list with fullpath populated
    items = CreateItemListFromMasterPath(master_path);

    // create an tag list for every item (all empty to start)
    for (int i = 0; i < items.count; i++)
        item_tags.add(int_pool::new_empty());

    // not needed, now we create thumbnail path for each item when creating that item
    // (could move that item creation here, though, if too slow on main thread,
    //  but should be fine, it's just string manipulation -- no hd reads)
    // // read paths
    // loading_status_msg = "Creating cache paths...";
    // PopulateThumbnailPathsForAllItems(&items);

    // create work queue for rest of loading thread (do after reading paths)
    item_indices_without_thumbs = int_pool::new_empty();
    // item_indices_without_metadata = int_pool::new_empty();
    for (int i = 0; i < items.count; i++) {
        loading_status_msg = "Looking for existing caches...";
        loading_reusable_count = i;
        loading_reusable_max = items.count;

        if (!win32_PathExists(items[i].thumbpath)) {
            item_indices_without_thumbs.add(i);
            DEBUGPRINT("this doesn't exist? %s\n", items[i].thumbpath.ToUTF8Reusable());
        }
        // if (!win32_PathExists(items[i].metadatapath)) {
        //     item_indices_without_metadata.add(i);
        //     DEBUGPRINT("this doesn't exist? %s\n", items[i].metadatapath.ToUTF8Reusable());
        // }
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
        newstring justname = items[item_indices_without_thumbs[i]].justname;
        if (ffmpeg_can_open(fullpath)) {
            DEBUGPRINT("creating media thumb for %s\n", fullpath.ToUTF8Reusable());
            DownresFileAtPathToPath(fullpath, thumbpath);
        } else  {
            DEBUGPRINT("creating dummy thumb for %s\n", fullpath.ToUTF8Reusable());
            CreateDummyThumb(fullpath, thumbpath, justname);
        }
        item_indices_without_thumbs.count--; // should add .pop()
    }
    item_indices_without_thumbs.free_pool();

    // // create cached metadata files
    // // do metadata second because it uses thumbnails as a fallback
    // for (int i = item_indices_without_metadata.count-1; i >= 0; i--) {
    //     loading_status_msg = "Creating metadata caches...";
    //     loading_reusable_count = i;
    //     loading_reusable_max = item_indices_without_metadata.count;

    //     string fullpath = items[item_indices_without_metadata[i]].fullpath;
    //     string thumbpath = items[item_indices_without_metadata[i]].thumbpath;
    //     string metadatapath = items[item_indices_without_metadata[i]].metadatapath;
    //     DEBUGPRINT("creating metadata file for %s\n", fullpath.ToUTF8Reusable());
    //     CreateCachedMetadataFile(fullpath, thumbpath, metadatapath);
    //     item_indices_without_metadata.count--; // should add .pop()
    // }


    // init tiles
    {
        tiles = tile_pool::new_empty();
        for (int i = 0; i < items.count; i++) {
            loading_status_msg = "Reading file paths...";
            loading_reusable_count = i;
            loading_reusable_max = items.count;

            tiles.add(CreateTileFromItem(items[i])); // note: no longer reads timestamps
        }
        // AddRandomColorToTilePool
        {
            for (int i = 0; i < tiles.count; i++) {
                tiles[i].rand_color = rand_col();
                // u32 col = rand() & 0xff;
                // col |= (rand() & 0xff) << 8;
                // col |= (rand() & 0xff) << 16;
                // col |= (rand() & 0xff) << 24;
                // tiles[i].rand_color = col;//rand();
            }
        }

        // read cached resolutions and tags from single master file
        {

            loading_status_msg = "Reading master metadata file...";
            loading_reusable_max = items.count;
            loading_reusable_count = 0;

            // first read list of master tags
            tag_list = ReadTagListFromFileOrSomethingUsableOtherwise(master_path);

            // also create resolution list, etc, to match length of items
            InitModifiedTimes(items.count);
            InitResolutionsListToMatchItemList(items.count);

            LoadMasterDataFileAndPopulateResolutionsAndTagsEtc(
                                                            // &items,
                                                            // &item_resolutions,
                                                            // &item_resolutions_valid,
                                                            &loading_reusable_count);
        }


        int items_found_in_cache = 0;
        int resolutions_read = 0;
        int tags_read = 0;
        for (int i = 0; i < items.count; i++) {
            if (items[i].found_in_cache) items_found_in_cache++;
            if (item_resolutions_valid[i]) resolutions_read++;
            if (item_tags[i].count > 0) tags_read += item_tags[i].count;
        }
        DEBUGPRINT("--------");
        DEBUGPRINT("actual items found %i\n", items.count);
        DEBUGPRINT("items_found_in_cache %i\n", items_found_in_cache);
        DEBUGPRINT("resolutions_read %i\n", resolutions_read);
        DEBUGPRINT("tags_read %i\n", tags_read);
        DEBUGPRINT("--------");


        // fill in resolutions for any items not in the cache
        {
            loading_status_msg = "Reading missing resolutions...";

            // this is entirely to just get a count of how many we need to do
            int_pool unset_resolution_indices = int_pool::new_empty();
            for (int i = 0; i < item_resolutions_valid.count; i++) {
                if (!item_resolutions_valid[i]) {
                    unset_resolution_indices.add(i);
                }
            }

            loading_reusable_max = unset_resolution_indices.count;
            for (int i = 0; i < unset_resolution_indices.count; i++) {
                loading_reusable_count = i;
                int item_index = unset_resolution_indices[i];
                item_resolutions[item_index] = ReadResolutionFromFile(items[item_index]);
            }

            unset_resolution_indices.free_pool();

            // method without needing to pre-figure which we need (wrong max progress)
            // loading_reusable_max = items.count;
            // for (int i = 0; i < items.count; i++) {
            //     loading_reusable_count = i;
            //     if (!item_resolutions_valid[i]) {
            //         v2 res = ReadResolutionFromFile(items[i]);
            //         item_resolutions[i] = res;
            //     }
            // }
        }


        // fill in modified times for any items not in the cache
        {
            loading_status_msg = "Reading missing modified times...";

            // this is entirely to just get a count of how many we need to do
            int_pool unset_times_indices = int_pool::new_empty();
            for (int i = 0; i < modifiedTimes.count; i++) {
                if (modifiedTimes[i] == TIME_NOT_SET) {
                    unset_times_indices.add(i);
                }
            }

            loading_reusable_max = modifiedTimes.count;
            for (int i = 0; i < unset_times_indices.count; i++) {
                loading_reusable_count = i;
                int item_index = unset_times_indices[i];
                modifiedTimes[item_index] = ReadModifedTimeFromFile(items[item_index]);
            }

            unset_times_indices.free_pool();

            // method without needing to pre-figure which we need (wrong max progress)
            // loading_reusable_max = items.count;
            // for (int i = 0; i < items.count; i++) {
            //     loading_reusable_count = i;
            //     if (!item_resolutions_valid[i]) {
            //         v2 res = ReadResolutionFromFile(items[i]);
            //         item_resolutions[i] = res;
            //     }
            // }
        }

        // fill in tags for any items that didn't have tags cached
        // basically just using directory name
        // maybe set as untagged instead?
        {
            loading_status_msg = "Adding tags to items without any...";
            loading_reusable_max = items.count;

            for (int i = 0; i < items.count; i++) {
                loading_reusable_count = i;
                if (item_tags[i].count == 0) {
                    if (PopulateTagFromPathsForItem(items[i], i)) {
                        // DEBUGPRINT("read tag from directory for %s\n", tiles[i].name.ToUTF8Reusable());
                    } else {
                        // DEBUGPRINT("unable to read tag from directory for %s\n", tiles[i].name.ToUTF8Reusable());
                        assert(false);
                    }
                }
            }

            SaveTagList();
        }

        // // read cached resolutions...
        // {
        //     loading_status_msg = "Reading resolutions from cached metadata...";
        //     loading_reusable_max = items.count;
        //     PopulateResolutionsFromSeparateFileCaches(&items, &loading_reusable_count);
        // }

        // // read cached tags (items and master list)
        // {
        //     loading_status_msg = "Reading item tags...";

        //     // first read list of master tags
        //     tag_list = ReadTagListFromFileOrSomethingUsableOtherwise(master_path);

        //     for (int i = 0; i < items.count; i++) {
        //         loading_reusable_count = i;
        //         loading_reusable_max = items.count;

        //         if (PopulateTagsFromCachedFileIfPossible(&items[i])) {
        //             DEBUGPRINT("cached tags read for %s\n", tiles[i].name.ToUTF8Reusable());
        //         } else {
        //             // fallback to directory orig file is in
        //             if (PopulateTagFromPath(&items[i])) {
        //                 // DEBUGPRINT("read tag from directory for %s\n", tiles[i].name.ToUTF8Reusable());
        //             } else {
        //                 DEBUGPRINT("unable to read tag from directory for %s\n", tiles[i].name.ToUTF8Reusable());
        //                 assert(false);
        //             }
        //         }
        //     }

        //     SaveTagList();
        // }


    }

    loading_status_msg = "Done loading!";

    app_change_mode(INIT);

    background_startup_thread_launched = false;
    return 0;
}
void LaunchBackgroundStartupLoopIfNeeded() {
    assert(!background_startup_thread_launched); // for now check if we're trying to launch multiple
    if (!background_startup_thread_launched) {
        CreateThread(0, 0, RunBackgroundStartupThread, 0, 0, 0);
        background_startup_thread_launched = true;
    }
}


DWORD WINAPI RunBackgroundLoadingThread( LPVOID lpParam ) {
    while (running) {
        if (app_mode == BROWSING_THUMBS) {
            for (int i = 0; i < tiles.count; i++) {
                if (tiles[i].needs_loading) {
                    if (!tiles[i].is_media_loaded) {
                        tiles[i].LoadMedia(items[i].thumbpath);
                        // DEBUGPRINT("loading: %s\n", tiles[i].thumbpath.ToUTF8Reusable());
                    }
                }
            }
        } else if (app_mode == VIEWING_FILE) {
            if (viewing_tile.needs_loading) {
                if (!viewing_tile.is_media_loaded) {
                    if (ffmpeg_can_open(items[viewing_file_index].fullpath)) {
                        viewing_tile.LoadMedia(items[viewing_file_index].fullpath);
                    } else {
                        // fallback to thumb for non-media files
                        // todo: without this branch, ffmpeg seems to actually open and display txt files?
                        // confused, can we somehow take out or special txt file handling in ffmpeg + thumbnails?
                        viewing_tile.LoadMedia(items[viewing_file_index].thumbpath);
                    }
                    // DEBUGPRINT("loading: %s\n", tiles[i].thumbpath.ToUTF8Reusable());
                }
            }
        } else if (app_mode == SETTINGS) {
            if (proposed_path_reevaluate && proposed_items_checkout==0) { // also wait until we have proposed items access
                proposed_path_reevaluate = false;

                proposed_items_checkout = 2; // flag to not change proposed_items in other thread
                item_pool bg_item_copy = copy_item_pool(proposed_items); // could be big! todo: better way to do this?
                proposed_items_checkout = 0;

                loading_status_msg = "Looking for existing thumbnails...";
                proposed_thumbs_found = int_pool::new_empty();
                for (int i = 0; i < bg_item_copy.count; i++) {
                    // loading_reusable_count = i;
                    // loading_reusable_max = bg_item_copy.count;

                    if (win32_PathExists(bg_item_copy[i].thumbpath)) {
                        proposed_thumbs_found.add(i);
                    }
                }
                free_all_item_pool_memory(&bg_item_copy);
            } else {
                loading_status_msg = "Waiting...";
            }
        }
        // static bool first_loop = true;
        // if (first_loop) {
        //     OutputDebugString("done loading\n");
        // }
        // first_loop = false;
        Sleep(16); // todo: hmm
    }
    return 0;
}
bool background_loading_thread_launched = false;
void LaunchBackgroundLoadingLoopIfNeeded() {
    if (!background_loading_thread_launched) {
        CreateThread(0, 0, RunBackgroundLoadingThread, 0, 0, 0);
        background_loading_thread_launched = true;
    }
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
bool background_unloading_thread_launched = false;
void LaunchBackgroundUnloadingLoopIfNeeded() {
    if (!background_unloading_thread_launched) {
        CreateThread(0, 0, RunBackgroundUnloadingThread, 0, 0, 0);
        background_unloading_thread_launched = true;
    }
}

