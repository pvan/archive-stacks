

//
// global app data
// for use by multiple threads, etc



// todo: capitalize these naames
const string archive_save_filename = string::create_using_passed_in_memory(L"~meta.txt");
const string archive_tag_list_filename = string::create_using_passed_in_memory(L"~taglist.txt");
const string thumb_dir_name = string::create_using_passed_in_memory(L"~thumbs");



string master_path;  // what project / collection directory we have currently open


bool show_debug_console = false;



//
// debug vars


float metric_max_dt;
float metric_time_to_first_frame;



//
// for viewing individual files

u64 viewing_file_index = 0; // what file do we have open if we're in VIEWING_FILE mode

tile viewing_tile; // tile for our open file (created from fullpath rather than thumbpath like the "browsing" tiles are)



//
// for browsing thumbs mode


int master_scroll_delta = 0;
int master_ctrl_scroll_delta = 0;

float master_desired_tile_width = 200;



//
// for finding items.. todo: better home for this?

string_pool FindAllItemPaths(string master_path) {
    string_pool top_folders = win32_GetAllFilesAndFoldersInDir(master_path);
    string_pool result = string_pool::new_empty();

    for (int folderI = 0; folderI < top_folders.count; folderI++) {
        if (win32_IsDirectory(top_folders[folderI])) {
            if (top_folders[folderI].ends_with(thumb_dir_name)
                // || top_folders[folderI].ends_with(metadata_dir_name)
            ) {
                // DEBUGPRINT("Ignoring: %s\n", top_folders[folderI].to_utf8_reusable());
            } else {
                string_pool subfiles = win32_GetAllFilesAndFoldersInDir(top_folders[folderI]);
                for (int fileI = 0; fileI < subfiles.count; fileI++) {
                    if (!win32_IsDirectory(subfiles[fileI])) {
                        result.add(subfiles[fileI]);
                        // if (result.count > 1630) {
                        //     result.count++;
                        //     result.count--;
                        // }
                    } else {
                        subfiles[fileI].free_all();
                        // wchar_t tempbuf[123];
                        // swprintf(tempbuf, L"%s is dir!\n", subfiles[fileI].chars);
                        // // OutputDebugStringW(tempbuf);
                        // MessageBoxW(0,tempbuf,0,0);
                    }
                }
                subfiles.free_pool();
                // deep_free_string_pool(subfiles);
            }
        } else {
            // files in top folder, add?
            // todo: see xgz
        }
    }
    deep_free_string_pool(top_folders);
    return result;
}

string_pool FindAllSubfolderPaths(string master_path, wc *subfolder) {

    string subfolder_path = master_path.copy_and_append(subfolder);
    string_pool top_files = win32_GetAllFilesAndFoldersInDir(subfolder_path);
    string_pool result = string_pool::new_empty();
    for (int folderI = 0; folderI < top_files.count; folderI++) {
        if (win32_IsDirectory(top_files[folderI])) {
            string_pool subfiles = win32_GetAllFilesAndFoldersInDir(top_files[folderI]);
            for (int fileI = 0; fileI < subfiles.count; fileI++) {
                if (!win32_IsDirectory(subfiles[fileI])) {
                    // DEBUGPRINT(subfiles[fileI].ParentDirectoryNameReusable().ToUTF8Reusable());
                    result.add(subfiles[fileI]);
                } else {
                    subfiles[fileI].free_all();
                    // wchar_t tempbuf[123];
                    // swprintf(tempbuf, L"%s is dir!\n", subfiles[fileI].chars);
                    // // OutputDebugStringW(tempbuf);
                    // MessageBoxW(0,tempbuf,0,0);
                }
            }
            // deep_free_string_pool(subfiles);
            subfiles.free_pool();
        } else {
            // files in top folder, add?
            // todo: see xgz
        }
    }
    deep_free_string_pool(top_files);
    return result;
}

string_pool ItemsInFirstPoolButNotSecond(string_pool p1, string_pool p2) {
    string_pool result = string_pool::new_empty();
    for (int i = 0; i < p1.count; i++) {
        if (!p2.has(p1[i]))
            result.add(p1[i]);
    }
    return result;
}



//
// tags

// todo: should be list not pool
// master tag list
// each item has list of indices into this array
string_pool tag_list;

void SaveTagList() {
    wc *path = CombinePathsIntoNewMemory(master_path, archive_tag_list_filename).to_wc_final();

    // todo: what is proper saving protocol: save as copy then replace old when done?

    // create new blank file first
    if (tag_list.count > 0)
        Win32WriteBytesToFileW(path, tag_list[0].to_utf8_reusable(), strlen(tag_list[0].to_utf8_reusable()) + 1); // include the null-terminator

    // then append each
    for (int i = 1; i < tag_list.count; i++) {
        Win32AppendBytesToFileW(path, tag_list[i].to_utf8_reusable(), strlen(tag_list[i].to_utf8_reusable()) + 1); // include the null-terminator
    }

    free(path);

    // for (int i = 0 ; i < tag_list.count; i++) {
    //     DEBUGPRINT(tag_list[i].ToUTF8Reusable());
    // }
    // DEBUGPRINT("saved %i\n", tag_list.count);
}
void AddNewTag(string tag) {
    assert(!tag_list.has(tag));
    tag_list.add(tag);
}
void AddNewTagAndSave(string tag) {
    AddNewTag(tag);
    SaveTagList();
}


string_pool ReadTagListFromFileOrSomethingUsableOtherwise(string master_path) {

    // if (!win32_PathExists(path.chars)) return;
    // // wchar version
    // FILE *file = _wfopen(path.chars, L"rb");
    // if (!file) { DEBUGPRINT("error reading %s\n", path.ToUTF8Reusable()); return; }
    // string
    // while (fwscanf(file, L"", ) {
    //     tag_list
    // }
    // fwscanf(file, L"%f,%f", &result->x, &result->y);
    // fclose(file);


    string_pool result = string_pool::new_empty();
    // result.add(string::Create(L"untagged"));  // always have this entry as index 0?? todo: decide

    wc *path = CombinePathsIntoNewMemory(master_path, archive_tag_list_filename).to_wc_final();

    if (!win32_PathExists(path)) {
        result.add(string::create_with_new_memory(L"untagged"));  // always have this entry as index 0?? todo: decide
        return result;
    }

    void *fileMem;
    int fileSize;
    Win32ReadFileBytesIntoNewMemoryW(path, &fileMem, &fileSize);

    free(path);

    char *fileChars = (char*)fileMem;
    int fileCharCount = fileSize / sizeof(char); // the same, but just to be sure

    int chars_written = 0;
    while (chars_written < fileCharCount) {
        char *thisTag = fileChars + chars_written;
        string thisTagString = string::create_with_new_memory(thisTag);
        if (!result.has(thisTagString)) {
            result.add(thisTagString);
            // DEBUGPRINT("added %s\n", thisTagString.ToUTF8Reusable());
        } else {
            DEBUGPRINT("WARNING: %s was in taglist more than once!\n", thisTagString.to_utf8_reusable());
        }
        // todo: prickly use-after-free bug potential in string_pool, should check all string_pool and just refactor pools in general
        // free(thisTagString.chars); // don't free! string_pool will use this address
        chars_written += (strlen(thisTag) + 1); // skip ahead to the next, remember to skip past the \0 as well
    }

    return result;
}


//
// indices into the tag_list (todo: make tag_list into an official list instead of "pool" or not too important as long as we never remove an item?)
//
// another way would be to store the item index in an int list for each tag
// (harder to find an item's tags that way, but easier to free and to find all items with a tag)
//
// a list (in same order as items -- they can share indices)
// of a list of tag indices (indicies into the tag_list)
// so it's kind of like this:
// [0] 0,1,2 ...
// [1] 2 ...
// ...
// where ... could be expanded at any time
// and the a,b,c numbers are indices into tag_list
// and [a] [b] [c] numbers are indices into the master item list (and the item_tags list)
DEFINE_TYPE_POOL(int_pool);
int_pool_pool item_tags;


//
// items (collection of paths basically)

struct item {
    string fullpath;
    string thumbpath;
    // string thumbpath128; // like this?
    // string thumbpath256;
    // string thumbpath512;
    string subpath;
    string justname;

    bool operator==(item o) { return fullpath==o.fullpath; } // for now just check fullpath

    bool found_in_cache = false; // just metric for debugging

    void free_all() {
        fullpath.free_all();
        thumbpath.free_all();
        subpath.free_all();
        justname.free_all();
    }

    item deep_copy() {
        item result = {0};
        result.fullpath = fullpath.copy_into_new_memory();
        result.thumbpath = thumbpath.copy_into_new_memory();
        result.subpath = subpath.copy_into_new_memory();
        result.justname = justname.copy_into_new_memory();
        return result;
    }

};

// recommend create all items with this
// should now set all paths stored in item (but not tags)
item CreateItemFromPath(string fullpath, string masterdir) {
    item newitem = {0};

    newitem.fullpath = fullpath.copy_into_new_memory();
    newitem.subpath = fullpath.copy_into_new_memory().trim_common_prefix(masterdir);

    string thumbpath = CombinePathsIntoNewMemory(masterdir, thumb_dir_name, newitem.subpath);
    // for now, special case for txt...
    // we need txt thumbs to be something other than txt so we can open them
    // with our ffmpeg code that specifically "ignores all .txt files" atm
    // but we need most thumbs to have original extensions to (for example) animate correctly
    if (fullpath.ends_with(L".txt")) {
        thumbpath.append(L".bmp");
    }
    newitem.thumbpath = thumbpath;//.copy_into_new_memory();
    // thumbpath.free_all();

    newitem.justname = newitem.subpath.copy_into_new_memory();
    trim_everything_after_last_slash(newitem.justname);

    return newitem;
}



DEFINE_TYPE_POOL(item);

item_pool copy_item_pool(item_pool its) {
    item_pool result = item_pool::new_empty();
    for (int i = 0; i < its.count; i++) {
        result.add(its[i].deep_copy());
    }
    return result;
}

void free_all_item_pool_memory(item_pool *its) {
    for (int i = 0; i < its->count; i++) {
        its->pool[i].free_all();
    }
    its->free_pool();
}


item_pool items;



item_pool CreateItemListFromMasterPath(string masterdir) {
    string_pool itempaths = FindAllItemPaths(masterdir);
    item_pool result = item_pool::new_empty();
    for (int i = 0; i < itempaths.count; i++) {
        item newitem = CreateItemFromPath(itempaths[i], masterdir);
        result.add(newitem);
    }
    deep_free_string_pool(itempaths);
    return result;
}



string laststr = string::new_empty();// = string::create_with_new_memory(L"empty");
bool PopulateTagFromPathsForItem(item it, int itemindex) {
    wc *directory = CopyJustParentDirectoryName(it.fullpath.to_wc_reusable());
    assert(directory);
    assert(directory[0]);

    string dir = string::create_using_passed_in_memory(directory);

    if (laststr != dir) {
        laststr = dir.copy_into_new_memory();
    }

    if (!tag_list.has(dir)) {
        AddNewTag(dir);
    }
    int index = tag_list.index_of(dir);
    if (index == -1) return false;
    item_tags[itemindex].add(index);

    free(directory);

    return true;
}

//
// other item data
// keeping as separate lists for now
// index into master item list (items) should match each of these
// eg modifiedTimes[i] and items[i] have data for the same item / file

//
// modified times

DEFINE_TYPE_POOL(u64);

// doing "uninitialized" this way for times, see resolutions for another approach
const u64 TIME_NOT_SET = 0;

u64_pool modifiedTimes; // time since last epoc

void InitModifiedTimes(int count) {
    for (int i = 0; i < count; i++) {
        modifiedTimes.add(TIME_NOT_SET);
    }
}

u64 ReadModifedTimeFromFile(item it) {
    return win32_GetModifiedTimeSinceEpoc(it.fullpath);
}

//
// resolutions

v2_pool item_resolutions;
bool_pool item_resolutions_valid; // could use -1,-1 or similar as code for this, but i like makign it explicit for now

void InitResolutionsListToMatchItemList(int count) {
    for (int i = 0; i < count; i++) {
        item_resolutions.add({0,0});
        item_resolutions_valid.add(false);
    }
}

// pull resolution from separate metadata file (unique one for each item)
bool GetCachedResolutionIfPossible(string path, v2 *result) {
    FILE *file = _wfopen(path.to_wc_reusable(), L"r");
    if (!file) {  DEBUGPRINT("error reading %s\n", path.to_utf8_reusable()); return false; }
    int x, y;
    fwscanf(file, L"%i,%i", &x, &y);
    result->x = x;
    result->y = y;
    fclose(file);
    return true;
}
// create separate resolution metadata file (unique one for each item)
void CreateCachedResolution(string path, v2 size) {
    win32_CreateAllDirectoriesForPathIfNeeded(path.to_wc_reusable());
    FILE *file = _wfopen(path.to_wc_reusable(), L"w");
    if (!file) { DEBUGPRINT("error creating %s\n", path.to_utf8_reusable()); return; }
    fwprintf(file, L"%i,%i", (int)size.x, (int)size.y); // todo: round up? (should all be square ints anyway, though)
    fclose(file);
}



// used to fallback to thumbnail, still do that? maybe even start with that? (faster?)
v2 ReadResolutionFromFile(item it) {
    v2 res = {0,0};
    if (ffmpeg_can_open(it.fullpath)) {
        res = ffmpeg_GetResolution(it.fullpath);
    }
    // fallback to using thumbnail resolution
    // this is for case where the item is not an image (eg txt)
    // so the thumbnail is soem placeholder image like image of the filename
    // todo: change this in the future?
    else if (ffmpeg_can_open(it.thumbpath)) {
        res = ffmpeg_GetResolution(it.thumbpath);
    }
    // if can't get resoution from either, hmm...
    // maybe the thumbnail doesn't exist yet?
    // (we usually are / should be running this after creating all needed thumbs though)
    else {
        res = {12, 12};
        assert(false); // for now, see if trigger
    }
    return res;
}



//
////



//
// new method that saves all metadata into one file
//

void SaveMetadataFile()
{
    wc *path = CombinePathsIntoNewMemory(master_path, archive_save_filename).to_wc_final();

    FILE *file = _wfopen(path, L"w, ccs=UTF-16LE");
    if (!file) {
        DEBUGPRINT("couldn't open master metadata file for writing");
        return;
    }
    for (int i = 0; i < items.count; i++) {
        assert(item_resolutions[i].x == (int)item_resolutions[i].x); // for now i just want to know if any resolutions are't whole numbers
        assert(item_resolutions[i].y == (int)item_resolutions[i].y);

        fwprintf(file, L"%i,%i,", (int)item_resolutions[i].x, (int)item_resolutions[i].y);
        fwprintf(file, L"%llu,", modifiedTimes[i]);
        fwprintf(file, L"%s", items[i].subpath.to_wc_reusable());
        fwprintf(file, L"%c", L'\0'); // note we add our own \0 after string for easier parsing later
        for (int j = 0; j < item_tags[i].count; j++) {
            fwprintf(file, L" %i", item_tags[i][j]);
        }
        fwprintf(file, L"\n");
    }

    free(path);
}



void LoadMasterDataFileAndPopulateResolutionsAndTagsEtc(
                                                     // item_pool *itemslocal,
                                                     // v2_pool *resolutions,
                                                     // bool_pool *res_are_valid,
                                                     int *progress)
{
    wc *path = CombinePathsIntoNewMemory(master_path, archive_save_filename).to_wc_final();
    if (!win32_PathExists(path)) {
        DEBUGPRINT("master metadata cache file doesn't exist");
        return;
    }

    FILE *file = _wfopen(path, L"r, ccs=UTF-16LE");
    if (!file) {
        DEBUGPRINT("couldn't open master metadata file for reading"); // could not exist, checking exists above now
        return;
    }
    int numread;
    int linesread = 0;
    while (true) {

        // read resolution and save for later
        int x, y;
        numread = fwscanf(file, L"%i,%i,", &x, &y);
        if (numread < 2/*note 2 here*/) {DEBUGPRINT("e1 %i\n",linesread);goto fileclose;} // eof (or maybe badly formed file?)

        // read modified time
        u64 time;
        numread = fwscanf(file, L"%llu,", &time);
        if (numread < 0) {DEBUGPRINT("e2 %i\n",linesread);goto fileclose;} // eof (or maybe badly formed file?)

        // read subpath
        string subpath = string::allocate_new(256);
        wc nextchar = L'a'; // just some non-0 initialization (overwritten before used)
        while (nextchar != L'\0') { // note we exit below so we could probably just use while(1) here
            numread = fwscanf(file, L"%c", &nextchar);
            if (numread < 0) {DEBUGPRINT("e3 %i\n",linesread);subpath.free_all();goto fileclose;} // eof (or maybe badly formed file?)
            if (nextchar == L'\0') break; // don't add the actual null terminator (new string class isn't null terminated)
            subpath.add(nextchar);
        }
        // note we want to exit loop without null terminator attached to our string subpath
        // so we can get a 1-1 comparison with our items[i].subpath
        // todo: maybe ignore null terminators in all string checks with new string class? or better to assert them somewhere

        // use subpath to find item index
        int this_item_i = -1;
        for (int i = 0; i < items.count; i++) {
            // wc *justsubpath = items[i].PointerToJustSubpath(master_path);
            // if (wc_equals(items[i].subpath.to_wc_reusable(), subpath))
            if (PathsAreSame(items[i].subpath, subpath))
            {
                this_item_i = i;
                break;
            }
        }
        if (this_item_i == -1) {
            DEBUGPRINT("ERROR: subpath in metadata list not found in item list: %s\n", subpath.to_utf8_reusable());
            assert(false);
        }
        subpath.free_all();
        (*progress)++;

        // todo: could put this with its parsing code if we put subpath first in the file
        item_resolutions[this_item_i] = {(float)x, (float)y};
        item_resolutions_valid[this_item_i] = true;

        modifiedTimes[this_item_i] = time;

        items[this_item_i].found_in_cache = true;

        // int nextint;
        // numread = fwscanf(file, L"%i \n", &nextint);
        // if (numread < 0) {DEBUGPRINT("33");goto fileclose;} // eof (or maybe badly formed file?)

        // numread = fwscanf(file, L"%c", &nextchar);
        // if (numread < 0) {DEBUGPRINT("3");goto fileclose;} // eof (or maybe badly formed file?)

        // if (numread != L'\n') {
        //     int nextint;
        //     numread = fwscanf(file, L"%i ", &nextint);
        //     items[this_item_i].tags.add(nextint);
        // }
        // numread = fwscanf(file, L"\n");
        // if (numread < 0) {DEBUGPRINT("4");goto fileclose;} // eof (or maybe badly formed file?)

        while (true) {
            numread = fwscanf(file, L"%c", &nextchar);
            if (numread < 0) {DEBUGPRINT("e4 %i\n",linesread);goto fileclose;} // eof (or maybe badly formed file?)
            if (nextchar == L'\n')
                break; // done ready tag ints

            int nextint;
            numread = fwscanf(file, L"%i", &nextint);
            if (numread < 0) {DEBUGPRINT("e5 %i\n",linesread);goto fileclose;} // eof (or maybe badly formed file?)
            item_tags[this_item_i].add(nextint);

        }
        linesread++;

    }

    fileclose:
    fclose(file);
    free(path);

}





//
// what tags are visible (filtered) and selected (colored)


string browse_tag_filter = string::allocate_new(128);
string view_tag_filter = string::allocate_new(128);


int_pool filtered_browse_tag_indices = int_pool::new_empty();

int_pool filtered_view_tag_indices = int_pool::new_empty();


// master list of tag indices that are selected for browsing
int_pool browse_tag_indices;


void SelectAllBrowseTags() {
    if (filtered_browse_tag_indices.count == 0) {
        // set all tags as selected
        browse_tag_indices.empty_out();
        for (int i = 0; i < tag_list.count; i++) {
            browse_tag_indices.add(i);
        }
    }
    else {
        // now just affect visible (filtered) tags
        for (int ft = 0; ft < filtered_browse_tag_indices.count; ft++) {
            int tagi = filtered_browse_tag_indices[ft];
            if (!browse_tag_indices.has(tagi))
                browse_tag_indices.add(tagi);
        }
    }
}

void DeselectAllBrowseTags() {
    if (filtered_browse_tag_indices.count == 0) {
        browse_tag_indices.empty_out();
    }
    else {
        // now just affect visible (filtered) tags
        for (int ft = 0; ft < filtered_browse_tag_indices.count; ft++) {
            int tagi = filtered_browse_tag_indices[ft];
            if (browse_tag_indices.has(tagi))
                browse_tag_indices.remove(tagi);
        }
    }
}




// master list of item indices for items currently selected for display
// ordered in the display order we want
int_pool display_list;

void CreateDisplayListFromBrowseSelection() {
    display_list.empty_out();
    // setup display list
    for (int i = 0; i < items.count; i++) {
        // could iterate through item's tags or browse tags here first
        for (int t = 0; t < item_tags[i].count; t++) {
            if (browse_tag_indices.has(item_tags[i][t])) {
                display_list.add(i);
                break; // next item
            }
        }
    }
}






//
// data for settings tab, proposed new master directory stuff

// all here?
// todo: store here or with similar (like put proposed_path next to master_path?)

string proposed_master_path;      // string in the settings textbox
string last_proposed_master_path; // string in the settings textbox last frame (for tracking changes -- change will trigger msg to bg thread)
string proposed_path_msg;  // string used to pass proposed path to background thread (owned by main thread and changed when textbox path changes, bg just makes copy)
string bg_path_copy;   // actual string background uses (makes copy of proposed_path_msg)

bool proposed_path_reevaluate = false; // trigger for our background thread to start analysing new path
bool proposed_path_msg_available = true; // can the main thread overwrite proposed_path_msg? or is bg thread making a copy of it right now?

// lists filled in by bg thread and displayed in settings menu
item_pool proposed_items;
int_pool proposed_thumbs_found = int_pool::new_empty();





//
// app mode (what screen we're on) related stuff


// call whenever proposed path changes in settings menu
// will trigger messages for bg thread to update proposed_items, thumbs, etc
void TriggerSettingsPathChange(string newpath) {
    while(!proposed_path_msg_available); // wait for bg to finish copying msg
    proposed_path_msg.overwrite_with_copy_of(newpath);
    proposed_path_reevaluate = true;
}




// could add loading/etc to this framework
const int BROWSING_THUMBS = 0;
const int VIEWING_FILE = 1;
const int SETTINGS = 2;
const int LOADING = 3;
const int INIT = 4;
int app_mode = LOADING;

void app_change_mode(int new_mode) {

    // leaving VIEW
    if (app_mode == VIEWING_FILE && new_mode != VIEWING_FILE) {
        // todo: remove tile stuff from memory or gpu ?
        viewing_tile.UnloadMedia();
    }

    // leaving SETTINGS
    if (app_mode == SETTINGS && new_mode != SETTINGS) {

    }

    // leaving LOADING
    if (app_mode == LOADING && new_mode != LOADING) {
        assert(new_mode == INIT);
    }

    // entering SETTINGS
    if (new_mode == SETTINGS) {
        proposed_master_path.overwrite_with_copy_of(master_path);
        TriggerSettingsPathChange(proposed_master_path);
    }

    // entering LOADING (creating thumbnail files, etc)
    if (new_mode == LOADING) {

    }

    app_mode = new_mode;
}


void LaunchBackgroundStartupLoopIfNeeded();

void SelectNewMasterDirectory(string newdir) {
    // basically need to undo everything we create when loading
    // see, at the very least, backgroundstartupthread and init_app()

    master_path.overwrite_with_copy_of(newdir);
    // todo: should probably do something like this:
    // master_path.find_replace(L'/', L'\\');
    // if (master_path.ends_with(L"\\")) master_path.rtrim(1);

    // or should we put all this stuff in app_change_mode, or even call this from there?
    app_change_mode(LOADING); // this will switch our background threads to idle (atm at least)

    // most of the loading will be done here, including setup/unloading previous
    LaunchBackgroundStartupLoopIfNeeded();


}







//
// tile functions

tile CreateTileFromFile(string path) {
    tile newTile = {0};
    assert(win32_PathExists(path));
    assert(!win32_IsDirectory(path));
    // newTile.name = string::CreateWithNewMem(CopyJustFilename(path.chars));
    return newTile;
};

tile CreateTileFromItem(item it) {
    tile newTile = CreateTileFromFile(it.fullpath);
    // newTile.paths_just_for_comparing_tiles = it;
    return newTile;
}



// although our pools aren't intended to have any guaranteed static order, we put one in order here anyway
void SortTilePoolByDate(tile_pool *pool)
{
    for (int d = 0; d < pool->count-1; d++) {
        int anyswap = false;

        for (int j = 0; j < pool->count-1; j++) {
            // if (pool->pool[j].modifiedTimeSinceLastEpoc > pool->pool[j+1].modifiedTimeSinceLastEpoc) {
            if (modifiedTimes[j] > modifiedTimes[j+1]) {

                // swap
                tile temp = pool->pool[j];
                pool->pool[j] = pool->pool[j+1];
                pool->pool[j+1] = temp;

                anyswap = true;
            }
        }
        if (!anyswap) break;
    }
}



// position tiles selected in display list
// tiles not selected... do what with them? set flag to ignore when rendering?
int ArrangeTilesForDisplayList(int_pool displaylist, tile_pool *tiles, float desired_tile_width, int dest_width) {

    desired_tile_width = fmax(desired_tile_width, MIN_TILE_WIDTH);
    float realWidth;
    int cols = CalcColumnsNeededForDesiredWidth(desired_tile_width, dest_width, &realWidth);

    float *colbottoms = (float*)malloc(cols*sizeof(float));
    for (int c = 0; c < cols; c++)
        colbottoms[c] = 0;

    // do something like this?
    // for (int i = 0; i < tiles->count; i++) {
    //     tiles->pool[i].pos={-1000,-1000};
    //     tiles->pool[i].size={0,0};
    // }

    // for (int j = 0; j < stubRectCount; j++) {
    for (int i = 0; i < tiles->count; i++) {

        if (!displaylist.has(i)) {
            // make sure we force unselected tiles to not render
            tiles->pool[i].skip_rendering = true;
            continue; // next tile
        }
        tiles->pool[i].skip_rendering = false;

        tile& thisTile = tiles->pool[i];
        v2& thisRes = item_resolutions[i];

        // todo: should be set to min of 10 from trying to find resolution originally
        // assert(thisTile.size.w >= 10 && thisTile.size.h >= 10);
        if (thisRes.w < 10) thisRes.w = 10;
        if (thisRes.h < 10) thisRes.h = 10;

        float aspect_ratio = thisRes.w / thisRes.h;

        thisTile.size.w = realWidth;
        thisTile.size.h = realWidth / aspect_ratio;

        // if (thisTile.size.w > 3000 || thisTile.size.h > 3000) {
        //     assert(false);
        // }

        int shortestCol = 0;
        for (int c = 1; c < cols; c++)
            if (colbottoms[c] < colbottoms[shortestCol])
                shortestCol = c;

        float x = shortestCol * realWidth;
        float y = colbottoms[shortestCol];

        thisTile.pos = {x, y};

        colbottoms[shortestCol] += thisTile.size.h;
    }



    float total_height;
    total_height = 0;
    int longestCol = 0;
    for (int c = 1; c < cols; c++)
        if (colbottoms[c] > colbottoms[longestCol])
            longestCol = c;
    total_height = colbottoms[longestCol];

    free(colbottoms);


    return total_height;
}




