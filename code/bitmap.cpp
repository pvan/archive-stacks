



struct bitmap
{
    u32 *data;
    int w;
    int h;

    bitmap NewCopy() {
        bitmap newBitmap;
        newBitmap.data = (u32*)malloc(w*h*sizeof(u32));
        memcpy(newBitmap.data, data, w*h*sizeof(u32));
        newBitmap.w = w;
        newBitmap.h = h;
        return newBitmap;
    }

    static bitmap AllocateNew(int nw, int nh) {
        bitmap newBitmap;
        newBitmap.data = (u32*)malloc(nw*nh*sizeof(u32));
        newBitmap.w = nw;
        newBitmap.h = nh;
        return newBitmap;
    }
};


