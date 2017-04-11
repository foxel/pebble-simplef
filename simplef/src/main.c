#include "pebble.h"
#include "vars.h"
#include "simple.h"
#include "status.h"

Window *window;

static void update_time(struct tm *tick_time) {
    simple_update_time(tick_time);
}

static void set_style(void) {
    bool inverse = persist_read_bool(STYLE_KEY);
    
    GColor background_color  = inverse ? GColorWhite : GColorBlack;
    
    window_set_background_color(window, background_color);

    simple_set_style(inverse);
    status_set_style(inverse);
}

static void update_bounds(void) {
    simple_update_bounds();
}

static void force_update(void) {
    status_update();

    time_t now = time(NULL);
    update_time(localtime(&now));

    update_bounds();
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    update_time(tick_time);
}

static void handle_tap(AccelAxisType axis, int32_t direction) {
    persist_write_bool(STYLE_KEY, !persist_read_bool(STYLE_KEY));
    set_style();
    force_update();
    vibes_long_pulse();
    accel_tap_service_unsubscribe();
}

static void handle_tap_timeout(void* data) {
    accel_tap_service_unsubscribe();
}

static void handle_unobstructed_change(AnimationProgress progress, void *context) {
    update_bounds();
}

// MESSAGING
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {

    // Look for item
    Tuple *t = dict_find(iterator, MESSAGE_KEY_INVERSE);
 
    // For all items
    if (t) {
        persist_write_bool(STYLE_KEY, t->value->int32 == 1);
        set_style();
        force_update();
        vibes_long_pulse();
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}
// /MESSAGING

static void handle_init(void) {
    window = window_create();
    window_stack_push(window, true /* Animated */);

    // child init
    simple_init(window);
    status_init(window);

    // Register callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

    // Open AppMessage
    AppMessageResult result = app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
    if (result != APP_MSG_OK) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Can't open inbox");
    }

    // handlers
    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
    UnobstructedAreaHandlers ua_handler = {
        .change = handle_unobstructed_change
    };
    unobstructed_area_service_subscribe(ua_handler, NULL);

    // style
    set_style();

    // draw first frame
    force_update();
}

static void handle_deinit(void) {
    status_deinit();
    simple_deinit();
    
    tick_timer_service_unsubscribe();
    
    window_destroy(window);
}


int main(void) {
    handle_init();

    app_event_loop();

    handle_deinit();
}
