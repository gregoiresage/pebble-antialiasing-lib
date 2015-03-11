# pebble-antialiasing-lib
Implementation of the [Xiaolin Wu's line algorithm] [1]

# API
```c
void graphics_draw_line_antialiased(GContext* ctx, GPoint p0, GPoint p1, GColor8 stroke_color);
void graphics_draw_circle_antialiased(GContext* ctx, GPoint p, uint16_t radius, GColor8 stroke_color);
void graphics_fill_circle_antialiased(GContext* ctx, GPoint p, uint16_t radius, GColor8 fill_color);
void gpath_draw_filled_antialiased(GContext* ctx, GPath *path, GColor8 fill_color);
void gpath_draw_outline_antialiased(GContext* ctx, GPath *path, GColor8 stroke_color);
```
# Example

Top lines are antialiased and bottom lines are drawn with the Pebble draw_line method.

![Alt text](/both-big.bmp?raw=true "Example")


# TODOs

 - ~~Implement graphics_draw_circle_antialiased~~
 - ~~Implement graphics_fill_circle_antialiased~~
 - ~~Implement gpath_draw_filled_antialiased~~
 - ~~Implement gpath_draw_outline_antialiased~~
 - ~~Remove float usage~~ thanks to @dwrose

[1]:http://en.wikipedia.org/wiki/Xiaolin_Wu%27s_line_algorithm
