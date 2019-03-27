




// start of our new gpu api (move to gpu file)
struct gpu_quad {
    float x0, y0, u0, v0; // TL
    float x1, y1, u1, v1; // BR
    float alpha;
    u32 color; // single color for entire quad
    float z; // z level for whole quad, for creating quads under previously-created quads, basically
    bool equ(gpu_quad o) {
        return x0==o.x0 && y0==o.y0 && u0==o.u0 && v0==o.v0 &&
               x1==o.x1 && y1==o.y1 && u1==o.u1 && v1==o.v1 &&
               alpha == o.alpha &&
               color == o.color &&
               z == o.z;
    }
    bool operator==(gpu_quad other) { return equ(other); }
    bool operator!=(gpu_quad other) { return !equ(other); }
    rect to_rect() { return { x0, y0, x1-x0, y1-y0 }; }
    void move(float dx, float dy) { x0+=dx; x1+=dx;  y0+=dy; y1+=dy; }
    float width() { return x1-x0; }
    float height() { return y1-y0; }
    static gpu_quad default_new() {
        gpu_quad q = {0};
        q.alpha = 1;
        q.color = 0xffffffff;
        q.z = 0.001;
        return q;
    }
};


gpu_quad gpu_quad_from_rect(rect r, float alpha = 1) {
    gpu_quad q = gpu_quad::default_new();
    // gpu_quad q;
    q.x0 = r.x;
    q.y0 = r.y;
    q.x1 = r.x + r.w;
    q.y1 = r.y + r.h;
    q.u0 = 0.0;
    q.v0 = 0.0;
    q.u1 = 1.0;
    q.v1 = 1.0;
    q.alpha = alpha;
    // q.color = 0xffffffff;
    // q.z = 0;
    return q;
}


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
    glVertexAttribPointer(0/*loc*/, 3/*comps*/, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), NULL);
    glEnableVertexAttribArray(0/*loc*/); // enable this attribute
    // color attrib
    glVertexAttribPointer(1/*loc*/, 3/*comps*/, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1/*loc*/); // enable this attribute
    // uv attrib
    glVertexAttribPointer(2/*loc*/, 2/*comps*/, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2/*loc*/); // enable this attribute
    // alpha attrib
    glVertexAttribPointer(3/*loc*/, 1/*comps*/, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), (void*)(8*sizeof(float)));
    glEnableVertexAttribArray(3/*loc*/); // enable this attribute

    // without z
    // // pos attrib
    // glVertexAttribPointer(0/*loc*/, 2/*comps*/, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), NULL);
    // glEnableVertexAttribArray(0/*loc*/); // enable this attribute
    // // color attrib
    // glVertexAttribPointer(1/*loc*/, 3/*comps*/, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (void*)(2*sizeof(float)));
    // glEnableVertexAttribArray(1/*loc*/); // enable this attribute
    // // uv attrib
    // glVertexAttribPointer(2/*loc*/, 2/*comps*/, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (void*)(5*sizeof(float)));
    // glEnableVertexAttribArray(2/*loc*/); // enable this attribute
    // // alpha attrib
    // glVertexAttribPointer(3/*loc*/, 1/*comps*/, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), (void*)(7*sizeof(float)));
    // glEnableVertexAttribArray(3/*loc*/); // enable this attribute

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

    // create texture
    GLuint texture = 0;
    glGenTextures(1, &texture);
    // set texture params
    glBindTexture(GL_TEXTURE_2D, texture);
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



    int floats_per_quad = 3/*verts per tri*/ * 2/*tris*/ * 9/*comps per vert*/;
    int total_floats = quadcount*floats_per_quad;

    gpu_cached_verts = (float*)malloc(total_floats * sizeof(float));

    int v = 0;
    for (int i = 0; i < quadcount; i++) {
        gpu_quad q = quads[i];

        // for color, alpha and z...
        // would it be better to pass some other way?
        // as uniforms, or some other buffer type?
        // since they are the same for the whole quad.. passing them 5 times more than needed

        float r = (float)(q.color>>16 & 0xff) / 255.0;
        float g = (float)(q.color>>8 & 0xff) / 255.0;
        float b = (float)(q.color>>0 & 0xff) / 255.0;

        // TL
        gpu_cached_verts[v++]=q.x0; gpu_cached_verts[v++]=q.y0; gpu_cached_verts[v++]=q.z;
        gpu_cached_verts[v++]=r; gpu_cached_verts[v++]=g; gpu_cached_verts[v++]=b;
        gpu_cached_verts[v++]=q.u0; gpu_cached_verts[v++]=q.v0;
        gpu_cached_verts[v++]=q.alpha;
        // TR
        gpu_cached_verts[v++]=q.x1; gpu_cached_verts[v++]=q.y0; gpu_cached_verts[v++]=q.z;
        gpu_cached_verts[v++]=r; gpu_cached_verts[v++]=g; gpu_cached_verts[v++]=b;
        gpu_cached_verts[v++]=q.u1; gpu_cached_verts[v++]=q.v0;
        gpu_cached_verts[v++]=q.alpha;
        // BR
        gpu_cached_verts[v++]=q.x1; gpu_cached_verts[v++]=q.y1; gpu_cached_verts[v++]=q.z;
        gpu_cached_verts[v++]=r; gpu_cached_verts[v++]=g; gpu_cached_verts[v++]=b;
        gpu_cached_verts[v++]=q.u1; gpu_cached_verts[v++]=q.v1;
        gpu_cached_verts[v++]=q.alpha;

        // BR
        gpu_cached_verts[v++]=q.x1; gpu_cached_verts[v++]=q.y1; gpu_cached_verts[v++]=q.z;
        gpu_cached_verts[v++]=r; gpu_cached_verts[v++]=g; gpu_cached_verts[v++]=b;
        gpu_cached_verts[v++]=q.u1; gpu_cached_verts[v++]=q.v1;
        gpu_cached_verts[v++]=q.alpha;
        // TL
        gpu_cached_verts[v++]=q.x0; gpu_cached_verts[v++]=q.y0; gpu_cached_verts[v++]=q.z;
        gpu_cached_verts[v++]=r; gpu_cached_verts[v++]=g; gpu_cached_verts[v++]=b;
        gpu_cached_verts[v++]=q.u0; gpu_cached_verts[v++]=q.v0;
        gpu_cached_verts[v++]=q.alpha;
        // BL
        gpu_cached_verts[v++]=q.x0; gpu_cached_verts[v++]=q.y1; gpu_cached_verts[v++]=q.z;
        gpu_cached_verts[v++]=r; gpu_cached_verts[v++]=g; gpu_cached_verts[v++]=b;
        gpu_cached_verts[v++]=q.u0; gpu_cached_verts[v++]=q.v1;
        gpu_cached_verts[v++]=q.alpha;

    }

    assert(v == total_floats);
    int cached_vert_count = total_floats / 9/*comps per vert*/;

    glBindVertexArray(gpu_vao); // need this here or no?
    glBindBuffer(GL_ARRAY_BUFFER, gpu_vbo);
    glBufferData(GL_ARRAY_BUFFER, total_floats*sizeof(float), gpu_cached_verts, GL_STATIC_DRAW);

    if (gpu_cached_verts) free(gpu_cached_verts);

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





DEFINE_TYPE_POOL(gpu_quad);

// this is kind of a single-texture submesh object
// basically a list of quads all sharing the same texture id (with that id attached)
struct gpu_quad_list_with_texture { // either the best or worst name i've ever come up with..
    gpu_quad_pool quads;
    gpu_texture_id texture_id;
    bool equ(gpu_quad_list_with_texture o) {
        if (texture_id != o.texture_id) return false;
        if (quads.count != o.quads.count) return false;
        for (int i = 0; i < quads.count; i++) {
            if (quads[i] != o.quads[i]) return false;
        }
        return true;
    }
    bool operator==(gpu_quad_list_with_texture other) { return equ(other); }
    bool operator!=(gpu_quad_list_with_texture other) { return !equ(other); }
};

DEFINE_TYPE_POOL(gpu_quad_list_with_texture);

// basically a list of submeshes
// "multi_texture_quad_lists" ? too much?
struct mesh {
    gpu_quad_list_with_texture_pool submeshes;
    bool equ(mesh o) {
        if (submeshes.count != o.submeshes.count) return false;
        for (int i = 0; i < submeshes.count; i++) {
            if (submeshes[i] != o.submeshes[i]) return false;
        }
        return true;
    }
    bool operator==(mesh other) { return equ(other); }
    bool operator!=(mesh other) { return !equ(other); }

    void add_submesh(gpu_quad_list_with_texture newsubmesh) {
        submeshes.add(newsubmesh);
    }

    void free_all_mem() {
        for (int i = 0; i < submeshes.count; i++) {
            if (submeshes[i].quads.pool) free(submeshes[i].quads.pool);
        }
        if (submeshes.pool) free(submeshes.pool);
    }
};

DEFINE_TYPE_POOL(mesh);

void gpu_render_mesh(mesh m) {
    // we assume all textures have been uploaded already (todo: add way to check/verify)
    for (int i = 0; i < m.submeshes.count; i++) {
        gpu_quad_list_with_texture& submesh = m.submeshes[i];
        gpu_render_quads_with_texture(&submesh.quads.pool[0], submesh.quads.count, submesh.texture_id, 1);
    }
}

