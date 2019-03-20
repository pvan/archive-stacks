

#define STB_TRUETYPE_IMPLEMENTATION
#include "../lib/stb_truetype.h"


// start of our new gpu api (move to gpu file)
struct gpu_quad {
    float x0, y0, u0, v0; // TL
    float x1, y1, u1, v1; // BR
    float alpha;
    bool operator==(gpu_quad o) {
        return x0==o.x0 && y0==o.y0 && u0==o.u0 && v0==o.v0 &&
               x1==o.x1 && y1==o.y1 && u1==o.u1 && v1==o.v1 &&
               alpha==o.alpha;
    }
};


// just using one vao/vbo for everything atm even though it's a little ridiculous
GLuint gpu_vao;
GLuint gpu_vbo;

void gpu_create_and_setup_global_vert_buffers() {

    // DEBUGPRINT("gpu_create_and_setup_global_vert_buffers");

    glGenVertexArrays(1, &gpu_vao);
    glBindVertexArray(gpu_vao);

    glGenBuffers(1, &gpu_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gpu_vbo);

    // pos attrib
    glVertexAttribPointer(0/*loc*/, 2/*comps*/, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), NULL);
    glEnableVertexAttribArray(0/*loc*/); // enable this attribute
    // color attrib
    glVertexAttribPointer(1/*loc*/, 3/*comps*/, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1/*loc*/); // enable this attribute
    // uv attrib
    glVertexAttribPointer(2/*loc*/, 2/*comps*/, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), (void*)(5*sizeof(float)));
    glEnableVertexAttribArray(2/*loc*/); // enable this attribute

    // // without color attrib
    // // pos attrib
    // glVertexAttribPointer(0/*loc*/, 2/*comps*/, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), NULL);
    // glEnableVertexAttribArray(0/*loc*/); // enable this attribute
    // // uv attrib
    // glVertexAttribPointer(2/*loc*/, 2/*comps*/, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (void*)(2*sizeof(float)));
    // glEnableVertexAttribArray(2/*loc*/); // enable this attribute
}



#define gpu_texture_id GLuint


// create spot for texture on gpu
// return id of texture
gpu_texture_id gpu_create_texture() {

    // DEBUGPRINT("gpu_create_texture");

    // MessageBox(0,"test",0,0);

    // create texture
    GLuint texture = 0;
    glGenTextures(1, &texture);  DEBUGPRINT("glGenTextures");
    // set texture params
    glBindTexture(GL_TEXTURE_2D, texture);   DEBUGPRINT("glBindTexture");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//GL_NEAREST_MIPMAP_LINEAR); // what to use?
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //GL_LINEAR
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return texture;
}

// pass texture data to gpu
// only supporting rgba format atm
void gpu_upload_texture(u32 *tex, int w, int h, gpu_texture_id tex_id) {

    // DEBUGPRINT("gpu_upload_texture");

    // if (!texture_created) create_texture(); // could cache what id's we've created already?

    glActiveTexture(GL_TEXTURE0); // texture unit   todo: curious, can you set texture unit after binding the texture?
    glBindTexture(GL_TEXTURE_2D, tex_id); // todo: what happens if we bind a texture id we haven't created yet?

    // todo look into
    // glGetInternalFormativ(GL_TEXTURE_2D, GL_RGBA8, GL_TEXTURE_IMAGE_FORMAT, 1, &preferred_format).

    // for now, just using one kind of data format on our textures: rgba
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, tex);

    // todo check for GL_OUT_OF_MEMORY ?
    glGenerateMipmap(GL_TEXTURE_2D); // i think we always need to call this even if we aren't using them
}
void gpu_upload_texture(bitmap img, gpu_texture_id tex_id) {
    gpu_upload_texture(img.data, img.w, img.h, tex_id);
}



// every time we pass verts to the gpu we need some memory to
// convert the list of quads to a list of triangle verts
// this is the pointer to that memory (freed/alloced every upload_ call)
float *gpu_cached_verts = 0;

// returns number of verts uploaded
int gpu_upload_vertices(gpu_quad *quads, int quadcount) {

    // DEBUGPRINT("gpu_upload_vertices");

    // float
    // float verts[] = {
    //     // pos   // col   // uv
    //     x,y,     1,1,1,   u0, v0,
    //     x,y+h,   1,1,1,   u0, v1,
    //     x+w,y,   1,1,1,   u1, v0,
    //     x+w,y+h, 1,1,1,   u1, v1,
    // };



    int floats_per_quad = 3/*verts per tri*/ * 2/*tris*/ * 7/*comps per vert*/;
    int total_floats = quadcount*floats_per_quad;

    if (gpu_cached_verts) free(gpu_cached_verts);
    gpu_cached_verts = (float*)malloc(total_floats * sizeof(float));

    int v = 0;
    for (int i = 0; i < quadcount; i++) {
        gpu_quad q = quads[i];

        // TL
        gpu_cached_verts[v++]=q.x0; gpu_cached_verts[v++]=q.y0; gpu_cached_verts[v++]=q.alpha; gpu_cached_verts[v++]=1; gpu_cached_verts[v++]=1;
        gpu_cached_verts[v++]=q.u0; gpu_cached_verts[v++]=q.v0;
        // TR
        gpu_cached_verts[v++]=q.x1; gpu_cached_verts[v++]=q.y0; gpu_cached_verts[v++]=q.alpha; gpu_cached_verts[v++]=1; gpu_cached_verts[v++]=1;
        gpu_cached_verts[v++]=q.u1; gpu_cached_verts[v++]=q.v0;
        // BR
        gpu_cached_verts[v++]=q.x1; gpu_cached_verts[v++]=q.y1; gpu_cached_verts[v++]=q.alpha; gpu_cached_verts[v++]=1; gpu_cached_verts[v++]=1;
        gpu_cached_verts[v++]=q.u1; gpu_cached_verts[v++]=q.v1;

        // BR
        gpu_cached_verts[v++]=q.x1; gpu_cached_verts[v++]=q.y1; gpu_cached_verts[v++]=q.alpha; gpu_cached_verts[v++]=1; gpu_cached_verts[v++]=1;
        gpu_cached_verts[v++]=q.u1; gpu_cached_verts[v++]=q.v1;
        // TL
        gpu_cached_verts[v++]=q.x0; gpu_cached_verts[v++]=q.y0; gpu_cached_verts[v++]=q.alpha; gpu_cached_verts[v++]=1; gpu_cached_verts[v++]=1;
        gpu_cached_verts[v++]=q.u0; gpu_cached_verts[v++]=q.v0;
        // BL
        gpu_cached_verts[v++]=q.x0; gpu_cached_verts[v++]=q.y1; gpu_cached_verts[v++]=q.alpha; gpu_cached_verts[v++]=1; gpu_cached_verts[v++]=1;
        gpu_cached_verts[v++]=q.u0; gpu_cached_verts[v++]=q.v1;

    }

    assert(v == total_floats);
    int cached_vert_count = total_floats / 7/*comps per vert*/;

    glBindVertexArray(gpu_vao); // need this here or no?
    glBindBuffer(GL_ARRAY_BUFFER, gpu_vbo);
    glBufferData(GL_ARRAY_BUFFER, total_floats*sizeof(float), gpu_cached_verts, GL_STATIC_DRAW);

    return cached_vert_count;
}



void gpu_render_quads_with_texture(gpu_quad *quads, int quadcount, gpu_texture_id tex_id, float a = 1) {

    // DEBUGPRINT("gpu_render_quads_with_texture");

    int vertcount = gpu_upload_vertices(quads, quadcount);
    // if (vertcount <= 0) return;

    // todo: it might be faster to rebind the attributes every frame than switch vaos?
    glBindVertexArray(gpu_vao); // has all our attribute info (et al?) tied to it

    // GLuint loc_color = glGetUniformLocation(shader_program, "color");
    // glUniform4f(loc_color, r,g,b,a);
    GLuint loc_alpha = glGetUniformLocation(opengl_shader_program, "alpha");
    glUniform1f(loc_alpha, a);

    glActiveTexture(GL_TEXTURE0); // texture unit
    glBindTexture(GL_TEXTURE_2D, tex_id);

    glDrawArrays(GL_TRIANGLES, 0, vertcount);
}


void gpu_init(HDC hdc) {
    // opengl_init(hdc); // uncomment this when removing old api

    gpu_create_and_setup_global_vert_buffers();  // or let caller manage their own / multiple vao/vbo ids?

}


////
//


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
            // *a = *(gray_bitmap);// + ((py*sourceW)+sx));
            // if (bgA != 0) *a = bgA;
            *a = 255;
        }
    }
    free(gray_bitmap);

    return {(u32*)color_bitmap, bitmapW, bitmapH};
}


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
            quadlist[quadcountsofar++] = {q.x0,q.y0,q.s0,q.t0, q.x1,q.y1,q.s1,q.t1, 1}; // 1 for alpha
            // // DEBUGPRINT("u0: %f  u1: %f \n", q.s0, q.s1);
            // // DEBUGPRINT("v0: %f  v1: %f \n", q.t0, q.t1);
            // if (render) {
            //     render_quad->set_verts_uvs(q.x0, q.y0, q.x1-q.x0, q.y1-q.y0,
            //                            q.s0, q.s1, q.t0, q.t1);
            //     render_quad->render(1);
            // }

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

void tf_init(int fontsize) {
    tf_openfont();
    tf_fontatlas = tf_bakefont(fontsize);
    tf_fonttexture = gpu_create_texture();
    gpu_upload_texture(tf_fontatlas.data, tf_fontatlas.w, tf_fontatlas.h, tf_fonttexture);
}

//
////


//
// old api



stbtt_fontinfo ttfont;  // font info loaded by stb

u8 *ttf_file_buffer = 0;  // opened font file


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



struct ttf_rect {int x, y, w, h;};


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



