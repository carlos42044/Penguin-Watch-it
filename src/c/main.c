#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer;
static BitmapLayer *s_bitmap_layer;
static GBitmapSequence *s_sequence;
static GBitmap *s_bitmap;

// Handles the animation of the apng(Raw binary resource)
static void timer_handler(void *context) {
  uint32_t next_delay_ms = 1000;
  int timer = 1000;
  // Advance to the next APNG Frame, and get the delay for this frame
  if(gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_bitmap, &next_delay_ms)) {
    // Set the new frame into the BitmapLayer
    bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
    layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));
    
    // Timer for that frame's delay
    app_timer_register(timer, timer_handler, NULL);
  }
}

// Handles the time and date
static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                         "%H:%M" : "%I:%M", tick_time);
  
  // Write the current date into a buffer
  static char s_buffer_date[12]; 
  strftime(s_buffer_date, sizeof(s_buffer_date), "%a, %b %d", tick_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "s_buffer_date[] value is: %s" , s_buffer_date);
  
  //Display this time on the TextLayer 
  text_layer_set_text(s_time_layer, s_buffer);
  text_layer_set_text(s_date_layer, s_buffer_date);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  update_time();
}

// Function to set all of the text layer's attributes
static void text_layer_attributes(TextLayer *text_layer, GColor background_color, GColor text_color, GTextAlignment text_alignment) {
  text_layer_set_background_color(text_layer, background_color);
  text_layer_set_text_color(text_layer, text_color);
  text_layer_set_text_alignment(text_layer, text_alignment);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));

}

static void animation(GRect bounds) {
  // Creating the sequence
  s_sequence = gbitmap_sequence_create_with_resource(RESOURCE_ID_SCALED_LARGE);
  // Create the blank GBitmap using APNG frame size
  GSize frame_size = gbitmap_sequence_get_bitmap_size(s_sequence);
  s_bitmap = gbitmap_create_blank(frame_size, GBitmapFormat8Bit);
  
  // Begin the bitmap layer
  s_bitmap_layer = bitmap_layer_create(GRect(bounds.size.w/2-(bounds.size.w/6), bounds.size.h/32, frame_size.w, frame_size.h));
  // SET THE LAYER TO COMPOSITING_MODE !!!!!!!!!!
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet); //To make it transparent 
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
  // for app_timer_register
  uint32_t first_delay_ms = 10;
  
  // Schedule a timer to advance the first frame
  app_timer_register(first_delay_ms, timer_handler, NULL);
      
}

static void main_window_load(Window *window) {
  // Get information about the window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Function that handles the bitmap and animation
  animation(bounds);
  
  // Gets the bounds of the animation in order to set the time text layer rigt underneath
  Layer *bound_penguin = bitmap_layer_get_layer(s_bitmap_layer);
  GRect bounds_1 = layer_get_bounds(bound_penguin);

  // Create a Time TextLayer with specific bounds
  s_time_layer = text_layer_create(GRect(0, bounds_1.size.h, bounds.size.w, 50)); 
  
  // Create TextLayer for date
 // Layer *time_layer = text_layer_get_layer(s_time_layer);
  //GRect time_layer_bounds = layer_get_bounds(time_layer);
  s_date_layer = text_layer_create(GRect(0,bounds.size.h-50, bounds.size.w, 30));
  
  // date layer attributes
  text_layer_attributes(s_date_layer, GColorClear, GColorBlack, GTextAlignmentCenter);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  
  // Add the text layer with the correct attributes
  text_layer_attributes(s_time_layer, GColorClear, GColorBlack, GTextAlignmentCenter);

  // setting the color of the window background
  window_set_background_color(s_main_window, GColorJaegerGreen);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));

}

// Destroys whatever was created
static void main_window_unload(Window *window) {
  // Destory the text layer
  text_layer_destroy(s_time_layer);
  gbitmap_sequence_destroy(s_sequence);
  gbitmap_destroy(s_bitmap);
  bitmap_layer_destroy(s_bitmap_layer);
}

void init(){
  // Create main window element and assign to pointer
  s_main_window = window_create();
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers){
    .load = main_window_load, 
    .unload = main_window_load
  });
  
  // Show the window on the watch, with animated=true
  window_stack_push(s_main_window, true);
}

void deinit(){
  // Destory Window
  window_destroy(s_main_window);
}

int main(void){
  init();
  app_event_loop();
  deinit();
}