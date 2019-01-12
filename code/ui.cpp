


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





// char display_log_reuseable_mem[256];
// int display_log_count;
// opengl_quad display_log_quad;
// void HUDPRINT(char *s) {
//     bitmap img = ttf_create_bitmap(s, LIL_TEXT_SIZE, 255, true, true, 200);
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