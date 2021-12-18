/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

/*
 * Raw MIDI output for the Atari ST line of computers.
 * Based on the ScummVM SEQ & CoreMIDI drivers.
 * Atari code by Keith Scroggins
 * We, unfortunately, could not use the SEQ driver because the /dev/midi under
 * FreeMiNT (and hence in libc) is considered to be a serial port for machine
 * access.  So, we just use OS calls then to send the data to the MIDI ports
 * directly.  The current implementation is sending 1 byte at a time because
 * in most cases we are only sending up to 3 bytes, I believe this saves a few
 * cycles.  I might change so sysex messages are sent the other way later.
 */

#include "stdafx.h"
#include "sound/mididrv.h"
#include "common/scummsys.h"

#if defined(__ATARI__)

#include <mint/osbind.h>
#include "sound/mpu401.h"
#include "common/util.h"
#include "backends/atari/irq.h"


// midi is 31250 baud = 3906 bytes/sec = 78 or 65 bytes per frame.
// HBL or TimerB is more than enough to send at that rate.
// todo: investigate if we can interrupt on Midi TX ready.

//#define STMIDI_TIMERB
//#define STMIDI_HBL


#if defined(STMIDI_TIMERB) || defined(STMIDI_HBL)

#define _midiBufSize		4096
#define	_midiBufMask		(_midiBufSize - 1)
uint8 						_midiBuffer[_midiBufSize];
volatile uint32				_midiBufWritten = 0;
volatile uint32				_midiBufPlayed = 0;

static void __attribute__ ((interrupt)) STMIDI_Interrupt(void)
{
	if(_midiBufPlayed < _midiBufWritten)
	{
		byte status = *((volatile byte*)0x00FFFC04);
		if (status & 2)
		{
			byte msg = _midiBuffer[_midiBufPlayed & _midiBufMask];
			*((volatile byte*)0x00FFFC06) = msg;
			_midiBufPlayed++;
		}
	}
#ifdef STMIDI_TIMERB
	irq_serviced_timerB();
#endif
}

#endif


class MidiDriver_STMIDI : public MidiDriver_MPU401 {
public:
	MidiDriver_STMIDI() : _isOpen (false) { }
	int open();
	bool isOpen() const { return _isOpen; }
	void close();
	void send(uint32 b);
	void sysEx(const byte *msg, uint16 length);

private:
	uint32	bufsize();
	void buffer(byte data);
	void buffer(const byte* data, int len, byte mask);
	bool _isOpen;
};

int MidiDriver_STMIDI::open() {
	if (_isOpen && (!Bcostat(4)))
		return MERR_ALREADY_OPEN;

	_isOpen = true;

#if defined(STMIDI_TIMERB) || defined(STMIDI_HBL)
	_midiBufPlayed = 0;
	_midiBufWritten = 0;
	memset(_midiBuffer, 0, _midiBufSize);

	// todo: check if there is an inerrupt when midi TX is ready instead.
#ifdef STMIDI_TIMERB
	irq_install_timerB(STMIDI_Interrupt, 8, 1);
	irq_enable_timerB(true);
#else
	irq_install_hbl(STMIDI_Interrupt);
	irq_enable_hbl(true);
#endif

#endif


	return 0;
}

void MidiDriver_STMIDI::close() {
	MidiDriver_MPU401::close();
	_isOpen = false;
}

uint32 MidiDriver_STMIDI::bufsize()
{
#if defined(STMIDI_TIMERB) || defined(STMIDI_HBL)
	uint32 readPos  = _midiBufPlayed  & _midiBufMask;
	uint32 writePos = _midiBufWritten & _midiBufMask;
	return (writePos < readPos) ? (readPos - writePos - 1) : (readPos + (_midiBufSize - writePos) - 1);
#else
	return 1024;
#endif
}

void MidiDriver_STMIDI::buffer(byte data)
{
#if defined(STMIDI_TIMERB) || defined(STMIDI_HBL)
	uint32 readPos = _midiBufPlayed & _midiBufMask;
	uint32 readPtr = (uint32) &_midiBuffer[readPos];
	uint32 writePos = _midiBufWritten & _midiBufMask;
	uint32 writePtr = (uint32) &_midiBuffer[writePos];
	_midiBuffer[writePos] = data;
	_midiBufWritten++;
#else
	while(1)
	{
		volatile byte status = *((volatile byte*)0x00FFFC04);
		if (status & 2)
		{
			*((volatile byte*)0x00FFFC06) = data;
			break;
		}
	}
#endif
}


void MidiDriver_STMIDI::buffer(const byte* data, int count, byte mask)
{
#if defined(STMIDI_TIMERB) || defined(STMIDI_HBL)
	uint32 readPos = _midiBufPlayed & _midiBufMask;
	uint32 readPtr = (uint32) &_midiBuffer[readPos];
	uint32 writePos = _midiBufWritten & _midiBufMask;
	uint32 writePtr = (uint32) &_midiBuffer[writePos];

	const byte* src = data;
	for (int i=0; i<count; i++)
	{
		_midiBuffer[writePos++] = ((*src++) & mask);
		writePos &= _midiBufMask;
	}
	_midiBufWritten += count;
#else
	for (int i=0; i<count; i++)
	{
		buffer(mask & *data++);
	}
#endif
}


void MidiDriver_STMIDI::send(uint32 b) {
	if (bufsize() < 3)
		return;

	byte data[4];
	data[0] = (b & 0x000000FF);
	data[1] = (b & 0x0000FF00) >> 8;
	data[2] = (b & 0x00FF0000) >> 16;

	switch (b & 0xF0) {
	case 0x80:	// Note Off
	case 0x90:	// Note On
	case 0xA0:	// Polyphonic Key Pressure
	case 0xB0:	// Controller
	case 0xE0:	// Pitch Bend
		buffer(data[0]);
		buffer(data[1]);
		buffer(data[2]);
		break;
	case 0xC0:	// Program Change
	case 0xD0:	// Aftertouch
		buffer(data[0]);
		buffer(data[1]);
		break;
	default:
		warning("Unknown : %08x\n", (int)b);
		break;
	}
}

void MidiDriver_STMIDI::sysEx (const byte *msg, uint16 length) {
	// FIXME: LordHoto doesn't know if this will still work
	// when sending 264 byte sysEx data, as needed by KYRA,
	// feel free to revert it to 254 again if needed.
	if (length > 254) {
		warning("Cannot send SysEx block - data too large");
		return;
	}

	if (bufsize() < (length + 2))
		return;

	buffer(0xF0);
	buffer(msg, length, 0x7F);
	buffer(0xF7);
}



MidiDriver *MidiDriver_STMIDI_create() {
	debug(9, " - MidiDriver_STMIDI_create - ");
	return new MidiDriver_STMIDI();
}

#endif
