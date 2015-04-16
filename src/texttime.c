#include <pebble.h>
#include <num2words.h>
  
//#define DEBUG 1
#define NUM_LAYERS 4
#define TIME_SLOT_ANIMATION_DURATION 700

#define DEFAULT_FONT_UPPER FONT_KEY_BITHAM_42_BOLD
#define DEFAULT_FONT_LOWER FONT_KEY_BITHAM_42_LIGHT
//options reference for storage
#define FLASH_OPTIONS 0
//options reference for messages
#define KEY_BACKGROUND 0
#define KEY_ALIGN 1
#define KEY_CAPITAL 2
#define KEY_FONT_STYLE 3
#define KEY_LIMIT 4
  
//define options for fonts
#define KEY_FONT_DEFAULT 0
//total number of system font selections
#define KEY_SYSTEM_FONT_SIZE 1
//custom font keys  
#define KEY_FONT_DANIEL 1
#define KEY_FONT_FINELINER 2
#define KEY_FONT_PHILOSOPHER 3
#define KEY_FONT_RETRO 4
#define KEY_FONT_STAR 5
#define KEY_FONT_STENCILIA 6
#define KEY_FONT_TECHNIQUE 7
#define KEY_FONT_VISITOR 8
#define KEY_FONT_FLORENCE 9
#define KEY_FONT_QUICKSAND 10
#define KEY_FONT_COLLEGE 11
#define KEY_FONT_LCD 12
#define KEY_FONT_ARCHISTICO 13
#define KEY_FONT_EURO 14
//total number of custom fonts
#define KEY_CUSTOM_FONT_SIZE 14

static const uint32_t s_custom_fonts_upper[KEY_CUSTOM_FONT_SIZE] = {RESOURCE_ID_FONT_DANIEL_BOLD_34, RESOURCE_ID_FONT_FINELINER_38, RESOURCE_ID_FONT_PHILO_BOLD_38, RESOURCE_ID_FONT_RETRO_44, RESOURCE_ID_FONT_STAR_BOLD_26, RESOURCE_ID_FONT_STENCILIA_34, RESOURCE_ID_FONT_TECH_BOLD_22, RESOURCE_ID_FONT_VISITOR_30, RESOURCE_ID_FONT_FLO_BOLD_34, RESOURCE_ID_FONT_QUICK_36, RESOURCE_ID_FONT_COLLEGE_BOLD_38, RESOURCE_ID_FONT_LCD_34, RESOURCE_ID_FONT_ARCHISTICO_34, RESOURCE_ID_FONT_EURO_BOLD_44};
static const uint32_t s_custom_fonts_lower[KEY_CUSTOM_FONT_SIZE] = {RESOURCE_ID_FONT_DANIEL_34, RESOURCE_ID_FONT_FINELINER_38, RESOURCE_ID_FONT_PHILO_38, RESOURCE_ID_FONT_RETRO_44, RESOURCE_ID_FONT_STAR_26, RESOURCE_ID_FONT_STENCILIA_34, RESOURCE_ID_FONT_TECH_22, RESOURCE_ID_FONT_VISITOR_30, RESOURCE_ID_FONT_FLO_34, RESOURCE_ID_FONT_QUICK_36, RESOURCE_ID_FONT_COLLEGE_38, RESOURCE_ID_FONT_LCD_34, RESOURCE_ID_FONT_ARCHISTICO_34, RESOURCE_ID_FONT_EURO_44};

//animation handlers
void handle_slide_out_animation_stopped(Animation *slide_out_animation, bool finished, void *context);
void handle_slide_in_animation_stopped(Animation *slide_out_animation, bool finished, void *context);

//structs and enums
enum e_align {
  ALIGN_LEFT = 0, ALIGN_CENTRE, ALIGN_RIGHT
};

enum e_layer_names {
	  MINUTES= 0, TENS, HOURS, DATE
};

typedef enum e_direction {
	OUT = 0, IN
} eDirection;

typedef struct CommonWordsData {
	TextLayer *label;
	PropertyAnimation *prop_animation;
	char buffer[BUFFER_SIZE];
  void (*update)(struct tm *t, char *words);
} CommonWordsData;

//member variables
static Window *s_main_window;
static CommonWordsData *s_layers[4];
static InverterLayer *s_inverter_layer;
static struct tm *s_new_time;
static GFont s_custom_font_upper;
static GFont s_custom_font_lower;
static int s_options [KEY_LIMIT];

/*  Apply the application settings from the options array */
void update_configuration(void)
{
  if(s_options[KEY_BACKGROUND] == 0){
    layer_set_hidden(inverter_layer_get_layer(s_inverter_layer), true);
  }else{
    layer_set_hidden(inverter_layer_get_layer(s_inverter_layer), false);
  }
  
  GTextAlignment align = GTextAlignmentLeft;
  switch(s_options[KEY_ALIGN]){
    case ALIGN_CENTRE:
    align = GTextAlignmentCenter;
    break;
    case ALIGN_RIGHT:
    align = GTextAlignmentRight;
    break;
    case ALIGN_LEFT:
    default:
    break;
  }
  
  if(s_options[KEY_FONT_STYLE] > 0){
    fonts_unload_custom_font(s_custom_font_upper);
    fonts_unload_custom_font(s_custom_font_lower);
    s_custom_font_upper = fonts_load_custom_font(resource_get_handle(s_custom_fonts_upper[(s_options[KEY_FONT_STYLE]-KEY_SYSTEM_FONT_SIZE)]));
    s_custom_font_lower = fonts_load_custom_font(resource_get_handle(s_custom_fonts_lower[(s_options[KEY_FONT_STYLE]-KEY_SYSTEM_FONT_SIZE)]));
    text_layer_set_font(s_layers[HOURS]->label, s_custom_font_upper);
    text_layer_set_font(s_layers[MINUTES]->label, s_custom_font_lower);
    text_layer_set_font(s_layers[TENS]->label, s_custom_font_lower);
    text_layer_set_font(s_layers[DATE]->label, s_custom_font_lower);
  }else{
    text_layer_set_font(s_layers[HOURS]->label, fonts_get_system_font(DEFAULT_FONT_UPPER));
    text_layer_set_font(s_layers[MINUTES]->label, fonts_get_system_font(DEFAULT_FONT_LOWER));
    text_layer_set_font(s_layers[TENS]->label, fonts_get_system_font(DEFAULT_FONT_LOWER));
    text_layer_set_font(s_layers[DATE]->label, fonts_get_system_font(DEFAULT_FONT_LOWER));
  }
  
  for(int j = 0; j < NUM_LAYERS; j++){
    text_layer_set_text_alignment((TextLayer*)text_layer_get_layer(s_layers[j]->label), align);
  }
}

/*  Abstract animation function */
void animate(CommonWordsData *layer, eDirection direction, GRect *from_frame,
		GRect *to_frame) {
	layer->prop_animation = property_animation_create_layer_frame(
			text_layer_get_layer(layer->label), from_frame, to_frame);
	animation_set_duration((Animation*) layer->prop_animation,
			TIME_SLOT_ANIMATION_DURATION);
	if(direction == OUT){
		animation_set_curve((Animation*) layer->prop_animation, AnimationCurveEaseIn);
		animation_set_handlers((Animation*) layer->prop_animation,(AnimationHandlers ) { .stopped = handle_slide_out_animation_stopped },(void *) layer);
	}else{
		animation_set_curve((Animation*) layer->prop_animation, AnimationCurveEaseOut);
		animation_set_handlers((Animation*) layer->prop_animation,(AnimationHandlers ) { .stopped = handle_slide_in_animation_stopped },(void *) layer);
	}
	animation_schedule((Animation*) layer->prop_animation);
}

/* Text Layer initialisation */
void init_layer(Layer *window_layer, CommonWordsData *layer, GRect rect,
		GFont font) {
	layer->label = text_layer_create(rect);
	text_layer_set_background_color(layer->label, GColorClear);
	text_layer_set_text_color(layer->label, GColorWhite);
	text_layer_set_font(layer->label, font);
	layer_add_child(window_layer, text_layer_get_layer(layer->label));
}

/*  Updates the text layer */
void update_layer(CommonWordsData *layer, struct tm *t) {
	GRect from_frame = layer_get_frame(text_layer_get_layer(layer->label));
	GRect frame = layer_get_frame(window_get_root_layer(s_main_window));

	GRect to_frame = GRect(-frame.size.w, from_frame.origin.y, frame.size.w,
			from_frame.size.h);

	// Schedule the out animation
		animate(layer, OUT, NULL, &to_frame);
}

/* Schedule new slide in animation for referenced text layer */
void slide_in(CommonWordsData *layer) {
	TextLayer * text_layer = layer->label;
	GRect origin_frame = layer_get_frame(text_layer_get_layer(text_layer));

	Layer *root_layer = window_get_root_layer(s_main_window);
	GRect frame = layer_get_frame(root_layer);
	GRect to_frame = GRect(0, origin_frame.origin.y, frame.size.w,
			origin_frame.size.h);
	GRect from_frame = GRect(2 * frame.size.w, origin_frame.origin.y,
			frame.size.w, origin_frame.size.h);

	layer_set_frame(text_layer_get_layer(text_layer), from_frame);
	text_layer_set_text(layer->label, layer->buffer);

	// Schedule the next animation
	animate(layer, IN, &from_frame, &to_frame);
}

/* handler run when slide out animation completes */
void handle_slide_out_animation_stopped(Animation *slide_out_animation, bool finished,
		void *context) {
	CommonWordsData *layer = (CommonWordsData *) context;
    property_animation_destroy(layer->prop_animation); 
    //schedule slide in animation
    if(finished){
  		layer->update(s_new_time, layer->buffer);
  		slide_in(layer);
  	}
}

/* handler run when slide in animation completes */
void handle_slide_in_animation_stopped(Animation *slide_out_animation, bool finished,
		void *context) {
	CommonWordsData *layer = (CommonWordsData *) context;
	property_animation_destroy(layer->prop_animation);
}

/* minute tick handler */
static void handle_minute_tick(struct tm *t, TimeUnits units_changed) {
	memcpy(t,s_new_time, sizeof(struct tm));
	if ((units_changed & MINUTE_UNIT) == MINUTE_UNIT) {
		if ((17 > t->tm_min || t->tm_min > 19)
				&& (11 > t->tm_min || t->tm_min > 13)) {
#ifdef DEBUG
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Update layer %u", MINUTES);
#endif
			update_layer(s_layers[MINUTES], t);
		}
		if (t->tm_min % 10 == 0 || (t->tm_min > 10 && t->tm_min < 20)
				|| t->tm_min == 1) {
#ifdef DEBUG
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Update layer %u", TENS);
#endif
			update_layer(s_layers[TENS], t);
		}
	}
	if ((units_changed & HOUR_UNIT) == HOUR_UNIT
			|| ((t->tm_hour == 00 || t->tm_hour == 12) && t->tm_min == 01)) {
#ifdef DEBUG
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Update layer %u", HOURS);
#endif
		update_layer(s_layers[HOURS], t);
	}
	if ((units_changed & DAY_UNIT) == DAY_UNIT) {
#ifdef DEBUG
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Update layer %u", DATE);
#endif
		update_layer(s_layers[DATE], t);
	}
}

/* App message input handler */
void handle_inbox_received(DictionaryIterator *iterator, void *context)
{
  tick_timer_service_unsubscribe();
  animation_unschedule_all();
  
  Tuple *t = dict_read_first(iterator); 
  while(t != NULL) {
    if(t->key < KEY_LIMIT){
#ifdef DEBUG      
      APP_LOG(APP_LOG_LEVEL_DEBUG, "key received: %u - %d", (unsigned int)t->key, (int)t->value->int32);
#endif
      s_options[t->key] = t->value->int32;
    }
    t = dict_read_next(iterator);
  }
  persist_write_data(FLASH_OPTIONS, &s_options, sizeof(s_options));
  fuzzy_set_date_lower(s_options[KEY_CAPITAL]);
  update_configuration();
  
  //refresh the layers
	time_t now = time(NULL);
	s_new_time = localtime(&now);
#ifdef DEBUG      
      s_new_time->tm_hour = 12;
      s_new_time->tm_min = 5;
      s_new_time->tm_sec = 0;
#endif
  
  
  for (int i = 0; i < NUM_LAYERS; ++i) {
		s_layers[i]->update(s_new_time, s_layers[i]->buffer);
		slide_in(s_layers[i]);
    //update_layer(s_layers[i], s_new_time);
	}
  // Register with TickTimerService
	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

/* App message dropped handler */
void handle_inbox_dropped(AppMessageResult reason, void *ctx)
{
    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "Message dropped, reason code %d",
            reason);
}

/* window init */
static void handle_main_window_load(Window *window) {

	window_set_background_color(window, GColorBlack);
	Layer *window_layer = window_get_root_layer(window);

#ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "options size is - %d", sizeof(s_options));
#endif
  
	for (int i = 0; i < NUM_LAYERS; ++i) {
		s_layers[i] = malloc(sizeof(CommonWordsData));
	}
  
  //initialise options
  memset(&s_options, 0, sizeof(s_options));
  if(persist_exists(FLASH_OPTIONS)){
     persist_read_data(FLASH_OPTIONS, &s_options, sizeof(s_options));
#ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Read options %d - %d, %d - %d, %d - %d, %d - %d", KEY_BACKGROUND, s_options[KEY_BACKGROUND], KEY_ALIGN, s_options[KEY_ALIGN], KEY_CAPITAL, s_options[KEY_CAPITAL], KEY_FONT_STYLE, s_options[KEY_FONT_STYLE]);
#endif
  }
  fuzzy_set_date_lower(s_options[KEY_CAPITAL]);
  s_custom_font_upper = fonts_load_custom_font(resource_get_handle(s_custom_fonts_upper[0]));
  s_custom_font_lower = fonts_load_custom_font(resource_get_handle(s_custom_fonts_lower[0]));

	// Update time callbacks
	s_layers[MINUTES]->update = &fuzzy_sminutes_to_words;
	s_layers[TENS]->update = &fuzzy_minutes_to_words;
	s_layers[HOURS]->update = &fuzzy_hours_to_words;
	s_layers[DATE]->update = &fuzzy_dates_to_words;

	// Get the bounds of the window for sizing the text layer
	GRect bounds = layer_get_bounds(window_layer);

	// initialise layers
  //hours
	init_layer(window_layer, s_layers[HOURS], GRect(0, 0, bounds.size.w, 50),
			fonts_get_system_font(DEFAULT_FONT_UPPER));

	// tens of minutes
	init_layer(window_layer, s_layers[TENS], GRect(0, 38, bounds.size.w, 50),
			fonts_get_system_font(DEFAULT_FONT_LOWER));

	// minutes
	init_layer(window_layer, s_layers[MINUTES],
			GRect(0, 76, bounds.size.w, 50),
			fonts_get_system_font(DEFAULT_FONT_LOWER));

	//Date
	init_layer(window_layer, s_layers[DATE], GRect(0, 114, bounds.size.w, 50),
			fonts_get_system_font(DEFAULT_FONT_LOWER));

	s_inverter_layer = inverter_layer_create(GRect(0, 0, 144, 168));
	layer_add_child(window_layer, inverter_layer_get_layer(s_inverter_layer));
	update_configuration();

	//show the layers
	time_t now = time(NULL);
	s_new_time = localtime(&now);

	for (int i = 0; i < NUM_LAYERS; ++i) {
		s_layers[i]->update(s_new_time, s_layers[i]->buffer);
		slide_in(s_layers[i]);
	}
}

/* window deinit */
static void handle_main_window_unload(Window *window) {
  if(s_custom_font_upper != 0){
    fonts_unload_custom_font(s_custom_font_upper);
    s_custom_font_upper = 0;
  }
  if(s_custom_font_lower != 0){
    fonts_unload_custom_font(s_custom_font_lower);
    s_custom_font_lower = 0;
  } 
	for (int i = 0; i < NUM_LAYERS; ++i) {
		text_layer_destroy(s_layers[i]->label);
		free(s_layers[i]);
		s_layers[i] = NULL;
	}
	inverter_layer_destroy(s_inverter_layer);
}

/* app init */
static void init() {
	// Create main window element and assign to pointer
	s_main_window = window_create();

	// Set handlers to manage the elements inside the Window
	window_set_window_handlers(s_main_window, (WindowHandlers ) { .load =
			handle_main_window_load, .unload = handle_main_window_unload });
  
  //Register handler for config messages from js
	app_message_register_inbox_received(&handle_inbox_received);
	app_message_register_inbox_dropped(&handle_inbox_dropped);

	if(app_message_open(app_message_inbox_size_maximum(),
			app_message_inbox_size_maximum()) != APP_MSG_OK)
		APP_LOG(APP_LOG_LEVEL_ERROR, "App message open failed");

	// Register with TickTimerService
	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

	// Show the Window on the watch, with animated = true
	window_stack_push(s_main_window, true);
}

/* app deinit */
static void deinit() {
	//cancel any animations
	animation_unschedule_all();
	app_message_deregister_callbacks();
	// Destroy the window
	window_destroy(s_main_window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
