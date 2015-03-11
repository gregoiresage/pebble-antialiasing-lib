// antialiasing.c by Grégoire Sage
// https://github.com/gregoiresage/pebble-antialiasing-lib

#include <pebble.h>
#include "antialiasing.h"

#ifdef PBL_COLOR

/**
 * Implementation of the Xiaolin Wu's line algorithm for Pebble
 * http://en.wikipedia.org/wiki/Xiaolin_Wu%27s_line_algorithm
 */

// In the following fixed-point algorithms, the bottom four bits
// (bottom one hex digit) are the fractional part.  1.0 == 0x10.
typedef int fixed;
#define fixed_1 0x10   // 1.0
#define fixed_05 0x08  // 0.5

#define int_to_fixed(i) ((i) << 4)
#define fixed_to_int(i) ((i) >> 4)
#define fixed_div(a, b) (((a) << 4) / (b))
#define fixed_mul(a, b) (((a) * (b)) >> 4)

//#define interpol_color_(c1, c2) c1 = (int16_t)(c1 * 0x55 + ((int16_t)(c2) * 0x55 - (int16_t)(c1) * 0x55) * br) >> 6;
#define interpol_color_(c1, c2) c1 = ((c1) * fixed_1 + ((c2) - (c1)) * br + fixed_05) >> 4
inline void _plot(uint8_t* pixels, int16_t w, int16_t h, int16_t x, int16_t y, GColor8 color, fixed br)
{
	if(x<0 || x>(w-1) || y<0 || y>(h-1))
		return;

	GColor8* oc = (GColor8*)(pixels + x + w * y);
	if( br >= fixed_1 ) {
      memcpy(oc, &color, sizeof(GColor8));
	}
	else {
      interpol_color_(oc->r, color.r);
      interpol_color_(oc->g, color.g);
      interpol_color_(oc->b, color.b);
	}
}

#define ipart_(X) ((X) >> 4)
#define fpart_(X) ((X) & 0xf)
#define rfpart_(X) (fixed_1 - fpart_(X))
#define swap_(a, b) { a ^= b; b ^= a; a ^= b; }
#define abs_(a) ((a) > 0 ? (a) : -(a))

void draw_line_antialias_(GBitmap* img, int16_t x1, int16_t y1, int16_t x2, int16_t y2, GColor8 color)
{
	uint8_t* img_pixels = gbitmap_get_data(img);
	int16_t  w 	= gbitmap_get_bounds(img).size.w;
	int16_t  h 	= gbitmap_get_bounds(img).size.h;

	fixed dx = int_to_fixed(abs_(x1 - x2));
	fixed dy = int_to_fixed(abs_(y1 - y2));
	
	bool steep = dy > dx;

	if(steep){
		swap_(x1, y1);
		swap_(x2, y2);
	}
	if(x1 > x2){
		swap_(x1, x2);
		swap_(y1, y2);
	}

	dx = x2 - x1;
	dy = y2 - y1;

    fixed intery;
	int x;
	for(x=x1; x <= x2; x++) {
        intery = int_to_fixed(y1) + (int_to_fixed(x - x1) * dy / dx);
		if(x>=0){
			if(steep){
				_plot(img_pixels, w, h, ipart_(intery)    , x, color, rfpart_(intery));
				_plot(img_pixels, w, h, ipart_(intery) + 1, x, color,  fpart_(intery));
			}
			else {
				_plot(img_pixels, w, h, x, ipart_(intery)	 , color, rfpart_(intery));
				_plot(img_pixels, w, h, x, ipart_(intery) + 1, color,  fpart_(intery));
			}
		}
	}
}

void graphics_draw_line_antialiased(GContext* ctx, GPoint p0, GPoint p1, GColor8 stroke_color){
	if(p0.x == p1.x || p0.y == p1.y || p0.x-p1.x == p0.y-p1.y){
		graphics_draw_line(ctx, p0, p1);
	}
	else {
		GBitmap* bitmap = graphics_capture_frame_buffer(ctx);
		draw_line_antialias_(bitmap, p0.x, p0.y, p1.x, p1.y, stroke_color);
		graphics_release_frame_buffer(ctx, bitmap);
	}
}

void gpath_draw_outline_antialiased(GContext* ctx, GPath *path, GColor8 stroke_color){
	if(path->num_points == 0)
		return;	

	GPoint offset = path->offset;
	int32_t rotation = path->rotation;
	GBitmap* bitmap = graphics_capture_frame_buffer(ctx);

  	int32_t s = sin_lookup(rotation);
  	int32_t c = cos_lookup(rotation);

	GPoint p = path->points[path->num_points-1];
	GPoint p1;
  	p1.x = (p.x * c - p.y * s) / TRIG_MAX_RATIO + offset.x;
  	p1.y = (p.x * s + p.y * c) / TRIG_MAX_RATIO + offset.y;

	for(uint32_t i=0; i<path->num_points; i++){
		GPoint p2;
		p2.x = (path->points[i].x * c - path->points[i].y * s) / TRIG_MAX_RATIO  + offset.x;
		p2.y = (path->points[i].x * s + path->points[i].y * c) / TRIG_MAX_RATIO  + offset.y;
		draw_line_antialias_(bitmap, p1.x, p1.y, p2.x, p2.y, stroke_color);
		p1 = p2;
	}

	graphics_release_frame_buffer(ctx, bitmap);
}

#define set_pixel_(pixels, bytes_per_row, x, y) (pixels[(y) * (bytes_per_row) + (x) / 8] |= (1<<((x)%8)))
#define get_pixel_(pixels, bytes_per_row, x, y) (pixels[(y) * (bytes_per_row) + (x) / 8]  & (1<<((x)%8)))

/**
  * From https://github.com/Jnmattern/Minimalist_2.0/blob/master/src/bitmap.h
  */
static void bmpDrawLine(uint8_t *pixels, int bytes_per_row, int x1, int y1, int x2, int y2) {
	int dx, dy, e;
    
	if ((dx = x2-x1) != 0) {
		if (dx > 0) {
			if ((dy = y2-y1) != 0) {
				if (dy > 0) {
					// vecteur oblique dans le 1er quadran
					if (dx >= dy) {
						// vecteur diagonal ou oblique proche de l’horizontale, dans le 1er octant
						e = dx;
						dx = 2*e;
						dy = 2*dy;
						while (1) {
							set_pixel_(pixels, bytes_per_row, x1, y1);
							x1++;
							if (x1 == x2) break;
							e -= dy;
							if (e < 0) {
								y1++;
								e += dx;
							}
						}
					} else {
						// vecteur oblique proche de la verticale, dans le 2nd octant
						e = dy;
						dy = 2*e;
						dx = 2*dx;
						while (1) {
							set_pixel_(pixels, bytes_per_row, x1, y1);
							y1++;
							if (y1 == y2) break;
							e -= dx;
							if (e < 0) {
								x1++;
								e += dy;
							}
						}
					}
				} else { // dy < 0 (et dx > 0)
					// vecteur oblique dans le 4e cadran
					if (dx >= -dy) {
						// vecteur diagonal ou oblique proche de l’horizontale, dans le 8e octant
						e = dx;
						dx = 2*e;
						dy = 2*dy;
						while (1) {
							set_pixel_(pixels, bytes_per_row, x1, y1);
							x1++;
							if (x1 == x2) break;
							e += dy;
							if (e < 0) {
								y1--;
								e += dx;
							}
						}
					} else {
						// vecteur oblique proche de la verticale, dans le 7e octant
						e = dy;
						dy = 2*e;
						dx = 2*dx;
						while (1) {
							set_pixel_(pixels, bytes_per_row, x1, y1);
							y1--;
							if (y1 == y2) break;
							e += dx;
							if (e > 0) {
								x1++;
								e += dy;
							}
						}
					}
				}
			} else {
				// dy = 0 (et dx > 0)
                // vecteur horizontal vers la droite
				while (1) {
					set_pixel_(pixels, bytes_per_row, x1, y1);
					x1++;
					if (x1 == x2) break;
				}
			}
		} else {
			// dx < 0
			if ((dy = y2-y1) != 0) {
				if (dy > 0) {
					// vecteur oblique dans le 2nd quadran
					if (-dx >= dy) {
						// vecteur diagonal ou oblique proche de l’horizontale, dans le 4e octant
						e = dx;
						dx = 2*e;
						dy = 2*dy;
						while (1) {
							set_pixel_(pixels, bytes_per_row, x1, y1);
							x1--;
							if (x1 == x2) break;
							e += dy;
							if (e >= 0) {
								y1++;
								e += dx;
							}
						}
					} else {
						// vecteur oblique proche de la verticale, dans le 3e octant
						e = dy;
						dy = 2*e;
						dx = 2*dx;
						while (1) {
							set_pixel_(pixels, bytes_per_row, x1, y1);
							y1++;
							if (y1 == y2) break;
							e += dx;
							if (e <= 0) {
								x1--;
								e += dy;
							}
						}
					}
				} else {
					// dy < 0 (et dx < 0)
					// vecteur oblique dans le 3e cadran
                    if (dx <= dy) {
						// vecteur diagonal ou oblique proche de l’horizontale, dans le 5e octant
						e = dx;
						dx = 2*e;
						dy = 2*dy;
						while (1) {
							set_pixel_(pixels, bytes_per_row, x1, y1);
							x1--;
							if (x1 == x2) break;
							e -= dy;
							if (e >= 0) {
								y1--;
								e += dx;
							}
						}
					} else {
						// vecteur oblique proche de la verticale, dans le 6e octant
						e = dy;
						dy = 2*e;
						dx = 2*dx;
						while (1) {
							set_pixel_(pixels, bytes_per_row, x1, y1);
							y1--;
							if (y1 == y2) break;
							e -= dx;
							if (e >= 0) {
								x1--;
								e += dy;
							}
						}
					}
				}
			} else {
				// dy = 0 (et dx < 0)
				// vecteur horizontal vers la gauche
				while (1) {
					set_pixel_(pixels, bytes_per_row, x1, y1);
					x1--;
					if (x1 == x2) break;
				}
			}
		}
	} else {
		// dx = 0
		if ((dy = y2-y1) != 0) {
			if (dy > 0) {
				// vecteur vertical croissant
				while (1) {
					set_pixel_(pixels, bytes_per_row, x1, y1);
					y1++;
					if (y1 == y2) break;
				}
			} else {
				// dy < 0 (et dx = 0)
				// vecteur vertical décroissant
				while (1) {
					set_pixel_(pixels, bytes_per_row, x1, y1);
					y1--;
					if (y1 == y2) break;
				}
            }
		}
	}
}

#define increase_queue_size(queue, max_size) \
GPoint *tmp_queue = malloc(sizeof(GPoint) * (max_size + 1)); \
memcpy(tmp_queue, queue, sizeof(GPoint) * max_size); \
max_size++; \
free(queue); \
queue = tmp_queue; 

/**
 * Flood fill algorithm : http://en.wikipedia.org/wiki/Flood_fill
 */
static void floodFill(GBitmap* bitmap, uint8_t* pixels, int bytes_per_row, GPoint start, GPoint offset, GColor8 fill_color){
	uint8_t* img_pixels = gbitmap_get_data(bitmap);
	GRect bounds_bmp = gbitmap_get_bounds(bitmap);
  	int16_t  w_bmp 	= bounds_bmp.size.w;

	uint32_t max_size = 6;
	GPoint *queue = malloc(sizeof(GPoint) * max_size);
	uint32_t size = 0;

	int32_t x = start.x - offset.x;
	int32_t y = start.y - offset.y;

	queue[size++] = (GPoint){x,y};
	int32_t w,e;

	while(size > 0)
	{
		size--;
		x = queue[size].x;
		y = queue[size].y;
		w = e = x;

		while(!get_pixel_(pixels, bytes_per_row, e, y))
			e++;
		while(w>=0 && !get_pixel_(pixels, bytes_per_row, w, y))
			w--;

		bool up = false;
		bool down = false;
		for(x=w+1; x<e; x++)
		{	
			// change the color of the pixel in the final image
			if(grect_contains_point(&bounds_bmp,&((GPoint){x + offset.x, y + offset.y})))
				img_pixels[x + offset.x + w_bmp * (y + offset.y)] = fill_color.argb;

			set_pixel_(pixels, bytes_per_row,  x, y);

			if(!get_pixel_(pixels, bytes_per_row, x, y+1)){
				down = true;
			}
			else if(down) {
				down = false;
				if(size == max_size){
					increase_queue_size(queue, max_size)
				}
				queue[size++] = (GPoint){x-1, y+1};
			}

			if(!get_pixel_(pixels, bytes_per_row, x, y-1)){
				up = true;
			}
			else if(up) {
				up = false;
				if(size == max_size){
					increase_queue_size(queue, max_size)
				}
				queue[size++] = (GPoint){x-1, y-1};
			}
		}
		if(down) {
			down = false;
			if(size == max_size){
				increase_queue_size(queue, max_size)
			}
			queue[size++] = (GPoint){x-1, y+1};
		}
		if(up) {
			up = false;
			if(size == max_size){
				increase_queue_size(queue, max_size)
			}
			queue[size++] = (GPoint){x-1, y-1};
		}
	}

	free(queue);
}


static void gpath_draw_filled_custom(GContext* ctx, GPath *path, GColor8 fill_color){
	if(path->num_points == 0)
		return;	

	GPoint offset = path->offset;
	int32_t rotation = path->rotation;

	int32_t s = sin_lookup(rotation);
  	int32_t c = cos_lookup(rotation);

  	// Rotate each point of the gpath and memorize the min/max
	GPoint* points_rot = malloc(sizeof(GPoint) * path->num_points);
	GPoint top_right = (GPoint){(1 << 15)-1,(1 << 15)-1};
	GPoint bottom_left= (GPoint){-(1 << 15),-(1 << 15)};

  	for(uint32_t i=0; i<path->num_points; i++){
  		points_rot[i].x = (path->points[i].x * c - path->points[i].y * s) / TRIG_MAX_RATIO  + offset.x;
		points_rot[i].y = (path->points[i].x * s + path->points[i].y * c) / TRIG_MAX_RATIO  + offset.y;
		if(points_rot[i].x > bottom_left.x)
			bottom_left.x = points_rot[i].x;
		if(points_rot[i].x < top_right.x)
			top_right.x = points_rot[i].x;
		if(points_rot[i].y > bottom_left.y)
			bottom_left.y = points_rot[i].y;
		if(points_rot[i].y < top_right.y)
			top_right.y = points_rot[i].y;
  	}

  	// Create an array bitmap pebble v2 style (1 bit equals 1 pixel)
  	int32_t bytes_per_row = (bottom_left.x - top_right.x + 1) / 8 + ((bottom_left.x - top_right.x  + 1) % 8 == 0 ? 0 : 1);
  	int32_t h = bottom_left.y - top_right.y + 1;
  	uint8_t* pixels = malloc(bytes_per_row * h);
  	memset(pixels, 0, bytes_per_row * h);

  	// And draw the outline path in this 1 bit image
  	GPoint prev_p = points_rot[path->num_points - 1];
  	GPoint p;
  	for(uint32_t i=0; i<path->num_points; i++){
  		p = points_rot[i];
  		bmpDrawLine(pixels, bytes_per_row, prev_p.x - top_right.x, prev_p.y - top_right.y, p.x - top_right.x, p.y - top_right.y);
  		prev_p = p;
  	}

  	free(points_rot);

  	// Compute the starting point for the flow fill algorithm 
  	// TODO tobe improved
  	GPoint start;
  	start.x = (points_rot[0].x + points_rot[1].x) / 2;
  	start.y = (points_rot[0].y + points_rot[1].y) / 2;

  	if(points_rot[0].x < points_rot[1].x){
  		if(points_rot[0].y < points_rot[1].y){
  			start.x--;
  			start.y++;
  		}
  		else {
  			start.x++;
  			start.y++;
  		}
  	}
  	else {
  		if(points_rot[0].y < points_rot[1].y){
  			start.x--;
  			start.y--;
  		}
  		else {
  			start.x++;
  			start.y--;
  		}
  	}

  	// Capture the frame buffer
  	GBitmap* bitmap = graphics_capture_frame_buffer(ctx);

  	// flood fill the gpath
  	floodFill(bitmap, pixels, bytes_per_row, start, top_right, fill_color);

  	// Release the frame buffer
  	graphics_release_frame_buffer(ctx, bitmap);  	

  	//Release the working variables
  	free(pixels);
}


// What I wanted to do here is to draw the gpath filled and draw the antialised outline like that :
// 		gpath_draw_filled(ctx, path) 
// 		gpath_draw_outline_antialiased(ctx, path) 
// but with the current API (3.0 and older) when you draw a path filled and its outline, sometimes, some pixels are not drawn between
// the outline and the interior of the form. That's not what I want...
// So I've implemented my own gpath_draw_filled : gpath_draw_filled_custom
void gpath_draw_filled_antialiased(GContext* ctx, GPath *path, GColor8 fill_color){
	// draw the filled gpath
	gpath_draw_filled_custom(ctx, path, fill_color);
	// Draw the antialiased outline around the filled gpath
	gpath_draw_outline_antialiased(ctx, path, fill_color);
}


void graphics_draw_circle_antialiased(GContext* ctx, GPoint center, uint16_t radius, GColor8 stroke_color){

	GBitmap* bitmap = graphics_capture_frame_buffer(ctx);

	uint8_t sections = 9; //TODO tweak that
	GPoint prev_p = (GPoint){0,0};
	GPoint p;
	for (uint8_t i = 0; i < sections; i++)
	{
  		int32_t angle = i * TRIG_MAX_ANGLE / (4 *  (sections - 1) );
  		int32_t s = sin_lookup(angle);
  		int32_t c = cos_lookup(angle);

  		p.x = -radius * s / TRIG_MAX_RATIO;
  		p.y =  radius * c / TRIG_MAX_RATIO;

  		// p is the point in the top right quarter (between 0 and 90°)
  		// by symmmetry we draw the other quarters
  		if(i>0){
  			draw_line_antialias_(bitmap, center.x + prev_p.x, center.y + prev_p.y, center.x + p.x, center.y + p.y, stroke_color);
  			draw_line_antialias_(bitmap, center.x - prev_p.x, center.y + prev_p.y, center.x - p.x, center.y + p.y, stroke_color);
  			draw_line_antialias_(bitmap, center.x + prev_p.x, center.y - prev_p.y, center.x + p.x, center.y - p.y, stroke_color);
  			draw_line_antialias_(bitmap, center.x - prev_p.x, center.y - prev_p.y, center.x - p.x, center.y - p.y, stroke_color);
  		}

  		prev_p = p;
	}
	graphics_release_frame_buffer(ctx, bitmap);
}

/**
  * From https://github.com/Jnmattern/Minimalist_2.0/blob/master/src/bitmap.h
  */
static void bmpFillCircle(GBitmap *bmp, GPoint center, int r, GColor8 c) {
	int x = 0, y = r, d = r-1, v;

	uint8_t* img_pixels = gbitmap_get_data(bmp);
	int16_t  w 	= gbitmap_get_bounds(bmp).size.w;
    
	while (y >= x) {
        for (v=center.x-x; v<=center.x+x; v++) img_pixels[v + w*(center.y+y)] = c.argb;
        for (v=center.x-y; v<=center.x+y; v++) img_pixels[v + w*(center.y+x)] = c.argb;
        for (v=center.x-x; v<=center.x+x; v++) img_pixels[v + w*(center.y-y)] = c.argb;
        for (v=center.x-y; v<=center.x+y; v++) img_pixels[v + w*(center.y-x)] = c.argb;
        
		if (d >= 2*x-2) {
			d = d-2*x;
			x++;
		} else if (d <= 2*r - 2*y) {
			d = d+2*y-1;
			y--;
		} else {
			d = d + 2*y - 2*x - 2;
			y--;
			x++;
		}
	}
}

void graphics_fill_circle_antialiased(GContext* ctx, GPoint center, uint16_t radius, GColor8 fill_color){
	GBitmap* bitmap = graphics_capture_frame_buffer(ctx);
	bmpFillCircle(bitmap, center, radius-1, fill_color);
	graphics_release_frame_buffer(ctx, bitmap);
	graphics_draw_circle_antialiased(ctx, center, radius, fill_color);
}

#undef swap_
#undef ipart_
#undef fpart_
#undef rfpart_
#undef abs_
#undef interpol_color_
#undef set_pixel_
#undef get_pixel

#endif  // PBL_COLOR

