


const int UI_TEXT_SIZE = 24;
const int UI_RESUSABLE_BUFFER_SIZE = 256;

char ui_text_reusable_buffer[UI_RESUSABLE_BUFFER_SIZE];
opengl_quad ui_reusable_quad;

// struct ui_rect {int x, y, w, h;};
#define ui_rect ttf_rect

#define recti ui_rect

// todo: what rounding to use?
recti to_recti(rect r) { return {(int)r.x, (int)r.y, (int)r.w, (int)r.h}; }


bitmap ui_font_atlas;
opengl_quad ui_font_quad;

void ui_init(bitmap baked_font) {
    ui_font_atlas = baked_font;
}


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
    ui_reusable_quad.render(0.3);
}
void ui_highlight(rect r) {
    ui_highlight(to_recti(r));
}

//
// buttons

struct button {
    rect rect;
    float z_level;
    bool highlight;

    void (*on_click)(int);
    int click_arg;

    void (*on_mouseover)(int);
    int mouseover_arg;

    bool operator==(button o) {
        return rect==o.rect &&
               z_level==o.z_level &&

               on_click==o.on_click &&
               click_arg==o.click_arg &&

               on_mouseover==o.on_mouseover &&
               mouseover_arg==o.mouseover_arg;
   }
};
// button *buttons = 0;
// int buttonCount = 0;
// int buttonAlloc = 0;

DEFINE_TYPE_POOL(button);

button_pool buttons;

void ButtonsReset() {  // call every frame
    buttons.empty_out();
    // buttons.count = 0; // same thing atm
}

button TopMostButtonUnderPoint(float mx, float my) {
    button result = {0};
    result.z_level = -99999; // max negative z level
    bool found_at_least_one = false;
    for (int i = 0; i < buttons.count; i++) {
        rect r = buttons[i].rect;
        if (mx > r.x && mx <= r.x+r.w && my > r.y && my <= r.y+r.h) {
            // if (buttons[i].z_level > result.z_level) {
                // note we keep checking the whole list, so we implicitly get the last/topmost rect
                result = buttons[i];
                found_at_least_one = true;
            // }
        }
    }
    if (found_at_least_one)
        return result;
    else
        return {0};
}

// void DefaultButtonHighlight(int pointer_to_rect_will_this_even_work) {
//     rect *r = (rect*)pointer_to_rect_will_this_even_work;
//     ui_highlight(to_recti(*r));
// }

void ButtonsHighlight(float mx, float my) {
    button top_most = TopMostButtonUnderPoint(mx, my);
    rect r = top_most.rect;
    if (r.w != 0 && r.h != 0) {

        if (top_most.highlight)
            ui_highlight(r);

        if (top_most.on_mouseover)
            top_most.on_mouseover(top_most.mouseover_arg);
    }
}

void ButtonsClick(float mx, float my) {
    button top_most = TopMostButtonUnderPoint(mx, my);
    if (top_most.on_click)
        top_most.on_click(top_most.click_arg);
}

void AddButton(rect r, bool hl, void(*on_click)(int),int click_arg, void(*on_mouseover)(int)=0,int mouseover_arg=0)
{
    buttons.add({r,1, hl, on_click,click_arg, on_mouseover,mouseover_arg});
    // if (!buttons)                   { buttonAlloc=256; buttons = (Button*)malloc(          buttonAlloc * sizeof(Button)); }
    // if (buttonCount >= buttonAlloc) { buttonAlloc*=2;  buttons = (Button*)realloc(buttons, buttonAlloc * sizeof(Button)); }
    // buttons[buttonCount++] = {r,z,  on_click,click_arg,  on_mouseover,mouseover_arg};
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




const int UI_CENTER = 0;
const int UI_LEFT = 1;
const int UI_RIGHT = 2;
const int UI_TOP = 3;
const int UI_BOTTOM = 4;


// hpos and vpos specify whether x,y are TL, top/center, center/center, or what
// note we return rect with TL pos but take as input whatever (as specified by v/h pos)
ui_rect ui_text(char *text, int x, int y, int hpos, int vpos, bool render = true) {

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


ui_rect ui_texti(char *text, int value, int x, int y, int hpos, int vpos) {
    sprintf(ui_text_reusable_buffer, text, value);
    return ui_text(ui_text_reusable_buffer, x, y, hpos, vpos);
}
ui_rect ui_texti(char *text, int v1, int v2, int x, int y, int hpos, int vpos) {
    sprintf(ui_text_reusable_buffer, text, v1, v2);
    return ui_text(ui_text_reusable_buffer, x, y, hpos, vpos);
}

ui_rect ui_textf(char *text, float value, int x, int y, int hpos, int vpos) {
    sprintf(ui_text_reusable_buffer, text, value);
    return ui_text(ui_text_reusable_buffer, x, y, hpos, vpos);
}
ui_rect ui_textf(char *text, float f1, float f2, int x, int y, int hpos, int vpos) {
    sprintf(ui_text_reusable_buffer, text, f1, f2);
    return ui_text(ui_text_reusable_buffer, x, y, hpos, vpos);
}



bool ui_mouse_over_rect(int mx, int my, ui_rect rect) {
    return mx > rect.x && mx < rect.x+rect.w && my > rect.y && my < rect.y+rect.h;
}

// bool ui_button(char *text, int x, int y, Input i, int hpos, int vpos) {
//     ui_rect button_rect = ui_text(text, x, y, hpos, vpos);
//     // if (out_rect) *out_rect = button_rect;
//     if (ui_mouse_over_rect(i.mouseX, i.mouseY, button_rect)) {
//         ui_highlight(button_rect);
//         if (i.mouseL) {
//             return true;
//         }
//     }
//     return false;
// }




// todo: audit this api

rect ui_button(char *text, float x, float y, bool hpos, bool vpos, void(*effect)(int), int arg=0)
{
    ttf_rect tr = ui_text(text, x, y, hpos, vpos); //RenderTextCenter(x, y, text);
    rect r = {(float)tr.x, (float)tr.y, (float)tr.w, (float)tr.h};
    AddButton(r, true, effect, arg);
    return r;
}

rect ui_button_invisible(rect br, void(*effect)(int), int arg=0)
{
    AddButton(br, true, effect, arg);
    return br;
}

// rect ui_button(char *text, rect br, bool hpos, bool vpos, void(*effect)(int), int arg=0)
// {
//     // todo: test all hpos/vpos paths here, i dont think this will work for all
//     ttf_rect tr = ui_text(text, br.x, br.y, hpos, vpos); //RenderTextCenter(x, y, text);
//     // rect r = {(float)rr.x, (float)rr.y, (float)rr.w, (float)rr.h};
//     AddButton(br, effect, arg);
//     return br;
// }


char ui_log_reuseable_mem[256];
int ui_log_count;
void UI_PRINT(char *s) {
    ui_text(s, 0, ui_log_count*UI_TEXT_SIZE, UI_LEFT,UI_TOP);
    ui_log_count++;
}
void UI_PRINT(u64 i)               { sprintf(ui_log_reuseable_mem, "%lli", i); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINT(int i)               { sprintf(ui_log_reuseable_mem, "%i", i); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINT(float f)             { sprintf(ui_log_reuseable_mem, "%f", f); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINT(char *text, int i)   { sprintf(ui_log_reuseable_mem, text, i); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINT(char *text, i64 i)   { sprintf(ui_log_reuseable_mem, text, i); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINT(char *text, char *s) { sprintf(ui_log_reuseable_mem, text, s); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINT(char *text, float f) { sprintf(ui_log_reuseable_mem, text, f); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINT(char *text, float x, float y) { sprintf(ui_log_reuseable_mem, text, x,y); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINTTIME(u64 i) { FormatTimeIntoCharBuffer(i, ui_log_reuseable_mem); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINTRESET() { ui_log_count = 0; }





