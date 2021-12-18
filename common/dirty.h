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
 * $Header: /cvsroot/scummvm/scummvm/common/util.h,v 1.42 2004/02/14 11:15:48 fingolfin Exp $
 */

#ifndef COMMON_DIRTY_H
#define COMMON_DIRTY_H
/*
#include "common/scummsys.h"
#include "common/system.h"
*/
#include "common/util.h"
#include "common/rect.h"

#define DIRTYMASK_FULL 0x01FFFFFF

namespace Common {

class DirtyMask {

public:
	DirtyMask() {
		//_limit = SCREEN_HEIGHT;
	}

	DirtyMask(const uint32* otherData) {
		uint32* d = maskBuf(); const uint32* s = otherData;
		*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
		*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
		*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
		*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
	}

	DirtyMask(const DirtyMask& other) {
		uint32* d = maskBuf(); const uint32* s = other.maskBuf();
		*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
		*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
		*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
		*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
	}

	DirtyMask& operator =(const DirtyMask& other) {
		uint32* d = maskBuf(); const uint32* s = other.maskBuf();
		*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
		*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
		*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
		*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
		return *this;
	}

	DirtyMask& operator|=(const DirtyMask& other) {
		uint32* d = maskBuf(); const uint32* s = other.maskBuf();
		*d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++;
		*d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++;
		*d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++;
		*d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++; *d++ |= *s++;
		return *this;
	}

	DirtyMask& operator|=(const uint32 mask) {
		uint32* d = maskBuf();
		*d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask;
		*d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask;
		*d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask;
		*d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask; *d++ |= mask;
		return *this;
	}

	DirtyMask& operator&=(const uint32 mask) {
		uint32* d = maskBuf();
		*d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask;
		*d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask;
		*d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask;
		*d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask; *d++ &= mask;
		return *this;
	}

	uint32& operator [](int strip) {
		assert(strip >= 0 && strip < SCREEN_STRIP_COUNT);
		return _maskBuffer[strip];
	}

	uint32 operator [](int strip) const {
		assert(strip >= 0 && strip < SCREEN_STRIP_COUNT);
		return _maskBuffer[strip];
	}

	inline static uint32 bitMask(int16 top, int16 bottom) {
		// todo: make fast..
		if (top >= SCREEN_HEIGHT) return 0;
		if (bottom <= 0) return 0;
		if (top < 0) top = 0;
		if (bottom > SCREEN_HEIGHT) bottom = SCREEN_HEIGHT;
		if (top == bottom) return 0;
		byte t = ((byte)CLAMP<int16>((top&~7), 0, 192)) >> 3;
		byte b = ((byte)CLAMP<int16>((bottom+7)&~7, 0, 200)) >> 3;
		return ((DIRTYMASK_FULL << t) & (DIRTYMASK_FULL >> (25-b)));
	}

	inline uint32* maskBuf(int16 strip = 0) const {
		assert(strip >= 0 && strip < SCREEN_STRIP_COUNT);
		return (uint32*)&_maskBuffer[strip];
	}

	inline void clear() {
		fill(0);
	}

	inline void fill(int16 top, int16 bottom) { 
		fill(bitMask(top,bottom));
	}

	inline void fill(uint32 m) {
		uint32* d = maskBuf();
		m &= DIRTYMASK_FULL;
		*d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; 
		*d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; 
		*d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; 
		*d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; *d++ = m; 
	}

	bool isDirty(int16 left, int16 top, int16 right, int16 bottom) {
		int16 tx0 = CLAMP<int16>(left>>3, 0, SCREEN_STRIP_COUNT-1);
		int16 tx1 = CLAMP<int16>(right>>3, 0, SCREEN_STRIP_COUNT-1);
		uint32 m = bitMask(top, bottom);
		while (tx0 <= tx1)
		{
			if (m & _maskBuffer[tx0])
				return true;
			tx0++;
		}
		return false;
	}

	void setRect(int16 left, int16 top, int16 right, int16 bottom) {
		uint32 m = bitMask(top, bottom);
		int16 x1 = left >> 3;
		int16 x2 = right >> 3;
		for( ; x1 <= x2; x1++)
			setStrip(x1, m);
	}

	void updateRect(int16 left, int16 top, int16 right, int16 bottom) {
		uint32 m = bitMask(top, bottom);
		int16 x1 = left >> 3;
		int16 x2 = right >> 3;
		for( ; x1 <= x2; x1++)
			updateStrip(x1, m);
	}

	void setRange(int16 top, int16 bottom, int16 strip = 0, int16 count = SCREEN_STRIP_COUNT) {
		uint32 m = bitMask(top, bottom);
		for (int16 i = strip; i < strip + count; i++) {
			setStrip(i, m);
		}
	}

	void updateRange(int16 top, int16 bottom, int16 strip = 0, int16 count = SCREEN_STRIP_COUNT) {
		uint32 m = bitMask(top, bottom);
		for (int16 i = strip; i < strip + count; i++) {
			updateStrip(i, m);
		}
	}

	inline void setStrip(uint16 strip, uint16 top, uint16 bottom) {
		setStrip(strip, bitMask(top,bottom));
	}
	inline void setStrip(uint16 strip, uint32 mask) {
		if (strip < 0 || strip >= SCREEN_STRIP_COUNT)
			return;
		_maskBuffer[strip] = mask;
	}

	inline void updateStrip(uint16 strip, uint16 top, uint16 bottom) {
		updateStrip(strip, bitMask(top,bottom));
	}
	inline void updateStrip(uint16 strip, uint32 mask) {
		if (strip < 0 || strip >= SCREEN_STRIP_COUNT)
			return;
		_maskBuffer[strip] |= mask;
	}


private:
/*
	int16	_left;
	int16	_right;
	int16	_limit;
*/
	uint32 	_maskBuffer[SCREEN_STRIP_COUNT];
};


}	// End of namespace Common

#endif
