

//
// global app data
// for use by multiple threads, etc



bool loading = true;
bool need_init = true;

// could add loading/etc to this framework
const int BROWSING_THUMBS = 0;
const int VIEWING_FILE = 1;
const int SETTINGS = 2;
int app_mode = BROWSING_THUMBS;

u64 viewing_file_index = 0; // what file do we have open if we're in VIEWING_FILE mode

newstring master_path;
newstring proposed_master_path;// = newstring::allocate_new(256);


void app_change_mode(int new_mode) {

    // leaving VIEW
    if (app_mode == VIEWING_FILE && new_mode != VIEWING_FILE) {
        // todo: remove tile stuff from memory or gpu ?
    }

    // leaving SETTINGS
    if (app_mode == SETTINGS && new_mode != SETTINGS) {

    }

    // entering SETTINGS
    if (new_mode == SETTINGS) {
        proposed_master_path.overwrite_with_copy_of(master_path);

    }

    app_mode = new_mode;

}


// paths...

// string_pool files_full; // string collection of paths to all item files
// string_pool files_thumb; // string collection of paths to the thumbnail files
// // string_pool files_thumb128; // string collection of paths to the thumbnail files
// // string_pool files_thumb256; // string collection of paths to the thumbnail files
// // string_pool files_thumb512; // string collection of paths to the thumbnail files
// string_pool files_metadata; // paths to metadata files (resolution, probably tags eventually)


// // TODO: rename, these should be "thumbpaths_that_need_creation" or something
// // because they are not item paths but the paths to the files that don't exist yet
// // kind of just for loading....
// string_pool existing_thumbs;
// string_pool items_without_matching_thumbs;
// string_pool thumbs_without_matching_item;

// string_pool existing_metadata;
// string_pool items_without_matching_metadata;
// string_pool metadata_without_matching_item;


newstring thumb_dir_name = newstring::create_with_new_memory(L"~thumbs");  // todo: where should these consts go
newstring metadata_dir_name = newstring::create_with_new_memory(L"~metadata");

newstring_pool FindAllItemPaths(newstring master_path) {
    newstring_pool top_folders = win32_GetAllFilesAndFoldersInDir(master_path);
    newstring_pool result = newstring_pool::new_empty();

    for (int folderI = 0; folderI < top_folders.count; folderI++) {
        if (win32_IsDirectory(top_folders[folderI])) {
            if (top_folders[folderI].ends_with(thumb_dir_name) ||
                top_folders[folderI].ends_with(metadata_dir_name))
            {
                DEBUGPRINT("Ignoring: %s\n", top_folders[folderI].to_utf8_reusable());
            } else {
                newstring_pool subfiles = win32_GetAllFilesAndFoldersInDir(top_folders[folderI]);
                for (int fileI = 0; fileI < subfiles.count; fileI++) {
                    if (!win32_IsDirectory(subfiles[fileI])) {
                        result.add(subfiles[fileI]);
                        // if (result.count > 1630) {
                        //     result.count++;
                        //     result.count--;
                        // }
                    } else {
                        // wchar_t tempbuf[123];
                        // swprintf(tempbuf, L"%s is dir!\n", subfiles[fileI].chars);
                        // // OutputDebugStringW(tempbuf);
                        // MessageBoxW(0,tempbuf,0,0);
                    }
                }
            }
        } else {
            // files in top folder, add?
            // todo: see xgz
        }
    }
    return result;
}

newstring_pool FindAllSubfolderPaths(newstring master_path, wc *subfolder) {

    newstring subfolder_path = master_path.copy_and_append(subfolder);
    newstring_pool top_files = win32_GetAllFilesAndFoldersInDir(subfolder_path);
    newstring_pool result = newstring_pool::new_empty();
    for (int folderI = 0; folderI < top_files.count; folderI++) {
        if (win32_IsDirectory(top_files[folderI])) {
            newstring_pool subfiles = win32_GetAllFilesAndFoldersInDir(top_files[folderI]);
            for (int fileI = 0; fileI < subfiles.count; fileI++) {
                if (!win32_IsDirectory(subfiles[fileI])) {
                    // DEBUGPRINT(subfiles[fileI].ParentDirectoryNameReusable().ToUTF8Reusable());
                    result.add(subfiles[fileI]);
                } else {
                    // wchar_t tempbuf[123];
                    // swprintf(tempbuf, L"%s is dir!\n", subfiles[fileI].chars);
                    // // OutputDebugStringW(tempbuf);
                    // MessageBoxW(0,tempbuf,0,0);
                }
            }
        } else {
            // files in top folder, add?
            // todo: see xgz
        }
    }
    return result;
}

string_pool ItemsInFirstPoolButNotSecond(string_pool p1, string_pool p2) {
    string_pool result = string_pool::new_empty();
    for (int i = 0; i < p1.count; i++) {
        if (!p2.has(p1[i]))
            result.add(p1[i]);
        // string thumbpath = string::Create(CopyItemPathAndConvertToThumbPath(files[i].chars));
        // if (!thumb_files.has(thumbpath)) {
        //     items_without_matching_thumbs.add(files[i].Copy());
        //     // DEBUGPRINT("can't find %s\n", thumbpath.ToUTF8Reusable());
        // }
        // free(thumbpath.chars);
    }
    return result;
}





// todo: should be list not pool
// master tag list
// each item has list of indices into this array
string_pool tag_list;

void SaveTagList() {
    // wc *path = master_path.CopyAndAppend(L"/~taglist.txt").chars;
    wc *path = master_path.copy_into_new_memory().append(L"/~taglist.txt").to_wc_final();

    // todo: what is proper saving protocol: save as copy then replace old when done?

    // create new blank file first
    if (tag_list.count > 0)
        Win32WriteBytesToFileW(path, tag_list[0].ToUTF8Reusable(), strlen(tag_list[0].ToUTF8Reusable()) + 1); // include the null-terminator

    // then append each
    for (int i = 1; i < tag_list.count; i++) {
        Win32AppendBytesToFileW(path, tag_list[i].ToUTF8Reusable(), strlen(tag_list[i].ToUTF8Reusable()) + 1); // include the null-terminator
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


string_pool ReadTagListFromFileOrSomethingUsableOtherwise(newstring master_path) {
    //char *path = DEFAULT_PATH"~tags\\~taglist.txt";

    // string path = master_path.CopyAndAppend(L"~taglist.txt");
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

    // wc *path = master_path.CopyAndAppend(L"/~taglist.txt").chars;
    wc *path = master_path.copy_into_new_memory().append(L"/~taglist.txt").to_wc_final();

    if (!win32_PathExists(path)) {
        result.add(string::KeepMemory(L"untagged"));  // always have this entry as index 0?? todo: decide
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
        string thisTagString = string::Create(thisTag);
        if (!result.has(thisTagString)) {
            result.add(thisTagString);
            // DEBUGPRINT("added %s\n", thisTagString.ToUTF8Reusable());
        } else {
            DEBUGPRINT("WARNING: %s was in taglist more than once!\n", thisTagString.ToUTF8Reusable());
        }
        // todo: prickly use-after-free bug potential in string_pool, should check all string_pool and just refactor pools in general
        // free(thisTagString.chars); // don't free! string_pool will use this address
        chars_written += (strlen(thisTag) + 1); // skip ahead to the next, remember to skip past the \0 as well
    }

    return result;
}




struct item {
    string fullpath;
    string thumbpath;

    newstring subpath;
    // string thumbpath128;
    // string thumbpath256;
    // string thumbpath512;
    string metadatapath; //cached metadata
    bool operator==(item o) { return fullpath==o.fullpath; } // for now just check fullpath

    string justname() { wc *result = CopyJustFilename(fullpath.chars); return string::KeepMemory(result); }
    // string justsubpath() { wc *result = CopyJustFilename(fullpath.chars); return string::Create(result); }

    // wc *PointerToJustSubpath(newstring rootpath) {
    //     return PointerToFirstUniqueCharInSecondString(rootpath.chars, fullpath.chars);
    // };

    bool found_in_cache = false; // just metric for debugging

    // indices into the pool tag_list todo: should be list not pool
    // todo: separate out of item struct? (would have to be array or pool of int_pools which might be weird)
    int_pool tags;
};

// recommend create all items with this
// note doesn't set thumbpath or metadata path or tags etc atm
item CreateItemFromPath(newstring fullpath, newstring masterdir) {
    item newitem = {0};
    newitem.fullpath = fullpath.to_old_string_temp();
    newitem.subpath = fullpath.copy_into_new_memory().trim_common_prefix(masterdir);
    return newitem;
}

DEFINE_TYPE_POOL(item);

item_pool items;


item_pool CreateItemListFromMasterPath(newstring masterdir) {
    newstring_pool itempaths = FindAllItemPaths(masterdir);
    item_pool result = item_pool::new_empty();
    for (int i = 0; i < itempaths.count; i++) {
        item newitem = CreateItemFromPath(itempaths[i], masterdir);
        result.add(newitem);
    }
    return result;
}


// todo: combine with reading resolution into one read/write_metadata function
bool PopulateTagsFromCachedFileIfPossible(item *it) {

    string path = it->metadatapath;

    if (!win32_PathExists(path)) return false;

    // wchar version
    FILE *file = _wfopen(path.chars, L"rb");
    if (!file) {  DEBUGPRINT("error reading %s\n", path.ToUTF8Reusable()); return false; }
    float notused;
    // int result = fwscanf(file, L"%f,%f", &notused, &notused); // skip resolution

    while (true) {
        char ch = fgetc(file);
        if (ch == EOF) { fclose(file); return false; }
        if (ch == '\n') break; // read until first new line
    }

    // if (feof(file)) return false;
    int next_index = 0;
    // if (!feof(file)) {
        fwscanf(file, L"%i\n", &next_index);
    //     if (feof(file)) return false;
        it->tags.add(next_index);
    // }
    fclose(file);
    return true;
}

static string laststr = string::CreateWithNewMem(L"empty");
bool PopulateTagFromPath(item *it) {
    wc *directory = CopyJustParentDirectoryName(it->fullpath.chars);
    assert(directory);
    assert(directory[0]);
    // if (!directory) return false; //todo: assert these instead?
    // if (directory[0] == 0) return false;
    string dir = string::KeepMemory(directory);

    if (laststr != dir) {
        laststr = dir.Copy();
    }

    if (!tag_list.has(dir)) {
        // DEBUGPRINT("\n\nok so you're telling me tag_list doesn't have %s\n", dir.ToUTF8Reusable());
        // DEBUGPRINT("and here's the list:");
        // for (int i = 0; i < tag_list.count; i++) {
        //     DEBUGPRINT(" %s, ", tag_list[i].ToUTF8Reusable());
        // }
        AddNewTag(dir);
    }
    int index = tag_list.index_of(dir);
    if (index == -1) return false;
    it->tags.add(index);
    // if (tag_list.count != lastcount) {
    //     DEBUGPRINT("change"); }
    // lastcount = tag_list.count;
    return true;
}

// // done async in loading thread now
// // consider: pass pool into this or keep items global??
// void PopulateItemPaths_DEP() {
//     for (int i = 0; i < items.count; i++) {
//         items[i].thumbpath = ItemPathToSubfolderPath(items[i].fullpath, L"~thumbs");
//         // items[i].thumbpath128 = ItemPathToSubfolderPath(items[i].fullpath, L"~thumbs128");
//         // items[i].thumbpath256 = ItemPathToSubfolderPath(items[i].fullpath, L"~thumbs256");
//         // items[i].thumbpath512 = ItemPathToSubfolderPath(items[i].fullpath, L"~thumbs512");
//         items[i].metadatapath = ItemPathToSubfolderPath(items[i].fullpath, L"~metadata");
//     }
// }





// for now, these are separate lists
// consider: combine with item struct?
// -wouldn't need to init list of same length as items
// -but i kind of like as separate for now


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
    FILE *file = _wfopen(path.chars, L"r");
    if (!file) {  DEBUGPRINT("error reading %s\n", path.ToUTF8Reusable()); return false; }
    int x, y;
    fwscanf(file, L"%i,%i", &x, &y);
    result->x = x;
    result->y = y;
    fclose(file);
    return true;
}
// create separate resolution metadata file (unique one for each item)
void CreateCachedResolution(string path, v2 size) {
    CreateAllDirectoriesForPathIfNeeded(path.chars);
    FILE *file = _wfopen(path.chars, L"w");
    if (!file) { DEBUGPRINT("error creating %s\n", path.ToUTF8Reusable()); return; }
    fwprintf(file, L"%i,%i", (int)size.x, (int)size.y); // todo: round up? (should all be square ints anyway, though)
    fclose(file);
}

void PopulateResolutionsFromSeparateFileCaches(item_pool *items, int *progress) {
    for (int i = 0; i < items->count; i++) {
        *progress = i;

        item_resolutions.add({0,0});
        v2& res = item_resolutions[i];

        if (GetCachedResolutionIfPossible(items->pool[i].metadatapath, &res)) {
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
            DEBUGPRINT("Couldn't load cached resolution from metadata for item: %s", items->pool[i].fullpath.ToUTF8Reusable());
            res = {7,7};
            assert(false);
        }

    }
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


// todo: replace inits with .init(count) in the collection?

void InitAllDataLists(int count) {
    InitModifiedTimes(count);
    InitResolutionsListToMatchItemList(count);
}

//
////



//
// new method that saves all metadata into one file
//

void SaveMetadataFile()
{
    // wc *path = L"D:\\Users\\phil\\Desktop\\meta.txt";
    wc *path = L"E:\\inspiration test folder\\~meta.txt";

    FILE *file = _wfopen(path, L"w, ccs=UTF-16LE");
    if (!file) {
        DEBUGPRINT("couldn't open master metadata file for writing");
        return;
    }
    for (int i = 0; i < items.count; i++) {

        // wc *justsubpath = PointerToFirstUniqueCharInSecondString(master_path.chars, items[i].fullpath.chars);

        assert(item_resolutions[i].x == (int)item_resolutions[i].x); // for now i just want to know if any resolutions are't whole numbers
        assert(item_resolutions[i].y == (int)item_resolutions[i].y);

        fwprintf(file, L"%i,%i,", (int)item_resolutions[i].x, (int)item_resolutions[i].y);
        fwprintf(file, L"%llu,", modifiedTimes[i]);
        fwprintf(file, L"%s", items[i].subpath.to_wc_reusable());
        fwprintf(file, L"%c", L'\0'); // note we add our own \0 after string for easier parsing later
        for (int j = 0; j < items[i].tags.count; j++) {
            fwprintf(file, L" %i", items[i].tags[j]);
        }
        fwprintf(file, L"\n");
    }
}

// for reading strings one wchar at a time (used as a stretch buffer, basically)
DEFINE_TYPE_POOL(wc);

void LoadMasterDataFileAndPopulateResolutionsAndTagsEtc(
                                                     // item_pool *itemslocal,
                                                     // v2_pool *resolutions,
                                                     // bool_pool *res_are_valid,
                                                     int *progress)
{
    // wc *path = L"D:\\Users\\phil\\Desktop\\meta.txt";
    wc *path = L"E:\\inspiration test folder\\~meta.txt";
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
        wc_pool result_string = wc_pool::new_empty();
        wc nextchar;
        do {
            numread = fwscanf(file, L"%c", &nextchar);
            if (numread < 0) {DEBUGPRINT("e3 %i\n",linesread);goto fileclose;} // eof (or maybe badly formed file?)
            result_string.add(nextchar);
        } while (nextchar != L'\0');
        result_string.add(L'\0'); // add \0 to end of array, we're going to treat array as a string

        wc *subpath = &result_string.pool[0]; // will only work if array ends in \0

        // use subpath to find item index
        int this_item_i = -1;
        for (int i = 0; i < items.count; i++) {
            // wc *justsubpath = items[i].PointerToJustSubpath(master_path);
            if (wc_equals(items[i].subpath.to_wc_reusable(), subpath))
            {
                this_item_i = i;
                break;
            }
        }
        if (this_item_i == -1) {
            DEBUGPRINT("ERROR: subpath in metadata list not found in item list: %s\n", string::KeepMemory(subpath).ToUTF8Reusable());
            assert(false);
        }
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
            items[this_item_i].tags.add(nextint);

        }
        linesread++;

    }

    fileclose:
    fclose(file);

}







newstring browse_tag_filter = newstring::allocate_new(128);
newstring view_tag_filter = newstring::allocate_new(128);


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
        for (int t = 0; t < items[i].tags.count; t++) {
            if (browse_tag_indices.has(items[i].tags[t])) {
                display_list.add(i);
                break; // next item
            }
        }
    }
}