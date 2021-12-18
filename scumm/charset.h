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
 * $Header: /cvsroot/scummvm/scummvm/scumm/charset.h,v 2.25 2004/01/08 21:21:40 fingolfin Exp $
 */

#ifndef CHARSET_H
#define CHARSET_H

#include "common/rect.h"
#include "common/scummsys.h"

namespace Scumm {

class ScummEngine;
struct VirtScreen;

class CharsetRenderer {
public:
	CharsetRenderer(ScummEngine *vm);
	~CharsetRenderer() {}

	void restoreCharsetBg();
	void clearCharsetMask();
	bool hasCharsetMask(int left, int top, int right, int bottom);

	void printChar(int chr);

	int getStringWidth(int a, const byte *str);
	void addLinebreaks(int a, byte *str, int pos, int maxwidth);
	
	void setCurID(byte id);
	int getCurID() { return _curId; }

	int getFontHeight() { return _fontPtr[1]; }
	
	void setColor(byte color)	{ _color = color; }

public:
	byte _ymask[SCREEN_HEIGHT];
	Common::Rect _mask;
	Common::Rect _str;
	int _nextLeft, _nextTop;
	int _top;
	int _left, _startLeft;
	int _right;
	bool _center;
	bool _hasMask;
	bool _ignoreCharsetMask;
	bool _blitAlso;
	bool _firstChar;
	bool _disableOffsX;

protected:
	ScummEngine *_vm;
	byte *_fontPtr;
	byte _curId;
	byte _color;
	byte _shadowColor;
	bool _dropShadow;

	void updateColorMap();
	int getCharWidth(byte chr);
	template<byte bpp, bool clipping, bool masking> void drawBitsN(VirtScreen *vs, byte *dst, int16 xpos, const byte *src, byte *mask, int16 drawTop, int16 width, int16 height);
};


} // End of namespace Scumm


#endif
