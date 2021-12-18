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
 * $Header: /cvsroot/scummvm/scummvm/sound/rate.h,v 1.24 2004/01/06 12:45:33 fingolfin Exp $
 *
 */

#ifndef SOUND_RATE_H
#define SOUND_RATE_H

#include "common/scummsys.h"
#include "common/engine.h"
#include "common/util.h"

//#include "sound/audiostream.h"
class AudioStream;

typedef uint8 st_sample_t;
typedef uint16 st_volume_t;
typedef uint32 st_size_t;
typedef uint32 st_rate_t;

/* Minimum and maximum values a sample can hold. */
#define ST_SAMPLE_MAX 0xFF
#define ST_SAMPLE_MIN 0

#define ST_EOF (-1)
#define ST_SUCCESS (0)

static inline void clampedAdd(uint8& a, int b) {
	register int val;
	val = a + b;

	if (val > ST_SAMPLE_MAX)
		val = ST_SAMPLE_MAX;
	else if (val < ST_SAMPLE_MIN)
		val = ST_SAMPLE_MIN;

	a = val;
}

// Q&D hack to get this SOX stuff to work
#define st_report warning
#define st_warn warning
#define st_fail error


class RateConverter {
public:
	RateConverter() {}
	virtual ~RateConverter() {}
	virtual int flow(AudioStream &input, st_sample_t *obuf, st_size_t osamp, st_volume_t vol_l, st_volume_t vol_r) = 0;
	virtual int drain(st_sample_t *obuf, st_size_t osamp, st_volume_t vol) = 0;
};

RateConverter *makeRateConverter(st_rate_t inrate, st_rate_t outrate);

#endif
