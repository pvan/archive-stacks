

//
// global app data
// for use by multiple threads, etc





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


string_pool FindAllItemPaths(string master_path) {
    string_pool top_folders = win32_GetAllFilesAndFoldersInDir(master_path);
    string_pool result = string_pool::empty();

    string ignore1 = string::Create("~thumbs");  // todo: these should be consts somewhere
    string ignore2 = string::Create("~metadata");

    for (int folderI = 0; folderI < top_folders.count; folderI++) {
        if (win32_IsDirectory(top_folders[folderI])) {
            if (StringEndsWith(top_folders[folderI].chars, ignore1.chars) ||
                StringEndsWith(top_folders[folderI].chars, ignore2.chars))
            {
                DEBUGPRINT("Ignoring: %s\n", top_folders[folderI].ToUTF8Reusable());
            } else {
                string_pool subfiles = win32_GetAllFilesAndFoldersInDir(top_folders[folderI]);
                for (int fileI = 0; fileI < subfiles.count; fileI++) {
                    if (!win32_IsDirectory(subfiles[fileI].chars)) {
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

string_pool FindAllSubfolderPaths(string master_path, wc *subfolder) {

    string subfolder_path = master_path.CopyAndAppend(subfolder);
    string_pool top_files = win32_GetAllFilesAndFoldersInDir(subfolder_path);
    string_pool result = string_pool::empty();
    for (int folderI = 0; folderI < top_files.count; folderI++) {
        if (win32_IsDirectory(top_files[folderI])) {
            string_pool subfiles = win32_GetAllFilesAndFoldersInDir(top_files[folderI]);
            for (int fileI = 0; fileI < subfiles.count; fileI++) {
                if (!win32_IsDirectory(subfiles[fileI].chars)) {
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
    string_pool result = string_pool::empty();
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


string_pool ReadTagListFromFileOrSomethingUsableOtherwise(string master_path) {
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


    string_pool result = string_pool::empty();
    result.add(string::Create(L"untagged"));  // always have this entry as index 0?? todo: decide

    wc *path = master_path.CopyAndAppend(L"~taglist.txt").chars;

    if (!win32_PathExists(path)) return result;

    void *fileMem;
    int fileSize;
    Win32ReadFileBytesIntoNewMemoryW(path, &fileMem, &fileSize);

    char *fileChars = (char*)fileMem;
    int fileCharCount = fileSize / sizeof(char); // the same, but just to be sure

    int chars_written = 0;
    while (chars_written < fileCharCount) {
        char *thisTag = fileChars + chars_written;
        string thisTagString = string::Create(thisTag);
        if (!result.has(thisTagString)) {
            result.add(thisTagString);
        }
        free(thisTagString.chars);
        chars_written += (strlen(thisTag) + 1); // skip ahead to the next, remember to skip past the \0 as well
    }
    return result;
}




struct item {
    string fullpath;
    string thumbpath;
    // string thumbpath128;
    // string thumbpath256;
    // string thumbpath512;
    string metadatapath; //cached metadata
    bool operator==(item o) { return fullpath==o.fullpath; } // for now just check fullpath

    string justname() { wc *result = CopyJustFilename(fullpath.chars); return string::Create(result); }

    // indices into the pool tag_list tood: should be list not pool
    int_pool tags;
};

DEFINE_TYPE_POOL(item);

item_pool items;


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

bool PopulateTagFromPath(item *it) {
    wc *directory = CopyJustParentDirectoryName(it->fullpath.chars);
    if (!directory) return false; //todo: assert these instead?
    if (directory[0] == 0) return false;
    string dir = string::Create(directory);
    if (!tag_list.has(dir)) tag_list.add(dir);
    int index = tag_list.index_of(dir);
    if (index == -1) return false;
    it->tags.add(index);
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



//struct tag {
