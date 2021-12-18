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
 * $Header: /cvsroot/scummvm/scummvm/backends/sdl/sdl.cpp,v 1.56.2.2 2004/02/28 13:01:43 fingolfin Exp $
 *
 * Heavily butchered into a PC testbed for ScummST by Anders Granlund
 */

#include "sdl-common.h"
#include "common/scaler.h"
#include "common/util.h"

class OSystem_SDL : public OSystem_SDL_Common {
public:
	OSystem_SDL();

	// Update the dirty areas of the screen
	void update_screen();

	// Set a parameter
	uint32 property(int param, Property *value);

protected:
	SDL_Surface *_hwscreen;    // hardware screen

	ScalerProc *_scaler_proc;

	virtual void load_gfx_mode();
	virtual void unload_gfx_mode();
	virtual bool save_screenshot(const char *filename);
	void hotswap_gfx_mode();
};

OSystem_SDL_Common *OSystem_SDL_Common::create_intern() {
	return new OSystem_SDL();
}

OSystem_SDL::OSystem_SDL()
	 : _hwscreen(0), _scaler_proc(0)
{
}

void OSystem_SDL::load_gfx_mode() {
	_forceFull = true;
	_mode_flags |= DF_UPDATE_EXPAND_1_PIXEL;

	_tmpscreen = NULL;
	_tmpScreenWidth = (SCREEN_WIDTH + 3);
	
	switch(_mode) {
	case GFX_NORMAL:
		_scaleFactor = 1;
		_scaler_proc = Normal1x;
		break;
	case GFX_DOUBLESIZE:
		_scaleFactor = 2;
		_scaler_proc = Normal2x;
		break;
	default:
		error("unknown gfx mode %d", _mode);
	}

	//
	// Create the surface that contains the 8 bit game data
	//
	_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH, SCREEN_HEIGHT, 8, 0, 0, 0, 0);
	if (_screen == NULL)
		error("_screen failed");

	_screenBak = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH, SCREEN_HEIGHT, 8, 0, 0, 0, 0);
	if (_screenBak == NULL)
		error("_screenBak failed");

	//
	// Create the surface that contains the scaled graphics in 16 bit mode
	//

	_hwscreen = SDL_SetVideoMode(SCREEN_WIDTH * _scaleFactor, (_adjustAspectRatio ? 240 : SCREEN_HEIGHT) * _scaleFactor, 16, 
		_full_screen ? (SDL_FULLSCREEN|SDL_SWSURFACE) : SDL_SWSURFACE
	);
	if (_hwscreen == NULL) {
		// DON'T use error(), as this tries to bring up the debug
		// console, which WON'T WORK now that _hwscreen is hosed.

		// FIXME: We should be able to continue the game without
		// shutting down or bringing up the debug console, but at
		// this point we've already screwed up all our member vars.
		// We need to find a way to call SDL_VideoModeOK *before*
		// that happens and revert to all the old settings if we
		// can't pull off the switch to the new settings.
		//
		// Fingolfin says: the "easy" way to do that is not to modify
		// the member vars before we are sure everything is fine. Think
		// of "transactions, commit, rollback" style... we use local vars
		// in place of the member vars, do everything etc. etc.. In case
		// of a failure, rollback is trivial. Only if everything worked fine
		// do we "commit" the changed values to the member vars.
		warning("SDL_SetVideoMode says we can't switch to that mode");
		quit();
	}

	//
	// Create the surface used for the graphics in 16 bit before scaling, and also the overlay
	//

	// Distinguish 555 and 565 mode
	if (_hwscreen->format->Rmask == 0x7C00)
		InitScalers(555);
	else
		InitScalers(565);
	
	// Need some extra bytes around when using 2xSaI
	uint16 *tmp_screen = (uint16 *)calloc(_tmpScreenWidth * (SCREEN_HEIGHT + 3), sizeof(uint16));
	_tmpscreen = SDL_CreateRGBSurfaceFrom(tmp_screen,
						_tmpScreenWidth, SCREEN_HEIGHT + 3, 16, _tmpScreenWidth * 2,
						_hwscreen->format->Rmask,
						_hwscreen->format->Gmask,
						_hwscreen->format->Bmask,
						_hwscreen->format->Amask);

	if (_tmpscreen == NULL)
		error("_tmpscreen failed");

	// keyboard cursor control, some other better place for it?
	km.x_max = SCREEN_WIDTH * _scaleFactor - 1;
	km.y_max = (_adjustAspectRatio ? 240 : SCREEN_HEIGHT) * _scaleFactor - 1;
	km.delay_time = 25;
	km.last_time = 0;
}

void OSystem_SDL::unload_gfx_mode() {
	if (_screen) {
		SDL_FreeSurface(_screen);
		_screen = NULL; 
	}

	if (_hwscreen) {
		SDL_FreeSurface(_hwscreen); 
		_hwscreen = NULL;
	}

	if (_tmpscreen) {
		free(_tmpscreen->pixels);
		SDL_FreeSurface(_tmpscreen);
		_tmpscreen = NULL;
	}
}

void OSystem_SDL::hotswap_gfx_mode() {
	if (!_screen)
		return;

	// Keep around the old _screen & _tmpscreen so we can restore the screen data
	// after the mode switch.
	SDL_Surface *old_screen = _screen;
	SDL_Surface *old_tmpscreen = _tmpscreen;

	// Release the HW screen surface
	SDL_FreeSurface(_hwscreen); 

	// Setup the new GFX mode
	load_gfx_mode();

	// reset palette
	SDL_SetColors(_screen, _currentPalette, 0, 256);

	// Restore old screen content
	SDL_BlitSurface(old_screen, NULL, _screen, NULL);
	SDL_BlitSurface(old_tmpscreen, NULL, _tmpscreen, NULL);
	
	// Free the old surfaces
	SDL_FreeSurface(old_screen);
	free(old_tmpscreen->pixels);
	SDL_FreeSurface(old_tmpscreen);

	// Blit everything to the screen
	update_screen();
	
	// Make sure that an EVENT_SCREEN_CHANGED gets sent later
	_modeChanged = true;
}

void OSystem_SDL::update_screen() {
	assert(_hwscreen != NULL);

	Common::StackLock lock(_graphicsMutex, this);	// Lock the mutex until this function ends
/*
	// If the shake position changed, fill the dirty area with blackness
	if (_currentShakePos != _newShakePos) {
		SDL_Rect blackrect;
		blackrect.x = 0;
		blackrect.y = 0;
		blackrect.w = SCREEN_WIDTH * _scaleFactor;
		blackrect.h = _newShakePos * _scaleFactor;

		if (_adjustAspectRatio)
			blackrect.h = real2Aspect(blackrect.h - 1) + 1;

		SDL_FillRect(_hwscreen, &blackrect, 0);

		_currentShakePos = _newShakePos;

		_forceFull = true;
	}
*/
	// Make sure the mouse is drawn, if it should be drawn.
	draw_mouse();
	
	// Check whether the palette was changed in the meantime and update the
	// screen surface accordingly. 
	if (_paletteDirtyEnd != 0) {
		SDL_SetColors(_screen, _currentPalette + _paletteDirtyStart, 
			_paletteDirtyStart,
			_paletteDirtyEnd - _paletteDirtyStart);
		
		_paletteDirtyEnd = 0;

		_forceFull = true;
	}

	// Force a full redraw if requested
	if (_forceFull) {
		_num_dirty_rects = 1;

		_dirty_rect_list[0].x = 0;
		_dirty_rect_list[0].y = 0;
		_dirty_rect_list[0].w = SCREEN_WIDTH;
		_dirty_rect_list[0].h = SCREEN_HEIGHT;
	}

	// Only draw anything if necessary
	if (_num_dirty_rects > 0) {

		SDL_Rect *r; 
		SDL_Rect dst;
		uint32 srcPitch, dstPitch;
		SDL_Rect *last_rect = _dirty_rect_list + _num_dirty_rects;

		if (_scaler_proc == Normal1x && !_adjustAspectRatio)
		{
			SDL_Surface *target = _screen;
			for (r = _dirty_rect_list; r != last_rect; ++r) {
				dst = *r;
				/*
				if (_overlayVisible) {
					// FIXME: I don't understand why this is necessary...
					dst.x--;
					dst.y--;
				}
				*/
				dst.y += _currentShakePos;
				if (SDL_BlitSurface(target, r, _hwscreen, &dst) != 0)
					error("SDL_BlitSurface failed: %s", SDL_GetError());
			}
		}
		else
		{
			for (r = _dirty_rect_list; r != last_rect; ++r) {
				dst = *r;
				dst.x++;	// Shift rect by one since 2xSai needs to acces the data around
				dst.y++;	// any pixel to scale it, and we want to avoid mem access crashes.
				if (SDL_BlitSurface(_screen, r, _tmpscreen, &dst) != 0)
					error("SDL_BlitSurface failed: %s", SDL_GetError());
			}

			SDL_LockSurface(_tmpscreen);
			SDL_LockSurface(_hwscreen);

			srcPitch = _tmpscreen->pitch;
			dstPitch = _hwscreen->pitch;

			for (r = _dirty_rect_list; r != last_rect; ++r) {
				register int dst_y = r->y + _currentShakePos;
				register int dst_h = 0;
				register int orig_dst_y = 0;

				if (dst_y < SCREEN_HEIGHT) {
					dst_h = r->h;
					if (dst_h > SCREEN_HEIGHT - dst_y)
						dst_h = SCREEN_HEIGHT - dst_y;

					dst_y *= _scaleFactor;

					if (_adjustAspectRatio) {
						orig_dst_y = dst_y;
						dst_y = real2Aspect(dst_y);
					}

				
					_scaler_proc((byte *)_tmpscreen->pixels + (r->x * 2 + 2) + (r->y + 1) * srcPitch, srcPitch,
						(byte *)_hwscreen->pixels + r->x * 2 * _scaleFactor + dst_y * dstPitch, dstPitch, r->w, dst_h);
				}

				r->x *= _scaleFactor;
				r->y = dst_y;
				r->w *= _scaleFactor;
				r->h = dst_h * _scaleFactor;

				/*
				if (_adjustAspectRatio && orig_dst_y / _scaleFactor < SCREEN_HEIGHT)
					r->h = stretch200To240((uint8 *) _hwscreen->pixels, dstPitch, r->w, r->h, r->x, r->y, orig_dst_y);
				*/
			}
			SDL_UnlockSurface(_tmpscreen);
			SDL_UnlockSurface(_hwscreen);
		}

		// Readjust the dirty rect list in case we are doing a full update.
		// This is necessary if shaking is active.
		if (_forceFull) {
			_dirty_rect_list[0].y = 0;
			_dirty_rect_list[0].h = (_adjustAspectRatio ? 240 : SCREEN_HEIGHT) * _scaleFactor;
		}

		// Finally, blit all our changes to the screen
		SDL_UpdateRects(_hwscreen, _num_dirty_rects, _dirty_rect_list);
	}

	_num_dirty_rects = 0;
	_forceFull = false;
}

uint32 OSystem_SDL::property(int param, Property *value) {

	Common::StackLock lock(_graphicsMutex, this);	// Lock the mutex until this function ends

	if (param == PROP_TOGGLE_FULLSCREEN) {
		assert(_hwscreen != 0);
		_full_screen ^= true;

		if (_mouseDrawn)
			undraw_mouse();

#ifdef MACOSX
		// On OS X, SDL_WM_ToggleFullScreen is currently not implemented. Worse,
		// it still always returns -1. So we simply don't call it at all and
		// use hotswap_gfx_mode() directly to switch to fullscreen mode.
		hotswap_gfx_mode();
#else
		if (!SDL_WM_ToggleFullScreen(_hwscreen)) {
			// if ToggleFullScreen fails, achieve the same effect with hotswap gfx mode
			hotswap_gfx_mode();
		}
#endif
		return 1;
	} else if (param == PROP_SET_GFX_MODE) {
		if (value->gfx_mode > 11)	// FIXME! HACK, hard coded threshold, not good
			return 0;

		_mode = value->gfx_mode;
		hotswap_gfx_mode();

		return 1;
	} else if (param == PROP_TOGGLE_ASPECT_RATIO) {
		if (SCREEN_HEIGHT == 200) {
			assert(_hwscreen != 0);
			_adjustAspectRatio ^= true;
			hotswap_gfx_mode();
		}
	} else if (param == PROP_HAS_SCALER) {
		if (value->gfx_mode <= 11)	// FIXME: Hardcoded
			return 1;
		return 0;
	}

	return OSystem_SDL_Common::property(param, value);
}

bool OSystem_SDL::save_screenshot(const char *filename) {
	assert(_hwscreen != NULL);

	Common::StackLock lock(_graphicsMutex, this);	// Lock the mutex until this function ends
	SDL_SaveBMP(_hwscreen, filename);
	return true;
}
