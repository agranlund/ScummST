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
 * $Header: /cvsroot/scummvm/scummvm/scumm/gfx.h,v 1.59 2004/01/08 21:21:40 fingolfin Exp $
 *
 */

#ifndef GFX_H
#define GFX_H

#include "common/rect.h"
#include "common/dirty.h"

extern uint32 c2ptable256[];
extern uint32 c2pfill256[];

namespace Scumm {

class ScummEngine;

/** Camera modes */
enum {
	kNormalCameraMode = 1,
	kFollowActorCameraMode = 2,
	kPanningCameraMode = 3
};

/** Camera state data */
struct CameraData {
	Common::Point _cur;
	Common::Point _dest;
	Common::Point _accel;
	Common::Point _last;
	int _leftTrigger, _rightTrigger;
	byte _follows, _mode;
	bool _movingToActor;
};

/** Virtual screen identifiers */
enum VirtScreenNumber {
	kMainVirtScreen = 0,	// The 'stage'
	kTextVirtScreen = 1,	// In V1-V3 games: the area where text is printed
	kVerbVirtScreen = 2,	// The verb area
	kUnkVirtScreen = 3		// ?? Not sure what this one is good for...
};

/**
 * In all Scumm games, one to four virtual screen (or 'windows') together make
 * up the content of the actual screen. Thinking of virtual screens as fixed
 * size, fixed location windows might help understanding them. Typical, in all
 * scumm games there is either one single virtual screen covering the entire
 * real screen (mostly in all newer games, e.g. Sam & Max, and all V7+ games).
 * The classic setup consists of three virtual screens: one at the top of the
 * screen, where all conversation texts are printed; then the main one (which
 * I like calling 'the stage', since all the actors are doing their stuff
 * there), and finally the lower part of the real screen is taken up by the
 * verb area.
 * Finally, in V5 games and some V6 games, it's almost the same as in the
 * original games, except that there is no separate conversation area.
 *
 * If you now wonder what the last screen is/was good for: I am not 100% sure,
 * but it appears that it was used by the original engine to display stuff
 * like the pause message, or questions ("Do you really want to restart?").
 * It seems that it is not used at all by ScummVM, so we probably could just
 * get rid of it and save a couple kilobytes of RAM.
 *
 * Each of these virtual screens has a fixed number or id (see also
 * \ref VirtScreenNumber).
 */
struct VirtScreen {
	/**
	 * The unique id of this screen (corresponds to its position in the
	 * ScummEngine:virtscr array).
	 */
	VirtScreenNumber number;
	
	/**
	 * Vertical position of the virtual screen. Tells how much the virtual
	 * screen is shifted along the y axis relative to the real screen.
	 * If you wonder why there is no horizontal position: there is none,
	 * because all virtual screens are always exactly as wide as the
	 * real screen. This might change in the future to allow smooth
	 * horizontal scrolling in V7-V8 games.
	 */
	uint16 topline;
	
	/** Width of the virtual screen (currently always identical to SCREEN_WIDTH). */
	uint16 width;

	/** Height of the virtual screen. */
	uint16 height;

	/**
	 * Horizontal scroll offset, tells how far the screen is scrolled to the
	 * right. Only used for the main screen. After all, verbs and the
	 * conversation text box don't have to scroll.
	 */
	uint16 xstart;

	/**
	 * Flag indicating  which tells whether this screen has a back buffer or
	 * not. This is yet another feature which is only used by the main screen.
	 * Strictly spoken one could remove this variable and replace checks
	 * on it with checks on backBuf. But since some code needs to temporarily
	 * disable the backBuf (so it can abuse drawBitmap; see drawVerbBitmap()
	 * and useIm01Cursor()), we keep it (at least for now).
	 */
	bool hasTwoBuffers;
	
	/**
	 * Pointer to the screen's data buffer. This is where the content of
	 * the screen is stored. Just as one would expect :-).
	 */
	byte *screenPtr;
	
	/**
	 * Pointer to the screen's back buffer, if it has one (see also
	 * the hasTwoBuffers member).
	 * The backBuf is used by drawBitmap to store the background graphics of
	 * the active room. This eases redrawing: whenever a portion of the screen
	 * has to be redrawn, first a copy from the backBuf content to screenPtr is
	 * performed. Then, any objects/actors in that area are redrawn atop that.
	 */
	byte *backBuf;

	/**
	 * Array containing for each visible strip of this virtual screen the
	 * coordinate at which the dirty region of that strip starts.
	 */
	Common::DirtyMask dirty;
	void setDirtyRange(int top, int bottom) {
		dirty.setRange(top, bottom);
	}

	void setDirty(int strip, int top, int bottom) {
		dirty.setStrip(strip, top, bottom);
	}
	
	void updateDirty(int strip, int top, int bottom) {
		dirty.updateStrip(strip, top, bottom);
	}


	/*
	uint16 dirty[SCREEN_STRIP_COUNT];
	#define DIRTY_TILE_HEIGHT		1
	#if DIRT_TILE_HEIGHT == 1
		#define DIRTY_ADJUST_TOP(_y)	(byte)CLAMP<int>(_y, 0, 255)	
		#define DIRTY_ADJUST_BOT(_y)	(byte)CLAMP<int>(_y, 0, 255)	
	#else
		#define DIRTY_ADJUST_TOP(_y)	(byte)CLAMP<int>((_y&~(DIRTY_TILE_HEIGHT-1)), 0, 255)	
		#define DIRTY_ADJUST_BOT(_y)	(byte)CLAMP<int>(((_y+(DIRTY_TILE_HEIGHT-1))&~(DIRTY_TILE_HEIGHT-1)), 0, 255)	
	#endif

	void setDirtyRange(int top, int bottom) {
		byte t = DIRTY_ADJUST_TOP(top);
		byte b = DIRTY_ADJUST_BOT(bottom);
		byte* dirt = (byte*)dirty;
		byte count = SCREEN_STRIP_COUNT;
		while(count)
		{
			*dirt++ = t;
			*dirt++ = b;
			count--;
		}
	}

	void setDirty(int strip, int top, int bottom) {
		if (strip < 0 || strip >= SCREEN_STRIP_COUNT)
			return;
		byte* dirt = (byte*)&dirty[strip];
		*dirt++ = DIRTY_ADJUST_TOP(top);
		*dirt++ = DIRTY_ADJUST_BOT(bottom);
	}
	
	void updateDirty(int strip, int top, int bottom) {
		if (strip < 0 || strip >= SCREEN_STRIP_COUNT)
			return;
		byte* dirt = (byte*)&dirty[strip];
		byte t = *(dirt+0);
		byte b = *(dirt+1);
		byte nt = DIRTY_ADJUST_TOP(top);
		byte nb = DIRTY_ADJUST_BOT(bottom);
		if (nt < t)	*(dirt+0) = nt;
		if (nb > b) *(dirt+1) = nb;
	}
	*/
};

/** Palette cycles */
struct ColorCycle {
	uint16 delay;
	uint16 counter;
	uint16 flags;
	byte start;
	byte end;
};

/** BlastObjects to draw */
struct BlastObject {
	uint16 number;
	Common::Rect rect;
	uint16 scaleX, scaleY;
	uint16 image;
	uint16 mode;
};

/** Bomp graphics data, used as parameter to ScummEngine::drawBomp. */
struct BompDrawData {
	byte *out;
	uint outoffs;
	int16 outwidth, outheight;
	int16 x, y;
	byte scale_x, scale_y;
	const byte *dataptr;
	int16 srcwidth, srcheight;
	uint16 shadowMode;

	int16 scaleRight, scaleBottom;
	byte *scalingXPtr, *scalingYPtr;
	byte *maskPtr;
	
	BompDrawData() { memset(this, 0, sizeof(*this)); }
};

struct StripTable;

class Gdi {
	friend class ScummEngine;	// Mostly for the code in saveload.cpp ...
	friend class AtariResourceConverter;
	ScummEngine *_vm;

public:
	int _numZBuffer;
	int _imgBufOffs[8];
	
	Gdi(ScummEngine *vm);

protected:
	byte *_roomPalette;
	byte _decomp_shr, _decomp_mask;
	byte _transparentColor;
	uint32 _horizStripNextInc;
	byte _tempDecodeBuffer[8*200];
	byte _tempDecodeMask[200];
	bool _zbufferDisabled;

	/* Bitmap decompressors */
	byte convertBitmap(byte* dst, const byte* src, int numLinesToProcess);
	bool decompressBitmap(byte *bgbak_ptr, const byte *src, int16 numLinesToProcess, int16 startLine = 0);
	template <bool preprocess> void unkDecodeA(byte *dst, const byte *src, int height);
	template <bool preprocess> void unkDecodeB(byte *dst, const byte *src, int height);
	template <bool preprocess> void unkDecodeC(byte *dst, const byte *src, int height);
	template <bool preprocess> void unkDecode7(byte *dst, const byte *src, int height);
	template <bool preprocess> void unkDecodeA_trans(byte *dst, const byte *src, int height);
	template <bool preprocess> void unkDecodeB_trans(byte *dst, const byte *src, int height);
	template <bool preprocess> void unkDecodeC_trans(byte *dst, const byte *src, int height);

	void draw8ColWithMasking(byte *dst, const byte *src, int height, byte *mask);
	void draw8Col(byte *dst, const byte *src, int height);
	void clear8ColWithMasking(byte *dst, int height, byte *mask);
	void clear8Col(byte *dst, int height);
	void decompressMaskImgOr(byte *dst, const byte *src, int16 height, int16 yoffs);
	void decompressMaskImg(byte *dst, const byte *src, int16 height, int16 yoffs);

	void updateDirtyScreen(VirtScreen *vs);
	
	byte *getMaskBuffer(int x, int y, int z = 0);


public:

	void drawBitmap(const byte *ptr, VirtScreen *vs, int16 x, int16 y, const int16 width, const int16 height, int16 stripnr, int16 numstrip, int16 yoffs, byte flag);
	void disableZBuffer() { _zbufferDisabled = true; }
	void enableZBuffer() { _zbufferDisabled = false; }

	void resetBackground(int top, int bottom, int strip);

	enum DrawBitmapFlags {
		dbAllowMaskOr = 1,
		dbDrawMaskOnAll = 2,
		dbClear = 4
	};
};


} // End of namespace Scumm

#endif
