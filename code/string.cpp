



//
// new try at a string lib
// basically start with a stretchy list
// and add whatever string manipulation functions we need
//
// storing everything internally as wchar for now since we're only on windows anyway
// but need utf8 conversions to interface with some libs
//
// no longer using reusable memory here for temp strings, wasn't thread safe
// (could have made it work but not worth it)
// instead just let caller copy strings whenever exports are needed
// we've ended up with a lot of this kind of thing all over, though:
// wc *temp = s.exportcopy(); use(temp); free(temp);
//




// note this basic structure started as a copy of pool, see that if needing similar functionality
struct string {

    wc *list; // dont use this as a typical string (not null terminated!)
    int count;
    int alloc;

    void realloc_mem(int amt) {
        if (amt > alloc) {
            alloc = amt;
            if (!list) { list = (wc*)malloc(alloc * sizeof(wc)); assert(list); }
            else { list = (wc*)realloc(list, alloc * sizeof(wc)); assert(list); }
        }
    }

    void add(wc newItem) {
        if (count >= alloc) {
            if (!list) { alloc = 32; list = (wc*)malloc(alloc * sizeof(wc)); assert(list); }
            else { alloc *= 2; list = (wc*)realloc(list, alloc * sizeof(wc)); assert(list); }
        }
        list[count++] = newItem;
    }

    void insert(wc newChar, int atIndex) {
        assert(atIndex >= 0 && atIndex <= count); //<=count since we allow adding to end
        add(newChar); // placeholder character, also note count now is +1 from original
        // move all chars back until spot
        for (int i = count; i > atIndex; i--) {
            list[i] = list[i-1];
        }
        list[atIndex] = newChar;
    }

    void del_at(int atIndex) {
        assert(atIndex >= 0 && atIndex < count);
        // move all chars forward starting at spot
        for (int i = atIndex; i < count-1; i++) {
            list[i] = list[i+1];
        }
        count--;
    }

    // if not found, return end of string
    int find_first(wc c) {
        int result = 0;
        for (int i = 0; i < count; i++) {
            if (list[i] == c) { return i; }
        }
        return count-1;
    }
    // if not found, return start of string
    int find_last(wc c) {
        int result = 0;
        for (int i = 0; i < count; i++) {
            if (list[i] == c) result = i;
        }
        return result;
    }

    string append(wc newchar) { add(newchar); return *this; }
    string append(wc *suffix) { for (wc *c=suffix;*c;c++) add(*c); return *this; }
    string append(string suffix) { for (int i=0;i<suffix.count;i++) add(suffix[i]); return *this; }

    void rtrim(int amt) { assert(count>=amt && amt>=0); count-=amt; }
    void ltrim(int amt) { assert(count>=amt && amt>=0);
        int newlength = count-amt;
        for (int i = 0; i < newlength; i++) {
            list[i] = list[i+amt];
        }
        count = newlength;
    }

    bool ends_with(string suffix) {
        if (suffix.count > count) return false;
        int j = count-1;
        for (int i = suffix.count-1; i >= 0; i--) {
            assert(j>=0); // shouldn't trigger since we check that suffix.count>count
            if (list[j] != suffix.list[i]) return false;
            j--;
        }
        return true;
    }
    bool ends_with(wc *suffix) {
        int suffixcount = wcslen(suffix);
        if (suffixcount > count) return false;
        int j = count-1;
        for (int i = suffixcount-1; i >= 0; i--) {
            assert(j>=0); // shouldn't trigger since we check that suffix.count>count
            if (list[j] != suffix[i]) return false;
            j--;
        }
        return true;
    }

    void find_replace(wc find, wc replace) {
        for (int i = 0; i < count; i++) {
            if (list[i] == find) list[i] = replace;
        }
    }

    string trim_common_prefix(string prefix) {
        int common_char_count = 0;
        for (int i = 0; i < prefix.count; i++) {
            if (prefix[i] != list[i]) {
                break;
            }
            common_char_count = i+1;
        }
        ltrim(common_char_count);
        return *this;
    }

    string overwrite_with_copy_of(string o) {
        empty_out();
        if (alloc < o.count) { realloc_mem(o.count); }
        memcpy(list, o.list, o.count*sizeof(list[0]));
        count = o.count;
        return *this;
    }

    string copy_into_new_memory(char *f, int l) {
        string copy = string::allocate_new(alloc, f, l);
        copy.count = count;
        memcpy(copy.list, list, count*sizeof(list[0]));
        return copy;
    }

    string copy_and_append(wc *suffix, char *f, int l) {
        string c = copy_into_new_memory(f, l);
        c.append(suffix);
        return c;
    }
    string copy_and_append(string suffix, char *f, int l) {
        string c = copy_into_new_memory(f, l);
        c.append(suffix);
        return c;
    }

    //
    // conversions / exports

    // "final" meaning our internal structure is broken, no more changes should be made (todo: enforce this with a bool)
    // (consider: var to indicate if null terminated and remove/add as needed?)
    wc *to_wc_final() {
        if (list && count>0) { if (list[count-1]==L'\0') return list; } // already has it
        append(L'\0');
        return list;
    }

    //
    // export to new memory (caller has to free)

    wc *to_wc_new_memory(char *f, int l) {
        int bytes_needed_with_null = (count+1)*sizeof(wc);
        // wc *result = (wc*)debugmalloc(bytes_needed_with_null, f, l);
        wc *result = (wc*)malloc(bytes_needed_with_null);
        assert(result);
        memcpy(result, list, bytes_needed_with_null-2); // list doesn't have \0 (-2 for that)
        result[count] = L'\0';
        return result;
    }
    char *to_utf8_new_memory() {
        // method without null terminator
        int bytes_needed_with_null = WideCharToMultiByte(CP_UTF8,0,  list,count,  0,0,  0,0) +1; // need +1 for null terminator if passing count instead of -1
        char *utf8 = (char*)malloc(bytes_needed_with_null);
        // assert(bytes_needed_with_null <= REUSABLE_MEM_BYTES); // not needed when allocating our own
        WideCharToMultiByte(CP_UTF8,0,  list,count,  utf8,bytes_needed_with_null,  0,0);
        utf8[bytes_needed_with_null-1] = 0; // add null terminator, it's not there by default if we pass in count instead of -1
        return utf8;
    }
    char *to_ascii_new_memory() {
        // todo: just use utf8 for now and worry about non-ascii chars later
        return to_utf8_new_memory();
    }

    //
    // static

    static string allocate_new(int amount, char *f, int l) {
        string str = {0};
        str.alloc = amount;
        // str.list = (wc*)debugmalloc(str.alloc * sizeof(wc), f, l);
        str.list = (wc*)malloc(str.alloc * sizeof(wc));
        assert(str.list);
        return str;
    }

    static string create_with_new_memory(wc *instring, char *f, int l) {
        int len = wcslen(instring);
        string result = string::allocate_new(len, f, l);
        result.count = len;
        memcpy(result.list, instring, len*sizeof(instring[0]));
        return result;
    }
    static string create_with_new_memory(char *source) {
        string result = {0};
        result.alloc = MultiByteToWideChar(CP_UTF8,0,  source,-1,  0,0);
        result.list = (wc*)malloc(result.alloc * sizeof(wc));
        MultiByteToWideChar(CP_UTF8,0,  source,-1,  result.list,result.alloc*sizeof(char));
        result.count = result.alloc-1; // no null terminator in actual result list
        return result;
    }

    static string create_using_passed_in_memory(wc *instring) {
        int len = wcslen(instring);
        string result;
        result.alloc = len + 1; // wcslen doesn't include null terminator which technically have memory for if we want to use it
        result.count = len;
        result.list = instring;
        return result;
    }

    //
    // misc / operators

    void free_all() { if (list) {assert(alloc>0);free(list);} list=0; count=0; alloc=0; }
    void empty_out() { count = 0; } /*note we keep the allocated memory*/ /*should call it .drain()*/
    bool is_empty() { return count==0; }
    wc& operator[] (int i) { assert(i>=0); assert(i<count); return list[i]; }
    bool operator==(string o) {
        if (count != o.count) return false;
        for (int i = 0; i < count; i++) {
            if(list[i] != o.list[i]) return false;
        }
        return true;
    }
    bool operator!=(string o) { return !(*this==o); }
    static string new_empty() { string new_blank = {0}; return new_blank;  }
};




// returns new string with the 3 paths strings combined into one (with joining slashes where needed)
// could use a simple path_combine(1,2) and call twice instead (needed anywhere other than creating thumbpaths?)
//
// should work whether or not paths have trailing or leading slashes (double slashes are not checked for atm)
// eg "E:/test folder" or "E:/test folder\" or "/~thumbs/" or "thumbs" or "/paintings/1.jpg" or "paintings/1.jpg"
string CombinePathsIntoNewMemory(string masterdir, string subdir_name, string subpath) {
    string result = masterdir.copy_into_new_memory(__FILE__, __LINE__);
    if (!result.ends_with(L"\\") && !result.ends_with(L"/") && subdir_name[0] != L'\\' && subdir_name[0] != L'/') {
        result.append(L'\\');
    }
    result.append(subdir_name);
    if (!result.ends_with(L"\\") && !result.ends_with(L"/") && subpath[0] != L'\\' && subpath[0] != L'/') {
        result.append(L'\\');
    }
    result.append(subpath);
    return result;
}

// todo: combine with above
string CombinePathsIntoNewMemory(string base, string tail) {
    string result = base.copy_into_new_memory(__FILE__, __LINE__);
    if (!result.ends_with(L"\\") && !result.ends_with(L"/") && tail[0] != L'\\' && tail[0] != L'/') {
        result.append(L'\\');
    }
    result.append(tail);
    return result;
}


// should treat all / and \ as equivalent and ignoring case
bool PathsAreSame(string path1, string path2) {
    // feels like there should be an easier way..
    // like an OS call, or a more general string parsing or find replace function to make?
    // i guess this isn't too bad

    // technically we could consider the paths "\" and "" the same... but i don't think we'd ever be checking those
    if ((path1.count == 0 && path2.count != 0) ||
        (path2.count == 0 && path1.count != 0)
    ) {
        return false;
    }

    string copy1 = path1.copy_into_new_memory(__FILE__, __LINE__);
    string copy2 = path2.copy_into_new_memory(__FILE__, __LINE__);

    if (copy1.ends_with(L"\\") || copy1.ends_with(L"/")) copy1.rtrim(1);
    if (copy2.ends_with(L"\\") || copy2.ends_with(L"/")) copy2.rtrim(1);

    // also check front of strings, in case of checking subpaths
    // don't really need this if we also trim leading slashes (below)
    if (copy1[0] == L'/') copy1[0] = L'\\';
    if (copy2[0] == L'/') copy2[0] = L'\\';

    // should we match "/example/file.txt" and "example/file.txt"? I guess so
    if (copy1[0] == L'/' || copy1[0] == L'\\') copy1.ltrim(1);
    if (copy2[0] == L'/' || copy2[0] == L'\\') copy2.ltrim(1);

    // basically a copy of string equals but ignoring slashes (and case for windows)
    if (copy1.count != copy2.count) {
        copy1.free_all();
        copy2.free_all();
        return false;
    }
    for (int i = 0; i < copy1.count; i++) {
        // don't flag mismatched slashes, just keep on looking at the rest of the string
        if ((copy1[i] == L'\\' || copy1[i] == L'/') &&
            (copy2[i] != L'\\' || copy2[i] != L'/'))
        {
            continue;
        }
        // note tolower for windows paths (not case sensitive)
        if (towlower(copy1[i]) != towlower(copy2[i])) {
            copy1.free_all();
            copy2.free_all();
            return false;
        }
    }
    copy1.free_all();
    copy2.free_all();
    return true;
}



// todo: made a bunch of these and now not using them,
// remove what we dont need

// basically rtrim_by_index(index) [todo: move to class?]
void trim_index_and_everything_after(string *path, int index) {
    path->rtrim(path->count - index);
}

// return index in path of the first slash \ or / in the string
// if no slash found, should return last index of string
int find_first_slash(string path) {
    int first_bslash = path.find_first(L'\\');
    int first_fslash = path.find_first(L'/');
    return min(first_bslash, first_fslash);
}
// return index in path of the last slash \ or / in the string
// if no slash found, should return 0
int find_last_slash(string path) {
    int last_bslash = path.find_last(L'\\');
    int last_fslash = path.find_last(L'/');
    return max(last_bslash, last_fslash);
}

void trim_first_slash_and_everything_after(string *path) {
    int first_slash_index = find_first_slash(*path);
    trim_index_and_everything_after(path, first_slash_index);
}

// for finding filenames from paths
void trim_last_slash_and_everything_before(string *path) {
    int last_slash_index = find_last_slash(*path);
    path->ltrim(last_slash_index+1); //+1 include L trimming the slash
}
void trim_last_slash_and_everything_after(string *path) {
    int last_slash_index = find_last_slash(*path);
    trim_index_and_everything_after(path, last_slash_index);
}


// edits memory of string passed in (no new memory)
string strip_to_just_parent_directory(string *path) {

    // remove any trailing slash
    if (path->ends_with(L"\\") || path->ends_with(L"/")) {
        path->rtrim(1);
    }

    // remove filename (including slash)
    trim_last_slash_and_everything_after(path);

    // remove parent path, left with just the original parent directory name
    trim_last_slash_and_everything_before(path);

    return *path;
}



int num_common_starting_chars(string s1, char *s2) {
    if (s1.count==0 || !s2) return 0;
    int s2count = strlen(s2);
    int common_chars = 0;
    for (int i = 0; i<s2count && i<s1.count; i++) {
        wc widechar;
        mbstowcs(&widechar, &(s2[i]), 1);
        if (s1[common_chars] != widechar) break;
        common_chars++;
    }
    return common_chars;
}


// todo: make this a class method?
void string_remove_all_chars_except(string str, string ok_chars) {

    for (int i = 0; i < str.count; i++) {
        bool char_is_ok = false;
        for (int j = 0; j < ok_chars.count; j++) {
            if (str[i] == ok_chars[j]) {
                char_is_ok = true;
                break;
            }
        }
        if (!char_is_ok) {
            str[i] = L'_';
        }
    }

    // //todo: operate without new allocation?
    // string tempcopy = str.copy_into_new_memory(__FILE__, __LINE__);

    // for (int i = 0; i < tempcopy.count; i++) {  // check copy...
    //     bool char_is_ok = false;
    //     for (int j = 0; j < ok_chars.count; j++) {
    //         if (tempcopy[i] == ok_chars[j]) {  // check copy...
    //             char_is_ok = true;
    //             break;
    //         }
    //     }
    //     if (!char_is_ok) {
    //         str.del_at(i); // ...but delete from original (this precaution needed or not?)
    //     }
    // }
    // tempcopy.free_all();
}


//
// generic wc stuff


bool wc_equals(wc *s1, wc *s2) {
    int result = wcscmp(s1, s2);
    return result == 0;
}


// i think basically just for escaping characters for ffmpeg command line calls
void CopyStringWithCharsEscaped(wc *outbuffer, int outsize, wc *instring, wc char_to_escape, wc escape_char) {
    int count = 0;
    for (wc *c = instring; *c; c++) {
        if (*c == char_to_escape)
            count++;
    }

    int newsize = wcslen(instring) + count + 1; // wcslen doesn't include null terminator
    assert(newsize <= outsize);
    wc *dest = outbuffer;
    wc *source = instring;

    while (*source) {
        if (*source == char_to_escape) {
            *dest = escape_char;
            dest++;
        }
        *dest = *source;
        dest++;
        source++;
    }
    *dest = 0;
    assert(outbuffer+newsize-1 == dest);
}


//
// generic char stuff

bool str_equals(char *s1, char *s2) {
    return strcmp(s1, s2) == 0;
}

