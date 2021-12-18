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
 * $Header: /cvsroot/scummvm/scummvm/common/scaler.cpp,v 1.54 2004/01/06 12:45:28 fingolfin Exp $
 *
 */

#include "stdafx.h"
#include "common/util.h"

#ifndef __ATARI__

int gBitFormat = 565;

void InitScalers(uint32 BitFormat) {
	gBitFormat = BitFormat;
}


/**
 * Trivial 'scaler' - in fact it doesn't do any scaling but just copies the
 * source to the destionation.
 */
void Normal1x(const uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch,
							int width, int height) {
	while (height--) {
		memcpy(dstPtr, srcPtr, 2 * width);
		srcPtr += srcPitch;
		dstPtr += dstPitch;
	}
}

/**
 * Trivial nearest-neighbour 2x scaler.
 */
void Normal2x(const uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch,
							int width, int height) {
	uint8 *r;

	while (height--) {
		r = dstPtr;
		for (int i = 0; i < width; ++i, r += 4) {
			uint16 color = *(((const uint16 *)srcPtr) + i);

			*(uint16 *)(r + 0) = color;
			*(uint16 *)(r + 2) = color;
			*(uint16 *)(r + 0 + dstPitch) = color;
			*(uint16 *)(r + 2 + dstPitch) = color;
		}
		srcPtr += srcPitch;
		dstPtr += dstPitch << 1;
	}
}

#endif