

struct Input
{
    bool q;
    bool w;
    bool e;
    bool r;
    bool t;
    bool y;
    bool u;
    bool i;
    bool o;
    bool p;

    bool a;
    bool s;
    bool d;
    bool f;
    bool g;
    bool h;
    bool j;
    bool k;
    bool l;

    bool z;
    bool x;
    bool c;
    bool v;
    bool b;
    bool n;
    bool m;

    bool squareL;
    bool squareR;
    bool bslash;
    bool semicolon;
    bool apostrophe;
    bool comma;
    bool period;
    bool fslash;

    bool up;
    bool down;
    bool left;
    bool right;

    bool space;
    bool shift;
    bool ctrl;
    bool enter;
    bool tab;
    bool caps;
    bool tilde;
    bool backspace;

    bool row[12];

    // bool carrotL;
    // bool carrotR;
    // bool questionMark;

    bool pgUp;
    bool pgDown;

    bool f1;
    bool f2;
    bool f3;
    bool f4;
    bool f5;
    bool f6;
    bool f7;
    bool f8;
    bool f9;
    bool f10;
    bool f11;
    bool f12;

    bool mouseL;
    bool mouseM;
    bool mouseR;

    float mouseX;
    float mouseY;


    bool point_in_client_area(int cw, int ch) {
        return mouseX>0 && mouseY>0 && mouseX<cw && mouseY<ch;
    }

};



// could miss a keypress here if it's faster than the framerate?
Input ReadInput(HWND window)
{
    Input result = {0};

    if (GetFocus() == window) { // ignore keys if we dont have keyboard focus
        if (GetAsyncKeyState('Q'))   result.q = true;
        if (GetAsyncKeyState('W'))   result.w = true;
        if (GetAsyncKeyState('E'))   result.e = true;
        if (GetAsyncKeyState('R'))   result.r = true;
        if (GetAsyncKeyState('T'))   result.t = true;
        if (GetAsyncKeyState('Y'))   result.y = true;
        if (GetAsyncKeyState('U'))   result.u = true;
        if (GetAsyncKeyState('I'))   result.i = true;
        if (GetAsyncKeyState('O'))   result.o = true;
        if (GetAsyncKeyState('P'))   result.p = true;
        if (GetAsyncKeyState('A'))   result.a = true;
        if (GetAsyncKeyState('S'))   result.s = true;
        if (GetAsyncKeyState('D'))   result.d = true;
        if (GetAsyncKeyState('F'))   result.f = true;
        if (GetAsyncKeyState('G'))   result.g = true;
        if (GetAsyncKeyState('H'))   result.h = true;
        if (GetAsyncKeyState('J'))   result.j = true;
        if (GetAsyncKeyState('K'))   result.k = true;
        if (GetAsyncKeyState('L'))   result.l = true;
        if (GetAsyncKeyState('Z'))   result.z = true;
        if (GetAsyncKeyState('X'))   result.x = true;
        if (GetAsyncKeyState('C'))   result.c = true;
        if (GetAsyncKeyState('V'))   result.v = true;
        if (GetAsyncKeyState('B'))   result.b = true;
        if (GetAsyncKeyState('N'))   result.n = true;
        if (GetAsyncKeyState('M'))   result.m = true;

        if (GetAsyncKeyState(VK_OEM_4))       result.squareL = true;
        if (GetAsyncKeyState(VK_OEM_6))       result.squareR = true;
        if (GetAsyncKeyState(VK_OEM_5))       result.bslash = true;
        if (GetAsyncKeyState(VK_OEM_1))       result.semicolon = true;
        if (GetAsyncKeyState(VK_OEM_7))       result.apostrophe = true;
        if (GetAsyncKeyState(VK_OEM_COMMA))   result.comma = true;
        if (GetAsyncKeyState(VK_OEM_PERIOD))  result.period = true;
        if (GetAsyncKeyState(VK_OEM_2))       result.fslash = true;

        if (GetAsyncKeyState(VK_UP))          result.up = true;
        if (GetAsyncKeyState(VK_DOWN))        result.down = true;
        if (GetAsyncKeyState(VK_LEFT))        result.left = true;
        if (GetAsyncKeyState(VK_RIGHT))       result.right = true;

        if (GetAsyncKeyState(VK_SPACE))       result.space = true;
        if (GetAsyncKeyState(VK_SHIFT))       result.shift = true;
        if (GetAsyncKeyState(VK_CONTROL))     result.ctrl = true;
        if (GetAsyncKeyState(VK_RETURN))      result.enter = true;
        if (GetAsyncKeyState(VK_TAB))         result.tab = true;
        if (GetAsyncKeyState(VK_CAPITAL))     result.caps = true;
        if (GetAsyncKeyState(VK_OEM_3))       result.tilde = true;
        if (GetAsyncKeyState(VK_BACK))        result.backspace = true;

        if (GetAsyncKeyState(VK_OEM_3))       result.row[0] = true;
        if (GetAsyncKeyState('1'))            result.row[1] = true;
        if (GetAsyncKeyState('2'))            result.row[2] = true;
        if (GetAsyncKeyState('3'))            result.row[3] = true;
        if (GetAsyncKeyState('4'))            result.row[4] = true;
        if (GetAsyncKeyState('5'))            result.row[5] = true;
        if (GetAsyncKeyState('6'))            result.row[6] = true;
        if (GetAsyncKeyState('7'))            result.row[7] = true;
        if (GetAsyncKeyState('8'))            result.row[8] = true;
        if (GetAsyncKeyState('9'))            result.row[9] = true;
        if (GetAsyncKeyState('0'))            result.row[10] = true;
        if (GetAsyncKeyState(VK_OEM_MINUS))   result.row[11] = true;
        if (GetAsyncKeyState(VK_OEM_PLUS))    result.row[12] = true;

        if (GetAsyncKeyState(VK_PRIOR))       result.pgUp = true;
        if (GetAsyncKeyState(VK_NEXT))        result.pgDown = true;

        if (GetAsyncKeyState(VK_F1))          result.f1 = true;
        if (GetAsyncKeyState(VK_F2))          result.f2 = true;
        if (GetAsyncKeyState(VK_F3))          result.f3 = true;
        if (GetAsyncKeyState(VK_F4))          result.f4 = true;
        if (GetAsyncKeyState(VK_F5))          result.f5 = true;
        if (GetAsyncKeyState(VK_F6))          result.f6 = true;
        if (GetAsyncKeyState(VK_F7))          result.f7 = true;
        if (GetAsyncKeyState(VK_F8))          result.f8 = true;
        if (GetAsyncKeyState(VK_F9))          result.f9 = true;
        if (GetAsyncKeyState(VK_F10))         result.f10 = true;
        if (GetAsyncKeyState(VK_F11))         result.f11 = true;
        if (GetAsyncKeyState(VK_F12))         result.f12 = true;
    }

    if (GetActiveWindow() == window) { // ignore mouse when window isn't active
        if (GetAsyncKeyState(VK_LBUTTON))     result.mouseL = true;
        if (GetAsyncKeyState(VK_MBUTTON))     result.mouseM = true;
        if (GetAsyncKeyState(VK_RBUTTON))     result.mouseR = true;

        POINT p;
        if (GetCursorPos(&p)) {
            if (ScreenToClient(window, &p)) {
                result.mouseX = p.x;
                result.mouseY = p.y;
            }
        }
    }

    return result;
}



Input input_keys_changed(Input current, Input last)
{
    Input result;

    result.q = current.q && !last.q;
    result.w = current.w && !last.w;
    result.e = current.e && !last.e;
    result.r = current.r && !last.r;
    result.t = current.t && !last.t;
    result.y = current.y && !last.y;
    result.u = current.u && !last.u;
    result.i = current.i && !last.i;
    result.o = current.o && !last.o;
    result.p = current.p && !last.p;
    result.a = current.a && !last.a;
    result.s = current.s && !last.s;
    result.d = current.d && !last.d;
    result.f = current.f && !last.f;
    result.g = current.g && !last.g;
    result.h = current.h && !last.h;
    result.j = current.j && !last.j;
    result.k = current.k && !last.k;
    result.l = current.l && !last.l;
    result.z = current.z && !last.z;
    result.x = current.x && !last.x;
    result.c = current.c && !last.c;
    result.v = current.v && !last.v;
    result.b = current.b && !last.b;
    result.n = current.n && !last.n;
    result.m = current.m && !last.m;

    result.squareL    = current.squareL     && !last.squareL    ;
    result.squareR    = current.squareR     && !last.squareR    ;
    result.bslash     = current.bslash      && !last.bslash     ;
    result.semicolon  = current.semicolon   && !last.semicolon  ;
    result.apostrophe = current.apostrophe  && !last.apostrophe ;
    result.comma      = current.comma       && !last.comma      ;
    result.period     = current.period      && !last.period     ;
    result.fslash     = current.fslash      && !last.fslash     ;
    result.up         = current.up          && !last.up         ;
    result.down       = current.down        && !last.down       ;
    result.left       = current.left        && !last.left       ;
    result.right      = current.right       && !last.right      ;
    result.space      = current.space       && !last.space      ;
    result.shift      = current.shift       && !last.shift      ;
    result.ctrl       = current.ctrl        && !last.ctrl       ;
    result.enter      = current.enter       && !last.enter      ;
    result.tab        = current.tab         && !last.tab        ;
    result.caps       = current.caps        && !last.caps       ;
    result.tilde      = current.tilde       && !last.tilde      ;
    result.backspace  = current.backspace   && !last.backspace  ;

    for (int i = 0; i < 12; i++)
        result.row[i] = current.row[i] && !last.row[i];

    result.pgUp = current.pgUp && !last.pgUp;
    result.pgDown = current.pgDown && !last.pgDown;

    result.f1 = current.f1 && !last.f1;
    result.f2 = current.f2 && !last.f2;
    result.f3 = current.f3 && !last.f3;
    result.f4 = current.f4 && !last.f4;
    result.f5 = current.f5 && !last.f5;
    result.f6 = current.f6 && !last.f6;
    result.f7 = current.f7 && !last.f7;
    result.f8 = current.f8 && !last.f8;
    result.f9 = current.f9 && !last.f9;
    result.f10 = current.f10 && !last.f10;
    result.f11 = current.f11 && !last.f11;
    result.f12 = current.f12 && !last.f12;

    result.mouseL = current.mouseL && !last.mouseL;
    result.mouseM = current.mouseM && !last.mouseM;
    result.mouseR = current.mouseR && !last.mouseR;

    // result.mouseX = current.mouseX && !last.mouseX;
    // result.mouseY = current.mouseY && !last.mouseY;

    return result;

}