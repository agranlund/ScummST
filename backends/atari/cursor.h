//-------------------------------------------------------------------------
// This file is distributed under the GPL v2, or at your option any
// later version.  Read COPYING for details.
// (c)2021, Anders Granlund
//-------------------------------------------------------------------------
#ifndef _SPRITE_H_
#define _SPRITE_H_

#include "common/stdafx.h"
#include "common/scummsys.h"

#define CURSOR_MAX_FRAMES	16
#define CURSOR_MAX_WIDTH	64
#define CURSOR_MAX_HEIGHT	32

class Cursor
{
public:
	Cursor();
	~Cursor();
	bool CreateFromPlanar(int16 idx, uint32 id, const byte* buf, const byte* mask, int16 w, int16 h, int16 hotx, int16 hoty, int16 pitch);

	void Draw(byte* screen, int16 x, int16 y);
	void Undraw();
	bool IsDrawn() { return undrawDest ? true : false; }

	bool SetFrame(uint32 id);
	bool HasFrame(uint32 id);

	uint32	Id()		{ return frames[currentFrame].id; }
	int16	Width() 	{ return frames[currentFrame].width; }
	int16	Height()	{ return frames[currentFrame].height; }
	int16	HotspotX()	{ return frames[currentFrame].hotspotX; }
	int16	HotspotY()	{ return frames[currentFrame].hotspotY; }


	void SetOverrideFrame(uint32 id = 0xFFFFFFFF);
	void ClearOverrideFrame() { SetOverrideFrame(0xFFFFFFFF); }

private:
	struct Frame
	{
		uint32	id;
		byte*	data;
		uint16	size;
		byte	width;
		byte	height;
		byte	hotspotX;
		byte	hotspotY;
	};
	Frame frames[CURSOR_MAX_FRAMES];

	int16	overrideFrame;
	int16	currentFrame;
	int16	undrawWidth;		// in 16 pixel steps
	int16	undrawHeight;		// in  8 pixel steps
	uint32*	undrawDest;
	byte	undrawData[(CURSOR_MAX_WIDTH+16) * CURSOR_MAX_HEIGHT / 2];

	void 			CopyUndrawData(byte* screen, int16 x, int16 y, int16 w, int16 h);
	void			Preshift(Frame* frame);
	int16			GetFrameIdx(uint32 id);
	Cursor::Frame*	GetFrame(uint32 id);
	Cursor::Frame*	CreateFrame(int16 idx, uint32 id, int16 w, int16 h);
};


#endif // _SPRITE_H_

