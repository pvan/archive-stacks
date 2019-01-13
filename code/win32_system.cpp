


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
}


