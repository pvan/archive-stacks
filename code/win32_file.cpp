


// return the pointer with the smaller address (for "which is first in a string" check)
// try to never return a null pointer though (treat 0 as +infinity for comparisons)
wchar *min_but_not_null(wchar *p1, wchar *p2) { return p1<p2 && p1!=0 ? p1 : p2==0? 0 : p2; }

void CreateAllDirectoriesForPathIfNeeded(wchar_t *path)
{
    // if (PathFileExistsA(path)) return;

    wchar_t folder[MAX_PATH];
    ZeroMemory(folder, MAX_PATH * sizeof(wchar_t));

    wchar_t *end = min_but_not_null(wcschr(path, '\\'), wcschr(path, '/'));

    while (end != 0)
    {
        wcsncpy(folder, path, end - path + 1);
        CreateDirectoryW(folder, 0);
        end++;
        end = min_but_not_null(wcschr(end, '\\'), wcschr(end, '/'));
    }
}



string_pool win32_GetAllFilesAndFoldersInDir(string path)
{
    string_pool results = string_pool::empty();

    string dir_path = path.CopyAndAppend(L"/"); // for use when appending subfolders to this path
    string search_path = dir_path.CopyAndAppend(L"*"); // wildcard for search

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search_path.chars, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) { return string_pool::empty(); }
    do {
        if(string::Equals(ffd.cFileName, L".") || string::Equals(ffd.cFileName, L"..")) continue;

        string full_path = dir_path.CopyAndAppend(ffd.cFileName);
        results.add(full_path);

        // string this_path = string::Create(ffd.cFileName);
        // results.add(this_path);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        }
        else {
        }
    }
    while (FindNextFileW(hFind, &ffd) != 0);
    FindClose(hFind);

    return results;
}

bool win32_IsDirectory(wchar_t *path) { DWORD res = GetFileAttributesW(path); return res!=INVALID_FILE_ATTRIBUTES && res&FILE_ATTRIBUTE_DIRECTORY; }
bool win32_IsDirectory(string path) { return win32_IsDirectory(path.chars); }

bool win32_PathExists(wchar_t *path) { return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES; }
bool win32_PathExists(string path) { return win32_PathExists(path.chars); }
// bool win32_PathExists(char *path) { return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES; }
// bool win32_PathExists(string path) { return win32_PathExists(path.ToUTF8Reusable()); }



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
u64 win32_GetModifiedTimeSinceEpoc(string path) { return win32_GetModifiedTimeSinceEpoc(path.chars); }



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



