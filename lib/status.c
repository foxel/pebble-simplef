#include "pebble.h"
#include "vars.h"
#include "status.h"

static BitmapLayer *layer_batt_img;
static BitmapLayer *layer_conn_img;

static GBitmap *img_battery_full;
static GBitmap *img_battery_half;
static GBitmap *img_battery_low;
static GBitmap *img_battery_charge;
static GBitmap *img_bt_connect;
static GBitmap *img_bt_disconnect;

static TextLayer *layer_batt_text;
static int charge_percent = 0;

static const uint32_t const segments[] = { 300, 100, 300, 100, 300 };
static VibePattern panicPattern = {
  .durations = segments,
  .num_segments = ARRAY_LENGTH(segments),
};

static void handle_battery(BatteryChargeState charge_state) {
    static char battery_text[] = "100 ";

    if (charge_state.is_charging) {
        bitmap_layer_set_bitmap(layer_batt_img, img_battery_charge);

        snprintf(battery_text, sizeof(battery_text), "+%d", charge_state.charge_percent);
        #if defined(PBL_COLOR)
        text_layer_set_text_color(layer_batt_text, GColorGreen);
        #endif
    } else {
        snprintf(battery_text, sizeof(battery_text), "%d", charge_state.charge_percent);
        if (charge_state.charge_percent <= 20) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_low);
            #if defined(PBL_COLOR)
            text_layer_set_text_color(layer_batt_text, GColorRed);
            #endif
        } else if (charge_state.charge_percent <= 50) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_half);
            #if defined(PBL_COLOR)
            text_layer_set_text_color(layer_batt_text, GColorYellow);
            #endif
        } else {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_full);
            #if defined(PBL_COLOR)
            text_layer_set_text_color(layer_batt_text, GColorGreen);
            #endif
        }

        /*if (charge_state.charge_percent < charge_percent) {
            if (charge_state.charge_percent==20){
                vibes_double_pulse();
            } else if(charge_state.charge_percent==10){
                vibes_long_pulse();
            }
        }*/ 
    }
    charge_percent = charge_state.charge_percent;
    
    text_layer_set_text(layer_batt_text, battery_text);
}

static void update_bluetooth(bool connected) {
    if (connected) {
        bitmap_layer_set_bitmap(layer_conn_img, img_bt_connect);
    } else {
        bitmap_layer_set_bitmap(layer_conn_img, img_bt_disconnect);
    }
}

static void handle_bluetooth(bool connected) {
    update_bluetooth(connected);
    
    if (!connected) {
        //vibes_long_pulse();
        vibes_enqueue_custom_pattern(panicPattern);
    }
}

static void handle_appfocus(bool in_focus){
    if (in_focus) {
        handle_bluetooth(bluetooth_connection_service_peek());
        handle_battery(battery_state_service_peek());
    }
}

// public methods
void status_set_style(bool inverse) {
    #if defined(PBL_COLOR)
    GCompOp compositing_mode = GCompOpSet;
    #else
    text_layer_set_text_color(layer_batt_text, inverse ? GColorBlack : GColorWhite);

    GCompOp compositing_mode = inverse ? GCompOpAssign : GCompOpAssignInverted;
    #endif

    bitmap_layer_set_compositing_mode(layer_batt_img, compositing_mode);
    bitmap_layer_set_compositing_mode(layer_conn_img, compositing_mode);
}

void status_update(void) {
    handle_battery(battery_state_service_peek());
    update_bluetooth(bluetooth_connection_service_peek());
}

void status_init(Window* window) {
    // resources
    img_bt_connect     = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CONNECT);
    img_bt_disconnect  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECT);
    img_battery_full   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_FULL);
    img_battery_half   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_HALF);
    img_battery_low    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_LOW);
    img_battery_charge = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_CHARGE);

    // layers
    layer_batt_text = text_layer_create(GRect(3,20,30,20));
    layer_batt_img  = bitmap_layer_create(GRect(10, 10, 16, 16));
    layer_conn_img  = bitmap_layer_create(GRect(118, 12, 20, 20));

    text_layer_set_background_color(layer_batt_text, GColorClear);
    text_layer_set_font(layer_batt_text, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(layer_batt_text, GTextAlignmentCenter);

    bitmap_layer_set_bitmap(layer_batt_img, img_battery_full);
    bitmap_layer_set_bitmap(layer_conn_img, img_bt_connect);

    // composing layers
    Layer *window_layer = window_get_root_layer(window);

    layer_add_child(window_layer, bitmap_layer_get_layer(layer_batt_img));
    layer_add_child(window_layer, bitmap_layer_get_layer(layer_conn_img));
    layer_add_child(window_layer, text_layer_get_layer(layer_batt_text));

    // handlers
    battery_state_service_subscribe(&handle_battery);
    bluetooth_connection_service_subscribe(&handle_bluetooth);
    app_focus_service_subscribe(&handle_appfocus);
}

void status_deinit(void) {
    battery_state_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
    app_focus_service_unsubscribe();

    text_layer_destroy(layer_batt_text);
    bitmap_layer_destroy(layer_batt_img);
    bitmap_layer_destroy(layer_conn_img);

    gbitmap_destroy(img_bt_connect);
    gbitmap_destroy(img_bt_disconnect);
    gbitmap_destroy(img_battery_full);
    gbitmap_destroy(img_battery_half);
    gbitmap_destroy(img_battery_low);
    gbitmap_destroy(img_battery_charge);
}
