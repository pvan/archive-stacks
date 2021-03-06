

struct v2 {
    union { float x; float w; };
    union { float y; float h; };
    v2 operator* (float s) { return {x*s, y*s}; }
    v2 operator/ (float s) { return {x/s, y/s}; }
    v2 operator+ (v2 o) { return {x+o.x, y+o.y}; }
    v2 operator- (v2 o) { return {x-o.x, y-o.y}; }
    bool operator== (v2 o) { return x==o.x && y==o.y; }
    float aspect_ratio() { return w/h; }
};
