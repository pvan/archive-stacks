


// todo: audit this api
// -float or int rects?
// -how to pass alignments in
// -rects or components for size/positions?
// -and how to pass auto w/h (most funcs defaults to this now)
// -names
// -deferred rendering probably
// -compress call chain?


const int UI_TEXT_SIZE = 24;
const int UI_RESUSABLE_BUFFER_SIZE = 256;

char ui_text_reusable_buffer[UI_RESUSABLE_BUFFER_SIZE];
opengl_quad ui_reusable_quad;

// struct ui_rect {int x, y, w, h;};
#define ui_rect ttf_rect

#define recti ui_rect

// todo: what rounding to use?
// todo: pick one and stick with it?
recti to_recti(rect r) { return {(int)r.x, (int)r.y, (int)r.w, (int)r.h}; }
rect to_rectf(recti r) { return {(float)r.x, (float)r.y, (float)r.w, (float)r.h}; }


bitmap ui_font_atlas;
opengl_quad ui_font_quad;

void ui_init(bitmap baked_font) {
    ui_font_atlas = baked_font;
}



//
// buttons / interactable elements

// todo: rename to ui_element ?
struct button {
    rect rect; // same name works huh? seems like trouble
    float z_level;
    bool highlight;

    bool is_scrollbar; // make into generic "type" var when needed
    float *callbackvalue; // so we can change value with our deferred handling
    float callbackvaluescale; // kind of a "units" factor for the callbackvalue

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



//
// store text quads

DEFINE_TYPE_POOL(gpu_quad);

gpu_quad_pool text_quads;

void ClearTextQuads() {
    text_quads.count = 0;
}

void AddDeferredTextQuads(gpu_quad *quads, int quadcount) {
    for (int i = 0; i < quadcount; i++) {
        text_quads.add(quads[i]);
    }
}




//
// store our render commands to render last after having all the elements

// todo: what language to use here? queue? command? etc?

// keeps pointer to texture so make sure not to change textures after passing them in
struct ui_deferred_quad {
    rect rect;
    bitmap img; // recall the embedded memory
    float alpha;
    bool free_mem_after_render = 0;
    bool operator==(ui_deferred_quad o) { return rect==o.rect && img.data==o.img.data && alpha==o.alpha; }
    void render() {
        // todo: consider: check if img memory is still vaid here somehow?
        if (!ui_reusable_quad.created) ui_reusable_quad.create(0,0,1,1);
        if (img.data) ui_reusable_quad.set_texture(img.data, img.w, img.h); // !img.data used intentionally in the case of invisible elements
        ui_reusable_quad.set_verts(rect.x, rect.y, rect.w, rect.h);
        ui_reusable_quad.render(alpha);
        if (free_mem_after_render) { free(img.data); } // gotta be a better way.. handle all memory outside ui_*?
    }
};

DEFINE_TYPE_POOL(ui_deferred_quad);

ui_deferred_quad_pool ui_deferred_quads;

void AddRenderQuad(rect r, bitmap img, float alpha, bool free_mem_after_render) {
    ui_deferred_quads.add({r, img, alpha, free_mem_after_render});
}

void ui_render_hl_immediately(ui_rect r) {
    u32 white = 0xffffffff;
    if (!ui_reusable_quad.created) ui_reusable_quad.create(0,0,1,1);
    ui_reusable_quad.set_texture(&white, 1, 1);
    ui_reusable_quad.set_verts(r.x, r.y, r.w, r.h);
    ui_reusable_quad.render(0.3);
}
void ui_render_hl_immediately(rect r) {
    ui_render_hl_immediately(to_recti(r));
}


void ui_RenderDeferredQuads(float mx, float my) {
    button top_most = TopMostButtonUnderPoint(mx, my);
    rect r = top_most.rect;

    for (int i = 0; i < ui_deferred_quads.count; i++) {
        ui_deferred_quads[i].render();

        // draw highlight quad here for now until we get a better system
        if (ui_deferred_quads[i].rect == r) { // should be some better way to check this
            if (r.w != 0 && r.h != 0) {
                ui_render_hl_immediately(r);
            }
        }
    }

    gpu_render_quads_with_texture(&text_quads.pool[0], text_quads.count, tf_fonttexture, 1);

}



//
//


void ui_Reset() {  // call every frame
    buttons.empty_out();
    ui_deferred_quads.empty_out();
    // buttons.count = 0; // same thing atm

    ClearTextQuads();
}

void ui_draw_rect(ui_rect r, u32 col = 0, float a = 1) {
    // have to allocate col now since we're keeping the mem for later
    // todo: free this at end of frame
    u32 *colmem = (u32*)malloc(sizeof(u32));
    *colmem = col;
    AddRenderQuad(to_rectf(r), {colmem,1,1}, a, true);
    // if (!ui_reusable_quad.created) ui_reusable_quad.create(0,0,1,1);
    // ui_reusable_quad.set_texture(&col, 1, 1);
    // ui_reusable_quad.set_verts(r.x, r.y, r.w, r.h);
    // ui_reusable_quad.render(a);
}
// void ui_highlight(ui_rect r) {
//     u32 white = 0xffffffff;
//     // have to allocate col now since we're keeping the mem for later
//     // todo: free this at end of frame
//     u32 *colmem = (u32*)malloc(sizeof(u32));
//     *colmem = white;
//     AddRenderQuad(to_rectf(r), {colmem,1,1}, 0.3, true);
//     // if (!ui_reusable_quad.created) ui_reusable_quad.create(0,0,1,1);
//     // ui_reusable_quad.set_texture(&white, 1, 1);
//     // ui_reusable_quad.set_verts(r.x, r.y, r.w, r.h);
//     // ui_reusable_quad.render(0.3);
// }


// try hl'ing at same time as render, now that it's deferred
// void ButtonsHighlight(float mx, float my) {
//     button top_most = TopMostButtonUnderPoint(mx, my);
//     rect r = top_most.rect;
//     if (r.w != 0 && r.h != 0) {

//         if (top_most.highlight)
//             ui_highlight(r);

//         if (top_most.on_mouseover)
//             top_most.on_mouseover(top_most.mouseover_arg);
//     }
// }

// store our dragging state across multiple frames
// so we can click and drag on something and move off that thing and still be dragging
bool ui_dragging = false;

// will persist over multiple frames unlike most elements, so be careful
// for example, if you are resizing scrollbars dynamically (for some reason)
// this will only be the scrollbar at the time of the drag start
button ui_drag_element = {0};

// call to update elements that respond to clicking
void ButtonsClick(float mx, float my) {
    button top_most = TopMostButtonUnderPoint(mx, my);

    if (top_most.is_scrollbar) {
        ui_drag_element = top_most;
        ui_dragging = true;
    }

    if (top_most.on_click)
        top_most.on_click(top_most.click_arg);
}

// call to update elements that respond to dragging (eg scrollbars)
void ButtonsDrag(float mx, float my, bool mouseDown) {
    if (!mouseDown) {
        ui_dragging = false;
    } else {
        if (ui_dragging) {
            float click_percent = (my - ui_drag_element.rect.y) / ui_drag_element.rect.h;
            // consider: pass function pointer instead of scale factor?
            // or some other alternative?
            *(ui_drag_element.callbackvalue) = ui_drag_element.callbackvaluescale * click_percent;
        }
    }
}


// should call this addbutton callback maybe?
void AddButton(rect r, bool hl, void(*on_click)(int),int click_arg, void(*on_mouseover)(int)=0,int mouseover_arg=0)
{
    buttons.add({r,1, hl, false,0,0, on_click,click_arg, on_mouseover,mouseover_arg});
    // if (!buttons)                   { buttonAlloc=256; buttons = (Button*)malloc(          buttonAlloc * sizeof(Button)); }
    // if (buttonCount >= buttonAlloc) { buttonAlloc*=2;  buttons = (Button*)realloc(buttons, buttonAlloc * sizeof(Button)); }
    // buttons[buttonCount++] = {r,z,  on_click,click_arg,  on_mouseover,mouseover_arg};
}

// void AddImageButton(rect r, bitmap img, void(*on_click)(int),int click_arg) {
//     buttons.add({r, 1, true, false,0,0, on_click, click_arg, 0,0});
//     AddRenderQuad(r, img, 1/*alpha*/, false);
// }

void AddScrollbar(rect r, bool hl, float *callbackvalue, float callbackscale) {
    buttons.add({r,1, hl, true,callbackvalue,callbackscale, 0,0, 0,0});
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

    // old
    // ui_rect final_bb = ttf_render_text(text, (float)x, (float)(y+margin), ui_font_atlas, &ui_font_quad, render);

    // todo: where to put this allocation? up one level?
    // if we leave it here, why not just nest these into one new api call
    int quadsneeded = tf_how_many_quads_needed_for_text(text);
    gpu_quad *quads = (gpu_quad*)malloc(quadsneeded*sizeof(gpu_quad));
    tf_create_quad_list_for_text_at_rect(text, (float)x,(float)(y+margin), quads, quadsneeded);

    AddDeferredTextQuads(quads, quadsneeded);

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




rect ui_button(char *text, float x, float y, bool hpos, bool vpos, void(*effect)(int), int arg=0)
{
    ttf_rect tr = ui_text(text, x, y, hpos, vpos); //RenderTextCenter(x, y, text);
    rect r = {(float)tr.x, (float)tr.y, (float)tr.w, (float)tr.h};
    AddButton(r, true, effect, arg);
    return r;
}

// kind of a hack so we can highlight tiles without bringing them into this system
// (they have their own opengl_quads so they don't re-send textures to the gpu every frame)
rect ui_button_invisible_highlight(rect br, void(*effect)(int), int arg=0)
{
    // ttf_rect tr = ui_text("#", br.x, br.y, true, true); //RenderTextCenter(x, y, text);
    AddButton(br, true, effect, arg);
    AddRenderQuad(br, {0}, 0, false); // 0 alpha, button handles any highlighting
    return br;
}

// // created for drawing tiles basically
// void ui_draw_bitmap_button(rect r, bitmap img, void(*effect)(int), int arg=0) {
//     // use this after removing the _invisible buttons on every tile
//     // AddImageButton(r, img, effect, arg);

//     AddRenderQuad(r, img, 1/*alpha*/, false);
// }

// rect ui_button(char *text, rect br, bool hpos, bool vpos, void(*effect)(int), int arg=0)
// {
//     // todo: test all hpos/vpos paths here, i dont think this will work for all
//     ttf_rect tr = ui_text(text, br.x, br.y, hpos, vpos); //RenderTextCenter(x, y, text);
//     // rect r = {(float)rr.x, (float)rr.y, (float)rr.w, (float)rr.h};
//     AddButton(br, effect, arg);
//     return br;
// }



// note takes two values, for top/bottom of scroll bar indicator (for variable size)
void ui_scrollbar(recti r, float top_percent, float bot_percent,
                  float *callbackvalue, float callbackscale,
                  void(*effect)(int), int arg=0)
{
    // bg
    ui_draw_rect(r, 0xffbbbbbb, .4); //0xffbbbbbb, .35

    float top_pixels = top_percent * (float)r.h;
    float bot_pixels = bot_percent * (float)r.h;

    float size = roundf(bot_pixels-top_pixels);
    if (size < 10) size = 10;

    // indicator
    // todo: round?
    ui_draw_rect({r.x, (int)top_pixels, r.w, (int)size}, 0xffdddddd, .9); //0xff888888, .75
    // ui_draw_rect({r.x+1, (int)top_pixels+1, r.w-2, (int)size-2}, 0xff888888, .9); //, .75

    // click handler
    AddScrollbar(to_rectf(r), true, callbackvalue, callbackscale);
}



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





