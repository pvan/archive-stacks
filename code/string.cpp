


// todo: look at this again after trying it for a bit
// basically just for comparisons or printing, as memory will expire after 1 more use
// we toggle between which we use, so we can technically use 2 to compare to each other
// todo: add longer history
char string_reusable_mem1[1024];
char string_reusable_mem2[1024];
bool string_reusable_toggle = false; // which reusable mem should we use next?

struct string
{

    wchar_t *chars;
    int length;

    bool operator== (string o) { return string::Equals(chars, o.chars); }
    bool operator!= (string o) { return !string::Equals(chars, o.chars); }


    string& Append(wchar *suffix) {
        assert(chars);
        length += wcslen(suffix);
        chars = (wchar_t*)realloc(chars, (length+1)*sizeof(wchar_t));
        wcscat(chars, suffix);
        return *this;
    }

    string Copy() {
        string newString = {0};
        newString.length = length;
        newString.chars = (wchar_t*)malloc((length+1)*sizeof(wchar_t));
        memcpy(newString.chars, chars, (length+1)*sizeof(wchar_t));
        return newString;
    }



    // win32
    char *ToUTF8() {
        int numChars = WideCharToMultiByte(CP_UTF8,0,  chars,-1,  0,0,  0,0);
        char *utf8 = (char*)malloc(numChars*sizeof(char));
        WideCharToMultiByte(CP_UTF8,0,  chars,-1,  utf8,numChars*sizeof(char),  0,0);
        return utf8;
    }

    char *ToUTF8Reusable() {
        int numChars = WideCharToMultiByte(CP_UTF8,0,  chars,-1,  0,0,  0,0);
        char *utf8 = string_reusable_toggle ? (char*)&string_reusable_mem1 : (char*)&string_reusable_mem2; string_reusable_toggle=!string_reusable_toggle;
        WideCharToMultiByte(CP_UTF8,0,  chars,-1,  utf8,numChars*sizeof(char),  0,0);
        return utf8;
    }


    static bool Equals(wchar_t *s1, wchar_t *s2) {
        return wcscmp(s1, s2) == 0;
    }

    static string Create(wchar_t *source) {
        string newString = {0};
        newString.length = wcslen(source);
        newString.chars = (wchar_t*)malloc((newString.length+1)*sizeof(wchar_t));
        memcpy(newString.chars, source, (newString.length+1)*sizeof(wchar_t));
        return newString;
    }

    static string NewStringUsingMem(wchar_t *source) {
        string newString = {0};
        newString.length = wcslen(source);
        newString.chars = source;
        return newString;
    }

    static string Create(char *source) {
        string newString = {0};
        // newString.length = strlen(source);

        newString.length = MultiByteToWideChar(CP_UTF8,0,  source,-1,  0,0);
        newString.chars = (wchar_t*)malloc((newString.length+1)*sizeof(wchar_t));
        MultiByteToWideChar(CP_UTF8,0,  source,-1,  newString.chars,newString.length*sizeof(char));

        // memcpy(newString.chars, source, (newString.length+1)*sizeof(wchar_t));
        return newString;
    }

    static wchar_t *CopyIntoReusableMem(wchar_t *source) {
        wchar_t *dest = string_reusable_toggle ? (wchar_t*)&string_reusable_mem1 : (wchar_t*)&string_reusable_mem2;
        string_reusable_toggle =! string_reusable_toggle;

        int length = wcslen(source);
        memcpy(dest, source, (length+1)*sizeof(wchar_t));
        return dest;
    }

};


//
// generic wchar* stuff

void ExpandAndAppend(wc **s1, wc *s2) {
    int len1 = wcslen(*s1);
    int len2 = wcslen(s2);
    *s1 = (wc*)realloc(*s1, (len1+len2+1) * sizeof(wc));
    swprintf(*s1, L"%s%s", *s1, s2);
}

bool StringEndsWith(wc *s1, wc *s2) {
    wc *startOfS2 = s2;
    while (*s1) s1++;
    while (*s2) s2++;
    while (true) {
        if (s2 == startOfS2) return true;
        if (*s1 != *s2) return false;
        s1--;
        s2--;
    }
}

//
// path stuff (move to io/file/platform oriented files?)


wc *CopyJustFilename(wc *source) {
    wc *temp = (wc*)malloc((wcslen(source)+1) * sizeof(wc));
    memcpy(temp, source, (wcslen(source)+1) * sizeof(wc));

    wc *c = temp;
    while (*c) c++; // foward to end
    while (*c != '/' && *c != '\\' && c > temp) c--;  // rewind to first slash (or first char)
    c++; // don't include the slash itself
    // result = c;  // nope! we'll lose our original malloc'd pointer

    wc *result = (wc*)malloc((wcslen(c)+1) * sizeof(wc));
    memcpy(result, c, (wcslen(c)+1) * sizeof(wc));
    free(temp);

    return result;
}

// if path ends with / it is ignored
// so parent(D:/example/subdir/) returns D:/example
wc *CopyJustParentDirectoryPath(wc *source) {
    wc *result = (wc*)malloc((wcslen(source)+1) * sizeof(wc));
    memcpy(result, source, (wcslen(source)+1) * sizeof(wc));

    wc *c = result;
    while (*c) c++; // foward to end
    while (*c == '/' || *c == '\\') {*c = 0; c--;} // remove all trailing / or \

    while (*c != '/' && *c != '\\' && c > result) c--;  // rewind to last slash in string (or first char)
    // while (*c != '/' && *c != '\\' && c > chars) c--;  // rewind to second slash (or first char)
    // c++; // don't include the slash itself
    *c = 0; // zap the slash and everything after it

    // wchar_t *tempmem = string::CopyIntoReusableMem(c); // copy from just the \

    // // skip forward to just the name
    // c = tempmem;
    // while(*c && *c != '/' && *c != '\\') c++;
    // *c = 0;

    return result;
}

wc *CopyJustParentDirectoryName(wc *source) {
    wc *temp = (wc*)malloc((wcslen(source)+1) * sizeof(wc));
    memcpy(temp, source, (wcslen(source)+1) * sizeof(wc));

    wc *c = temp;
    while (*c) c++; // foward to end
    while (*c == '/' || *c == '\\') {*c = 0; c--;} // remove all trailing / or \

    while (*c != '/' && *c != '\\' && c > temp) c--;  // rewind to last slash in string (or first char)
    *c = 0; // zap the slash and everything after it

    while (*c != '/' && *c != '\\' && c > temp) c--;  // rewind again to find the new last slash
    c++; // don't include the slash itself
    // result = c; // that's our new starting point // nope! we'll lose our original malloc'd pointer

    wc *result = (wc*)malloc((wcslen(c)+1) * sizeof(wc));
    memcpy(result, c, (wcslen(c)+1) * sizeof(wc));
    free(temp);

    return result;
}



// //
// // string manipulation (move to different place than string.cpp?)

// wchar_t *TrimPathToJustFilename(wchar_t *path)
// {
//     wchar_t *c = path;
//     while (*c) c++; // foward to end
//     while (*c != '/' && *c != '\\' && c > path) c--;  // rewind to first slash (or first char)
//     c++; // don't include the slash itself
//     return c;
// }
// wchar_t *TrimCommonPrefix(wchar_t *commonPrefix, wchar_t *fullString)
// {
//     wchar_t *c = fullString;
//     wchar_t *p = commonPrefix;
//     while (*c && *p && *p==*c) { c++; p++; }
//     return c;
// }
// wchar_t *TrimToJustExtension(wchar_t *path) {
//     wchar_t *c = path;
//     while (*c) c++; // foward to end
//     while (*c != '.' && c > path) c--;  // rewind to first .
//     c++; // don't include the . itself
//     return c;
// }
// void TrimLastSlashAndEverythingAfter(wchar_t *str)
// {
//     wchar_t *lastSlash = 0;
//     for (wchar_t *c = str; *c; c++) {
//         if (*c == '\\') lastSlash = c;
//     }
//     if (*lastSlash) { // found a slash at all
//         // lastSlash++;
//         *lastSlash = '\0';
//     }
// }

// string TrimPathToJustFilename(string path) { TrimPathToJustFilename(path.chars); return path; }
// string TrimCommonPrefix(string commonPrefix, string fullString) { return TrimCommonPrefix(commonPrefix.chars, fullString.chars); }
// string TrimToJustExtension(string path) { return TrimToJustExtension(path.chars); }
// void TrimLastSlashAndEverythingAfter(string str) { TrimLastSlashAndEverythingAfter(str.chars); }


//
// app-specific path stuff

wc *CopyItemPathAndConvertToThumbPath(wc *mainpath) {

    // TODO: warning: wouldn't work on items not in a subdirectory, see xgz
        // OutputDebugStringW(L"orignl: ");
        // OutputDebugStringW(mainpath.chars);
        // OutputDebugStringW(L"\n");
    // string parent = mainpath.ParentDirectoryPathCopy();
    wc *parent = CopyJustParentDirectoryPath(mainpath);
    // wc *parent = (wc*)malloc((wcslen(mainpath)+1) * sizeof(wc));
        // OutputDebugStringW(L"parent: ");
        // OutputDebugStringW(parent.chars);
        // OutputDebugStringW(L"\n");
    wc *assumed_master_dir = CopyJustParentDirectoryPath(parent);
    // wc *assumed_master_dir = (wc*)malloc((wcslen(mainpath)+1) * sizeof(wc));
    // memcpy(assumed_master_dir, mainpath, (wcslen(mainpath)+1) * sizeof(wc));
    // wc *c = assumed_master_dir;
    // while (*c) c++; // foward to end
    // c--; while (*c == '/' || *c == '\\') {*c = 0; c--;} // remove all trailing \ or /
    // while (*c != '/' && *c != '\\' && c > assumed_master_dir) c--;  // rewind to last slash in string (or first char)
    // if (c>assumed_master_dir) c--; // if not alerady at start of string, skip over this slash to find next
    // while (*c != '/' && *c != '\\' && c > assumed_master_dir) c--;  // rewind to next slash in string (or first char)
    // *c = 0; // zap the slash and everything after it
        // OutputDebugStringW(L"master: ");
        // OutputDebugStringW(assumed_master_dir.chars);
        // OutputDebugStringW(L"\n");

    wc *directory = CopyJustParentDirectoryName(mainpath);
    wc *filename = CopyJustFilename(mainpath);


    // wc *directory = (wc*)malloc((wcslen(mainpath)+1) * sizeof(wc));
    // memcpy(directory, mainpath, (wcslen(mainpath)+1) * sizeof(wc));
    // c = directory;
    // while (*c) c++; // foward to end
    // c--; while (*c == '/' || *c == '\\') {*c = 0; c--;} // remove all trailing \ or /
    // while (*c != '/' && *c != '\\' && c > directory) c--;  // rewind to last slash in string (or first char)
    // *c = 0; // zap the slash and everything after it
    // if (c>assumed_master_dir) c--; // if not alerady at start of string, skip over this slash to find next
    // while (*c != '/' && *c != '\\' && c > assumed_master_dir) c--;  // rewind to next slash in string (or first char)
    // c++;
    // directory = c; // new start


    // wc *filename = (wc*)malloc((wcslen(mainpath)+1) * sizeof(wc));
    // memcpy(filename, mainpath, (wcslen(mainpath)+1) * sizeof(wc));
    // c = filename;
    // while (*c) c++; // foward to end
    // c--; while (*c == '/' || *c == '\\') {*c = 0; c--;} // remove all trailing \ or /
    // while (*c != '/' && *c != '\\' && c > filename) c--;  // rewind to last slash in string (or first char)
    // c++;
    // filename = c; // new start


    // wc *result = assumed_master_dir;
    wc *result = (wc*)malloc((wcslen(assumed_master_dir) +
                             wcslen(L"/~thumbs/") +
                             wcslen(directory) +
                             wcslen(L"/") +
                             wcslen(filename) +
                             1) * sizeof(wc));
    swprintf(result, L"%s/~thumbs/%s/%s", assumed_master_dir, directory, filename);

    // ExpandAndAppend(&result, L"/~thumbs/");
    // ExpandAndAppend(&result, directory);
    // ExpandAndAppend(&result, L"/");
    // ExpandAndAppend(&result, filename);

    free(parent);
    free(directory);
    free(filename);

    return result;
}


// string CopyThumbPathAndConvertToItemPath(string thumbpath) {

//     // TODO: warning: wouldn't work on items not in a subdirectory, see xgz
//     wc (subdir = thumbpath.ParentDirectoryPathReusable();
//     string thumbdir = subdir.ParentDirectoryPathReusable();
//     string assumed_master_dir = thumbdir.ParentDirectoryPathReusable().Copy();

//     string directory = thumbpath.ParentDirectoryNameReusable();
//     string filename = string::CopyJustFilename(thumbpath);

//     // haha, this api is a mess
//     // some of the above vars are still valid and some aren't, and some need to be freed

//     string result = assumed_master_dir;

//     result.Append(L"/");
//     result.Append(directory.chars);
//     result.Append(L"/");
//     result.Append(filename.chars);

//     free(filename.chars);

//     return result;
// }


