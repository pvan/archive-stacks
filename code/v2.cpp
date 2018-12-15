

struct v2 {
    float x, y;
    v2 operator* (float s) { return {x*s, y*s}; }
    v2 operator/ (float s) { return {x/s, y/s}; }
    v2 operator+ (v2 o) { return {x+o.x, y+o.y}; }
    v2 operator- (v2 o) { return {x-o.x, y-o.y}; }
    bool operator== (v2 o) { return x==o.x && y==o.y; }
};
