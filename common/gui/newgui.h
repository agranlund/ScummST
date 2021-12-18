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
 * $Header: /cvsroot/scummvm/scummvm/gui/newgui.h,v 1.37 2004/01/06 12:45:29 fingolfin Exp $
 */

#ifndef NEWGUI_H
#define NEWGUI_H

#include "common/scummsys.h"
#include "common/singleton.h"
#include "common/str.h"
#include "common/system.h"	// For events
#include "common/dirty.h"


// Height of a single text line
enum {
	kLineHeight			= 10
};

namespace GUI {

class Dialog;

#define hLine(x, y, x2, color) line(x, y, x2, y, color);
#define vLine(x, y, y2, color) line(x, y, x, y2, color);

#define g_gui	(GUI::NewGui::instance())


// Text alignment modes for drawString()
enum {
	kTextAlignLeft,
	kTextAlignCenter,
	kTextAlignRight
};

// Extremly simple stack class, doesn't even do any error checking (for now)
class DialogStack {
protected:
	Dialog	*_stack[10];	// Anybody nesting dialogs deeper than 4 is mad anyway
	int		_size;
public:
	DialogStack() : _size(0) {}
	
	bool	empty() const		{ return _size <= 0; }
	void	push(Dialog *d)		{ _stack[_size++] = d; }
	Dialog	*top() const		{ return _stack[_size - 1]; }
	void	pop()				{ if (_size > 0) _stack[--_size] = 0; }
	int		size() const		{ return _size; }
	Dialog	*operator [](int i)	{ return _stack[i]; }
};

/**
 * GUI manager singleton.
 */ 
class NewGui : public Common::Singleton<NewGui> {
	typedef Common::String String;
	friend class Dialog;
	friend class Common::Singleton<NewGui>;
	NewGui();
public:

	// Main entry for the GUI: this will start an event loop that keeps running
	// until no dialogs are active anymore.
	void runLoop();

	bool isActive()	{ return ! _dialogStack.empty(); }

protected:
	OSystem		*_system;
	NewGuiColor	*_screen;
	
	bool		_needRedraw;
	bool		_needClear;
	DialogStack	_dialogStack;
	
	bool		_stateIsSaved;
	
	Common::DirtyMask	_dirtyMask;
	
	// position and time of last mouse click (used to detect double clicks)
	struct {
		int16 x, y;	// Position of mouse when the click occured
		uint32 time;	// Time
		int count;	// How often was it already pressed?
	} _lastClick;
	
	// mouse cursor state
	bool		_oldCursorMode;

	void saveState();
	void restoreState();
	
	void openDialog(Dialog *dialog);
	void closeTopDialog();
	
	void loop();

	void updateColors();

public:
	// Theme colors
	NewGuiColor _color, _shadowcolor;
	NewGuiColor _bgcolor;
	NewGuiColor _textcolor;
	NewGuiColor _textcolorhi;

	// Drawing primitives
	NewGuiColor *getBasePtr(int x, int y);
	void box(int x, int y, int width, int height, NewGuiColor colorA, NewGuiColor colorB);
	void line(int x, int y, int x2, int y2, NewGuiColor color);
	void blendRect(int x, int y, int w, int h, NewGuiColor color, int level = 3);
	void fillRect(int x, int y, int w, int h, NewGuiColor color);
	void checkerRect(int x, int y, int w, int h, NewGuiColor color);
	void frameRect(int x, int y, int w, int h, NewGuiColor color);
	void drawChar(byte c, int x, int y, NewGuiColor color);
	int getStringWidth(const String &str);
	int getCharWidth(byte c);
	void drawString(const String &str, int x, int y, int w, NewGuiColor color, int align = kTextAlignLeft, int deltax = 0, bool useEllipsis = true);

	void drawBitmap(uint32 *bitmap, int x, int y, NewGuiColor color, int h = 8);

	void addDirtyRect(int x, int y, int w, int h);
	void flushDirtyRects();
};

} // End of namespace GUI

#endif
