
// simple program that downloads the quote of the day from "https://quotes.rest/qod",
// parses it and extracts the quote and the background image url,
// downloads the background image from said url,
// creates a texture based off of the downloaded image data (not necessarily a jpg),
// and displays all that on the screen (quote text follows mouse if left-clicked)

#include "root.h"

#include "lib/loaders/image.h"

#include "lib/utils/string/strcmp.h"
#include "lib/utils/string/strlen.h"

#include "lib/extensions/font.h"
#include "lib/extensions/float.h"
#include "lib/extensions/mouse.h"
#include "lib/extensions/surface.h"
#include "lib/extensions/requests.h"

bool main(void) {
	request_t quote_request;
	if (request_get(&quote_request, "https://quotes.rest/qod")) {
		print("WARNING Quote request failed (error %lld)\n", quote_request.code);
		return true;
		
	}
	
	char* quote_text      = nullptr;
	char* quote_image_url = nullptr; 
	
	iterate (quote_request.bytes) {
		if (sncmp(quote_request.data + i, "\"quote\": \"", 9) == 0) { // found quote text
			var k;
			for (k = i += 10; quote_request.data[k] != '"' and quote_request.data[k + 1] != ',' and k < quote_request.bytes; k++);
			quote_text = (char*) malloc(k - i + 1);
			mset(quote_text, 0, k - i + 1);
			mcpy(quote_text, quote_request.data + i, k - i);
			i = k + 1;
			
		} elif (sncmp(quote_request.data + i, "\"background\": \"", 14) == 0) { // found quote image url
			var k;
			for (k = i += 15; quote_request.data[k] != '"' and quote_request.data[k + 1] != ',' and k < quote_request.bytes; k++);
			quote_image_url = (char*) malloc(k - i + 1);
			mset(quote_image_url, 0, k - i + 1);
			mcpy(quote_image_url, quote_request.data + i, k - i);
			break;
			
		}
		
	}
	
	request_free(&quote_request);
	if (no quote_text or no quote_image_url) {
		print("WARNING Failed to extract data from request response\n");
		return true;
		
	}
	
	request_t background_request;
	if (request_get(&background_request, quote_image_url)) {
		mfree(quote_text,      slen(quote_text)      + 1);
		mfree(quote_image_url, slen(quote_image_url) + 1);
		
		print("WARNING Quote background image request failed (error %lld)\n", background_request.code);
		return true;
		
	}
	
	texture_t background = create_texture_from_image(background_request.data, background_request.bytes);
	surface_t background_surface;
	new_surface(&background_surface, -UMAX_MARGIN, -UMAX_MARGIN, UMAX_MARGIN * 2, UMAX_MARGIN * 2);
	surface_set_texture(&background_surface, background);
	
	font_t font = create_font("config/fonts/main.ttf", UMAX / 20);
	texture_t quote_texture = create_texture_from_font(font, quote_text);
	surface_t quote_surface;
	
	flt x = 0;
	always {
		x += FLOAT_ONE / (video_fps() | 1);
		
		video_clear(0, 0, 0, 0);
		surface_draw(&background_surface);
		
		var width  = RATIO_X(get_font_width (font, quote_text));
		var height = RATIO_Y(get_font_height(font, quote_text));
		
		if (get_mouse_button(0, MOUSE_BUTTON_LEFT)) new_surface(&quote_surface, FLOAT_TO_UMAX_MARGIN(get_mouse_x(0)) - width / 2, FLOAT_TO_UMAX_MARGIN(get_mouse_y(0)) - height / 2, width, height);
		else new_surface(&quote_surface, -width / 2, FLOAT_TO_UMAX_MARGIN(MUL_FLOAT(SIN_FLOAT(x), FLOAT_ONE - UMAX_MARGIN_TO_FLOAT(height / 2))) - height / 2, width, height);
		
		surface_set_texture(&quote_surface, quote_texture);
		surface_draw(&quote_surface);
		
		video_flip();
		events_t events;
		
		get_events(&events);
		if (events.quit) {
			break;
			
		}
		
	}
	
	font_remove(font);
	texture_remove(quote_texture);
	texture_remove(background);
	
	mfree(quote_text,      slen(quote_text)      + 1);
	mfree(quote_image_url, slen(quote_image_url) + 1);
	
	request_free(&background_request);
	return false;
	
}
