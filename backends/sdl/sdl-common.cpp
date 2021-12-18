/* ScummVM - Scumm Interpreter
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
 * $Header: /cvsroot/scummvm/scummvm/backends/sdl/Attic/sdl-common.cpp,v 1.106.2.1 2004/03/04 20:53:04 fingolfin Exp $
 *
 * Heavily butchered into a PC testbed for ScummST by Anders Granlund
 */

#include "backends/sdl/sdl-common.h"
#include "sound/mididrv.h"
//#include "common/config-manager.h"
#include "common/scaler.h"
#include "common/util.h"

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "scummvm.xpm"


// FIXME move joystick defines out and replace with confile file options
// we should really allow users to map any key to a joystick button
#define JOY_DEADZONE 3200
#define JOY_ANALOG
// #define JOY_INVERT_Y
#define JOY_XAXIS 0
#define JOY_YAXIS 1
// buttons
#define JOY_BUT_LMOUSE 0
#define JOY_BUT_RMOUSE 2
#define JOY_BUT_ESCAPE 3
#define JOY_BUT_PERIOD 1
#define JOY_BUT_SPACE 4
#define JOY_BUT_F5 5

OSystem *OSystem_SDL_create(int gfx_mode) {
	return OSystem_SDL_Common::create(gfx_mode);
}

OSystem *OSystem_SDL_Common::create(int gfx_mode) {
	OSystem_SDL_Common *syst = OSystem_SDL_Common::create_intern();

	syst->init_intern(gfx_mode);

	return syst;
}

void OSystem_SDL_Common::init_intern(int gfx_mode) {

	int joystick_num = 0;
	uint32 sdlFlags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;

	cksum_valid = false;
	_mode = gfx_mode;
	_full_screen = false;
	_adjustAspectRatio = false;
	_mode_flags = 0;

	if (joystick_num > -1)
		sdlFlags |= SDL_INIT_JOYSTICK;

	if (SDL_Init(sdlFlags) ==-1) {
		error("Could not initialize SDL: %s.\n", SDL_GetError());
	}

	_graphicsMutex = create_mutex();

	SDL_ShowCursor(SDL_DISABLE);
	
	// Enable unicode support if possible
	SDL_EnableUNICODE(1); 

#ifndef MACOSX		// Don't set icon on OS X, as we use a nicer external icon there
	// Setup the icon
	setup_icon();
#endif

	// enable joystick
	if (joystick_num > -1 && SDL_NumJoysticks() > 0) {
		printf("Using joystick: %s\n", SDL_JoystickName(0));
		init_joystick(joystick_num);
	}
}

void OSystem_SDL_Common::set_timer(TimerProc callback, int timer) {
	SDL_SetTimer(timer, (SDL_TimerCallback) callback);
}

OSystem_SDL_Common::OSystem_SDL_Common()
	: _screen(0), _screen_width(0), _screen_height(0),
	_tmpscreen(0), _tmpScreenWidth(0), _overlayVisible(false),
	_cdrom(0), _modeChanged(false), _dirty_checksums(0),
	_mouseVisible(false), _mouseDrawn(false), _mouseData(0),
	_mouseHotspotX(0), _mouseHotspotY(0),
	_currentShakePos(0), _newShakePos(0),
	_paletteDirtyStart(0), _paletteDirtyEnd(0),
	_graphicsMutex(0) {

	// allocate palette storage
	_currentPalette = (SDL_Color *)calloc(sizeof(SDL_Color), 256);

	// allocate the dirty rect storage
	_mouseBackup = (byte *)malloc(MAX_MOUSE_W * MAX_MOUSE_H * MAX_SCALING * 2);

	// reset mouse state
	memset(&km, 0, sizeof(km));
}

OSystem_SDL_Common::~OSystem_SDL_Common() {
//	unload_gfx_mode();

	if (_dirty_checksums)
		free(_dirty_checksums);
	free(_currentPalette);
	free(_mouseBackup);
	delete_mutex(_graphicsMutex);

	SDL_ShowCursor(SDL_ENABLE);
	SDL_Quit();
}

void OSystem_SDL_Common::init_size(uint w, uint h) {
	// Avoid redundant res changes
	if ((int)w == _screen_width && (int)h == _screen_height)
		return;

	_screen_width = w;
	_screen_height = h;

	if (h != 200)
		_adjustAspectRatio = false;

	CKSUM_NUM = (_screen_width * _screen_height / (8 * 8));
	if (_dirty_checksums)
		free(_dirty_checksums);
	_dirty_checksums = (uint32 *)calloc(CKSUM_NUM * 2, sizeof(uint32));

	unload_gfx_mode();
	load_gfx_mode();
}

void OSystem_SDL_Common::copy_rect(const byte *src, int pitch, int x, int y, int w, int h) {
	if (_screen == NULL)
		return;

	Common::StackLock lock(_graphicsMutex, this);	// Lock the mutex until this function ends
	
	// restrict to 8pixel strips
	x = x & ~7;
	w = (w + 7) & ~7;

	if (x < 0) {
		w += x;
		src -= x;
		x = 0;
	}

	if (y < 0) {
		h += y;
		src -= (y * pitch) >> 1;
		y = 0;
	}
	if (w > _screen_width - x) {
		w = _screen_width - x;
	}

	if (h > _screen_height - y) {
		h = _screen_height - y;
	}

	if (w <= 0 || h <= 0)
		return;

	cksum_valid = false;
	add_dirty_rect(x, y, w, h);

	/* FIXME: undraw mouse only if the draw rect intersects with the mouse rect */
	if (_mouseDrawn)
		undraw_mouse();

	// Try to lock the screen surface
	if (SDL_LockSurface(_screen) == -1)
		error("SDL_LockSurface failed: %s.\n", SDL_GetError());

	byte *dst = (byte *)_screen->pixels + y * _screen_width + x;

#if 1
	// atari planar to 8bit chunky
	uint32 numstrips = w >> 3;
	uint32 srcInc = (pitch - w) >> 1;
	uint32 dstInc = (_screen_width - w);
	do {
		for (uint32 i=0; i<numstrips; ++i)
		{
			byte s0 = *src++;
			byte s1 = *src++;
			byte s2 = *src++;
			byte s3 = *src++;

			for (int j=0; j<8; ++j)
			{
				*dst++ = (s0 >> 7) | ((s1 >> 6) & 2) | ((s2 >> 5) & 4) | ((s3 >> 4) & 8);
				s0 <<= 1; s1 <<= 1; s2 <<= 1; s3 <<= 1;
			}
		}
		dst += dstInc;
		src += srcInc;
	} while (--h);

#else
	do {
		for(int i=0; i<w; ++i)
			dst[i] = src[i] & 0xF;
		dst += _screen_width;
		src += pitch;
	} while (--h);
#endif
	
	// Unlock the screen surface
	SDL_UnlockSurface(_screen);
}

void OSystem_SDL_Common::copy_screen(const byte *src, int y, int h, const uint32* mask, bool doublebuffer)
{
	copy_rect(src, 320, 0, y, 320, h);
}

void OSystem_SDL_Common::move_screen(int dx, int dy, int height) {
}

void OSystem_SDL_Common::add_dirty_rect(int x, int y, int w, int h) {
	if (_forceFull)
		return;

	if (_num_dirty_rects == NUM_DIRTY_RECT)
		_forceFull = true;
	else {
		SDL_Rect *r = &_dirty_rect_list[_num_dirty_rects++];
		// Extend the dirty region by 1 pixel for scalers
		// that "smear" the screen, e.g. 2xSAI
		if (_mode_flags & DF_UPDATE_EXPAND_1_PIXEL) {
			x--;
			y--;
			w+=2;
			h+=2;
		}

		// clip
		if (x < 0) {
			w += x; x = 0;
		}

		if (y < 0) {
			h += y;
			y=0;
		}

		if (w > _screen_width - x) {
			w = _screen_width - x;
		}

		if (h > _screen_height - y) {
			h = _screen_height - y;
		}

/*
		if (_adjustAspectRatio)
			makeRectStretchable(x, y, w, h);
*/	
		r->x = x;
		r->y = y;
		r->w = w;
		r->h = h;
	}
}



void OSystem_SDL_Common::kbd_mouse() {
	uint32 curTime = get_msecs();
	if (curTime >= km.last_time + km.delay_time) {
		km.last_time = curTime;
		if (km.x_down_count == 1) {
			km.x_down_time = curTime;
			km.x_down_count = 2;
		}
		if (km.y_down_count == 1) {
			km.y_down_time = curTime;
			km.y_down_count = 2;
		}

		if (km.x_vel || km.y_vel) {
			if (km.x_down_count) {
				if (curTime > km.x_down_time + km.delay_time * 12) {
					if (km.x_vel > 0)
						km.x_vel++;
					else
						km.x_vel--;
				} else if (curTime > km.x_down_time + km.delay_time * 8) {
					if (km.x_vel > 0)
						km.x_vel = 5;
					else
						km.x_vel = -5;
				}
			}
			if (km.y_down_count) {
				if (curTime > km.y_down_time + km.delay_time * 12) {
					if (km.y_vel > 0)
						km.y_vel++;
					else
						km.y_vel--;
				} else if (curTime > km.y_down_time + km.delay_time * 8) {
					if (km.y_vel > 0)
						km.y_vel = 5;
					else
						km.y_vel = -5;
				}
			}

			km.x += km.x_vel;
			km.y += km.y_vel;

			if (km.x < 0) {
				km.x = 0;
				km.x_vel = -1;
				km.x_down_count = 1;
			} else if (km.x > km.x_max) {
				km.x = km.x_max;
				km.x_vel = 1;
				km.x_down_count = 1;
			}

			if (km.y < 0) {
				km.y = 0;
				km.y_vel = -1;
				km.y_down_count = 1;
			} else if (km.y > km.y_max) {
				km.y = km.y_max;
				km.y_vel = 1;
				km.y_down_count = 1;
			}

			SDL_WarpMouse(km.x, km.y);
		}
	}
}

bool OSystem_SDL_Common::show_mouse(bool visible) {
	if (_mouseVisible == visible)
		return visible;
	
	bool last = _mouseVisible;
	_mouseVisible = visible;

	if (visible)
		draw_mouse();
	else
		undraw_mouse();

	return last;
}

void OSystem_SDL_Common::set_mouse_pos(int x, int y) {
	if (x != _mouseCurState.x || y != _mouseCurState.y) {
		_mouseCurState.x = x;
		_mouseCurState.y = y;
		undraw_mouse();
		update_screen();
	}
}

void OSystem_SDL_Common::warp_mouse(int x, int y) {
	if (_mouseCurState.x != x || _mouseCurState.y != y) {
		SDL_WarpMouse(x * _scaleFactor, y * _scaleFactor);

		// SDL_WarpMouse() generates a mouse movement event, so
		// set_mouse_pos() would be called eventually. However, the
		// cannon script in CoMI calls this function twice each time
		// the cannon is reloaded. Unless we update the mouse position
		// immediately the second call is ignored, causing the cannon
		// to change its aim.

		set_mouse_pos(x, y);
	}
}
	
void OSystem_SDL_Common::set_mouse_cursor(uint32 id, const byte *buf, const byte* mask, uint w, uint h, int hotspot_x, int hotspot_y) {
	assert(w <= MAX_MOUSE_W);
	assert(h <= MAX_MOUSE_H);
	_mouseCurState.w = w;
	_mouseCurState.h = h;

	_mouseHotspotX = hotspot_x;
	_mouseHotspotY = hotspot_y;

	_mouseData = buf;

	undraw_mouse();
}

void OSystem_SDL_Common::set_shake_pos(int shake_pos) {
	_newShakePos = shake_pos;
}

uint32 OSystem_SDL_Common::get_msecs() {
	return SDL_GetTicks();	
}

void OSystem_SDL_Common::delay_msecs(uint msecs) {
	SDL_Delay(msecs);
}

static int mapKey(SDLKey key, SDLMod mod, Uint16 unicode)
{
	if (key >= SDLK_F1 && key <= SDLK_F9) {
		return key - SDLK_F1 + 315;
	} else if (key >= SDLK_KP0 && key <= SDLK_KP9) {
		return key - SDLK_KP0 + '0';
	} else if (key >= SDLK_UP && key <= SDLK_PAGEDOWN) {
		return key;
	} else if (unicode) {
		return unicode;
	} else if (key >= 'a' && key <= 'z' && mod & KMOD_SHIFT) {
		return key & ~0x20;
	} else if (key >= SDLK_NUMLOCK && key <= SDLK_EURO) {
		return 0;
	}
	return key;
}

void OSystem_SDL_Common::fillMouseEvent(Event &event, int x, int y) {
	event.mouse.x = x;
	event.mouse.y = y;
	
	// Update the "keyboard mouse" coords
	km.x = event.mouse.x;
	km.y = event.mouse.y;

	// Adjust for the screen scaling
	event.mouse.x /= _scaleFactor;
	event.mouse.y /= _scaleFactor;

	// Optionally perform aspect ratio adjusting
	if (_adjustAspectRatio)
		event.mouse.y = aspect2Real(event.mouse.y);
}

bool OSystem_SDL_Common::poll_event(Event *event) {
	SDL_Event ev;
	int axis;
	byte b = 0;
	
	kbd_mouse();
	
	// If the screen mode changed, send an EVENT_SCREEN_CHANGED
	if (_modeChanged) {
		_modeChanged = false;
		event->event_code = EVENT_SCREEN_CHANGED;
		return true;
	}

	while(SDL_PollEvent(&ev)) {
		switch(ev.type) {
		case SDL_KEYDOWN:
#ifdef LINUPY
			// Yopy has no ALT key, steal the SHIFT key 
			// (which isn't used much anyway)
			if (ev.key.keysym.mod & KMOD_SHIFT)
				b |= KBD_ALT;
			if (ev.key.keysym.mod & KMOD_CTRL)
				b |= KBD_CTRL;
#else
			if (ev.key.keysym.mod & KMOD_SHIFT)
				b |= KBD_SHIFT;
			if (ev.key.keysym.mod & KMOD_CTRL)
				b |= KBD_CTRL;
			if (ev.key.keysym.mod & KMOD_ALT)
				b |= KBD_ALT;
#endif
			event->kbd.flags = b;

			// Alt-Return toggles full screen mode				
			if (b == KBD_ALT && ev.key.keysym.sym == SDLK_RETURN) {
				property(PROP_TOGGLE_FULLSCREEN, NULL);
				break;
			}

			if (b == KBD_ALT && ev.key.keysym.sym == 's') {
				char filename[20];

				for (int n = 0;; n++) {
					SDL_RWops *file;

					sprintf(filename, "scummvm%05d.bmp", n);
					file = SDL_RWFromFile(filename, "r");
					if (!file)
						break;
					SDL_RWclose(file);
				}
				if (save_screenshot(filename))
					printf("Saved '%s'\n", filename);
				else
					printf("Could not save screenshot!\n");
				break;
			}

			// Ctrl-m toggles mouse capture
			if (b == KBD_CTRL && ev.key.keysym.sym == 'm') {
				property(PROP_TOGGLE_MOUSE_GRAB, NULL);
				break;
			}

#ifdef MACOSX
			// On Macintosh', Cmd-Q quits
			if ((ev.key.keysym.mod & KMOD_META) && ev.key.keysym.sym == 'q') {
				event->event_code = EVENT_QUIT;
				return true;
			}
#else
			// Ctrl-z and Alt-X quit
			if ((b == KBD_CTRL && ev.key.keysym.sym == 'z') || (b == KBD_ALT && ev.key.keysym.sym == 'x')) {
				event->event_code = EVENT_QUIT;
				return true;
			}
#endif

			// Ctrl-Alt-<key> will change the GFX mode
			if ((b & (KBD_CTRL|KBD_ALT)) == (KBD_CTRL|KBD_ALT)) {
				static const int gfxModes[][4] = {
						{ GFX_NORMAL, GFX_DOUBLESIZE, -1, -1 },
					};

				// FIXME EVIL HACK: This shouldn't be a static int, rather it
				// should be a member variable. Furthermore, it shouldn't be
				// set in this code, rather it should be set by load_gfx_mode().
				// But for now this quick&dirty hack works.
				static int _scalerType = 0;
				if (_mode != GFX_NORMAL) {
					// Try to figure out which gfx mode "group" we are in
					// This is just a temporary hack until the proper solution
					// (i.e. code in load_gfx_mode()) is in effect.
					for (int i = 0; i < ARRAYSIZE(gfxModes); i++) {
						if (gfxModes[i][1] == _mode || gfxModes[i][2] == _mode) {
							_scalerType = i;
							break;
						}
					}
				}
				

				Property prop;
				int factor = _scaleFactor - 1;

				// Ctrl-Alt-a toggles aspect ratio correction
				if (ev.key.keysym.sym == 'a') {
					property(PROP_TOGGLE_ASPECT_RATIO, NULL);
					break;
				}

				// Increase/decrease the scale factor
				// TODO: Shall we 'wrap around' here?
				if (ev.key.keysym.sym == '=' || ev.key.keysym.sym == '+' || ev.key.keysym.sym == '-') {
					factor += (ev.key.keysym.sym == '-' ? -1 : +1);
					if (0 <= factor && factor < 4 && gfxModes[_scalerType][factor] >= 0) {
						prop.gfx_mode = gfxModes[_scalerType][factor];
						property(PROP_SET_GFX_MODE, &prop);
					}
					break;
				}
				
				if ('1' <= ev.key.keysym.sym && ev.key.keysym.sym <= '9') {
					_scalerType = ev.key.keysym.sym - '1';
					if (_scalerType >= ARRAYSIZE(gfxModes))
						break;
					
					while (gfxModes[_scalerType][factor] < 0) {
						assert(factor > 0);
						factor--;
					}
					prop.gfx_mode = gfxModes[_scalerType][factor];
					property(PROP_SET_GFX_MODE, &prop);
					break;
				}
			}

#ifdef LINUPY
			// On Yopy map the End button to quit
			if ((ev.key.keysym.sym==293)) {
				event->event_code = EVENT_QUIT;
				return true;
			}
			// Map menu key to f5 (scumm menu)
			if (ev.key.keysym.sym==306) {
				event->event_code = EVENT_KEYDOWN;
				event->kbd.keycode = SDLK_F5;
				event->kbd.ascii = mapKey(SDLK_F5, ev.key.keysym.mod, 0);
				return true;
			}
			// Map action key to action
			if (ev.key.keysym.sym==291) {
				event->event_code = EVENT_KEYDOWN;
				event->kbd.keycode = SDLK_TAB;
				event->kbd.ascii = mapKey(SDLK_TAB, ev.key.keysym.mod, 0);
				return true;
			}
			// Map OK key to skip cinematic
			if (ev.key.keysym.sym==292) {
				event->event_code = EVENT_KEYDOWN;
				event->kbd.keycode = SDLK_ESCAPE;
				event->kbd.ascii = mapKey(SDLK_ESCAPE, ev.key.keysym.mod, 0);
				return true;
			}
#endif

#ifdef QTOPIA
			// quit on fn+backspace on zaurus
			if (ev.key.keysym.sym == 127) {
				event->event_code = EVENT_QUIT;
				return true;
			}

			// map menu key (f11) to f5 (scumm menu)
			if (ev.key.keysym.sym == SDLK_F11) {
				event->event_code = EVENT_KEYDOWN;
				event->kbd.keycode = SDLK_F5;
				event->kbd.ascii = mapKey(SDLK_F5, ev.key.keysym.mod, 0);
			}
			// map center (space) to tab (default action )
			// I wanted to map the calendar button but the calendar comes up
			//
			else if (ev.key.keysym.sym == SDLK_SPACE) {
				event->event_code = EVENT_KEYDOWN;
				event->kbd.keycode = SDLK_TAB;
				event->kbd.ascii = mapKey(SDLK_TAB, ev.key.keysym.mod, 0);
			}
			// since we stole space (pause) above we'll rebind it to the tab key on the keyboard
			else if (ev.key.keysym.sym == SDLK_TAB) {
				event->event_code = EVENT_KEYDOWN;
				event->kbd.keycode = SDLK_SPACE;
				event->kbd.ascii = mapKey(SDLK_SPACE, ev.key.keysym.mod, 0);
			} else {
			// let the events fall through if we didn't change them, this may not be the best way to
			// set it up, but i'm not sure how sdl would like it if we let if fall through then redid it though.
			// and yes i have an huge terminal size so i dont wrap soon enough.
				event->event_code = EVENT_KEYDOWN;
				event->kbd.keycode = ev.key.keysym.sym;
				event->kbd.ascii = mapKey(ev.key.keysym.sym, ev.key.keysym.mod, ev.key.keysym.unicode);
			}
#else
			event->event_code = EVENT_KEYDOWN;
			event->kbd.keycode = ev.key.keysym.sym;
			event->kbd.ascii = mapKey(ev.key.keysym.sym, ev.key.keysym.mod, ev.key.keysym.unicode);
#endif
			
			switch(ev.key.keysym.sym) {
			case SDLK_LEFT:
				km.x_vel = -1;
				km.x_down_count = 1;
				break;
			case SDLK_RIGHT:
				km.x_vel =  1;
				km.x_down_count = 1;
				break;
			case SDLK_UP:
				km.y_vel = -1;
				km.y_down_count = 1;
				break;
			case SDLK_DOWN:
				km.y_vel =  1;
				km.y_down_count = 1;
				break;
			default:
				break;
			}

			return true;
	
		case SDL_KEYUP:
			event->event_code = EVENT_KEYUP;
			event->kbd.keycode = ev.key.keysym.sym;
			event->kbd.ascii = mapKey(ev.key.keysym.sym, ev.key.keysym.mod, ev.key.keysym.unicode);

			switch(ev.key.keysym.sym) {
			case SDLK_LEFT:
				if (km.x_vel < 0) {
					km.x_vel = 0;
					km.x_down_count = 0;
				}
				break;
			case SDLK_RIGHT:
				if (km.x_vel > 0) {
					km.x_vel = 0;
					km.x_down_count = 0;
				}
				break;
			case SDLK_UP:
				if (km.y_vel < 0) {
					km.y_vel = 0;
					km.y_down_count = 0;
				}
				break;
			case SDLK_DOWN:
				if (km.y_vel > 0) {
					km.y_vel = 0;
					km.y_down_count = 0;
				}
				break;
			default:
				break;
			}
			return true;

		case SDL_MOUSEMOTION:
			event->event_code = EVENT_MOUSEMOVE;
			fillMouseEvent(*event, ev.motion.x, ev.motion.y);
			
			set_mouse_pos(event->mouse.x, event->mouse.y);
			return true;

		case SDL_MOUSEBUTTONDOWN:
			if (ev.button.button == SDL_BUTTON_LEFT)
				event->event_code = EVENT_LBUTTONDOWN;
			else if (ev.button.button == SDL_BUTTON_RIGHT)
				event->event_code = EVENT_RBUTTONDOWN;
#if defined(SDL_BUTTON_WHEELUP) && defined(SDL_BUTTON_WHEELDOWN)
			else if (ev.button.button == SDL_BUTTON_WHEELUP)
				event->event_code = EVENT_WHEELUP;
			else if (ev.button.button == SDL_BUTTON_WHEELDOWN)
				event->event_code = EVENT_WHEELDOWN;
#endif
			else
				break;

			fillMouseEvent(*event, ev.button.x, ev.button.y);

			return true;

		case SDL_MOUSEBUTTONUP:
			if (ev.button.button == SDL_BUTTON_LEFT)
				event->event_code = EVENT_LBUTTONUP;
			else if (ev.button.button == SDL_BUTTON_RIGHT)
				event->event_code = EVENT_RBUTTONUP;
			else
				break;
			fillMouseEvent(*event, ev.button.x, ev.button.y);

			return true;

		case SDL_JOYBUTTONDOWN:
			if (ev.jbutton.button == JOY_BUT_LMOUSE) {
				event->event_code = EVENT_LBUTTONDOWN;
			} else if (ev.jbutton.button == JOY_BUT_RMOUSE) {
				event->event_code = EVENT_RBUTTONDOWN;
			} else {
				event->event_code = EVENT_KEYDOWN;
				switch (ev.jbutton.button) {
					case JOY_BUT_ESCAPE:
						event->kbd.keycode = SDLK_ESCAPE;
						event->kbd.ascii = mapKey(SDLK_ESCAPE, ev.key.keysym.mod, 0);
						break;
					case JOY_BUT_PERIOD:
						event->kbd.keycode = SDLK_PERIOD;
						event->kbd.ascii = mapKey(SDLK_PERIOD, ev.key.keysym.mod, 0);
						break;
					case JOY_BUT_SPACE:
						event->kbd.keycode = SDLK_SPACE;
						event->kbd.ascii = mapKey(SDLK_SPACE, ev.key.keysym.mod, 0);
						break;
					case JOY_BUT_F5:
						event->kbd.keycode = SDLK_F5;
						event->kbd.ascii = mapKey(SDLK_F5, ev.key.keysym.mod, 0);
						break; 
				}
			}
			return true;

		case SDL_JOYBUTTONUP:
			if (ev.jbutton.button == JOY_BUT_LMOUSE) {
				event->event_code = EVENT_LBUTTONUP;
			} else if (ev.jbutton.button == JOY_BUT_RMOUSE) {
				event->event_code = EVENT_RBUTTONUP;
			} else {
				event->event_code = EVENT_KEYUP;
				switch (ev.jbutton.button) {
					case JOY_BUT_ESCAPE:
						event->kbd.keycode = SDLK_ESCAPE;
						event->kbd.ascii = mapKey(SDLK_ESCAPE, ev.key.keysym.mod, 0);
						break;
					case JOY_BUT_PERIOD:
						event->kbd.keycode = SDLK_PERIOD;
						event->kbd.ascii = mapKey(SDLK_PERIOD, ev.key.keysym.mod, 0);
						break;
					case JOY_BUT_SPACE:
						event->kbd.keycode = SDLK_SPACE;
						event->kbd.ascii = mapKey(SDLK_SPACE, ev.key.keysym.mod, 0);
						break;
					case JOY_BUT_F5:
						event->kbd.keycode = SDLK_F5;
						event->kbd.ascii = mapKey(SDLK_F5, ev.key.keysym.mod, 0);
						break;
				} 
			}
			return true;

		case SDL_JOYAXISMOTION:
			axis = ev.jaxis.value;
			if ( axis > JOY_DEADZONE) {
				axis -= JOY_DEADZONE;
				event->event_code = EVENT_MOUSEMOVE;
			} else if ( axis < -JOY_DEADZONE ) {
				axis += JOY_DEADZONE;
				event->event_code = EVENT_MOUSEMOVE;
			} else
				axis = 0;

			if ( ev.jaxis.axis == JOY_XAXIS) { 
#ifdef JOY_ANALOG
				km.x_vel = axis/2000;
				km.x_down_count = 0;
#else
				if (axis != 0) {
					km.x_vel = (axis > 0) ? 1:-1;
					km.x_down_count = 1;
				} else {
					km.x_vel = 0;
					km.x_down_count = 0;
				}
#endif

			} else if (ev.jaxis.axis == JOY_YAXIS) { 
#ifndef JOY_INVERT_Y
				axis = -axis;
#endif
#ifdef JOY_ANALOG
				km.y_vel = -axis / 2000;
				km.y_down_count = 0;
#else
				if (axis != 0) {
					km.y_vel = (-axis > 0) ? 1: -1;
					km.y_down_count = 1;
				} else {
					km.y_vel = 0;
					km.y_down_count = 0;
				}
#endif
			}
			event->mouse.x = km.x;
			event->mouse.y = km.y;
			event->mouse.x /= _scaleFactor;
			event->mouse.y /= _scaleFactor;

			if (_adjustAspectRatio)
				event->mouse.y = aspect2Real(event->mouse.y);

			return true;

		case SDL_VIDEOEXPOSE:
			_forceFull = true;
			break;

		case SDL_QUIT:
			event->event_code = EVENT_QUIT;
			return true;
		}
	}
	return false;
}

bool OSystem_SDL_Common::set_sound_proc(SoundProc proc, void *param, SoundFormat format) {
	SDL_AudioSpec desired;

	memset(&desired, 0, sizeof(desired));

	/* only one format supported at the moment */
	desired.freq = SAMPLES_PER_SEC;
	desired.format = AUDIO_U8; //AUDIO_S16SYS
	desired.channels = 1;
	desired.samples = 2048;
	desired.callback = proc;
	desired.userdata = param;
	if (SDL_OpenAudio(&desired, NULL) != 0) {
		return false;
	}
	SDL_PauseAudio(0);
	return true;
}

void OSystem_SDL_Common::clear_sound_proc() {
	SDL_CloseAudio();
}

uint32 OSystem_SDL_Common::property(int param, Property *value) {
	switch(param) {

	case PROP_WANT_RECT_OPTIM:
		_mode_flags |= DF_WANT_RECT_OPTIM;
		break;

	case PROP_GET_FULLSCREEN:
		return _full_screen;

	case PROP_GET_GFX_MODE:
		return _mode;

	case PROP_SET_WINDOW_CAPTION:
		SDL_WM_SetCaption(value->caption, value->caption);
		return 1;

	case PROP_OPEN_CD:
		if (SDL_InitSubSystem(SDL_INIT_CDROM) == -1)
			_cdrom = NULL;
		else {
			_cdrom = SDL_CDOpen(value->cd_num);
			// Did it open? Check if _cdrom is NULL
			if (!_cdrom) {
				warning("Couldn't open drive: %s", SDL_GetError());
			} else {
				cd_num_loops = 0;
				cd_stop_time = 0;
				cd_end_time = 0;
			}
		}
		break;

	case PROP_GET_SAMPLE_RATE:
		return SAMPLES_PER_SEC;

	case PROP_TOGGLE_MOUSE_GRAB:
		if (SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_OFF)
			SDL_WM_GrabInput(SDL_GRAB_ON);
		else
			SDL_WM_GrabInput(SDL_GRAB_OFF);
		break;
	}

	return 0;
}

void OSystem_SDL_Common::quit() {
	if(_cdrom) {
		SDL_CDStop(_cdrom);
		SDL_CDClose(_cdrom);
	}
	unload_gfx_mode();

	SDL_ShowCursor(SDL_ENABLE);
	SDL_Quit();

	exit(0);
}

void OSystem_SDL_Common::draw_mouse() {
	if (_mouseDrawn || !_mouseVisible)
		return;

	int x = _mouseCurState.x - _mouseHotspotX;
	int y = _mouseCurState.y - _mouseHotspotY;
	int w = _mouseCurState.w;
	int h = _mouseCurState.h;
	byte color;
	const byte *src = _mouseData;		// Image representing the mouse

	// clip the mouse rect, and addjust the src pointer accordingly
	if (x < 0) {
		w += x;
		src -= x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		src -= y * _mouseCurState.w;
		y = 0;
	}

	if (w > _screen_width - x)
		w = _screen_width - x;
	if (h > _screen_height - y)
		h = _screen_height - y;

	// Quick check to see if anything has to be drawn at all
	if (w <= 0 || h <= 0)
		return;

	// Store the bounding box so that undraw mouse can restore the area the
	// mouse currently covers to its original content.
	_mouseOldState.x = x;
	_mouseOldState.y = y;
	_mouseOldState.w = w;
	_mouseOldState.h = h;

	// Draw the mouse cursor; backup the covered area in "bak"
	if (SDL_LockSurface(_screen) == -1)
		error("SDL_LockSurface failed: %s.\n", SDL_GetError());

	// Mark as dirty
	add_dirty_rect(x, y, w, h);

	byte *bak = _mouseBackup;		// Surface used to backup the area obscured by the mouse
	byte *dst;					// Surface we are drawing into

	dst = (byte *)_screen->pixels + y * _screen_width + x;
	while (h > 0) {
		int width = w;
		while (width > 0) {
			*bak++ = *dst;
			color = *src++;
			if (color != 0xFF)	// 0xFF = transparent, don't draw
				*dst = color;
			dst++;
			width--;
		}
		src += _mouseCurState.w - w;
		bak += MAX_MOUSE_W - w;
		dst += _screen_width - w;
		h--;
	}

	SDL_UnlockSurface(_screen);

	// Finally, set the flag to indicate the mouse has been drawn
	_mouseDrawn = true;
}

void OSystem_SDL_Common::undraw_mouse() {
	if (!_mouseDrawn)
		return;
	_mouseDrawn = false;

	if (SDL_LockSurface(_screen) == -1)
		error("SDL_LockSurface failed: %s.\n", SDL_GetError());

	const int old_mouse_x = _mouseOldState.x;
	const int old_mouse_y = _mouseOldState.y;
	const int old_mouse_w = _mouseOldState.w;
	const int old_mouse_h = _mouseOldState.h;
	int x, y;

	byte *dst, *bak = _mouseBackup;

	// No need to do clipping here, since draw_mouse() did that already
	dst = (byte *)_screen->pixels + old_mouse_y * _screen_width + old_mouse_x;
	for (y = 0; y < old_mouse_h; ++y, bak += MAX_MOUSE_W, dst += _screen_width) {
		for (x = 0; x < old_mouse_w; ++x) {
			dst[x] = bak[x];
		}
	}


	add_dirty_rect(old_mouse_x, old_mouse_y, old_mouse_w, old_mouse_h);

	SDL_UnlockSurface(_screen);
}

void OSystem_SDL_Common::stop_cdrom() {	/* Stop CD Audio in 1/10th of a second */
	cd_stop_time = SDL_GetTicks() + 100;
	cd_num_loops = 0;
}

void OSystem_SDL_Common::play_cdrom(int track, int num_loops, int start_frame, int duration) {
	if (!num_loops && !start_frame)
		return;

	if (!_cdrom)
		return;
	
	if (duration > 0)
		duration += 5;

	cd_track = track;
	cd_num_loops = num_loops;
	cd_start_frame = start_frame;

	SDL_CDStatus(_cdrom);
	if (start_frame == 0 && duration == 0)
		SDL_CDPlayTracks(_cdrom, track, 0, 1, 0);
	else
		SDL_CDPlayTracks(_cdrom, track, start_frame, 0, duration);
	cd_duration = duration;
	cd_stop_time = 0;
	cd_end_time = SDL_GetTicks() + _cdrom->track[track].length * 1000 / CD_FPS;
}

bool OSystem_SDL_Common::poll_cdrom() {
	if (!_cdrom)
		return false;

	return (cd_num_loops != 0 && (SDL_GetTicks() < cd_end_time || SDL_CDStatus(_cdrom) != CD_STOPPED));
}

void OSystem_SDL_Common::update_cdrom() {
	if (!_cdrom)
		return;

	if (cd_stop_time != 0 && SDL_GetTicks() >= cd_stop_time) {
		SDL_CDStop(_cdrom);
		cd_num_loops = 0;
		cd_stop_time = 0;
		return;
	}

	if (cd_num_loops == 0 || SDL_GetTicks() < cd_end_time)
		return;

	if (cd_num_loops != 1 && SDL_CDStatus(_cdrom) != CD_STOPPED) {
		// Wait another second for it to be done
		cd_end_time += 1000;
		return;
	}

	if (cd_num_loops > 0)
		cd_num_loops--;

	if (cd_num_loops != 0) {
		if (cd_start_frame == 0 && cd_duration == 0)
			SDL_CDPlayTracks(_cdrom, cd_track, 0, 1, 0);
		else
			SDL_CDPlayTracks(_cdrom, cd_track, cd_start_frame, 0, cd_duration);
		cd_end_time = SDL_GetTicks() + _cdrom->track[cd_track].length * 1000 / CD_FPS;
	}
}

void OSystem_SDL_Common::setup_icon() {
	int w, h, ncols, nbytes, i;
	unsigned int rgba[256], icon[32 * 32];
	unsigned char mask[32][4];

	sscanf(scummvm_icon[0], "%d %d %d %d", &w, &h, &ncols, &nbytes);
	if ((w != 32) || (h != 32) || (ncols > 255) || (nbytes > 1)) {
		warning("Could not load the icon (%d %d %d %d)", w, h, ncols, nbytes);
		return;
	}
	for (i = 0; i < ncols; i++) {
		unsigned char code;
		char color[32];
		unsigned int col;
		sscanf(scummvm_icon[1 + i], "%c c %s", &code, color);
		if (!strcmp(color, "None"))
			col = 0x00000000;
		else if (!strcmp(color, "black"))
			col = 0xFF000000;
		else if (color[0] == '#') {
			sscanf(color + 1, "%06x", &col);
			col |= 0xFF000000;
		} else {
			warning("Could not load the icon (%d %s - %s) ", code, color, scummvm_icon[1 + i]);
			return;
		}
		
		rgba[code] = col;
	}
	memset(mask, 0, sizeof(mask));
	for (h = 0; h < 32; h++) {
		const char *line = scummvm_icon[1 + ncols + h];
		for (w = 0; w < 32; w++) {
			icon[w + 32 * h] = rgba[(int)line[w]];
			if (rgba[(int)line[w]] & 0xFF000000) {
				mask[h][w >> 3] |= 1 << (7 - (w & 0x07));
			}
		}
	}

	SDL_Surface *sdl_surf = SDL_CreateRGBSurfaceFrom(icon, 32, 32, 32, 32 * 4, 0xFF0000, 0x00FF00, 0x0000FF, 0xFF000000);
	SDL_WM_SetIcon(sdl_surf, (unsigned char *) mask);
	SDL_FreeSurface(sdl_surf);
}

OSystem::MutexRef OSystem_SDL_Common::create_mutex(void) {
	return (MutexRef) SDL_CreateMutex();
}

void OSystem_SDL_Common::lock_mutex(MutexRef mutex) {
	SDL_mutexP((SDL_mutex *) mutex);
}

void OSystem_SDL_Common::unlock_mutex(MutexRef mutex) {
	SDL_mutexV((SDL_mutex *) mutex);
}

void OSystem_SDL_Common::delete_mutex(MutexRef mutex) {
	SDL_DestroyMutex((SDL_mutex *) mutex);
}

void OSystem_SDL_Common::show_overlay() {
	// hide the mouse
	undraw_mouse();

	printf("show overlay\n\r");

	// backup game screen
	Common::StackLock lock(_graphicsMutex, this);	// Lock the mutex until this function ends

	SDL_LockSurface(_screen);
	SDL_LockSurface(_screenBak);
	byte *src = (byte *)_screen->pixels;
	byte *dst = (byte *)_screenBak->pixels;
	memcpy(dst, src, _screen_width * _screen_height);
	SDL_UnlockSurface(_screen);
	SDL_UnlockSurface(_screenBak);

	_overlayVisible = true;
}

void OSystem_SDL_Common::hide_overlay() {
	clear_overlay();
	_overlayVisible = false;
	_forceFull = true;
	clear_overlay();
}

void OSystem_SDL_Common::clear_overlay() {
	if (!_overlayVisible)
		return;
	
	Common::StackLock lock(_graphicsMutex, this);	// Lock the mutex until this function ends
	
	// hide the mouse
	undraw_mouse();

	// Clear the overlay by making the game screen "look through" everywhere.
	SDL_LockSurface(_screen);
	SDL_LockSurface(_screenBak);
	byte *src = (byte *)_screenBak->pixels;
	byte *dst = (byte *)_screen->pixels;
	memcpy(dst, src, _screen_width * _screen_height);
	SDL_UnlockSurface(_screen);
	SDL_UnlockSurface(_screenBak);

	_forceFull = true;
}

int16 OSystem_SDL_Common::get_height() {
	return _screen_height;
}

int16 OSystem_SDL_Common::get_width() {
	return _screen_width;
}

void OSystem_SDL_Common::grab_overlay(NewGuiColor *buf, int pitch) {
	if (!_overlayVisible)
		return;
/*
	Common::StackLock lock(_graphicsMutex, this);	// Lock the mutex until this function ends

	if (SDL_LockSurface(_screenBak) == -1)
		error("SDL_LockSurface failed: %s.\n", SDL_GetError());

	byte *src = (byte *)_screenBak->pixels;// + _ScreenWidth + 1;
	int h = _screen_height;
	do {
		memcpy(buf, src, _screen_width);
		src += _screen_width;
		buf += pitch;
	} while (--h);

	SDL_UnlockSurface(_screenBak);
*/	
}

void OSystem_SDL_Common::set_palette(const byte *colors, uint start, uint num) {
	const byte *b = colors;
	uint i;
	SDL_Color *base = _currentPalette + start;
	for (i = 0; i < num; i++) {
		base[i].r = b[0] & 0xE0;
		base[i].g = b[1] & 0xE0;
		base[i].b = b[2] & 0xE0;
		b += 4;
	}

	if (start < _paletteDirtyStart)
		_paletteDirtyStart = start;

	if (start + num > _paletteDirtyEnd)
		_paletteDirtyEnd = start + num;
}

NewGuiColor OSystem_SDL_Common::RGBToColor(uint8 r, uint8 g, uint8 b) {
	NewGuiColor bestitem = 0;
	uint bestsum = (uint) - 1;
	SDL_Color *base = _currentPalette;
	r &= ~3; g &= ~3; b &= ~3;
	for (NewGuiColor i = 0; i < 16; i++)
	{
		int ar = base[i].r & ~3;
		int ag = base[i].g & ~3;
		int ab = base[i].b & ~3;
		if (ar == r && ag == g && ab == b)
			return i;

		int r1 = ar - r;
		int r2 = r1 * r1;
		int g1 = ag - g;
		int g2 = g1 * g1;
		int b1 = ab - b;
		int b2 = b1 * b1;
		uint sum = (MUL3(r2) + MUL6(g2) + MUL2(b2));

		if (sum < bestsum) {
			bestsum = sum;
			bestitem = i;
		}
	}
	return bestitem;
}

