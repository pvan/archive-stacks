

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



struct item {
    string fullpath;
    string thumbpath;
    // string thumbpath128;
    // string thumbpath256;
    // string thumbpath512;
    string metadatapath; //cached metadata
    bool operator==(item o) { return fullpath==o.fullpath; } // for now just check fullpath
};

DEFINE_TYPE_POOL(item);

item_pool items;

// done async in loading thread now
// consider: pass pool into this or keep items global??
void PopulateItemPaths_DEP() {
    for (int i = 0; i < items.count; i++) {
        items[i].thumbpath = ItemPathToSubfolderPath(items[i].fullpath, L"~thumbs");
        // items[i].thumbpath128 = ItemPathToSubfolderPath(items[i].fullpath, L"~thumbs128");
        // items[i].thumbpath256 = ItemPathToSubfolderPath(items[i].fullpath, L"~thumbs256");
        // items[i].thumbpath512 = ItemPathToSubfolderPath(items[i].fullpath, L"~thumbs512");
        items[i].metadatapath = ItemPathToSubfolderPath(items[i].fullpath, L"~metadata");
    }
}

