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
 * $Header: /cvsroot/scummvm/scummvm/common/scaler.h,v 1.24 2004/01/06 12:45:28 fingolfin Exp $
 */

#ifndef COMMON_SCALER_H
#define COMMON_SCALER_H

#ifndef __ATARI__

extern void InitScalers(uint32 BitFormat);

typedef void ScalerProc(const uint8 *srcPtr, uint32 srcPitch,
							uint8 *dstPtr, uint32 dstPitch, int width, int height);

#define DECLARE_SCALER(x)	\
	extern void x(const uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, \
					uint32 dstPitch, int width, int height)


DECLARE_SCALER(Normal1x);
DECLARE_SCALER(Normal2x);

FORCEINLINE int real2Aspect(int y) {
	return y + (y + 1) / 5;
}

FORCEINLINE int aspect2Real(int y) {
	return (y * 5 + 4) / 6;
}

//extern void makeRectStretchable(int &x, int &y, int &w, int &h);

//extern int stretch200To240(uint8 *buf, uint32 pitch, int width, int height, int srcX, int srcY, int origSrcY);

enum {
	GFX_NORMAL = 0,
	GFX_DOUBLESIZE = 1,
};

#else //__ATARI__

enum {
	GFX_NORMAL = 0,

};

#endif //__ATARI__


#endif
