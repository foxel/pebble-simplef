#include "pebble.h"
#include "vars.h"
#include "termo.h"

#define MAX_AGE 3600

static TextLayer *s_weather_layer;
static GFont s_weather_font;
static int termo_timestamp = 0;

static char weather_layer_buffer[] = "-18.50C";

void termo_inbox_received(DictionaryIterator *iterator, void *context) {

    // Look for item
    Tuple *t = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
 
    // if there are some data
    if (t) {
        snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s", t->value->cstring);
        termo_timestamp = time(NULL);
        persist_write_string(TERMO_KEY, weather_layer_buffer);
        persist_write_int(TERMO_TS_KEY, termo_timestamp);
        // display
        text_layer_set_text(s_weather_layer, weather_layer_buffer);
    }
}

static void check_termo_age(void) {
    int age = time(NULL) - termo_timestamp;
    if (age > MAX_AGE) { // clear temperature
        text_layer_set_text(s_weather_layer, "...");
    }
}
 
// public methods
void termo_set_style(bool inverse) {
    GColor foreground_color  = inverse ? GColorBlack : GColorWhite;
    text_layer_set_text_color(s_weather_layer, foreground_color);
}

void termo_update_time(struct tm *tick_time) {
    // Get weather update every 30 minutes
    if (tick_time->tm_min % 15 == 0) {
        if (bluetooth_connection_service_peek()) {
            // Begin dictionary
            DictionaryIterator *iter;
            app_message_outbox_begin(&iter);

            // Add a key-value pair
            dict_write_uint8(iter, 0, 0);

            // Send the message!
            app_message_outbox_send();
        }
        
        check_termo_age();
    }
}

void termo_init(Window* window) {
    Layer *window_layer = window_get_root_layer(window);
    // Get the total available screen real-estate
    GRect bounds = layer_get_bounds(window_layer);
    int padding_v = PBL_IF_ROUND_ELSE(16, 8);

    // Create temperature Layer
    s_weather_layer = text_layer_create(GRect(
        (bounds.size.w - 80)/2,
        PBL_IF_ROUND_ELSE(16, 8),
        80,
        23
    ));
    text_layer_set_background_color(s_weather_layer, GColorClear);
    text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
    text_layer_set_text(s_weather_layer, "...");
    if (persist_exists(TERMO_KEY)) {
        termo_timestamp = persist_read_int(TERMO_TS_KEY);
        int age = time(NULL) - termo_timestamp;
        if (age < MAX_AGE) { // restore only temp stored less than MAX_AGE
            persist_read_string(TERMO_KEY, weather_layer_buffer, sizeof(weather_layer_buffer));
            text_layer_set_text(s_weather_layer, weather_layer_buffer);
        }
    }

    text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
    layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));

}

void termo_deinit(void) {
    text_layer_destroy(s_weather_layer);
    app_message_deregister_callbacks();
}
