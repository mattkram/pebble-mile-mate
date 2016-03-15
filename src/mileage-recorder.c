#include <pebble.h>
#include "helper_functions.h"
#include "windows/pin_window.h"

#define NUM_MENU_ICONS 4

enum {
    KEY_ODOMETER = 0,
    KEY_PRICE,
    KEY_QUANTITY
};

static Window *s_main_window;
static MenuLayer *s_menu_layer;

static int s_odometer_value;
static int s_price_value;
static int s_quantity_value;

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {

}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send success!");
}

static void submit_http_request() {
  
  DictionaryIterator *iter;

  app_message_outbox_begin(&iter);

  dict_write_uint32(iter, KEY_ODOMETER, s_odometer_value);
  dict_write_uint32(iter, KEY_PRICE, s_price_value);
  dict_write_uint32(iter, KEY_QUANTITY, s_quantity_value);

  app_message_outbox_send();
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return NUM_MENU_ICONS;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  // Use the row to specify which item we'll draw
  switch (cell_index->row) {

    // String buffer for each button's value
    char s_buffer[12];
    
    case 0:
      // Odometer
      snprintf(s_buffer, sizeof(s_buffer), (s_odometer_value > 0 ? "%d mi" : "mi"), 
          s_odometer_value);
      menu_cell_basic_draw(ctx, cell_layer, "Odometer", s_buffer, NULL);
      break;
    case 1:
      // Price
      snprintf(s_buffer, sizeof(s_buffer), (s_price_value > 0 ? "$%d.%03d/gal" : "$/gal"),
           s_price_value / pow10(3), s_price_value % pow10(3));
      menu_cell_basic_draw(ctx, cell_layer, "Price", s_buffer, NULL);
      break;
    case 2:
      // Quantity
      snprintf(s_buffer, sizeof(s_buffer), (s_quantity_value > 0 ? "%d.%03d gal" : "gal"),
          s_quantity_value / pow10(3), s_quantity_value % pow10(3));
      menu_cell_basic_draw(ctx, cell_layer, "Quantity", s_buffer, NULL);
      break;
    case 3:
      // Submit
      menu_cell_basic_draw(ctx, cell_layer, "Submit", NULL, NULL);
      break;
  }
}

static void odometer_complete_callback(PIN *pin, void *context) {
  s_odometer_value = pin_digits_to_int(pin);
  APP_LOG(APP_LOG_LEVEL_INFO, "Odometer entry was was %d", s_odometer_value);
  pin_window_pop((PinWindow*)context, true);
}

static void price_complete_callback(PIN *pin, void *context) {
  s_price_value = pin_digits_to_int(pin);

  APP_LOG(APP_LOG_LEVEL_INFO, "Price entry was was %d.%03d", 
    s_price_value / pow10(3), s_price_value % pow10(3));

  pin_window_pop((PinWindow*)context, true);
}

static void quantity_complete_callback(PIN *pin, void *context) {
  s_quantity_value = pin_digits_to_int(pin);

  APP_LOG(APP_LOG_LEVEL_INFO, "Quantity entry was was %d.%03d", 
    s_quantity_value / pow10(3), s_quantity_value % pow10(3));

  pin_window_pop((PinWindow*)context, true);
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  // Use the row to specify which item will receive the select action
  switch (cell_index->row) {
    // Create behavior for clicking each menu item
    case 0: {
      PinWindow *pin_window = pin_window_create(5, 0, s_odometer_value, (PinWindowCallbacks) {
          .pin_complete = odometer_complete_callback
        });
        pin_window_set_title(pin_window, "Odometer Reading");
        pin_window_push(pin_window, true);
      }
      break;
    case 1: {
      PinWindow *pin_window = pin_window_create(4, 3, s_price_value, (PinWindowCallbacks) {
          .pin_complete = price_complete_callback
        });
        pin_window_set_title(pin_window, "Fuel Price");
        pin_window_push(pin_window, true);
      }
      break;
    case 2: {
      PinWindow *pin_window = pin_window_create(5, 3, s_quantity_value, (PinWindowCallbacks) {
          .pin_complete = quantity_complete_callback
        });
        pin_window_set_title(pin_window, "Quantity");
        pin_window_push(pin_window, true);
      }
      break;
    case 3:
      submit_http_request();
  }
}

#ifdef PBL_ROUND
static int16_t get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  if (menu_layer_is_index_selected(menu_layer, cell_index)) {
    switch (cell_index->row) {
      case 0:
        return MENU_CELL_ROUND_FOCUSED_SHORT_CELL_HEIGHT;
        break;
      default:
        return MENU_CELL_ROUND_FOCUSED_TALL_CELL_HEIGHT;
    }
  } else {
    return MENU_CELL_ROUND_UNFOCUSED_SHORT_CELL_HEIGHT;
  }
}
#endif

static void main_window_load() {
  // Prepare to initialize the menu layer
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the menu layer
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = menu_get_num_rows_callback,
    .draw_row = menu_draw_row_callback,
    .get_cell_height = PBL_IF_ROUND_ELSE(get_cell_height_callback, NULL),
    .select_click = menu_select_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_menu_layer, s_main_window);

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void main_window_unload() {
  menu_layer_destroy(s_menu_layer);
}

static void init() {
  // Create main window and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });

  // Push Window to stack, with animated=true
  window_stack_push(s_main_window, true);
    
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_main_window);

  app_event_loop();
  deinit();
}
