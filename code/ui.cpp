


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

ui_rect get_text_size(char *text) {
    // bitmap img = ttf_create_bitmap(text, UI_TEXT_SIZE, 255, true, true, 200);
    // ui_rect result = {0,0,img.w,img.h};
    // free(img.data);
    // return result;

    return ttf_render_text(text, (float)0, (float)0, ui_font_atlas, &ui_font_quad, false);
    // return {
    //     (int)floor(res.x),
    //     (int)floor(res.y),
    //     (int)ceil(res.w),
    //     (int)ceil(res.h)
    // };

}

const int UI_CENTER = 0;
const int UI_LEFT = 1;
const int UI_RIGHT = 2;
const int UI_TOP = 3;
const int UI_BOTTOM = 4;

// hpos and vpos specify whether x,y are TL, top center, or what
// note we return TL pos but take as input whatever (as specified by v/h pos)
// todo: clean up this api, add v/h centering/L/R options
ui_rect ui_text(char *text, int x, int y, int hpos=0, int vpos=3) {

    // without any changes, bb will be TL
    ttf_rect bb = ttf_render_text(text, (float)x, (float)y, ui_font_atlas, &ui_font_quad, false);

    // can't seem to get this method to turn out
    // i think the font bb metrics are way larger than we need with just ascii chars
    // or i'm scaling something wrong somewhere
    // {
    // // shift so baseline is not at text baseline but at top of highest possible char
    // int x0, y0, x1, y1;
    // stbtt_GetFontBoundingBox(&ttfont, &x0, &y0, &x1, &y1);
    // float scale = stbtt_ScaleForPixelHeight(&ttfont, UI_TEXT_SIZE);
    // int max_height = (y1-y0);
    // // y1 and y0 are signed distances from base line (- is above, + below)
    // // typically y0 is negative, y1 is positive
    // // from the docs: -y0 is the max a char could extend above baseline (scale to convert to pixels)
    // // i'm getting y0 -842 and y1 2659 for this font
    // // wouldn't the accent be a lot larger than decent?
    // y += (int)ceil(scale*(float)(-y0));
    // // DEBUGPRINT("y0 %i\n", y0);
    // // DEBUGPRINT("y1 %i\n", y1);
    // }

    int largest_ascent = find_largest_baked_ascent();
    y += largest_ascent;
    DEBUGPRINT(largest_ascent);


    // by default here x,y will be left/top
    if (hpos == UI_RIGHT) x-= bb.w;
    if (hpos == UI_CENTER) x-= bb.w/2;
    if (vpos == UI_BOTTOM) y-= UI_TEXT_SIZE;
    if (vpos == UI_CENTER) y-= UI_TEXT_SIZE/2;
    // if (vpos == UI_BOTTOM) y-= max_height;
    // if (vpos == UI_CENTER) y-= max_height/2;


    return ttf_render_text(text, (float)x, (float)y, ui_font_atlas, &ui_font_quad);
    // return {
    //     (int)floor(res.x),
    //     (int)floor(res.y),
    //     (int)ceil(res.w),
    //     (int)ceil(res.h)
    // };
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

ui_rect ui_text(char *text, float value, int x, int y, int hpos=0, int vpos=3) {
    sprintf(ui_text_reusable_buffer, text, value);
    return ui_text(ui_text_reusable_buffer, x, y, hpos, vpos);
}

ui_rect ui_text(char *text, float f1, float f2, int x, int y, int hpos=0, int vpos=3) {
    sprintf(ui_text_reusable_buffer, text, f1, f2);
    return ui_text(ui_text_reusable_buffer, x, y, hpos, vpos);
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

bool ui_button(char *text, int x, int y, Input i, int hpos=0, int vpos=3) {
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

// // todo: add proper h/v centering options to all api here
// bool ui_button_TL(char *text, int x, int y, Input i, ui_rect *out_rect = 0) {
//     ui_rect temp = ui_text(text, x, y);
//     return ui_button(text, x+temp.w/2, y+temp.h/2, i, out_rect);
// }



