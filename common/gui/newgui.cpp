/* ScummVM - Scumm Interpreter
 * Copyright (C) 2002-2004 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /cvsroot/scummvm/scummvm/gui/newgui.cpp,v 1.77 2004/02/09 01:37:20 fingolfin Exp $
 */

#include "stdafx.h"
#include "common/util.h"
#include "newgui.h"
#include "dialog.h"

extern uint32 rptTable[256];

namespace GUI {

/*
 * TODO list
 * - get a nicer font which contains diacrits
 * - add more widgets: edit field, popup, radio buttons, ...
 *
 * Other ideas:
 * - allow multi line (l/c/r aligned) text via StaticTextWidget ?
 * - add "close" widget to all dialogs (with a flag to turn it off) ?
 * - make dialogs "moveable" ?
 * - come up with a new look & feel / theme for the GUI 
 * - ...
 */

enum {
	kDoubleClickDelay = 500, // milliseconds
	kCursorAnimateDelay = 250,
	kKeyRepeatInitialDelay = 400,
	kKeyRepeatSustainDelay = 100
};


// Built-in font
static const byte guifont[] = {
0,0,99,1,226,8,4,8,6,8,6,0,0,0,0,0,0,0,0,0,0,0,8,2,1,8,0,0,0,0,0,0,0,0,0,0,0,0,4,3,7,8,7,7,8,4,5,5,8,7,4,7,3,8,7,7,7,7,8,7,7,7,7,7,3,4,7,5,7,7,8,7,7,7,7,7,7,7,7,5,7,7,
7,8,7,7,7,7,7,7,7,7,7,8,7,7,7,5,8,5,8,8,7,7,7,6,7,7,7,7,7,5,6,7,5,8,7,7,7,7,7,7,7,7,7,8,7,7,7,5,3,5,7,8,7,7,7,7,7,7,0,6,7,7,7,5,5,5,7,0,6,8,8,7,7,7,7,7,0,7,7,0,0,
0,0,0,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,0,0,1,3,6,12,
24,62,3,0,128,192,96,48,24,124,192,0,0,3,62,24,12,6,3,1,0,192,124,24,48,96,192,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,237,74,72,0,0,0,0,0,128,128,128,0,0,0,0,0,0,0,0,0,0,0,0,0,60,66,153,161,161,153,66,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,96,96,96,96,0,0,96,0,102,102,102,0,0,0,0,0,102,102,255,102,255,102,102,0,24,62,96,60,6,124,24,0,98,102,12,24,48,102,70,0,60,102,60,56,103,102,63,0,96,48,16,0,0,0,0,0,24,48,96,96,96,48,24,0,96,48,24,24,24,48,96,0,
0,102,60,255,60,102,0,0,0,24,24,126,24,24,0,0,0,0,0,0,0,48,48,96,0,0,0,126,0,0,0,0,0,0,0,0,0,96,96,0,0,3,6,12,24,48,96,0,60,102,102,102,102,102,60,0,24,24,56,24,24,24,126,0,60,102,6,12,48,96,126,0,60,102,6,28,6,102,60,0,6,
14,30,102,127,6,6,0,126,96,124,6,6,102,60,0,60,102,96,124,102,102,60,0,126,102,12,24,24,24,24,0,60,102,102,60,102,102,60,0,60,102,102,62,6,102,60,0,0,0,96,0,0,96,0,0,0,0,48,0,0,48,48,96,14,24,48,96,48,24,14,0,0,0,120,0,120,0,0,0,112,24,
12,6,12,24,112,0,60,102,6,12,24,0,24,0,0,0,0,255,255,0,0,0,24,60,102,126,102,102,102,0,124,102,102,124,102,102,124,0,60,102,96,96,96,102,60,0,120,108,102,102,102,108,120,0,126,96,96,120,96,96,126,0,126,96,96,120,96,96,96,0,60,102,96,110,102,102,60,0,102,102,102,
126,102,102,102,0,120,48,48,48,48,48,120,0,30,12,12,12,12,108,56,0,102,108,120,112,120,108,102,0,96,96,96,96,96,96,126,0,99,119,127,107,99,99,99,0,102,118,126,126,110,102,102,0,60,102,102,102,102,102,60,0,124,102,102,124,96,96,96,0,60,102,102,102,102,60,14,0,124,102,102,124,
120,108,102,0,60,102,96,60,6,102,60,0,126,24,24,24,24,24,24,0,102,102,102,102,102,102,60,0,102,102,102,102,102,60,24,0,99,99,99,107,127,119,99,0,102,102,60,24,60,102,102,0,102,102,102,60,24,24,24,0,126,6,12,24,48,96,126,0,120,96,96,96,96,96,120,0,3,6,12,24,48,
96,192,0,120,24,24,24,24,24,120,0,0,0,0,0,0,219,219,0,0,0,0,0,0,0,0,255,102,102,102,0,0,0,0,0,0,0,60,6,62,102,62,0,0,96,96,124,102,102,124,0,0,0,60,96,96,96,60,0,0,6,6,62,102,102,62,0,0,0,60,102,126,96,60,0,0,14,24,62,24,24,
24,0,0,0,62,102,102,62,6,124,0,96,96,124,102,102,102,0,0,48,0,112,48,48,120,0,0,12,0,12,12,12,12,120,0,96,96,108,120,108,102,0,0,112,48,48,48,48,120,0,0,0,102,127,127,107,99,0,0,0,124,102,102,102,102,0,0,0,60,102,102,102,60,0,0,0,124,102,102,124,96,
96,0,0,62,102,102,62,6,6,0,0,124,102,96,96,96,0,0,0,62,96,60,6,124,0,0,24,126,24,24,24,14,0,0,0,102,102,102,102,62,0,0,0,102,102,102,60,24,0,0,0,99,107,127,62,54,0,0,0,102,60,24,60,102,0,0,0,102,102,102,62,12,120,0,0,126,12,24,48,126,0,
24,48,48,96,48,48,24,0,96,96,96,0,96,96,96,0,96,48,48,24,48,48,96,0,0,0,97,153,134,0,0,0,8,12,14,255,255,14,12,8,60,102,96,96,102,60,24,56,102,0,102,102,102,102,62,0,12,24,60,102,126,96,60,0,24,36,60,6,62,102,62,0,102,0,60,6,62,102,62,0,48,
24,60,6,62,102,62,0,0,0,0,0,0,0,0,0,0,60,96,96,96,60,24,56,24,36,60,102,126,96,60,0,102,0,60,102,126,96,60,0,48,24,60,102,126,96,60,0,0,216,0,112,48,48,120,0,48,72,0,112,48,48,120,0,96,48,0,112,48,48,120,0,102,24,60,102,126,102,102,0,0,0,
0,0,0,0,0,0,24,48,124,96,120,96,124,0,0,0,108,26,126,216,110,0,30,40,40,126,72,136,142,0,24,36,60,102,102,102,60,0,102,0,60,102,102,102,60,0,48,24,60,102,102,102,60,0,24,36,0,102,102,102,62,0,48,24,102,102,102,102,62,0,0,0,0,0,0,0,0,0,102,60,102,
102,102,102,60,0,102,0,102,102,102,102,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,24,60,6,62,102,62,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,28,54,54,124,102,102,124,64,0,0,0
};


#ifndef __ATARI__
static byte _cursor[2048];
#endif

// Constructor
NewGui::NewGui() : _screen(0), _needRedraw(false), _needClear(false), _stateIsSaved(false) {
	#ifndef __ATARI__
	memset(_cursor, 0xFF, sizeof(_cursor));
	for (int i = 0; i < 15; i++) {
		if ((i < 6) || (i > 8)) {
			_cursor[16 * 7 + i] = 15;
			_cursor[16 * i + 7] = 15;
		}
	}
	#endif
	_system = OSystem::instance();
	_dirtyMask.clear();
}

void NewGui::updateColors() {
	_bgcolor = _system->RGBToColor(64,128,196);
	_color = _system->RGBToColor(128,196,196);
	_shadowcolor = _system->RGBToColor(32,32,96);
	_textcolor = _system->RGBToColor(128,196,196);
	_textcolorhi = _system->RGBToColor(255, 255, 255);
}

void NewGui::runLoop() {
	Dialog *activeDialog = _dialogStack.top();
	bool didSaveState = false;

	if (activeDialog == 0)
		return;

	_dirtyMask.clear();

	// Setup some default GUI colors. Normally this will be done whenever an
	// EVENT_SCREEN_CHANGED is received. However, not yet all backends support
	// that event, so we also do it "manually" whenever a run loop is entered.
	updateColors();

	if (!_stateIsSaved) {
		saveState();
		didSaveState = true;
	}

	#ifdef __ATARI__
	_system->set_mouse_cursor(0, 0, 0, 16, 16, 7, 7);
	#else
	_system->set_mouse_cursor(0xFFFFFFFF, _cursor, 0, 16, 16, 7, 7);
	#endif

	while (!_dialogStack.empty() && activeDialog == _dialogStack.top())
	{
		activeDialog->handleTickle();
	
		if (_needRedraw)
		{
			flushDirtyRects();

			// Restore the overlay to its initial state, then draw all dialogs.
			// This is necessary to get the blending right.
			if (_needClear)
			{
				//_system->clear_overlay();
				_system->grab_overlay(_screen, SCREEN_WIDTH);
			}

			// ATARI: We have no blending, only draw the top dialog.
			//for (int i = 0; i < _dialogStack.size(); i++)
			//	_dialogStack[i]->drawDialog();
			
			if (_dialogStack.size() > 0)
				_dialogStack[_dialogStack.size()-1]->drawDialog();
			
			_dirtyMask.setRange(0, SCREEN_HEIGHT);
			_needRedraw = false;
		}

		flushDirtyRects();
		_system->update_screen();		

		OSystem::Event event;
		uint32 time = _system->get_msecs();

		int pollEvents = 5;
		while (pollEvents)
		{
			pollEvents = _system->poll_event(&event) ? pollEvents - 1 : 0;
			switch (event.event_code)
			{
			case OSystem::EVENT_KEYDOWN:
				activeDialog->handleKeyDown(event.kbd.ascii, event.kbd.keycode, event.kbd.flags);
				pollEvents = 0;
				break;
			case OSystem::EVENT_MOUSEMOVE:
				activeDialog->handleMouseMoved(event.mouse.x - activeDialog->_x, event.mouse.y - activeDialog->_y, 0);
				pollEvents = 0;
				break;
			// We don't distinguish between mousebuttons (for now at least)
			case OSystem::EVENT_LBUTTONDOWN:
			case OSystem::EVENT_RBUTTONDOWN: {
				if (_lastClick.count && (time < _lastClick.time + kDoubleClickDelay)
							&& ABS(_lastClick.x - event.mouse.x) < 3
							&& ABS(_lastClick.y - event.mouse.y) < 3) {
					_lastClick.count++;
				} else {
					_lastClick.x = event.mouse.x;
					_lastClick.y = event.mouse.y;
					_lastClick.count = 1;
				}
				_lastClick.time = time;
				}
				activeDialog->handleMouseDown(event.mouse.x - activeDialog->_x, event.mouse.y - activeDialog->_y, 1, _lastClick.count);
				break;
			case OSystem::EVENT_LBUTTONUP:
			case OSystem::EVENT_RBUTTONUP:
				activeDialog->handleMouseUp(event.mouse.x - activeDialog->_x, event.mouse.y - activeDialog->_y, 1, _lastClick.count);
				break;
			case OSystem::EVENT_WHEELUP:
				activeDialog->handleMouseWheel(event.mouse.x - activeDialog->_x, event.mouse.y - activeDialog->_y, -1);
				break;
			case OSystem::EVENT_WHEELDOWN:
				activeDialog->handleMouseWheel(event.mouse.x - activeDialog->_x, event.mouse.y - activeDialog->_y, 1);
				break;
			case OSystem::EVENT_QUIT:
				_system->quit();
				return;
			case OSystem::EVENT_SCREEN_CHANGED:
				updateColors();
				flushDirtyRects();
				break;
			default:
				break;
			}
		}

		// Delay for a moment
		_system->delay_msecs(1);
	}
	
	if (didSaveState)
		restoreState();
}

#pragma mark -

void NewGui::saveState() {

	// Backup old cursor
	_oldCursorMode = _system->show_mouse(true);

	// Enable the overlay
	_system->show_overlay();

	// Create a screen buffer for the overlay data, and fill it with
	// whatever is visible on the screen rught now.
	_screen = (NewGuiColor*)calloc(SCREEN_WIDTH * SCREEN_HEIGHT >> 1, 1);
	_lastClick.x = _lastClick.y = 0;
	_lastClick.time = 0;
	_lastClick.count = 0;

	_stateIsSaved = true;
}

void NewGui::restoreState() {
	_system->show_mouse(_oldCursorMode);

	_system->hide_overlay();
	if (_screen) {
		free(_screen);
		_screen = 0;
	}

	_system->update_screen();
	
	_stateIsSaved = false;
}

void NewGui::openDialog(Dialog *dialog) {
	_dialogStack.push(dialog);
	if (_dialogStack.size() > 2) {
		// options screen popups
		_needClear = false;
		_needRedraw = true;
	} else {
		_needClear = true;
		_needRedraw = true;
	}
}

void NewGui::closeTopDialog() {
	// Don't do anything if no dialog is open
	if (_dialogStack.empty())
		return;

	// Remove the dialog from the stack
	_dialogStack.pop();
	if (_dialogStack.size() > 2) {
		// options screen popups
		_needClear = false;
		_needRedraw = true;
	} else {
		_needClear = true;
		_needRedraw = true;
	}
}


#pragma mark -

NewGuiColor *NewGui::getBasePtr(int x, int y) {
	return _screen + x + MUL160(y);
}

void NewGui::box(int x, int y, int width, int height, NewGuiColor colorA, NewGuiColor colorB) {
	hLine(x + 1, y, x + width - 2, colorA);
	hLine(x, y + 1, x + width - 1, colorA);
	vLine(x, y + 1, y + height - 2, colorA);
	vLine(x + 1, y, y + height - 1, colorA);

	hLine(x + 1, y + height - 2, x + width - 1, colorB);
	hLine(x + 1, y + height - 1, x + width - 2, colorB);
	vLine(x + width - 1, y + 1, y + height - 2, colorB);
	vLine(x + width - 2, y + 1, y + height - 1, colorB);
}

void NewGui::line(int x, int y, int x2, int y2, NewGuiColor color) {

	if (x2 < x)
		SWAP(x2, x);

	if (y2 < y)
		SWAP(y2, y);

	if (x == x2) {
		// vertical line
		fillRect(x, y, 1, y2 - y, color);
	} else if (y == y2) {
		// horizontal line
		fillRect(x, y, x2 - x, 1, color);
	}

}

void NewGui::blendRect(int x, int y, int w, int h, NewGuiColor color, int level) {
	fillRect(x, y, w, h, color);
}

void NewGui::fillRect(int x, int y, int w, int h, NewGuiColor color) {

	const uint32 boxmask[8 * 8] = {
		0x80808080, 0xC0C0C0C0, 0xE0E0E0E0, 0xF0F0F0F0, 0xF8F8F8F8, 0xFCFCFCFC, 0xFEFEFEFE, 0xFFFFFFFF, // xbit 0
		0x40404040, 0x60606060, 0x70707070, 0x78787878, 0x7C7C7C7C, 0x7E7E7E7E, 0x7F7F7F7F, 0x7F7F7F7F, // xbit 1
		0x20202020, 0x30303030, 0x38383838, 0x3C3C3C3C, 0x3E3E3E3E, 0x3F3F3F3F, 0x3F3F3F3F, 0x3F3F3F3F, // xbit 2
		0x10101010, 0x18181818, 0x1C1C1C1C, 0x1E1E1E1E, 0x1F1F1F1F, 0x1F1F1F1F, 0x1F1F1F1F, 0x1F1F1F1F, // xbit 3
		0x08080808, 0x0C0C0C0C, 0x0E0E0E0E, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, // xbit 4
		0x04040404, 0x06060606, 0x07070707, 0x07070707, 0x07070707, 0x07070707, 0x07070707, 0x07070707, // xbit 5
		0x02020202, 0x03030303, 0x03030303, 0x03030303, 0x03030303, 0x03030303, 0x03030303, 0x03030303, // xbit 6
		0x01010101, 0x01010101, 0x01010101, 0x01010101, 0x01010101, 0x01010101, 0x01010101, 0x01010101, // xbit 7
	};

	if (w <= 0 || h <= 0)
		return;

	// left
	uint16 len = w;
	byte xbit = x & 7;
	byte bitcount = 8 - xbit;
	byte xlen = len > bitcount ? bitcount : len;
	len -= xlen;
	uint32 mask = boxmask[(xbit << 3) | (xlen - 1)];
	uint32 c = c2pfill16[color];

	uint32* dstart = (uint32*) _screen;
	dstart += MUL40(y) + (x >> 3);
	register uint32* d = dstart;
	register uint32 c32 = c & mask;
	register uint32 mask32 = ~mask;
	register int16 height = h;
	while (height > 7) 	{ d[0*40] = (d[0*40] & mask32) | c32; d[1*40] = (d[1*40] & mask32) | c32; d[2*40] = (d[2*40] & mask32) | c32; d[3*40] = (d[3*40] & mask32) | c32; d[4*40] = (d[4*40] & mask32) | c32; d[5*40] = (d[5*40] & mask32) | c32; d[6*40] = (d[6*40] & mask32) | c32; d[7*40] = (d[7*40] & mask32) | c32; d += (40 * 8); height-= 8; }
	while (height > 3) 	{ d[0*40] = (d[0*40] & mask32) | c32; d[1*40] = (d[1*40] & mask32) | c32; d[2*40] = (d[2*40] & mask32) | c32; d[3*40] = (d[3*40] & mask32) | c32; d += (40 * 4); height-= 4; }
	while (height != 0)	{ *d = ((*d & mask32) | c32); d += 40; height--; }

	// middle 
	c32 = c;
	while (len > 7)
	{
		dstart++;
		d = dstart;
		height = h;
		while (height > 7) 	{ d[0*40] = c32; d[1*40] = c32; d[2*40] = c32; d[3*40] = c32; d[4*40] = c32; d[5*40] = c32; d[6*40] = c32; d[7*40] = c32; d += (40 * 8); height-= 8; }
		while (height > 3) 	{ d[0*40] = c32; d[1*40] = c32; d[2*40] = c32; d[3*40] = c32; d += (40 * 4); height-= 4; }
		while(height != 0) { *d = c32; d+= 40; height--; }
		len-=8;
	}

	// right
	if (len != 0)
	{
		mask = boxmask[len-1];
		c32 = c & mask;
		mask32 = ~mask;
		dstart++;
		d = dstart;
		height = h;
		while (height > 7) 	{ d[0*40] = (d[0*40] & mask32) | c32; d[1*40] = (d[1*40] & mask32) | c32; d[2*40] = (d[2*40] & mask32) | c32; d[3*40] = (d[3*40] & mask32) | c32; d[4*40] = (d[4*40] & mask32) | c32; d[5*40] = (d[5*40] & mask32) | c32; d[6*40] = (d[6*40] & mask32) | c32; d[7*40] = (d[7*40] & mask32) | c32; d += (40 * 8); height-= 8; }
		while (height > 3) 	{ d[0*40] = (d[0*40] & mask32) | c32; d[1*40] = (d[1*40] & mask32) | c32; d[2*40] = (d[2*40] & mask32) | c32; d[3*40] = (d[3*40] & mask32) | c32; d += (40 * 4); height-= 4; }
		while (height != 0)	{ *d = (*d & mask32) | c32; d += 40; height--; }
	}
}

void NewGui::checkerRect(int x, int y, int w, int h, NewGuiColor color) {
	/*
	int i;
	NewGuiColor *ptr = getBasePtr(x, y);

	while (h--) {
		for (i = 0; i < w; i++) {
			if ((h ^ i) & 1)
				ptr[i] = color;
		}
		ptr += _screenPitch;
	}
	*/
}

void NewGui::frameRect(int x, int y, int w, int h, NewGuiColor color) {
	int x2 = x + w - 1;
	int y2 = y + h - 1;
	line(x,  y,  x2,  y, color);
	line(x,  y2, x2, y2, color);
	line(x,  y,  x,  y2, color);
	line(x2, y,  x2, y2, color);
}

void NewGui::addDirtyRect(int x, int y, int w, int h) {
	int16 tx = (x & ~7) >> 3;
	int16 tc = ((((x+w)+7)&~7) >> 3) - tx;
	if (tc > 0)
		_dirtyMask.updateRange(y, y+h, tx, tc);

}

void NewGui::flushDirtyRects()
{
	_system->copy_screen(_screen, 0, SCREEN_HEIGHT, _dirtyMask.maskBuf());
	_dirtyMask.clear();
}

void NewGui::drawChar(byte chr, int x, int y, NewGuiColor color) {
	register byte xbitl = x & 7;
	register byte fmask;
	register uint32 fmask32;
	register uint32 c32 = c2pfill16[color];
	register uint32* d = ((uint32*)_screen) + (x >> 3) + MUL40(y);
	register byte *tmp = (byte*)guifont + 6 + guifont[4] + (chr << 3);

	if (xbitl == 0)
	{
		fmask = *tmp++; fmask32 = FILLUINT32(fmask); *d = (c32 & fmask32) | (d[0] & ~fmask32); d+= (SCREEN_WIDTH >> 3);
		fmask = *tmp++; fmask32 = FILLUINT32(fmask); *d = (c32 & fmask32) | (d[0] & ~fmask32); d+= (SCREEN_WIDTH >> 3);
		fmask = *tmp++; fmask32 = FILLUINT32(fmask); *d = (c32 & fmask32) | (d[0] & ~fmask32); d+= (SCREEN_WIDTH >> 3);
		fmask = *tmp++; fmask32 = FILLUINT32(fmask); *d = (c32 & fmask32) | (d[0] & ~fmask32); d+= (SCREEN_WIDTH >> 3);
		fmask = *tmp++; fmask32 = FILLUINT32(fmask); *d = (c32 & fmask32) | (d[0] & ~fmask32); d+= (SCREEN_WIDTH >> 3);
		fmask = *tmp++; fmask32 = FILLUINT32(fmask); *d = (c32 & fmask32) | (d[0] & ~fmask32); d+= (SCREEN_WIDTH >> 3);
		fmask = *tmp++; fmask32 = FILLUINT32(fmask); *d = (c32 & fmask32) | (d[0] & ~fmask32); d+= (SCREEN_WIDTH >> 3);
		fmask = *tmp++; fmask32 = FILLUINT32(fmask); *d = (c32 & fmask32) | (d[0] & ~fmask32); d+= (SCREEN_WIDTH >> 3);
	}
	else
	{
		register byte xbitr = 8 - xbitl;
		fmask = *tmp >> xbitl; fmask32 = FILLUINT32(fmask); d[0] = (c32 & fmask32) | (d[0] & ~fmask32); fmask = *tmp++ << xbitr; fmask32 = FILLUINT32(fmask); d[1] = (c32 & fmask32) | (d[1] & ~fmask32); d+= (SCREEN_WIDTH >> 3);
		fmask = *tmp >> xbitl; fmask32 = FILLUINT32(fmask); d[0] = (c32 & fmask32) | (d[0] & ~fmask32); fmask = *tmp++ << xbitr; fmask32 = FILLUINT32(fmask); d[1] = (c32 & fmask32) | (d[1] & ~fmask32); d+= (SCREEN_WIDTH >> 3);
		fmask = *tmp >> xbitl; fmask32 = FILLUINT32(fmask); d[0] = (c32 & fmask32) | (d[0] & ~fmask32); fmask = *tmp++ << xbitr; fmask32 = FILLUINT32(fmask); d[1] = (c32 & fmask32) | (d[1] & ~fmask32); d+= (SCREEN_WIDTH >> 3);
		fmask = *tmp >> xbitl; fmask32 = FILLUINT32(fmask); d[0] = (c32 & fmask32) | (d[0] & ~fmask32); fmask = *tmp++ << xbitr; fmask32 = FILLUINT32(fmask); d[1] = (c32 & fmask32) | (d[1] & ~fmask32); d+= (SCREEN_WIDTH >> 3);
		fmask = *tmp >> xbitl; fmask32 = FILLUINT32(fmask); d[0] = (c32 & fmask32) | (d[0] & ~fmask32); fmask = *tmp++ << xbitr; fmask32 = FILLUINT32(fmask); d[1] = (c32 & fmask32) | (d[1] & ~fmask32); d+= (SCREEN_WIDTH >> 3);
		fmask = *tmp >> xbitl; fmask32 = FILLUINT32(fmask); d[0] = (c32 & fmask32) | (d[0] & ~fmask32); fmask = *tmp++ << xbitr; fmask32 = FILLUINT32(fmask); d[1] = (c32 & fmask32) | (d[1] & ~fmask32); d+= (SCREEN_WIDTH >> 3);
		fmask = *tmp >> xbitl; fmask32 = FILLUINT32(fmask); d[0] = (c32 & fmask32) | (d[0] & ~fmask32); fmask = *tmp++ << xbitr; fmask32 = FILLUINT32(fmask); d[1] = (c32 & fmask32) | (d[1] & ~fmask32); d+= (SCREEN_WIDTH >> 3);
		fmask = *tmp >> xbitl; fmask32 = FILLUINT32(fmask); d[0] = (c32 & fmask32) | (d[0] & ~fmask32); fmask = *tmp++ << xbitr; fmask32 = FILLUINT32(fmask); d[1] = (c32 & fmask32) | (d[1] & ~fmask32); d+= (SCREEN_WIDTH >> 3);
	}
}

int NewGui::getStringWidth(const String &str) {
	int space = 0;

	for (uint i = 0; i < str.size(); ++i)
		space += getCharWidth(str[i]);
	return space;
}

int NewGui::getCharWidth(byte c) {
	return guifont[c+6];
}

void NewGui::drawString(const String &s, int x, int y, int w, NewGuiColor color, int align, int deltax, bool useEllipsis) {

	const int leftX = x, rightX = x + w;
	uint i;
	int width = getStringWidth(s);
	String str;
	
	if (useEllipsis && width > w) {
		// String is too wide. So we shorten it "intellegently", by replacing
		// parts of it by an ellipsis ("..."). There are three possibilities
		// for this: replace the start, the end, or the middle of the string.
		// What is best really depends on the context; but unless we want to
		// make this configurable, replacing the middle probably is a good
		// compromise.
		const int ellipsisWidth = getStringWidth("...");
		
		// SLOW algorithm to remove enough of the middle. But it is good enough
		// for now.
		const int halfWidth = (w - ellipsisWidth) / 2;
		int w2 = 0;
		
		for (i = 0; i < s.size(); ++i) {
			int charWidth = getCharWidth(s[i]);
			if (w2 + charWidth > halfWidth)
				break;
			w2 += charWidth;
			str += s[i];
		}
		// At this point we know that the first 'i' chars are together 'w2'
		// pixels wide. We took the first i-1, and add "..." to them.
		str += "...";
		
		// The original string is width wide. Of those we already skipped past
		// w2 pixels, which means (width - w2) remain.
		// The new str is (w2+ellipsisWidth) wide, so we can accomodate about
		// (w - (w2+ellipsisWidth)) more pixels.
		// Thus we skip ((width - w2) - (w - (w2+ellipsisWidth))) =
		// (width + ellipsisWidth - w)
		int skip = width + ellipsisWidth - w;
		for (; i < s.size() && skip > 0; ++i) {
			skip -= getCharWidth(s[i]);
		}

		// Append the remaining chars, if any
		for (; i < s.size(); ++i) {
			str += s[i];
		}

		width = getStringWidth(str);

	} else {
		str = s;
	}

	if (align == kTextAlignCenter)
		x = x + (w - width - 1)/2;
	else if (align == kTextAlignRight)
		x = x + w - width;
	x += deltax;

	for (i = 0; i < str.size(); ++i) {
		w = getCharWidth(str[i]);
		if (x+w > rightX)
			break;
		if (x >= leftX)
			drawChar(str[i], x, y, color);
		x += w;
	}
}

//
// Draw an 8x8 bitmap at location (x,y)
//
void NewGui::drawBitmap(uint32 *bitmap, int x, int y, NewGuiColor color, int h) {
	/*
	NewGuiColor *ptr = getBasePtr(x, y);

	for (y = 0; y < h; y++) {
		uint32 mask = 0xF0000000;
		for (x = 0; x < 8; x++) {
			if (bitmap[y] & mask)
				ptr[x] = color;
			mask >>= 4;
		}
		ptr += _screenPitch;
	}
	*/
}


} // End of namespace GUI

