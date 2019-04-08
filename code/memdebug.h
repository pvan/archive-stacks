






struct md_allocation {
    void *ptr;
    uint size;
    char *file;
    uint line;
    bool free;
};

md_allocation *md_records;
uint md_r_count;
uint md_r_alloc;

int total_alloc = 0;

void md_record_alloc(md_allocation r) {
    if (md_r_count >= md_r_alloc) {
        if (!md_records) {
            // md_r_alloc = 32;
            md_r_alloc = 1<<16;
            md_records = (md_allocation*)malloc(md_r_alloc * sizeof(md_allocation));
            assert(md_records);
        }
        else {
            md_r_alloc *= 2;
            // this realloc throws exception sometimes?
            // larger starting allocation (1<<16) set above seems to help with realloc crashing here?
            // todo: pull out gflags to debug this
            md_records = (md_allocation*)realloc(md_records, md_r_alloc * sizeof(md_allocation));
            assert(md_records);
        }
    }
    md_records[md_r_count++] = r;
}
void md_record_free(void *ptr) {
    // find the latest matching allocation
    for (int i = md_r_count-1; i >= 0; i--) {
        if (md_records[i].ptr == ptr) {
            md_records[i].free = true;
            total_alloc -= md_records[i].size;
            return;
        }
    }
    // assert(false); // free something we haven't alloc'd? (i would assume crash before this point)
}

void *memdebug_malloc(uint size, char *file, uint line)
{
    void *result = malloc(size);
// if (strstr(file,"string.cpp")) return result;
   md_record_alloc({result, size, file, line});
    total_alloc += size;
    return result;
}

void memdebug_free(void *ptr, char *file, uint line)
{
    if (ptr) { // ignore free(null) calls for now
        free(ptr);
// if (strstr(file,"string.cpp")) return;
        md_record_free({ptr});
    }
}

void *memdebug_realloc(void *ptr, uint size, char *file, uint line) {
    md_record_free({ptr});
    ptr = realloc(ptr, size);
    md_record_alloc({ptr, size, file, line});
    return ptr;
}

void memdebug_reset()
{
    md_r_count = 0;
    total_alloc = 0;
}

void memdebug_print()
{
    char buf[256];
    sprintf(buf, "leak since last reset: %i\n", total_alloc);
    OutputDebugString(buf);

    OutputDebugString("--unfree'd memory allocations--\n");
    for (int i = 0; i < md_r_count; i++) {
        if (!md_records[i].free) {
            char buf[256];
            // if (strstr(md_records[i].file, "string") != 0) {
            //     // int counthack = (int)(((char*)md_records[i].ptr) + sizeof(wc*)); // pull count out of string struct
            //     void *address_of_count = (char*)md_records[i].ptr + sizeof(wc*);
            //     int count = *(int*)address_of_count; // super sketch because count could be changed by the time we're here
            //     sprintf(buf, "%s (line %u), %u bytes : %.*ls \n", md_records[i].file, md_records[i].line, md_records[i].size, count, (wc*)md_records[i].ptr);
            // } else {
                sprintf(buf, "%s (line %u), %u bytes \n", md_records[i].file, md_records[i].line, md_records[i].size);
            // }
            OutputDebugString(buf);
        }
    }
}


#define malloc(n) memdebug_malloc(n, __FILE__, __LINE__)
#define realloc(n,m) memdebug_realloc(n,m, __FILE__, __LINE__)
#define free(n) memdebug_free(n, __FILE__, __LINE__)


// over allocate all alloc, add magic number to end
// then check all those tags and make sure they are intact

// can tag an alloc'd pointer with a comment



