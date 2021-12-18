/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001/2002 The ScummVM project
 * Copyright (C) 2002 ph0x (GP32 port)
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
 * Standalone DOS->Atari resource converter for PC.
 * Anders Granlund
 * 
 */


#include "common/stdafx.h"
#include "common/scummsys.h"
#include "common/system.h"
#include "common/engine.h"
#include "common/util.h"
#include "backends/intern.h"

class OSystem_Converter : public OSystem {
public:
	typedef void (*SoundProc)(void *param, byte *buf, int len);
	typedef int (*TimerProc)(int interval);

	bool init() { return false; }
	void init_size(uint w, uint h) {}
	int16 get_height() { return 200; }
	int16 get_width() { return 320; }
	void set_palette(const byte *colors, uint start, uint num) {}
	void copy_rect(const byte *buf, int pitch, int x, int y, int w, int h) {}
	void copy_screen(const byte* src, int y, int h, const uint32* mask, bool doublebuffer) {}
	void move_screen(int dx, int dy, int height) {}
	void update_screen() {}
	void set_shake_pos(int shakeOffset) {}
	NewGuiColor RGBToColor(uint8 r, uint8 g, uint8 b) { return 1; }
	bool show_mouse(bool visible) { return 0; }
	void warp_mouse(int x, int y) {}
	void set_mouse_cursor(uint32 id, const byte *buf, const byte* mask, uint w, uint h, int hotspot_x, int hotspot_y) {}
	uint32 get_msecs() { return 0; }
	void delay_msecs(uint msecs) {}
	void set_timer(TimerProc callback, int interval) {}
	bool poll_event(Event *event) { return false; }
	bool set_sound_proc(SoundProc proc, void *param, SoundFormat format) { return false; }
	void clear_sound_proc() {}
	bool poll_cdrom() { return false; }
	void play_cdrom(int track, int num_loops, int start_frame, int duration) {}
	void stop_cdrom() {}
	void update_cdrom() {}
	MutexRef create_mutex(void) { return (MutexRef)0; }
	void lock_mutex(MutexRef mutex) {}
	void unlock_mutex(MutexRef mutex) {};
	void delete_mutex(MutexRef mutex) {};
	
	void show_overlay() {}
	void hide_overlay() {}
	void clear_overlay() {}
	void grab_overlay(NewGuiColor *buf, int pitch) {}

	uint32 property(int param, Property *value);

	void quit() { exit(0); }
	static OSystem *create();

protected:
	OSystem_Converter() {}
	virtual ~OSystem_Converter() {}
};
