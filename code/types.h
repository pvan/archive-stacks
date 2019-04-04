

#include <stdint.h>


#define uint unsigned int

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define i8  int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t

#define int8  int8_t
#define int16 int16_t
#define int32 int32_t
#define int64 int64_t


#define NBSP '\u00A0'


#define wchar wchar_t
#define wc wchar_t



u32 rand_col() {
    u32 col = rand() & 0xff;
    col |= (rand() & 0xff) << 8;
    col |= (rand() & 0xff) << 16;
    col |= (rand() & 0xff) << 24;
    return col;
}

u32 rand_col(uint seed) {
    srand(seed);
    u32 col = rand() & 0xff;
    col |= (rand() & 0xff) << 8;
    col |= (rand() & 0xff) << 16;
    col |= (rand() & 0xff) << 24;
    return col;
}



