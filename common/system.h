/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001-2004 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /cvsroot/scummvm/scummvm/common/system.h,v 1.52 2004/01/17 14:32:22 arisme Exp $
 *
 */

#ifndef COMMON_SYSTEM_H
#define COMMON_SYSTEM_H

#include "common/scummsys.h"
#include "common/savefile.h"

/**
 * Interface for ScummVM backends. If you want to port ScummVM to a system
 * which is not currently covered by any of our backends, this is the place
 * to start. ScummVM will create an instance of a subclass of this interface
 * and use it to interact with the system.
 *
 * In particular, a backend provides a video surface for ScummVM to draw in;
 * methods to create timers, to handle user input events,
 * control audio CD playback, and sound output.
 */
class OSystem {
public:
	static OSystem *instance();

public:
	typedef struct Mutex *MutexRef;
	typedef void (*SoundProc)(void *param, byte *buf, int len);
	typedef int (*TimerProc)(int interval);

	/**
	 * The types of events backends can generate.
	 * @see Event
	 */
	enum EventCode {
		EVENT_KEYDOWN = 1,
		EVENT_KEYUP = 2,
		EVENT_MOUSEMOVE = 3,
		EVENT_LBUTTONDOWN = 4,
		EVENT_LBUTTONUP = 5,
		EVENT_RBUTTONDOWN = 6,
		EVENT_RBUTTONUP = 7,
		EVENT_WHEELUP = 8,
		EVENT_WHEELDOWN = 9,

		EVENT_QUIT = 10,
		EVENT_SCREEN_CHANGED = 11
	};

	enum {
		KBD_CTRL = 1,
		KBD_ALT = 2,
		KBD_SHIFT = 4
	};

	/**
	 * Data structure for an event. A pointer to an instance of Event
	 * can be passed to poll_event. 
	 */
	struct Event {
		EventCode event_code;
		struct {
			int keycode;
			uint16 ascii;
			byte flags;
		} kbd;
		struct {
			int x;
			int y;
		} mouse;
	};

	enum {
		PROP_TOGGLE_FULLSCREEN = 1,
		PROP_SET_WINDOW_CAPTION,
		PROP_OPEN_CD,
		PROP_SET_GFX_MODE,
		PROP_GET_GFX_MODE,
		PROP_GET_SAMPLE_RATE,
		PROP_GET_FULLSCREEN,
		PROP_GET_FMOPL_ENV_BITS,
		PROP_GET_FMOPL_EG_ENT,
		PROP_TOGGLE_ASPECT_RATIO,
		PROP_TOGGLE_MOUSE_GRAB,
		PROP_WANT_RECT_OPTIM,
		PROP_HAS_SCALER,
		PROP_TOGGLE_VIRTUAL_KEYBOARD
	};
	union Property {
		const char *caption;
		int cd_num;
		int gfx_mode;
		bool show_cursor;
		bool show_keyboard;
	};
	
	enum SoundFormat {
		SOUND_8BIT = 0,
		SOUND_16BIT = 1
	};


	/** Virtual destructor */
	virtual ~OSystem() {}


	virtual void installStart()	{ }
	virtual void installDone() {}
	virtual void installUpdate(byte progCur, byte progMax) { }
	virtual void setIsLoading(bool isloading) {}


	/** @name Graphics */
	//@{

	virtual bool init() = 0;

	/** Set the size of the video bitmap. Typically 320x200 pixels. */
	virtual void init_size(uint w, uint h) = 0;

	/**
	 * Returns the currently set screen height.
	 * @see init_size
	 * @return the currently set screen height
	 */
	virtual int16 get_height() = 0;

	/**
	 * Returns the currently set screen width.
	 * @see init_size
	 * @return the currently set screen width
	 */
	virtual int16 get_width() = 0;

	/** Set colors of the palette. */
	virtual void set_palette(const byte *colors, uint start, uint num) = 0;

	/**
	 * Draw a bitmap to screen.
	 * The screen will not be updated to reflect the new bitmap, you have
	 * to call update_screen to do that.
	 * @see update_screen
	 */
	virtual void copy_rect(const byte *buf, int pitch, int x, int y, int w, int h) = 0;
	virtual void copy_screen(const byte* buf, int y, int h, const uint32* dirtymask = 0, bool doublebuffer = true) = 0;

	/**
	 * Moves the screen content by the offset specified via dx/dy.
	 * Only the region from x=0 till x=height-1 is affected.
	 * @param dx	the horizontal offset.
	 * @param dy	the vertical offset.
	 * @param height	the number of lines which in which the move will be done.
	 */
	virtual void move_screen(int dx, int dy, int height) = 0;

	/** Update the dirty areas of the screen. */
	virtual void update_screen() = 0;

	/**
	 * Set current shake position, a feature needed for some SCUMM screen effects.
	 * The effect causes the displayed graphics to be shifted upwards by the specified 
	 * (always positive) offset. The area at the bottom of the screen which is moved
	 * into view by this is filled by black. This does not cause any graphic data to
	 * be lost - that is, to restore the original view, the game engine only has to
	 * call this method again with a 0 offset. No calls to copy_rect are necessary.
	 * @param shakeOffset	the shake offset
	 */
	virtual void set_shake_pos(int shakeOffset) = 0;

	/** Convert the given RGB triplet into a NewGuiColor. A NewGuiColor can be
	 * 8bit, 16bit or 32bit, depending on the target system. The default
	 * implementation generates a 16 bit color value, in the 565 format
	 * (that is, 5 bits red, 6 bits green, 5 bits blue).
	 * @see colorToRGB
	 */
	virtual NewGuiColor RGBToColor(uint8 r, uint8 g, uint8 b) = 0;


	//@}



	/** @name Mouse */
	//@{

	/** Show or hide the mouse cursor. */
	virtual bool show_mouse(bool visible) = 0;

	/** 
	 * Move ("warp) the mouse cursor to the specified position.
	 */
	virtual void warp_mouse(int x, int y) = 0;

	/** Set the bitmap used for drawing the cursor. */
	virtual void set_mouse_cursor(uint32 id, const byte *buf, const byte* mask, uint w, uint h, int hotspot_x, int hotspot_y) = 0;
	virtual bool mouse_cursor_cached(uint32 id) { return false; }
	//@}

	/** @name Events and Time */
	//@{

	/** Get the number of milliseconds since the program was started. */
	virtual uint32 get_msecs() = 0;

	/** Delay/sleep for the specified amount of milliseconds. */
	virtual void delay_msecs(uint msecs) = 0;

	/** Set the timer callback. */
	virtual void set_timer(TimerProc callback, int interval) = 0;

	/**
	 * Get the next event in the event queue.
	 * @param event	point to an Event struct, which will be filled with the event data.
	 * @return true if an event was retrieved.
	 */
	virtual bool poll_event(Event *event) = 0;

	//@}



	/** @name Sound */
	//@{
	/**
	 * Set the audio callback which is invoked whenever samples need to be generated.
	 * Currently, only the 16-bit signed mode is ever used for Simon & Scumm
	 * @param proc		pointer to the callback.
	 * @param param		an arbitrary parameter which is stored and passed to proc.
	 * @param format	the sample type format.
	 */
	virtual bool set_sound_proc(SoundProc proc, void *param, SoundFormat format) = 0;

	/**
	 * Remove any audio callback previously set via set_sound_proc, thus effectively
	 * stopping all audio output immediately.
	 * @see set_sound_proc
	 */
	virtual void clear_sound_proc() = 0;
	//@} 
		


	/**
	 * @name Audio CD
	 * The methods in this group deal with Audio CD playback.
	 */
	//@{

	/**
	 * Poll CD status
	 * @return true if CD audio is playing
	 */
	virtual bool poll_cdrom() = 0;

	/**
	 * Start audio CD playback. 
	 * @param track			the track to play.
	 * @param num_loops		how often playback should be repeated (-1 = infinitely often).
	 * @param start_frame	the frame at which playback should start (75 frames = 1 second).
	 * @param duration		the number of frames to play.
	 */
	virtual void play_cdrom(int track, int num_loops, int start_frame, int duration) = 0;

	/**
	// Stop audio CD playback
	 */
	virtual void stop_cdrom() = 0;

	/**
	// Update cdrom audio status
	 */
	virtual void update_cdrom() = 0;
	//@} 



	/** @name Mutex handling */
	//@{
	/**
	 * Create a new mutex.
	 * @return the newly created mutex, or 0 if an error occured.
	 */
	virtual MutexRef create_mutex(void) = 0;

	/**
	 * Lock the given mutex.
	 * @param mutex	the mutex to lock.
	 */
	virtual void lock_mutex(MutexRef mutex) = 0;

	/**
	 * Unlock the given mutex.
	 * @param mutex	the mutex to unlock.
	 */
	virtual void unlock_mutex(MutexRef mutex) = 0;

	/**
	 * Delete the given mutex. Make sure the mutex is unlocked before you delete it.
	 * If you delete a locked mutex, the behavior is undefined, in particular, your
	 * program may crash.
	 * @param mutex	the mutex to delete.
	 */
	virtual void delete_mutex(MutexRef mutex) = 0;
	//@} 


	
	/** @name Overlay */
	//@{
	virtual void show_overlay() = 0;
	virtual void hide_overlay() = 0;
	virtual void clear_overlay() = 0;
	virtual void grab_overlay(NewGuiColor *buf, int pitch) = 0;
	//@} 



	/** @name Miscellaneous */
	//@{
	/** Get or set a backend property. */
	virtual uint32 property(int param, Property *value) = 0;

	/** Quit (exit) the application. */
	virtual void quit() = 0;

	/** Savefile management. */
	virtual SaveFileManager *get_savefile_manager() {
		return new SaveFileManager();
	}
	//@}
};

/** The global OSystem instance. Inited in main(). */
extern OSystem *g_system;

#endif 
