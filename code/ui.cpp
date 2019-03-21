


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



bitmap ui_font_atlas;
opengl_quad ui_font_quad;

void ui_init(bitmap baked_font, gpu_texture_id atlas_tex_id) {
    ui_font_atlas = baked_font;


    // force change last pixel to white and reupload
    // we use this pixel when drawing solid-color quads
    // (so we can use the same texture as our letters)
    // uv for this would be 511/512
    // todo: magic numbers
    baked_font.data[512*512-1] = 0xffffffff;
    gpu_upload_texture(baked_font, atlas_tex_id);
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
// store quads for rects (bg or hl mostly)

// DEFINE_TYPE_POOL(u32);

// gpu_quad_pool rect_quads;
// u32_pool rect_colors;

// void ClearRectQuads() {
//     rect_quads.count = 0;
// }

void AddDeferredRectQuad(gpu_quad quad) {
    text_quads.add(quad);
    // rect_quads.add(quad);
    // rect_colors.add(col);
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

// void ui_render_hl_immediately(rect r) {
//     u32 white = 0xffffffff;
//     if (!ui_reusable_quad.created) ui_reusable_quad.create(0,0,1,1);
//     ui_reusable_quad.set_texture(&white, 1, 1);
//     ui_reusable_quad.set_verts(r.x, r.y, r.w, r.h);
//     ui_reusable_quad.render(0.3);
// }
// void ui_render_hl_immediately(rect r) {
//     ui_render_hl_immediately(r);
// }


void ui_RenderDeferredQuads(float mx, float my) {
    button top_most = TopMostButtonUnderPoint(mx, my);
    rect r = top_most.rect;

    for (int i = 0; i < ui_deferred_quads.count; i++) {
        ui_deferred_quads[i].render();

    //     // draw highlight quad here for now until we get a better system
    //     if (ui_deferred_quads[i].rect == r) { // should be some better way to check this
    //         if (r.w != 0 && r.h != 0) {
    //             ui_render_hl_immediately(r);
    //         }
    //     }
    }


    // change alpha of the hidden quad above our mouse
    // go in reverse and stop at the first one,
    // should give us only the topmost quad
    for (int i = text_quads.count-1; i >= 0; i--) {
        if (r.x == text_quads[i].x0 && r.y == text_quads[i].y0 &&
            r.w == text_quads[i].x1-text_quads[i].x0 &&
            r.h == text_quads[i].y1-text_quads[i].y0)
        {
            text_quads[i].alpha = 0.3;
            break;
        }
    }

    // change color of highlighted

    gpu_render_quads_with_texture(&text_quads.pool[0], text_quads.count, tf_fonttexture, 1);

    // for (int i = 0; i < rect_quads.count; i++) {
    //     // eventually could use col attrib for colors

    //     gpu_render_quads_with_texture(&rect_quads.pool[i], 1, tf_fonttexture, 1);
    //     // gpu_quad_pool rect_quads;
    //     // u32_pool rect_colors;
    // }

}



//
//


void ui_Reset() {  // call every frame
    buttons.empty_out();
    ui_deferred_quads.empty_out();
    // buttons.count = 0; // same thing atm

    ClearTextQuads();
    // ClearRectQuads();
}

void ui_draw_rect(rect r, u32 col = 0, float a = 1) {
    // have to allocate col now since we're keeping the mem for later
    // todo: free this at end of frame
    u32 *colmem = (u32*)malloc(sizeof(u32));
    *colmem = col;
    AddRenderQuad(r, {colmem,1,1}, a, true);
    // if (!ui_reusable_quad.created) ui_reusable_quad.create(0,0,1,1);
    // ui_reusable_quad.set_texture(&col, 1, 1);
    // ui_reusable_quad.set_verts(r.x, r.y, r.w, r.h);
    // ui_reusable_quad.render(a);
}

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

void AddScrollbar(rect r, bool hl, float *callbackvalue, float callbackscale) {
    buttons.add({r,1, hl, true,callbackvalue,callbackscale, 0,0, 0,0});
}





const int UI_CENTER = 0;
const int UI_LEFT = 1;
const int UI_RIGHT = 2;
const int UI_TOP = 3;
const int UI_BOTTOM = 4;


// hpos and vpos specify whether x,y are TL, top/center, center/center, or what
// note we return rect with TL pos but take as input whatever (as specified by v/h pos)
rect ui_text(char *text, float x, float y, int hpos, int vpos, bool render = true) {

    // without any changes, bb will be TL
    rect bb = tf_text_bounding_box(text, x, y);

    // by default here x,y will be left/top
    if (hpos == UI_RIGHT) x -= bb.w;
    if (hpos == UI_CENTER) x -= bb.w/2;
    if (vpos == UI_BOTTOM) y -= UI_TEXT_SIZE;
    if (vpos == UI_CENTER) y -= UI_TEXT_SIZE/2;


    // todo: find root cause of this bug
    // it's ugly but if x is on an exact 0.5 edge,
    // we seem to get inconsistent rounding somewhere in our rendering pipeline
    // it wouldn't matter except we draw the same quad twice -- once for the highlight
    // and if they round differently.. we'll end up with an extra pixel bar
    // so in that case, add a fudge factor
    if (x-0.5 == (int)x) {
        x+=0.01;
        // assert(false);
    }


    // space around actual letters
    int margin = 2;


    gpu_quad bg_quad;
    bg_quad.x0 = x                - margin;
    bg_quad.y0 = y                - margin;
    bg_quad.x1 = x + bb.w         + margin;
    bg_quad.y1 = y + UI_TEXT_SIZE + margin;
    bg_quad.u0 = 1/512.0;
    bg_quad.v0 = 2/512.0;
    bg_quad.u1 = 1/512.0;
    bg_quad.v1 = 2/512.0;


    // bg
    if (render) {
        bg_quad.alpha = 1;
        AddDeferredRectQuad(bg_quad);
    }


    // text
    if (render) {
        y += tf_cached_largest_ascent; // move y to top instead of baseline of text

        int quadsneeded = tf_how_many_quads_needed_for_text(text);
        gpu_quad *quads = (gpu_quad*)malloc(quadsneeded*sizeof(gpu_quad));
        tf_create_quad_list_for_text_at_rect(text, x,y+margin, quads, quadsneeded);

        AddDeferredTextQuads(quads, quadsneeded);

        free(quads);
    }


    // highlight
    if (render) {
        // seems like a terrible way to do this,
        // but just add an invisible rect above every text
        // and if highlighted (checked when rendering), change the alpha up from 0
        gpu_quad invisible_hl_quad = bg_quad;
        invisible_hl_quad.alpha = 0; // will get changed if the highlight quad in ui_RenderDeferredQuads
        // change uv to bottom right pixel of font atlas (specially overridden to be white)
        invisible_hl_quad.u0 = 511.0/512.0;
        invisible_hl_quad.v0 = 511.0/512.0;
        invisible_hl_quad.u1 = 1.0;
        invisible_hl_quad.v1 = 1.0;
        AddDeferredRectQuad(invisible_hl_quad);
    }

    return bg_quad.to_rect();
}


rect ui_texti(char *text, int value, int x, int y, int hpos, int vpos) {
    sprintf(ui_text_reusable_buffer, text, value);
    return ui_text(ui_text_reusable_buffer, x, y, hpos, vpos);
}
rect ui_texti(char *text, int v1, int v2, int x, int y, int hpos, int vpos) {
    sprintf(ui_text_reusable_buffer, text, v1, v2);
    return ui_text(ui_text_reusable_buffer, x, y, hpos, vpos);
}

rect ui_textf(char *text, float value, int x, int y, int hpos, int vpos) {
    sprintf(ui_text_reusable_buffer, text, value);
    return ui_text(ui_text_reusable_buffer, x, y, hpos, vpos);
}
rect ui_textf(char *text, float f1, float f2, int x, int y, int hpos, int vpos) {
    sprintf(ui_text_reusable_buffer, text, f1, f2);
    return ui_text(ui_text_reusable_buffer, x, y, hpos, vpos);
}



bool ui_mouse_over_rect(int mx, int my, rect rect) {
    return mx > rect.x && mx < rect.x+rect.w && my > rect.y && my < rect.y+rect.h;
}




rect ui_button(char *text, float x, float y, bool hpos, bool vpos, void(*effect)(int), int arg=0)
{
    rect tr = ui_text(text, x, y, hpos, vpos); //RenderTextCenter(x, y, text);
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


// note takes two values, for top/bottom of scroll bar indicator (for variable size)
void ui_scrollbar(rect r, float top_percent, float bot_percent,
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
    ui_draw_rect({r.x, top_pixels, r.w, size}, 0xffdddddd, .9); //0xff888888, .75
    // ui_draw_rect({r.x+1, (int)top_pixels+1, r.w-2, (int)size-2}, 0xff888888, .9); //, .75

    // click handler
    AddScrollbar(r, true, callbackvalue, callbackscale);
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





