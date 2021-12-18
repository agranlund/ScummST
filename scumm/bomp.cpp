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
 * $Header: /cvsroot/scummvm/scummvm/scumm/bomp.cpp,v 2.17 2004/01/31 16:49:52 fingolfin Exp $
 *
 */

// sam'n'max:
//	always draw in mode0
//	minor use of scaling (title screen + highway minigame)
//	never user mirroring
//	_bompActorPalettePtr is always null
//	width is always multiple of 8

#include "stdafx.h"
#include "scumm/scumm.h"
#include "scumm/bomp.h"
#include "scumm/charset.h"
extern bool g_slowMachine;

// we may not need this for samnmax...
#define BOMP_SUPPORT_CHARSET_MASK

namespace Scumm {

void decompressBomp(byte *dst, const byte *src, int w, int h) {
	assert(w > 0);
	assert(h > 0);
#if 1

	assertAligned(src);
	const byte* hdr = src;
	const byte* frameHeader = hdr + 4 + 16;

	int16 xtiles = *(frameHeader + 0);
	int16 height = *(frameHeader + 1);
	const uint32* offsets = (uint32*)(frameHeader + 8);
	assertAligned(offsets);
	int16 frame = 0;

	uint32 offset = READ_BE_UINT32(&offsets[frame]);
	const byte* 	sptr = (src + offset);
	assertAligned(sptr);
	uint32* 		dptr = (uint32*)dst;
	assertAligned(dptr);
	int16 			outtiles = (w + 7) >> 3;

	debug(1,"decompBomp %d,%d,%d. %d (%x,%x, %x,%x)", xtiles, height, outtiles, offset, src, dst, sptr, dptr);
	if (outtiles == 0)
		return;

	int16 sinc = 2;
	while(height)
	{
		for (int16 i=0; i<outtiles; i++)
		{
			uint32 px = *sptr++;
			uint32 pm = *sptr++;
			*dptr++ = px;
		}
		sptr += sinc;
		height--;
	}

	debug(1,"decompBomp done");

#else
	const byte* pal = g_scumm->_roomPalette;
	while(h)
	{
		uint16 linelen = READ_LE_UINT16(src); src += 2;
		const byte* lnsrc = src;
		src += linelen;
		byte* lndst = dst;
		dst += w;

		uint16 len = w;
		while (len)
		{
			uint16 code = *lnsrc++;
			uint16 num = (code >> 1) + 1;
			if (num > len)
				num = len;
			len -= num;
			if (code & 1) {
				int16 color = *lnsrc++;
				if (color != 0xFF)
					color = pal[color];
				while(num) {
					*lndst++ = color;
					num--;
				}
			} else {
				while (num) {
					int16 color = *lnsrc++;
					if (color != 0xFF)
						color = pal[color];
					*lndst++ = color;
					num--; 
				}
			}
		}
		h--;
	}
#endif
}

void ScummEngine::drawBomp(const BompDrawData &bd, bool mirror)
{
	// bd values
	int16 x = bd.x;
	int16 y = bd.y;
	int16 scale = bd.scale_x < bd.scale_y ? bd.scale_x : bd.scale_y;
	const byte* src = (const byte*) bd.dataptr;
	assertAligned(src);
	if (((uint32)src) & 1)
		return;
	
	// read base header
	//const byte magic	= *(src + 0);
	const byte flags 	= *(src + 1);
	const byte minscale = *(src + 2);
	const byte maxscale = *(src + 3);

	// reject scale
	if (scale < minscale || scale > maxscale)
		return;

	// slow machine special treatment
	if ((flags & 0x80) && (scale != 255) && g_slowMachine)
		return;

	// determine scale and shift indices
	const int16 scaleIdx = 7 - (scale >> 5);
	const int16 frameIdx = x & 7;

	// fetch frame header for scale index
	const uint16*	scaleHeader	= (uint16*)(src + 4);
	assertAligned(scaleHeader);
	const byte*		frameHeader	= (byte*)(src + READ_BE_UINT16(&scaleHeader[scaleIdx]));
	assertAligned(frameHeader);
	const int16 	xtiles 		= *(frameHeader + 0);
	int16 			height 		= *(frameHeader + 1);

	// read frame header
	//const uint32	dataSize	= READ_BE_UINT32(frameHeader + 4);
	const uint32*	offsets		= (uint32*)(frameHeader + 8);
	assertAligned(offsets);
	const uint32 	offset		= READ_BE_UINT32(&offsets[frameIdx]);
	assertAligned(offset);
	const uint32* 	sptr 		= (uint32*)(src + offset);
	assertAligned(sptr);
	uint32* 		dptr 		= (uint32*)(bd.out + MUL160(y));
	assertAligned(dptr);
	int16			tx			= (x >> 3);
	int16			tw			= xtiles;

	// mask
	#ifdef BOMP_SUPPORT_CHARSET_MASK
	byte* mptr = 0;
	if (_charset->hasCharsetMask(x, y, x + (tw << 3), y + height))
		mptr = getMaskBuffer(0, y, 0);
	#endif

	// clip top
	if (y < 0) {
		if (y + height <= 0)
			return;
		int16 yoffs = -y;
		height -= yoffs;
		dptr += MUL40(yoffs);
		#ifdef BOMP_SUPPORT_CHARSET_MASK
		if (mptr)
			mptr += MUL40(yoffs);
		#endif
		yoffs *= xtiles;
		sptr += (yoffs << 1);
	}

	// clip bottom
	if (y + height > SCREEN_HEIGHT) {
		if (y >= SCREEN_HEIGHT)
			return;
		height = SCREEN_HEIGHT - y;
	}

	// clip left
	if (tx < 0) {
		if (tx + tw <= 0)
			return;
		int16 xoffs = -tx;
		sptr += (xoffs << 1);
		dptr += xoffs;
		#ifdef BOMP_SUPPORT_CHARSET_MASK
		if (mptr)
			mptr += xoffs;
		#endif
		tw -= xoffs;
	}

	// clip right
	if (tx + tw > (SCREEN_WIDTH >> 3)) {
		if (tx >= (SCREEN_WIDTH >> 3))
			return;
		tw = (SCREEN_WIDTH >> 3) - tx;
	}

	// draw it
	#ifdef BOMP_SUPPORT_CHARSET_MASK
	if (mptr)
	{
		// masked
		dptr += tx;
		mptr += tx;
		int16 sinc = ((xtiles - tw) << 1);
		int16 dinc = (SCREEN_WIDTH >> 3) - tw;
		int16 ycounter = height;
		while (ycounter)
		{
			int16 xcounter = tw;
			while (xcounter)
			{
				uint32 c = *sptr++;
				uint32 m = *sptr++;
				uint32 mm = ~(*mptr++);
				if (m && mm)
				{
					m &= rptTable[mm];
					*dptr = (*dptr & ~m) | (c & m);
				}
				dptr++;
				xcounter--;
			}
			sptr += sinc;
			dptr += dinc;
			mptr += dinc;
			ycounter--;
		}
	}
	else
	#endif
	{
		// direct
		dptr += tx;
		int16 sinc = ((xtiles - tw) << 1);
		int16 dinc = (SCREEN_WIDTH >> 3) - tw;
		int16 ycounter = height;
		while (ycounter)
		{
			int16 xcounter = tw;
			while (xcounter)
			{
				uint32 c = *sptr++;
				uint32 m = *sptr++;
				if (m)
					*dptr = (*dptr & ~m) | (c & m);
				dptr++;
				xcounter--;
			}
			sptr += sinc;
			dptr += dinc;
			ycounter--;
		}
	}
	return;
}


/*

static void bompScaleFuncX(byte *line_buffer, byte *scaling_x_ptr, byte skip, int32 size);
static void bompApplyShadow0(const byte *line_buffer, byte *dst, int32 size, byte transparency);
static void bompApplyShadow1(const byte *line_buffer, byte *dst, int32 size, byte transparency);
static void bompApplyShadow3(const byte *shadowPalette, const byte *line_buffer, byte *dst, int32 size, byte transparency);
static void bompApplyActorPalette(byte *actorPalette, byte *line_buffer, int32 size);

void decompressBomp(byte *dst, const byte *src, int w, int h) {
	assert(w > 0);
	assert(h > 0);

	do {
		bompDecodeLine(dst, src + 2, w);
		src += READ_LE_UINT16(src) + 2;
		dst += w;
	} while (--h);
}

void bompDecodeLine(byte *dst, const byte *src, int len) {
	assert(len > 0);

	int num;
	byte code, color;
	byte* pal = g_scumm->_roomPalette;

	while (len > 0) {
		code = *src++;
		num = (code >> 1) + 1;
		if (num > len)
			num = len;
		len -= num;
		if (code & 1) {
			color = pal[*src];
			src++;
			memset(dst, color, num);
		} else {
			for (int i=0; i<num; i++)
				dst[i] = pal[src[i]];
			//memcpy(dst, src, num);
			src += num;
		}
		dst += num;
	}
}

void bompDecodeLineReverse(byte *dst, const byte *src, int len) {
	assert(len > 0);

	dst += len;
	
	int num;
	byte code, color;

	while (len > 0) {
		code = *src++;
		num = (code >> 1) + 1;
		if (num > len)
			num = len;
		len -= num;
		dst -= num;
		if (code & 1) {
			color = *src++;
			memset(dst, color, num);
		} else {
			memcpy(dst, src, num);
			src += num;
		}
	}
}

void bompApplyMask(byte *line_buffer, byte *mask, byte maskbit, int32 size, byte transparency) {
	while (1) {
		do {
			if (size-- == 0) 
				return;
			if (*mask & maskbit) {
				*line_buffer = transparency;
			}
			line_buffer++;
			maskbit >>= 1;
		} while	(maskbit);
		mask++;
		maskbit = 128;
	}
}

void bompApplyShadow(int shadowMode, const byte *line_buffer, byte *dst, int32 size, byte transparency) {
	assert(size > 0);
	switch(shadowMode) {
	case 0:
		bompApplyShadow0(line_buffer, dst, size, transparency);
		break;
	case 1:
		bompApplyShadow1(line_buffer, dst, size, transparency);
		break;
	case 3:
		bompApplyShadow1(line_buffer, dst, size, transparency);
		//bompApplyShadow3(line_buffer, dst, size, transparency);
		break;

	default:
		error("Unknown shadow mode %d", shadowMode);
	}
}
void bompApplyShadow0(const byte *line_buffer, byte *dst, int32 size, byte transparency) {
	while (size-- > 0) {
		byte tmp = *line_buffer++;
		if (tmp != transparency) {
			*dst = tmp;
		}
		dst++;
	}
}

void bompApplyShadow1(const byte *line_buffer, byte *dst, int32 size, byte transparency) {
	while (size-- > 0) {
		byte tmp = *line_buffer++;
		if (tmp != transparency) {
			if (tmp == 13) {
				tmp = 0; //shadowPalette[*dst];
			}
			*dst = tmp;
		}
		dst++;
	}
}

void bompApplyShadow3(const byte *shadowPalette, const byte *line_buffer, byte *dst, int32 size, byte transparency) {
	while (size-- > 0) {
		byte tmp = *line_buffer++;
		if (tmp != transparency) {
			if (tmp < 8) {
				tmp = shadowPalette[*dst + (tmp << 8)];
			}
			*dst = tmp;
		}
		dst++;
	}
}
void bompApplyActorPalette(byte *actorPalette, byte *line_buffer, int32 size) {
	if (actorPalette != 0) {
		actorPalette[255] = 255;
		while (size-- > 0) {
			*line_buffer = actorPalette[*line_buffer];
			line_buffer++;
		}
	}
}

void bompScaleFuncX(byte *line_buffer, byte *scaling_x_ptr, byte skip, int32 size) {
	byte *line_ptr1 = line_buffer;
	byte *line_ptr2 = line_buffer;

	byte tmp = *scaling_x_ptr++;

	while (size--) {
		if ((skip & tmp) == 0) {
			*line_ptr1++ = *line_ptr2;
		}
		line_ptr2++;
		skip >>= 1;
		if (skip == 0) {
			skip = 128;
			tmp = *scaling_x_ptr++;
		}
	}
}

void ScummEngine::drawBomp(const BompDrawData &bd, bool mirror) {
	const byte *src;
	byte *dst;
	byte maskbit;
	byte *mask = 0;
	byte *charset_mask;
	Common::Rect clip;
	byte *scalingYPtr = bd.scalingYPtr;
	byte skip_y_bits = 0x80;
	byte skip_y_new = 0;
	byte tmp;


	if (bd.x < 0) {
		clip.left = -bd.x;
	} else {
		clip.left = 0;
	}

	if (bd.y < 0) {
		clip.top = -bd.y;
	} else {
		clip.top = 0;
	}

	clip.right = bd.srcwidth;
	if (clip.right > bd.outwidth - bd.x) {
		clip.right = bd.outwidth - bd.x;
	}

	clip.bottom = bd.srcheight;
	if (clip.bottom > bd.outheight - bd.y) {
		clip.bottom = bd.outheight - bd.y;
	}

	src = bd.dataptr;
	dst = bd.out + bd.y * bd.outwidth + bd.x + clip.left;

	maskbit = revBitMask[(bd.x + clip.left) & 7];

	// Always mask against the charset mask
	charset_mask = getMaskBuffer(bd.x + clip.left, bd.y, 0);

	// Also mask against any additionally imposed mask
	if (bd.maskPtr) {
		mask = bd.maskPtr + MUL_SCREEN_STRIPS(bd.y) + ((bd.x + clip.left) / 8);
	}

	// Setup vertical scaling
	if (bd.scale_y != 255) {
		assert(scalingYPtr);

		skip_y_new = *scalingYPtr++;
		skip_y_bits = 0x80;

		if (clip.bottom > bd.scaleBottom) {
			clip.bottom = bd.scaleBottom;
		}
	}

	// Setup horizontal scaling
	if (bd.scale_x != 255) {
		assert(bd.scalingXPtr);
		if (clip.right > bd.scaleRight) {
			clip.right = bd.scaleRight;
		}
	}

	const int width = clip.right - clip.left;

	if (width <= 0)
		return;

	int pos_y = 0;
	byte line_buffer[1024];

	byte *line_ptr = line_buffer + clip.left;

	// Loop over all lines
	while (pos_y < clip.bottom) {
		// Decode a single (bomp encoded) line, reversed if we are in mirror mode
		if (mirror)
			bompDecodeLineReverse(line_buffer, src + 2, bd.srcwidth);
		else
			bompDecodeLine(line_buffer, src + 2, bd.srcwidth);
		src += READ_LE_UINT16(src) + 2;

		// If vertical scaling is enabled, do it
		if (bd.scale_y != 255) {
			// A bit set means we should skip this line...
			tmp = skip_y_new & skip_y_bits;
			
			// Advance the scale-skip bit mask, if it's 0, get the next scale-skip byte
			skip_y_bits /= 2;
			if (skip_y_bits == 0) {
				skip_y_bits = 0x80;
				skip_y_new = *scalingYPtr++;
			}

			// Skip the current line if the above check tells us to
			if (tmp != 0) 
				continue;
		}

		// Perform horizontal scaling
		if (bd.scale_x != 255) {
			bompScaleFuncX(line_buffer, bd.scalingXPtr, 0x80, bd.srcwidth);
		}

		// The first clip.top lines are to be clipped, i.e. not drawn
		if (clip.top > 0) {
			clip.top--;
		} else {
			// Replace the parts of the line which are masked with the transparency color
			if (bd.maskPtr)
				bompApplyMask(line_ptr, mask, maskbit, width, 255);
			bompApplyMask(line_ptr, charset_mask, maskbit, width, 255);
	
			// Apply custom color map, if available
			if (_bompActorPalettePtr)
				bompApplyActorPalette(_bompActorPalettePtr, line_ptr, width);
			
			// Finally, draw the decoded, scaled, masked and recolored line onto
			// the target surface, using the specified shadow mode
			bompApplyShadow(bd.shadowMode, line_ptr, dst, width, 255);
		}

		// Advance to the next line
		pos_y++;
		mask += SCREEN_STRIP_COUNT;
		charset_mask += SCREEN_STRIP_COUNT;
		dst += bd.outwidth;
	}
}

static const byte bitCount[] = {
	8, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6, 5, 6, 5, 5, 4,
	7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
	7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
	6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
	7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
	6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
	6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
	5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
	7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
	6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
	6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
	5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
	6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
	5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
	5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
	4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0,
};

const byte defaultScaleTable[768] = {
	0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
	0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
	0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
	0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
	0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
	0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
	0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
	0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
	0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
	0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
	0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
	0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
	0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
	0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
	0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
	0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
	0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
	0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
	0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
	0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
	0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
	0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
	0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
	0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
	0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
	0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
	0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
	0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
	0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
	0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
	0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFE,

	0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
	0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
	0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
	0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
	0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
	0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
	0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
	0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
	0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
	0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
	0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
	0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
	0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
	0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
	0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
	0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
	0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
	0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
	0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
	0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
	0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
	0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
	0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
	0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
	0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
	0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
	0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
	0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
	0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
	0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
	0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFE,

	0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
	0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
	0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
	0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
	0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
	0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
	0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
	0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
	0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
	0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
	0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
	0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
	0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
	0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
	0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
	0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
	0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
	0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
	0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
	0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
	0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
	0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
	0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
	0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
	0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
	0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
	0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
	0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
	0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
	0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
	0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF,
};

int16 setupBompScale(byte *scaling, int16 size, byte scale) {
	byte tmp;
	int16 count;
	const byte *tmp_ptr;
	byte *tmp_scaling = scaling;
	byte a = 0;
	byte ret_value = 0;
	const int offsets[8] = { 3, 2, 1, 0, 7, 6, 5, 4 };

	count = (256 - size / 2);
	assert(0 <= count && count < 768);
	tmp_ptr = defaultScaleTable + count;
	
	count = (size + 7) / 8;
	while (count--) {
		a = 0;
		for (int i = 0; i < 8; i++) {
			tmp = *(tmp_ptr + offsets[i]);
			a <<= 1;
			if (scale < tmp) {
				a |= 1;
			}
		}
		tmp_ptr += 8;

		*tmp_scaling++ = a;
	}
	if ((size & 7) != 0) {
		*(tmp_scaling - 1) |= revBitMask[size & 7];
	}

	count = (size + 7) / 8;
	while (count--) {
		tmp = *scaling++;
		ret_value += bitCount[tmp];
	}

	return ret_value;
}
*/

} // End of namespace Scumm
