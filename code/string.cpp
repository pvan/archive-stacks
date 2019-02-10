


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


    // might be a bug in here...
    // string& Append(wchar *suffix) {
    //     assert(chars);
    //     length += wcslen(suffix);
    //     chars = (wchar_t*)realloc(chars, (length+1)*sizeof(wchar_t));
    //     wcscat(chars, suffix);
    //     return *this;
    // }
    string CopyAndAppend(wchar *suffix) {
        string c = Copy();
        c.length += wcslen(suffix);
        c.chars = (wchar_t*)realloc(c.chars, (c.length+1)*sizeof(wchar_t));
        wcscat(c.chars, suffix);
        return c;
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

// void ExpandAndAppend(wc **s1, wc *s2) {
//     int len1 = wcslen(*s1);
//     int len2 = wcslen(s2);
//     *s1 = (wc*)realloc(*s1, (len1+len2+1) * sizeof(wc));
//     swprintf(*s1, L"%s%s", *s1, s2);
// }

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
    wc *parent = CopyJustParentDirectoryPath(mainpath);
    wc *assumed_master_dir = CopyJustParentDirectoryPath(parent);

    wc *directory = CopyJustParentDirectoryName(mainpath);
    wc *filename = CopyJustFilename(mainpath);

    wc *result = (wc*)malloc((wcslen(assumed_master_dir) +
                             wcslen(L"/~thumbs/") +
                             wcslen(directory) +
                             wcslen(L"/") +
                             wcslen(filename) +
                             1) * sizeof(wc));
    swprintf(result, L"%s/~thumbs/%s/%s", assumed_master_dir, directory, filename);

    free(parent);
    free(assumed_master_dir);
    free(directory);
    free(filename);

    return result;
}


// subfolder needs to not have leading and trailing /, eg; "~thumbs" not "/~thumbs/"
string ItemPathToSubfolderPath(string mainpath, wc *subfolder, wc *suffix) {

    // TODO: warning: wouldn't work on items not in a subdirectory, see xgz
    wc *parent = CopyJustParentDirectoryPath(mainpath.chars);
    wc *assumed_master_dir = CopyJustParentDirectoryPath(parent);

    wc *directory = CopyJustParentDirectoryName(mainpath.chars);
    wc *filename = CopyJustFilename(mainpath.chars);

    wc *result = (wc*)malloc((wcslen(assumed_master_dir) +
                             wcslen(L"/") +
                             wcslen(subfolder) +
                             wcslen(L"/") +
                             wcslen(directory) +
                             wcslen(L"/") +
                             wcslen(filename) +
                             wcslen(suffix) +
                             1) * sizeof(wc));
    swprintf(result, L"%s/%s/%s/%s%s", assumed_master_dir, subfolder, directory, filename, suffix);

    free(parent);
    free(assumed_master_dir);
    free(directory);
    free(filename);

    string resultString;
    resultString.chars = result;
    resultString.length = wcslen(result);
    return resultString;
}

