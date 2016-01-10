#include "simple_analog.h"
#include "pebble.h"
#include "PDUtils.h"


static Window *s_window;
static Layer *s_simple_bg_layer, *s_date_layer, *s_hands_layer;
static TextLayer *s_day_label, *s_num_label, *s_count_label;

static GPath *s_tick_paths[NUM_CLOCK_TICKS];
static GPath *s_minute_arrow, *s_hour_arrow;
static GPath *s_triangle;

static char s_num_buffer[4], s_day_buffer[6], s_count_buffer[10], s_date_buffer[10];

static int EVENT_MONTH = 11;
static int EVENT_DAY = 8;
static int EVENT_YEAR = 14;
static int EVENT_HOUR = 0;
static int EVENT_MINUTE = 0;

static void bg_update_proc(Layer *layer, GContext *ctx) {
  int i;
  const int x_offset = PBL_IF_ROUND_ELSE(18, 0);
  const int y_offset = PBL_IF_ROUND_ELSE(6, 0);
  
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  
  graphics_context_set_fill_color(ctx, GColorBlueMoon);
  graphics_context_set_stroke_color(ctx, GColorBlueMoon);
  gpath_draw_outline(ctx, s_triangle);
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  for (i = 0; i < NUM_CLOCK_TICKS_WHITE; ++i) {
    gpath_move_to(s_tick_paths[i], GPoint(x_offset, y_offset));
    gpath_draw_filled(ctx, s_tick_paths[i]);
  }
  
  graphics_context_set_fill_color(ctx, GColorRed);
  for (; i < NUM_CLOCK_TICKS_RED; ++i) {
    gpath_move_to(s_tick_paths[i], GPoint(x_offset, y_offset));
    gpath_draw_filled(ctx, s_tick_paths[i]);
  }
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);

  const int16_t second_hand_length = PBL_IF_ROUND_ELSE((bounds.size.w / 2) - 19, bounds.size.w / 2);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
//  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
//  GPoint second_hand = {
//    .x = (int16_t)(sin_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.x,
//    .y = (int16_t)(-cos_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.y,
//  };

  // second hand
//  graphics_context_set_stroke_color(ctx, GColorRed);
//  graphics_draw_line(ctx, second_hand, center);

  // minute/hour hand
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
  gpath_draw_outline(ctx, s_minute_arrow);

  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  gpath_draw_outline(ctx, s_hour_arrow);

  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 1, bounds.size.h / 2 - 1, 3, 3), 0, GCornerNone);
}

static void date_update_proc(Layer *layer, GContext *ctx) {
  time_t t = time(NULL);
  struct tm *now = localtime(&t);
  int difference;
  time_t seconds_now, seconds_event;

  strftime(s_day_buffer, sizeof(s_day_buffer), "%a", now);
  strftime(s_num_buffer, sizeof(s_num_buffer), "%d", now);
  snprintf(s_date_buffer,sizeof(s_date_buffer),"%s %s",s_day_buffer,s_num_buffer);
  text_layer_set_text(s_day_label, s_date_buffer);
  
  // Set the current time
  seconds_now = p_mktime(now);
	
	now->tm_year = EVENT_YEAR + 100;
	now->tm_mon = EVENT_MONTH - 1;
	now->tm_mday = EVENT_DAY;
	now->tm_hour = EVENT_HOUR;
	now->tm_min = EVENT_MINUTE;
	now->tm_sec = 0;
	
	seconds_event = p_mktime(now);
	
	// Determine the time difference
	difference = ((((seconds_now - seconds_event) / 60) / 60) / 24);

  snprintf (s_count_buffer,sizeof(s_count_buffer),"%d Days",difference);
  text_layer_set_text(s_count_label, s_count_buffer);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_window));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);

  s_date_layer = layer_create(bounds);
  layer_set_update_proc(s_date_layer, date_update_proc);
  layer_add_child(window_layer, s_date_layer);

  s_day_label = text_layer_create(PBL_IF_ROUND_ELSE(
    GRect(48, 40, 90, 20),
    GRect(31, 40, 90, 20)));
  text_layer_set_text_alignment(s_day_label,GTextAlignmentCenter);
  text_layer_set_text(s_day_label, s_day_buffer);
  text_layer_set_background_color(s_day_label, GColorBlack);
  text_layer_set_text_color(s_day_label, GColorWhite);
  text_layer_set_font(s_day_label, fonts_get_system_font(FONT_KEY_GOTHIC_18));

  layer_add_child(s_date_layer, text_layer_get_layer(s_day_label));

  s_count_label = text_layer_create(PBL_IF_ROUND_ELSE(
    GRect(48, 114, 90, 20),
    GRect(31, 114, 90, 20)));
  text_layer_set_text_alignment(s_count_label,GTextAlignmentCenter);
  text_layer_set_text(s_count_label, s_count_buffer);
  text_layer_set_background_color(s_count_label, GColorBlack);
  text_layer_set_text_color(s_count_label, GColorWhite);
  text_layer_set_font(s_count_label, fonts_get_system_font(FONT_KEY_GOTHIC_18));

  layer_add_child(s_date_layer, text_layer_get_layer(s_count_label));
  
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_simple_bg_layer);
  layer_destroy(s_date_layer);

  text_layer_destroy(s_day_label);
  text_layer_destroy(s_count_label);
    

  layer_destroy(s_hands_layer);
}

static void init() {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  s_day_buffer[0] = '\0';
  s_num_buffer[0] = '\0';
  s_num_buffer[0] = '\0';
  

  // init hand paths
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);

  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    s_tick_paths[i] = gpath_create(&ANALOG_BG_POINTS[i]);
  }
  
  TRIANGLE_POINTS.points[0].x = bounds.size.w / 2;
  TRIANGLE_POINTS.points[1].x = bounds.size.h * .11;
  TRIANGLE_POINTS.points[1].y = bounds.size.h * .72;
  TRIANGLE_POINTS.points[2].x = bounds.size.h * .88;
  TRIANGLE_POINTS.points[2].y = bounds.size.h * .72;
    
  s_triangle = gpath_create(&TRIANGLE_POINTS);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_second_tick);
}

static void deinit() {
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_destroy(s_tick_paths[i]);
  }

  gpath_destroy(s_triangle);
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
