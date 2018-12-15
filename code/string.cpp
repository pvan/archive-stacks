

struct string
{

    wchar_t *chars;
    int length;

    bool operator== (string o) { return string::Equals(chars, o.chars); }


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

    // todo: make into win32_file function instead?
    static string CopyJustFilename(string source) {
        string result;

        wchar_t *c = source.chars;
        while (*c) c++; // foward to end
        while (*c != '/' && *c != '\\' && c > source.chars) c--;  // rewind to first slash (or first char)
        c++; // don't include the slash itself

        return Create(c);
    }


};





// //
// // string manipulation

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