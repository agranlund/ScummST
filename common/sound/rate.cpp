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
 * $Header: /cvsroot/scummvm/scummvm/sound/rate.cpp,v 1.34 2004/01/06 12:45:33 fingolfin Exp $
 *
 */

/*
 * The code in this file is based on code with Copyright 1998 Fabrice Bellard
 * Fabrice original code is part of SoX (http://sox.sourceforge.net).
 * Max Horn adapted that code to the needs of ScummVM and rewrote it partial,
 * in the process removing any use of floating point arithmetic. Various other
 * improvments over the original code were made.
 */

#include "stdafx.h"
#include "sound/rate.h"
#include "sound/audiostream.h"

/**
 * The precision of the fractional computations used by the rate converter.
 * Normally you should never have to modify this value.
 */
#define FRAC_BITS 16

/**
 * The size of the intermediate input cache. Bigger values may increase
 * performance, but only until some point (depends largely on cache size,
 * target processor and various other factors), at which it will decrease
 * again.
 */
#define INTERMEDIATE_BUFFER_SIZE 512


/**
 * Audio rate converter based on simple linear Interpolation.
 *
 * The use of fractional increment allows us to use no buffer. It
 * avoid the problems at the end of the buffer we had with the old
 * method which stored a possibly big buffer of size
 * lcm(in_rate,out_rate).
 *
 * Limited to sampling frequency <= 65535 Hz.
 */

class LinearRateConverter : public RateConverter {
protected:
	uint32 step;

public:
	LinearRateConverter(st_rate_t inrate, st_rate_t outrate);
	int flow(AudioStream &input, st_sample_t *obuf, st_size_t osamp, st_volume_t vol_l, st_volume_t vol_r);
	int drain(st_sample_t *obuf, st_size_t osamp, st_volume_t vol) {
		return (ST_SUCCESS);
	}
};


LinearRateConverter::LinearRateConverter(st_rate_t inrate, st_rate_t outrate) {

	if (inrate == outrate) {
		error("Input and Output rates must be different to use rate effect");
	}

	if (inrate >= 65536 || outrate >= 65536) {
		error("rate effect can only handle rates < 65536");
	}
	step = (inrate << 8) / outrate;
}


int LinearRateConverter::flow(AudioStream &input, st_sample_t *obuf, st_size_t osamp, st_volume_t vol_l, st_volume_t vol_r) {
	input.readBufferConv(obuf, osamp, step);
	return (ST_SUCCESS);
}


#pragma mark -


/**
 * Simple audio rate converter for the case that the inrate equals the outrate.
 */
class CopyRateConverter : public RateConverter {
public:
	CopyRateConverter() {
	}

	~CopyRateConverter() {
	}

	virtual int flow(AudioStream &input, st_sample_t *obuf, st_size_t osamp, st_volume_t vol_l, st_volume_t vol_r) {
		input.readBuffer(obuf, osamp);
		return (ST_SUCCESS);
	}
	virtual int drain(st_sample_t *obuf, st_size_t osamp, st_volume_t vol) {
		return (ST_SUCCESS);
	}
};


#pragma mark -


/**
 * Create and return a RateConverter object for the specified input and output rates.
 */
RateConverter *makeRateConverter(st_rate_t inrate, st_rate_t outrate)
{
	if (inrate == outrate)
		return new CopyRateConverter();

	return new LinearRateConverter(inrate, outrate);
}
