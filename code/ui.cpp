


const int UI_TEXT_SIZE = 24;
const int UI_RESUSABLE_BUFFER_SIZE = 256;

char ui_text_reusable_buffer[UI_RESUSABLE_BUFFER_SIZE];
opengl_quad ui_reusable_quad;

struct ui_rect {int x, y, w, h;};

ui_rect ui_text(char *text, int x, int y) {
    bitmap img = ttf_create_bitmap(text, UI_TEXT_SIZE, 255, true, true, 200);
    if (!ui_reusable_quad.created) ui_reusable_quad.create(0,0,img.w,img.h);
    ui_reusable_quad.set_texture(img.data, img.w, img.h);
    ui_reusable_quad.set_verts(x-img.w/2, y-img.h/2, img.w, img.h);
    ui_reusable_quad.render();
    free(img.data);
    return {x-img.w/2, y-img.h/2, img.w, img.h};
}

ui_rect ui_text(char *text, float value, int x, int y) {
    sprintf(ui_text_reusable_buffer, text, value);
    return ui_text(ui_text_reusable_buffer, x, y);
}

ui_rect ui_text(char *text, float f1, float f2, int x, int y) {
    sprintf(ui_text_reusable_buffer, text, f1, f2);
    return ui_text(ui_text_reusable_buffer, x, y);
}


void ui_highlight(ui_rect r) {
    u32 white = 0xffffffff;
    if (!ui_reusable_quad.created) ui_reusable_quad.create(0,0,1,1);
    ui_reusable_quad.set_texture(&white, 1, 1);
    ui_reusable_quad.set_verts(r.x, r.y, r.w, r.h);
    ui_reusable_quad.render(0.4);
}

bool ui_mouse_over_rect(int mx, int my, ui_rect rect) {
    return mx > rect.x && mx < rect.x+rect.w && my > rect.y && my < rect.y+rect.h;
}

bool ui_button(char *text, int x, int y, Input i) {
    ui_rect button_rect = ui_text(text, x, y);
    if (ui_mouse_over_rect(i.mouseX, i.mouseY, button_rect)) {
        ui_highlight(button_rect);
        if (i.mouseL) {
            return true;
        }
    }
    return false;
}



