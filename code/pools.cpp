


// todo:
// need to restructure this a bit
// need more than just one kind of collection
// collections could be...
// 1. ordered or un-ordered (say, "list" and "pool" -- the only reason to un-order is easy removal)
// 2. allow duplicates or not (i think we don't care about dups atm.. allow them in all cases)
// 3. copy by ref or value when adding items? (generic way to handle both?)
// definitely need both kinds of option 1 and both of option 3 couldn't hurt
//
// at the very least, as-is, it's going to be easy to make use-after-free errors (see 3.)
// and accidently re-order when we didn't want to (see 1.)


// can have duplicates
// we call it a "pool" since things will get re-ordered when removing
#define DEFINE_TYPE_POOL(thetype) struct thetype##_pool \
{ \
    thetype *pool; \
    int count; \
    int alloc; \
    int index_of(thetype possiblyExistingItem) { \
        for (int i = 0; i < count; i++) { \
            if (pool[i] == possiblyExistingItem) return i; \
        } \
        return -1; /*better way to error?*/ \
    } \
    bool has(thetype possiblyExistingItem) { \
        for (int i = 0; i < count; i++) { \
            if (pool[i] == possiblyExistingItem) return true; \
        } \
        return false; \
    } \
    void add(thetype newItem) { \
        if (count >= alloc) { \
            if (!pool) { alloc = 32; pool = (thetype*)malloc(alloc * sizeof(thetype)); assert(pool); } \
            else { alloc *= 2; pool = (thetype*)realloc(pool, alloc * sizeof(thetype)); assert(pool); } \
        } \
        /* if (!has(newItem)) {*/ \
            pool[count++] = newItem; \
        /* } */ \
    } \
    void remove(thetype existingItem) { \
        for (int i = 0; i < count; i++) { \
            if (pool[i] == existingItem) { \
                pool[i] = pool[count-1]; \
                count--; \
                return; \
            } \
        } \
        OutputDebugString("WARNING: trying to remove an item from a(n) thetype##_list that doesn't have it\n"); \
    } \
    void empty_out() { count = 0; } /*note we keep the allocated memory*/ /*should call it .drain()*/ \
    bool is_empty() { return count==0; } \
    thetype& operator[] (int i) { assert(i>=0); assert(i<count); return pool[i]; } \
    static thetype##_pool new_empty() { thetype##_pool new_blank = {0}; return new_blank;  } /*todo: probably should always create empty pools like this. rename new_empty()? */ \
};

DEFINE_TYPE_POOL(int);
DEFINE_TYPE_POOL(v2);
DEFINE_TYPE_POOL(string);
DEFINE_TYPE_POOL(bool);
DEFINE_TYPE_POOL(rect);
DEFINE_TYPE_POOL(float);

// add generic sort to pool struct? (would require > operator on all pool types)
void sort_float_pool(float_pool *floats) {
    int i, j;
    float key;
    for (i = 1; i < floats->count; i++) {
        key = floats->pool[i];
        j = i - 1;

        // move forward all and only the elements > key
        while (j >= 0 && floats->pool[j] > key) {
            floats->pool[j + 1] = floats->pool[j];
            j = j - 1;
        }
        floats->pool[j + 1] = key;
    }
}
void sort_int_pool(int_pool *ints) {
    int i, key, j;
    for (i = 1; i < ints->count; i++) {
        key = ints->pool[i];
        j = i - 1;

        // move forward all and only the elements > key
        while (j >= 0 && ints->pool[j] > key) {
            ints->pool[j + 1] = ints->pool[j];
            j = j - 1;
        }
        ints->pool[j + 1] = key;
    }
}
// kind of a terrible name
// sort the second list (floats) and keep first (ints) in same re-ordering
void sort_int_pool_by_float_pool(int_pool *indices, float_pool *floats) {
    int i, j, ikey;
    float key;
    for (i = 1; i < floats->count; i++) {
        key = floats->pool[i];
        ikey = indices->pool[i];
        j = i - 1;

        // move forward all and only the elements > key
        while (j >= 0 && floats->pool[j] > key) {
            floats->pool[j + 1] = floats->pool[j];
            indices->pool[j + 1] = indices->pool[j];
            j = j - 1;
        }
        floats->pool[j + 1] = key;
        indices->pool[j + 1] = ikey;
    }
}


// todo: are we using this?
// queue, can only push/pop items to end
#define DEFINE_TYPE_QUEUE(thetype) struct thetype##_queue \
{ \
    thetype *items; \
    int count; \
    int alloc; \
    bool has(thetype possiblyExistingItem) { \
        for (int i = 0; i < count; i++) { \
            if (items[i] == possiblyExistingItem) return true; \
        } \
        return false; \
    } \
    void push(thetype newItem) { \
        if (count >= alloc) { \
            if (!items) { alloc = 32; items = (thetype*)malloc(alloc * sizeof(thetype)); assert(items); } \
            else { alloc *= 2; items = (thetype*)realloc(items, alloc * sizeof(thetype)); assert(items); } \
        } \
        items[count++] = newItem; \
    } \
    thetype pop() { \
        if (count > 0) { return items[--count]; } \
        else { \
            OutputDebugString("WARNING: trying to pop from an empty queue\n"); \
            return 0; \
        } \
    } \
    void empty_out() { count = 0; } /*note we keep the allocated memory*/ /*should call it .drain()*/ \
    bool is_empty() { return count==0; } \
    thetype& operator[] (int i) { assert(i>=0); assert(i<count); return items[i]; } \
    static thetype##_queue new_empty() { thetype##_queue new_blank = {0}; return new_blank;  } \
};

DEFINE_TYPE_QUEUE(int);


