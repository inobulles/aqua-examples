
// simple program that loads an mp3 song from user/music,
// starts playing it midway through,
// prints out some info about said song,
// changes the screen colour based on how much of the song is left,
// oscillates between 0% and 100% volume based on a sine wave 500 times during the remainder of the song,
// pauses on a left click, and resumes on a right click

#include "root.h"

#include "lib/loaders/mp3.h"
#include "lib/extensions/sound.h"

#include "lib/extensions/time.h"
#include "lib/extensions/mouse.h"
#include "lib/extensions/float.h"

bool main(void) {
	sound_t music = create_sound_from_mp3_file("user/music/Owl City - Fireflies.mp3");
	if (no music) {
		print("WARNING Failed to load mp3 file\n");
		return true;
		
	}
	
	sound_setup   (music);
	sound_play    (music);
	sound_position(music, FLOAT_HALF); // start midway through the song
	
	flt seconds = get_sound_seconds(music);
	print("Song is %lld seconds long and is at %lld Hz\n", FLOAT_TO_INT(seconds), get_sound_frequency(music));
	seconds /= 2; // since we're starting midway through the song, we need to divide the remaining time by 2
	
	const flt x = unix_time();
	always {
		flt y = DIV_FLOAT(unix_time() - x, seconds);
		
		sound_volume(music, SIN_FLOAT(y * 500) / 2 + FLOAT_HALF);
		video_clear(FLOAT_TO_UMAX(y), 0, 0, UMAX);
		
		if (get_mouse_button(0, MOUSE_BUTTON_LEFT )) sound_action(music, SOUND_PAUSE);
		if (get_mouse_button(0, MOUSE_BUTTON_RIGHT)) sound_action(music, SOUND_RESUME);
		
		video_flip();
		events_t events;
		
		get_events(&events);
		if (events.quit) {
			break;
			
		}
		
	}
	
	sound_remove(music);
	return false;
	
}
