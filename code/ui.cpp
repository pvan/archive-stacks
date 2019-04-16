


// todo: audit this api
// -float or int rects? floats, but some bugs see zxoi (fixed?)
// -how to pass alignments in? booleans for now
// -rects or components for size/positions? whatever is more convenient for caller (using more rects now instead)
// -and how to pass auto w/h (most funcs defaults to this now) reexamine this
// -names? mostly done, needs another pass or two
// -deferred rendering probably. done. undone.
// -compress call chain? yes, still needs (better now)

//
// this module is meant to be used by our main app for any gui, shape, text drawing, etc
// it's a layer between the main app and lower level modules like text (tf_*) and gpu (gpu_*)
// which may also use even lower level constructs like bitmap and rect
//
//
// we've now converted to a pretty standard immediate-mode structure
// instead of deferring all events and drawing to the end of the frame,
// we just use a 1-frame lag to find (eg) the topmost control
//
// state of what is active/hot is stored in ui_context
//
// api functions come in two flavors: render only (no behavior) and id-needing (behavior)
// render only will just draw something to the screen immediately
// id-needing will have behavior and require passing in a unqiue ui_id to track over multiple frames
//

const int UI_TEXT_SIZE = 24;

const int UI_RESUSABLE_BUFFER_SIZE = 256;
char ui_text_reusable_buffer[UI_RESUSABLE_BUFFER_SIZE];

gpu_texture_id ui_solid_tex_id;
u32 white = 0xffffffff;
bitmap ui_solid_bitmap;

void ui_create_solid_color_texture_and_upload()
{
    // ui_solid_bitmap.data = (u32*)malloc(2 * sizeof(u32));
    // ui_solid_bitmap.data[0] = 0;
    // ui_solid_bitmap.data[1] = 0xffffffff;
    // ui_solid_bitmap.w = 2;
    // ui_solid_bitmap.h = 1;
    ui_solid_bitmap = { &white, 1, 1 };

    ui_solid_tex_id = gpu_create_texture();
    gpu_upload_texture(ui_solid_bitmap, ui_solid_tex_id);
}


//
// id system

// we use the concept of an id to track controls from one frame to another
// use something unique but that won't change frame-to-frame
// could use struct like { void *ptr; int index; etc } if need more to uniquely id things
#define ui_id void*

// the global state for this lib
// here we track what controls are active
// so we can, eg, ignore events on other controls (eg textbox)
struct ui_context {
    ui_id hot;    //hover
    ui_id active; //selected (holding focus)
    ui_id next_hot; // track what will be hot for next frame, so we always get the topmost
    // todo: should probably add textbox state to this if we want it to be complete
};

ui_context ui_context;

void ui_set_hot(ui_id id) {
    // ui_context.hot = id; // 1 frame of lag now
    ui_context.next_hot = id;
}
void ui_set_active(ui_id id) {
    ui_context.active = id;
}

bool ui_active(ui_id id) { return ui_context.active==id; }
bool ui_hot(ui_id id) { return ui_context.hot==id; }


//
// intended exposed api

// call once at startup
void ui_init() {
    ui_create_solid_color_texture_and_upload();
}

// need to call this every frame to update what's hot (pretty much just to get accurate topmost control)
void ui_update() {
    ui_context.hot = ui_context.next_hot;
    ui_context.next_hot = 0;
}


const int UI_CENTER = 0;
const int UI_LEFT = 1;
const int UI_RIGHT = 2;
const int UI_TOP = 3;
const int UI_BOTTOM = 4;


//
// pure render functions (no effects / no ids needed)
// todo: use prefix ui_draw_* for all these?

void ui_rect(rect r, u32 color, float alpha) {
    gpu_quad quad = gpu_quad_from_rect(r, color, alpha);
    gpu_render_quads_with_texture(&quad, 1, ui_solid_tex_id);
}

// hpos and vpos specify whether x,y are TL, top/center, center/center, or what
// note we return rect with TL pos but take as input whatever (as specified by v/h pos)
rect ui_text(char *text, rect r, int hpos, int vpos, bool render, float bgalpha, u32 textcol = 0xffffffff) {

    // without any changes, bb will be TL
    rect textbb = tf_text_bounding_box(text, r.x, r.y);

    // pad beyond just our letter-tight bb
    int margin = 2;

    // bg (and hl) quad
    // setup for TL coords as default
    gpu_quad bg_quad;
    bg_quad.x0 = r.x;
    bg_quad.y0 = r.y;
    bg_quad.x1 = r.x + textbb.w+1 + margin*2;  // note fencepost error with w/h.. don't include last edge
    bg_quad.y1 = r.y + tf_cached_largest_total_height+1 + margin*2; // todo: check if correct

    bg_quad.y1 -= 1; // a little hand-tweaking (tbh not sure if +1 in the y1 above is correct, x1 def correct tho)

    // adjust for alignment
    if (hpos == UI_RIGHT) bg_quad.move(-bg_quad.width(), 0);
    if (hpos == UI_CENTER) bg_quad.move(-bg_quad.width()/2, 0);
    if (vpos == UI_BOTTOM) bg_quad.move(0, -bg_quad.height());
    if (vpos == UI_CENTER) bg_quad.move(0, -bg_quad.height()/2);

    // text position (inside the larger quad)
    float textx = bg_quad.x0 + margin;
    float texty = bg_quad.y0 + margin + tf_cached_largest_ascent; // text is drawn from the baseline todo: let text module handle this offset?



    if (render) {
        //--bg--
        ui_rect(bg_quad.to_rect(), 0x0, bgalpha);

        //--text--
        if (strlen(text) > 0) { // don't bother if we don't have text
            int quadsneeded = tf_how_many_quads_needed_for_text(text);
            gpu_quad *quads = (gpu_quad*)malloc(quadsneeded*sizeof(gpu_quad));
                tf_create_quad_list_for_text_at_rect(text, textx,texty, quads, quadsneeded, textcol);
                gpu_render_quads_with_texture(quads, quadsneeded, tf_fonttexture, 1/*alpha*/);
            free(quads);
        }
    }

    return bg_quad.to_rect();
}


rect ui_texti(char *text, int value, rect r, int hpos, int vpos, bool render, float bgalpha, u32 textcol = 0xffffffff) {
    sprintf(ui_text_reusable_buffer, text, value);
    return ui_text(ui_text_reusable_buffer, r, hpos, vpos, render, bgalpha, textcol);
}
rect ui_textii(char *text, int v1, int v2, rect r, int hpos, int vpos, bool render, float bgalpha, u32 textcol = 0xffffffff) {
    sprintf(ui_text_reusable_buffer, text, v1, v2);
    return ui_text(ui_text_reusable_buffer, r, hpos, vpos, render, bgalpha, textcol);
}


// semi-debugish function
// will re-upload texture to gpu when called
gpu_texture_id reusable_texture_id_for_bitmap = -1;
void ui_bitmap(bitmap img, float x, float y) {

    if (reusable_texture_id_for_bitmap == -1) {
        reusable_texture_id_for_bitmap = gpu_create_texture();
    }

    gpu_upload_texture(img, reusable_texture_id_for_bitmap);

    gpu_quad quad = gpu_quad_from_rect({0,0, (float)img.w, (float)img.h}, 0xffffffff, 1);
    gpu_render_quads_with_texture(&quad, 1, reusable_texture_id_for_bitmap, 1);
}


//
// controls with effects, ids needed to track over multiple frames

bool ui_button(ui_id id, rect r) {

    bool result = false;
    if (ui_active(id)) {
        if (input.up.mouseL) {
            if (ui_hot(id)) result = true;
            ui_set_active(0);
        }
    } else {
        if (ui_hot(id)) {
            if (input.down.mouseL)
                ui_set_active(id);
        }
    }

    if (ui_hot(id)) {
        ui_rect(r, 0xffffffff, 0.3);
    }

    if (ui_active(id)) {
        ui_rect(r, 0xff777700, 0.3);
    }

    bool mouseOver = r.ContainsPoint(input.current.mouseX,input.current.mouseY);
    if (mouseOver && (ui_active(id) || ui_active(0))) ui_set_hot(id);
    if (!mouseOver && ui_hot(id)) ui_set_hot(0);

    return result;
}

bool ui_button_text(ui_id id, char *text, rect r, int hpos, int vpos, rect *outrect)
{
    r = ui_text(text, r, hpos, vpos, true, 0.66);
    if (outrect) *outrect = r;
    return ui_button(id, r);
}

bool ui_button_rect(ui_id id, rect r, u32 color, float alpha) {
    ui_rect(r, color, alpha);
    return ui_button(id, r);
}


// note takes two values, for top/bottom of scroll bar indicator (for variable size)
void ui_scrollbar(ui_id id,
                  rect r, float top_percent, float bot_percent,
                  float *callbackvalue, float callbackscale)
{

    float bgalpha = 0.4;
    u32 bgcolor = 0xffbbbbbb;
    u32 indicatorcolor = 0xffeeeeeeee;

    if (ui_active(id)) {
        float click_percent = (input.current.mouseY - r.y) / r.h;
        // consider: pass function pointer instead of scale factor?
        // or some other alternative?
        if (callbackvalue) *callbackvalue = callbackscale * click_percent;
        if (input.up.mouseL) {
            ui_set_active(0);
        } else {
            indicatorcolor = 0xffff77ff;
        }
    } else {
        if (ui_hot(id)) {
            bgcolor = 0xff777777;
            bgalpha = 0.7;
            if (input.down.mouseL) {
                ui_set_active(id);
            }
        }
    }

    // --bg--
    ui_rect(r, bgcolor, bgalpha);

    float top_pixels = top_percent * (float)r.h;
    float bot_pixels = bot_percent * (float)r.h;

    float size = roundf(bot_pixels-top_pixels);
    if (size < 10) size = 10;

    // --indicator--
    ui_rect({r.x, top_pixels, r.w, size}, indicatorcolor, 0.9);

    // --active color--
    if (ui_active(id)) {
        ui_rect(r, 0xff777700, 0.3);
    }

    bool mouseOver = r.ContainsPoint(input.current.mouseX,input.current.mouseY);
    if (mouseOver && (ui_active(id) || ui_active(0))) ui_set_hot(id);
    if (!mouseOver && ui_hot(id)) ui_set_hot(0);
}



float ui_cursor_blink = 0;
float ui_cursor_blink_ms = 700;
int ui_cursor_pos = 0;
void ui_textbox(ui_id id, string *text, rect r, float dt) {

    ui_cursor_blink += dt;
    if (ui_cursor_blink > ui_cursor_blink_ms)
        ui_cursor_blink -= ui_cursor_blink_ms;

    if (ui_active(id)) {
        // typing text
        if (!input.current.shift) {
            if (input.down.q) { text->insert(L'q', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.w) { text->insert(L'w', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.e) { text->insert(L'e', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.r) { text->insert(L'r', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.t) { text->insert(L't', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.y) { text->insert(L'y', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.u) { text->insert(L'u', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.i) { text->insert(L'i', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.o) { text->insert(L'o', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.p) { text->insert(L'p', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.a) { text->insert(L'a', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.s) { text->insert(L's', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.d) { text->insert(L'd', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.f) { text->insert(L'f', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.g) { text->insert(L'g', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.h) { text->insert(L'h', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.j) { text->insert(L'j', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.k) { text->insert(L'k', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.l) { text->insert(L'l', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.z) { text->insert(L'z', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.x) { text->insert(L'x', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.c) { text->insert(L'c', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.v) { text->insert(L'v', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.b) { text->insert(L'b', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.n) { text->insert(L'n', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.m) { text->insert(L'm', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.squareL   ) { text->insert(L'[', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.squareR   ) { text->insert(L']', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.bslash    ) { text->insert(L'\\', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.semicolon ) { text->insert(L';', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.apostrophe) { text->insert(L'\'', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.comma     ) { text->insert(L',', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.period    ) { text->insert(L'.', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.fslash    ) { text->insert(L'/', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.space     ) { text->insert(L' ', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.tilde     ) { text->insert(L'`', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[1] ) { text->insert(L'1', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[2] ) { text->insert(L'2', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[3] ) { text->insert(L'3', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[4] ) { text->insert(L'4', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[5] ) { text->insert(L'5', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[6] ) { text->insert(L'6', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[7] ) { text->insert(L'7', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[8] ) { text->insert(L'8', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[9] ) { text->insert(L'9', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[10]) { text->insert(L'0', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[11]) { text->insert(L'-', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[12]) { text->insert(L'=', ui_cursor_pos); ui_cursor_pos++; }
        } else {
            if (input.down.q) { text->insert(L'Q', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.w) { text->insert(L'W', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.e) { text->insert(L'E', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.r) { text->insert(L'R', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.t) { text->insert(L'T', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.y) { text->insert(L'Y', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.u) { text->insert(L'U', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.i) { text->insert(L'I', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.o) { text->insert(L'O', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.p) { text->insert(L'P', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.a) { text->insert(L'A', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.s) { text->insert(L'S', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.d) { text->insert(L'D', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.f) { text->insert(L'F', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.g) { text->insert(L'G', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.h) { text->insert(L'H', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.j) { text->insert(L'J', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.k) { text->insert(L'K', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.l) { text->insert(L'L', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.z) { text->insert(L'Z', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.x) { text->insert(L'X', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.c) { text->insert(L'C', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.v) { text->insert(L'V', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.b) { text->insert(L'B', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.n) { text->insert(L'N', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.m) { text->insert(L'M', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.squareL   ) { text->insert(L'{', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.squareR   ) { text->insert(L'}', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.bslash    ) { text->insert(L'|', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.semicolon ) { text->insert(L':', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.apostrophe) { text->insert(L'"', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.comma     ) { text->insert(L'<', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.period    ) { text->insert(L'>', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.fslash    ) { text->insert(L'?', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.space     ) { text->insert(L' ', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.tilde     ) { text->insert(L'~', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[1] ) { text->insert(L'!', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[2] ) { text->insert(L'@', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[3] ) { text->insert(L'#', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[4] ) { text->insert(L'$', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[5] ) { text->insert(L'%', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[6] ) { text->insert(L'^', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[7] ) { text->insert(L'&', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[8] ) { text->insert(L'*', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[9] ) { text->insert(L'(', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[10]) { text->insert(L')', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[11]) { text->insert(L'_', ui_cursor_pos); ui_cursor_pos++; }
            if (input.down.row[12]) { text->insert(L'+', ui_cursor_pos); ui_cursor_pos++; }
        }
        // same regardless of shift key (thought could check for numlock state)
        if (input.down.num0)       { text->insert(L'0', ui_cursor_pos); ui_cursor_pos++; }
        if (input.down.num1)       { text->insert(L'1', ui_cursor_pos); ui_cursor_pos++; }
        if (input.down.num2)       { text->insert(L'2', ui_cursor_pos); ui_cursor_pos++; }
        if (input.down.num3)       { text->insert(L'3', ui_cursor_pos); ui_cursor_pos++; }
        if (input.down.num4)       { text->insert(L'4', ui_cursor_pos); ui_cursor_pos++; }
        if (input.down.num5)       { text->insert(L'5', ui_cursor_pos); ui_cursor_pos++; }
        if (input.down.num6)       { text->insert(L'6', ui_cursor_pos); ui_cursor_pos++; }
        if (input.down.num7)       { text->insert(L'7', ui_cursor_pos); ui_cursor_pos++; }
        if (input.down.num8)       { text->insert(L'8', ui_cursor_pos); ui_cursor_pos++; }
        if (input.down.num9)       { text->insert(L'9', ui_cursor_pos); ui_cursor_pos++; }
        if (input.down.numDiv)     { text->insert(L'/', ui_cursor_pos); ui_cursor_pos++; }
        if (input.down.numMul)     { text->insert(L'*', ui_cursor_pos); ui_cursor_pos++; }
        if (input.down.numSub)     { text->insert(L'-', ui_cursor_pos); ui_cursor_pos++; }
        if (input.down.numAdd)     { text->insert(L'+', ui_cursor_pos); ui_cursor_pos++; }
        if (input.down.numDecimal) { text->insert(L'.', ui_cursor_pos); ui_cursor_pos++; }

        // deleting text
        if (input.down.backspace && ui_cursor_pos>0) { text->del_at(ui_cursor_pos-1); ui_cursor_pos--; }
        if (input.down.del && ui_cursor_pos<text->count) { text->del_at(ui_cursor_pos); }

        // moving cursor
        if (input.down.left  && ui_cursor_pos>0)           { ui_cursor_pos--; }
        if (input.down.right && ui_cursor_pos<text->count) { ui_cursor_pos++; }
        if (input.down.home) { ui_cursor_pos = 0; }
        if (input.down.end) { ui_cursor_pos = text->count; }

        // done editing
        if ((input.down.mouseL && !ui_hot(id)) ||
            input.down.enter) {
            ui_set_active(0);
        }
    } else {
        if (ui_hot(id) && input.up.mouseL) {
            ui_set_active(id);
            ui_cursor_pos = text->count;
        }
    }

    // sanity check
    if (ui_active(id)) {
        if (ui_cursor_pos < 0) ui_cursor_pos = 0;
        if (ui_cursor_pos > text->count) ui_cursor_pos = text->count;
    }

    u32 bgcolor = 0xffffffff;
    float bgalpha = 0.8;

    if (ui_active(id)) {
        bgcolor = 0xffffffaa;
        bgalpha = 0.8;
    }

    // --bg--
    ui_rect(r, bgcolor, bgalpha);

    // --hl--
    if (ui_hot(id) && !ui_active(id)) { // for now try not highlighting when active and see how it feels
        ui_rect(r, 0xffffffff, 0.5);
    }

    // --text--
    char *ascii = text->to_ascii_new_memory();
    ui_text(ascii, r, UI_LEFT,UI_TOP, true, 0, 0x0);

    // --cursor--
    if (ui_active(id)) {
        // measure position of cursor
        if (ui_cursor_pos < text->count) ascii[ui_cursor_pos] = '\0'; // trim everything after cursor
        rect rectleftofcursor = ui_text(ascii, r, UI_LEFT,UI_TOP, false, 0);
        int cursorW = rectleftofcursor.w;
        // draw cursor
        if (ui_cursor_blink < ui_cursor_blink_ms/2) {
            ui_rect({r.x+cursorW-3,r.y+2,2,r.h-4}, 0xff000000, 1);
        }
    }

    bool mouseOver = r.ContainsPoint(input.current.mouseX,input.current.mouseY);
    if (mouseOver && (ui_active(id) || ui_active(0))) ui_set_hot(id);
    if (!mouseOver && ui_hot(id)) ui_set_hot(0);

    free(ascii);
}


char ui_log_reuseable_mem[256];
int ui_log_count;
void UI_PRINT(char *s) {
    ui_text(s, {0, (float)ui_log_count*UI_TEXT_SIZE}, UI_LEFT,UI_TOP, true, 0.66);
    ui_log_count++;
}
void UI_PRINT(u64 i)               { sprintf(ui_log_reuseable_mem, "%lli", i); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINT(int i)               { sprintf(ui_log_reuseable_mem, "%i", i); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINT(float f)             { sprintf(ui_log_reuseable_mem, "%f", f); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINT(char *text, int i)   { sprintf(ui_log_reuseable_mem, text, i); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINT(char *text, i64 i)   { sprintf(ui_log_reuseable_mem, text, i); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINT(char *text, char *s) { sprintf(ui_log_reuseable_mem, text, s); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINT(char *text, string s) { char *temp = s.to_utf8_new_memory(); UI_PRINT(text, temp); free(temp); }
void UI_PRINT(char *text, float f) { sprintf(ui_log_reuseable_mem, text, f); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINT(char *text, float x, float y) { sprintf(ui_log_reuseable_mem, text, x,y); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINTTIME(u64 i) { FormatTimeIntoCharBuffer(i, ui_log_reuseable_mem); UI_PRINT(ui_log_reuseable_mem); }
void UI_PRINTRESET() { ui_log_count = 0; }





