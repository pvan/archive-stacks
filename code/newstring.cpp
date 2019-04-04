



//
// new try at a string lib
// basically start with a stretchy list
// and add whatever string manipulation functions we need
//
// storing everything internally as wchar for now since we're only on windows anyway
// but need utf8 conversions to interface with some libs
//
// will probably also need some re-usable memory buffers for comparisons or one-off conversions
//



// todo: consider using pointers and malloc instead of fixed size?
const int REUSABLE_MEM_BYTES = 1024;
char string2_reusable_mem1[REUSABLE_MEM_BYTES];
char string2_reusable_mem2[REUSABLE_MEM_BYTES];
bool string2_reusable_toggle = false; // which reusable mem should we use next?

char *next_open_reusable_mem() {
    return string2_reusable_toggle ? (char*)&string2_reusable_mem1 : (char*)&string2_reusable_mem2;
    string2_reusable_toggle = !string2_reusable_toggle;
}


// note this basic structure started as a copy of pool, see that if needing similar functionality
struct newstring {

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

    newstring append(wc newchar) { add(newchar); return *this; }
    newstring append(wc *newchar) { for (wc *c=newchar;*c;c++) add(*c); return *this; }

    void rtrim(int amt) { assert(count>=amt && amt>=0); count-=amt; }
    void ltrim(int amt) { assert(count>=amt && amt>=0);
        int newlength = count-amt;
        for (int i = 0; i < newlength; i++) {
            list[i] = list[i+amt];
        }
        count = newlength;
    }

    bool ends_with(newstring o) {
        if (o.count > count) return false;
        for (int i = o.count-1; i >= 0; i--) {
            if (list[i] != o.list[i]) return false;
        }
        return true;
    }

    newstring trim_common_prefix(newstring prefix) {
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

    newstring overwrite_with_copy_of(newstring o) {
        empty_out();
        if (alloc < o.count) { realloc_mem(o.count); }
        memcpy(list, o.list, o.count*sizeof(list[0]));
        count = o.count;
        return *this;
    }

    newstring copy_into_new_memory() {
        newstring copy = newstring::allocate_new(alloc);
        copy.count = count;
        memcpy(copy.list, list, count*sizeof(list[0]));
        return copy;
    }

    newstring copy_and_append(wc *suffix) {
        newstring c = copy_into_new_memory();
        c.append(suffix);
        // c.count += wcslen(suffix);
        // c.list = (wchar_t*)realloc(c.chars, (c.length+1)*sizeof(wchar_t));
        // wcscat(c.chars, suffix);
        return c;
    }

    // delete this when done with old system
    string to_old_string_temp() {
        string result = string::CreateWithNewMem(to_wc_reusable());
        return result;
    }

    //
    // conversions / exports

    // "final" = now should no longer be changed (todo: enforce this with a bool)
    // (consider: var to indicate if null terminated and remove/add as needed?)
    wc *to_wc_final() {
        append(L'\0');
        return list;
    }
    wc *to_wc_reusable() {
        // method without needing to temporarily add \0 to our orig string
        int bytes_needed_without_null = count*sizeof(wc);
        wc *result = (wc*)next_open_reusable_mem();
        assert(bytes_needed_without_null+2 <= REUSABLE_MEM_BYTES); // +2 bytes for size of L'\0'
        memcpy(result, list, bytes_needed_without_null); // no \0 yet
        result[count] = L'\0';
        return result;
    }

    char *to_utf8_reusable() {
        // method without null terminator
        int bytes_needed_with_null = WideCharToMultiByte(CP_UTF8,0,  list,count,  0,0,  0,0) +1; // need +1 for null terminator if passing count instead of -1
        char *utf8 = next_open_reusable_mem();
        assert(bytes_needed_with_null <= REUSABLE_MEM_BYTES);
        WideCharToMultiByte(CP_UTF8,0,  list,count,  utf8,bytes_needed_with_null,  0,0);
        utf8[bytes_needed_with_null-1] = 0; // add null terminator, it's not there by default if we pass in count instead of -1
        return utf8;
    }
    char *to_ascii_reusuable() {
        // todo: just use utf8 for now and worry about non-ascii chars later
        return to_utf8_reusable();
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

    static newstring allocate_new(int amount) {
        newstring str = {0};
        str.alloc = amount;
        str.list = (wc*)malloc(str.alloc * sizeof(wc)); assert(str.list);
        return str;
    }

    static newstring create_with_new_memory(wc *instring) {
        int len = wcslen(instring);
        newstring result = newstring::allocate_new(len);
        result.count = len;
        memcpy(result.list, instring, len*sizeof(instring[0]));
        return result;
    }

    //
    // misc / operators

    void free_all() { if (list) free(list); list=0; }
    void empty_out() { count = 0; } /*note we keep the allocated memory*/ /*should call it .drain()*/
    bool is_empty() { return count==0; }
    wc& operator[] (int i) { assert(i>=0); assert(i<count); return list[i]; }
    bool operator==(newstring o) {
        if (count != o.count) return false;
        for (int i = 0; i < count; i++) {
            if( list[i] != o.list[i]) return false;
        }
        return true;
    }
    bool operator!=(newstring o) { return !(*this==o); }
    static newstring new_empty() { newstring new_blank = {0}; return new_blank;  }
};






