
#include "root.h"
#include "lib/extensions/font.h"
#include "lib/extensions/surface.h"

#define STRING "Hello world!"

bool main(void) {
	font_t font = create_font("config/fonts/main.ttf", UMAX / 20);
	texture_t texture = create_texture_from_font(font, STRING);
	
	var width  = RATIO_X(get_font_width (font, STRING));
	var height = RATIO_Y(get_font_height(font, STRING));
	
	surface_t surface;
	new_surface(&surface, -width / 2, -height / 2, width, height);
	surface_set_texture(&surface, texture);
	
	events_t events;
	always {
		video_clear(0, 0, 0, 0);
		surface_draw(&surface);
		video_flip();
		
		get_events(&events);
		if (events.quit) break;
		
	}
	
	texture_remove(texture);
	font_remove(font);
	return false;
	
}
