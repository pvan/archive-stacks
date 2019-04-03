



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

    wc *list;
    int count;
    int alloc;

    void add(wc newItem) {
        if (count >= alloc) {
            if (!list) { alloc = 32; list = (wc*)malloc(alloc * sizeof(wc)); assert(list); }
            else { alloc *= 2; list = (wc*)realloc(list, alloc * sizeof(wc)); assert(list); }
        }
        list[count++] = newItem;
    }

    static newstring allocate_new(int amount) {
        newstring str = {0};
        str.alloc = amount;
        str.list = (wc*)malloc(str.alloc * sizeof(wc)); assert(str.list);
        return str;
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

    void append(wc newchar) { add(newchar); }
    // void append(char newchar) { add((wc)newchar); }
    // void append(char *c) { for (;*c;c++) add((wc)c); }

    void rtrim(int amt) { assert(count>=amt && amt>=0); count-=amt; }

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

    newstring copy_into_new_memory() {
        newstring copy = newstring::allocate_new(alloc);
        copy.count = count;
        memcpy(copy.list, list, count*sizeof(list[0]));
        return copy;
    }

    void free_all() { if (list) free(list); }
    void empty_out() { count = 0; } /*note we keep the allocated memory*/ /*should call it .drain()*/
    bool is_empty() { return count==0; }
    wc& operator[] (int i) { assert(i>=0); assert(i<count); return list[i]; }
    static newstring new_empty() { newstring new_blank = {0}; return new_blank;  }
};




