/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001-2004 The ScummVM project
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
 * $Header: /cvsroot/scummvm/scummvm/scumm/charset.cpp,v 2.85 2004/01/15 22:37:48 fingolfin Exp $
 */

#include "stdafx.h"
#include "scumm/charset.h"
#include "scumm/scumm.h"
#include "scumm/gfx.h"


extern uint32 rptTable[256];

namespace Scumm {

CharsetRenderer::CharsetRenderer(ScummEngine *vm) {

	_nextLeft = 0;
	_nextTop = 0;

	_top = 0;
	_left = 0;
	_startLeft = 0;
	_right = 0;

	_color = 0;

	_dropShadow = false;
	_center = false;
	_hasMask = false;
	_ignoreCharsetMask = false;
	_blitAlso = false;
	_firstChar = false;
	_disableOffsX = false;

	_vm = vm;
	_curId = 0;
}


void CharsetRenderer::setCurID(byte id) {
	checkRange(_vm->_numCharsets - 1, 0, id, "Printing with bad charset %d");

	_curId = id;

	_fontPtr = _vm->getResourceAddress(rtCharset, id);
	if (_fontPtr == 0)
		error("CharsetRendererCommon::setCurID: charset %d not found!", id);

	_fontPtr += 29;
}

// do spacing for variable width old-style font
int CharsetRenderer::getCharWidth(byte chr) {
	int spacing = 0;

	int offs = READ_LE_UINT32(_fontPtr + chr * 4 + 4);
	if (offs) {
		spacing = _fontPtr[offs] + (signed char)_fontPtr[offs + 2];
	}
	
	return spacing;
}

int CharsetRenderer::getStringWidth(int arg, const byte *text) {
	int pos = 0;
	int width = 1;
	byte chr;
	int oldID = getCurID();

	while ((chr = text[pos++]) != 0) {
		if (chr == 0xD)
			break;
		if (chr == '@')
			continue;
		if (chr == 254 || chr == 255) {
			chr = text[pos++];
			if (chr == 3)	// 'WAIT'
				break;
			if (chr == 8) { // 'Verb on next line'
				if (arg == 1)
					break;
				while (text[pos++] == ' ')
					;
				continue;
			}
			if (chr == 10 || chr == 21 || chr == 12 || chr == 13) {
				pos += 2;
				continue;
			}
			if (chr == 9 || chr == 1 || chr == 2) // 'Newline'
				break;
			if (chr == 14) {
				int set = text[pos] | (text[pos + 1] << 8);
				pos += 2;
				setCurID(set);
				continue;
			}
		}
		width += getCharWidth(chr);
	}

	setCurID(oldID);

	return width;
}

void CharsetRenderer::addLinebreaks(int a, byte *str, int pos, int maxwidth) {
	int lastspace = -1;
	int curw = 1;
	byte chr;
	int oldID = getCurID();

	while ((chr = str[pos++]) != 0) {
		if (chr == '@')
			continue;
		if (chr == 254)
			chr = 255;
		if (chr == 255) {
			chr = str[pos++];
			if (chr == 3) // 'Wait'
				break;
			if (chr == 8) { // 'Verb on next line'
				if (a == 1) {
					curw = 1;
				} else {
					while (str[pos] == ' ')
						str[pos++] = '@';
				}
				continue;
			}
			if (chr == 10 || chr == 21 || chr == 12 || chr == 13) {
				pos += 2;
				continue;
			}
			if (chr == 1) { // 'Newline'
				curw = 1;
				continue;
			}
			if (chr == 2) // 'Don't terminate with \n'
				break;
			if (chr == 14) {
				int set = str[pos] | (str[pos + 1] << 8);
				pos += 2;
				setCurID(set);
				continue;
			}
		}

		if (chr == ' ')
			lastspace = pos - 1;

		curw += getCharWidth(chr);
		if (lastspace == -1)
			continue;
		if (curw > maxwidth) {
			str[lastspace] = 0xD;
			curw = 1;
			pos = lastspace + 1;
			lastspace = -1;
		}
	}

	setCurID(oldID);
}

void CharsetRenderer::printChar(int chr) {
	int16 width, height, origWidth, origHeight;
	int16 offsX, offsY;
	VirtScreen *vs;
	const byte *charPtr;

	checkRange(_vm->_numCharsets - 1, 1, _curId, "Printing with bad charset %d");
	
	if ((vs = _vm->findVirtScreen(_top)) == NULL && (vs = _vm->findVirtScreen(_top + getFontHeight())) == NULL)
		return;

	if (chr == '@')
		return;

	_vm->_charsetColorMap[1] = _color;

	uint32 charOffs = READ_LE_UINT32(_fontPtr + chr * 4 + 4);
	assert(charOffs < 0x10000);
	if (!charOffs)
		return;
	charPtr = _fontPtr + charOffs;
	
	width = charPtr[0];
	height = charPtr[1];

	if (_disableOffsX) {
		offsX = 0;
	} else {
		offsX = (signed char)charPtr[2];
	}

	offsY = (signed char)charPtr[3];

	charPtr += 4;	// Skip over char header

	origWidth = width;
	origHeight = height;
	
	if (_dropShadow) {
		width++;
		height++;
	}
	if (_firstChar) {
		_str.left = 0;
		_str.top = 0;
		_str.right = 0;
		_str.bottom = 0;
	}

	_top += offsY;
	_left += offsX;

	if (_left + origWidth > _right + 1 || _left < 0) {
		_left += origWidth;
		_top -= offsY;
		return;
	}

	_disableOffsX = false;

	if (_firstChar) {
		_str.left = _left;
		_str.top = _top;
		_str.right = _left;
		_str.bottom = _top;
		_firstChar = false;
	}

	if (_left < _str.left)
		_str.left = _left;

	if (_top < _str.top)
		_str.top = _top;

	int16 drawTop = _top - vs->topline;

	_vm->markRectAsDirty(vs->number, _left, _left + width, drawTop, drawTop + height + offsY);

	if (!vs->hasTwoBuffers)
		_blitAlso = false;
	if (vs->number == kMainVirtScreen && !_ignoreCharsetMask)
		_hasMask = true;


	byte *mask = _vm->getMaskBuffer(_left, drawTop, 0);
	uint32 offs = MUL160(drawTop);
	int16 xpos = vs->xstart + _left;
	byte *dst = vs->screenPtr + offs;

	byte *back = dst;
	if (_blitAlso) {
		dst = vs->backBuf + offs;
	}

	byte bpp = *_fontPtr;
	bool masking = (vs->number == kMainVirtScreen && !_ignoreCharsetMask);
	bool clipping = (drawTop < 0);
	byte flag = bpp | (clipping << 4) | (masking << 5);
	switch(flag)
	{
		case 1:		drawBitsN<1, false, false>(vs, dst, xpos, charPtr, mask, drawTop, origWidth, origHeight);	break;
		case 2:		drawBitsN<2, false, false>(vs, dst, xpos, charPtr, mask, drawTop, origWidth, origHeight);	break;
		case 4:		drawBitsN<4, false, false>(vs, dst, xpos, charPtr, mask, drawTop, origWidth, origHeight);	break;
		case 8:		drawBitsN<8, false, false>(vs, dst, xpos, charPtr, mask, drawTop, origWidth, origHeight);	break;
		case 1+16:	drawBitsN<1,  true, false>(vs, dst, xpos, charPtr, mask, drawTop, origWidth, origHeight);	break;
		case 2+16:	drawBitsN<2,  true, false>(vs, dst, xpos, charPtr, mask, drawTop, origWidth, origHeight);	break;
		case 4+16:	drawBitsN<4,  true, false>(vs, dst, xpos, charPtr, mask, drawTop, origWidth, origHeight);	break;
		case 8+16:	drawBitsN<8,  true, false>(vs, dst, xpos, charPtr, mask, drawTop, origWidth, origHeight);	break;
		case 1+32:	drawBitsN<1, false,  true>(vs, dst, xpos, charPtr, mask, drawTop, origWidth, origHeight);	break;
		case 2+32:	drawBitsN<2, false,  true>(vs, dst, xpos, charPtr, mask, drawTop, origWidth, origHeight);	break;
		case 4+32:	drawBitsN<4, false,  true>(vs, dst, xpos, charPtr, mask, drawTop, origWidth, origHeight);	break;
		case 8+32:	drawBitsN<8, false,  true>(vs, dst, xpos, charPtr, mask, drawTop, origWidth, origHeight);	break;
		case 1+48:	drawBitsN<1,  true,  true>(vs, dst, xpos, charPtr, mask, drawTop, origWidth, origHeight);	break;
		case 2+48:	drawBitsN<2,  true,  true>(vs, dst, xpos, charPtr, mask, drawTop, origWidth, origHeight);	break;
		case 4+48:	drawBitsN<4,  true,  true>(vs, dst, xpos, charPtr, mask, drawTop, origWidth, origHeight);	break;
		case 8+48:	drawBitsN<8,  true,  true>(vs, dst, xpos, charPtr, mask, drawTop, origWidth, origHeight);	break;
	}

	if (_blitAlso)
	{
		debug(1,"blitalso %d,%d -  %d,%d", xpos, drawTop, width, height);
		// TODO_ATARI: make optimized blit function
		int bx = xpos & ~7;
		int bw = ((xpos + width) + 7) & ~7;
		int h = height;

		back += bx;
		dst += bx;

		do {
			memcpy(back, dst, bw);
			back += 160;
			dst += 160;
		} while (--h);
	}
	
	_left += origWidth;

	if (_str.right < _left) {
		_str.right = _left;
		if (_dropShadow)
			_str.right++;
	}

	if (_str.bottom < _top + height)
		_str.bottom = _top + height;

	_top -= offsY;
}



const byte charsetDrawMask[8*8] =
{
	// x_offs + (x_size << 3)
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
	0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01,
	0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x07, 0x03, 0x01,
	0xF0, 0x78, 0x3C, 0x1E, 0x0F, 0x07, 0x03, 0x01,
	0xF8, 0x7C, 0x3E, 0x1F, 0x0F, 0x07, 0x03, 0x01,
	0xFC, 0x7E, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01,
	0xFE, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01,
	0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01,
};

template <byte bpp, bool clipping, bool masking>
void CharsetRenderer::drawBitsN(VirtScreen *vs, byte *dst, int16 xpos, const byte *src, byte *mask, int16 drawTop, int16 width, int16 height)
{
	const byte* colorMap = _vm->_charsetColorMap;
	byte srcBits = *src++;
	int16 numBits = 8;
	int16 xoffs = (xpos & 7);
	uint16 xtile = (xpos & ~7);
	const uint32* c2pcstart = &c2ptable256[xoffs << 8];
	uint32* dstart = (uint32*) (dst + (xtile >> 1));
	byte* mstart = mask;
	byte* m;

	int16 h = ((vs->height - drawTop) < height) ? vs->height - drawTop : height;
	for (int16 y = 0; y < h; y++)
	{
		uint32 px = 0;
		byte pm = 0;
		byte maskmask = charsetDrawMask[xoffs];
		const uint32* c2pc = c2pcstart;
		uint32* d = dstart;
		if (masking)
			m = mstart;

		for (int16 x = 0; x < width; x++)
		{
			byte color;
			if (bpp == 1)	color = srcBits & 0x80;
			if (bpp == 2)	color = srcBits & 0xC0;
			if (bpp == 4)	color = srcBits & 0xF0;
			if (bpp == 8)	color = srcBits;
			if (color && (!clipping || ((y + drawTop) >= 0)))
			{
				#ifdef __ATARI__
					if (bpp == 1)	__asm__ ( "rol.b #1, %0\n\t" : "=d"(color) : "0"(color) : );
					if (bpp == 2)	__asm__ ( "rol.b #2, %0\n\t" : "=d"(color) : "0"(color) : );
					if (bpp == 4)	__asm__ ( "lsr.b #4, %0\n\t" : "=d"(color) : "0"(color) : );
				#else
					if (bpp == 1)	color >>= 7;
					if (bpp == 2)	color >>= 6;
					if (bpp == 4)	color >>= 4;
				#endif
				color = colorMap[color];
				px |= c2pc[color];
				pm |= maskmask;
			}

			maskmask >>= 1;
			if (maskmask == 0)
			{
				maskmask = 0x80;
				c2pc = c2ptable256;
				if (pm)
				{
					if (masking)
						*m |= pm;
					uint32 pm32 = ~rptTable[pm];
					*d = ((*d & pm32) | px);
					pm = 0;
				}
				d++;
				if (masking)
					m++;
				px = 0;
			}
			else
			{
				c2pc += 256;
			}

			if (bpp != 8)
			{
				srcBits <<= bpp;
				numBits -= bpp;
				if (numBits == 0) {
					srcBits = *src++;
					numBits = 8;
				}
			} else {
				srcBits = *src++;
			}

		}
		if (pm)
		{
			if (masking)
				*m |= pm;
			uint32 pm32 = ~rptTable[pm];
			*d = ((*d & pm32) | px);
		}

		dstart += SCREEN_STRIP_COUNT;
		mstart += SCREEN_STRIP_COUNT;
	}
}


} // End of namespace Scumm

