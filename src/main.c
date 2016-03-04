#include <pebble.h>
#include "main.h"

static Window *window;
static AppTimer *timer; // to manage when to update dynamic_layer
static Layer *dynamic_layer; // layer to update for dynamic rendering
static BitmapLayer *background_layer;  // static background layer
static BitmapLayer *shuttle_layer; // shuttle layer
static TextLayer *ending_text_layer; // Win or Loss Display
static TextLayer *wins_layer; // number of wins

static GBitmap *background; // static background bitmap
static GBitmap *shuttle; // static (for now) shuttle bitmap
static GBitmap *shuttle_boost; // shuttle with booster flames
static GBitmap *explosion; // exploding shuttle

static uint32_t delta = 33;	//30FPS

/******** animation variables *********/
static const float gravity = 0.01; // Positive is downward
static const float acceleration = 0.005; // Positive is upward
static float velocity = 0;  // Positive is downward
static bool thrust = false;
static float losing_speed = 0.2;
static bool game_over = false;
static int wins = 0;
static int fuel = 1000;

struct Ship_coords{
  float x;
  float y;
};

static struct Ship_coords *shcoords;


static struct Ship_coords* create_Ship_coords() {
  struct Ship_coords *this = malloc(sizeof(struct Ship_coords));
	this->x = 60.0;
	this->y = 0.0;
	
	return this;
}


/********************* RENDERING FUNCTIONS *********************/
/*
 * Update the items to be drawn in the dynamic_layer
 */
static void update() {
  if (thrust && shcoords->y > 0) {
    shcoords->y += velocity;
    velocity -= acceleration;
    bitmap_layer_set_bitmap(shuttle_layer, shuttle_boost);
  }
  else if (!thrust && shcoords->y < 168 - 25) {
    shcoords->y += velocity;
    velocity += gravity;
    bitmap_layer_set_bitmap(shuttle_layer, shuttle);
  }
  else {
    velocity = 0;
  }
  
  // Check if shuttle is landing
  if (shcoords->y > 168 - 25 - 26) {
    if (velocity > losing_speed) {
      // Ship has crashed, destroy
      bitmap_layer_set_bitmap(shuttle_layer, explosion);
      ending_text_set_loss();
      wins = 0;
    }
    else {
      // Ship landed successfully!
      ending_text_set_win();
      bitmap_layer_set_bitmap(shuttle_layer, shuttle);
      wins++;
    }
    game_over = true;
    update_wins_text();
  }
  layer_set_frame(bitmap_layer_get_layer(shuttle_layer),
                  GRect((int)shcoords->x,(int)shcoords->y,25,45));
}

/*
 * Reset everything for new round
 */
static void reset() {
  // Reset Ship
  bitmap_layer_set_bitmap(shuttle_layer, shuttle);
  shcoords->y = 0;
  layer_set_frame(bitmap_layer_get_layer(shuttle_layer),
                  GRect((int)shcoords->x,(int)shcoords->y,25,45));
  velocity = 0.0;
  game_over = false;
  
  // Clear ending text from last round
  ending_text_clear();
  
  // set timer for new animations
  timer = app_timer_register(delta, (AppTimerCallback) timer_callback, 0);
}

/*
 * Callback to manage updating the drawn screen.
 */
static void timer_callback(void *data) {	
	// Update the screen animations
	update();

	// Render
	layer_mark_dirty(dynamic_layer);
	
	// Register next render
  if (!game_over) {
	  timer = app_timer_register(delta, (AppTimerCallback) timer_callback, 0);
  }
}

/*
 * Start rendering loop
 */
static void start()
{
	timer = app_timer_register(delta, (AppTimerCallback) timer_callback, 0);
}

/*
 * Render items onto the dynamic_layer
 */
static void render_dynamic_layer(Layer *layer, GContext *ctx) {

}

/************************* CLICKING HANDLING ********************/
static void click_handler_up_pressed() {
  thrust = true;
}
static void click_handler_up_released() {
  thrust = false;
}
static void click_handler_select_clicked() {
  if (game_over == true) {
    reset();
  }
}
static void click_provider(Window *window) {
  // Assign handlers for raw UP button input
  window_raw_click_subscribe(BUTTON_ID_UP, click_handler_up_pressed, click_handler_up_released, NULL);
  // Assign for select button clicked
  window_single_click_subscribe(BUTTON_ID_SELECT, click_handler_select_clicked);
}
/************************* WINDOW LIFECYCLE ***********************/

static void window_load(Window *window) {
  // init background image
  background_layer = bitmap_layer_create(GRect(0,0,144,168));
  background = gbitmap_create_with_resource(RESOURCE_ID_SPACE_BACKGROUND);
  bitmap_layer_set_bitmap(background_layer, background);
    
  // init game message text layers
  ending_text_layer = text_layer_create(GRect(0, 30, 144, 70));
  text_layer_set_background_color(ending_text_layer, GColorClear);
  wins_layer = text_layer_create(GRect(60,0,84,25));
  text_layer_set_background_color(wins_layer, GColorClear);
  text_layer_set_text_color(wins_layer, GColorWhite);
  text_layer_set_text_alignment(wins_layer, GTextAlignmentRight);
  text_layer_set_font(wins_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(wins_layer, "0");
  
  
  // init shuttle image
  shuttle_layer = bitmap_layer_create(GRect(60,0,25,45));
  shuttle = gbitmap_create_with_resource(RESOURCE_ID_SHUTTLE);
  explosion = gbitmap_create_with_resource(RESOURCE_ID_EXPLOSION_IMG);
  shuttle_boost = gbitmap_create_with_resource(RESOURCE_ID_SHUTTLE_FLAMES);
  bitmap_layer_set_compositing_mode(shuttle_layer, GCompOpSet);
  bitmap_layer_set_alignment(shuttle_layer,GAlignTopLeft);
  bitmap_layer_set_bitmap(shuttle_layer, shuttle);
  
  // init dynamic layer
  dynamic_layer = layer_create(GRect(0,0,144,168));
  layer_set_update_proc(dynamic_layer, (LayerUpdateProc) render_dynamic_layer);
  layer_add_child(dynamic_layer, bitmap_layer_get_layer(shuttle_layer));
  
  // add layers to window
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(background_layer));
  layer_add_child(window_get_root_layer(window), dynamic_layer);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(ending_text_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(wins_layer));
  
  // set click provider
  window_set_click_config_provider(window, (ClickConfigProvider) click_provider);
  // Start rendering on dynamic_layer
  start();
}

static void window_unload(Window *window) {
  // Destroy layers
  bitmap_layer_destroy(background_layer);
  text_layer_destroy(ending_text_layer);
  text_layer_destroy(wins_layer);
  layer_destroy(dynamic_layer);
  gbitmap_destroy(background);
  gbitmap_destroy(shuttle);
  gbitmap_destroy(explosion);
  gbitmap_destroy(shuttle_boost);
  
  // Cancel timer
	app_timer_cancel(timer);
}

static void ending_text_set_win() {
  text_layer_set_font(ending_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(ending_text_layer, GColorWhite);
  text_layer_set_text_color(ending_text_layer, GColorBlack);
  text_layer_set_text_alignment(ending_text_layer, GTextAlignmentCenter);
  text_layer_set_text(ending_text_layer, "You Win?!? No way!!");
}

static void ending_text_set_loss() {
  text_layer_set_font(ending_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(ending_text_layer, GColorWhite);
  text_layer_set_text_color(ending_text_layer, GColorBlack);
  text_layer_set_text_alignment(ending_text_layer, GTextAlignmentCenter);
  text_layer_set_text(ending_text_layer, "You suck, man");
}

static void ending_text_clear() {
  text_layer_set_background_color(ending_text_layer, GColorClear);
  text_layer_set_text(ending_text_layer, "");
}

static void update_wins_text() {
  static char str[4];
  snprintf(str, sizeof str, "%d", wins);
  text_layer_set_text(wins_layer, str);
}

/************************* APP LIFECYCLE ***********************/

static void handle_init(void) {
  // Create main window
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) 
	{
		.load = window_load,
		.unload = window_unload,
	});
  
  // init ship coords
  shcoords = create_Ship_coords();
  
  // Push window
  window_stack_push(window, true);
}

static void handle_deinit(void) {
  // deallocate ship coords
  free(shcoords);
  
  // Destry window
  window_destroy(window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}