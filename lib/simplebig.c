#include "pebble.h"
#include "vars.h"
#include "simplebig.h"

#define TOTAL_IMAGE_SLOTS 4

#define NUMBER_OF_IMAGES 10
#define DIGIT_IMAGE_WIDTH 34
#define DIGIT_IMAGE_HEIGHT 84
#define EMPTY_SLOT -1

// generated by the `fonttools/font2png.py` script.
static const int IMAGE_RESOURCE_IDS[NUMBER_OF_IMAGES] = {
    RESOURCE_ID_IMAGE_NUM_0, RESOURCE_ID_IMAGE_NUM_1, RESOURCE_ID_IMAGE_NUM_2,
    RESOURCE_ID_IMAGE_NUM_3, RESOURCE_ID_IMAGE_NUM_4, RESOURCE_ID_IMAGE_NUM_5,
    RESOURCE_ID_IMAGE_NUM_6, RESOURCE_ID_IMAGE_NUM_7, RESOURCE_ID_IMAGE_NUM_8,
    RESOURCE_ID_IMAGE_NUM_9
};

static Window *main_window;
static Layer *main_window_layer;
static GColor foreground_color;
static GCompOp compositing_mode = GCompOpAssign;

static GBitmap *digit_images[TOTAL_IMAGE_SLOTS];
static BitmapLayer *digit_layers[TOTAL_IMAGE_SLOTS];

static int image_slot_state[TOTAL_IMAGE_SLOTS] = {EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT};

static TextLayer *layer_date_text;
static TextLayer *layer_wday_text;
static Layer *layer_line;

static BitmapLayer *layer_sep_img;
static GBitmap *img_dig_separator;
static int cur_day = -1;


static void load_digit_image_into_slot(int slot_number, int digit_value) {

    if ((slot_number < 0) || (slot_number >= TOTAL_IMAGE_SLOTS)) {
        return;
    }

    if ((digit_value < 0) || (digit_value > 9)) {
        return;
    }

    if (image_slot_state[slot_number] == digit_value) {
        return;
    }

    GBitmap* oldBitmap = digit_images[slot_number];
    digit_images[slot_number] = gbitmap_create_with_resource(IMAGE_RESOURCE_IDS[digit_value]);
    BitmapLayer *bitmap_layer = digit_layers[slot_number];
    bitmap_layer_set_bitmap(bitmap_layer, digit_images[slot_number]);
    layer_set_hidden(bitmap_layer_get_layer(bitmap_layer), false);
    gbitmap_destroy(oldBitmap);
    image_slot_state[slot_number] = digit_value;
}

static void unload_digit_image_from_slot(int slot_number) {
    if (image_slot_state[slot_number] != EMPTY_SLOT) {
        layer_set_hidden(bitmap_layer_get_layer(digit_layers[slot_number]), true);
        image_slot_state[slot_number] = EMPTY_SLOT;
    }
}

static void display_time_value(unsigned short value, unsigned short row_number, bool show_first_leading_zero) {
    value = value % 100; // Maximum of two digits per row.
    for (int column_number = 1; column_number >= 0; column_number--) {
        int slot_number = (row_number * 2) + column_number;
        if (!((value == 0) && (column_number == 0) && !show_first_leading_zero)) {
            load_digit_image_into_slot(slot_number, value % 10);
        } else {
            unload_digit_image_from_slot(slot_number);
        }
        value = value / 10;
    }
}

static unsigned short get_display_hour(unsigned short hour) {
    if (clock_is_24h_style()) {
        return hour;
    }
    unsigned short display_hour = hour % 12;
    return display_hour ? display_hour : 12;
}

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
void simplebig_update_bounds(void) {
    // Get the full size of the screen
    GRect full_bounds = layer_get_bounds(main_window_layer);
    // Get the total available screen real-estate
    GRect bounds = layer_get_unobstructed_bounds(main_window_layer);

    int time_display_top = bounds.size.h - DIGIT_IMAGE_HEIGHT;
    for (int i = 0; i < 4; i++) {
        layer_set_y(bitmap_layer_get_layer(digit_layers[i]), time_display_top);
    }
    layer_set_y(bitmap_layer_get_layer(layer_sep_img), time_display_top);
    layer_set_y(layer_line, time_display_top);

    // Hide the date if screen is obstructed
    bool hide_date = !grect_equal(&full_bounds, &bounds);
    layer_set_hidden(text_layer_get_layer(layer_date_text), hide_date);
    layer_set_hidden(text_layer_get_layer(layer_wday_text), hide_date);
    layer_set_hidden(layer_line, hide_date);
}

void simplebig_update_time(struct tm *tick_time) {
    // Need to be static because they're used by the system later.
    static char date_text[] = "Xxxxxxxxx 00";
    static char wday_text[] = "Xxxxxxxxx";
    
    // Only update the date when it's changed.
    int new_cur_day = tick_time->tm_year*1000 + tick_time->tm_yday;
    if (new_cur_day != cur_day) {
        cur_day = new_cur_day;
        
        strftime(date_text, sizeof(date_text), "%B %e", tick_time);
        text_layer_set_text(layer_date_text, date_text);

        strftime(wday_text, sizeof(wday_text), "%A", tick_time);
        text_layer_set_text(layer_wday_text, wday_text);
    }

    display_time_value(get_display_hour(tick_time->tm_hour), 0, clock_is_24h_style());
    display_time_value(tick_time->tm_min, 1, true);
}

void simplebig_set_style(bool inverse) {
    foreground_color  = inverse ? GColorBlack : GColorWhite;
    compositing_mode  = inverse ? GCompOpAssign : GCompOpAssignInverted;
    
    text_layer_set_text_color(layer_wday_text, foreground_color);
    text_layer_set_text_color(layer_date_text, foreground_color);
    bitmap_layer_set_compositing_mode(layer_sep_img, compositing_mode);
    for (int i = 0; i < 4; i++) {
        bitmap_layer_set_compositing_mode(digit_layers[i], compositing_mode);
    }
}

void simplebig_init(Window* window) {
    main_window = window;
    main_window_layer = window_get_root_layer(window);
    foreground_color = GColorBlack;

    // Get the total available screen real-estate
    GRect bounds = layer_get_bounds(main_window_layer);

    int padding = (bounds.size.w - 144) / 2;

    // resources
    img_dig_separator  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NUM_SEP);

    // layers
    layer_wday_text = text_layer_create(GRect(20, 35, bounds.size.w-40, 58-35));
    layer_date_text = text_layer_create(GRect(5, 58, bounds.size.w-10, 84-58));

    text_layer_set_text_alignment(layer_wday_text, GTextAlignmentCenter);
    text_layer_set_text_alignment(layer_date_text, GTextAlignmentCenter);

    layer_sep_img   = bitmap_layer_create(GRect(
        padding + DIGIT_IMAGE_WIDTH*2,
        bounds.size.h - DIGIT_IMAGE_HEIGHT,
        8,
        DIGIT_IMAGE_HEIGHT
    ));
    layer_line      = layer_create(GRect(
        padding + 8,
        bounds.size.h - DIGIT_IMAGE_HEIGHT,
        128,
        2
    ));

    // time layers
    for (int i = 0; i < 4; i++) {
        digit_layers[i] = bitmap_layer_create(GRect(
            padding + (i % 2) * DIGIT_IMAGE_WIDTH + (i / 2) * (144-DIGIT_IMAGE_WIDTH*2),
            bounds.size.h - DIGIT_IMAGE_HEIGHT,
            DIGIT_IMAGE_WIDTH,
            DIGIT_IMAGE_HEIGHT
        ));
    }

    text_layer_set_background_color(layer_wday_text, GColorClear);
    text_layer_set_font(layer_wday_text, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));

    text_layer_set_background_color(layer_date_text, GColorClear);
    text_layer_set_font(layer_date_text, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));

    bitmap_layer_set_bitmap(layer_sep_img,  img_dig_separator);

    layer_set_update_proc(layer_line, line_layer_update_callback);

    // composing layers
    Layer *window_layer = window_get_root_layer(window);

    layer_add_child(window_layer, text_layer_get_layer(layer_wday_text));
    layer_add_child(window_layer, text_layer_get_layer(layer_date_text));
    for (int i = 0; i < 4; i++) {
        layer_add_child(window_layer, bitmap_layer_get_layer(digit_layers[i]));
    }
    layer_add_child(window_layer, bitmap_layer_get_layer(layer_sep_img));
    layer_add_child(window_layer, layer_line);

    for (int i = 0; i < 4; i++) {
        load_digit_image_into_slot(i, 0);
    }
}

void simplebig_deinit(void) {
    text_layer_destroy(layer_wday_text);
    text_layer_destroy(layer_date_text);
    bitmap_layer_destroy(layer_sep_img);
    gbitmap_destroy(img_dig_separator);
    layer_destroy(layer_line);
        
    for (int i = 0; i < 4; i++) {
        bitmap_layer_destroy(digit_layers[i]);
        if (digit_images[i]) {
            gbitmap_destroy(digit_images[i]);
        }
    }
}

