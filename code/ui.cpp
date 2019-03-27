


// todo: audit this api
// -float or int rects? floats, but some bugs see zxoi (fixed?)
// -how to pass alignments in? booleans for now
// -rects or components for size/positions? whatever is more convenient for caller
// -and how to pass auto w/h (most funcs defaults to this now) reexamine this
// -names? mostly done, needs another pass or two
// -deferred rendering probably. done
// -compress call chain? yes, still needs

//
// this module meant to be used by our main app for any gui, shape, text drawing
// it's a layer between the main app and lower level modules like text (tf_*) and gpu (gpu_*)
// which may also use even lower level constructs like bitmap and rect
//
//
// this is the way it works right now...
//
// interacting with and rendering all elements is deferred until the end of frame
// see
// ui_update_draggables(input.mouseX, input.mouseY, input.mouseL);
// ui_update_clickables(input.mouseX, input.mouseY, keysDown.mouseL);
// ui_render_elements(input.mouseX, input.mouseY); // pass mouse pos for highlighting
// ui_reset(); // call at the end or start of every frame so buttons don't carry over between frames
//
//
// ui_clickables is a collection of everything that can be clicked on
//               they are invisible and aren't drawn
//
// ui_elements is the collection of elements to render
//             usually multiple quads and textures will go into one element
//             including an invisible quad on top of its other quads that
//             is used to highlight the element on mouse hover (at the moment)
//
// most exposed api will end up creating a clickable and an element
// todo: it might be worth combining these into one thing
//
// text is drawn with a collection of quads that have uvs into the texture atlas
// the quads are created by the text (tf_) module which also owns the text atlas
//
// solid-color quads are drawn with a single-pixel texture handled by this layer
//
//
// a group of quads that use the same texture are combined into a struct called a submesh
// a group of submeshes are combined into one mesh (each ui_element has a single mesh attached)
// (this is so we can have a single object that can use multiple textures)
// these structs all handle their own memory using dynamically-sized object pools, so be careful with leaks
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

// the abstractions are getting a little layered... shh bby is ok
struct ui_element {
    mesh mesh;

    // keep pointer to this when creating so it's easy to activate later
    // this is also used for mouse picking/detection at the moment...
    gpu_quad *highlight_quad;

    bool operator==(ui_element o) { return mesh==o.mesh; /* todo: check this is right when done*/ }

    // a little awkward since quad has color/alpha in it atm
    // and we just overwrite it with new
    // todo: maybe keep color/alpha out of gpu_quad?
    // or maybe make gpu_quad constructor that takes color/alpha and
    // chagne these functions to jsut take a single gpu_quad
    void add_solid_quad(gpu_quad q, u32 color, float alpha) {
        // int colorcode = 1;
        // q.u0 = colorcode / (float)ui_solid_bitmap.w;
        // q.v0 = colorcode / (float)ui_solid_bitmap.h;
        // q.u1 = (colorcode+1) / (float)ui_solid_bitmap.w;
        // q.v1 = (colorcode+1) / (float)ui_solid_bitmap.h;
        q.u0 = 0;
        q.v0 = 0;
        q.u1 = 1;
        q.v1 = 1;
        q.alpha = alpha;
        q.color = color;
        gpu_quad_pool newquadlist = gpu_quad_pool::new_empty();
        newquadlist.add(q);
        mesh.add_submesh({newquadlist, ui_solid_tex_id});
    }
    void add_hl_quad(gpu_quad q, u32 col = 0xffffffff, float a = 0) {
        add_solid_quad(q, col, a);
        // HL quad will be last quad entered
        gpu_quad_list_with_texture *lastsubmesh = &(mesh.submeshes.pool[mesh.submeshes.count-1]);
        highlight_quad = &(lastsubmesh->quads[lastsubmesh->quads.count-1]);
    }
    void add_solid_rect(rect r, u32 col, float a) {
        gpu_quad q = gpu_quad_from_rect(r);
        // int colcode = 1;
        // if (col == 0) colcode = 0;
        add_solid_quad(q, col, a);
    }
};


DEFINE_TYPE_POOL(ui_element);

ui_element_pool ui_elements;

void ui_queue_element(ui_element gizmo) {
    ui_elements.add(gizmo);
}

// void ui_queue_solo_rect(rect r, u32 col, float a) {
//     ui_element gizmo = {0};
//     gpu_quad q = gpu_quad_from_rect(r);
//     int colcode = 1;
//     if (col == 0) colcode = 0;
//     gizmo.add_solid_quad(q, colcode, a);
//     ui_elements.add(gizmo);
// }

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
// click handling basically (previously known as "button")


// basically a rect with some callbacks for when it's clicked
struct ui_clickable {
    rect rect; // same name works huh? seems like trouble

    bool is_scrollbar; // make into generic "type" var when needed
    float *callbackvalue; // so we can change value with our deferred handling
    float callbackvaluescale; // kind of a "units" factor for the callbackvalue

    void (*on_click)(int);
    int click_arg;

    void (*on_mouseover)(int);
    int mouseover_arg;

    bool operator==(ui_clickable o) {
        return rect==o.rect &&

               is_scrollbar == o.is_scrollbar &&
               callbackvalue == o.callbackvalue &&
               callbackvaluescale == o.callbackvaluescale &&

               on_click==o.on_click &&
               click_arg==o.click_arg &&

               on_mouseover==o.on_mouseover &&
               mouseover_arg==o.mouseover_arg;
   }
};

DEFINE_TYPE_POOL(ui_clickable);

ui_clickable_pool ui_clickables;


ui_clickable ui_find_topmost_clickable_under_point(float mx, float my) {
    ui_clickable result = {0};
    // result.z_level = -99999; // max negative z level
    bool found_at_least_one = false;
    for (int i = 0; i < ui_clickables.count; i++) {
        rect r = ui_clickables[i].rect;
        if (mx > r.x && mx <= r.x+r.w && my > r.y && my <= r.y+r.h) {
            // if (buttons[i].z_level > result.z_level) {
                // note we keep checking the whole list, so we implicitly get the last/topmost rect
                // should just search reverse and quit after first
                result = ui_clickables[i];
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
ui_clickable ui_drag_element = {0};

// call to update elements that respond to clicking
void ui_update_clickables(float mx, float my, bool mouse_click, int clientW, int clientH) {
    // todo: should also ignore when window doesn't have focus right?
    // ignore out of bounds clicks
    if (mx>0 && my>0 && mx<clientW && my<clientH) {
        if (mouse_click) {
            ui_clickable top_most = ui_find_topmost_clickable_under_point(mx, my);

            if (top_most.is_scrollbar) {
                ui_drag_element = top_most;
                ui_dragging = true;
            }

            if (top_most.on_click)
                top_most.on_click(top_most.click_arg);
        }
    }
}

// call to update elements that respond to dragging (eg scrollbars)
void ui_update_draggables(float mx, float my, bool mouse_down) {
    if (!mouse_down) {
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


void ui_add_clickable(rect r, void(*on_click)(int),int click_arg, void(*on_mouseover)(int)=0,int mouseover_arg=0) {
    ui_clickables.add({r, false,0,0, on_click,click_arg, on_mouseover,mouseover_arg});
}

void ui_add_draggable(rect r, float *callbackvalue, float callbackscale) {
    ui_clickables.add({r, true,callbackvalue,callbackscale, 0,0, 0,0});
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
    rect textbb = tf_text_bounding_box(text, x, y);

    // pad beyond just our letter-tight bb
    int margin = 2;

    // bg (and hl) quad
    // setup for TL coords as default
    gpu_quad bg_quad;
    bg_quad.x0 = x;
    bg_quad.y0 = y;
    bg_quad.x1 = x + textbb.w+1 + margin*2;  // note fencepost error with w/h.. don't include last edge
    bg_quad.y1 = y + tf_cached_largest_total_height+1 + margin*2; // todo: check if correct

    bg_quad.y1 -= 1; // a little hand-tweaking (tbh not sure if +1 in the y1 above is correct, x1 def correct tho)

    // adjust for alignment
    if (hpos == UI_RIGHT) bg_quad.move(-bg_quad.width(), 0);
    if (hpos == UI_CENTER) bg_quad.move(-bg_quad.width()/2, 0);
    if (vpos == UI_BOTTOM) bg_quad.move(0, -bg_quad.height());
    if (vpos == UI_CENTER) bg_quad.move(0, -bg_quad.height()/2);

    // text position (inside the larger quad)
    float textx = bg_quad.x0 + margin;
    float texty = bg_quad.y0 + margin + tf_cached_largest_ascent; // text is drawn from the baseline todo: let text module handle this offset?

    // fixed?
    // // todo: find root cause of this bug (id: zxoi)
    // // (this is maybe related to some inconsistent kerning.. need subpixel rendering?)
    // // it's ugly but if x is on an exact 0.5 edge,
    // // we seem to get inconsistent rounding somewhere in our rendering pipeline
    // // it wouldn't matter too much except we draw the same quad twice -- once for the highlight
    // // and if they round differently.. we'll end up with an extra pixel bar where they don't match perfectly
    // // so in that case, add a fudge factor
    // if (x-0.5 == (int)x) {
    //     x+=0.01;
    //     // assert(false);
    // }


    if (render) {
        ui_element gizmo = {0};
        {
            // --bg--
            // gizmo.add_solid_quad(bg_quad, 0x0, 0.5);

            // --text--
            int quadsneeded = tf_how_many_quads_needed_for_text(text);
            gpu_quad *quads = (gpu_quad*)malloc(quadsneeded*sizeof(gpu_quad));
            tf_create_quad_list_for_text_at_rect(text, textx,texty, quads, quadsneeded);

            gpu_quad_list_with_texture textsubmesh = {0};
            for (int i = 0; i < quadsneeded; i++) {
                // quads[i].alpha = 0.5;
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
            gizmo.add_hl_quad(bg_quad);

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


rect ui_button(char *text, float x, float y, int hpos, int vpos, void(*effect)(int), int arg=0)
{
    rect tr = ui_text(text, x, y, hpos, vpos);
    rect r = {(float)tr.x, (float)tr.y, (float)tr.w, (float)tr.h};
    ui_add_clickable(r, effect, arg);
    return tr;
}

// kind of a hack so we can highlight tiles without bringing them into our normal system
// (they have their own opengl_quads so they don't have to re-send textures to the gpu every frame)
rect ui_button_permanent_highlight(rect br, void(*effect)(int), int arg=0)
{
    // note our button has the normal hl disabled
    ui_add_clickable(br, effect, arg);

    ui_element gizmo = {0};
    gpu_quad q = gpu_quad_from_rect(br);
    gizmo.add_hl_quad(q);
    ui_elements.add(gizmo);

    return br;
}

void ui_rect(float x, float y, float w, float h, u32 col, float a) {
    // feels like this internal api needs some work but it's functional for now
    ui_element gizmo = {0};
    gizmo.add_solid_rect({x,y,w,h}, col, a);
    ui_elements.add(gizmo);
}


// note takes two values, for top/bottom of scroll bar indicator (for variable size)
void ui_scrollbar(rect r, float top_percent, float bot_percent,
                  float *callbackvalue, float callbackscale,
                  void(*effect)(int), int arg=0)
{
    ui_element gizmo = {0};
    {
        // lets try something funny, put the hl below the others as a kind of optional "less opacity"
        // todo: could have variable opacity settings on our hl instead of always going to .3 or w/e
        // --hl--
        gizmo.add_hl_quad(gpu_quad_from_rect(r), 0, 0);

        // --bg--
        gizmo.add_solid_rect(r, 0xffbbbbbb, 0.4);

        float top_pixels = top_percent * (float)r.h;
        float bot_pixels = bot_percent * (float)r.h;

        float size = roundf(bot_pixels-top_pixels);
        if (size < 10) size = 10;

        // --indicator--
        gizmo.add_solid_rect({r.x, top_pixels, r.w, size}, 0xffeeeeeeee, 0.9);

    }
    ui_elements.add(gizmo);

    // click handler
    ui_add_draggable(r, callbackvalue, callbackscale);
}


gpu_texture_id reusable_texture_id_for_bitmap = -1;
void ui_bitmap(bitmap img, float x, float y) {

    if (reusable_texture_id_for_bitmap == -1) {
        reusable_texture_id_for_bitmap = gpu_create_texture();
    }

    gpu_upload_texture(img, reusable_texture_id_for_bitmap);

    gpu_quad quad = gpu_quad_from_rect({0,0, (float)img.w, (float)img.h}, 1);
    gpu_render_quads_with_texture(&quad, 1, reusable_texture_id_for_bitmap, 1);
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





