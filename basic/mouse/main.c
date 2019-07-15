
// simple program that prints out the mouse count
// and displays a colour based on the first mouse's state
// red component: mouse x
// green component: mouse y
// blue component: is mouse's left button pressed?

#include "root.h"
#include "lib/extensions/mouse.h"

bool main(void) {
	print("There are %lld mice\n", get_mouse_count());
	mouse_t mouse = 0; // whatever
	
	always {
		video_clear(FLOAT_TO_UMAX(get_mouse_x(mouse) / 2 + FLOAT_HALF), FLOAT_TO_UMAX(get_mouse_y(mouse) / 2 + FLOAT_HALF), get_mouse_button(mouse, MOUSE_BUTTON_LEFT) * UMAX, UMAX);
		video_flip();
		events_t events;
		
		get_events(&events);
		if (events.quit) {
			return true;
			
		}
		
	}
	
	return false;
	
}
