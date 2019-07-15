// simple program that uses AQUA's YouTube integration to search for "abba wateroo" (first for 4 results),
// extract the video information for each returned video from the query,
// automatically create a UI context element for each of said videos,
// and display all these video contexts in a larger context, that fills the whole screen

#define LIST_VIEW true // set this to false if you want a different format

#include "root.h"
#include "lib/extensions/mouse.h"
#include "lib/internet/integrations/youtube.h"

bool main(void) {
	youtube_video_t videos[4];
	#define video_count (sizeof(videos) / sizeof(youtube_video_t))
	if (youtube_search(videos, video_count, "abba waterloo")) {
		print("WARNING Failed to search YouTube\n");
		return true;
		
	}
	
	ui_t ui;
	new_ui(&ui, false);
	
	ui_element_t context;
	new_ui_element(&context, &ui);
	ui_element_add_context(&context, UI_ELEMENT_CONTEXT_FULL);
	
	youtube_context_t video_contexts[video_count];
	iterate (video_count) {
		new_youtube_context(&video_contexts[i], &ui, &videos[i], LIST_VIEW, i + 1);
		ui_element_context_add_element(&context, &video_contexts[i].context, UI_ELEMENT_CONTEXT_ROLE_NONE, LIST_VIEW ? UI_ELEMENT_CONTEXT_PACK_VERTICAL : UI_ELEMENT_CONTEXT_PACK_HORIZONTAL, true);
		
	}
	
	always {
		var fps = video_fps() | 1;
		video_clear(0, 0, 0, 0);
		
		ui_draw(&ui);
		
		var mouse_count = get_mouse_count();
		iterate (mouse_count) {
			bool button = get_mouse_button(i, MOUSE_BUTTON_LEFT);
			
			flt mx = get_mouse_x(i);
			flt my = get_mouse_y(i);
			
			for (var j = 0; j < video_count; j++) {
				ui_element_event(&video_contexts[j].context, fps, button, mx, my);
				
			}
			
		} iterate (video_count) {
			youtube_context_update(&video_contexts[i]);
			
		}
		
		ui_element_update(&context);
		ui_element_draw(&context, 0, 0);
		video_flip();
		
		events_t events;
		get_events(&events);
		if (events.quit) {
			break;
			
		} elif (events.resize) {
			ui_resize(&ui);
			
		}
		
	}
	
	dispose_ui(&ui);
	dispose_ui_element(&context);
	
	iterate (video_count) {
		dispose_youtube_context(&video_contexts[i]);
		dispose_youtube_video(&videos[i]);
		
	}
	
	return false;
	
}
