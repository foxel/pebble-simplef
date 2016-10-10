#include "pebble.h"
#include "vars.h"
#include "simple.h"

#define TIME_DIGIT_HEIGHT 52
#define TIME_DISPLAY_MAX_Y 96

static Window *main_window;
static Layer *main_window_layer;
static GColor foreground_color;

static TextLayer *layer_date_text;
static TextLayer *layer_wday_text;
static TextLayer *layer_time_text;
static Layer *layer_line;

static int cur_day = -1;


static void line_layer_update_callback(Layer *layer, GContext* ctx) {
    graphics_context_set_fill_color(ctx, foreground_color);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void layer_set_y(Layer *layer, int y) {
    GRect frame = layer_get_frame(layer);
    frame.origin.y = y;
    layer_set_frame(layer, frame);
}

// Public methods
void simple_update_bounds(void) {
    // Get the full size of the screen
    GRect full_bounds = layer_get_bounds(main_window_layer);
    // Get the total available screen real-estate
    GRect bounds = layer_get_unobstructed_bounds(main_window_layer);

    int time_display_top = bounds.size.h - TIME_DIGIT_HEIGHT;
    if (time_display_top > TIME_DISPLAY_MAX_Y) {
        time_display_top = TIME_DISPLAY_MAX_Y;
    }

    layer_set_y(text_layer_get_layer(layer_time_text), time_display_top - 5);
    layer_set_y(text_layer_get_layer(layer_date_text), time_display_top - 28);
    layer_set_y(layer_line, time_display_top);

    // Hide the date if screen is obstructed
    bool hide_date = !grect_equal(&full_bounds, &bounds);
    layer_set_hidden(text_layer_get_layer(layer_wday_text), hide_date);
}

void simple_update_time(struct tm *tick_time) {
    // Need to be static because they're used by the system later.
    static char time_text[] = "00:00";
    static char date_text[] = "Xxxxxxxxx 00";
    static char wday_text[] = "Xxxxxxxxx";

    char *time_format;
    
    // Only update the date when it's changed.
    int new_cur_day = tick_time->tm_year*1000 + tick_time->tm_yday;
    if (new_cur_day != cur_day) {
        cur_day = new_cur_day;
        
        strftime(date_text, sizeof(date_text), "%B %e", tick_time);
        text_layer_set_text(layer_date_text, date_text);

        strftime(wday_text, sizeof(wday_text), "%A", tick_time);
        text_layer_set_text(layer_wday_text, wday_text);
    }

    if (clock_is_24h_style()) {
        time_format = "%R";
    } else {
        time_format = "%I:%M";
    }

    strftime(time_text, sizeof(time_text), time_format, tick_time);

    // Kludge to handle lack of non-padded hour format string
    // for twelve hour clock.
    if (!clock_is_24h_style() && (time_text[0] == '0')) {
        memmove(time_text, &time_text[1], sizeof(time_text) - 1);
    }

    text_layer_set_text(layer_time_text, time_text);
}

void simple_set_style(bool inverse) {
    foreground_color  = inverse ? GColorBlack : GColorWhite;
    
    text_layer_set_text_color(layer_time_text, foreground_color);
    text_layer_set_text_color(layer_wday_text, foreground_color);
    text_layer_set_text_color(layer_date_text, foreground_color);
}

void simple_init(Window* window) {
    main_window = window;
    main_window_layer = window_get_root_layer(window);
    foreground_color = GColorBlack;

    // Get the total available screen real-estate
    GRect bounds = layer_get_bounds(main_window_layer);

    int padding = (bounds.size.w - 144) / 2;

    // layers
    layer_time_text = text_layer_create(GRect(7, TIME_DISPLAY_MAX_Y - 5, bounds.size.w-14, TIME_DIGIT_HEIGHT));

    layer_wday_text = text_layer_create(GRect(8, 47, bounds.size.w-16, 23));
    layer_date_text = text_layer_create(GRect(8, TIME_DISPLAY_MAX_Y - 28, bounds.size.w-16, 23));
    
    text_layer_set_text_alignment(layer_time_text, GTextAlignmentCenter);
    text_layer_set_text_alignment(layer_wday_text, GTextAlignmentLeft);
    text_layer_set_text_alignment(layer_date_text, GTextAlignmentLeft);

    layer_line      = layer_create(GRect(
        padding + 8,
        TIME_DISPLAY_MAX_Y,
        128,
        2
    ));

    text_layer_set_background_color(layer_time_text, GColorClear);
    text_layer_set_font(layer_time_text, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));

    text_layer_set_background_color(layer_wday_text, GColorClear);
    text_layer_set_font(layer_wday_text, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));

    text_layer_set_background_color(layer_date_text, GColorClear);
    text_layer_set_font(layer_date_text, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));

    layer_set_update_proc(layer_line, line_layer_update_callback);

    // composing layers
    Layer *window_layer = window_get_root_layer(window);

    layer_add_child(main_window_layer, text_layer_get_layer(layer_time_text));
    layer_add_child(main_window_layer, text_layer_get_layer(layer_wday_text));
    layer_add_child(main_window_layer, text_layer_get_layer(layer_date_text));
    layer_add_child(window_layer, layer_line);
}

void simple_deinit(void) {
    text_layer_destroy(layer_time_text);
    text_layer_destroy(layer_wday_text);
    text_layer_destroy(layer_date_text);
    layer_destroy(layer_line);
}

