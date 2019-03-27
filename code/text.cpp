

#define STB_TRUETYPE_IMPLEMENTATION
#include "../lib/stb_truetype.h"


// new api design
// tf_

// seeing what it's like without abstracting the gpu data

// trying to keep: the lower level the api, the more functional
// (less state, less memory allocs, less side effects)


//
// state and inits...

// right now just support one font
// to change we could make a font struct with this data
// and pass it in to the render functions

u8 *tf_file_buffer = 0;  // opened font file

stbtt_fontinfo tf_font;  // font info loaded by stb

stbtt_bakedchar tf_bakedchars[96]; // ASCII 32..126 is 95 glyphs

bitmap tf_fontatlas; // bitmap with our baked chars onto it
gpu_texture_id tf_fonttexture; // texture id of our font atlas bitmap (sent to gpu on creation)

// let stb open and parse font
void tf_openfont() // could pass in path
{
    tf_file_buffer = (u8*)malloc(1<<20);
    fread(tf_file_buffer, 1, 1<<20, fopen("c:/windows/fonts/segoeui.ttf", "rb"));
    // fread(ttf_file_buffer, 1, 1<<20, fopen("c:/windows/fonts/arial.ttf", "rb"));

    if (!stbtt_InitFont(&tf_font, tf_file_buffer, 0)) {
        MessageBox(0, "stbtt init font failed", 0,0);
    }
    // free(ttf_file_buffer);  // looks like we need to keep this around?
}

// return newly-allocated bitmap with baked chars on it
// i'm ok just allocating this mem because it's kind of a one-time thing
// (whereas we let the caller allocate mem for the rendering calls)
bitmap tf_bakefont(float pixel_height)
{
    assert(tf_file_buffer);

    int first_char = 32;
    int num_chars = 96;

    int bitmapW = 512;
    int bitmapH = 512;
    u8 *gray_bitmap = (u8*)malloc(bitmapW * bitmapH);
    ZeroMemory(gray_bitmap, bitmapW * bitmapH);

    // no guarantee this fits, esp with variable pixel_height
    stbtt_BakeFontBitmap(tf_file_buffer, 0,
                         pixel_height,
                         gray_bitmap, bitmapW, bitmapH,
                         first_char, num_chars,
                         tf_bakedchars);

    // stb gives us a 1-channel grey-scale image
    // convert to full color bitmap here (or just add greyscale support to gpu api)
    u8 *color_bitmap = (u8*)malloc(bitmapW * bitmapH * 4);
    for (int px = 0; px < bitmapW; px++) {
        for (int py = 0; py < bitmapH; py++) {
            u8 *r = color_bitmap + ((py*bitmapW)+px)*4 + 0;
            u8 *g = color_bitmap + ((py*bitmapW)+px)*4 + 1;
            u8 *b = color_bitmap + ((py*bitmapW)+px)*4 + 2;
            u8 *a = color_bitmap + ((py*bitmapW)+px)*4 + 3;

            int sx = px;// + first_non_blank_column;
            *r = *(gray_bitmap + ((py*bitmapW)+sx));
            *g = *(gray_bitmap + ((py*bitmapW)+sx));
            *b = *(gray_bitmap + ((py*bitmapW)+sx));
            // *a = *(gray_bitmap + ((py*bitmapW)+sx));
            // if (bgA != 0) *a = bgA;
            // *a = 255;
            *a = 0;
        }
    }
    free(gray_bitmap);

    return {(u32*)color_bitmap, bitmapW, bitmapH};
}


//
// calls for rendering text...

// call to know how much memory to pass in to create_quad_list
int tf_how_many_quads_needed_for_text(char *text) {
    int result = 0;
    while (*text) {
        if (*text >= 32 && *text < 128) {
            result++;
        }
        text++;
    }
    return result;
}

// caller passes in quad memory to be filled in
// use tf_how_many_quads_needed_for_text to know how much memory to pass in
// return bounding box of all resulting quads
rect tf_create_quad_list_for_text_at_rect(char *text, float x, float y, gpu_quad *quadlist, int quadcount) {

    assert(quadlist && quadcount>0);

    // to find bb
    bool set_left_most_x = false;
    float left_most_x = 0;
    float right_most_x = 0;
    float largest_y = 0;
    float smallest_y = 10000;

    int quadcountsofar = 0;

    float tx = /*r.*/x; // i think these keep track of where next quad goes
    float ty = /*r.*/y; // (updated by stbtt_GetBakedQuad)
    while (*text) {
        if (*text >= 32 && *text < 128) {
            assert(quadcountsofar+1 <= quadcount); // just checking ourselves
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(tf_bakedchars, 512,512, *text-32, &tx,&ty,&q,1);//1=opengl & d3d10+,0=d3d9

            // hmm not sure if i like alpha and color piggybacking on the quad or not
            gpu_quad quad = {q.x0,q.y0,q.s0,q.t0, q.x1,q.y1,q.s1,q.t1};
            quad.alpha = 1;
            quad.color = 0xffffffff;
            quadlist[quadcountsofar++] = quad;

            // x bounding box
            if (!set_left_most_x) {
                set_left_most_x = true;
                left_most_x = q.x0;
            }
            right_most_x = q.x1;

            // y bounding box
            if (q.y0 > largest_y) largest_y = q.y0;
            if (q.y1 > largest_y) largest_y = q.y1;
            if (q.y0 < smallest_y) smallest_y = q.y0;
            if (q.y1 < smallest_y) smallest_y = q.y1;
        }
        text++;
    }
    rect bb;
    bb.x = left_most_x;
    bb.w = right_most_x-left_most_x;
    bb.y = smallest_y;
    bb.h = largest_y-smallest_y;

    return bb;
}




//
// for getting size of text before rendering it


// basically how far above the baseline a glyph can go
// used to positition drawn text by top edge
int tf_cached_largest_ascent = 0;

// so we know how big the text will ever get
int tf_cached_largest_descent = 0;

// the above are both stored as positive values, so this is basically them added together
int tf_cached_largest_total_height;

// ran once when baking and cached
int tf_largest_baked_ascent() {
    // float scale = stbtt_ScaleForPixelHeight(&ttfont, pixel_size);
    int smallest_y = 10000;
    for (int i = 0; i < 96; i++) {
        // yoff appears to be basically what the y0 from stbtt_GetBakedQuad becomes
        if (tf_bakedchars[i].yoff < smallest_y) smallest_y = tf_bakedchars[i].yoff;
    }
    return -smallest_y; // y0s are negative, return the farthest as a distance basically
}

// ran once when baking and cached
int tf_largest_baked_descent() {
    // float scale = stbtt_ScaleForPixelHeight(&ttfont, pixel_size);
    int largest_y = 0;
    for (int i = 0; i < 96; i++) {
        // we want what y1 is from stbtt_GetBakedQuad (basically it's yoff+h = yoff+y1-y0)
        float descent = tf_bakedchars[i].yoff + (tf_bakedchars[i].y1 - tf_bakedchars[i].y0);
        if (descent > largest_y) largest_y = descent;
    }
    return largest_y;
}

// returns bounding box of rendered text
// todo: or add optional "render" flag to tf_create_quad_list_for_text_at_rect(
// and call that with the flag to get the size?
rect tf_text_bounding_box(char *text, float screenX = 0, float screenY = 0) {

    // to find bb
    bool set_left_most_x = false;
    float left_most_x = 0;
    float right_most_x = 0;
    float largest_y = 0;
    float smallest_y = 10000;

    float tx = screenX;
    float ty = screenY;
    while (*text) {
        if (*text >= 32 && *text < 128) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(tf_bakedchars, 512,512, *text-32, &tx,&ty,&q,1);//1=opengl & d3d10+,0=d3d9

            // x bounding box
            if (!set_left_most_x) {
                set_left_most_x = true;
                left_most_x = q.x0;
            }
            right_most_x = q.x1;

            // y bounding box
            if (q.y0 > largest_y) largest_y = q.y0;
            if (q.y1 > largest_y) largest_y = q.y1;
            if (q.y0 < smallest_y) smallest_y = q.y0;
            if (q.y1 < smallest_y) smallest_y = q.y1;
        }
        text++;
    }
    rect bb;
    bb.x = left_most_x;
    bb.w = right_most_x-left_most_x +1; // fencepost issue here, eg 3-1=2 but we want w=3
    bb.y = smallest_y;
    bb.h = largest_y-smallest_y +1; // +1 here too, although i don't think this value is used anywhere atm to test

    return bb;
}




void tf_init(int fontsize) {
    tf_openfont();

    tf_fontatlas = tf_bakefont(fontsize);
    tf_fonttexture = gpu_create_texture();
    gpu_upload_texture(tf_fontatlas.data, tf_fontatlas.w, tf_fontatlas.h, tf_fonttexture);

    tf_cached_largest_ascent = tf_largest_baked_ascent();
    tf_cached_largest_descent = tf_largest_baked_descent();
    tf_cached_largest_total_height = tf_cached_largest_ascent + tf_cached_largest_descent;
}



//
//


// still useful in some cases (eg making thumbnails from text)
bitmap tf_create_bitmap(char *text, int fsize, float alpha, bool centerH, bool centerV, int bgA = 0)
{
    float bitmapWf = 0;
    float bitmapHf = 0;
    int fontH = fsize;
    float scale = stbtt_ScaleForPixelHeight(&tf_font, fontH);

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&tf_font, &ascent,&descent,&lineGap);
    ascent*=scale; descent*=scale; lineGap*=scale;

    float widthThisLine = 0;
    bitmapHf = fontH; // should be same as ascent-descent because of our scale factor
    for (int i = 0; i < strlen(text); i++)
    {
        if (text[i] == '\n')
        {
            widthThisLine = 0;
            bitmapHf += (fontH+lineGap);
        }
        else
        {
            // int x0,y0,x1,y1; stbtt_GetCodepointBitmapBox(&ttfont, text[i], scale,scale, &x0,&y0,&x1,&y1);
            int advance;    stbtt_GetCodepointHMetrics(&tf_font, text[i], &advance, 0);
            int kerning =   text[i+1] ? stbtt_GetCodepointKernAdvance(&tf_font, text[i], text[i+1]) : 0;
            widthThisLine += ((float)advance*scale + (float)kerning*scale);
            if (widthThisLine > bitmapWf) bitmapWf = widthThisLine;
        }
    }

    // width calc is still a little iffy
    int bitmapW = round(bitmapWf + 1);
    int bitmapH = round(bitmapHf + 1);
    // int bitmapW = ceil(bitmapWf);
    // int bitmapH = ceil(bitmapHf);

    bitmapW+=2; // compensate for starting 2 pixels in?
    // bitmapW+=2; // similar space at the end?

    u8 *gray_bitmap = (u8*)malloc(bitmapW * bitmapH);
    ZeroMemory(gray_bitmap, bitmapW * bitmapH);

    // TODO: the docs recommend we go char by char on a temp bitmap and alpha blend into the final
    // as the method we're using could cutoff chars that ovelap e.g. lj in arialbd

    int xpos = 2; // "leave a little padding in case the character extends left"
    int ypos = ascent; // not sure why we start at ascent actually..
    for (int i = 0; i < strlen(text); i++)
    {
        if (text[i] == '\n')
        {
            xpos = 0;
            ypos += (fontH+lineGap);
        }
        else
        {
            float x_shift = xpos - (float)floor(xpos);

            // int L,R,T,B;     stbtt_GetCodepointBitmapBox(&ttfont, text[i], scale,scale, &L,&T,&R,&B);
            // int x0,y0,x1,y1; stbtt_GetCodepointBitmapBox(&ttfont, text[i], scale,scale, &x0,&y0,&x1,&y1);
            int x0,y0,x1,y1; stbtt_GetCodepointBitmapBoxSubpixel(&tf_font, text[i], scale,scale,x_shift,0, &x0,&y0,&x1,&y1);
            int advance;     stbtt_GetCodepointHMetrics(&tf_font, text[i], &advance, 0);
            int kerning =    text[i+1] ? stbtt_GetCodepointKernAdvance(&tf_font, text[i], text[i+1]) : 0;

            int offset = ((ypos+y0)*bitmapW) + (int)xpos+x0;
            if (text[i] != ' ' && text[i] != NBSP)
                stbtt_MakeCodepointBitmapSubpixel(&tf_font, gray_bitmap+offset, x1-x0, y1-y0, bitmapW, scale,scale,x_shift,0, text[i]);

            xpos += ((float)advance*scale + (float)kerning*scale);
        }
    }

    // trim blank pixel columns
    bool found_first_non_blank = false;
    int first_non_blank_column = 0;
    int last_non_blank_column = 0;
    for (int px = 0; px < bitmapW; px++) {
        for (int py = 0; py < bitmapH; py++) {
            if (gray_bitmap[py*bitmapW + px] != 0) {
                if (!found_first_non_blank) { first_non_blank_column = px; found_first_non_blank = true; }
                last_non_blank_column = px;
                break; // next column
            }
        }
    }

    // for now leave a 2px border on each side
    first_non_blank_column -= 2; if (first_non_blank_column<0) first_non_blank_column = 0;
    last_non_blank_column += 2; if (last_non_blank_column>bitmapW) last_non_blank_column = bitmapW;

    int sourceW = bitmapW;
    bitmapW = (last_non_blank_column - first_non_blank_column) +1; // +1 to get correct width (width that includes both indices)

    // first_non_blank_column = 0;
    // bitmapW += 4; // 2 front 2 back

    u8 *color_temp_bitmap = (u8*)malloc(bitmapW * bitmapH * 4);
    for (int px = 0; px < bitmapW; px++)
    {
        for (int py = 0; py < bitmapH; py++)
        {
            u8 *r = color_temp_bitmap + ((py*bitmapW)+px)*4 + 0;
            u8 *g = color_temp_bitmap + ((py*bitmapW)+px)*4 + 1;
            u8 *b = color_temp_bitmap + ((py*bitmapW)+px)*4 + 2;
            u8 *a = color_temp_bitmap + ((py*bitmapW)+px)*4 + 3;

            int sx = px + first_non_blank_column;
            *r = *(gray_bitmap + ((py*sourceW)+sx));
            *g = *(gray_bitmap + ((py*sourceW)+sx));
            *b = *(gray_bitmap + ((py*sourceW)+sx));
            *a = *(gray_bitmap + ((py*sourceW)+sx));
            if (bgA != 0) *a = bgA;
            // *a = 255;
        }
    }
    free(gray_bitmap);
    return {(u32*)color_temp_bitmap, bitmapW, bitmapH};
}


// const int LIL_TEXT_SIZE = 24;

// char display_log_reuseable_mem[256];
// int display_log_count;
// opengl_quad display_log_quad;
// void HUDPRINT(char *s) {
//     bitmap img = tf_create_bitmap(s, LIL_TEXT_SIZE, 255, true, true, 200);
//     if (!display_log_quad.created) display_log_quad.create(0,0,img.w,img.h);
//     display_log_quad.set_texture(img.data, img.w, img.h);
//     display_log_quad.set_verts(0, display_log_count*(LIL_TEXT_SIZE-1), img.w, img.h);
//     display_log_quad.render();
//     free(img.data);
//     display_log_count++;
// }
// void HUDPRINT(u64 i)               { sprintf(display_log_reuseable_mem, "%lli", i); HUDPRINT(display_log_reuseable_mem); }
// void HUDPRINT(int i)               { sprintf(display_log_reuseable_mem, "%i", i); HUDPRINT(display_log_reuseable_mem); }
// void HUDPRINT(float f)             { sprintf(display_log_reuseable_mem, "%f", f); HUDPRINT(display_log_reuseable_mem); }
// void HUDPRINT(char *text, int i)   { sprintf(display_log_reuseable_mem, text, i); HUDPRINT(display_log_reuseable_mem); }
// void HUDPRINT(char *text, i64 i)   { sprintf(display_log_reuseable_mem, text, i); HUDPRINT(display_log_reuseable_mem); }
// void HUDPRINT(char *text, char *s) { sprintf(display_log_reuseable_mem, text, s); HUDPRINT(display_log_reuseable_mem); }
// void HUDPRINT(char *text, float f) { sprintf(display_log_reuseable_mem, text, f); HUDPRINT(display_log_reuseable_mem); }
// void HUDPRINT(char *text, float x, float y) { sprintf(display_log_reuseable_mem, text, x,y); HUDPRINT(display_log_reuseable_mem); }
// void HUDPRINTTIME(u64 i) { FormatTimeIntoCharBuffer(i, display_log_reuseable_mem); HUDPRINT(display_log_reuseable_mem); }
// void HUDPRINTRESET() { display_log_count = 0; }





void CreateDummyThumb(string inpath, string outpath, string dummytext) {
    CreateAllDirectoriesForPathIfNeeded(outpath.chars);
    OutputDebugString("Saving bmp!!\n");
    bitmap img = tf_create_bitmap(dummytext.ToUTF8Reusable(), 64, 255, true, true, 200);
        int result = stbi_write_bmp(outpath.ToUTF8Reusable(), img.w, img.h, 4, img.data);
        if (!result) { OutputDebugString("ERROR SAVING BITMAP\n"); }
    free(img.data);
}



