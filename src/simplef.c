#include "pebble.h"

Window *window;
TextLayer *text_date_layer;
TextLayer *text_time_layer;
Layer *line_layer;

BitmapLayer *batteryLayer;
BitmapLayer *bluetoothLayer;
GBitmap *battery;
GBitmap *bluetooth;
GBitmap *disconnect;
GRect battery_rect;
GRect bluetooth_rect;
TextLayer *batterytext_layer;

void handle_battery(BatteryChargeState charge_state) {
    static char battery_text[] = "100 ";
    if (charge_state.is_charging) {
        snprintf(battery_text, sizeof(battery_text), "+%d", charge_state.charge_percent);
    } else {
        snprintf(battery_text, sizeof(battery_text), "%d", charge_state.charge_percent);
        if (charge_state.charge_percent==20){
            vibes_double_pulse();
        } else if(charge_state.charge_percent==10){
            vibes_long_pulse();
        }
    }
    text_layer_set_text(batterytext_layer, battery_text);
}

void handle_bluetooth(bool connected) {
    Layer *window_layer = window_get_root_layer(window);
    
    layer_remove_from_parent(bitmap_layer_get_layer(bluetoothLayer));
    bitmap_layer_destroy(bluetoothLayer);

    bluetoothLayer = bitmap_layer_create(bluetooth_rect);
    if (connected) {
        bitmap_layer_set_bitmap(bluetoothLayer, bluetooth);
    } else {
        bitmap_layer_set_bitmap(bluetoothLayer, disconnect);
        vibes_long_pulse();
    }
    layer_add_child(window_layer, bitmap_layer_get_layer(bluetoothLayer));
}

void handle_appfocus(bool in_focus){
    if (in_focus) {
        handle_bluetooth(bluetooth_connection_service_peek());
    }
}


void line_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char time_text[] = "00:00";
  static char date_text[] = "Xxxxxxxxx 00";

  char *time_format;


  // TODO: Only update the date when it's changed.
  strftime(date_text, sizeof(date_text), "%B %e", tick_time);
  text_layer_set_text(text_date_layer, date_text);


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

void handle_deinit(void) {
  tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
    app_focus_service_unsubscribe();
}

void handle_init(void) {
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  Layer *window_layer = window_get_root_layer(window);

  text_date_layer = text_layer_create(GRect(8, 68, 144-8, 168-68));
  text_layer_set_text_color(text_date_layer, GColorWhite);
  text_layer_set_background_color(text_date_layer, GColorClear);
  text_layer_set_font(text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
  layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

  text_time_layer = text_layer_create(GRect(7, 92, 144-7, 168-92));
  text_layer_set_text_color(text_time_layer, GColorWhite);
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
  layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

  GRect line_frame = GRect(8, 97, 128, 2);
  line_layer = layer_create(line_frame);
  layer_set_update_proc(line_layer, line_layer_update_callback);
  layer_add_child(window_layer, line_layer);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  // TODO: Update display here to avoid blank display on launch?

  // Mods here
    bluetooth = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
    disconnect = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECT);
    battery = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);

    battery_rect = GRect(10,10,16,16);
    bluetooth_rect = GRect(118,12,20,20);
    
    batteryLayer = bitmap_layer_create(battery_rect);
    bitmap_layer_set_bitmap(batteryLayer, battery);

    bluetoothLayer = bitmap_layer_create(bluetooth_rect);
    bitmap_layer_set_bitmap(bluetoothLayer, bluetooth);

    batterytext_layer = text_layer_create(GRect(3,20,30,20));
    text_layer_set_text_color(batterytext_layer, GColorWhite);
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
}


int main(void) {
  handle_init();

  app_event_loop();
  
  handle_deinit();
}
