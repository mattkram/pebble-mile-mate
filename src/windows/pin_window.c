#include <pebble.h>
#include "pin_window.h"
#include "../layers/selection_layer.h"

static char* selection_handle_get_text(int index, void *context) {
  PinWindow *pin_window = (PinWindow*)context;
  snprintf(
    pin_window->field_buffs[index], 
    sizeof(pin_window->field_buffs[0]), "%d",
    (int)pin_window->pin->digits[index]
  );
  return pin_window->field_buffs[index];
}

static void selection_handle_complete(void *context) {
  PinWindow *pin_window = (PinWindow*)context;
  pin_window->callbacks.pin_complete(pin_window->pin, pin_window);
}

static void selection_handle_inc(int index, uint8_t clicks, void *context) {
  PinWindow *pin_window = (PinWindow*)context;
  pin_window->pin->digits[index]++;
  if(pin_window->pin->digits[index] > PIN_WINDOW_MAX_VALUE) {
    pin_window->pin->digits[index] = 0;
  }
}

static void selection_handle_dec(int index, uint8_t clicks, void *context) {
  PinWindow *pin_window = (PinWindow*)context;
  pin_window->pin->digits[index]--;
  if(pin_window->pin->digits[index] < 0) {
    pin_window->pin->digits[index] = PIN_WINDOW_MAX_VALUE;
  }
}

int pin_digits_to_int(PIN *pin) {
  int value = 0;

  for (int i=0; i < pin->num_digits; ++i) {
    int digit_value = pin->digits[i];
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Digit: %d", digit_value);
    for (int j=0; j < pin->num_digits - i - 1; ++j) {
      digit_value *= 10;
    }
    value += digit_value;
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Value: %d", value);
  return value;
}

PIN* pin_create(int num_digits, int num_decimals) {
  PIN* pin = (PIN*)malloc(sizeof(PIN));
  pin->num_digits = num_digits;
  pin->num_decimals = num_decimals;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Number of digits: %d", pin->num_digits);
  pin->digits = (int*)malloc(sizeof(int)*num_digits);
  return pin;
}

void pin_destroy(PIN* pin) {
  free(pin->digits);
  pin->digits = NULL;
  free(pin);
  pin = NULL;
}

void pin_window_set_title(PinWindow *pin_window, char *title) {
  text_layer_set_text(pin_window->main_text, title);
}

PinWindow* pin_window_create(int num_digits, int num_decimals, PinWindowCallbacks callbacks) {
  PinWindow *pin_window = (PinWindow*)malloc(sizeof(PinWindow));
  if (pin_window) {
    pin_window->window = window_create();
    pin_window->callbacks = callbacks;
   
    pin_window->pin = pin_create(num_digits, num_decimals);
    pin_window->field_buffs = (char**)malloc(sizeof(char*)*num_digits);
    for (int i = 0; i < num_digits; ++i) {
      pin_window->field_buffs[i] = (char*)malloc(sizeof(char)*2);
    }

    if (pin_window->window) {
      pin_window->field_selection = 0;
      for(int i = 0; i < num_digits; i++) {
        pin_window->pin->digits[i] = 0;
      }
      
      // Get window parameters
      Layer *window_layer = window_get_root_layer(pin_window->window);
      GRect bounds = layer_get_bounds(window_layer);
      
      // Main TextLayer
      const GEdgeInsets main_text_insets = {.top = 30};
      pin_window->main_text = text_layer_create(grect_inset(bounds, main_text_insets));
      pin_window_set_title(pin_window, "");
      text_layer_set_font(pin_window->main_text, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
      text_layer_set_text_alignment(pin_window->main_text, GTextAlignmentCenter);
      layer_add_child(window_layer, text_layer_get_layer(pin_window->main_text));
     
      // Calculate width of selection cells and window
      int pin_window_width = PIN_WINDOW_MAX_WIDTH;
      int digit_width = pin_window_width - (num_digits - 1) * PIN_WINDOW_CELL_PADDING -
                        (num_decimals > 0 ? DECIMAL_CELL_WIDTH : 0);
      digit_width /= num_digits;
      pin_window_width = (digit_width + PIN_WINDOW_CELL_PADDING) * num_digits -
                          PIN_WINDOW_CELL_PADDING + (num_decimals > 0 ? DECIMAL_CELL_WIDTH : 0);

      // Create selection layer
      const GEdgeInsets selection_insets = GEdgeInsets(
        (bounds.size.h - PIN_WINDOW_HEIGHT) / 2, 
        (bounds.size.w - pin_window_width) / 2);
      pin_window->selection = selection_layer_create(grect_inset(bounds, selection_insets), num_digits, num_decimals);
      for (int i = 0; i < num_digits; i++) {
        selection_layer_set_cell_width(pin_window->selection, i, digit_width);
      }
      selection_layer_set_cell_padding(pin_window->selection, PIN_WINDOW_CELL_PADDING);
      selection_layer_set_active_bg_color(pin_window->selection, GColorRed);
      selection_layer_set_inactive_bg_color(pin_window->selection, GColorDarkGray);
      selection_layer_set_click_config_onto_window(pin_window->selection, pin_window->window);
      selection_layer_set_callbacks(pin_window->selection, pin_window, (SelectionLayerCallbacks) {
        .get_cell_text = selection_handle_get_text,
        .complete = selection_handle_complete,
        .increment = selection_handle_inc,
        .decrement = selection_handle_dec,
      });
      layer_add_child(window_get_root_layer(pin_window->window), pin_window->selection);
 
      // Create status bar
      pin_window->status = status_bar_layer_create();
      status_bar_layer_set_colors(pin_window->status, GColorClear, GColorBlack);
      layer_add_child(window_layer, status_bar_layer_get_layer(pin_window->status));
      return pin_window;
    }
  }

  APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create PinWindow");
  return NULL;
}

void pin_window_destroy(PinWindow *pin_window) {
  if (pin_window) {
    // Destroy various laters
    status_bar_layer_destroy(pin_window->status);
    selection_layer_destroy(pin_window->selection);
    text_layer_destroy(pin_window->sub_text);
    text_layer_destroy(pin_window->main_text);
    
    // Destroy pin and its digits
    pin_destroy(pin_window->pin);
    
    // Destroy field buffers
    for (int i = 0; i < pin_window->pin->num_digits; ++i) {
      free(pin_window->field_buffs[i]);
      pin_window->field_buffs[i] = NULL;
    }
    free(pin_window->field_buffs);
    pin_window->field_buffs = NULL;
    
    // Destroy window
    free(pin_window);
    pin_window = NULL;
    return;
  }
}

void pin_window_push(PinWindow *pin_window, bool animated) {
  window_stack_push(pin_window->window, animated);
}

void pin_window_pop(PinWindow *pin_window, bool animated) {
  window_stack_remove(pin_window->window, animated);
}

bool pin_window_get_topmost_window(PinWindow *pin_window) {
  return window_stack_get_top_window() == pin_window->window;
}

void pin_window_set_highlight_color(PinWindow *pin_window, GColor color) {
  pin_window->highlight_color = color;
  selection_layer_set_active_bg_color(pin_window->selection, color);
}
