


const int UI_TEXT_SIZE = 24;
const int UI_RESUSABLE_BUFFER_SIZE = 256;

char ui_text_reusable_buffer[UI_RESUSABLE_BUFFER_SIZE];
opengl_quad ui_reusable_quad;

struct ui_rect {int x, y, w, h;};


bitmap ui_font_atlas;
opengl_quad ui_font_quad;

void ui_init(bitmap baked_font) {
    ui_font_atlas = baked_font;
}

ui_rect get_text_size(char *text) {
    bitmap img = ttf_create_bitmap(text, UI_TEXT_SIZE, 255, true, true, 200);
    ui_rect result = {0,0,img.w,img.h};
    free(img.data);
    return result;
}


// note we return TL pos but take as input the center
// todo: clean up this api, add v/h centering/L/R options
ui_rect ui_text(char *text, int x, int y) {

    ttf_rect bb = ttf_render_text(text, (float)x, float(y), ui_font_atlas, &ui_font_quad, false);
    // if (hCenter == ui_hpos::right) x-= bb.w;
    // if (hCenter == ui_hpos::center) x-= bb.w/2;
    // if (vCenter == ui_vpos::top) y-= bb.h;
    // if (vCenter == ui_vpos::center) y+= bb.h/2;
    ttf_rect res = ttf_render_text(text, (float)x, float(y), ui_font_atlas, &ui_font_quad);
    return {
        (int)floor(res.x),
        (int)floor(res.y),
        (int)ceil(res.w),
        (int)ceil(res.h)
    };
    // return {x, y, text_width, UI_TEXT_SIZE};

    // bitmap img = ttf_create_bitmap(text, UI_TEXT_SIZE, 255, true, true, 200);
    // if (!ui_reusable_quad.created) ui_reusable_quad.create(0,0,img.w,img.h);
    // ui_reusable_quad.set_texture(img.data, img.w, img.h);
    // if (hCenter) x-= img.w/2;
    // if (vCenter) y-= img.h/2;
    // ui_reusable_quad.set_verts(x, y, img.w, img.h);
    // ui_reusable_quad.render();
    // free(img.data);
    // return {x, y, img.w, img.h};
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
    // if (out_rect) *out_rect = button_rect;
    if (ui_mouse_over_rect(i.mouseX, i.mouseY, button_rect)) {
        ui_highlight(button_rect);
        if (i.mouseL) {
            return true;
        }
    }
    return false;
}

// // todo: add proper h/v centering options to all api here
// bool ui_button_TL(char *text, int x, int y, Input i, ui_rect *out_rect = 0) {
//     ui_rect temp = ui_text(text, x, y);
//     return ui_button(text, x+temp.w/2, y+temp.h/2, i, out_rect);
// }



