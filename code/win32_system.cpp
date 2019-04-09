


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

    wc path[MAX_PATH];
    // memset(path, 0, MAX_PATH);

    if (!mem || bytes==0) {
        DEBUGPRINT("no data to save to appdata");
        return;
    }

    if (subpath.count < 1) {
        DEBUGPRINT("no subpath in appdata to save data to");
        return;
    }

    if (SHGetFolderPathW(0, CSIDL_LOCAL_APPDATA, 0, SHGFP_TYPE_CURRENT, path) == S_OK) {

        if (!win32_IsDirectory(path)) {
            DEBUGPRINT("ERROR: app data folder seems to not exist...");
            return;
        }

        string finalpath = string::create_with_new_memory(path);
        if (subpath[0] != L'\\' && subpath[0] != L'/') finalpath.append(L'\\');
        finalpath.append(subpath);

        DEBUGPRINT("saving data to:");
        DEBUGPRINT(finalpath);

        // wc *final = finalpath.to_wc_final();
        win32_create_all_directories_needed_for_path(finalpath); // should this be embedded in writetofile?
        Win32WriteBytesToFileW(finalpath.to_wc_final(), mem, bytes);

        finalpath.free_all();

    } else {
        DEBUGPRINT("ERROR: unable to find app data folder");
    }

}
