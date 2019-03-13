


const int UI_TEXT_SIZE = 24;
const int UI_RESUSABLE_BUFFER_SIZE = 256;

char ui_text_reusable_buffer[UI_RESUSABLE_BUFFER_SIZE];
opengl_quad ui_reusable_quad;

// struct ui_rect {int x, y, w, h;};
#define ui_rect ttf_rect


bitmap ui_font_atlas;
opengl_quad ui_font_quad;

void ui_init(bitmap baked_font) {
    ui_font_atlas = baked_font;
}

// ui_rect get_text_size(char *text, float x = 0, float y = 0) {
//     // could certain values of x/y change our size by 1? i think so
//     ui_rect bb = ttf_render_text(text, x, y, ui_font_atlas, &ui_font_quad, false);
//     int margin = 5;
//     bb.x -= margin;
//     bb.y -= margin;
//     bb.w += margin*2;
//     bb.h += margin*1.5; // (only use half margin on bottom (recall *1 is to comp the x-=)
//     return bb;
// }


void ui_draw_rect(ui_rect r, u32 col = 0, float a = 1) {
    if (!ui_reusable_quad.created) ui_reusable_quad.create(0,0,1,1);
    ui_reusable_quad.set_texture(&col, 1, 1);
    ui_reusable_quad.set_verts(r.x, r.y, r.w, r.h);
    ui_reusable_quad.render(a);
}
void ui_highlight(ui_rect r) {
    u32 white = 0xffffffff;
    if (!ui_reusable_quad.created) ui_reusable_quad.create(0,0,1,1);
    ui_reusable_quad.set_texture(&white, 1, 1);
    ui_reusable_quad.set_verts(r.x, r.y, r.w, r.h);
    ui_reusable_quad.render(0.4);
}


const int UI_CENTER = 0;
const int UI_LEFT = 1;
const int UI_RIGHT = 2;
const int UI_TOP = 3;
const int UI_BOTTOM = 4;


// hpos and vpos specify whether x,y are TL, top/center, center/center, or what
// note we return rect with TL pos but take as input whatever (as specified by v/h pos)
ui_rect ui_text(char *text, int x, int y, int hpos=UI_LEFT, int vpos=UI_TOP, bool render = true) {

    // without any changes, bb will be TL
    ui_rect bb = ttf_render_text(text, (float)x, (float)y, ui_font_atlas, &ui_font_quad, false);
    // ui_rect bb = get_text_size(text, x, y);

    int largest_ascent = find_largest_baked_ascent();
    y += largest_ascent;
    // DEBUGPRINT(largest_ascent);

    // by default here x,y will be left/top
    if (hpos == UI_RIGHT) x -= bb.w;
    if (hpos == UI_CENTER) x -= bb.w/2;
    if (vpos == UI_BOTTOM) y -= UI_TEXT_SIZE;
    if (vpos == UI_CENTER) y -= UI_TEXT_SIZE/2;

    // space around actual letters
    int margin = 2;

    // calc rect for background
    ui_rect background_rect = {x, y-largest_ascent, bb.w, UI_TEXT_SIZE};
    background_rect.x -= margin;
    background_rect.y -= margin;
    background_rect.w += margin*2;
    background_rect.h += margin*1; // really does not seem to need... hmm

    if (render)
        ui_draw_rect(background_rect);

    ui_rect final_bb = ttf_render_text(text, (float)x, (float)(y+margin), ui_font_atlas, &ui_font_quad, render);

    return background_rect;
}


ui_rect ui_text(char *text, float value, int x, int y, int hpos=UI_LEFT, int vpos=UI_TOP) {
    sprintf(ui_text_reusable_buffer, text, value);
    return ui_text(ui_text_reusable_buffer, x, y, hpos, vpos);
}

ui_rect ui_text(char *text, float f1, float f2, int x, int y, int hpos=UI_LEFT, int vpos=UI_TOP) {
    sprintf(ui_text_reusable_buffer, text, f1, f2);
    return ui_text(ui_text_reusable_buffer, x, y, hpos, vpos);
}



bool ui_mouse_over_rect(int mx, int my, ui_rect rect) {
    return mx > rect.x && mx < rect.x+rect.w && my > rect.y && my < rect.y+rect.h;
}

bool ui_button(char *text, int x, int y, Input i, int hpos=UI_LEFT, int vpos=UI_TOP) {
    ui_rect button_rect = ui_text(text, x, y, hpos, vpos);
    // if (out_rect) *out_rect = button_rect;
    if (ui_mouse_over_rect(i.mouseX, i.mouseY, button_rect)) {
        ui_highlight(button_rect);
        if (i.mouseL) {
            return true;
        }
    }
    return false;
}



