


void win32_run_cmd_command(wchar_t *cmd) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcessW(0, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        DEBUGPRINT("CreateProcess() error!\n");
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
    }
    // check return value or anything from this?
    // bind it so any child processes are closes when this program ends?
    // see: https://stackoverflow.com/questions/3342941/kill-child-process-when-parent-process-is-killed
}


#include <Shlobj.h>  // SHGetFolderPathW

// should save to c:\users\[username]\appdata\local\[subpath]
// eg subpath could be "the-stacks\last-directory.txt" subpath should work with or without leading slash
void win32_save_memory_to_appdata(string subpath, char *mem, int bytes) {

    if (!mem || bytes==0) {
        DEBUGPRINT("no data to save to appdata");
        return;
    }

    if (subpath.count < 1) {
        DEBUGPRINT("no subpath in appdata to save data to");
        return;
    }

    wc appdata_path[MAX_PATH];
    if (SHGetFolderPathW(0, CSIDL_LOCAL_APPDATA, 0, SHGFP_TYPE_CURRENT, appdata_path) == S_OK) {

        if (!win32_IsDirectory(appdata_path)) {
            DEBUGPRINT("ERROR: app data folder seems to not exist...");
            return;
        }

        string finalpath = string::create_with_new_memory(appdata_path, __FILE__, __LINE__);
        if (subpath[0] != L'\\' && subpath[0] != L'/') finalpath.append(L'\\');
        finalpath.append(subpath);

        DEBUGPRINT("saving data to:");
        DEBUGPRINT(finalpath);

        win32_create_all_directories_needed_for_path(finalpath); // should this be embedded in writetofile?
        Win32WriteBytesToFileW(finalpath.to_wc_final(), mem, bytes);

        finalpath.free_all();

    } else {
        DEBUGPRINT("ERROR: unable to find app data folder");
    }

}
void win32_save_string_to_appdata(string subpath, string filecontents) {
    win32_save_memory_to_appdata(subpath, (char*)filecontents.list, filecontents.count*sizeof(filecontents.list[0]));
}


// should load from c:\users\[username]\appdata\local\[subpath]
// eg subpath could be "the-stacks\last-directory.txt" subpath should work with or without leading slash
// allocates new memory for the string we read
// return true if success, false otherwise
bool win32_load_memory_from_appdata_into_new_memory(string subpath, void **mem, int *size) {

    string result = string::new_empty();

    if (subpath.count < 1) {
        DEBUGPRINT("no subpath in appdata to load data from");
        return false;
    }

    wc appdata_path[MAX_PATH];
    if (SHGetFolderPathW(0, CSIDL_LOCAL_APPDATA, 0, SHGFP_TYPE_CURRENT, appdata_path) == S_OK) {

        if (!win32_IsDirectory(appdata_path)) {
            DEBUGPRINT("ERROR: app data folder seems to not exist...");
            return false;
        }

        string finalpath = string::create_with_new_memory(appdata_path, __FILE__, __LINE__);
        if (subpath[0] != L'\\' && subpath[0] != L'/') finalpath.append(L'\\');
        finalpath.append(subpath);

        if (!win32_PathExists(finalpath)) {
            DEBUGPRINT("cannot find subpath file in app data, probably doesn't exist yet");
            return false;
        }

        DEBUGPRINT("loading data from:");
        DEBUGPRINT(finalpath);

        Win32ReadFileBytesIntoNewMemoryW(finalpath.to_wc_final(), mem, size);

        finalpath.free_all();

    } else {
        DEBUGPRINT("ERROR: unable to find app data folder");
        return false;
    }
    return true;
}
// returns blank/null {0} string if fail
string win32_load_string_from_appdata_into_new_memory(string subpath) {
    string result = string::new_empty();
    void *mem;
    int size;
    if (win32_load_memory_from_appdata_into_new_memory(subpath, &mem, &size)) {
        if (size > 0) {
            assert(size % sizeof(result.list[0]) == 0); // just make sure we can convert from bytes to wc size (2)
            result.list = (wc*)mem;
            result.alloc = size / sizeof(result.list[0]);
            result.count = result.alloc; // unless there's a \0 in the file, our memory wont have a null terminator
        }
    } else {
    }
    return result;
}

