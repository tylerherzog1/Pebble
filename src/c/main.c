#include <pebble.h>

#define font_color GColorWhite
#define bg_color GColorBlack
#define line_color GColorWhite

static Window *s_main_window;
static TextLayer *s_hour_layer, *s_minute_layer, *s_date_layer;
static TextLayer *s_step_layer, *s_hr_layer, *s_battery_layer;
static TextLayer *s_top_line, *s_bottom_line, *s_left_line, *s_right_line;
static BitmapLayer *s_bt_layer, *s_heart_layer, *s_shoe_layer;
static GBitmap *s_bt_bitmap, *s_heart_bitmap, *s_shoe_bitmap;
static GFont s_font_time, s_font_stats, s_font_date;

static void update_time(){
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
	
	static char hour[3];
	strftime(hour, sizeof(hour), clock_is_24h_style() ? "%H" : "%I", tick_time);
	text_layer_set_text(s_hour_layer, hour);

	static char minute[3];
	strftime(minute, sizeof(minute), "%M", tick_time);
	text_layer_set_text(s_minute_layer, minute);
	
	static char date[18];
	strftime(date, sizeof(date), "%A %b %d", tick_time);
	text_layer_set_text(s_date_layer, date);
	
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
	update_time();
}

static void handle_battery(BatteryChargeState charge_state){
	static char battery[7];
	snprintf(battery, sizeof(battery), "%d", charge_state.charge_percent);
	strcat(battery, "%");
	text_layer_set_text(s_battery_layer, battery);
}

static void update_step_layer(){
	//int stepcount = 4824;
	int stepcount = (int)health_service_sum_today(HealthMetricStepCount);
	static char steps[6];
	snprintf(steps, sizeof(steps), "%d", stepcount);
	text_layer_set_text(s_step_layer, steps);
}

static void update_hr_layer(){
	HealthServiceAccessibilityMask hr = health_service_metric_accessible(HealthMetricHeartRateBPM, time(NULL), time(NULL));
	if (hr & HealthServiceAccessibilityMaskAvailable) {
  		HealthValue val = health_service_peek_current_value(HealthMetricHeartRateBPM);
		APP_LOG(APP_LOG_LEVEL_INFO, "HR value: %d", (int)val);
  		if(val > 0) {
    		// Display HRM value
    		static char s_hr_buffer[8];
    		snprintf(s_hr_buffer, sizeof(s_hr_buffer), "%lu", (uint32_t)val);
    		text_layer_set_text(s_hr_layer, s_hr_buffer);
  		}
		else {
			text_layer_set_text(s_hr_layer, "--");
		}
	}
	else{
		text_layer_set_text(s_hr_layer, "NA");
	}
}

static void handle_health(HealthEventType event, void *context){
	switch(event){
		case HealthEventSignificantUpdate:
			update_step_layer();
			update_hr_layer();
		case HealthEventMetricAlert:
		case HealthEventSleepUpdate:
        	break;
    	case HealthEventMovementUpdate: 
			update_step_layer();
        	break;
    	case HealthEventHeartRateUpdate:
			update_hr_layer();
        	break;
	}
}

static void handle_bluetooth(bool connected){
	layer_set_hidden(bitmap_layer_get_layer(s_bt_layer), !connected);
}

static void main_window_load(Window *window){
	Layer *window_layer = window_get_root_layer(window);
	GRect windowbounds = layer_get_bounds(window_layer);
	
	//establish fonts 
    s_font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_VARELA_ROUND_54));
	s_font_stats = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_EXO_THIN_12));
	s_font_date = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_EXO_REGULAR_14));
	
	//Date Layer 
	s_date_layer = text_layer_create(GRect(0,2,windowbounds.size.w, 32));
	text_layer_set_background_color(s_date_layer, bg_color);
	text_layer_set_text_color(s_date_layer, font_color);
	text_layer_set_text(s_date_layer, "");
	text_layer_set_font(s_date_layer, s_font_date);
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
	
	//Hour 
	s_hour_layer = text_layer_create(GRect(0,18,windowbounds.size.w, 54));
	text_layer_set_background_color(s_hour_layer, bg_color);
	text_layer_set_text_color(s_hour_layer, font_color);
	text_layer_set_text(s_hour_layer, "00");
	text_layer_set_font(s_hour_layer, s_font_time);
	text_layer_set_text_alignment(s_hour_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(s_hour_layer));
	
	//Minute
	s_minute_layer = text_layer_create(GRect(0,72,windowbounds.size.w, 54));
	text_layer_set_background_color(s_minute_layer, bg_color);
	text_layer_set_text_color(s_minute_layer, font_color);
	text_layer_set_text(s_minute_layer, "00");
	text_layer_set_font(s_minute_layer, s_font_time);
	text_layer_set_text_alignment(s_minute_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(s_minute_layer));
	
    //Steps
	s_step_layer = text_layer_create(GRect(49,152,48, 28));
	text_layer_set_background_color(s_step_layer, bg_color);
	text_layer_set_text_color(s_step_layer, font_color);
	text_layer_set_text(s_step_layer, "");
	text_layer_set_font(s_step_layer, s_font_stats);
	text_layer_set_text_alignment(s_step_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(s_step_layer));
	
	//Shoe
	s_shoe_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SHOE);
	s_shoe_layer = bitmap_layer_create(GRect(49, 137, 48, 16));
	bitmap_layer_set_background_color(s_shoe_layer, bg_color);
	bitmap_layer_set_compositing_mode(s_shoe_layer, GCompOpSet);
	bitmap_layer_set_bitmap(s_shoe_layer, s_shoe_bitmap);
	layer_add_child(window_layer, bitmap_layer_get_layer(s_shoe_layer));
	
	//HR
	s_hr_layer = text_layer_create(GRect(0,145,44, 28));
	text_layer_set_background_color(s_hr_layer, bg_color);
	text_layer_set_text_color(s_hr_layer, font_color);
	text_layer_set_text(s_hr_layer, "~");
	text_layer_set_font(s_hr_layer, s_font_stats);
	text_layer_set_text_alignment(s_hr_layer, GTextAlignmentRight);
	layer_add_child(window_layer, text_layer_get_layer(s_hr_layer));
	
	//Heart
	s_heart_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEART);
	s_heart_layer = bitmap_layer_create(GRect(2, 142, 23, 20));
	bitmap_layer_set_background_color(s_heart_layer, bg_color);
	bitmap_layer_set_compositing_mode(s_heart_layer, GCompOpSet);
	bitmap_layer_set_bitmap(s_heart_layer, s_heart_bitmap);
	layer_add_child(window_layer, bitmap_layer_get_layer(s_heart_layer));
	
	//Battery
	s_battery_layer = text_layer_create(GRect(100,137,46,16));
	text_layer_set_background_color(s_battery_layer, bg_color);
	text_layer_set_text_color(s_battery_layer, font_color);
	text_layer_set_text(s_battery_layer, "100%");
	text_layer_set_font(s_battery_layer, s_font_stats);
	text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));	
	
	//Bluetooth
	s_bt_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);
	s_bt_layer = bitmap_layer_create(GRect(99, 152, 46, 16));
	bitmap_layer_set_background_color(s_bt_layer, bg_color);
	bitmap_layer_set_compositing_mode(s_bt_layer, GCompOpSet);
	bitmap_layer_set_bitmap(s_bt_layer, s_bt_bitmap);
	layer_add_child(window_layer, bitmap_layer_get_layer(s_bt_layer));
	
	//Lines
	s_top_line = text_layer_create(GRect(13,24,120,1));
	text_layer_set_background_color(s_top_line, line_color);
	layer_add_child(window_layer, text_layer_get_layer(s_top_line));
	
	s_bottom_line = text_layer_create(GRect(13,135,120,1));
	text_layer_set_background_color(s_bottom_line, line_color);
	layer_add_child(window_layer, text_layer_get_layer(s_bottom_line));
	
	s_left_line = text_layer_create(GRect(47,137, 1, 32));
	text_layer_set_background_color(s_left_line, line_color);
	layer_add_child(window_layer, text_layer_get_layer(s_left_line));
	
	s_right_line = text_layer_create(GRect(97, 137, 1, 32));
	text_layer_set_background_color(s_right_line, line_color);
	layer_add_child(window_layer, text_layer_get_layer(s_right_line));
}

static void main_window_unload(Window *window){
	text_layer_destroy(s_hour_layer);
	text_layer_destroy(s_minute_layer);
	text_layer_destroy(s_date_layer);
	text_layer_destroy(s_step_layer);
	text_layer_destroy(s_hr_layer);
	text_layer_destroy(s_battery_layer);
	text_layer_destroy(s_top_line);
	text_layer_destroy(s_bottom_line);
	text_layer_destroy(s_left_line);
	text_layer_destroy(s_right_line);
	gbitmap_destroy(s_bt_bitmap);
	gbitmap_destroy(s_heart_bitmap);
	gbitmap_destroy(s_shoe_bitmap);
	bitmap_layer_destroy(s_bt_layer);
	bitmap_layer_destroy(s_heart_layer);
	bitmap_layer_destroy(s_shoe_layer);
}

static void init(){
	s_main_window = window_create();
	window_set_background_color(s_main_window, bg_color);
	
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load, 
		.unload = main_window_unload
	});
	
	window_stack_push(s_main_window, true);
	
	tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
	update_time();
	
	battery_state_service_subscribe(handle_battery);
	connection_service_subscribe((ConnectionHandlers){
		.pebble_app_connection_handler = handle_bluetooth
	});
	health_service_events_subscribe(handle_health, NULL);
	update_step_layer();
	update_hr_layer();
	
}

static void deinit(){
	window_destroy(s_main_window);
	battery_state_service_unsubscribe();
	connection_service_unsubscribe();
	health_service_events_unsubscribe();
}

int main(void){
	init();
	app_event_loop();
	deinit();
}