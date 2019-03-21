






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

void md_record_alloc(md_allocation r) {
    if (md_r_count >= md_r_alloc) {
        if (!md_records) {
            md_r_alloc = 32;
            md_records = (md_allocation*)malloc(md_r_alloc * sizeof(md_allocation));
            assert(md_records);
        }
        else {
            md_r_alloc *= 2;
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
            return;
        }
    }
    // assert(false); // free something we haven't alloc'd? (i would assume crash before this point)
}

void *memdebug_malloc(uint size, char *file, uint line)
{
    void *result = malloc(size);
    md_record_alloc({result, size, file, line});
    return result;
}

void memdebug_free(void *ptr, char *file, uint line)
{
    if (ptr) { // ignore free(null) calls for now
        free(ptr);
        md_record_free({ptr});
    }
}

void memdebug_reset()
{
    md_r_count = 0;
}

void memdebug_print()
{
    OutputDebugString("--unfree'd memory allocations--\n");
    for (int i = 0; i < md_r_count; i++) {
        if (!md_records[i].free) {
            char buf[256];
            sprintf(buf, "%s (line %u), %u bytes \n", md_records[i].file, md_records[i].line, md_records[i].size);
            OutputDebugString(buf);
        }
    }
}


#define malloc(n) memdebug_malloc(n, __FILE__, __LINE__)
#define free(n) memdebug_free(n, __FILE__, __LINE__)


// over allocate all alloc, add magic number to end
// then check all those tags and make sure they are intact

// can tag an alloc'd pointer with a comment



