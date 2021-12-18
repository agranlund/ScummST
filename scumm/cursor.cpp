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
 * $Header: /cvsroot/scummvm/scummvm/scumm/cursor.cpp,v 2.3 2004/01/06 12:45:30 fingolfin Exp $
 *
 */

#include "stdafx.h"
#include "scumm/bomp.h"
#include "scumm/scumm.h"
#include "scumm/resource.h"

#ifdef __ATARI__
	#ifdef GAME_SAMNMAX
		#define CURSOR_SUPPORT_DEFAULT	0
		#define CURSOR_SUPPORT_GRAB		0
		#define CURSOR_SUPPORT_IM01		1
		#define CURSOR_SUPPORT_BOMP		1
	#else
		#define CURSOR_SUPPORT_DEFAULT	0
		#define CURSOR_SUPPORT_GRAB		0
		#define CURSOR_SUPPORT_IM01		0
		#define CURSOR_SUPPORT_BOMP		0
	#endif
#else
	#ifdef GAME_SAMNMAX
		#define CURSOR_SUPPORT_DEFAULT	1
		#define CURSOR_SUPPORT_GRAB		0
		#define CURSOR_SUPPORT_IM01		0
		#define CURSOR_SUPPORT_BOMP		0
	#else
		#define CURSOR_SUPPORT_DEFAULT	1
		#define CURSOR_SUPPORT_GRAB		0
		#define CURSOR_SUPPORT_IM01		0
		#define CURSOR_SUPPORT_BOMP		0
	#endif
#endif

#define CURSOR_MAX_WIDTH	64
#define CURSOR_MAX_HEIGHT	32

// todo:
//
//	decompress to atari format.
//	call set_mouse_cursor() with MOVEP data + optional mask
//	_grabbedCursor is quite large so we can store both img+mask in there
//	make sure nothing breaks in terms of savegames
//


namespace Scumm {

/*
 * Mouse cursor cycle colors (for the default crosshair).
 */

static const byte cursor_dummy_mask[CURSOR_MAX_HEIGHT] = {
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};

#if CURSOR_SUPPORT_DEFAULT
static const byte default_v1_cursor_colors[4] = {
	1, 1, 12, 11
};
static const byte default_cursor_colors[4] = {
	15, 15, 7, 8
};
static const uint16 default_cursor_images[5][16] = {
	/* cross-hair */
	{ 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0000, 0x7e3f,
	  0x0000, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0000 },
	/* hourglass */
	{ 0x0000, 0x7ffe, 0x6006, 0x300c, 0x1818, 0x0c30, 0x0660, 0x03c0,
	  0x0660, 0x0c30, 0x1998, 0x33cc, 0x67e6, 0x7ffe, 0x0000, 0x0000 },
	/* arrow */
	{ 0x0000, 0x4000, 0x6000, 0x7000, 0x7800, 0x7c00, 0x7e00, 0x7f00,
	  0x7f80, 0x78c0, 0x7c00, 0x4600, 0x0600, 0x0300, 0x0300, 0x0180 },
	/* hand */
	{ 0x1e00, 0x1200, 0x1200, 0x1200, 0x1200, 0x13ff, 0x1249, 0x1249,
	  0xf249, 0x9001, 0x9001, 0x9001, 0x8001, 0x8001, 0x8001, 0xffff },
	/* cross-hair zak256 - chrilith palmos */
/*
	{ 0x0080, 0x0080, 0x02a0, 0x01c0, 0x0080, 0x1004, 0x0808, 0x7c1f,
	  0x0808, 0x1004, 0x0080, 0x01c0, 0x02a0, 0x0080, 0x0080, 0x0000 },
*/
	{ 0x0080, 0x02a0, 0x01c0, 0x0080, 0x0000, 0x2002, 0x1004, 0x780f,
	  0x1004, 0x2002, 0x0000, 0x0080, 0x01c0, 0x02a0, 0x0080, 0x0000 },
};

static const byte default_cursor_hotspots[10] = {
	8, 7,   8, 7,   1, 1,   5, 0,
	8, 7, //zak256
};
#endif // CURSOR_SUPPORT_DEFAULT

void ScummEngine::setupCursor()
{
	debug(1, "setupCursor");
	_currentCursor = 0;
	_cursor.state = 0;	
	_cursor.animate = 1;
	_grabbedCursorId = 0;
	memset(_grabbedCursorTransp, 0xFF, sizeof(_grabbedCursorTransp));
	if (_gameId == GID_TENTACLE && res.roomno[rtRoom][60]) {
		// HACK: For DOTT we manually set the default cursor. See also bug #786994
		setCursorImg(697, 60, 1);
		makeCursorColorTransparent(1);
	}
}

void ScummEngine::grabCursor(uint32 id, int16 x, int16 y, int16 w, int16 h)
{
	// called by v6 script (but is it ever used in samnmax?)
	debug(1, "grabCursor: 0x%08x: %d,%d (%dx%d)", id, x, y, w, h);
#if CURSOR_SUPPORT_GRAB
	VirtScreen *vs = findVirtScreen(y);
	if (vs == NULL)
		return;
	byte* ptr = vs->screenPtr + (x >> 1) + ((vs->width >> 1) * (y - vs->topline));
	assertAligned(ptr);

	w = CLAMP<int16>(w, 0, CURSOR_MAX_WIDTH);
	h = CLAMP<int16>(h, 0, CURSOR_MAX_HEIGHT);

	#ifndef __ATARI__
	uint32 size = width * height;
	if (size > sizeof(_grabbedCursor))
		error("grabCursor: grabbed cursor too big");
	#endif

	// todo: implement me..

	byte* dst = _grabbedCursor;
	const byte* src = ptr;
	int16 numstrips = width >> 3;
	uint32 srcInc = (pitch - width) >> 1;
	uint32 dstInc = 0;
	while(height)
	{
		for (uint16 i=0; i<numstrips; ++i)
		{
			byte s0 = *src++;
			byte s1 = *src++;
			byte s2 = *src++;
			byte s3 = *src++;

			for (int16 j=8; j!=0; j--)
			{
				*dst++ = (s0 >> 7) | ((s1 >> 6) & 2) | ((s2 >> 5) & 4) | ((s3 >> 4) & 8);
				s0 <<= 1; s1 <<= 1; s2 <<= 1; s3 <<= 1;
			}
		}
		dst += dstInc;
		src += srcInc;
		height--;
	};

	_grabbedCursorId = id;
	_grabbedCursorTransp[0] = 0xFF;
	_cursor.width = width;
	_cursor.height = height;
	_cursor.animate = 0;
	updateCursor();
#else
	_currentCursor = 0;
	_cursor.animate = 1;
	decompressDefaultCursor((_cursor.animateIndex >> 1) & 3);
#endif
}

void ScummEngine::useIm01Cursor(uint32 id, const byte *im, int16 width, int16 height)
{
	// called from setCursorImg()
	int16 numstrips = width;
#if CURSOR_SUPPORT_IM01
	if (width & 1)
		width++;

	width <<= 3; height <<= 3;
	width = CLAMP<int16>(width, 0, CURSOR_MAX_WIDTH);
	height = CLAMP<int16>(height, 0, CURSOR_MAX_HEIGHT);
	_grabbedCursorId = id;
	_grabbedCursorTransp[0] = 0xFF;
	_cursor.width = width;
	_cursor.height = height;
	_cursor.animate = 0;

	bool cached = _system->mouse_cursor_cached(id);
	debug(1, "useIm01Cursor: 0x%08x: %dx%d %s", id, width, height, cached ? "(CACHED)" : "");
	if (cached) {
		updateCursor();
		return;
	}

	byte* tempBuffer = _grabbedCursor;
	byte* maskBuffer = _grabbedCursor + (sizeof(_grabbedCursor)>>1);
	int16 dstPtrInc = (width >> 1);
	int16 dstMskInc = (width >> 3);

	const byte* smap_ptr = findResource(MKID('SMAP'), im);
	if (!smap_ptr)
		return;

	assertAligned(smap_ptr);
	int16 stripnr = 0;

	while (stripnr < numstrips)
	{
		byte* dstMsk = maskBuffer + (stripnr & ~1);
		byte* dstPtr = tempBuffer + ((stripnr & ~1) << 2);
		const byte* stripData = smap_ptr + READ_LE_UINT32(smap_ptr + (stripnr << 2) + 8);
		byte code = *stripData++;
		if (code == 0xFE || code == 0xFF)
		{
			byte extra = *(stripData - 2);
			const uint32* srcPtr = (const uint32*)stripData;
			assertAligned(srcPtr);
			const byte* mskPtr = (const byte*) (srcPtr + height);
			if (extra == 2)
			{
				// skip standard object mask if any
				uint32 maskSize = (height + 3) & ~3;
				if (code & 1)
					mskPtr += maskSize;
				// select cursor mask variation
				if ((id & (1<<1)) == 0)
					mskPtr += maskSize;
			}
			else if ((code & 1) == 0)
			{
				mskPtr = cursor_dummy_mask;
			}

			int16 lines = height;
			if ((stripnr & 1) == 0)
			{
				while (lines)
				{
					uint32 px = *srcPtr++;
					byte pm = *mskPtr++;
#ifdef __ATARI__
					*(dstMsk+0) = pm;
					__asm__ volatile ("movep.l %1,0(%0)\n\t" : : "a"(dstPtr), "d"(px) : "memory");
#else
					// todo: PC
#endif
					dstMsk += dstMskInc;
					dstPtr += dstPtrInc;
					lines--;
				}
			}
			else
			{
				while (lines)
				{
					uint32 px = *srcPtr++;
					byte pm = *mskPtr++;
#ifdef __ATARI__
					*(dstMsk+1) = pm;
					__asm__ volatile ("movep.l %1,1(%0)\n\t" : : "a"(dstPtr), "d"(px) : "memory");
#else
					// todo: PC
#endif
					dstMsk += dstMskInc;
					dstPtr += dstPtrInc;
					lines--;
				}
			}
		}
		stripnr++;
	}
	if (numstrips & 1)
	{
		byte* mskPtr = maskBuffer + numstrips;
		while (height) {
			*mskPtr = 0;
			mskPtr += width;
			height--;
		}
	}

	updateCursor();
#else
	_currentCursor = 0;
	_cursor.animate = 1;
	decompressDefaultCursor((_cursor.animateIndex >> 1) & 3);
#endif
}


#ifdef ENGINE_SCUMM6
void ScummEngine::useBompCursor(uint32 id, const byte *im, int16 width, int16 height)
{
	// called from setCursorImg()
#if CURSOR_SUPPORT_BOMP
/*
	bool cached = _system->mouse_cursor_cached(_grabbedCursorId);
	debug(1, "useBompCursor: 0x%08x: %dx%d %s", id, width, height, cached ? "(CACHED)" : "");
	if (cached) {
		updateCursor();
		return;
	}
*/
	#ifndef __ATARI__
	uint32 size = width * height;
	if (size > sizeof(_grabbedCursor))
		error("useBompCursor: cursor too big (%d)", size);
	#endif

	assertAligned(im);
	const byte*		hdr 		= im + 18;
	const int16 	scaleIdx 	= 0;
	const int16 	frameIdx 	= 0;
	const uint16* 	scaleHeader = (uint16*)(hdr + 4);
	const byte* 	frameHeader = (byte*)(hdr + READ_BE_UINT16(&scaleHeader[scaleIdx]));
	int16 			w 			= *(frameHeader + 0);
	int16			h			= *(frameHeader + 1);
	const uint32*	offsets		= (uint32*)(frameHeader + 8);
	const uint32 	offset		= READ_BE_UINT32(&offsets[frameIdx]);
	const uint32* 	sptr 		= (uint32*)(hdr + offset);

	uint16 dstMskInc = 0;
	uint16 dstPtrInc = 0;
	if (w > width) width = w;
	if (width & 1) width++;
	if (width > w) {
		dstMskInc = (width - w);
		dstPtrInc = dstMskInc << 2;
	}
	if (w & 1)
		dstPtrInc += 4;

	width = CLAMP<int16>(width<<3, 0, CURSOR_MAX_WIDTH);
	height = CLAMP<int16>(height<<3, 0, CURSOR_MAX_HEIGHT);
	_grabbedCursorId = id;
	_grabbedCursorTransp[0] = 0xFF;
	_cursor.width = width;
	_cursor.height = height;
	_cursor.animate = 0;
	
	byte* dstPtr = _grabbedCursor;
	byte* dstMsk = _grabbedCursor + (sizeof(_grabbedCursor)>>1);
	memset(dstMsk, 0, (width * height) >> 3);

#ifdef __ATARI__
	for (int16 y = 0; y < h; y++)
	{
		for (int16 x = 0; x < w; x++)
		{
			uint32 px, pm;

			px = *sptr++;
			if ((x & 1) == 0) {
				__asm__ volatile ("movep.l %1,0(%0)\n\t" : : "a"(dstPtr), "d"(px) : "memory");
			} else {
				__asm__ volatile ("movep.l %1,1(%0)\n\t" : : "a"(dstPtr), "d"(px) : "memory");
				dstPtr += 8;
			}
			*dstMsk++ = (byte) *sptr++;
		}
#else
		// todo: PC
#endif
		dstPtr += dstPtrInc;
		dstMsk += dstMskInc;
		height--;
	}

	updateCursor();
#else
	_currentCursor = 0;
	_cursor.animate = 1;
	decompressDefaultCursor((_cursor.animateIndex >> 1) & 3);
#endif
}
#endif //ENGINE_SCUMM6

void ScummEngine::decompressDefaultCursor(int16 idx)
{
	// todo: decompress to atari format
	_grabbedCursorId = ((_currentCursor & 0xFFFF) << 8) | ((idx & 0xF) << 4);
	_grabbedCursorTransp[0] = 0xFF;
#if CURSOR_SUPPORT_DEFAULT
	_cursor.width = 16;
	_cursor.height = 16;
	byte currentCursor = _currentCursor;
	_cursor.hotspotX = default_cursor_hotspots[2 * currentCursor];
	_cursor.hotspotY = default_cursor_hotspots[2 * currentCursor + 1];
	bool cached = _system->mouse_cursor_cached(_grabbedCursorId);
	debug(1, "decompressDefaultCursor: %d,%d %s", _currentCursor, idx, cached ? "(CACHED)" : "");
	if (!cached)
	{
		byte color = default_cursor_colors[idx];
		memset(_grabbedCursor, 0xFF, sizeof(_grabbedCursor));
		for (int16 i = 0; i < 16; i++) {
			for (int16 j = 0; j < 16; j++) {
				if (default_cursor_images[currentCursor][i] & (1 << j))	
					_grabbedCursor[(i * 16) + 15 - j] = color;
			}
		}
	}
#endif
	updateCursor();
}

void ScummEngine::setCursor(int16 cursor)
{
	if ((cursor & ~3) == 0)
		_currentCursor = cursor;
	else
		warning("setCursor(%d)", cursor);
}

void ScummEngine::setCursorHotspot(int16 x, int16 y) {
	_cursor.hotspotX = x;
	_cursor.hotspotY = y;
}

void ScummEngine::updateCursor()
{
	byte* mask = _grabbedCursor + (sizeof(_grabbedCursor) >> 1);
	_system->set_mouse_cursor(_grabbedCursorId, _grabbedCursor, mask, _cursor.width, _cursor.height, _cursor.hotspotX, _cursor.hotspotY);
}

void ScummEngine::animateCursor()
{
	if (_cursor.animate)
	{
		if (!(_cursor.animateIndex & 0x1))
		{
			decompressDefaultCursor((_cursor.animateIndex >> 1) & 3);
		}
		_cursor.animateIndex++;
	}
}

void ScummEngine::makeCursorColorTransparent(int16 a)
{
#if CURSOR_SUPPORT_DEFAULT || CURSOR_SUPPORT_IM01 || CURSOR_SUPPORT_BOMP
	debug(1, "makeCursorColorTransparent: %d", a);

	// ignore if bomp
	if (_grabbedCursorId & 1)
		return;

	// sanity check room
	int16 room = (_grabbedCursorId >> 24) & 0xFF;
	if (room == 0 || room == 0xFF)
		return;

	// interaction outline?
	if (a == 180)
	{
		if ((_grabbedCursorId & (1 << 1)) == 0)
		{
			int16 img = (_grabbedCursorId >> 8) & 0xFFFF;
			int16 imgindex = (_grabbedCursorId >> 4) & 0xF;
			setCursorImg(img, room, imgindex, 1);
		}
	}

	// save calls
	const int16 maxCursorTransp = sizeof(_grabbedCursorTransp) - 1;
	for (int16 i = 0; i != maxCursorTransp; i++)
	{
		if (_grabbedCursorTransp[i] == 0xFF)
		{
			_grabbedCursorTransp[i] = a;
			_grabbedCursorTransp[i+1] = 0xFF;
		}
		if (_grabbedCursorTransp[i] == a)
			break;
	}

	//updateCursor();
#endif
}

} // End of namespace Scumm
