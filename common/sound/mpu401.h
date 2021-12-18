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
 * $Header: /cvsroot/scummvm/scummvm/sound/mpu401.h,v 1.18 2004/01/06 12:45:33 fingolfin Exp $
 *
 */

#ifndef SOUND_MPU401_H
#define SOUND_MPU401_H

#include "stdafx.h"
#include "common/system.h"
#include "mididrv.h"

////////////////////////////////////////
//
// Common MPU401 implementation methods
//
////////////////////////////////////////

class MidiDriver_MPU401;

class MidiChannel_MPU401 : public MidiChannel {
	friend class MidiDriver_MPU401;

private:
	MidiDriver_MPU401 *_owner;
	bool _allocated;
	byte _channel;

	void init (MidiDriver_MPU401 *owner, byte channel);
	void allocate() { _allocated = true; }

public:
	MidiDriver *device();
	byte getNumber() { return _channel; }
	void release() { _allocated = false; }

	void send (uint32 b);

	// Regular messages
	void noteOff (byte note);
	void noteOn (byte note, byte velocity);
	void programChange (byte program);
	void pitchBend (int16 bend);

	// Control Change messages
	void controlChange (byte control, byte value);
	void pitchBendFactor (byte value);

	// SysEx messages
	void sysEx_customInstrument (uint32 type, byte *instr);
};



class MidiDriver_MPU401 : public MidiDriver {
private:
	typedef void (*TimerProc)(void *refCon); // Copied from class Timer

	MidiChannel_MPU401 _midi_channels [16];
	TimerProc _timer_proc;
	uint16 _channel_mask;

public:
	MidiDriver_MPU401();

	virtual void close();
	void setTimerCallback(void *timer_param, TimerProc timer_proc);
	uint32 getBaseTempo(void) { return 1000000 / 50; }
	uint32 property(int prop, uint32 param);

	MidiChannel *allocateChannel();
	MidiChannel *getPercussionChannel() { return &_midi_channels [9]; }
};


#endif
