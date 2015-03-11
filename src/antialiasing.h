// antialiasing.h by Grégoire Sage
// https://github.com/gregoiresage/pebble-antialiasing-lib

#pragma once

#include <pebble.h>

#ifdef PBL_COLOR

// //! Draws a 1-pixel wide line in the current stroke color with antialiasing
// //! @param ctx The destination graphics context in which to draw
// //! @param p0 The starting point of the line
// //! @param p1 The ending point of the line
// //! @param path The stroke color
void graphics_draw_line_antialiased(GContext* ctx, GPoint p0, GPoint p1, GColor8 stroke_color);

// //! Draws the outline of a circle in the current stroke color with antialiasing
// //! @param ctx The destination graphics context in which to draw
// //! @param p The center point of the circle
// //! @param radius The radius in pixels
// //! @param path The stroke color
void graphics_draw_circle_antialiased(GContext* ctx, GPoint p, uint16_t radius, GColor8 stroke_color);

// //! Fills a circle in the current fill color with antialiasing
// //! @param ctx The destination graphics context in which to draw
// //! @param p The center point of the circle
// //! @param radius The radius in pixels
// //! @param path The fill color
void graphics_fill_circle_antialiased(GContext* ctx, GPoint p, uint16_t radius, GColor8 fill_color);

// //! Draws the fill of a path with antialiasing into a graphics context,
// //! relative to the drawing area as set up by the layering system.
// //! @param ctx The graphics context to draw into
// //! @param path The path to fill
// //! @param path The fill color
// //! @see \ref graphics_context_set_fill_color()
void gpath_draw_filled_antialiased(GContext* ctx, GPath *path, GColor8 fill_color);

//! Draws the outline of a path with antialiasing into a graphics context, using the current stroke color,
//! relative to the drawing area as set up by the layering system.
//! @param ctx The graphics context to draw into
//! @param path The path to fill
//! @param path The stroke color
//! @see \ref graphics_context_set_stroke_color()
void gpath_draw_outline_antialiased(GContext* ctx, GPath *path, GColor8 stroke_color);

#else  //PBL_COLOR

#define gpath_draw_outline_antialiased gpath_draw_outline

#endif // PBL_COLOR
