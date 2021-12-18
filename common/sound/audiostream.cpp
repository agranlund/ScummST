/* ScummVM - Scumm Interpreter
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
 * $Header: /cvsroot/scummvm/scummvm/sound/audiostream.cpp,v 1.55 2004/02/08 17:19:09 ender Exp $
 *
 */

#include "stdafx.h"
#include "common/file.h"
#include "common/util.h"
#include "sound/audiostream.h"
#include "sound/mixer.h"



#pragma mark -
#pragma mark --- LinearMemoryStream ---
#pragma mark -



/**
 * A simple raw audio stream, purely memory based. It operates on a single
 * block of data, which is passed to it upon creation. 
 * Optionally supports looping the sound.
 *
 * Design note: This code tries to be as optimiized as possible (without
 * resorting to assembly, that is). To this end, it is written as a template
 * class. This way the compiler can actually create optimized code for each
 * special code. This results in a total of 12 versions of the code being
 * generated.
 */
class LinearMemoryStream : public AudioStream {
protected:
	const byte *_ptr;
	const byte *_origPtr;
	const int _rate;

	uint32 _pos;
	uint32 _end;
	uint32 _loopStart;
	uint32 _loopEnd;

	inline bool eosIntern() const	{ return _pos >= _end; };
public:
	LinearMemoryStream(int rate, const byte *ptr, uint len, uint loopOffset, uint loopLen, bool autoFreeMemory)
	 : _ptr(ptr), _rate(rate)
	{
		_pos = 0;
		_end = (len << 8);

		if (loopLen)
		{
			_loopStart = (loopOffset << 8);
			_loopEnd = (loopOffset + loopLen) << 8;
		}
		else
		{
			_loopStart = 0;
			_loopEnd = 0;
		}
		
		_origPtr = autoFreeMemory ? ptr : 0;
	}
	~LinearMemoryStream() {
		free(const_cast<byte *>(_origPtr));
	}
	int readBuffer(uint8 *buffer, const int numSamples);
	int readBufferConv(uint8 *buffer, const int numSamples, const uint32 step);

	inline bool endOfData() const		{ return eosIntern(); }
	inline int getRate() const			{ return _rate; }
};


int LinearMemoryStream::readBuffer(uint8 *buffer, const int numSamples) {
	byte* bufend = buffer + numSamples;
	while ((buffer < bufend) && !eosIntern())
	{
		uint32 len = bufend - buffer;
		uint32 avail = (_end - _pos) >> 8;
		if (avail <= (1 << 8))
		{
			_pos = _end;
		}
		else
		{
			if (avail < len)
				len = avail;

			#ifdef WRITESAMPLE
			#undef WRITESAMPLE
			#endif
			#ifdef __ATARI__
				#define WRITESAMPLE()	*buffer++ = *src++ + 0x80;
			#else
				#define WRITESAMPLE()	*buffer++ = *src++;
			#endif

			const byte* src = _ptr + (_pos >> 8);
			_pos += (len << 8);
			while(len > 127)
			{
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				len -= 128;
			}
			while(len > 31)
			{
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				len -= 32;
			}
			
			while(len > 0)
			{
				WRITESAMPLE();
				len--;
			}
		}
		if (eosIntern())
		{
			if (_loopStart)
			{
				_pos = _loopStart;
				_end = _loopEnd;
			}
			else
			{
				_pos = _end;
				break;
			}
		}
	}

	// todo: might remove
	if (buffer < bufend)
	{
		memset(buffer, 0, bufend - buffer);
	}

	// paranoia
	if (_pos > _end)
		_pos = _end;

	//if ((_end - _pos) < (2 << 8))
	//	_pos = _end;

	return bufend - buffer;	
}

int LinearMemoryStream::readBufferConv(uint8 *buffer, const int numSamples, const uint32 step)
{
	byte* bufend = buffer + numSamples;
	while ((buffer < bufend) && !eosIntern())
	{
		uint32 len = bufend - buffer;
		uint32 avail = (step < (1<<8)) ? (((_end - _pos) >> 8) * step) >> 8 : (_end - _pos) / step;
		
		if (avail <= (1<<8))
		{
			_pos = _end;
		}
		else
		{
			if (avail < len)
				len = avail;

			#ifdef WRITESAMPLE
			#undef WRITESAMPLE
			#endif
			#ifdef __ATARI__
				#define WRITESAMPLE()	*buffer++ = _ptr[_pos >> 8] + 0x80; _pos += step;
			#else
				#define WRITESAMPLE()	*buffer++ = _ptr[_pos >> 8]; _pos += step;
			#endif

			while(len > 127)
			{
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				len -= 128;
			}
			while(len > 31)
			{
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE(); WRITESAMPLE();
				len -= 32;
			}
			
			while(len > 0)
			{
				WRITESAMPLE();
				len--;
			}
		}

		if (eosIntern())
		{
			if (_loopStart)
			{
				_pos = _loopStart;
				_end = _loopEnd;
			}
			else
			{
				_pos = _end;
				break;
			}
		}
	}

	// todo: might remove
	/*
	if (buffer < bufend)
	{
		memset(buffer, 0, bufend - buffer);
	}
	*/
	// paranoia
	if (_pos > _end)
		_pos = _end;

	//if ((_end - _pos) < (2 << 8))
	//	_pos = _end;

	return bufend - buffer;
}


#pragma mark -
#pragma mark --- Input stream factories ---
#pragma mark -

/* In the following, we use preprocessor / macro tricks to simplify the code
 * which instantiates the input streams. We used to use template functions for
 * this, but MSVC6 / EVC 3-4 (used for WinCE builds) are extremely buggy when it
 * comes to this feature of C++... so as a compromise we use macros to cut down
 * on the (source) code duplication a bit.
 * So while normally macro tricks are said to make maintenance harder, in this
 * particular case it should actually help it :-)
 */

AudioStream *makeLinearInputStream(int rate, byte _flags, const byte *ptr, uint32 len, uint loopOffset, uint loopLen) {
	const bool isStereo   = (_flags & SoundMixer::FLAG_STEREO) != 0;
	const bool is16Bit    = (_flags & SoundMixer::FLAG_16BITS) != 0;
	const bool isUnsigned = (_flags & SoundMixer::FLAG_UNSIGNED) != 0;
	const bool autoFree   = (_flags & SoundMixer::FLAG_AUTOFREE) != 0;

	debug(1,"mlis: r=%d, s=%d, 16bit=%d, us=%d", rate, isStereo, is16Bit, isUnsigned);

	if (isStereo || is16Bit)
		return NULL;
	
	return new LinearMemoryStream(rate, ptr, len, loopOffset, loopLen, autoFree);
}
