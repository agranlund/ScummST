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
 *
 * simple midi driver for YM2149 chip
 * written by Anders Granlund
 *
 */


#include "stdafx.h"
#include "sound/mididrv.h"
#include "common/scummsys.h"

#if defined(__ATARI__)

#include <mint/osbind.h>
#include "backends/atari/atari.h"
#include "sound/mpu401.h"
#include "sound/mixer.h"
#include "common/util.h"
#include "common/config-manager.h"


class MidiDriver_STCHIP : public MidiDriver_MPU401 {
public:
	MidiDriver_STCHIP() : _isOpen (false) { }
	int open();
	bool isOpen() const { return _isOpen; }
	void close();
	void send(uint32 b);
	void sysEx(const byte *msg, uint16 length);
	void Update();

	struct Inst
	{
		#define SEQ_MAX_LEN		16
		#define SEQ_NUM_TYPES	5

		enum kSeq
		{
			SEQ_VOL = 0,
			SEQ_ARP,
			SEQ_VIB,
			SEQ_FREQ,
			SEQ_NOISE,
		};
		enum kInstFlag
		{
			FLG_SQUARE		= (1 << 0),
			//FLG_SOFTBUZZ	= (1 << 1),
			//FLG_HARDBUZZ	= (1 << 2),
			FLG_NOISE		= (1 << 3),
		};

		struct Seq
		{
			byte	length;					// length of sequence
			byte	repeat;					// repeat from position
			uint16	data[SEQ_MAX_LEN];		// sequence data
		};

		byte	flags;						// flags
		byte	sustain;					// max length auto note-off (255 = infinite)
		byte	decay;						// volume decay after note-off (1.7 fixed point)
		Seq		seq[SEQ_NUM_TYPES];			// vol, arp, vib, freq, noise
	};

	void stopOutput();						// when starting sample playback etc
	void resumeOutput(bool fadein = false);	// when sample playback is done

private:
	void noteOn(byte chn, byte note, byte vel);
	void noteOff(byte chn, byte note);
	void allNotesOff(byte chn);
	void setVolume(byte chn, byte volume);
	void setPriority(byte chn, byte priority);
	void setProgram(byte chn, byte prg);
	void setDetune(byte chn, byte detune);

	struct Voice
	{
		uint16		id;
		uint16		prio;
		uint32		time;
		uint16		cnt;
		byte		state;
		byte		note;
		byte		vol;
		const Inst*	inst;
		byte		pos[5];
	};

	struct Channel
	{
		byte	program;
		byte	volume;
		byte	priority;
		byte	detune;
	};

	Channel		_channels[16];
	uint16		_activeChannels;
	Voice		_voices[3];
	bool 		_isOpen;
	uint16 		_duckVol;
	uint16		_oldDuckVol;
	byte		_voltable[256];
	byte		_mute;
public:
	static uint32 _frame;
};

uint32 MidiDriver_STCHIP::_frame = 0;
extern const uint16 YM2149_NoteTable[];
extern const MidiDriver_STCHIP::Inst* YM2149_InstTable[129];	
extern const MidiDriver_STCHIP::Inst* YM2149_DrumTable[128];	

static MidiDriver_STCHIP* _this;

void MidiDriver_STCHIP_Update() {
	if (_this) {
		_this->Update();
	}
}

void MidiDriver_STCHIP_Mute(bool mute, bool fadein) {
	if (_this) {
		if (mute)
			_this->stopOutput();
		else
			_this->resumeOutput(fadein);
	}
}

int MidiDriver_STCHIP::open()
{
	if (_isOpen)
		return MERR_ALREADY_OPEN;
	_isOpen = true;
	_duckVol = 255;
	_oldDuckVol = 255;
	_frame = 0;
	_mute = 0;
	
	for (int i=0; i<3; i++)
	{
		_voices[i].id = 0;
		_voices[i].prio = 0;
		_voices[i].time = 0;
		_voices[i].vol = 0;
	}

	for (int i=0; i<16; i++)
	{
		_channels[i].program = 0;
		_channels[i].volume = 0xF0;
		_channels[i].priority = 1;
		_channels[i].detune = 0;
	}
	_activeChannels = 0;

	for (int i=0; i<16; i++)
	{
		byte* table = _voltable + (i << 4);
		for (int j=0; j<16; j++)
		{
			table[j] = ((i + 1) * j) >> 4;
		}
	}

	YM2149_LOCK();
	YM2149_WR(0, 0);
	YM2149_WR(1, 0);
	YM2149_WR(2, 0);
	YM2149_WR(3, 0);
	YM2149_WR(4, 0);
	YM2149_WR(5, 0);
	YM2149_WR(6, 0);
	YM2149_WR_MASKED(7, 0xFF, 0x3F);
	YM2149_WR(8, 0);
	YM2149_WR(9, 0);
	YM2149_WR(10, 0);
	YM2149_WR(11, 0);
	YM2149_WR(12, 0);
	YM2149_WR(13, 0);
	YM2149_UNLOCK();

	_this = this;
	if (g_engine && g_engine->_system)
	{
		((OSystem_Atari*)g_engine->_system)->setMidiUpdateFunc(MidiDriver_STCHIP_Update);
	}
	return 0;
}

void MidiDriver_STCHIP::close()
{
	MidiDriver_MPU401::close();
	if (g_engine && g_engine->_system)
		((OSystem_Atari*)g_engine->_system)->setMidiUpdateFunc(NULL);	
	_this = NULL;
	_isOpen = false;
}

void MidiDriver_STCHIP::stopOutput()
{
	_mute++;
	if (_mute > 1)
		return;

	YM2149_LOCK();
	YM2149_WR_MASKED(7, 0xFF, 0x3F);
	YM2149_UNLOCK();
	_oldDuckVol = _duckVol;
	_duckVol = 0;
}

void MidiDriver_STCHIP::resumeOutput(bool fadein)
{
	if (_mute > 0)
	{
		_mute--;
		if (_mute > 0)
			return;
	}

	YM2149_LOCK();
	YM2149_WR_MASKED(7, 0xFF, 0x3F);
	YM2149_UNLOCK();
	_duckVol = fadein ? 1 : _oldDuckVol;
}

void MidiDriver_STCHIP::Update()
{
	_frame++;

	if ((_mute > 0) || (_duckVol == 0))
		return;

	if (_duckVol < 255)
		_duckVol++;

	for (int i=0; i<3; i++)
	{
		Voice& v = _voices[i];
		if (v.id == 0)
			continue;

		Channel& chn = _channels[v.id>>8];
		const Inst* inst = v.inst;
		byte onval = ~(inst->flags & 9);
		
		switch (v.state)
		{
			// play
			case 0:
				v.state = 1;

			case 1:
			{
				if ((inst->sustain != 255) && ((_frame - v.time) > inst->sustain))
					v.state = 2;
			} break;

			// decay
			case 2:
			{
				if (v.vol > inst->decay)
				{
					v.vol -= inst->decay;
				}
				else
				{
					v.vol = 0;
					v.id = 0;
					v.state = 3;
					onval = 0xFF;
				}
			} break;
		}

		// volume
		uint16 volume;
		if (inst->seq[Inst::SEQ_VOL].length > 0)	volume = _voltable[chn.volume + inst->seq[Inst::SEQ_VOL].data[v.pos[Inst::SEQ_VOL]]];
		else										volume = chn.volume >> 4;
		if (_duckVol != 255)						volume = _voltable[volume + (_duckVol & 0xF0)];
		if (v.vol != 0xFF)							volume = _voltable[volume + (v.vol & 0xF0)];
	
		// note/frequency
		uint16 freq;
		if (inst->seq[Inst::SEQ_FREQ].length > 0)
		{
			// fixed frequency
			freq = inst->seq[Inst::SEQ_FREQ].data[v.pos[Inst::SEQ_FREQ]];
		}
		else
		{
			// arpeggio
			uint16 note = (uint16) v.note;
			if (inst->seq[Inst::SEQ_ARP].length > 0)
				note += inst->seq[Inst::SEQ_ARP].data[v.pos[Inst::SEQ_ARP]];
			freq = YM2149_NoteTable[note];
		}

		// vibrato
		if (inst->seq[Inst::SEQ_VIB].length > 0)
			freq += inst->seq[Inst::SEQ_VIB].data[v.pos[Inst::SEQ_VIB]];
		byte freq_lo = (freq & 0xFF);
		byte freq_hi = (freq >> 8) & 0xF;

		// noise
		byte noisefreq = 0xFF;
		if (inst->seq[Inst::SEQ_NOISE].length > 0)
			noisefreq = (byte) inst->seq[Inst::SEQ_NOISE].data[v.pos[Inst::SEQ_NOISE]];
		onval |= ((noisefreq & 0x80) >> 4);

		YM2149_LOCK();
		YM2149_WR_MASKED(7, (onval << i), (9 << i));
		YM2149_WR((i << 1) + 0, freq_lo);
		YM2149_WR((i << 1) + 1, freq_hi);
		YM2149_WR((i + 8), volume);
		YM2149_UNLOCK();

		for (int i=0; i<SEQ_NUM_TYPES; i++)
		{
			if (inst->seq[i].length > 0)
			{
				v.pos[i]++;
				if (v.pos[i] >= inst->seq[i].length)
					v.pos[i] = inst->seq[i].repeat;
			}
		}
	}
}

void MidiDriver_STCHIP::noteOn(byte chn, byte note, byte vel)
{
	if (_duckVol == 0)
		return;

	const Inst* inst;
	Channel& channel = _channels[chn];

	if (note & 0x80)
		return;

	bool drums = (chn == 9);
	if (!drums)
	{
		if (note & 0x80)
			return;
		inst = YM2149_InstTable[channel.program];
	}
	else
	{
		inst = YM2149_DrumTable[note];
	}
	if (!inst)
		return;

	uint16 freq = YM2149_NoteTable[note];
	if (!drums && (freq == 0))
	{
		allNotesOff(chn);
		return;
	}

	uint16 prio = (channel.priority << 8) | note;
	int bestIdx = -1;
	int bestVal = 0xFFFF;

	// replace existing drums
	if (drums)
	{
		prio = 0xFFFF;
		for (int i=0; i<3; ++i)
		{
			if ((_voices[i].id >> 8) == chn)
			{
				if ((_frame - _voices[i].time) < 4)
					return;

				bestIdx = i;
				bestVal = 0;
				break;				
			}
		}
	}
	else
	{
		for (int i=0; i<3; ++i)
		{
			byte chn2 = _voices[i].id >> 8;
			if ((chn == chn2) || (_voices[i].inst == inst) /*|| (_channels[chn].program == _channels[chn2].program)*/ )
			{
				byte n1 = note;
				byte n2 = _voices[i].prio & 0xFF;
				byte nd = (n1 > n2) ? n1 - n2 : n2 - n1;
				if (nd < 24)
				{
					bestIdx = i;
					bestVal = 0;
					break;
				}
			}
		}
	}


	// replace old or unused voices
	if (bestIdx < 0)
	{
		bestVal = 200;	// cutoff time in 200Hz clocks
		for (int i=0; i<3; ++i)
		{
			if (_voices[i].id == 0)
			{
				bestIdx = i;
				bestVal = 0;
				break;
			}

			uint32 val = _frame - _voices[i].time;
			if (val > bestVal)
			{
				bestVal = val;
				bestIdx = i;
			}
		}
	}


	// replace lower prio voices
	if (bestIdx < 0)
	{
		bestVal = 0;
		for (int i=0; i<3; ++i)
		{
			if (_voices[i].prio <= prio)
			{
				int val = prio - _voices[i].prio;
				if (val > bestVal)
				{
					bestVal = val;
					bestIdx = i;
				}
			}
		}
	}


	if (bestIdx < 0)
		return;

	Voice& v = _voices[bestIdx];
	v.id = (chn << 8) | note;
	v.prio = prio;
	v.time = _frame;
	v.vol = 0xFF;
	v.inst = inst;
	v.note = note;
	v.state = 1;
	v.pos[Inst::SEQ_VOL] = 0;
	v.pos[Inst::SEQ_ARP] = 0;
	v.pos[Inst::SEQ_VIB] = 0;
	v.pos[Inst::SEQ_FREQ] = 0;
	v.pos[Inst::SEQ_NOISE] = 0;
}

void MidiDriver_STCHIP::noteOff(byte chn, byte note)
{
	if (note & 0x80)
		return;

	byte mask = 0;
	uint16 id = (chn << 8) | note;
	for (int i=0; i<3; ++i)
	{
		Voice& v = _voices[i];
		if (v.id == id)
		{
			v.prio = 0;
			if (v.state != 2)
				v.state = 2;
		}
	}
}

void MidiDriver_STCHIP::allNotesOff(byte chn)
{
	for (int i=0; i<3; ++i)
	{
		Voice& v = _voices[i];
		if ((v.id >> 8) == chn)
		{
			v.prio = 0;
			if (v.state != 2)
				v.state = 2;
		}
	}
}

void MidiDriver_STCHIP::setVolume(byte chn, byte volume)
{
	volume = 196;
	uint16 musicVolume = GetConfig(kConfig_MusicVolume);
	uint16 masterVolume = GetConfig(kConfig_MasterVolume);
	if (musicVolume > 0)	musicVolume++;
	if (masterVolume > 0)	masterVolume++;
	_channels[chn].volume = 0xF0 & ((((musicVolume * masterVolume) >> 8) * volume) >> 8);
}

void MidiDriver_STCHIP::setPriority(byte chn, byte priority)
{
	if (chn < 16)
		_channels[chn].priority = priority;
}

void MidiDriver_STCHIP::setDetune(byte chn, byte detune)
{
	if (chn < 16)
		_channels[chn].detune = detune;
}

void MidiDriver_STCHIP::setProgram(byte chn, byte prg)
{
	if (chn < 16)
	{
		_channels[chn].program = prg;
		if (prg != 0)
			_activeChannels |= (1 << chn);
		else
			_activeChannels &= ~(1 << chn);
	}
}


void MidiDriver_STCHIP::sysEx(const byte *msg, uint16 length)
{
}


void MidiDriver_STCHIP::send(uint32 b)
{
	switch (b & 0xF0)
	{
		case 0x90:		// note on
			noteOn((b&0x0F), (b >> 8) & 0xFF, (b >> 16) & 0xFF);
			break;

		case 0x80:
			noteOff((b&0x0F), (b >> 8) & 0xFF);
			break;

		case 0xC0:
			setProgram((b & 0x0F), (b >> 8) & 0xFF);
			break;

		case 0xB0:
			switch ((b >> 8) & 0xFF)
			{
				case 7:			// volume
					setVolume((b & 0x0F), (b >> 16) & 0xFF);
					break;

				case 123:
					allNotesOff(b & 0x0F);
					break;

				case 18:		// priority
					setPriority((b & 0x0F), (b >> 16) & 0xFF);
					break;

				case 17:		// detune
					setDetune((b & 0x0F), (b >> 16) & 0xFF);
					break;


				case 1:			// modulation
				case 10:		// pan
				case 64:		// sustain
				case 91:		// effect level
				case 93:		// chorus level
					break;
				
			}
			break;
	}
}


MidiDriver *MidiDriver_STCHIP_create()
{
	debug(1, " - MidiDriver_STCHIP_create - ");
	return new MidiDriver_STCHIP();
}

//62500 ?

const uint16 YM2149_NoteTable[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,
//	0,0,0,0,0,0,0,0,0,0,0,0,

	3822,	// MIDI 12, 16.352 Hz
	3608,	// MIDI 13, 17.324 Hz 
	3405,	// MIDI 14, 18.354 Hz
	3214,	// MIDI 15, 19.445 Hz
	3034,	// MIDI 16, 20.602 Hz
	2863,	// MIDI 17, 21.827 Hz
	2703,	// MIDI 18, 23.125 Hz
	2551,	// MIDI 19, 24.500 Hz
	2408,	// MIDI 20, 25.957 Hz
	2273,	// MIDI 21, 27.500 Hz
	2145,	// MIDI 22, 29.135 Hz
	2025,	// MIDI 23, 30.868 Hz

  1911, // MIDI 24, 32.70 Hz
   1804, // MIDI 25, 34.65 Hz
   1703, // MIDI 26, 36.71 Hz
   1607, // MIDI 27, 38.89 Hz
   1517, // MIDI 28, 41.20 Hz
   1432, // MIDI 29, 43.65 Hz
   1351, // MIDI 30, 46.25 Hz
   1276, // MIDI 31, 49.00 Hz
   1204, // MIDI 32, 51.91 Hz
   1136, // MIDI 33, 55.00 Hz
   1073, // MIDI 34, 58.27 Hz
   1012, // MIDI 35, 61.74 Hz

   1911, // MIDI 24, 32.70 Hz
   1804, // MIDI 25, 34.65 Hz
   1703, // MIDI 26, 36.71 Hz
   1607, // MIDI 27, 38.89 Hz
   1517, // MIDI 28, 41.20 Hz
   1432, // MIDI 29, 43.65 Hz
   1351, // MIDI 30, 46.25 Hz
   1276, // MIDI 31, 49.00 Hz
   1204, // MIDI 32, 51.91 Hz
   1136, // MIDI 33, 55.00 Hz
   1073, // MIDI 34, 58.27 Hz
   1012, // MIDI 35, 61.74 Hz

   956, // MIDI 36, 65.41 Hz
   902, // MIDI 37, 69.30 Hz
   851, // MIDI 38, 73.42 Hz
   804, // MIDI 39, 77.78 Hz
   758, // MIDI 40, 82.41 Hz
   716, // MIDI 41, 87.31 Hz
   676, // MIDI 42, 92.50 Hz
   638, // MIDI 43, 98.00 Hz
   602, // MIDI 44, 103.83 Hz
   568, // MIDI 45, 110.00 Hz
   536, // MIDI 46, 116.54 Hz
   506, // MIDI 47, 123.47 Hz
   478, // MIDI 48, 130.81 Hz
   451, // MIDI 49, 138.59 Hz
   426, // MIDI 50, 146.83 Hz
   402, // MIDI 51, 155.56 Hz
   379, // MIDI 52, 164.81 Hz
   358, // MIDI 53, 174.61 Hz
   338, // MIDI 54, 185.00 Hz
   319, // MIDI 55, 196.00 Hz
   301, // MIDI 56, 207.65 Hz
   284, // MIDI 57, 220.00 Hz
   268, // MIDI 58, 233.08 Hz
   253, // MIDI 59, 246.94 Hz
   239, // MIDI 60, 261.63 Hz
   225, // MIDI 61, 277.18 Hz
   213, // MIDI 62, 293.66 Hz
   201, // MIDI 63, 311.13 Hz
   190, // MIDI 64, 329.63 Hz
   179, // MIDI 65, 349.23 Hz
   169, // MIDI 66, 369.99 Hz
   159, // MIDI 67, 392.00 Hz
   150, // MIDI 68, 415.30 Hz
   142, // MIDI 69, 440.00 Hz
   134, // MIDI 70, 466.16 Hz
   127, // MIDI 71, 493.88 Hz
   119, // MIDI 72, 523.25 Hz
   113, // MIDI 73, 554.37 Hz
   106, // MIDI 74, 587.33 Hz
   100, // MIDI 75, 622.25 Hz
   95, // MIDI 76, 659.26 Hz
   89, // MIDI 77, 698.46 Hz
   84, // MIDI 78, 739.99 Hz
   80, // MIDI 79, 783.99 Hz
   75, // MIDI 80, 830.61 Hz
   71, // MIDI 81, 880.00 Hz
   67, // MIDI 82, 932.33 Hz
   63, // MIDI 83, 987.77 Hz
   60, // MIDI 84, 1046.50 Hz
   56, // MIDI 85, 1108.73 Hz
   53, // MIDI 86, 1174.66 Hz
   50, // MIDI 87, 1244.51 Hz
   47, // MIDI 88, 1318.51 Hz
   45, // MIDI 89, 1396.91 Hz
   42, // MIDI 90, 1479.98 Hz
   40, // MIDI 91, 1567.98 Hz
   38, // MIDI 92, 1661.22 Hz
   36, // MIDI 93, 1760.00 Hz
   34, // MIDI 94, 1864.66 Hz
   32, // MIDI 95, 1975.53 Hz
   30, // MIDI 96, 2093.00 Hz
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
};



// default
MidiDriver_STCHIP::Inst YM2149_instDefault {
	MidiDriver_STCHIP::Inst::FLG_SQUARE, 4, 16, 
	{
		{ 2, 1, { 15, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vol
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// arp
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vib
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// freq
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }			// noise
	}
};
MidiDriver_STCHIP::Inst YM2149_instVibraphone {
	MidiDriver_STCHIP::Inst::FLG_SQUARE, 100, 4, 
	{
		{ 1, 0, { 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vol
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// arp
		{ 8, 0, { 0, -1, 0, 1, 0, -1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vib
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// freq
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }			// noise
	}
};
MidiDriver_STCHIP::Inst YM2149_instOrgan {
	MidiDriver_STCHIP::Inst::FLG_SQUARE, 255, 255, 
	{
		{ 1, 0, { 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vol
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// arp
		{ 8, 0, { 0, 0, -2, -2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vib
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// freq
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }			// noise
	}
};
MidiDriver_STCHIP::Inst YM2149_instStrings {
	MidiDriver_STCHIP::Inst::FLG_SQUARE, 100, (1<<3), 
	{
		{ 4, 3, { 4, 8, 12, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vol
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// arp
		{ 8, 0, { 0, 0, -2, -2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vib
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// freq
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }			// noise
	}
};
MidiDriver_STCHIP::Inst YM2149_instEnsemble {
	MidiDriver_STCHIP::Inst::FLG_SQUARE, 255, 255, 
	{
		{ 1, 0, { 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vol
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// arp
		{ 8, 0, { 0, 0, -2, -2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vib
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// freq
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }			// noise
	}
};
MidiDriver_STCHIP::Inst YM2149_instBrass {
	MidiDriver_STCHIP::Inst::FLG_SQUARE, 255, 64, 
	{
		{ 2, 1, { 12, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vol
		{ 2, 1, { 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// arp
		{ 8, 0, { 0, 0, -2, -2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vib
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// freq
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }			// noise
	}
};
MidiDriver_STCHIP::Inst YM2149_instLead {
	MidiDriver_STCHIP::Inst::FLG_SQUARE, 255, 64, 
	{
		{ 2, 1, { 12, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vol
		{ 2, 1, { 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// arp
		{ 8, 0, { 0, 0, -2, -2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vib
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// freq
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }			// noise
	}
};
MidiDriver_STCHIP::Inst YM2149_instPipe {
	MidiDriver_STCHIP::Inst::FLG_SQUARE, 255, 64, 
	{
		{ 2, 1, { 12, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vol
		{ 2, 1, { 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// arp
		{ 8, 0, { 0, 0, -2, -2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vib
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// freq
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }			// noise
	}
};
MidiDriver_STCHIP::Inst YM2149_instSynth {
	MidiDriver_STCHIP::Inst::FLG_SQUARE, 255, 64, 
	{
		{ 2, 1, { 12, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vol
		{ 2, 1, { 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// arp
		{ 8, 0, { 0, 0, -2, -2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0 } },			// vib
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },			// freq
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }			// noise
	}
};

const MidiDriver_STCHIP::Inst* YM2149_InstTable[129] =
{
	0,						//   0
	// Piano
	&YM2149_instDefault,	//	 1 - Piano 1
	&YM2149_instDefault,	//   2 - Piano 2
	&YM2149_instDefault,	//   3 - Piano 3
	&YM2149_instDefault,	//   4 - Honky-Tonk Piano
	&YM2149_instDefault,	//   5 - Electric Piano 1 
	&YM2149_instDefault,	//   6 - Electric Piano 2
	&YM2149_instDefault,	//   7 - Harpsichord
	&YM2149_instDefault,	//   8 - Claw
	// Chromatic Percussion
	&YM2149_instVibraphone,	//   9 - Celesta
	&YM2149_instVibraphone,	//  10 - Glockenspiel
	&YM2149_instVibraphone,	//  11 - Music box
	&YM2149_instVibraphone,	//  12 - Vibraphone 
	&YM2149_instVibraphone,	//	13 - Marimba
	&YM2149_instVibraphone,	//  14 - Xylophone
	&YM2149_instVibraphone, //  15 - Tubular Bells
	&YM2149_instVibraphone,	//  16 - Santar
	// Organ
	&YM2149_instOrgan,		//  17 - Organ1
	&YM2149_instOrgan,		//  18 - Organ2
	&YM2149_instOrgan,		//  19 - Organ3 
	&YM2149_instOrgan,		//  20 - Church Organ
	&YM2149_instOrgan,		//  21 - Reed Organ
	&YM2149_instOrgan,		//  22 - Accordion
	&YM2149_instOrgan,		//  23 - Harmonica
	&YM2149_instOrgan,		//  24 - Tango accordion
	// Guitar
	&YM2149_instDefault,	//  25 - Nylon
	&YM2149_instDefault,	//  26 - Steel
	&YM2149_instDefault,	//  27 - Jazz
	&YM2149_instDefault,	//  28 - Clean
	&YM2149_instDefault,	//  29 - Muted
	&YM2149_instDefault,	//  30 - Overdrive
	&YM2149_instDefault,	//  31 - Distortion
	&YM2149_instDefault, 	//  32 - Harmonics
	// Bass
	&YM2149_instDefault,	//  33 - Acostic bass
	&YM2149_instDefault,	//  34 - Fingered bass
	&YM2149_instDefault,	//  35 - Picked bass 
	&YM2149_instDefault,	//  36 - Fretless bass
	&YM2149_instDefault,	//  37 - Slap bass 1
	&YM2149_instDefault,	//  38 - Slap bass 2 
	&YM2149_instDefault,	//  39 - Synth bass 1
	&YM2149_instDefault,	//  40 - Synth bass 2
	//Strings
	&YM2149_instStrings,	//  41 - Violin
	&YM2149_instStrings,	//  42 - Viola
	&YM2149_instStrings,	//  43 - Cello
	&YM2149_instStrings,	//  44 - Contrabass
	&YM2149_instStrings,	//  45 - Tremolo strings
	&YM2149_instStrings,	//  46 - Pizziacto
	&YM2149_instStrings,	//  47 - Harp
	&YM2149_instStrings, 	//  48 - Timpani
	// Ensemble
	&YM2149_instEnsemble,	//  49 - Strings
	&YM2149_instEnsemble,	//  50 - Slow strings
	&YM2149_instEnsemble,	//  51 - Synth Strings 1
	&YM2149_instEnsemble,	//  52 - Synth Strings 2
	&YM2149_instEnsemble,	//  53 - Choir Aahs
	&YM2149_instEnsemble,	//  54 - Voice Oohs
	&YM2149_instEnsemble,	//  55 - Synth voice
	0,						//  56 - Orchestra hit
	// Brass
	&YM2149_instBrass,		//  57 - Trumpet
	&YM2149_instBrass,		//  58 - Trombone
	&YM2149_instBrass,		//  59 - Tuba
	&YM2149_instBrass,		//  60 - Muted trumpet
	&YM2149_instBrass,		//  61 - French horn
	&YM2149_instBrass,		//  62 - Brass 1
	&YM2149_instBrass,		//  63 - Synth brass 1
	&YM2149_instBrass, 		//  64 - Synth brass 2
	// Lead
	&YM2149_instLead,		//  65 - Soprano sax
	&YM2149_instLead,		//  66 - Alto sax
	&YM2149_instLead,		//  67 - Tenor sax
	&YM2149_instLead,		//  68 - Baritone sax
	&YM2149_instLead,		//  69 - Oboe
	&YM2149_instLead,		//  70 - English horn
	&YM2149_instLead,		//  71 - Bassoon
	&YM2149_instLead,		//  72 - Clarinet
	// Pipe
	&YM2149_instPipe,		//  73 - Piccolo
	&YM2149_instPipe,		//  74 - Flute
	&YM2149_instPipe,		//  75 - Recorder
	&YM2149_instPipe,		//  76 - Pan flute
	&YM2149_instPipe,		//  77 - Bottle Blow
	&YM2149_instPipe, 		//  78 - Shakuhachi
	&YM2149_instPipe,		//  79 - Whistle
	&YM2149_instPipe, 		//  80 - Ocarina
	// Synth lead
	&YM2149_instSynth,		//  81 - Square wave
	&YM2149_instSynth,		//  82 - Saw wave
	&YM2149_instSynth,		//  83 - Synth calliope
	&YM2149_instSynth,		//  84 - Chiffer lead
	&YM2149_instSynth,		//  85 - Charang
	&YM2149_instSynth,		//  86 - Solo vox
	&YM2149_instSynth,		//  87 - 5th saw wave
	&YM2149_instSynth,		//  88 - Bass & Lead
	// Synth pad
	0, 0, 0, 0, 0, 0, 0, 0, 
	// Synth FX
	0, 	0, 0, 0, 0, 0, 0, 0,
	// Ethnic
	&YM2149_instDefault,	// 105 - Sitar
	&YM2149_instDefault,	// 106 - Banjo
	&YM2149_instDefault,	// 107 - Shamisen
	&YM2149_instDefault,	// 108 - Koto
	&YM2149_instDefault,	// 109 - Kalimba
	&YM2149_instDefault,	// 110 - Bagpipe
	&YM2149_instDefault,	// 111 Fiddle
	&YM2149_instDefault, 	// 112 - Shanai
	// Percussive
	0,						// 113 - Tinkle Bell
	0,						// 114 - Agogo
	0,						// 115 - Steel drums
	0,						// 116 - Woodblock
	0,						// 117 - Taiko
	0,						// 118 - Melo Tom
	0,						// 119 - Synth Drum
	0,						// 120 - Reverse cymbal
	// Sound FX
	0,						// 121 - Guitar fret noise
	0,						// 122 - Breath noise
	0,						// 123 - Seashore
	0,						// 124 - Bird
	0,						// 125 - Telephone
	0,						// 126 - Helicopter
	0,						// 127 - Applause
	0, 						// 128 - Gunshot
};



// default noise drum
MidiDriver_STCHIP::Inst YM2149_drumDefault{
	MidiDriver_STCHIP::Inst::FLG_NOISE, 2, 255, 
	{
		{ 1, 0, { 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
		{ 2, 1, { 7, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }
	}
};

// bass drum
MidiDriver_STCHIP::Inst YM2149_drumBass{
	(MidiDriver_STCHIP::Inst::FLG_SQUARE | MidiDriver_STCHIP::Inst::FLG_NOISE), 5, 255, 
	{
		{ 1, 0, { 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
		{ 4, 3, { 0x300, 0x500, 0x600, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
		{ 3, 2, { 15, 7, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }
	}
};

// snare drum
MidiDriver_STCHIP::Inst YM2149_drumSnare{
	(MidiDriver_STCHIP::Inst::FLG_SQUARE | MidiDriver_STCHIP::Inst::FLG_NOISE), 3, (1<<5), 
	{
		{ 1, 0, { 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
		{ 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
		{ 3, 2, { 0x300, 0x400, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
		{ 3, 2, { 25, 15, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }
	}
};


const MidiDriver_STCHIP::Inst* YM2149_DrumTable[128] =
{
	// 0 - 34
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0,

	// 35 - 74
	&YM2149_drumDefault,				// 35 - Acoustic bass drum
	&YM2149_drumBass,				// 36 - Bass drum
	&YM2149_drumDefault,				// 37 - Side stick
	&YM2149_drumSnare,				// 38 - Acoustic snare
	&YM2149_drumDefault,				// 39 - Hand clap
	&YM2149_drumDefault,				// 40 - Electric snare
	&YM2149_drumDefault,				// 41 - Low floor tom
	&YM2149_drumDefault,				// 42 - Closed hi-hat
	&YM2149_drumDefault,				// 43 - High floor hi-hat
	&YM2149_drumDefault,				// 44 - Pedal hi-hat
	&YM2149_drumDefault,				// 45 - Low tom
	&YM2149_drumDefault,				// 46 - Open hi-hat
	&YM2149_drumDefault,				// 47 - Low-Mid tom
	&YM2149_drumDefault,				// 48 - Hi-Mid tom
	&YM2149_drumDefault,				// 49 - Crash cymbal 1
	&YM2149_drumDefault, 				// 50 - High tom
	&YM2149_drumDefault,				// 51 - Ride cymbal 1
	&YM2149_drumDefault,				// 52 - Chinese cymbal
	&YM2149_drumDefault,				// 53 - Ride bell
	&YM2149_drumDefault,				// 54 - Tambourine
	&YM2149_drumDefault,				// 55 - Splash cymbal
	&YM2149_drumDefault,				// 56 - Cowbell
	&YM2149_drumDefault,				// 57 - Crash cymbal 2
	&YM2149_drumDefault,				// 58 - Vibraslap
	&YM2149_drumDefault,				// 59 - Ride cymbal 2
	&YM2149_drumDefault,				// 60 - Hi bongo
	&YM2149_drumDefault,				// 61 - Low bongo
	&YM2149_drumDefault,				// 62 - Mute Hi Conga
	&YM2149_drumDefault,				// 63 - Open Hi Conga
	&YM2149_drumDefault,				// 64 - Low Conga
	&YM2149_drumDefault,				// 65 - High Timable
	&YM2149_drumDefault, 				// 66 - Low Timbale
	&YM2149_drumDefault,				// 67 - High Agogo
	&YM2149_drumDefault,				// 68 - Low Agogo
	&YM2149_drumDefault,				// 69 - Cabasa
	&YM2149_drumDefault,				// 70 - Maracas
	&YM2149_drumDefault,				// 71 - Showrt whistle
	&YM2149_drumDefault,				// 72 - Long whistle
	&YM2149_drumDefault,				// 73 - Short guiro
	&YM2149_drumDefault,				// 74 - Long guiro
	&YM2149_drumDefault,				// 75 - Claves
	&YM2149_drumDefault,				// 76 - Hi wood block
	&YM2149_drumDefault,				// 77 - Lo wood block
	&YM2149_drumDefault,				// 78 - Mute cuica
	&YM2149_drumDefault,				// 79 - Open cuica
	&YM2149_drumDefault,				// 80 - Mute triangle
	&YM2149_drumDefault,				// 81 - Open triangle

	// 82-127
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};



#endif
