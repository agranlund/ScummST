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
 * $Header: /cvsroot/scummvm/scummvm/scumm/verbs.cpp,v 1.93 2004/01/16 10:20:43 kirben Exp $
 *
 */

#include "stdafx.h"
#include "scumm/charset.h"
#include "scumm/object.h"
#include "scumm/resource.h"
#include "scumm/scumm.h"
#include "scumm/verbs.h"

namespace Scumm {

enum {
	kInventoryUpArrow = 4,
	kInventoryDownArrow = 5,
	kSentenceLine = 6
};


void ScummEngine::redrawVerbs() {
	int i;
	int verb = (_cursor.state > 0 ? checkMouseOver(_mouse.x, _mouse.y) : 0);
	for (i = _numVerbs-1; i >= 0; i--) {
		if (i == verb && _verbs[verb].hicolor)
			drawVerb(i, 1);
		else
			drawVerb(i, 0);
	}
	_verbMouseOver = verb;
}

void ScummEngine::checkExecVerbs() {
	int i, over;
	VerbSlot *vs;

	if (_userPut <= 0 || _mouseButStat == 0)
		return;

	if (_mouseButStat < MBS_MAX_KEY) {
		/* Check keypresses */
		vs = &_verbs[1];
		for (i = 1; i < _numVerbs; i++, vs++) {
			if (vs->verbid && vs->saveid == 0 && vs->curmode == 1) {
				if (_mouseButStat == vs->key) {
					// Trigger verb as if the user clicked it
					runInputScript(1, vs->verbid, 1);
					return;
				}
			}
		}
		// Generic keyboard input
		runInputScript(4, _mouseButStat, 1);
	} else if (_mouseButStat & MBS_MOUSE_MASK) {
		VirtScreen *zone = findVirtScreen(_mouse.y);
		byte code = _mouseButStat & MBS_LEFT_CLICK ? 1 : 2;
		over = checkMouseOver(_mouse.x, _mouse.y);
		if (over != 0) {
			// Verb was clicked
			runInputScript(1, _verbs[over].verbid, code);
		} else {
			// Scene was clicked
			runInputScript((zone->number == 0) ? 2 : 1, 0, code);
		}
	}
}

void ScummEngine::verbMouseOver(int verb) {
	if (_verbMouseOver == verb)
		return;

	if (_verbs[_verbMouseOver].type != kImageVerbType) {
		drawVerb(_verbMouseOver, 0);
		_verbMouseOver = verb;
	}

	if (_verbs[verb].type != kImageVerbType && _verbs[verb].hicolor) {
		drawVerb(verb, 1);
		_verbMouseOver = verb;
	}
}

int ScummEngine::checkMouseOver(int x, int y) const {
	VerbSlot *vs;
	int i = _numVerbs - 1;

	vs = &_verbs[i];
	do {
		if (vs->curmode != 1 || !vs->verbid || vs->saveid || y < vs->curRect.top || y >= vs->curRect.bottom)
			continue;
		if (vs->center) {
			if (x < -(vs->curRect.right - 2 * vs->curRect.left) || x >= vs->curRect.right)
				continue;
		} else {
			if (x < vs->curRect.left || x >= vs->curRect.right)
				continue;
		}

		return i;
	} while (--vs, --i);

	return 0;
}

void ScummEngine::drawVerb(int verb, int mode) {
	VerbSlot *vs;
	bool tmp;

	if (!verb)
		return;

	vs = &_verbs[verb];

	if (!vs->saveid && vs->curmode && vs->verbid) {
		if (vs->type == kImageVerbType) {
			drawVerbBitmap(verb, vs->curRect.left, vs->curRect.top);
			return;
		}
		
		restoreVerbBG(verb);

		_string[4].charset = vs->charset_nr;
		_string[4].xpos = vs->curRect.left;
		_string[4].ypos = vs->curRect.top;
		_string[4].right = SCREEN_WIDTH - 1;
		_string[4].center = vs->center;

		if (vs->curmode == 2)
			_string[4].color = vs->dimcolor;
		else if (mode && vs->hicolor)
			_string[4].color = vs->hicolor;
		else
			_string[4].color = vs->color;

		// FIXME For the future: Indy3 and under inv scrolling
		/*
		   if (verb >= 31 && verb <= 36) 
		   verb += _inventoryOffset;
		 */

		_messagePtr = getResourceAddress(rtVerb, verb);
		if (!_messagePtr)
			return;
		assert(_messagePtr);

		tmp = _charset->_center;
		_charset->_center = 0;
		drawString(4);
		_charset->_center = tmp;

		vs->curRect.right = _charset->_str.right;
		vs->curRect.bottom = _charset->_str.bottom;
		vs->oldRect = _charset->_str;
		_charset->_str.left = _charset->_str.right;
	} else {
		restoreVerbBG(verb);
	}
}

void ScummEngine::restoreVerbBG(int verb) {
	VerbSlot *vs;

	vs = &_verbs[verb];

	if (vs->oldRect.left != -1) {
		restoreBG(vs->oldRect, vs->bkcolor);
		vs->oldRect.left = -1;
	}
}

void ScummEngine::drawVerbBitmap(int verb, int x, int y) {
	VirtScreen *vs;
	VerbSlot *vst;
	bool twobufs;
	const byte *imptr = 0;
	int ydiff, xstrip;
	int imgw, imgh;
	int i, tmp;
	byte *obim;
	const ImageHeader *imhd;

	if ((vs = findVirtScreen(y)) == NULL)
		return;

	gdi.disableZBuffer();

	twobufs = vs->hasTwoBuffers;
	vs->hasTwoBuffers = 0;

	xstrip = x / 8;
	ydiff = y - vs->topline;

	obim = getResourceAddress(rtVerb, verb);
	assert(obim);
	imhd = (const ImageHeader *)findResourceData(MKID('IMHD'), obim);
	imgw = READ_LE_UINT16(&imhd->old.width) / 8;
	imgh = READ_LE_UINT16(&imhd->old.height) / 8;
	imptr = getObjectImage(obim, 1);
	assert(imptr);
	for (i = 0; i < imgw; i++) {
		tmp = xstrip + i;
		if (tmp < SCREEN_STRIP_COUNT)
			gdi.drawBitmap(imptr, vs, tmp, ydiff, imgw * 8, imgh * 8, i, 1, 0, Gdi::dbAllowMaskOr);
	}

	vst = &_verbs[verb];
	vst->curRect.right = vst->curRect.left + imgw * 8;
	vst->curRect.bottom = vst->curRect.top + imgh * 8;
	vst->oldRect = vst->curRect;

	gdi.enableZBuffer();

	vs->hasTwoBuffers = twobufs;
}

int ScummEngine::getVerbSlot(int id, int mode) const {
	int i;
	for (i = 1; i < _numVerbs; i++) {
		if (_verbs[i].verbid == id && _verbs[i].saveid == mode) {
			return i;
		}
	}
	return 0;
}

void ScummEngine::killVerb(int slot) {
	VerbSlot *vs;

	if (slot == 0)
		return;

	vs = &_verbs[slot];
	vs->verbid = 0;
	vs->curmode = 0;

	nukeResource(rtVerb, slot);

	if (vs->saveid == 0) {
		drawVerb(slot, 0);
		verbMouseOver(0);
	}
	vs->saveid = 0;
}

void ScummEngine::setVerbObject(uint room, uint object, uint verb) {
	const byte *obimptr;
	uint32 size;
	FindObjectInRoom foir;

	if (whereIsObject(object) == WIO_FLOBJECT)
		error("Can't grab verb image from flobject");

	findObjectInRoom(&foir, foImageHeader, object, room);
	size = READ_BE_UINT32(foir.obim + 4);
	createResource(rtVerb, verb, size);
	obimptr = getResourceAddress(rtRoom, room) - foir.roomptr + foir.obim;
	memcpy(getResourceAddress(rtVerb, verb), obimptr, size);
}

} // End of namespace Scumm
