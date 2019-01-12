

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

    out vec4 vertColor;
    out vec2 vertUV;

    uniform vec4 camera;

    void main()
    {
        vertColor = vec4(aColor,1);
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
        fragColor.a = alpha;
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
// glDeleteTextures



HDC opengl_hdc;

GLuint opengl_shader_program;
GLuint opengl_vao;


void APIENTRY opengl_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                        const GLchar* message, const void* userParam) {
    char msg[1024];
    sprintf(msg, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "", type, severity, message);
    OutputDebugString(msg);
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


    // i think we can just set up a vao here since we're only using one vert/attrib style
    glGenVertexArrays(1, &opengl_vao);
    glBindVertexArray(opengl_vao);
    // // position attrib
    // glVertexAttribPointer(0/*loc*/, 2/*comps*/, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), NULL);
    // glEnableVertexAttribArray(0/*loc*/); // enable this attribute
    // // color attrib
    // glVertexAttribPointer(1/*loc*/, 3/*comps*/, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), (void*)(2*sizeof(float)));
    // glEnableVertexAttribArray(1/*loc*/); // enable this attribute
    // // uv attrib
    // glVertexAttribPointer(2/*loc*/, 2/*comps*/, GL_FLOAT, GL_FALSE, 7*sizeof(GLfloat), (void*)(5*sizeof(float)));
    // glEnableVertexAttribArray(2/*loc*/); // enable this attribute

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

    // GLuint vao;
    GLuint vbo;
    GLuint texture;

    float cached_w;
    float cached_h;

    bool texture_created = false;
    bool created = false;

    // void set_verts_raw(float x1,float y1, float x2,float y2, float x3,float y3,float x4,float y4) {
    //     cached_w = 1;
    //     cached_h = 1;
    //     float verts[] = {
    //         // pos   // col   // uv
    //         x1,y1,   1,1,1,   0.0f, 0.0f,
    //         x2,y2,   1,1,1,   0.0f, 1.0f,
    //         x4,y4,   1,1,1,   1.0f, 0.0f,
    //         x3,y3,   1,1,1,   1.0f, 1.0f,
    //     };
    //     glBindBuffer(GL_ARRAY_BUFFER, vbo);
    //     glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    // }

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
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    }

    void create_texture() {
        // create texture
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR); // what to use?
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
        //glGetInternalFormativ(GL_TEXTURE_2D, GL_RGBA8, GL_TEXTURE_IMAGE_FORMAT, 1, &preferred_format).
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
        // todo check for GL_OUT_OF_MEMORY ?
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    void create(float x, float y, float w, float h) {
        if (created) return;
        // float verts[] = {
        //     // pos   // col
        //     x,y,     1,0,0,
        //     x,y+h,   1,1,0,
        //     x+w,y,   1,0,1,
        //     x+w,y+h, 0,1,1,
        // };

        glBindVertexArray(opengl_vao); // kind of unnecessary since we only have 1 atm
        // glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        set_verts(x, y, w, h);

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

    void set_pos(float x, float y) {
        set_verts(x, y, cached_w, cached_h);
    }

    void render(float a = 1) {

        // we only have one atm anyway so should be already bound
        glBindVertexArray(opengl_vao); // has all our attribute info (et al?) tied to it
        // glBindVertexArray(vao); // has all our attribute info (et al?) tied to it

        // GLuint loc_color = glGetUniformLocation(shader_program, "color");
        // glUniform4f(loc_color, r,g,b,a);
        GLuint loc_alpha = glGetUniformLocation(opengl_shader_program, "alpha");
        glUniform1f(loc_alpha, a);

        glActiveTexture(GL_TEXTURE0); // texture unit
        glBindTexture(GL_TEXTURE_2D, texture);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
};

// #include "../lib/stb_easy_font.h"

// // opengl_quad text_quad;
// u32 text_colorW = 0xffffffff;
// u32 text_colorB = 0xff000000;
// struct easy_font_vert { float x, y, z; u8 col[4]; };

// void opengl_print_string(float x, float y, char *text, float r, float g, float b, opengl_quad q)
// {
//     static char buffer[99999]; // ~500 chars
//     int num_quads = stb_easy_font_print(x, y, text, NULL, buffer, sizeof(buffer));

//     // only need to call once..
//     // text_quad.create(0,0,1,1);
//     stb_easy_font_spacing(-0.5);

//     float s = 3;

//     easy_font_vert *verts = (easy_font_vert*)buffer;
//     for (int i = 0; i < num_quads*4; i+=4) {
//         easy_font_vert v1 = verts[i];
//         easy_font_vert v2 = verts[i+1];
//         easy_font_vert v3 = verts[i+2];
//         easy_font_vert v4 = verts[i+3];

//         q.set_verts_raw(v1.x*s,v1.y*s, v2.x*s,v2.y*s, v3.x*s,v3.y*s, v4.x*s,v4.y*s);
//         q.set_texture(&text_colorB, 1, 1);
//         q.render(1);

//         // q.set_verts_raw(v1.x*s+1,v1.y*s-1, v2.x*s-1,v2.y*s-1, v3.x*s-1,v3.y*s+1, v4.x*s+1,v4.y*s+1);
//         // q.set_texture(&text_colorW, 1, 1);
//         // q.render(1);
//     }
// }

void opengl_clear() {
    glClearColor(0, 0.5, 0.7, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void opengl_swap() {
    SwapBuffers(opengl_hdc);
}



