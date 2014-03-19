// Standard includes
#include "pebble.h"

//bluetooth icon = IMAGE_BT_ICON

// App-specific data
Window *window; // All apps must have at least one window
TextLayer *time_layer; 
TextLayer *battery_layer;
static BitmapLayer *connection_layer;
static GBitmap *bt_image;
TextLayer *date_layer;
TextLayer *date_layer_right;
TextLayer *line_layer1;
TextLayer *line_layer2;
TextLayer *city_layer;

//Keys for message transfer 
#define KEY_STATUS 0
#define KEY_FETCH 1
#define KEY_TEMP 2


////// App Message Things //////////////////////////////////////////////////////////
//// Receiving //////
static void in_received_handler(DictionaryIterator *iter, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Received handler called");
  Tuple *condition_tuple = dict_find(iter, 0);
	
  if (condition_tuple) { //we received a "status"	  
		APP_LOG(APP_LOG_LEVEL_DEBUG, "status text: %s", condition_tuple->value->cstring);
		text_layer_set_text(city_layer, condition_tuple->value->cstring);
  } 
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  text_layer_set_text(city_layer, "Msg dropped...");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped!");
	
	//APP_LOG(APP_LOG_LEVEL_DEBUG, reason);
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  text_layer_set_text(city_layer, "Msg failed to send.");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send!");
}

static void app_message_init(void) {
  // Register message handlers
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_failed(out_failed_handler);
  // Init buffers
  app_message_open(64, 64);
}
//// Sending ////////////////////
static void fetch_weather() {
  Tuplet m_tuple = TupletCString(KEY_FETCH, "here");

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "iter was null, why?");
    return;
  }

  dict_write_tuplet(iter, &m_tuple);
  dict_write_end(iter);

  app_message_outbox_send();
  text_layer_set_text(city_layer, "App sent request.");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App sent request to JS!");
}

////////////////////////////////////////////////////////////////////////////////////

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100% charged";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "charging");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(battery_layer, battery_text);
}

// Called once per second
static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
  static char time_text[] = "00:00"; // Needs to be static because it's used by the system later.
  static char date_text[] = "Mon"; //don't know if this will work
  static char date_text2[] = "01-01"; //don't know if this will work 
  strftime(time_text, sizeof(time_text), "%H:%M", tick_time);
  strftime(date_text, sizeof(date_text), "%a", tick_time);
  strftime(date_text2, sizeof(date_text2), "%m-%e", tick_time);
	
  //Every 30 min check weather
	if(tick_time->tm_min % 30 == 0)
		if(tick_time->tm_sec == 0)
			fetch_weather();
	
  text_layer_set_text(time_layer, time_text);
  text_layer_set_text(date_layer, date_text);
  text_layer_set_text(date_layer_right, date_text2);
  //fetch_weather();
  handle_battery(battery_state_service_peek());
}

//called based on bluetooth connection
static void handle_bluetooth(bool connected) {
	if(connected != true){
        layer_remove_from_parent(bitmap_layer_get_layer(connection_layer));
	}else{
        layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(connection_layer));
	}
}

// Handle the start-up of the app
static void do_init(void) {

  // Create our app's base window
  window = window_create();
  window_stack_push(window, true);
  window_set_background_color(window, GColorBlack);
	
  //establish the app_message
  app_message_init();

  Layer *root_layer = window_get_root_layer(window);
  GRect frame = layer_get_frame(root_layer);

  // Init the text layer used to show the time
  time_layer = text_layer_create(GRect(0, 50, frame.size.w /* width */,50/* height */));
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_background_color(time_layer, GColorClear);
  //text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));
  text_layer_set_font(time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_AVENIR_MEDIUM_ITALIC_48)));
	text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
	
  date_layer = text_layer_create(GRect(2,0, frame.size.w, 26));
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(date_layer, GTextAlignmentLeft);

  date_layer_right = text_layer_create(GRect(0,0,frame.size.w, 26));
  text_layer_set_text_color(date_layer_right, GColorWhite);
  text_layer_set_background_color(date_layer_right, GColorClear);
  text_layer_set_font(date_layer_right, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(date_layer_right, GTextAlignmentRight);
  
  //all the layer is set up in the function.
  bt_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);	
  //the bitmap layer holds the image for display
  connection_layer = bitmap_layer_create(GRect(0,142,frame.size.w,34));
  bitmap_layer_set_bitmap(connection_layer, bt_image);
  bitmap_layer_set_alignment(connection_layer, GAlignLeft);
  //we are done with the image... I think
  handle_bluetooth(bluetooth_connection_service_peek());

  battery_layer = text_layer_create(GRect(0, 146, /* width */ frame.size.w, 34 /* height */));
  text_layer_set_text_color(battery_layer, GColorWhite);
  text_layer_set_background_color(battery_layer, GColorClear);
  text_layer_set_font(battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(battery_layer, GTextAlignmentRight);
  text_layer_set_text(battery_layer, "100%");
	
  line_layer1 = text_layer_create(GRect(0,28,frame.size.w, 2));
  text_layer_set_background_color(line_layer1, GColorWhite);
  line_layer2 = text_layer_create(GRect(0,126,frame.size.w, 2));
  text_layer_set_background_color(line_layer2, GColorWhite);

	city_layer = text_layer_create(GRect(0,127,frame.size.w,21));
	text_layer_set_background_color(city_layer,GColorBlack);
	text_layer_set_text_color(city_layer, GColorWhite);
	text_layer_set_font(city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(city_layer, GTextAlignmentCenter);	
	//text_layer_set_text(city_layer, "init");
	//CALL THIS AS A TEST
	fetch_weather();
 
  // Ensures time is displayed immediately (will break if NULL tick event accessed).
  // (This is why it's a good idea to have a separate routine to do the update itself.)
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  handle_second_tick(current_time, SECOND_UNIT);

  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
  battery_state_service_subscribe(&handle_battery);
  bluetooth_connection_service_subscribe(&handle_bluetooth);

  layer_add_child(root_layer, text_layer_get_layer(time_layer));
  layer_add_child(root_layer, bitmap_layer_get_layer(connection_layer));
  layer_add_child(root_layer, text_layer_get_layer(battery_layer));
  layer_add_child(root_layer, text_layer_get_layer(date_layer));
  layer_add_child(root_layer, text_layer_get_layer(date_layer_right));
	layer_add_child(root_layer, text_layer_get_layer(city_layer));
	layer_add_child(root_layer, text_layer_get_layer(line_layer1));
	layer_add_child(root_layer, text_layer_get_layer(line_layer2));
}

static void do_deinit(void) {
	//unsubscribe from the services 
  	tick_timer_service_unsubscribe();
  	battery_state_service_unsubscribe();
  	bluetooth_connection_service_unsubscribe();
	
	//destroy the layers
  	text_layer_destroy(time_layer);
  	bitmap_layer_destroy(connection_layer);
  	gbitmap_destroy(bt_image);	
	text_layer_destroy(battery_layer);
 	text_layer_destroy(date_layer);
	text_layer_destroy(date_layer_right);
	text_layer_destroy(city_layer);
	text_layer_destroy(line_layer1);
	text_layer_destroy(line_layer2);
	
  	window_destroy(window);
}

// The main event/run loop for our app
int main(void) {
  do_init();
  app_event_loop();
  do_deinit();
}
