

//
// global app data
// for use by multiple threads, etc

tile_pool tiles; // access from background thread as well




// paths...

string_pool files; // string collection of paths to all item files
string_pool thumb_files; // string collection of paths to the thumbnail files


// if we start using index lists like this, we'll need to keep items in the same order at all times
string_pool items_without_matching_thumbs;
string_pool thumbs_without_matching_item;



// struct item {
//     string fullpath;
//     string thumbpath128;
//     string thumbpath256;
//     string thumbpath512;
//     string metadatapath; //cached metadata
//     bool operator==(item o) { return fullpath==o.fullpath; } // for now just check fullpath
// };

// DEFINE_TYPE_POOL(item);

// item_pool items;

// // todo: pass pool into this or keep items global??
// void PopulateItemPaths() {
//     for (int i = 0; i < items.count; i++) {
//         items[i].thumbpath128 = ItemPathToSubfolderPath(items[i].fullpath, L"~thumbs128");
//         items[i].thumbpath256 = ItemPathToSubfolderPath(items[i].fullpath, L"~thumbs256");
//         items[i].thumbpath512 = ItemPathToSubfolderPath(items[i].fullpath, L"~thumbs512");
//         items[i].metadatapath = ItemPathToSubfolderPath(items[i].fullpath, L"~metadata");
//     }
// }

