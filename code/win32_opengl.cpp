

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



HDC opengl_hdc;

GLuint shader_program;

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
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vshader);
    glAttachShader(shader_program, fshader);
    glLinkProgram(shader_program);
        glGetShaderInfoLog(shader_program, 256, 0, shaderlog);
        sprintf(displaybuf, "\nshader_program log:%s\n", shaderlog);
        OutputDebugString(displaybuf);
    glUseProgram(shader_program);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    // enable alpha
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
}

float cached_screen_width;
float cached_screen_height;
void opengl_resize_if_change(float width, float height) {
    if (cached_screen_width != width || cached_screen_height != height) {
        glViewport(0,0, width, height);
        GLuint loc_cam = glGetUniformLocation(shader_program, "camera");
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

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    void create(float x, float y, float w, float h) {

        // float verts[] = {
        //     // pos   // col
        //     x,y,     1,0,0,
        //     x,y+h,   1,1,0,
        //     x+w,y,   1,0,1,
        //     x+w,y+h, 0,1,1,
        // };
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);

        set_verts(x, y, w, h);


        // position attrib
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

        glGenVertexArrays(1, &vao); // call in case we've switched vaos in the mean time

        // GLuint loc_color = glGetUniformLocation(shader_program, "color");
        // glUniform4f(loc_color, r,g,b,a);
        GLuint loc_alpha = glGetUniformLocation(shader_program, "alpha");
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


