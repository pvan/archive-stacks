


const int UI_TEXT_SIZE = 24;
const int UI_RESUSABLE_BUFFER_SIZE = 256;

char ui_text_reusable_buffer[UI_RESUSABLE_BUFFER_SIZE];
opengl_quad ui_reusable_quad;

void ui_text(char *text, int x, int y) {
    bitmap img = ttf_create_bitmap(text, UI_TEXT_SIZE, 255, true, true, 200);
    if (!ui_reusable_quad.created) ui_reusable_quad.create(0,0,img.w,img.h);
    ui_reusable_quad.set_texture(img.data, img.w, img.h);
    ui_reusable_quad.set_verts(x-img.w/2, y-img.h/2, img.w, img.h);
    ui_reusable_quad.render();
    free(img.data);
}

void ui_text(char *text, float value, int x, int y) {
    sprintf(ui_text_reusable_buffer, text, value);
    ui_text(ui_text_reusable_buffer, x, y);
}




