#include <pebble.h>
#include <num2words.h>
#define DEBUG 1
#define NUM_LAYERS 4
#define TIME_SLOT_ANIMATION_DURATION 700
#define FONT_LINE_1 FONT_KEY_BITHAM_42_BOLD
#define FONT_LINE_2 FONT_KEY_BITHAM_42_LIGHT
#define FONT_LINE_3 FONT_KEY_BITHAM_42_LIGHT
#define FONT_LINE_4 FONT_KEY_BITHAM_42_LIGHT
#define OFFSET_LINE_1 0
#define OFFSET_LINE_2 0
#define OFFSET_LINE_3 0
#define OFFSET_LINE_4 0
#define ALIGN_LINE_1 GTextAlignmentLeft
#define ALIGN_LINE_2 GTextAlignmentLeft
#define ALIGN_LINE_3 GTextAlignmentLeft
#define ALIGN_LINE_4 GTextAlignmentLeft
#define INVERT_COLOR_LINE_1 0
#define INVERT_COLOR_LINE_2 0
#define INVERT_COLOR_LINE_3 0
#define INVERT_COLOR_LINE_4 0

enum e_layer_names {
	MINUTES = 0, TENS, HOURS, DATE
};

typedef enum e_direction {
	OUT = 0, IN
} eDirection;

void slide_out_animation_stopped(Animation *slide_out_animation, bool finished, void *context);
void slide_in_animation_stopped(Animation *slide_out_animation, bool finished, void *context);

typedef struct CommonWordsData {
	TextLayer *label;
	PropertyAnimation *prop_animation;
	void (*update)(struct tm *t, char *words);
	char buffer[BUFFER_SIZE];
} CommonWordsData;

static Window *s_main_window;
static CommonWordsData* s_layers[4];
static struct tm *new_time;

void animate(CommonWordsData *layer, eDirection direction, GRect *from_frame,
		GRect *to_frame) {
	layer->prop_animation = property_animation_create_layer_frame(
			text_layer_get_layer(layer->label), from_frame, to_frame);
	animation_set_duration((Animation*) layer->prop_animation,
			TIME_SLOT_ANIMATION_DURATION);
	if(direction == OUT){
		animation_set_curve((Animation*) layer->prop_animation, AnimationCurveEaseIn);
		animation_set_handlers((Animation*) layer->prop_animation,(AnimationHandlers ) { .stopped = slide_out_animation_stopped },(void *) layer);
	}else{
		animation_set_curve((Animation*) layer->prop_animation, AnimationCurveEaseOut);
		animation_set_handlers((Animation*) layer->prop_animation,(AnimationHandlers ) { .stopped = slide_in_animation_stopped },(void *) layer);
	}
	animation_schedule((Animation*) layer->prop_animation);
}

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

void slide_out_animation_stopped(Animation *slide_out_animation, bool finished,
		void *context) {
	CommonWordsData *layer = (CommonWordsData *) context;
	property_animation_destroy(layer->prop_animation);
	if(finished){
		layer->update(new_time, layer->buffer);
		slide_in(layer);
	}
}

void slide_in_animation_stopped(Animation *slide_out_animation, bool finished,
		void *context) {
	CommonWordsData *layer = (CommonWordsData *) context;
	property_animation_destroy(layer->prop_animation);
}

void update_layer(CommonWordsData *layer, struct tm *t) {
	GRect from_frame = layer_get_frame(text_layer_get_layer(layer->label));
	GRect frame = layer_get_frame(window_get_root_layer(s_main_window));

	GRect to_frame = GRect(-frame.size.w, from_frame.origin.y, frame.size.w,
			from_frame.size.h);

	// Schedule the out animation
		animate(layer, OUT, NULL, &to_frame);
}

static void handle_minute_tick(struct tm *t, TimeUnits units_changed) {
	memcpy(t,new_time, sizeof(struct tm));
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

void init_layer(Layer *window_layer, CommonWordsData *layer, GRect rect,
		GFont font) {
	layer->label = text_layer_create(rect);
	text_layer_set_background_color(layer->label, GColorClear);
	text_layer_set_text_color(layer->label, GColorWhite);
	text_layer_set_font(layer->label, font);
	layer_add_child(window_layer, text_layer_get_layer(layer->label));
}

static void main_window_load(Window *window) {

	window_set_background_color(window, GColorBlack);
	Layer *window_layer = window_get_root_layer(window);

	for (int i = 0; i < NUM_LAYERS; ++i) {
		s_layers[i] = malloc(sizeof(CommonWordsData));
	}

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
			fonts_get_system_font(FONT_LINE_1));

	// tens of minutes
	init_layer(window_layer, s_layers[TENS], GRect(0, 38, bounds.size.w, 50),
			fonts_get_system_font(FONT_LINE_2));

	// minutes
	init_layer(window_layer, s_layers[MINUTES],
			GRect(0, 76, bounds.size.w, 50),
			fonts_get_system_font(FONT_LINE_3));

	//Date
	init_layer(window_layer, s_layers[DATE], GRect(0, 114, bounds.size.w, 50),
			fonts_get_system_font(FONT_LINE_4));

	//show your face
	time_t now = time(NULL);
	new_time = localtime(&now);

	for (int i = 0; i < NUM_LAYERS; ++i) {
		s_layers[i]->prop_animation = NULL;
		s_layers[i]->update(new_time, s_layers[i]->buffer);
		slide_in(s_layers[i]);
	}
}

static void main_window_unload(Window *window) {
	for (int i = 0; i < NUM_LAYERS; ++i) {
		text_layer_destroy(s_layers[i]->label);
		free(s_layers[i]);
		s_layers[i] = NULL;
	}
}

static void init() {
	// Create main window element  and assign to pointer
	s_main_window = window_create();

	// Set handlers to manage the elements inside the Window
	window_set_window_handlers(s_main_window, (WindowHandlers ) { .load =
					main_window_load, .unload = main_window_unload });

	// Register with TickTimerService
	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

	// Show the Window on the watch, with animated = true
	window_stack_push(s_main_window, true);
}

static void deinit() {
	animation_unschedule_all();
	// Destroy the window
	window_destroy(s_main_window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
