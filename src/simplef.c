#include "pebble.h"
#include "vars.h"

Window *window;
TextLayer *text_date_layer;
TextLayer *text_wday_layer;
TextLayer *text_time_layer;
Layer *line_layer;

BitmapLayer *batteryLayer;
BitmapLayer *bluetoothLayer;
GBitmap *img_battery_full;
GBitmap *img_battery_half;
GBitmap *img_battery_low;
GBitmap *img_battery_charge;
GBitmap *img_bt_connect;
GBitmap *img_bt_disconnect;
GRect battery_rect;
GRect bluetooth_rect;
TextLayer *batterytext_layer;
int charge_percent = 0;
int cur_day = -1;

void handle_battery(BatteryChargeState charge_state) {
    static char battery_text[] = "100 ";

    if (charge_state.is_charging) {
        bitmap_layer_set_bitmap(batteryLayer, img_battery_charge);

        snprintf(battery_text, sizeof(battery_text), "+%d", charge_state.charge_percent);
    } else {
        snprintf(battery_text, sizeof(battery_text), "%d", charge_state.charge_percent);
        if (charge_state.charge_percent <= 20) {
            bitmap_layer_set_bitmap(batteryLayer, img_battery_low);
        } else if (charge_state.charge_percent <= 50) {
            bitmap_layer_set_bitmap(batteryLayer, img_battery_half);
        } else {
            bitmap_layer_set_bitmap(batteryLayer, img_battery_full);
        }
            
        if (charge_state.charge_percent < charge_percent) {
            if (charge_state.charge_percent==20){
                vibes_double_pulse();
            } else if(charge_state.charge_percent==10){
                vibes_long_pulse();
            }
        }
    }
    charge_percent = charge_state.charge_percent;
    
    text_layer_set_text(batterytext_layer, battery_text);
}

void handle_bluetooth(bool connected) {
    if (connected) {
        bitmap_layer_set_bitmap(bluetoothLayer, img_bt_connect);
    } else {
        bitmap_layer_set_bitmap(bluetoothLayer, img_bt_disconnect);
        vibes_long_pulse();
    }
}

void handle_appfocus(bool in_focus){
    if (in_focus) {
        handle_bluetooth(bluetooth_connection_service_peek());
        handle_battery(battery_state_service_peek());
    }
}

void line_layer_update_callback(Layer *layer, GContext* ctx) {
    graphics_context_set_fill_color(ctx, foreground_color);
    graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

void update_time(struct tm *tick_time) {
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
        text_layer_set_text(text_date_layer, date_text);

        strftime(wday_text, sizeof(wday_text), "%A", tick_time);
        text_layer_set_text(text_wday_layer, wday_text);
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

    text_layer_set_text(text_time_layer, time_text);
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    update_time(tick_time);
}

void handle_deinit(void) {
    tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
    app_focus_service_unsubscribe();
}

void handle_init(void) {
    window = window_create();
    window_stack_push(window, true /* Animated */);
    window_set_background_color(window, background_color);

    Layer *window_layer = window_get_root_layer(window);

    text_wday_layer = text_layer_create(GRect(8, 47, 144-8, 168-68));
    text_layer_set_text_color(text_wday_layer, foreground_color);
    text_layer_set_background_color(text_wday_layer, GColorClear);
    text_layer_set_font(text_wday_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
    layer_add_child(window_layer, text_layer_get_layer(text_wday_layer));

    text_date_layer = text_layer_create(GRect(8, 68, 144-8, 168-68));
    text_layer_set_text_color(text_date_layer, foreground_color);
    text_layer_set_background_color(text_date_layer, GColorClear);
    text_layer_set_font(text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
    layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

    text_time_layer = text_layer_create(GRect(7, 92, 144-7, 168-92));
    text_layer_set_text_color(text_time_layer, foreground_color);
    text_layer_set_background_color(text_time_layer, GColorClear);
    text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
    layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

    GRect line_frame = GRect(8, 97, 128, 2);
    line_layer = layer_create(line_frame);
    layer_set_update_proc(line_layer, line_layer_update_callback);
    layer_add_child(window_layer, line_layer);

    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

    // Mods here
    img_bt_connect     = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CONNECT);
    img_bt_disconnect  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECT);
    img_battery_full   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_FULL);
    img_battery_half   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_HALF);
    img_battery_low    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_LOW);
    img_battery_charge = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_CHARGE);

    battery_rect = GRect(10,10,16,16);
    bluetooth_rect = GRect(118,12,20,20);
    
    batteryLayer   = bitmap_layer_create(battery_rect);
    bluetoothLayer = bitmap_layer_create(bluetooth_rect);
    bitmap_layer_set_compositing_mode(batteryLayer, composition_mode);
    bitmap_layer_set_compositing_mode(bluetoothLayer, composition_mode);
    
    bitmap_layer_set_bitmap(batteryLayer, img_battery_full);
    bitmap_layer_set_bitmap(bluetoothLayer, img_bt_connect);

    batterytext_layer = text_layer_create(GRect(3,20,30,20));
    text_layer_set_text_color(batterytext_layer, foreground_color);
    text_layer_set_background_color(batterytext_layer, GColorClear);
    text_layer_set_font(batterytext_layer, fonts_get_system_font(FONT_KEY_FONT_FALLBACK));
    text_layer_set_text_alignment(batterytext_layer, GTextAlignmentCenter);

    layer_add_child(window_layer, bitmap_layer_get_layer(batteryLayer));
    layer_add_child(window_layer, bitmap_layer_get_layer(bluetoothLayer));
    layer_add_child(window_layer, text_layer_get_layer(batterytext_layer));

    battery_state_service_subscribe(&handle_battery);
    bluetooth_connection_service_subscribe(&handle_bluetooth);
    app_focus_service_subscribe(&handle_appfocus);

    handle_battery(battery_state_service_peek());
    handle_bluetooth(bluetooth_connection_service_peek());
    time_t now = time(NULL);
    update_time(localtime(&now));
}


int main(void) {
    handle_init();

    app_event_loop();

    handle_deinit();
}
