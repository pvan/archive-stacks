

#include <gl\gl.h>

#include "..\lib\glext.h"
#include "..\lib\wglext.h"



#define MULTILINE_STRING(...) #__VA_ARGS__

char *vertex_shader = MULTILINE_STRING
(
    #version 330 \n

    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec3 aColor;
    layout (location = 2) in vec2 aUV;
    layout (location = 3) in float aAlpha;

    out vec4 vertColor;
    out vec2 vertUV;
    out float vertA;

    uniform vec4 camera;

    void main()
    {
        vertA = aAlpha;
        vertColor = vec4(aColor,1);
        // vertColor = vec4(1,1,1,1);
        vertUV = aUV;
        vec4 ppp = vec4(aPos.x, aPos.y, 1, 1);
        ppp.xy = (ppp.xy / camera.zw)*2.0 - 1.0;
        ppp.y *= -1;  // move origin to top
        gl_Position = ppp;
    }
);

char *fragment_shader = MULTILINE_STRING
(
    #version 330 \n

    in vec4 vertColor;
    in vec2 vertUV;
    in float vertA;

    out vec4 fragColor;

    uniform sampler2D tex;
    // uniform vec4 color;
    uniform float alpha;

    void main()
    {
        // fragColor = texture(tex, vertUV);
        // fragColor = texture(tex, vertUV) * color;
        fragColor = texture(tex, vertUV) * vertColor;
        fragColor.rb = fragColor.br; // swap rb
        fragColor.a = alpha * vertA;
        // fragColor = texture(tex, vertUV) * vertColor * color;
        // fragColor = mix(texture(tex, vertUV), vertColor, 0.2);
        // fragColor = mix(mix(texture(tex, vertUV), vertColor, 0.5), color, 0.5);
        // fragColor = mix(texture(tex, vertUV) * vertColor, color, 0.5);
    }
);



PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormat;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribs;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLGENBUFFERSARBPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM4FPROC glUniform4f;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
PFNGLACTIVETEXTUREPROC glActiveTexture;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLGETINTERNALFORMATIVPROC glGetInternalFormativ;



HDC opengl_hdc;

GLuint opengl_shader_program;
// GLuint opengl_vao;


bool OPENGL_DEBUG_MESSAGES = true;
// bool OPENGL_DEBUG_MESSAGES = false;


void APIENTRY opengl_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                        const GLchar* message, const void* userParam) {
    if (severity != 0x826b) { // ignore notifications
        char msg[1024];
        sprintf(msg, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
                type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "", type, severity, message);
        if (OPENGL_DEBUG_MESSAGES) OutputDebugString(msg);
    }
}

void opengl_init(HDC hdc)
{
    opengl_hdc = hdc;

    PIXELFORMATDESCRIPTOR pfd;
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE;
    pfd.iPixelType = PFD_TYPE_RGBA,
    pfd.cColorBits = 32;

    int pixel_format = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixel_format, &pfd);
    HGLRC hglrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc);


    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
    glGenBuffers = (PFNGLGENBUFFERSARBPROC)wglGetProcAddress("glGenBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
    glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
    glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
    glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
    glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
    glUniform4f = (PFNGLUNIFORM4FPROC)wglGetProcAddress("glUniform4f");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
    glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
    glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)wglGetProcAddress("glGetAttribLocation");
    glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
    glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)wglGetProcAddress("glDebugMessageCallback");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
    glGetInternalFormativ = (PFNGLGETINTERNALFORMATIVPROC)wglGetProcAddress("glGetInternalFormativ");


        char shaderlog[256];
        char displaybuf[256];

    // create vertex shader program
    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, 1, &vertex_shader, 0);
    glCompileShader(vshader);
        glGetShaderInfoLog(vshader, 256, 0, shaderlog);
        sprintf(displaybuf, "\nvshader log:%s\n", shaderlog);
        OutputDebugString(displaybuf);

    // create fragment shader program
    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, 1, &fragment_shader, 0);
    glCompileShader(fshader);
        glGetShaderInfoLog(fshader, 256, 0, shaderlog);
        sprintf(displaybuf, "\nfshader log:%s\n", shaderlog);
        OutputDebugString(displaybuf);

    // link shaders
    opengl_shader_program = glCreateProgram();
    glAttachShader(opengl_shader_program, vshader);
    glAttachShader(opengl_shader_program, fshader);
    glLinkProgram(opengl_shader_program);
        glGetShaderInfoLog(opengl_shader_program, 256, 0, shaderlog);
        sprintf(displaybuf, "\nshader_program log:%s\n", shaderlog);
        OutputDebugString(displaybuf);
    glUseProgram(opengl_shader_program);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    // enable alpha
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    // enable debug msg callback
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(opengl_message_callback, 0);


    // on trying this, it seems lke maybe we need a vao for every vbo?
    // // i think we can just set up a vao here since we're only using one vert/attrib style
    // glGenVertexArrays(1, &opengl_vao);
    // glBindVertexArray(opengl_vao);
    // // position attrib
    // glVertexAttribPointer(0/*loc*/, 2/*comps*/, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), NULL);
    // glEnableVertexAttribArray(0/*loc*/); // enable this attribute
    // // color attrib
    // glVertexAttribPointer(1/*loc*/, 3/*comps*/, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), (void*)(2*sizeof(float)));
    // glEnableVertexAttribArray(1/*loc*/); // enable this attribute
    // // uv attrib
    // glVertexAttribPointer(2/*loc*/, 2/*comps*/, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), (void*)(5*sizeof(float)));
    // glEnableVertexAttribArray(2/*loc*/); // enable this attribute


    // need opengl 4.2 at least
    // // todo: look at glGetInternalFormativ to make sure we're passing texture in format gpu likes
    // GLint preferred_format = 0;
    // glGetInternalFormativ(GL_TEXTURE_2D, GL_RGBA, GL_TEXTURE_IMAGE_FORMAT, 1, &preferred_format);
    // DEBUGPRINT("preferred_format: %i\n", preferred_format);

}

float cached_screen_width;
float cached_screen_height;
void opengl_resize_if_change(float width, float height) {
    if (cached_screen_width != width || cached_screen_height != height) {
        glViewport(0,0, width, height);
        GLuint loc_cam = glGetUniformLocation(opengl_shader_program, "camera");
        glUniform4f(loc_cam, 0,0, width,height);
    }
}

struct opengl_quad {

    GLuint vao;
    GLuint vbo;
    GLuint texture;

    float cached_w;
    float cached_h;

    bool texture_created = false;
    bool created = false;

    void set_verts(float x, float y, float w, float h) {
        cached_w = w;
        cached_h = h;
        float verts[] = {
            // pos   // col   // uv
            x,y,     1,1,1,   0.0f, 0.0f,
            x,y+h,   1,1,1,   0.0f, 1.0f,
            x+w,y,   1,1,1,   1.0f, 0.0f,
            x+w,y+h, 1,1,1,   1.0f, 1.0f,
        };
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    }
    void set_verts_uvs(float x, float y, float w, float h,
                       float u0, float u1, float v0, float v1) {
        cached_w = w;
        cached_h = h;
        float verts[] = {
            // pos   // col   // uv
            x,y,     1,1,1,   u0, v0,
            x,y+h,   1,1,1,   u0, v1,
            x+w,y,   1,1,1,   u1, v0,
            x+w,y+h, 1,1,1,   u1, v1,
        };
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    }

    void create_texture() {
        // create texture
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//GL_NEAREST_MIPMAP_LINEAR); // what to use?
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //GL_LINEAR
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        texture_created = true;
    }

    void set_texture(u32 *tex, int w, int h) {
        if (!texture_created) create_texture();

        glActiveTexture(GL_TEXTURE0); // texture unit
        glBindTexture(GL_TEXTURE_2D, texture);

        // todo look into
        // glGetInternalFormativ(GL_TEXTURE_2D, GL_RGBA8, GL_TEXTURE_IMAGE_FORMAT, 1, &preferred_format).
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
        // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, tex);
        // todo check for GL_OUT_OF_MEMORY ?
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    void create(float x, float y, float w, float h) {
        if (created) return;

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);  // only reason this works without this is it's bound in set_verts() as well

        set_verts(x, y, w, h);

        // pos attrib
        glVertexAttribPointer(0/*loc*/, 2/*comps*/, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), NULL);
        glEnableVertexAttribArray(0/*loc*/); // enable this attribute
        // color attrib
        glVertexAttribPointer(1/*loc*/, 3/*comps*/, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), (void*)(2*sizeof(float)));
        glEnableVertexAttribArray(1/*loc*/); // enable this attribute
        // uv attrib
        glVertexAttribPointer(2/*loc*/, 2/*comps*/, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), (void*)(5*sizeof(float)));
        glEnableVertexAttribArray(2/*loc*/); // enable this attribute

        created = true;
    }

    // delete texture memory
    void destroy() {
        glDeleteTextures(1, &texture);
        glDeleteBuffers(1, &vbo);
        created = false;
    }

    void set_pos(float x, float y) {
        set_verts(x, y, cached_w, cached_h);
    }

    void render(float a = 1) {

        // todo: it might be faster to rebind the attributes every frame than switch vaos?
        glBindVertexArray(vao); // has all our attribute info (et al?) tied to it

        // GLuint loc_color = glGetUniformLocation(shader_program, "color");
        // glUniform4f(loc_color, r,g,b,a);
        GLuint loc_alpha = glGetUniformLocation(opengl_shader_program, "alpha");
        glUniform1f(loc_alpha, a);

        glActiveTexture(GL_TEXTURE0); // texture unit
        glBindTexture(GL_TEXTURE_2D, texture);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
};


void opengl_clear() {
    glClearColor(0, 0.5, 0.7, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void opengl_swap() {
    SwapBuffers(opengl_hdc);
}



//
// BATCHED QUADS idea below
// untested
//

float *batch_verts;
int batch_count = 0;
int batch_allocated = 0;

// int texture_ids[1024]; // TODO: just assume this max for now

GLuint batch_vao;
GLuint batch_vbo;


void opengl_batch_init() {

    glGenVertexArrays(1, &batch_vao);
    glBindVertexArray(batch_vao);

    glGenBuffers(1, &batch_vbo);
    glBindVertexArray(batch_vao);
    glBindBuffer(GL_ARRAY_BUFFER, batch_vbo);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // pass verts later

    // do we need to set these up for every vbo? shouldn't they be tied to our vao somehow?
    glVertexAttribPointer(0/*loc*/, 2/*comps*/, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), NULL);
    glEnableVertexAttribArray(0/*loc*/); // enable this attribute
    // color attrib
    glVertexAttribPointer(1/*loc*/, 3/*comps*/, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1/*loc*/); // enable this attribute
    // uv attrib
    glVertexAttribPointer(2/*loc*/, 2/*comps*/, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), (void*)(5*sizeof(float)));
    glEnableVertexAttribArray(2/*loc*/); // enable this attribute
}

void opengl_batch_add_quad(float x, float y, float w, float h) {

    float new_verts[] = {
        // pos   // col   // uv
        x,y,     1,1,1,   0.0f, 0.0f,
        x,y+h,   1,1,1,   0.0f, 1.0f,
        x+w,y,   1,1,1,   1.0f, 0.0f,
        x+w,y+h, 1,1,1,   1.0f, 1.0f,
    };

    int new_vert_count = sizeof(new_verts)/sizeof(new_verts[0]);

    int new_total_count = batch_count + new_vert_count;

    if (!batch_verts) {
        batch_allocated = 1024;
        batch_verts = (float*)malloc(batch_allocated * sizeof(float));
    }
    if (new_total_count > batch_allocated) {
        batch_allocated *= 2;
        batch_verts = (float*)realloc(batch_verts, batch_allocated * sizeof(float));
        // potentially could still not have enough space
    }

    for (int i = 0; i < new_vert_count; i++) {
        batch_verts[batch_count++] = new_verts[i];
    }


}

bool opengl_batch_texture_created = false;
GLuint opengl_batch_texture;

void opengl_batch_upload_textures() {

    if (!opengl_batch_texture_created) {
        glGenTextures(1, &opengl_batch_texture);
        glBindTexture(GL_TEXTURE_2D, opengl_batch_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR); // what to use?
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //GL_LINEAR
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        opengl_batch_texture_created = true;
    }

    glActiveTexture(GL_TEXTURE0); // texture unit
    glBindTexture(GL_TEXTURE_2D, opengl_batch_texture);

    // todo: upload texture/textures here
    // but how to tie certain textures back to certain verts?
    // and do we atlas them (how to tile?) or array them (need to be same size)?
    // // todo look into
    // // glGetInternalFormativ(GL_TEXTURE_2D, GL_RGBA8, GL_TEXTURE_IMAGE_FORMAT, 1, &preferred_format).
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
    // // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, tex);
    // // todo check for GL_OUT_OF_MEMORY ?
    // glGenerateMipmap(GL_TEXTURE_2D);
}

void opengl_batch_upload_verts() {
    glBindVertexArray(batch_vao);
    glBindBuffer(GL_ARRAY_BUFFER, batch_vbo);
    glBufferData(GL_ARRAY_BUFFER, batch_count*sizeof(float), batch_verts, GL_STATIC_DRAW);
}

void opengl_batch_render() {

    glBindVertexArray(batch_vao); // has all our attribute info (et al?) tied to it

    // GLuint loc_color = glGetUniformLocation(shader_program, "color");
    // glUniform4f(loc_color, r,g,b,a);
    GLuint loc_alpha = glGetUniformLocation(opengl_shader_program, "alpha");
    glUniform1f(loc_alpha, 1);

    glActiveTexture(GL_TEXTURE0); // texture unit
    glBindTexture(GL_TEXTURE_2D, opengl_batch_texture);

    // glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, batch_count);

}


