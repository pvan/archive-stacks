
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

    // todo: make into win32_file function or path/platform function instead?
    static string CopyJustFilename(string source) {
        string result;

        wchar_t *c = source.chars;
        while (*c) c++; // foward to end
        while (*c != '/' && *c != '\\' && c > source.chars) c--;  // rewind to first slash (or first char)
        c++; // don't include the slash itself

        return Create(c);
    }
    // todo: make into win32_file function or path/platform function instead?
    // returns name of parent directory assuming string is a path
    string ParentDirectoryNameReusable() {
        string result;

        wchar_t *c = chars;
        while (*c) c++; // foward to end
        while (*c != '/' && *c != '\\' && c > chars) c--;  // rewind to first slash (or first char)
        c--; // skip past slash and to look for the next one
        while (*c != '/' && *c != '\\' && c > chars) c--;  // rewind to second slash (or first char)
        c++; // don't include the slash itself

        wchar_t *tempmem = string::CopyIntoReusableMem(c);
        c = tempmem;
        while(*c && *c != '/' && *c != '\\') c++;
        *c = 0;

        return NewStringUsingMem(tempmem);
    }


};





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