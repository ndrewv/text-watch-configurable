#include <pebble.h>
#include <num2words.h>
  
//#define DEBUG 1
  
#define version_major 1 
#define version_minor 4

#define NUM_LAYERS 4
#define TIME_SLOT_ANIMATION_DURATION 700

  //flash storage key
#define FLASH_OPTIONS 0
//option message keys
#define KEY_BACKGROUND 0
#define KEY_ALIGN 1
#define KEY_CAPITAL 2
#define KEY_FONT_STYLE 3
#define KEY_DATE_STYLE 4
#define KEY_VERT_OFFSET 5
#define KEY_ANIMATION 6
#define KEY_LIMIT 7

#define DEFAULT_FONT_UPPER FONT_KEY_BITHAM_42_BOLD
#define DEFAULT_FONT_LOWER FONT_KEY_BITHAM_42_LIGHT

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

//layer reference array
  static const uint32_t s_offset_base[NUM_LAYERS] = {0, 38, 76, 114};
  static const char *s_font_base[NUM_LAYERS] = {DEFAULT_FONT_UPPER, DEFAULT_FONT_LOWER, DEFAULT_FONT_LOWER, DEFAULT_FONT_LOWER};
//font reference array
static const uint32_t s_custom_fonts_upper[KEY_CUSTOM_FONT_SIZE] = {RESOURCE_ID_FONT_DANIEL_BOLD_32, RESOURCE_ID_FONT_FINELINER_38, RESOURCE_ID_FONT_PHILO_BOLD_38, RESOURCE_ID_FONT_RETRO_44, RESOURCE_ID_FONT_STAR_BOLD_26, RESOURCE_ID_FONT_STENCILIA_34, RESOURCE_ID_FONT_TECH_BOLD_22, RESOURCE_ID_FONT_VISITOR_30, RESOURCE_ID_FONT_FLO_BOLD_34, RESOURCE_ID_FONT_QUICK_BOLD_36, RESOURCE_ID_FONT_COLLEGE_BOLD_38, RESOURCE_ID_FONT_LCD_34, RESOURCE_ID_FONT_ARCHISTICO_34, RESOURCE_ID_FONT_EURO_BOLD_44};
static const uint32_t s_custom_fonts_lower[KEY_CUSTOM_FONT_SIZE] = {RESOURCE_ID_FONT_DANIEL_32, RESOURCE_ID_FONT_FINELINER_38, RESOURCE_ID_FONT_PHILO_38, RESOURCE_ID_FONT_RETRO_44, RESOURCE_ID_FONT_STAR_26, RESOURCE_ID_FONT_STENCILIA_34, RESOURCE_ID_FONT_TECH_22, RESOURCE_ID_FONT_VISITOR_30, RESOURCE_ID_FONT_FLO_34, RESOURCE_ID_FONT_QUICK_36, RESOURCE_ID_FONT_COLLEGE_38, RESOURCE_ID_FONT_LCD_34, RESOURCE_ID_FONT_ARCHISTICO_34, RESOURCE_ID_FONT_EURO_44};

//animation handler prototypes
void handle_slide_out_animation_stopped(Animation *slide_out_animation, bool finished, void *context);

//structs and enums
enum e_align {
  ALIGN_LEFT = 0, ALIGN_CENTRE, ALIGN_RIGHT
};

enum e_layer_names {
	  HOURS= 0, TENS, MINUTES, DATE
};

typedef enum e_direction {
	OUT = 0, IN
} eDirection;

typedef struct CommonWordsData {
	TextLayer *label;
	PropertyAnimation *prop_animation_out;
  PropertyAnimation *prop_animation_in;
	char buffer[BUFFER_SIZE];
  void (*update)(struct tm *t, char *words);
} CommonWordsData;

//member variables
static Window *s_main_window;
static CommonWordsData *s_layers[4];
static InverterLayer *s_inverter_layer;
static struct tm *s_new_time;
static GFont *s_custom_font_upper;
static GFont *s_custom_font_lower;
static int s_options [KEY_LIMIT];

/*  Apply the application settings from the options array */
void update_configuration(void)
{
  //update background colour
  if(s_options[KEY_BACKGROUND] == 0){
    layer_set_hidden(inverter_layer_get_layer(s_inverter_layer), true);
  }else{
    layer_set_hidden(inverter_layer_get_layer(s_inverter_layer), false);
  }
  
  //update font style
  if(s_options[KEY_FONT_STYLE] > 0){
    fonts_unload_custom_font(*s_custom_font_upper);
    fonts_unload_custom_font(*s_custom_font_lower);
    *s_custom_font_upper = fonts_load_custom_font(resource_get_handle(s_custom_fonts_upper[(s_options[KEY_FONT_STYLE]-KEY_SYSTEM_FONT_SIZE)]));
    *s_custom_font_lower = fonts_load_custom_font(resource_get_handle(s_custom_fonts_lower[(s_options[KEY_FONT_STYLE]-KEY_SYSTEM_FONT_SIZE)]));
    text_layer_set_font(s_layers[HOURS]->label, *s_custom_font_upper);
    text_layer_set_font(s_layers[MINUTES]->label, *s_custom_font_lower);
    text_layer_set_font(s_layers[TENS]->label, *s_custom_font_lower);
    text_layer_set_font(s_layers[DATE]->label, *s_custom_font_lower);
  }else{
    text_layer_set_font(s_layers[HOURS]->label, fonts_get_system_font(DEFAULT_FONT_UPPER));
    text_layer_set_font(s_layers[MINUTES]->label, fonts_get_system_font(DEFAULT_FONT_LOWER));
    text_layer_set_font(s_layers[TENS]->label, fonts_get_system_font(DEFAULT_FONT_LOWER));
    text_layer_set_font(s_layers[DATE]->label, fonts_get_system_font(DEFAULT_FONT_LOWER));
  }
  
  //update text alignment
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
  
  //apply update
  for(int j = 0; j < NUM_LAYERS; j++){
    TextLayer* layer = (TextLayer*)text_layer_get_layer(s_layers[j]->label);
    text_layer_set_text_alignment(layer, align);
    layer_set_frame((Layer*)layer, GRect(0, ((j*38) + s_options[KEY_VERT_OFFSET]), layer_get_bounds(window_get_root_layer(s_main_window)).size.w, 42));
    //update animations
    GRect frame = layer_get_frame(window_get_root_layer(s_main_window));  
    property_animation_destroy(s_layers[j]->prop_animation_out); 
    property_animation_destroy(s_layers[j]->prop_animation_in); 
    GRect from_frame = layer_get_frame(text_layer_get_layer(s_layers[j]->label));
    GRect to_frame = GRect(-frame.size.w, from_frame.origin.y, frame.size.w, from_frame.size.h);
    //create animations
    s_layers[j]->prop_animation_out = property_animation_create_layer_frame(text_layer_get_layer(s_layers[j]->label), &from_frame, &to_frame);
    animation_set_duration((Animation*) s_layers[j]->prop_animation_out, s_options[KEY_ANIMATION]);
	  animation_set_curve((Animation*) s_layers[j]->prop_animation_out, AnimationCurveEaseIn);
	  animation_set_handlers((Animation*) s_layers[j]->prop_animation_out,(AnimationHandlers ) { .stopped = handle_slide_out_animation_stopped },(void *) s_layers[j]);
    to_frame = GRect(2 * frame.size.w, from_frame.origin.y, frame.size.w, from_frame.size.h);
    s_layers[j]->prop_animation_in = property_animation_create_layer_frame(text_layer_get_layer(s_layers[j]->label), &to_frame, &from_frame);
    animation_set_duration((Animation*) s_layers[j]->prop_animation_in, s_options[KEY_ANIMATION]);
	  animation_set_curve((Animation*) s_layers[j]->prop_animation_in, AnimationCurveEaseOut);
  }
}

/* Text Layer initialisation */
void init_layer(Layer *window_layer, CommonWordsData *layer, GRect rect,
		GFont font) {
	layer->label = text_layer_create(rect);
	text_layer_set_background_color(layer->label, GColorClear);
	text_layer_set_text_color(layer->label, GColorWhite);
	text_layer_set_font(layer->label, font);
  layer_set_clips((Layer*)layer->label, false);
	layer_add_child(window_layer, text_layer_get_layer(layer->label));
}

/* Schedule new slide in animation for referenced text layer */
void slide_in(CommonWordsData *layer) {
	GRect origin_frame = layer_get_frame(text_layer_get_layer(layer->label));
	GRect frame = layer_get_frame(window_get_root_layer(s_main_window));
	GRect from_frame = GRect(2 * frame.size.w, origin_frame.origin.y,
			frame.size.w, origin_frame.size.h);

	layer_set_frame(text_layer_get_layer(layer->label), from_frame);
	text_layer_set_text(layer->label, layer->buffer);

  animation_schedule((Animation*) layer->prop_animation_in);
}

/* handler run when slide out animation completes */
void handle_slide_out_animation_stopped(Animation *slide_out_animation, bool finished,
		void *context) {
	CommonWordsData *layer = (CommonWordsData *) context;
    //schedule slide in animation
    if(finished){
  		layer->update(s_new_time, layer->buffer);
      slide_in(layer);
  	}
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
      animation_schedule((Animation*) s_layers[MINUTES]->prop_animation_out);
		}
		if (t->tm_min % 10 == 0 || (t->tm_min > 10 && t->tm_min < 20)
				|| t->tm_min == 1) {
#ifdef DEBUG
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Update layer %u", TENS);
#endif
      animation_schedule((Animation*) s_layers[TENS]->prop_animation_out);
		}
	}
	if ((units_changed & HOUR_UNIT) == HOUR_UNIT
			|| ((t->tm_hour == 00 || t->tm_hour == 12) && t->tm_min == 01)) {
#ifdef DEBUG
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Update layer %u", HOURS);
#endif
    animation_schedule((Animation*) s_layers[HOURS]->prop_animation_out);
	}
	if ((units_changed & DAY_UNIT) == DAY_UNIT) {
#ifdef DEBUG
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Update layer %u", DATE);
#endif
    animation_schedule((Animation*) s_layers[DATE]->prop_animation_out);
	}
}

/* App message input handler */
void handle_inbox_received(DictionaryIterator *iterator, void *context)
{
  //unsubscribe to prevent update collision
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
  fuzzy_set_date_style(s_options[KEY_DATE_STYLE]);
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
	}
  // Reregister with TickTimerService
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
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Read options %d - %d, %d - %d, %d - %d, %d - %d, %d - %d", KEY_BACKGROUND, s_options[KEY_BACKGROUND], KEY_ALIGN, s_options[KEY_ALIGN], KEY_CAPITAL, s_options[KEY_CAPITAL], KEY_FONT_STYLE, s_options[KEY_FONT_STYLE], KEY_DATE_STYLE, s_options[KEY_DATE_STYLE]);
#endif
  }else{
    s_options[KEY_ANIMATION] = TIME_SLOT_ANIMATION_DURATION;
  }
  fuzzy_set_date_lower(s_options[KEY_CAPITAL]);
  fuzzy_set_date_style(s_options[KEY_DATE_STYLE]);
  //load default font
  s_custom_font_upper = malloc(sizeof(GFont));
  s_custom_font_lower = malloc(sizeof(GFont));
  *s_custom_font_upper = fonts_load_custom_font(resource_get_handle(s_custom_fonts_upper[s_options[KEY_FONT_STYLE]]));
  *s_custom_font_lower = fonts_load_custom_font(resource_get_handle(s_custom_fonts_lower[s_options[KEY_FONT_STYLE]]));

	// Update time callbacks
	s_layers[MINUTES]->update = &fuzzy_sminutes_to_words;
	s_layers[TENS]->update = &fuzzy_minutes_to_words;
	s_layers[HOURS]->update = &fuzzy_hours_to_words;
	s_layers[DATE]->update = &fuzzy_dates_to_words;

	// Get the bounds of the window for sizing the text layer
	GRect bounds = layer_get_bounds(window_layer);

	// initialise layers
	GRect frame = layer_get_frame(window_get_root_layer(s_main_window));
  //hours
  for (int i = 0; i < NUM_LAYERS; ++i) {
    init_layer(window_layer, s_layers[i], GRect(0, s_offset_base[i] + s_options[KEY_VERT_OFFSET], bounds.size.w, 42),
		fonts_get_system_font(s_font_base[i]));
    //calculate offsets
    GRect from_frame = layer_get_frame(text_layer_get_layer(s_layers[i]->label));
    GRect to_frame = GRect(-frame.size.w, from_frame.origin.y, frame.size.w, from_frame.size.h);
    //create animations
    s_layers[i]->prop_animation_out = property_animation_create_layer_frame(text_layer_get_layer(s_layers[i]->label), &from_frame, &to_frame);
    animation_set_duration((Animation*) s_layers[i]->prop_animation_out, s_options[KEY_ANIMATION]);
	  animation_set_curve((Animation*) s_layers[i]->prop_animation_out, AnimationCurveEaseIn);
	  animation_set_handlers((Animation*) s_layers[i]->prop_animation_out,(AnimationHandlers ) { .stopped = handle_slide_out_animation_stopped },(void *) s_layers[i]);
    to_frame = GRect(2 * frame.size.w, from_frame.origin.y, frame.size.w, from_frame.size.h);
    s_layers[i]->prop_animation_in = property_animation_create_layer_frame(text_layer_get_layer(s_layers[i]->label), &to_frame, &from_frame);
    animation_set_duration((Animation*) s_layers[i]->prop_animation_in, s_options[KEY_ANIMATION]);
	  animation_set_curve((Animation*) s_layers[i]->prop_animation_in, AnimationCurveEaseOut);
  }
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
    fonts_unload_custom_font(*s_custom_font_upper);
    s_custom_font_upper = 0;
  }
  if(s_custom_font_lower != 0){
    fonts_unload_custom_font(*s_custom_font_lower);
    s_custom_font_lower = 0;
  } 
	for (int i = 0; i < NUM_LAYERS; ++i) {
    property_animation_destroy(s_layers[i]->prop_animation_out); 
    property_animation_destroy(s_layers[i]->prop_animation_in); 
		text_layer_destroy(s_layers[i]->label);
		free(s_layers[i]);
		s_layers[i] = NULL;
	}
  free(s_custom_font_upper);
  free(s_custom_font_lower);
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

	// Show the Window on the watch, with animated = true
	window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

/* app deinit */
static void deinit() {
  //unsubscribe
  tick_timer_service_unsubscribe();
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
