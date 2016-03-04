#ifndef _MAIN_
#define _MAIN_

/***** Rendering Functions *****/
static void update();
static void timer_callback(void *data);
static void start();
static void reset();
static void render_dynamic_layer(Layer *layer, GContext *ctx);


/***** Clicking Handling *****/
static void click_handler_up_pressed();
static void click_handler_up_released();
static void click_handler_select_clicked();
static void click_provider(Window *window);

/***** Window Lifecycle *****/
static void window_load(Window *window);
static void window_unload(Window *window);
static void ending_text_set_win();
static void ending_text_set_loss();
static void ending_text_clear();
static void update_wins_text();

/***** App Lifecycle *****/


#endif