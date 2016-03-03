#include <pebble.h>

static Window *window;
static AppTimer *timer; // to manage when to update dynamic_layer
static Layer *dynamic_layer; // layer to update for dynamic rendering
static BitmapLayer *background_layer;  // static background layer
static BitmapLayer *shuttle_layer; // shuttle layer

static GBitmap *background; // static background bitmap
static GBitmap *shuttle; // static (for now) shuttle bitmap

static uint32_t delta = 33;	//30FPS

/******** animation variables *********/
static const float gravity = 0.01; // Positive is downward
static const float acceleration = 0.005; // Positive is upward
static float velocity = 0;  // Positive is downward
static bool thrust = false;

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
  }
  else if (!thrust && shcoords->y < 168 - 25) {
    shcoords->y += velocity;
    velocity += gravity;
  }
  else {
    velocity = 0;
  }
  
  layer_set_frame(bitmap_layer_get_layer(shuttle_layer),
                  GRect((int)shcoords->x, (int)shcoords->y, 25,25));
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
	timer = app_timer_register(delta, (AppTimerCallback) timer_callback, 0);
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
static void click_provider(Window *window) {
  // Assign handlers for raw UP button input
  window_raw_click_subscribe(BUTTON_ID_UP, click_handler_up_pressed, click_handler_up_released, NULL);
}
/************************* WINDOW LIFECYCLE ***********************/

static void window_load(Window *window) {
  // init background image
  background_layer = bitmap_layer_create(GRect(0,0,144,168));
  background = gbitmap_create_with_resource(RESOURCE_ID_SPACE_BACKGROUND);
  bitmap_layer_set_bitmap(background_layer, background);
  
  // init shuttle image
  shuttle_layer = bitmap_layer_create(GRect(60,0,25,25));
  shuttle = gbitmap_create_with_resource(RESOURCE_ID_SHUTTLE);
  bitmap_layer_set_compositing_mode(shuttle_layer, GCompOpSet);
  bitmap_layer_set_bitmap(shuttle_layer, shuttle);
  
  // init dynamic layer
  dynamic_layer = layer_create(GRect(0,0,144,168));
  layer_set_update_proc(dynamic_layer, (LayerUpdateProc) render_dynamic_layer);
  layer_add_child(dynamic_layer, bitmap_layer_get_layer(shuttle_layer));
  
  // add layers to window
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(background_layer));
  layer_add_child(window_get_root_layer(window), dynamic_layer);
  
  // set click provider
  window_set_click_config_provider(window, (ClickConfigProvider) click_provider);
  // Start rendering on dynamic_layer
  start();
}

static void window_unload(Window *window) {
  // Destroy layers
  bitmap_layer_destroy(background_layer);
  layer_destroy(dynamic_layer);
  gbitmap_destroy(background);
  
  // Cancel timer
	app_timer_cancel(timer);
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