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

#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#include "common/scummsys.h"
#include "common/system.h"

extern uint16 squareTable[256];
extern uint32 rptTable[256];
extern uint32 c2pfill16[];
extern uint32 c2pfill16_2[];

#define SCREEN_WIDTH			320
#define SCREEN_HEIGHT			200
#define SCREEN_STRIP_COUNT		40
#define SCREEN_STRIP_SIZE	 	8
#define SCREEN_STRIP_SHIFT	 	3
#define SCREEN_TO_STRIP(a)		((a) >> SCREEN_STRIP_SHIFT)
#define STRIP_TO_SCREEN(a)		((a) << SCREEN_STRIP_SHIFT)
#define MUL_SCREEN_WIDTH(a)		MUL320(a)
#define MUL_SCREEN_STRIPS(a)	MUL40(a)

#define MULSQUARE(x)	squareTable[(x)]
#define MUL2(a)			((a)<<1)
#define MUL3(a)			(((a)<<1)+(a))
#define MUL6(a)			(((a)<<2)+((a)<<1))
#define MUL8(a)			((a)<<3)
#define MUL20(a)		(((a)<<4)+((a)<<2))
#define MUL40(a)		(((a)<<5)+((a)<<3))
#define MUL80(a)		(((a)<<6)+((a)<<4))
#define MUL160(a)		(((a)<<7)+((a)<<5))
#define MUL320(a)		(((a)<<8)+((a)<<6))
#define MUL512(a)		((a)<<9)

#if 1
#define FILLUINT32(x)			rptTable[x]
#else
inline uint32 FILLUINT32(uint8 x)
{
	return rptTable[x];
}
#endif

template<typename T> inline T ABS (T x)				{ return (x>=0) ? x : -x; }
template<typename T> inline T MIN (T a, T b)		{ return (a<b) ? a : b; }
template<typename T> inline T MAX (T a, T b)		{ return (a>b) ? a : b; }
template<typename T> inline T CLAMP(T a, T b, T c)	{ return (a<b) ? b : (a>c) ? c : a; }

/**
 * Template method which swaps the vaulues of its two parameters.
 */
template<typename T> inline void SWAP(T &a, T &b) { T tmp = a; a = b; b = tmp; }


#ifndef ARRAYSIZE
#define ARRAYSIZE(x) ((int)(sizeof(x) / sizeof(x[0])))
#endif

namespace Common {


namespace Util {
void Init();
};

class String;

/**
 * Simple random number generator. Although it is definitely not suitable for
 * cryptographic purposes, it serves our purposes just fine.
 */
class RandomSource {
private:
	uint32 _randSeed;

public:
	RandomSource();
	void setSeed(uint32 seed);

	/**
	 * Generates a random unsigned integer in the interval [0, max].
	 * @param max	the upper bound
	 * @return	a random number in the interval [0, max].
	 */
	uint getRandomNumber(uint max);
	/**
	 * Generates a random unsigned integer in the interval [min, max].
	 * @param min	the lower bound
	 * @param max	the upper bound
	 * @return	a random number in the interval [min, max].
	 */
	uint getRandomNumberRng(uint min, uint max);
};

/**
 * Auxillary class to (un)lock a mutex on the stack.
 */
class StackLock {
	OSystem::MutexRef _mutex;
	OSystem *_syst;
	const char *_mutexName;

	void lock();
	void unlock();
public:
	StackLock(OSystem::MutexRef mutex, OSystem *syst = 0, const char *mutexName = NULL);
	~StackLock();
};

/**
 * List of language ids.
 * @note The order and mappings of the values 0..8 are *required* to stay the
 * way they are now, as scripts in COMI rely on them. So don't touch them.
 * I am working on removing this restriction.
 */
enum Language {
	UNK_LANG = -1,	// Use default language (i.e. none specified)
	EN_USA = 0,
	DE_DEU = 1,
	FR_FRA = 2,
	IT_ITA = 3,
	PT_BRA = 4,
	ES_ESP = 5,
	JA_JPN = 6,
	ZH_TWN = 7,
	KO_KOR = 8,
	SE_SWE = 9,
	EN_GRB = 10,
	HB_ISR = 20,
	RU_RUS = 21,
	CZ_CZE = 22,
	NL_NLD = 23
};


/**
 * List of game platforms. Specifying a platform for a target can be used to
 * give the game engines a hint for which platform the game data file are.
 * This may be optional or required, depending on the game engine and the
 * game in question.
 */
enum Platform {
	kPlatformUnknown = -1,
	kPlatformPC = 0,
	kPlatformAmiga = 1,
	kPlatformAtariST = 2,
	kPlatformMacintosh = 3,
	kPlatformFMTowns = 4
/*
	kPlatformNES,
	kPlatformSEGA,
	kPlatformPCEngine
*/
};

}	// End of namespace Common


#ifndef RELEASEBUILD
	void CDECL error(const char *s, ...) NORETURN;
	void CDECL warning(const char *s, ...);
	void CDECL debug(int level, const char *s, ...);
	void CDECL debug(const char *s, ...);
	void checkHeap();
	#define assertAligned(a) { if(1 & (uint32)a) { debug(1, "FATAL: '" #a "' is not aligned (0x%08x). " __FILE__ ":%d", (uint32)a, __LINE__); exit(0); } }
#else
	#define error(...)
	#define warning(...)
	#define debug(...)
	#define checkHeap()
	#define assertAligned(a) {}
#endif


#endif
