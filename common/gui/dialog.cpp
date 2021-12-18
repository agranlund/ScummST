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
 * $Header: /cvsroot/scummvm/scummvm/gui/dialog.cpp,v 1.43 2004/01/06 12:45:29 fingolfin Exp $
 */

#include <ctype.h>

#include "stdafx.h"
#include "newgui.h"
#include "dialog.h"
#include "widget.h"

namespace GUI {

/*
 * TODO list
 * - add some sense of the window being "active" (i.e. in front) or not. If it 
 *   was inactive and just became active, reset certain vars (like who is focused).
 *   Maybe we should just add lostFocus and receivedFocus methods to Dialog, just
 *   like we have for class Widget?
 * ...
 */

Dialog::~Dialog() {
	delete _firstWidget;
	_firstWidget = 0;
}

int Dialog::runModal() {
	// Open up
	open();

	// Start processing events
	g_gui.runLoop();

	// Return the result code
	return _result;
}

void Dialog::open() {
	Widget *w = _firstWidget;

	_result = 0;
	_visible = true;
	g_gui.openDialog(this);

	// Search for the first objects that wantsFocus() (if any) and give it the focus
	while (w && !w->wantsFocus()) {
		w = w->_next;
	}

	if (w) {
		w->receivedFocus();
		_focusedWidget = w;
	}
}

void Dialog::close() {
	_visible = false;
	g_gui.closeTopDialog();

	if (_mouseWidget) {
		_mouseWidget->handleMouseLeft(0);
		_mouseWidget = 0;
	}
	releaseFocus();
}

void Dialog::releaseFocus() {
	if (_focusedWidget) {
		_focusedWidget->lostFocus();
		_focusedWidget = 0;
	}
}

void Dialog::draw() {
	g_gui._needRedraw = true;
}

void Dialog::drawDialog() {
	
	if (!isVisible())
		return;

	g_gui.blendRect(_x, _y, _w, _h, g_gui._bgcolor);
	g_gui.box(_x, _y, _w, _h, g_gui._color, g_gui._shadowcolor);

	// Draw all children
	Widget *w = _firstWidget;
	while (w) {
		w->draw();
		w = w->_next;
	}

	// Flag the draw area as dirty
	g_gui.addDirtyRect(_x, _y, _w, _h);
}

void Dialog::handleMouseDown(int x, int y, int button, int clickCount) {
	Widget *w;
	w = findWidget(x, y);

	// If the click occured inside a widget which is not the currently
	// focused one, change the focus to that widget.
	// TODO: use the wantsFocus() method to objects, so that only fields
	// that want it get the focus (like edit fields, list field...)
	// However, right now we "abuse" the focus also for the click&drag
	// behaviour of buttons. This should probably be changed by adding
	// a nother field, e.g. _clickedWidget or _dragWidget.
	if (w && w != _focusedWidget) {
		// The focus will change. Tell the old focused widget (if any)
		// that it lost the focus.
		releaseFocus();

		// Tell the new focused widget (if any) that it just gained the focus.
		if (w)
			w->receivedFocus();

		_focusedWidget = w;
	}

	if (w && w == _focusedWidget)
		_focusedWidget->handleMouseDown(x - (_focusedWidget->getAbsX() - _x), y - (_focusedWidget->getAbsY() - _y), button, clickCount);
}

void Dialog::handleMouseUp(int x, int y, int button, int clickCount) {
	Widget *w;

	if (_focusedWidget) {
		w = _focusedWidget;
		
		// Lose focus on mouseup unless the widget requested to retain the focus
		if (! (_focusedWidget->getFlags() & WIDGET_RETAIN_FOCUS )) {
			releaseFocus();
		}

	} else {
		w = findWidget(x, y);
	}

	if (w)
		w->handleMouseUp(x - (w->getAbsX() - _x), y - (w->getAbsY() - _y), button, clickCount);
}

void Dialog::handleMouseWheel(int x, int y, int direction) {
	Widget *w;

	// This may look a bit backwards, but I think it makes more sense for
	// the mouse wheel to primarily affect the widget the mouse is at than
	// the widget that happens to be focused.

	w = findWidget(x, y);
	if (!w)
		w = _focusedWidget;
	if (w)
		w->handleMouseWheel(x, y, direction);
}

void Dialog::handleKeyDown(uint16 ascii, int keycode, int modifiers) {
	if (_focusedWidget) {
		if (_focusedWidget->handleKeyDown(ascii, keycode, modifiers))
			return;
	}

	// Hotkey handling
	if (ascii != 0) {
		Widget *w = _firstWidget;
		ascii = toupper(ascii);
		while (w) {
			if (w->_type == kButtonWidget && ascii == toupper(((ButtonWidget *)w)->_hotkey)) {
				// The hotkey for widget w was pressed. We fake a mouse click into the
				// button by invoking the appropriate methods.
				w->handleMouseDown(0, 0, 1, 1);
				w->handleMouseUp(0, 0, 1, 1);
				return;
			}
			w = w->_next;
		}
	}

	// ESC closes all dialogs by default
	if (keycode == 27) {
		setResult(-1);
		close();
	}

	// TODO: tab/shift-tab should focus the next/previous focusable widget
}

void Dialog::handleMouseMoved(int x, int y, int button) {
	Widget *w;
	
	if (_focusedWidget) {
		w = _focusedWidget;
		int wx = w->getAbsX() - _x;
		int wy = w->getAbsY() - _y;
		
		// We still send mouseEntered/Left messages to the focused item
		// (but to no other items).
		bool mouseInFocusedWidget = (x >= wx && x < wx + w->_w && y >= wy && y < wy + w->_h);
		if (mouseInFocusedWidget && _mouseWidget != w) {
			_mouseWidget = w;
			w->handleMouseEntered(button);
		} else if (!mouseInFocusedWidget && _mouseWidget == w) {
			_mouseWidget = 0;
			w->handleMouseLeft(button);
		}

		w->handleMouseMoved(x - wx, y - wy, button);
	}

	w = findWidget(x, y);

	if (_mouseWidget != w) {
		if (_mouseWidget)
			_mouseWidget->handleMouseLeft(button);
		if (w)
			w->handleMouseEntered(button);
		_mouseWidget = w;
	} 

	if (!w || !(w->getFlags() & WIDGET_TRACK_MOUSE)) {
		return;
	}

	w->handleMouseMoved(x - (w->getAbsX() - _x), y - (w->getAbsY() - _y), button);
}

void Dialog::handleTickle() {
	// Focused widget receives tickle notifications
	if (_focusedWidget && _focusedWidget->getFlags() & WIDGET_WANT_TICKLE) {
		_focusedWidget->handleTickle();
	}
}

void Dialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	switch (cmd) {
	case kCloseCmd:
		close();
		break;
	}
}

/*
 * Determine the widget at location (x,y) if any. Assumes the coordinates are
 * in the local coordinate system, i.e. relative to the top left of the dialog.
 */
Widget *Dialog::findWidget(int x, int y) {
	return Widget::findWidgetInChain(_firstWidget, x, y);
}

ButtonWidget *Dialog::addButton(int x, int y, const Common::String &label, uint32 cmd, char hotkey) {
	return new ButtonWidget(this, x, y, kButtonWidth, 16, label, cmd, hotkey);
}

} // End of namespace GUI
