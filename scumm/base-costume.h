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
 * $Header: /cvsroot/scummvm/scummvm/scumm/base-costume.h,v 1.18 2004/01/12 14:09:39 kirben Exp $
 *
 */

#ifndef BASE_COSTUME_H
#define BASE_COSTUME_H

#include "common/scummsys.h"
#include "scumm/actor.h"		// for CostumeData

namespace Scumm {

#if !defined(__GNUC__)
#pragma START_PACK_STRUCTS
#endif

struct CostumeInfo {
	uint16 width, height;
	int16 rel_x, rel_y;
	int16 move_x, move_y;
} GCC_PACK;

#if !defined(__GNUC__)
#pragma END_PACK_STRUCTS
#endif

class Actor;
class ScummEngine;
struct VirtScreen;

/**
 * Base class for both CostumeRenderer and AkosRenderer.
 */
class BaseCostumeRenderer {
public:
	byte _actorID;

	int _actorX, _actorY;
	byte _zbuf;
	byte _scaleX, _scaleY;

	int _draw_top, _draw_bottom, _draw_left, _draw_right;
	bool _skipLimb;

protected:
	ScummEngine *_vm;

	// Destination
	byte *_outptr;
	uint _outheight;
	uint _outoffs;

	// Source pointer
	const byte *_srcptr;

	// current move offset
	int _xmove, _ymove;

	// whether to draw the actor mirrored
	bool _mirror;

	// width and height of cel to decode
	int _width, _height;

	struct {
		// Parameters for the original ("V1") costume codec.
		const byte *scaletable;
		byte mask, shr;
		byte repcolor;
		byte replen;
		int scaleXstep;
		int x, y;
		int scaleXindex, scaleYindex;
		int skip_width;
		byte *destptr;
		const byte *mask_ptr;
		int imgbufoffs;
	} v1;

public:
	BaseCostumeRenderer(ScummEngine *scumm) {
		_actorID = 0;
		_actorX = _actorY = 0;
		_zbuf = 0;
		_scaleX = _scaleY = 0;
		_draw_top = _draw_bottom = 0;
		_draw_left = _draw_right = 0;
		
		_vm = scumm;
		_srcptr = 0;
		_xmove = _ymove = 0;
		_mirror = false;
		_width = _height = 0;
		_skipLimb = 0;

		
		_outptr = 0;
		_outheight = 0;
	}

	virtual ~BaseCostumeRenderer() {
	}

	virtual void setPalette(byte *palette) = 0;
	virtual void setFacing(Actor *a) = 0;
	virtual void setCostume(int costume) = 0;


	byte drawCostume(const VirtScreen &vs, const CostumeData &cost);

protected:

	virtual byte drawLimb(const CostumeData &cost, int limb) = 0;

	void codec1_ignorePakCols(int num);
};

} // End of namespace Scumm

#endif
