
// can have duplicates
// we call it a "pool" since things will get re-ordered when removing
#define DEFINE_TYPE_POOL(thetype) struct thetype##_pool \
{ \
    thetype *pool; \
    int count; \
    int alloc; \
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

DEFINE_TYPE_POOL(v2);
DEFINE_TYPE_POOL(string);

