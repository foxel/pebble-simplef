#include "pebble.h"
#include "vars.h"
#include "simplebig.h"
#include "status.h"

Window *window;

static void update_time(struct tm *tick_time) {
    simplebig_update_time(tick_time);
}

static void set_style(void) {
    bool inverse = persist_read_bool(STYLE_KEY);
    
    GColor background_color  = inverse ? GColorWhite : GColorBlack;
    
    window_set_background_color(window, background_color);

    simplebig_set_style(inverse);
    status_set_style(inverse);
}

static void update_bounds(void) {
    simplebig_update_bounds();
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

static void handle_init(void) {
    window = window_create();
    window_stack_push(window, true /* Animated */);

    // child init
    simplebig_init(window);
    status_init(window);

    // handlers
    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
    accel_tap_service_subscribe(handle_tap);
    app_timer_register(2000, handle_tap_timeout, NULL);
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
    simplebig_deinit();
    
    tick_timer_service_unsubscribe();
    accel_tap_service_unsubscribe();
    
    window_destroy(window);
}


int main(void) {
    handle_init();

    app_event_loop();

    handle_deinit();
}
