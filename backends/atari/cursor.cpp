//-------------------------------------------------------------------------
// This file is distributed under the GPL v2, or at your option any
// later version.  Read COPYING for details.
// (c)2021, Anders Granlund
//-------------------------------------------------------------------------
#include "cursor.h"
#include "common/util.h"


Cursor::Cursor()
{
	memset(frames, 0, sizeof(Frame) * CURSOR_MAX_FRAMES);
	undrawDest = 0;
	undrawWidth = 0;
	undrawHeight = 0;
	currentFrame = 0;
	overrideFrame = -1;
}

Cursor::~Cursor()
{
	for (int16 i=0; i<CURSOR_MAX_FRAMES; i++)
		if (frames[i].data)
			free(frames[i].data);
}

void Cursor::SetOverrideFrame(uint32 id)
{
	overrideFrame = -1;
	if (id != 0xFFFFFFFF)
	{
		for (int16 i=0; i<CURSOR_MAX_FRAMES; i++)
		{
			if ((frames[i].id == id) && (frames[i].data))
			{
				overrideFrame = i;
				return;
			}
		}
	}
}

bool Cursor::SetFrame(uint32 id)
{
	for (int16 i=0; i<CURSOR_MAX_FRAMES; i++)
	{
		if (frames[i].id == id)
		{
			if (frames[i].data)
			{
				currentFrame = i;
				return true;
			}
			return false;
		}
	}
	return false;
}

bool Cursor::HasFrame(uint32 id)
{
	for (int16 i=0; i<CURSOR_MAX_FRAMES; i++)
		if ((frames[i].id == id) && (frames[i].data != 0))
			return true;
	return false;
}

void Cursor::Undraw()
{
	if (undrawDest)
	{
		const uint32* s = (const uint32*) undrawData;
		uint32* d = (uint32*) undrawDest;
		int16 w = undrawWidth;
		int16 h = undrawHeight;
		undrawDest = 0;
		switch(w)
		{
			case 1:
			{
				// 16 pixels
				while (h)
				{
					*d++ = *s++; *d++ = *s++; d += 38;
					*d++ = *s++; *d++ = *s++; d += 38;
					*d++ = *s++; *d++ = *s++; d += 38;
					*d++ = *s++; *d++ = *s++; d += 38;
					*d++ = *s++; *d++ = *s++; d += 38;
					*d++ = *s++; *d++ = *s++; d += 38;
					*d++ = *s++; *d++ = *s++; d += 38;
					*d++ = *s++; *d++ = *s++; d += 38;
					h--;
				}
			} break;
			case 2:
			{	// 32 pixels
				while (h)
				{
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 36;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 36;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 36;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 36;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 36;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 36;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 36;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 36;
					h--;
				}
			} break;
			case 3:
			{	// 48 pixels
				while (h)
				{
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 34;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 34;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 34;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 34;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 34;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 34;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 34;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 34;
					h--;
				}
			} break;
			case 4:
			{	// 64 pixels
				while (h)
				{
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 32;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 32;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 32;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 32;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 32;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 32;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 32;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 32;
					h--;
				}
			} break;
			case 5:
			{	// 72 pixels
				while (h)
				{
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;	*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 30;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;	*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 30;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;	*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 30;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;	*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 30;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;	*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 30;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;	*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 30;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;	*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 30;
					*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;	*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; d += 30;
					h--;
				}
			} break;
		}
	}
}

void Cursor::CopyUndrawData(byte* screen, int16 x, int16 y, int16 w, int16 h)
{
	const int16 mx = CLAMP<int16>((x & ~15), 0, 320 - w);
	const int16 my = CLAMP<int16>(y, 0, 200 - h);
	w >>= 4;
	h >>= 3;
	undrawWidth = w;
	undrawHeight = h;
	undrawDest = (uint32*)(screen + (mx >> 1) + MUL160(my));
	const uint32* s = undrawDest;
	uint32* d = (uint32*)undrawData;
	switch (w)
	{
		case 1:
		{
			// 16 pixels
			while (h)
			{
				*d++ = *s++; *d++ = *s++; s += 38;
				*d++ = *s++; *d++ = *s++; s += 38;
				*d++ = *s++; *d++ = *s++; s += 38;
				*d++ = *s++; *d++ = *s++; s += 38;
				*d++ = *s++; *d++ = *s++; s += 38;
				*d++ = *s++; *d++ = *s++; s += 38;
				*d++ = *s++; *d++ = *s++; s += 38;
				*d++ = *s++; *d++ = *s++; s += 38;
				h--;
			}
		} break;
		case 2:
		{	// 32 pixels
			while (h)
			{
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 36;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 36;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 36;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 36;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 36;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 36;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 36;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 36;
				h--;
			}
		} break;
		case 3:
		{	// 48 pixels
			while (h)
			{
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 34;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 34;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 34;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 34;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 34;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 34;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 34;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 34;
				h--;
			}
		} break;
		case 4:
		{	// 64 pixels
			while (h)
			{
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 32;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 32;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 32;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 32;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 32;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 32;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 32;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 32;
				h--;
			}
		} break;
		case 5:
		{	// 72 pixels
			while (h)
			{
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;	*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 30;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;	*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 30;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;	*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 30;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;	*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 30;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;	*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 30;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;	*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 30;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;	*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 30;
				*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;	*d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++; s += 30;
				h--;
			}
		} break;		
	}
}

void Cursor::Draw(byte* screen, int16 x, int16 y)
{
	// frame info

	int16 idx = overrideFrame < 0 ? currentFrame : overrideFrame;
	int16 s = frames[idx].size;
	int16 w = frames[idx].width;
	int16 h = frames[idx].height;
	byte* data = frames[idx].data;
	x -= frames[idx].hotspotX;
	y -= frames[idx].hotspotY;

	// early reject
	if (!data || (x > 319) || (y > 199) || ((x + w) <= 0) || ((y + h) <= 0))
		return;

	// make undraw data
	CopyUndrawData(screen, x, y, w, h);

	// clip
	int16 srcOffsetX = 0;
	int16 srcOffsetY = 0;
	int16 srcOffsetS = (x & 15) * s;
	int16 srcPitch   = 0;
	if (y < 0)
	{
		int16 negy = -y;
		srcOffsetY = ((w + (w << 2)) * negy) >> 3;
		h -= negy;
		y = 0;
	}
	else if (y + h > 200)
	{
		h = 200 - y;
	}

	int16 ax = x & ~0xF;
	if (x < 0)
	{
		int16 negx = -x;
		w = ((w - negx) + 0) & ~0xF;
		srcPitch = (negx + 15) >> 4;
		srcPitch += (srcPitch << 2);
		srcOffsetX = srcPitch << 1;
		ax = 0;
	}
	else if ((ax + w) > 320)
	{
		srcPitch = (((ax + w) - 320) + 15) >> 4;
		srcPitch += (srcPitch << 2);
		w = (320 - ax);
	}

	// draw
	int16 dstOffsetX = ax >> 1;
	int16 dstOffsetY = MUL160(y);
	uint16* dst = (uint16*) (screen + dstOffsetX + dstOffsetY);
	uint16* src = (uint16*) (data + srcOffsetS + srcOffsetX + srcOffsetY);

	int16 dstPitch = (320 - w) >> 2;
	for (int16 ty=0; ty<h; ty++)
	{
		for (int16 tx=0; tx<(w>>4); tx++)
		{
			uint16 mask = *src++;
			*dst++ = (*dst & ~mask) | (*src++ & mask);
			*dst++ = (*dst & ~mask) | (*src++ & mask);
			*dst++ = (*dst & ~mask) | (*src++ & mask);
			*dst++ = (*dst & ~mask) | (*src++ & mask);
		}
		src += srcPitch;
		dst += dstPitch;
	}
}

Cursor::Frame* Cursor::GetFrame(uint32 id)
{
	for (int16 i=0; i<CURSOR_MAX_FRAMES; i++)
		if (frames[i].data && frames[i].id == id)
			return &frames[i];
	return 0;
}

Cursor::Frame* Cursor::CreateFrame(int16 idx, uint32 id, int16 w, int16 h)
{
	if (idx >= CURSOR_MAX_FRAMES)
		return 0;
		
	int16 fw = ((w + 15) & ~15) + 16;
	int16 fh = h;
	Frame* frame = GetFrame(id);
	if (frame)
	{
		if (idx >= 0 && frame != &frames[idx]) {
			// found id but at wrong position, remove old
			free(frame->data);
			frame->data = 0;
			frame = 0;
		} else {
			// found it, return if same size else delete old data
			if (frame->width == fw && frame->height == fh)
				return frame;
			free(frame->data);
		}
	}

	if (!frame)
	{
		if (idx >= 0) {
			// replace whatever is at the index we want
			frame = &frames[idx];
			if (frame->data) {
				free(frame->data);
			}
		} else {
			// try to find an empty slot
			for (int16 i=0; i<CURSOR_MAX_FRAMES && !frame; i++) {
				if (frames[i].data == 0) {
					frame = &frames[i];
				}
			}
		}
	}
	if (!frame)
		return 0;
	int16 size = ((fw * fh) >> 3);	// size for 1 plane
	size += (size << 2);			// size for 5 planes
	int16 totalsize = (size << 4);	// 16 sprites
	frame->data = (byte*) malloc(totalsize);
	memset(frame->data, 0, size);
	if (!frame->data)
		return 0;
	frame->width = fw;
	frame->height = fh;
	frame->size = size;
	frame->id = id;
	frame->hotspotX = frame->width >> 1;
	frame->hotspotY = frame->height >> 1;
	return frame;
}

bool Cursor::CreateFromPlanar(int16 idx, uint32 id, const byte* buf, const byte* mask, int16 w, int16 h, int16 hotx, int16 hoty, int16 pitch)
{
	w = ((w + 15) & ~0xF);
	Frame* frame = CreateFrame(idx, id, w, h);
	if (!frame)
		return false;

	uint16* s = (uint16*) buf;
	uint16* m = (uint16*) mask;
	uint16* d = (uint16*) frame->data;

	pitch = (w - pitch) >> 2;
	for (int16 y=0; y<h; y++)
	{
		for (int16 x=0; x<(w>>4); x++)
		{
			*d++ = m ? *m++ : 0xFFFF;
			*d++ = *s++;
			*d++ = *s++;
			*d++ = *s++;
			*d++ = *s++;
		} 
		*d++ = 0;	// mask
		*d++ = 0;	// plane0
		*d++ = 0;	// plane1
		*d++ = 0;	// plane2
		*d++ = 0;	// plane3
		s += pitch;
		m += pitch;
	}

	frame->hotspotX = hotx;
	frame->hotspotY = hoty;

	// preshift
	Preshift(frame);
	return true;
}

void Cursor::Preshift(Frame* frame)
{
	int16 w = frame->width >> 4;
	int16 h = frame->height;
	uint16* src = (uint16*) frame->data;
	uint16* dst = (uint16*) (frame->data + frame->size);
	for (int16 i=1; i<16; ++i)
	{
		for (int16 y=0; y<h; y++)
		{
			uint16 p, c0, c1, c2, c3, c4;
			c0 = c1 = c2 = c3 = c4 = 0;
			for (int16 x=0; x<w; x++)
			{
				p = *src++; *dst++ = (p >> 1) | c0; c0 = (p & 1) << 15;
				p = *src++; *dst++ = (p >> 1) | c1; c1 = (p & 1) << 15;
				p = *src++; *dst++ = (p >> 1) | c2; c2 = (p & 1) << 15;
				p = *src++; *dst++ = (p >> 1) | c3; c3 = (p & 1) << 15;
				p = *src++; *dst++ = (p >> 1) | c4; c4 = (p & 1) << 15;
			}
		}
	}
}

