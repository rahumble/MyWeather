#include "pebble.h"

static Window *s_main_window;

static TextLayer *s_temperature_layer;
static TextLayer *s_city_layer;
static BitmapLayer *s_icon_layer;
static GBitmap *s_icon_bitmap = NULL;

static TextLayer *s_low_layer;
static TextLayer *s_high_layer;
static TextLayer *s_hum_layer;
static TextLayer *s_wind_layer;
static TextLayer *s_sunrise_layer;
static TextLayer *s_sunset_layer;
static TextLayer *s_low_label;
static TextLayer *s_high_label;
static TextLayer *s_hum_label;
static TextLayer *s_wind_label;
static TextLayer *s_sunrise_label;
static TextLayer *s_sunset_label;
static char *detailInfo[] = {NULL, NULL, NULL, NULL};

static uint8_t numForecasts = 4;
static TextLayer *s_time_layers[4];
static TextLayer *s_temp_layers[4];
static BitmapLayer *s_icon_layers[4];
static GBitmap *s_icon_bitmaps[] = {NULL, NULL, NULL, NULL};
static char **timeInfo;
static char **tempInfo;
static char **iconInfo;

uint8_t activeWin;

enum WeatherKey {
  WEATHER_ICON_KEY = 0,         // TUPLE_INT
  WEATHER_TEMPERATURE_KEY = 1,  // TUPLE_CSTRING
  WEATHER_CITY_KEY = 2,         // TUPLE_CSTRING
  WEATHER_HUM_KEY = 3,          // TUPLE_CSTRING
  WEATHER_WIND_KEY = 4,         // TUPLE_CSTRING
  WEATHER_SUNRISE_KEY = 5,      // TUPLE_CSTRING
  WEATHER_SUNSET_KEY = 6,       // TUPLE_CSTRING
  WEATHER_TIMES_KEY = 7,        // TUPLE_BYTES
  WEATHER_TEMPS_KEY = 8,        // TUPLE_BYTES
  WEATHER_ICONS_KEY = 9,        // TUPLE_BYTES
  WEATHER_HIGH_KEY = 10,        // TUPLE_INT
  WEATHER_LOW_KEY = 11,        // TUPLE_INT
  WEATHER_REQUEST_KEY = 12      // TUPLE_INT
};

static const uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_IMAGE_SUN, // 0
  RESOURCE_ID_IMAGE_CLOUD, // 1
  RESOURCE_ID_IMAGE_RAIN, // 2
  RESOURCE_ID_IMAGE_SNOW, // 3
  RESOURCE_ID_IMAGE_SUN_SMALL, // 4
  RESOURCE_ID_IMAGE_CLOUD_SMALL, // 5
  RESOURCE_ID_IMAGE_RAIN_SMALL, // 6
  RESOURCE_ID_IMAGE_SNOW_SMALL // 7
};

static void printMemoryUsage() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Used: %u. Free: %u.", heap_bytes_used(), heap_bytes_free());
}

static char *strCopy(char *dest, char *str) {
  if(dest) {
    free(dest);
  }
  dest = (char*) malloc((strlen(str) + 1)*sizeof(char));
  strcpy(dest, str);
  dest[strlen(str)] = '\0';
  return dest;
}

static void request_weather() {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (!iter) {
    // Error creating outbound message
    return;
  }

  int value = 1;
  if(activeWin) {
    value = 0;
  }
  dict_write_int(iter, 12, &value, sizeof(int), true);
  dict_write_end(iter);

  app_message_outbox_send();
}

static char **convertByteArray(char *info, int width) {
  char **array = (char**) malloc(numForecasts*sizeof(char*));

  for(int i = 0; i < numForecasts; i ++) {
    char *tmp = (char*) malloc((width + 1)*sizeof(char));
    strncpy(tmp, info + i*width, width);
    tmp[width] = '\0';
    array[i] = tmp;
  }
  return array;
}

static void digestTemps(char *byteArray) {
  if(tempInfo) {
    for(int i = 0; i < numForecasts; i++) {
      free(tempInfo[i]);
    }
    free(tempInfo);
  }

  tempInfo = convertByteArray(byteArray, 6);
  for(int i = 0; i < numForecasts; i++) {
    text_layer_set_text(s_temp_layers[i], tempInfo[i]);
  }
}

static void digestTimes(char *byteArray) {
  if(timeInfo) {
    for(int i = 0; i < numForecasts; i++) {
      free(timeInfo[i]);
    }
    free(timeInfo);
  }

  timeInfo = convertByteArray(byteArray, 5);
  for(int i = 0; i < numForecasts; i++) {
    text_layer_set_text(s_time_layers[i], timeInfo[i]);
  }
}

static void digestIcons(char *byteArray) {
  iconInfo = convertByteArray(byteArray, 2);
  for(int i = 0; i < numForecasts; i++) {
    if(s_icon_bitmaps[i]) {
      gbitmap_destroy(s_icon_bitmaps[i]);
    }
    int iconNum = 4 + atoi(iconInfo[i]);
    s_icon_bitmaps[i] = gbitmap_create_with_resource(WEATHER_ICONS[iconNum]);
  
    #ifdef PBL_SDK_3
    bitmap_layer_set_compositing_mode(s_icon_layers[i], GCompOpSet);  
    #endif

    bitmap_layer_set_bitmap(s_icon_layers[i], s_icon_bitmaps[i]);
    free(iconInfo[i]);
  }
  free(iconInfo);
}

static void addLayers(Layer *window_layer) {
  if(activeWin == 0) {
    layer_add_child(window_layer, bitmap_layer_get_layer(s_icon_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_temperature_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_city_layer));
  }
  else if(activeWin == 1) {
    layer_add_child(window_layer, text_layer_get_layer(s_low_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_high_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_hum_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_wind_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_sunrise_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_sunset_layer));

    layer_add_child(window_layer, text_layer_get_layer(s_low_label));
    layer_add_child(window_layer, text_layer_get_layer(s_high_label));
    layer_add_child(window_layer, text_layer_get_layer(s_hum_label));
    layer_add_child(window_layer, text_layer_get_layer(s_wind_label));
    layer_add_child(window_layer, text_layer_get_layer(s_sunrise_label));
    layer_add_child(window_layer, text_layer_get_layer(s_sunset_label));
  }
  else {
    for(int i = 0; i < numForecasts; i++) {
      layer_add_child(window_layer, text_layer_get_layer(s_temp_layers[i]));
      layer_add_child(window_layer, text_layer_get_layer(s_time_layers[i]));
      layer_add_child(window_layer, bitmap_layer_get_layer(s_icon_layers[i]));
    }
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *in_tuple = dict_read_first(iterator);

  printf("DictIter Size: %d", (int) dict_size(iterator));

  while(in_tuple != NULL) {

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Key: %d", (int) in_tuple->key);

    switch(in_tuple->key) {
      case WEATHER_ICON_KEY:
        if (s_icon_bitmap) {
          gbitmap_destroy(s_icon_bitmap);
        }

        s_icon_bitmap = gbitmap_create_with_resource(WEATHER_ICONS[in_tuple->value->uint8]);

        #ifdef PBL_SDK_3
        bitmap_layer_set_compositing_mode(s_icon_layer, GCompOpSet);  
        #endif

        bitmap_layer_set_bitmap(s_icon_layer, s_icon_bitmap);
        break;

      case WEATHER_TEMPERATURE_KEY:
        text_layer_set_text(s_temperature_layer, in_tuple->value->cstring);
        break;

      case WEATHER_CITY_KEY:
        text_layer_set_text(s_city_layer, in_tuple->value->cstring);
        break;

      case WEATHER_HUM_KEY:
        detailInfo[0] = strCopy(detailInfo[0], in_tuple->value->cstring);
        text_layer_set_text(s_hum_layer, detailInfo[0]);
        break;

      case WEATHER_WIND_KEY:
        detailInfo[1] = strCopy(detailInfo[1], in_tuple->value->cstring);  
        text_layer_set_text(s_wind_layer, detailInfo[1]);
        break;

      case WEATHER_SUNRISE_KEY:
        detailInfo[2] = strCopy(detailInfo[2], in_tuple->value->cstring);
        text_layer_set_text(s_sunrise_layer, detailInfo[2]);
        break;

      case WEATHER_SUNSET_KEY:
        detailInfo[3] = strCopy(detailInfo[3], in_tuple->value->cstring);
        text_layer_set_text(s_sunset_layer, detailInfo[3]);
        break;

      case WEATHER_HIGH_KEY:
        text_layer_set_text(s_high_layer, in_tuple->value->cstring);
        break;

      case WEATHER_LOW_KEY:
        text_layer_set_text(s_low_layer, in_tuple->value->cstring);
        break;

      case WEATHER_TIMES_KEY:
        digestTimes((char*) in_tuple->value);
        break;

      case WEATHER_TEMPS_KEY:
        digestTemps((char*) in_tuple->value);
        break;

      case WEATHER_ICONS_KEY:  
        digestIcons((char*) in_tuple->value);
        break;

      default:
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Key %d not recognized.", (int) in_tuple->key);
        break;
    }

    in_tuple = dict_read_next(iterator);
  }
  layer_remove_child_layers(window_get_root_layer(s_main_window));
  addLayers(window_get_root_layer(s_main_window));
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed: %d", reason);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static TextLayer *createTextLayer(int16_t x, int16_t y, int16_t w, int16_t h, char *font, GTextAlignment alignment) {
  TextLayer *layer = text_layer_create(GRect(x, y, w, h));
  text_layer_set_text_color(layer, COLOR_FALLBACK(GColorBlack, GColorWhite));
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_font(layer, fonts_get_system_font(font));
  text_layer_set_text_alignment(layer, alignment);
  return layer;
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received up click.");

  if(activeWin > 0) {
    activeWin -= 1;
  }
  else {
    activeWin = 2;
  }

  request_weather();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received down click.");

  activeWin = (activeWin + 1) % 3;

  request_weather();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received select click.");

  request_weather();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  uint8_t width = bounds.size.w;
  uint8_t height = bounds.size.h;

  s_icon_layer = bitmap_layer_create(GRect(32, 10, 80, 80));
  s_temperature_layer = createTextLayer(0, 90, width, 32, FONT_KEY_GOTHIC_28_BOLD, GTextAlignmentCenter);
  s_city_layer = createTextLayer(0, 122, width, 32, FONT_KEY_GOTHIC_28_BOLD, GTextAlignmentCenter);

  s_low_layer = createTextLayer(0, height/6, width/2, height/3, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft);
  s_high_layer = createTextLayer(0 + width/2, height/6, width/2, height/3, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft);
  s_hum_layer = createTextLayer(0, height/6 + height/3, width/2, height/3, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft);
  s_wind_layer = createTextLayer(0 + width/2, height/6 + height/3, width/2, height/3, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft);
  s_sunrise_layer = createTextLayer(0, height/6 + 2*height/3, width/2, height/3, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft);
  s_sunset_layer = createTextLayer(0+ width/2, height/6 + 2*height/3, width/2, height/3, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft);

  s_low_label = createTextLayer(0, 0, width/2, height/3, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft);
  text_layer_set_text(s_low_label, "Low");
  s_high_label = createTextLayer(width/2, 0, width/2, height/3, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft);
  text_layer_set_text(s_high_label, "High");
  s_hum_label = createTextLayer(0, height/3, width/2, height/3, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft);
  text_layer_set_text(s_hum_label, "Humidity");
  s_wind_label = createTextLayer(width/2, height/3, width/2, height/3, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft);
  text_layer_set_text(s_wind_label, "Wind");
  s_sunrise_label = createTextLayer(0, 2*height/3, width/2, height/3, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft);
  text_layer_set_text(s_sunrise_label, "Sunrise");
  s_sunset_label = createTextLayer(width/2, 2*height/3, width/2, height/3, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft);
  text_layer_set_text(s_sunset_label, "Sunset");\

  for(int i = 0; i < numForecasts; i++) {
    s_icon_layers[i] = bitmap_layer_create(GRect(width/3, i*height/numForecasts, width/3, height/numForecasts));
    s_temp_layers[i] = createTextLayer(2*width/3, i*height/numForecasts, width/3, height/numForecasts, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentRight);
    s_time_layers[i] = createTextLayer(0, i*height/numForecasts, width/3, height/numForecasts, FONT_KEY_GOTHIC_18_BOLD, GTextAlignmentLeft);
  }

  addLayers(window_layer);
}

static void window_unload(Window *window) {
  layer_remove_child_layers(window_get_root_layer(window));
}

static void init(void) {
  s_main_window = window_create();
  window_set_background_color(s_main_window, COLOR_FALLBACK(GColorIndigo, GColorBlack));

  #ifdef PBL_SDK_2
  window_set_fullscreen(s_main_window, true);
  #endif

  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_set_click_config_provider(s_main_window, click_config_provider);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  activeWin = 0;
  window_stack_push(s_main_window, true);
}

static void deinit(void) {
  if (s_icon_bitmap) {
    gbitmap_destroy(s_icon_bitmap);
  }

  text_layer_destroy(s_city_layer);
  text_layer_destroy(s_temperature_layer);
  bitmap_layer_destroy(s_icon_layer);

  text_layer_destroy(s_low_layer);
  text_layer_destroy(s_high_layer);
  text_layer_destroy(s_hum_layer);
  text_layer_destroy(s_wind_layer);
  text_layer_destroy(s_sunrise_layer);
  text_layer_destroy(s_sunset_layer);

  text_layer_destroy(s_low_label);
  text_layer_destroy(s_high_label);
  text_layer_destroy(s_hum_label);
  text_layer_destroy(s_wind_label);
  text_layer_destroy(s_sunrise_label);
  text_layer_destroy(s_sunset_label); 

  for(int i = 0; i < numForecasts; i++) {
    if (s_icon_bitmaps[i]) {
      gbitmap_destroy(s_icon_bitmaps[i]);
    }
    bitmap_layer_destroy(s_icon_layers[i]);
    text_layer_destroy(s_time_layers[i]);
    text_layer_destroy(s_temp_layers[i]);
  } 

  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}