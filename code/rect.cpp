


struct rect {
    float x, y, w, h;

    bool ContainsPoint(float px, float py) {
        return px>x && px<x+w && py>y && py<y+h;
    }

    // void operator+=(Rect second) { x+=second.x; y+=second.y; w+=second.w; h+=second.h; }
};

