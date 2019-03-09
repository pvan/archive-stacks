
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
    static thetype##_pool empty() { thetype##_pool new_blank = {0}; return new_blank;  } \
};

DEFINE_TYPE_POOL(int);
DEFINE_TYPE_POOL(v2);
DEFINE_TYPE_POOL(string);


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



