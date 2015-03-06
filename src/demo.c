#include <pebble.h>

#include "antialiasing.h"

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

static void timer_callback(void *data) {
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
    if(antialias){
      graphics_draw_line_antialiased(ctx, (GPoint){0,i*h/10}, (GPoint){w*i/10,h});
      graphics_draw_line_antialiased(ctx, (GPoint){w*i/10,0}, (GPoint){0,h - h*i/10});
      graphics_draw_line_antialiased(ctx, (GPoint){w*i/10,0}, (GPoint){w,h*i/10});
      graphics_draw_line_antialiased(ctx, (GPoint){w*i/10,h}, (GPoint){w,h-h*i/10});
    }
    else{
      graphics_draw_line(ctx, (GPoint){0,i*h/10}, (GPoint){w*i/10,h});
      graphics_draw_line(ctx, (GPoint){w*i/10,0}, (GPoint){0,h - h*i/10});
      graphics_draw_line(ctx, (GPoint){w*i/10,0}, (GPoint){w,h*i/10});
      graphics_draw_line(ctx, (GPoint){w*i/10,h}, (GPoint){w,h-h*i/10});
    }
  }


  graphics_draw_text(ctx, antialias ? "AA" : " ", fonts_get_system_font(FONT_KEY_FONT_FALLBACK), GRect(0, 0, w, 30), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);


  // Update fps counter
  time_t t = 0;
  time(&t);

  frames++;

  if(t - prev_t > 1){
    prev_t = t;
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
}

static void window_unload(Window *window) {
  layer_destroy(layer);
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
