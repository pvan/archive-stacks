

#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#include "../lib/stb_truetype.h"


stbtt_fontinfo ttfont;





u8 *ttf_file_buffer = 0;


stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs

// should just cache this
int find_largest_baked_ascent(/*float pixel_size*/) {
    // float scale = stbtt_ScaleForPixelHeight(&ttfont, pixel_size);
    int smallest_y = 10000;
    for (int i = 0; i < 96; i++) {
        // yoff appears to be basically what the y0 from stbtt_GetBakedQuad becomes
        if (cdata[i].yoff < smallest_y) smallest_y = cdata[i].yoff;
    }
    return -smallest_y; // y0s are negative, return the farthest as a distance basically
}


struct ttf_rect {int x, y, w, h;};

// todo: batch verts into one buffer
// returns bounding box of rendered text
// pass render = false to just get the bb without rendering anything
ttf_rect ttf_render_text(char *text, float screenX, float screenY,
                     bitmap baked_font, opengl_quad *render_quad,
                     bool render = true) {

    // pass font atlas to gpu if not already
    if (!render_quad->created) render_quad->create(0,0,baked_font.w, baked_font.h);
    if (!render_quad->texture_created) render_quad->set_texture(baked_font.data, baked_font.w, baked_font.h);

    // to find bb
    bool set_left_most_x = false;
    float left_most_x = 0;
    float right_most_x = 0;
    float largest_y = 0;
    float smallest_y = 10000;


    // float scale = stbtt_ScaleForPixelHeight(&ttfont, );

    // maybe affected by stbtt_GetBakedQuad? not sure
    float tx = screenX;
    float ty = screenY;
    while (*text) {
        if (*text >= 32 && *text < 128) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(cdata, 512,512, *text-32, &tx,&ty,&q,1);//1=opengl & d3d10+,0=d3d9
            // DEBUGPRINT("u0: %f  u1: %f \n", q.s0, q.s1);
            // DEBUGPRINT("v0: %f  v1: %f \n", q.t0, q.t1);
            if (render) {
                render_quad->set_verts_uvs(q.x0, q.y0, q.x1-q.x0, q.y1-q.y0,
                                       q.s0, q.s1, q.t0, q.t1);
                render_quad->render(1);
            }

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
        ++text;
    }
    ttf_rect bb;
    bb.x = left_most_x;
    bb.w = right_most_x-left_most_x;
    bb.y = smallest_y;
    bb.h = largest_y-smallest_y;

    // //add some margin to the resultbb
    // int margin = 2;
    // bb.x -= margin;
    // bb.y -= margin;
    // bb.w += margin*2;
    // bb.h += margin; // don't bother with the bottom margin (just compensate for x-=)

    // return bb;
    return {
        (int)floor(bb.x),
        (int)floor(bb.y),
        (int)ceil(bb.w),
        (int)ceil(bb.h)
    };
}


// return bitmap with baked chars on it
bitmap ttf_bake(float pixel_height)
{
    assert(ttf_file_buffer);

    int first_char = 32;
    int num_chars = 96;

    int bitmapW = 512;
    int bitmapH = 512;
    u8 *gray_bitmap = (u8*)malloc(bitmapW * bitmapH);
    ZeroMemory(gray_bitmap, bitmapW * bitmapH);

    // no guarantee this fits, esp with variable pixel_height
    stbtt_BakeFontBitmap(ttf_file_buffer, 0,
                         pixel_height,
                         gray_bitmap, bitmapW, bitmapH,
                         first_char, num_chars,
                         cdata);



    u8 *color_bitmap = (u8*)malloc(bitmapW * bitmapH * 4);
    // for (int px = 0; px < bitmapW; px++)
    for (int px = 0; px < bitmapW; px++)
    {
        for (int py = 0; py < bitmapH; py++)
        {
            u8 *r = color_bitmap + ((py*bitmapW)+px)*4 + 0;
            u8 *g = color_bitmap + ((py*bitmapW)+px)*4 + 1;
            u8 *b = color_bitmap + ((py*bitmapW)+px)*4 + 2;
            u8 *a = color_bitmap + ((py*bitmapW)+px)*4 + 3;

            int sx = px;// + first_non_blank_column;
            *r = *(gray_bitmap + ((py*bitmapW)+sx));
            *g = *(gray_bitmap + ((py*bitmapW)+sx));
            *b = *(gray_bitmap + ((py*bitmapW)+sx));
            // *a = *(gray_bitmap);// + ((py*sourceW)+sx));
            // if (bgA != 0) *a = bgA;
            *a = 255;
        }
    }
    free(gray_bitmap);

    return {(u32*)color_bitmap, bitmapW, bitmapH};
}


void ttf_init()
{
    ttf_file_buffer = (u8*)malloc(1<<20);
    fread(ttf_file_buffer, 1, 1<<20, fopen("c:/windows/fonts/segoeui.ttf", "rb"));
    // fread(ttf_file_buffer, 1, 1<<20, fopen("c:/windows/fonts/arial.ttf", "rb"));

    if (!stbtt_InitFont(&ttfont, ttf_file_buffer, 0))
    {
        MessageBox(0, "stbtt init font failed", 0,0);
    }
    // free(ttf_file_buffer);  // looks like we need to keep this around?
}

// todo: return height as well
// todo: call this in ttf_create_bitmap
float ttf_calc_width_of_text(char *text, int fsize)
{
    float bitmapWf = 0;
    float bitmapHf = 0;
    int fontH = fsize;
    float scale = stbtt_ScaleForPixelHeight(&ttfont, fontH);

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&ttfont, &ascent,&descent,&lineGap);
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
            int advance;    stbtt_GetCodepointHMetrics(&ttfont, text[i], &advance, 0);
            int kerning =   text[i+1] ? stbtt_GetCodepointKernAdvance(&ttfont, text[i], text[i+1]) : 0;
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

    return bitmapW;
}

bitmap ttf_create_bitmap(char *text, int fsize, float alpha, bool centerH, bool centerV, int bgA = 0)
{
    // d3d_textured_quad fontquad = {0};

    float bitmapWf = 0;
    float bitmapHf = 0;
    int fontH = fsize;
    float scale = stbtt_ScaleForPixelHeight(&ttfont, fontH);

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&ttfont, &ascent,&descent,&lineGap);
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
            int advance;    stbtt_GetCodepointHMetrics(&ttfont, text[i], &advance, 0);
            int kerning =   text[i+1] ? stbtt_GetCodepointKernAdvance(&ttfont, text[i], text[i+1]) : 0;
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
            int x0,y0,x1,y1; stbtt_GetCodepointBitmapBoxSubpixel(&ttfont, text[i], scale,scale,x_shift,0, &x0,&y0,&x1,&y1);
            int advance;     stbtt_GetCodepointHMetrics(&ttfont, text[i], &advance, 0);
            int kerning =    text[i+1] ? stbtt_GetCodepointKernAdvance(&ttfont, text[i], text[i+1]) : 0;

            int offset = ((ypos+y0)*bitmapW) + (int)xpos+x0;
            if (text[i] != ' ' && text[i] != NBSP)
                stbtt_MakeCodepointBitmapSubpixel(&ttfont, gray_bitmap+offset, x1-x0, y1-y0, bitmapW, scale,scale,x_shift,0, text[i]);

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
    // for (int px = 0; px < bitmapW; px++)
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

    // fontquad.update_tex(color_temp_bitmap, bitmapW,bitmapH);


    // free(color_temp_bitmap);
    free(gray_bitmap);

    return {(u32*)color_temp_bitmap, bitmapW, bitmapH};
    // return fontquad;
}

// d3d_textured_quad ttf_create(char *text, int fsize, float alpha, bool centerH, bool centerV, int bgA = 0) {

//     Bitmap bitmap = ttf_create_bitmap(text, fsize, alpha, centerH, centerV, bgA);

//     d3d_textured_quad fontquad = {0};
//     fontquad.update_tex((u8*)bitmap.mem, bitmap.w, bitmap.h);

//     free(bitmap.mem);

//     return fontquad;
// }



const int LIL_TEXT_SIZE = 24;

char display_log_reuseable_mem[256];
int display_log_count;
opengl_quad display_log_quad;
void HUDPRINT(char *s) {
    bitmap img = ttf_create_bitmap(s, LIL_TEXT_SIZE, 255, true, true, 200);
    if (!display_log_quad.created) display_log_quad.create(0,0,img.w,img.h);
    display_log_quad.set_texture(img.data, img.w, img.h);
    display_log_quad.set_verts(0, display_log_count*(LIL_TEXT_SIZE-1), img.w, img.h);
    display_log_quad.render();
    free(img.data);
    display_log_count++;
}
void HUDPRINT(u64 i)               { sprintf(display_log_reuseable_mem, "%lli", i); HUDPRINT(display_log_reuseable_mem); }
void HUDPRINT(int i)               { sprintf(display_log_reuseable_mem, "%i", i); HUDPRINT(display_log_reuseable_mem); }
void HUDPRINT(float f)             { sprintf(display_log_reuseable_mem, "%f", f); HUDPRINT(display_log_reuseable_mem); }
void HUDPRINT(char *text, int i)   { sprintf(display_log_reuseable_mem, text, i); HUDPRINT(display_log_reuseable_mem); }
void HUDPRINT(char *text, i64 i)   { sprintf(display_log_reuseable_mem, text, i); HUDPRINT(display_log_reuseable_mem); }
void HUDPRINT(char *text, char *s) { sprintf(display_log_reuseable_mem, text, s); HUDPRINT(display_log_reuseable_mem); }
void HUDPRINT(char *text, float f) { sprintf(display_log_reuseable_mem, text, f); HUDPRINT(display_log_reuseable_mem); }
void HUDPRINT(char *text, float x, float y) { sprintf(display_log_reuseable_mem, text, x,y); HUDPRINT(display_log_reuseable_mem); }
void HUDPRINTTIME(u64 i) { FormatTimeIntoCharBuffer(i, display_log_reuseable_mem); HUDPRINT(display_log_reuseable_mem); }
void HUDPRINTRESET() { display_log_count = 0; }





void CreateDummyThumb(string inpath, string outpath, string dummytext) {
    CreateAllDirectoriesForPathIfNeeded(outpath.chars);
    OutputDebugString("Saving bmp!!\n");
    bitmap img = ttf_create_bitmap(dummytext.ToUTF8Reusable(), 64, 255, true, true, 200);
        int result = stbi_write_bmp(outpath.ToUTF8Reusable(), img.w, img.h, 4, img.data);
        if (!result) { OutputDebugString("ERROR SAVING BITMAP\n"); }
    free(img.data);
}



