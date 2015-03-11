#include <pebble.h>

#include "antialiasing.h"

// This is an example of a path that looks like a compound path
// If you rotate it however, you will see it is a single shape
static const GPathInfo INFINITY_RECT_PATH_POINTS = {
  16,
  (GPoint []) {
    {-50, 0},
    {-50, -60},
    {10, -60},
    {10, -20},
    {-10, -20},
    {-10, -40},
    {-30, -40},
    {-30, -20},
    {50, -20},
    {50, 40},
    {-10, 40},
    {-10, 0},
    {10, 0},
    {10, 20},
    {30, 20},
    {30, 0}
  }
};

// This defines graphics path information to be loaded as a path later
static const GPathInfo HOUSE_PATH_POINTS = {
  // This is the amount of points
  11,
  // A path can be concave, but it should not twist on itself
  // The points should be defined in clockwise order due to the rendering
  // implementation. Counter-clockwise will work in older firmwares, but
  // it is not officially supported
  (GPoint []) {
    {-40, 0},
    {0, -40},
    {40, 0},
    {28, 0},
    {28, 40},
    {10, 40},
    {10, 16},
    {-10, 16},
    {-10, 40},
    {-28, 40},
    {-28, 0}
  }
};

static GPath *s_house_path, *s_infinity_path;

static Window *window;
static Layer *layer;
static AppTimer* timer;

static uint8_t  stroke_color = GColorBrightGreenARGB8;
static uint8_t  background_color = 0;
static bool     antialias = false;
static time_t   prev_t = 0;
static uint8_t  frames = 0;
static uint8_t  fps = 0;
static char     s_fps[64] = "fps";
static uint16_t angle = 209;

static void timer_callback(void *data) {
  angle++;
  angle = angle % 360;
  gpath_rotate_to(s_infinity_path, (TRIG_MAX_ANGLE * angle) / 360);
  gpath_rotate_to(s_house_path, (TRIG_MAX_ANGLE * angle) / 360);
  layer_mark_dirty(layer);
}

static void update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int16_t w = bounds.size.w;
  int16_t h = bounds.size.h;

  // Set fill color
  graphics_context_set_fill_color(ctx, (GColor8){.argb=(0xC0 + background_color)});

  // Set text color
  graphics_context_set_text_color(ctx, GColorWhite);

  // Set stroke color
  graphics_context_set_stroke_color(ctx, (GColor8){.argb=(0xC0 + stroke_color)});

  // Draw background
  graphics_fill_rect(ctx,bounds,0,0);

  // Draw lines
  for(int i=0; i<10; i++){
    if(antialias)
    {
      graphics_draw_line_antialiased(ctx, (GPoint){0,i*h/10}, (GPoint){w*i/10,h}, (GColor8){.argb=(0xC0 + stroke_color)});
      graphics_draw_line_antialiased(ctx, (GPoint){w*i/10,0}, (GPoint){0,h - h*i/10}, (GColor8){.argb=(0xC0 + stroke_color)});
      graphics_draw_line_antialiased(ctx, (GPoint){w*i/10,0}, (GPoint){w,h*i/10}, (GColor8){.argb=(0xC0 + stroke_color)});
      graphics_draw_line_antialiased(ctx, (GPoint){w*i/10,h}, (GPoint){w,h-h*i/10}, (GColor8){.argb=(0xC0 + stroke_color)});
    }
    else
    {
      graphics_draw_line(ctx, (GPoint){0,i*h/10}, (GPoint){w*i/10,h});
      graphics_draw_line(ctx, (GPoint){w*i/10,0}, (GPoint){0,h - h*i/10});
      graphics_draw_line(ctx, (GPoint){w*i/10,0}, (GPoint){w,h*i/10});
      graphics_draw_line(ctx, (GPoint){w*i/10,h}, (GPoint){w,h-h*i/10});
    }
  }

  if(antialias)
  {
    gpath_draw_filled_antialiased(ctx, s_infinity_path, (GColor8){.argb=(0xC0 + stroke_color)});
    gpath_draw_filled_antialiased(ctx, s_house_path, (GColor8){.argb=(0xC0 + stroke_color)});
  }
  else 
  {
    graphics_context_set_fill_color(ctx, (GColor8){.argb=(0xC0 + stroke_color)});
    gpath_draw_filled(ctx, s_infinity_path);
    gpath_draw_filled(ctx, s_house_path);
  }

  graphics_draw_text(ctx, antialias ? "AA" : " ", fonts_get_system_font(FONT_KEY_FONT_FALLBACK), GRect(0, 0, w, 30), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);

  // Update fps counter
  time_t now = time(NULL);

  frames++;

  if(now - prev_t > 1){
    prev_t = now;
    fps = frames;
    snprintf(s_fps, sizeof(s_fps), "%d fps", fps);
    frames = 0;
  }
  
  graphics_draw_text(ctx, s_fps, fonts_get_system_font(FONT_KEY_FONT_FALLBACK), GRect(0, h-30, w, 30), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  
  // force redraw
  timer = app_timer_register(10, timer_callback, NULL);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  antialias = !antialias;
  layer_mark_dirty(layer);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  background_color++;
  background_color = background_color%64;
  layer_mark_dirty(layer);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  stroke_color++;
  stroke_color = stroke_color%64;
  layer_mark_dirty(layer);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  layer = layer_create(bounds);
  layer_set_update_proc(layer,update_proc);
  layer_add_child(window_layer, layer);

  s_infinity_path = gpath_create(&INFINITY_RECT_PATH_POINTS);
  gpath_move_to(s_infinity_path,(GPoint){144/2,168/4});
  s_house_path = gpath_create(&HOUSE_PATH_POINTS);
  gpath_move_to(s_house_path,(GPoint){144/2,3*168/4});
}

static void window_unload(Window *window) {
  layer_destroy(layer);

  gpath_destroy(s_infinity_path);
  gpath_destroy(s_house_path);
}

static void init(void) {
  window = window_create();
  window_set_fullscreen(window,true);
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
