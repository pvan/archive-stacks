


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



// new "element" setup

gpu_texture_id ui_solid_tex_id;
bitmap ui_solid_bitmap;


// new "element" setup

void ui_create_solid_color_texture_and_upload()
{
    ui_solid_bitmap.data = (u32*)malloc(2 * sizeof(u32));
    ui_solid_bitmap.data[0] = 0;
    ui_solid_bitmap.data[1] = 0xffffffff;
    ui_solid_bitmap.w = 2;
    ui_solid_bitmap.h = 1;

    ui_solid_tex_id = gpu_create_texture();
    gpu_upload_texture(ui_solid_bitmap, ui_solid_tex_id);
}

// the abstractions are getting a little layered... shh bby is ok
struct ui_element {
    mesh mesh;

    // keep pointer to this when creating so it's easy to activate later
    // this is also used for mouse picking/detection at the moment...
    gpu_quad *highlight_quad;

    bool operator==(ui_element o) { return mesh==o.mesh; /* todo: check this is right when done*/ }

    void add_solid_quad(gpu_quad q, int colorcode, float alpha) {
        q.u0 = colorcode / (float)ui_solid_bitmap.w;
        q.v0 = colorcode / (float)ui_solid_bitmap.h;
        q.u1 = (colorcode+1) / (float)ui_solid_bitmap.w;
        q.v1 = (colorcode+1) / (float)ui_solid_bitmap.h;
        q.alpha = alpha;
        gpu_quad_pool newquadlist = gpu_quad_pool::new_empty();
        newquadlist.add(q);
        mesh.add_submesh({newquadlist, ui_solid_tex_id});
    }
    void add_hl_quad(gpu_quad q, int colorcode, float alpha) {
        add_solid_quad(q, colorcode, alpha);
        // HL quad will be last quad entered
        gpu_quad_list_with_texture *lastsubmesh = &(mesh.submeshes.pool[mesh.submeshes.count-1]);
        highlight_quad = &(lastsubmesh->quads[lastsubmesh->quads.count-1]);
    }
    void add_solid_rect(rect r, u32 col, float a) {
        gpu_quad q = gpu_quad_from_rect(r);
        int colcode = 1;
        if (col == 0) colcode = 0;
        add_solid_quad(q, colcode, a);
    }
};


DEFINE_TYPE_POOL(ui_element);

ui_element_pool ui_elements;

void ui_queue_element(ui_element gizmo) {
    ui_elements.add(gizmo);
}

void ui_queue_solo_rect(rect r, u32 col, float a) {
    ui_element gizmo = {0};
    gpu_quad q = gpu_quad_from_rect(r);
    int colcode = 1;
    if (col == 0) colcode = 0;
    gizmo.add_solid_quad(q, colcode, a);
    ui_elements.add(gizmo);
}

gpu_quad *ui_find_topmost_element_under_point(float mx, float my) {
    gpu_quad *result = 0;
    // result.z_level = -99999; // max negative z level
    bool found_at_least_one = false;
    for (int i = 0; i < ui_elements.count; i++) {
        if (ui_elements[i].highlight_quad) {
            rect r = ui_elements[i].highlight_quad->to_rect();
            if (mx > r.x && mx <= r.x+r.w && my > r.y && my <= r.y+r.h) {
                // if (buttons[i].z_level > result.z_level) {
                    // note we keep checking the whole list, so we implicitly get the last/topmost rect
                    result = ui_elements[i].highlight_quad;
                    found_at_least_one = true;
                // }
            }
        }
    }
    if (found_at_least_one)
        return result;
    else
        return 0;
}

void ui_render_elements(float mx, float my) {
    gpu_quad *topmost = ui_find_topmost_element_under_point(mx, my); // oh no i've looked at the word element too long and it's starting to look funny
    if (topmost) topmost->alpha = 0.3; // defauts to 0 let's say
    for (int i = 0; i < ui_elements.count; i++) {
        gpu_render_mesh(ui_elements[i].mesh);
    }
}



void ui_init() {
    ui_create_solid_color_texture_and_upload();
}


void ui_reset() {  // call every frame

    // free all the mem used in the subelements of our ui_elements before clearing it
    for (int i = 0; i < ui_elements.count; i++) {
        ui_elements[i].mesh.free_all_mem();
    }
    ui_elements.empty_out();
}



//
// click handling, basically (rename from "button" ?)


// todo: rename to ui_clickable or something ?
struct button {
    rect rect; // same name works huh? seems like trouble
    float z_level; // todo: remove
    bool highlight; // todo: remove

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
}

void AddScrollbar(rect r, bool hl, float *callbackvalue, float callbackscale) {
    buttons.add({r,1, hl, true,callbackvalue,callbackscale, 0,0, 0,0});
}




//
// intended exposed api


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
    // it wouldn't matter too much except we draw the same quad twice -- once for the highlight
    // and if they round differently.. we'll end up with an extra pixel bar where they don't match perfectly
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



    if (render) {
        ui_element gizmo = {0};
        {
            // --bg--
            gizmo.add_solid_quad(bg_quad, 0, 1);


            // --text--
            y += tf_cached_largest_ascent; // move y to top instead of baseline of text

            int quadsneeded = tf_how_many_quads_needed_for_text(text);
            gpu_quad *quads = (gpu_quad*)malloc(quadsneeded*sizeof(gpu_quad));
            tf_create_quad_list_for_text_at_rect(text, x,y+margin, quads, quadsneeded);

            gpu_quad_list_with_texture textsubmesh = {0};
            for (int i = 0; i < quadsneeded; i++) {
                textsubmesh.quads.add(quads[i]);
            }
            textsubmesh.texture_id = tf_fonttexture;
            // gpu_quad_list_with_texture textsubmesh = submesh_from_quads(quads, quadsneeded, tf_fonttexture);
            gizmo.mesh.add_submesh(textsubmesh);
            // AddDeferredTextQuads(quads, quadsneeded);
            free(quads);


            // --highlight--
            // seems like not a great way to do this,
            // but just add an invisible rect above every text
            // and if highlighted (checked when rendering), change the alpha up from 0
            gizmo.add_hl_quad(bg_quad, 1, 0);

        }
        ui_queue_element(gizmo);
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


rect ui_button(char *text, float x, float y, bool hpos, bool vpos, void(*effect)(int), int arg=0)
{
    rect tr = ui_text(text, x, y, hpos, vpos); //RenderTextCenter(x, y, text);
    rect r = {(float)tr.x, (float)tr.y, (float)tr.w, (float)tr.h};
    AddButton(r, true, effect, arg);
    return tr;
}

// kind of a hack so we can highlight tiles without bringing them into our normal system
// (they have their own opengl_quads so they don't have to re-send textures to the gpu every frame)
rect ui_button_permanent_highlight(rect br, void(*effect)(int), int arg=0)
{
    // note our button has the normal hl disabled
    AddButton(br, false, effect, arg);

    ui_element gizmo = {0};
    gpu_quad q = gpu_quad_from_rect(br);
    gizmo.add_hl_quad(q, 1, 0);
    ui_elements.add(gizmo);

    return br;
}


// note takes two values, for top/bottom of scroll bar indicator (for variable size)
void ui_scrollbar(rect r, float top_percent, float bot_percent,
                  float *callbackvalue, float callbackscale,
                  void(*effect)(int), int arg=0)
{
    ui_element gizmo = {0};
    {
        // --bg--
        // ui_draw_rect(r, 0xffbbbbbb, .4); //0xffbbbbbb, .35
        gizmo.add_solid_rect(r, 0xffffffff, 0.4);

        float top_pixels = top_percent * (float)r.h;
        float bot_pixels = bot_percent * (float)r.h;

        float size = roundf(bot_pixels-top_pixels);
        if (size < 10) size = 10;

        // --indicator--
        // ui_draw_rect({r.x, top_pixels, r.w, size}, 0xffdddddd, .9); //0xff888888, .75
        gizmo.add_solid_rect({r.x, top_pixels, r.w, size}, 0xffffffff, 0.9);

        // --hl--
        gizmo.add_hl_quad(gpu_quad_from_rect(r), 1, 0);
    }
    ui_elements.add(gizmo);

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





