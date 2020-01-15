




void win32_create_all_directories_needed_for_path(string path)
{
    // in terms of operations and memory, this could be done with a lot less
    string copy = path.copy_into_new_memory(__FILE__, __LINE__);
    copy.find_replace(L'/', L'\\');
    string_pool split = split_by_delim(copy, L'\\');

    string growing_partial_path = split[0].copy_into_new_memory(__FILE__, __LINE__);
    for (int i = 1; i < split.count; i++) {
        growing_partial_path.append(L'\\');
        growing_partial_path.append(split[i]);
        growing_partial_path.append(L'\0');
        // DEBUGPRINT(growing_partial_path);
        CreateDirectoryW(growing_partial_path.list, 0); // only works if we apply null terminator before
        growing_partial_path.rtrim(1); // remove null terminator
    }

    growing_partial_path.free_all();
    copy.free_all();
    deep_free_string_pool(split);
}



string_pool win32_GetAllFilesAndFoldersInDir(string path)
{
    string_pool results = string_pool::new_empty();

    string dir_path = path.copy_into_new_memory(__FILE__, __LINE__); // for use when appending subfolders to this path
    if (!dir_path.ends_with(L"/") && !dir_path.ends_with(L"\\")) // todo: not fully tested, code at time of comment works if we just append '/' no matter what
        dir_path.append(L"/");
    string search_path = dir_path.copy_and_append(L"*", __FILE__, __LINE__); // wildcard for search

    WIN32_FIND_DATAW ffd;
    wc *temp = search_path.to_wc_new_memory(__FILE__, __LINE__);
    HANDLE hFind = FindFirstFileW(temp, &ffd);
    free(temp);
    if (hFind == INVALID_HANDLE_VALUE) { return string_pool::new_empty(); }
    do {
        if(wc_equals(ffd.cFileName, L".") || wc_equals(ffd.cFileName, L"..")) continue;

        string full_path = dir_path.copy_and_append(ffd.cFileName, __FILE__, __LINE__);
        results.add(full_path);

        // string this_path = string::Create(ffd.cFileName);
        // results.add(this_path);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // is directory
        }
        else {
            // is file
        }
    }
    while (FindNextFileW(hFind, &ffd) != 0);
    FindClose(hFind);

    dir_path.free_all();
    search_path.free_all();

    return results;
}

bool win32_IsDirectory(wchar_t *path) { DWORD res = GetFileAttributesW(path); return res!=INVALID_FILE_ATTRIBUTES && res&FILE_ATTRIBUTE_DIRECTORY; }
// bool win32_IsDirectory(string path) { return win32_IsDirectory(path.chars); }
bool win32_IsDirectory(string path) {
    wc *temp = path.to_wc_new_memory(__FILE__, __LINE__);
    bool res = win32_IsDirectory(temp);
    free(temp);
    return res;
}

bool win32_PathExists(wchar_t *path) { return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES; }
// bool win32_PathExists(string path) { return win32_PathExists(path.chars); }
bool win32_PathExists(string path) {
    wc *temp = path.to_wc_new_memory(__FILE__, __LINE__);
    bool res = win32_PathExists(temp);
    free(temp);
    return res;
}


void win32_Rename(wchar_t *orig, wchar_t *dest) {
    // todo: do we need special handling for diff drive?

    // todo what an allocation mess lol

    // todo: improve \\?\ prepend method? (needed for paths longer than MAX_PATH)
    string a = string::create_with_new_memory(orig, __FILE__, __LINE__);
    string b = string::create_with_new_memory(dest, __FILE__, __LINE__);

    string prefix = string::create_with_new_memory(L"\\\\?\\", __FILE__, __LINE__);

    string aa = prefix.copy_and_append(a, __FILE__, __LINE__);
    string bb = prefix.copy_and_append(b, __FILE__, __LINE__);

    wc *a_wc = aa.to_wc_new_memory(__FILE__, __LINE__);
    wc *b_wc = bb.to_wc_new_memory(__FILE__, __LINE__);

    prefix.free_all();
    a.free_all();
    b.free_all();
    aa.free_all();
    bb.free_all();

    if (win32_PathExists(b_wc)) {
        DEBUGPRINT("Trying to rename file to file that already exists!\n");
        DEBUGPRINT("Maybe a bug with thumbnail renaming?\n");
        // consider deleting old orig file in this case?
        // or force overwrite old dest file?
        assert(false);
    }

    // if (!MoveFileW(orig, dest)) {
    if (!MoveFileW(a_wc, b_wc)) {

        DEBUGPRINT("MoveFileW() error!\n");
        DEBUGPRINT(a_wc);
        DEBUGPRINT(b_wc);
        { // print error msg (todo: make into function)
            DWORD err = GetLastError();
            char *errtext;
            DWORD fm = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                0,
                err,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&errtext,
                0,
                0
            );
            DEBUGPRINT(errtext);
        }

        // // todo: improve error handling?
        // DEBUGPRINT("failed renaming file");
        assert(false);
    }

    free(a_wc);
    free(b_wc);
}



// for GetOpenFileNameW
#pragma comment(lib, "Comdlg32.lib")

// for IFileOpenDialog and friends
#include <shobjidl.h>
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Shell32.lib")

// create popup dialog to select a folder
// pass a pointer that will be changed to the selected folder
// todo: value of pointer passed in will be the default starting directory
void win32_OpenFolderSelectDialog(HWND hwnd, string *inAndOutString) {

    // GetOpenFileNameW
    // older, but lighterweight
    // unfortunately no easy folder selection

    // wc *buffer = (wc*)malloc(256 * sizeof(wc));

    // OPENFILENAMEW ofn = {0};
    // ofn.lStructSize = sizeof(OPENFILENAME);
    // ofn.hwndOwner = hwnd;
    // ofn.lpstrFile = buffer;
    // ofn.nMaxFile = 256;
    // // ofn.lpstrInitialDir

    // if (GetOpenFileNameW(&ofn)) {
    //     string result = {0};
    //     result = string::create_with_new_memory(ofn.lpstrFile);
    //     return result;
    // } else {

    // }


    // newer way, IFileDialog
    // loads a bunch of shell-related dlls though
    // and my god, it's full of hoops to go through
    // but easy to select folders, so let's go with it

    if (CoInitializeEx(0, COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE) >=0)
    {
        IFileOpenDialog *pfo;
        if (CoCreateInstance(CLSID_FileOpenDialog, 0, CLSCTX_ALL,
                             IID_IFileOpenDialog, (void**)(&pfo)) >=0)
        {

            // set as folder select
            FILEOPENDIALOGOPTIONS options;
            pfo->GetOptions(&options);
            pfo->SetOptions(options | FOS_PICKFOLDERS);

            // set initial folder
            IShellItem *psi = 0;

            if (win32_IsDirectory(*inAndOutString)) {
                wc *temp = inAndOutString->to_wc_new_memory(__FILE__, __LINE__);
                SHCreateItemFromParsingName(temp, 0, IID_PPV_ARGS(&psi));
                free(temp);
                pfo->SetFolder(psi);
            }

            if (pfo->Show(0) >=0)
            {
                IShellItem *pitem;
                if (pfo->GetResult(&pitem) >=0)
                {
                    PWSTR filepath;
                    if (pitem->GetDisplayName(SIGDN_FILESYSPATH, &filepath) >=0)
                    {
                        // MessageBoxW(0,filepath, L"yurp", MB_OK);
                        inAndOutString->empty_out();
                        inAndOutString->append(filepath);
                        CoTaskMemFree(filepath);
                    }
                    pitem->Release();
                }
            }

            if(psi) psi->Release();
            pfo->Release();
        }
        CoUninitialize();
    }

}


// DATE / TIME

u64 win32_GetModifiedTimeSinceEpoc(wchar_t *path)
{
    HANDLE file_handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    assert(file_handle != INVALID_HANDLE_VALUE);

    FILETIME creation;
    FILETIME access;
    FILETIME modified;

    GetFileTime(file_handle, &creation, &access, &modified);

    ULARGE_INTEGER result;
    result.LowPart = modified.dwLowDateTime;
    result.HighPart = modified.dwHighDateTime;
    return result.QuadPart;
}
u64 win32_GetModifiedTimeSinceEpoc(string path) {
    wc *temp = path.to_wc_new_memory(__FILE__, __LINE__);
    u64 res = win32_GetModifiedTimeSinceEpoc(temp);
    free(temp);
    return res;
}



// unused atm, for later maybe

void FormatTimeIntoCharBuffer(u64 time_since_epoch, char *buffer) {
    ULARGE_INTEGER ularge;
    ularge.QuadPart = time_since_epoch;

    FILETIME filetime;
    filetime.dwLowDateTime = ularge.LowPart;
    filetime.dwHighDateTime = ularge.HighPart;

    SYSTEMTIME stUTC, stLocal;
    FileTimeToSystemTime(&filetime, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

    // Build a string showing the date and time.
    sprintf(buffer,  "%02d/%02d/%d  %02d:%02d",
            stLocal.wMonth, stLocal.wDay, stLocal.wYear,
            stLocal.wHour, stLocal.wMinute);
}
void FormatDateIntoCharBuffer(u64 time_since_epoch, char *buffer) {
    ULARGE_INTEGER ularge;
    ularge.QuadPart = time_since_epoch;

    FILETIME filetime;
    filetime.dwLowDateTime = ularge.LowPart;
    filetime.dwHighDateTime = ularge.HighPart;

    SYSTEMTIME stUTC, stLocal;
    FileTimeToSystemTime(&filetime, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

    // Build a string showing the date and time.
    sprintf(buffer,  "%02d/%02d/%d",
            stLocal.wMonth, stLocal.wDay, stLocal.wYear);
}

void DebugPrintTime(u64 time_since_epoch)
{
    ULARGE_INTEGER ularge;
    ularge.QuadPart = time_since_epoch;

    FILETIME filetime;
    filetime.dwLowDateTime = ularge.LowPart;
    filetime.dwHighDateTime = ularge.HighPart;

    SYSTEMTIME stUTC, stLocal;
    FileTimeToSystemTime(&filetime, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

    // Build a string showing the date and time.
    char outstring[256];
    sprintf(outstring,  "%02d/%02d/%d  %02d:%02d\n",
            stLocal.wMonth, stLocal.wDay, stLocal.wYear,
            stLocal.wHour, stLocal.wMinute);
    OutputDebugString(outstring);
}



//------------------



// //
// // copied in from prev and maybe not used (now definitely used)
// // |
// // v
// //

void Win32ReadFileBytesIntoNewMemoryW(wc *path, void **memory, int *bytes)
{
    HANDLE file_handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    assert(file_handle != INVALID_HANDLE_VALUE);

    LARGE_INTEGER file_size;
    if (GetFileSizeEx(file_handle, &file_size)) {
        int file_size_bytes = (int)file_size.QuadPart;
        if (file_size_bytes) {
            // *memory = VirtualAlloc(0, file_size_bytes, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            *memory = malloc(file_size_bytes); // so we can realloc later in case we use this for a string
            if (*memory) {
                DWORD bytes_read;
                if (ReadFile(file_handle, *memory, file_size_bytes, &bytes_read, 0) && file_size_bytes == bytes_read) {
                    *bytes = file_size_bytes;
                } else {
                    // couldn't read file
                    free(*memory);
                    // VirtualFree(*memory, 0, MEM_RELEASE);
                    *bytes = 0;
                    assert(false);
                }
            } else {
                // no mem
                assert(false);
            }
        } else {
            // 0 file size, file probably exists but is empty
            *bytes = 0;
        }
    } else {
        // no file size
        assert(false);
    }
    CloseHandle(file_handle);
}

void Win32WriteBytesToFileW(wc *path, void *memory, int bytes) {
    HANDLE file_handle = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
    assert(file_handle != INVALID_HANDLE_VALUE);
    if (bytes > 0) {
        DWORD bytes_written;
        if (WriteFile(file_handle, memory, bytes, &bytes_written, 0) && bytes == bytes_written) {
            // success
        }
    } else {
        // not passed any mem to write
    }
    CloseHandle(file_handle);
}

void Win32AppendBytesToFileW(wc *path, void *memory, int bytes) {
    HANDLE file_handle = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_ALWAYS, 0, 0);
    assert(file_handle != INVALID_HANDLE_VALUE);
    if (bytes > 0) {
        SetFilePointer(file_handle, 0, NULL, FILE_END); // move write cursor to end of file
        DWORD bytes_written;
        if (WriteFile(file_handle, memory, bytes, &bytes_written, 0) && bytes == bytes_written) {
            // success
        }
    } else {
        // no mem to write
    }
    CloseHandle(file_handle);
}




// void Win32ReadFileBytesIntoNewMemoryA(char *path, void **memory, int *bytes)
// {
//     HANDLE file_handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
//     assert(file_handle != INVALID_HANDLE_VALUE);

//     LARGE_INTEGER file_size;
//     if (GetFileSizeEx(file_handle, &file_size)) {
//         int file_size_bytes = (int)file_size.QuadPart;
//         if (file_size_bytes) {
//             *memory = VirtualAlloc(0, file_size_bytes, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
//             if (*memory) {
//                 DWORD bytes_read;
//                 if (ReadFile(file_handle, *memory, file_size_bytes, &bytes_read, 0) && file_size_bytes == bytes_read) {
//                     *bytes = file_size_bytes;
//                 } else {
//                     // couldn't read file
//                     VirtualFree(*memory, 0, MEM_RELEASE);
//                     *bytes = 0;
//                     assert(false);
//                 }
//             } else {
//                 // no mem
//                 assert(false);
//             }
//         } else {
//             // 0 file size, file probably exists but is empty
//             *bytes = 0;
//         }
//     } else {
//         // no file size
//         assert(false);
//     }
//     CloseHandle(file_handle);
// }

// void Win32WriteBytesToFileA(char *path, void *memory, int bytes) {
//     HANDLE file_handle = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
//     assert(file_handle != INVALID_HANDLE_VALUE);
//     if (bytes > 0) {
//         DWORD bytes_written;
//         if (WriteFile(file_handle, memory, bytes, &bytes_written, 0) && bytes == bytes_written) {
//             // success
//         }
//     } else {
//         // not passed any mem to write
//     }
//     CloseHandle(file_handle);
// }

// void Win32AppendBytesToFileA(char *path, void *memory, int bytes) {
//     HANDLE file_handle = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_ALWAYS, 0, 0);
//     assert(file_handle != INVALID_HANDLE_VALUE);
//     if (bytes > 0) {
//         SetFilePointer(file_handle, 0, NULL, FILE_END); // move write cursor to end of file
//         DWORD bytes_written;
//         if (WriteFile(file_handle, memory, bytes, &bytes_written, 0) && bytes == bytes_written) {
//             // success
//         }
//     } else {
//         // no mem to write
//     }
//     CloseHandle(file_handle);
// }


