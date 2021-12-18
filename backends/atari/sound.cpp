//-------------------------------------------------------------------------
// ScummST sound related stuff
//
// This file is distributed under the GPL v2, or at your option any
// later version.  Read COPYING for details.
// (c)2021, Anders Granlund
//-------------------------------------------------------------------------
#include "atari.h"
#include "irq.h"
#include "sound/mixer.h"
#include "common/config-manager.h"
#include "common/sound/mididrv.h"
#include <mint/sysbind.h>

#define SNDCOOKIE_YM		(1 << 0)
#define SNDCOOKIE_DMA		(1 << 1)
#define SNDCOOKIE_CODEC		(1 << 2)
#define SNDCOOKIE_DSP		(1 << 3)
#define SNDCOOKIE_MATRIX	(1 << 4)
#define SNDCOOKIE_XBIOS		(1 << 5)


static volatile uint32 		_soundBufSize = 0;
static volatile uint32 		_soundChunkSize = 0;
static volatile uint32 		_soundBufMask = 0;
static volatile uint8*		_soundBuffer = 0;
static volatile uint8*		_tempSoundBuffer = 0;
static volatile uint32		_soundBufWritten = 0;
static volatile uint32		_soundBufPlayed = 0;
extern const uint16 		_soundTable_S8[];

#define CMA_INT			((volatile uint8*)0x00FF8900)
#define DMA_CTRL		((volatile uint8*)0x00FF8901)
#define DMA_STARTH		((volatile uint8*)0x00FF8903)
#define DMA_STARTM		((volatile uint8*)0x00FF8905)
#define DMA_STARTL		((volatile uint8*)0x00FF8907)
#define DMA_COUNTH		((volatile uint8*)0x00FF8909)
#define DMA_COUNTM		((volatile uint8*)0x00FF890B)
#define DMA_COUNTL		((volatile uint8*)0x00FF890D)
#define DMA_ENDH		((volatile uint8*)0x00FF890F)
#define DMA_ENDM		((volatile uint8*)0x00FF8911)
#define DMA_ENDL		((volatile uint8*)0x00FF8913)
#define DMA_TRACK		((volatile uint8*)0x00FF8920)
#define DMA_MODE		((volatile uint8*)0x00FF8921)

#ifndef Devconnect
#define Devconnect(a,b,c,d,e) (long)trap_14_wwwwww(0x8b,(short)(a),(short)(b),(short)(c),(short)(d),(short)(e))
#endif
#ifndef Soundcmd
#define Soundcmd(a,b) (long)trap_14_www(0x82,(short)(a),(short)(b))
#endif
#ifndef Setmode
#define Setmode(a) (long)trap_14_ww(0x84,(short)(a))
#endif

void DMA_SetCtrl(bool enable, bool looping)
{
	uint8 d = enable | (looping << 1);
	*DMA_CTRL = d;
}

void DMA_SetMode(bool stereo, byte rate)
{
	// always 8bit, stereo and rate selectable
	uint8 d = ((!stereo) << 7) | (rate & 3);
	*DMA_MODE = d;
}

void DMA_SetAddr(uint32 start, uint32 end)
{
	*DMA_ENDH 	= ((end	  >> 16) & 0xFF);
	*DMA_ENDM 	= ((end	  >>  8) & 0xFF);
	*DMA_ENDL 	= ((end        ) & 0xFF);
	*DMA_STARTH = ((start >> 16) & 0xFF);
	*DMA_STARTM = ((start >>  8) & 0xFF);
	*DMA_STARTL = ((start      ) & 0xFF);
}

uint32 DMA_GetAddr()
{
	uint32 addr = *DMA_COUNTH; addr <<= 8;
	addr |= *DMA_COUNTM; addr <<= 8;
	addr |= *DMA_COUNTL;
	return addr;
}


//-------------------------------------------------------------------------------------
bool YM2149_LOCK()
{
	irq_disable();
	return true;
}

void YM2149_UNLOCK()
{
	irq_enable();
}


//-------------------------------------------------------------------------------------
static void __attribute__ ((interrupt)) timerA_YM(void)
{
	if (_soundBufPlayed < _soundBufWritten)
	{
		uint32 readpos = _soundBufPlayed & _soundBufMask;
		volatile uint8* sample = &_soundBuffer[readpos];
		_soundBufPlayed++;
		__asm__ volatile							\
		(											\
			"moveq.l	#0,d3\n\t"					\
			"move.b		(%0),d3\n\t"				\
			"lsl.w		#3,d3\n\t"					\
			"move.l		0(%1,d3.w),d5\n\t"			\
			"move.l		4(%1,d3.w),d4\n\t"			\
			"lea.l		0x00ff8800,a4\n\t"			\
			"movep.l	d5,0(a4)\n\t"				\
			"movep.w	d4,0(a4)\n\t"				\
		: 											\
		: "a"(sample), "a"(_soundTable_S8)			\
		: "d3", "d4", "d5", "a4", "memory", "cc"	\
		);
	}
	*((volatile unsigned char*)0x00FFFA0F) &= ~(1<<5);
}

//-------------------------------------------------------------------------------------
static void __attribute__ ((interrupt)) timerA_YM_Falcon(void)
{
	if (_soundBufPlayed < _soundBufWritten)
	{
		uint32 readpos = _soundBufPlayed & _soundBufMask;
		volatile uint8* sample = &_soundBuffer[readpos];
		_soundBufPlayed++;
		__asm__ volatile							\
		(											\
			"moveq.l	#0,d3\n\t"					\
			"move.b		(%0),d3\n\t"				\
			"lsl.w		#3,d3\n\t"					\
			"move.l		0(%1,d3.w),d5\n\t"			\
			"move.l		4(%1,d3.w),d4\n\t"			\
			"lea.l		0x00ff8800,a4\n\t"			\
			"movep.w	d5,0(a4)\n\t"				\
			"swap		d5\n\t"						\
			"movep.w	d5,0(a4)\n\t"				\
			"movep.w	d4,0(a4)\n\t"				\
		: 											\
		: "a"(sample), "a"(_soundTable_S8)			\
		: "d3", "d4", "d5", "a4", "memory", "cc"	\
		);
	}
	*((volatile unsigned char*)0x00FFFA0F) &= ~(1<<5);
}

//-------------------------------------------------------------------------------------
static void __attribute__ ((interrupt)) timerA_COVOX(void)
{
	if (_soundBufPlayed < _soundBufWritten)
	{
		uint32 readpos = _soundBufPlayed & _soundBufMask;
		uint8 sample = _soundBuffer[readpos] + 0x80;
		YM2149_WR(15, sample);
		_soundBufPlayed++;
	}
	*((volatile unsigned char*)0x00FFFA0F) &= ~(1<<5);
}


//-------------------------------------------------------------------------------------
static void __attribute__ ((interrupt)) timerA_MV16(void)
{
	if (_soundBufPlayed < _soundBufWritten)
	{
		uint32 readpos = _soundBufPlayed & _soundBufMask;
		uint16 sample = ((_soundBuffer[readpos] + 0x80) & 0xFF) << 4;
		byte b = *((volatile byte*)(0x00fa0000 + sample));
		_soundBufPlayed++;
	}
	*((volatile unsigned char*)0x00FFFA0F) &= ~(1<<5);
}

//-------------------------------------------------------------------------------------
static void __attribute__ ((interrupt)) timerA_Replay8(void)
{
	if (_soundBufPlayed < _soundBufWritten)
	{
		uint32 readpos = _soundBufPlayed & _soundBufMask;
		uint16 sample = ((_soundBuffer[readpos] + 0x80) & 0xFF) << 1;
		byte b = *((volatile byte*)(0x00fa0000 + sample));
		_soundBufPlayed++;
	}
	*((volatile unsigned char*)0x00FFFA0F) &= ~(1<<5);
}

//-------------------------------------------------------------------------------------
static void __attribute__ ((interrupt)) timerA_Replay8S(void)
{
	if (_soundBufPlayed < _soundBufWritten)
	{
		uint32 readpos = _soundBufPlayed & _soundBufMask;
		uint16 sample = ((_soundBuffer[readpos] + 0x80) & 0xFF) << 1;
		byte b1 = *((volatile byte*)(0x00fa0000 + sample));
		byte b2 = *((volatile byte*)(0x00fa0200 + sample));
		_soundBufPlayed++;
	}
	*((volatile unsigned char*)0x00FFFA0F) &= ~(1<<5);
}

//-------------------------------------------------------------------------------------
static void __attribute__ ((interrupt)) timerA_Replay16(void)
{
	if (_soundBufPlayed < _soundBufWritten)
	{
		uint32 readpos = _soundBufPlayed & _soundBufMask;
		uint16 sample = ((_soundBuffer[readpos] + 0x80) & 0xFF) << 7;
		byte b = *((volatile byte*)(0x00fa0000 + sample));
		_soundBufPlayed++;
	}
	*((volatile unsigned char*)0x00FFFA0F) &= ~(1<<5);
}



//-------------------------------------------------------------------------------------
uint32 OSystem_Atari::soundFreq(SoundPlayer player, byte quality)
{
	uint32 freq = 11025;

	switch(player)
	{
		case kSoundPlayer_DMA:
			freq = quality ? 12516 : 6258;
			break;

		case kSoundPlayer_YM:
			freq = quality ? 11025 : 5512;
			break;

		case kSoundPlayer_Covox:
		case kSoundPlayer_MV16:
		case kSoundPlayer_Replay8:
		case kSoundPlayer_Replay8S:
		case kSoundPlayer_Replay16:
		default:
			freq = quality ? 11025 : 5512;
			break;
	}
	return freq;
}

//-------------------------------------------------------------------------------------
void OSystem_Atari::initSound(SoundPlayer player, byte quality)
{
	irq_disable();

	// force stop sounds
	stopSound(true);

	// remove sound player
	if (_soundPlayer != kSoundPlayer_Off)
	{
		if (_soundBuffer)
		{
			free((void*)_soundBuffer);
			_soundBuffer = 0;
		}
		switch(_soundPlayer)
		{
			case kSoundPlayer_DMA:
				DMA_SetCtrl(false, false);
				break;
			case kSoundPlayer_YM:
			case kSoundPlayer_Covox:
			case kSoundPlayer_MV16:
			case kSoundPlayer_Replay8:
			case kSoundPlayer_Replay8S:
			case kSoundPlayer_Replay16:
				break;
		}
	}

	_soundActive = false;
	_soundBufWritten = 0;
	_soundBufPlayed = 0;	

	// auto select player based on hardware capabilities
	if (player == kSoundPlayer_Auto)
	{
		if (_snd & SNDCOOKIE_DMA)		player = kSoundPlayer_DMA;
		else if (_snd & SNDCOOKIE_YM)	player = kSoundPlayer_YM;
		else							player = kSoundPlayer_Off;
	}

	// install sound player
	if (player != kSoundPlayer_Off)
	{
		// sanity check against hardware capabilities
		if ((player == kSoundPlayer_DMA) && !(_snd & SNDCOOKIE_DMA))
			player = kSoundPlayer_YM;

		if ((player == kSoundPlayer_YM) && !(_snd & SNDCOOKIE_YM))
			player = kSoundPlayer_Off;

		// quality setting
		switch (player)
		{
			case kSoundPlayer_DMA:
				// low quality on standard STE with 68000 CPU.
				// high quality on everything else including Mega STE.
				//if ((_cpu == 0) && MCH_IS_STE && !MCH_IS_MEGA_STE)
				//	quality = 0;

				// paranoia check. 6258hz is only available on STE/MegaSTE
				if (!MCH_IS_STE)
					quality = 1;
				break;

			case kSoundPlayer_YM:
				quality = (_cpu >= 30) ? 1 : 0;
				break;

			default:
				quality = (_cpu >= 30) ? 1 : 0;
				break;
		}

		// sanity check quality choice
		quality &= 1;

		// create sound buffer
		uint16 memtype	= (player == kSoundPlayer_DMA) ? ALLOC_STRAM_ONLY : ALLOC_TTRAM_PREFER;
		_soundBufSize 	= (2 * 1024);
		_soundChunkSize = (2 * 1024);
		if (player == kSoundPlayer_DMA)
		{
			_soundBufSize <<= quality;
			_soundChunkSize <<= quality;
			_soundChunkSize = _soundBufSize >> 1;
		}
		_soundBuffer = (byte*) atari_alloc((_soundBufSize * 3), memtype);
		_tempSoundBuffer = &_soundBuffer[_soundBufSize];
		_soundBufMask = (_soundBufSize - 1);	
		memset((void*)_soundBuffer,0, _soundBufSize);	
		memset((void*)_tempSoundBuffer,0, _soundBufSize);	

		// player specific init
		int hz = soundFreq(player, quality);
		switch (player)
		{
			case kSoundPlayer_DMA:
			{
				// Put Falcon in STE compatibility the same way as FPATCH2.PRG
				if (MCH_IS_FALCON)
				{
				#if 1
					Devconnect(0,8,0,0,1);	// src = DMA, dst = D/A, clk=internal, prescale=STE compatible, handshake=off
					Setmode(0);				// 8bit stereo
					Soundcmd(2, 64);		// left gain
					Soundcmd(3, 64);		// right gain
					Soundcmd(6, 3);			// STE compatible. Prescale 160
					Soundcmd(4, 3);			// adder input: A/D & matrix
					Soundcmd(5, 3);			// A/D input: Left+Right
				#elif 0
					__asm__ volatile							\
					(											\
						/*	devconnect(0,8,0,0,1)			*/	\
						/*		src = DMA output			*/	\
						/*		dst = D/A converter			*/	\
						/*		clk = internal 25.175Mhz	*/	\
						/*		prescale = STE compatible	*/	\
						/*		handshake = off				*/	\
						"move.w		#1,-(sp)\n\t"				\
						"move.w		#0,-(sp)\n\t"				\
						"move.w		#0,-(sp)\n\t"				\
						"move.w		#8,-(sp)\n\t"				\
						"move.w		#0,-(sp)\n\t"				\
						"move.w		#139,-(sp)\n\t"				\
						"trap		#14\n\t"					\
						"adda.w		#12,sp\n\r"					\
						/*	setmode(0)						*/	\
						/*		mode = 8bit stereo			*/	\
						"move.w		#0,-(sp)\n\t"				\
						"move.w		#132,-(sp)\n\t"				\
						"trap		#14\n\t"					\
						"addq.l		#4,sp\n\t"					\
						/*	soundcmd(2,64)					*/	\
						/*		D/A left in atten. = 64		*/	\
						/*		D/A right in atten. = 64	*/	\
						"move.w		#64,-(sp)\n\t"				\
						"move.w		#2,-(sp)\n\t"				\
						"move.w		#130,-(sp)\n\t"				\
						"trap		#14\n\t"					\
						"move.w		#3,2(sp)\n\t"				\
						"trap		#14\n\t"					\
						"addq.l		#6,sp\n\t"					\
						/*	soundcmd(6,3)					*/	\
						/*		STE compat prescale 160		*/	\
						"move.w		#3,-(sp)\n\t"				\
						"move.w		#6,-(sp)\n\t"				\
						"move.w		#130,-(sp)\n\t"				\
						"trap		#14\n\t"					\
						/*	soundcmd(4,3)					*/	\
						/*		A/D adder					*/	\
						"move.w		#4,2(sp)\n\t"				\
						"trap		#14\n\t"					\
						/*	soundcmd(5,3)					*/	\
						/*		A/D input channels			*/	\
						"move.w		#5,2(sp)\n\t"				\
						"trap		#14\n\t"					\
						"addq.w		#6,sp\n\t"					\
						: : : "memory", "cc"					\
					);
				#endif
				}

				memset((void*)_soundBuffer, 0, _soundBufSize);
				uint32 st = (uint32) &_soundBuffer[0];
				uint32 en = (uint32) &_soundBuffer[_soundChunkSize];
				DMA_SetMode(false, quality);
				DMA_SetAddr(st, en);
				DMA_SetCtrl(true, true);
			}
			break;

			case kSoundPlayer_YM:
				if (MCH_IS_FALCON)
					irq_install_timerA(timerA_YM_Falcon, hz);
				else
					irq_install_timerA(timerA_YM, hz);
				break;

			case kSoundPlayer_Covox:
				YM2149_LOCK();
				YM2149_WR_MASKED(7, 0x80, 0x80);
				YM2149_UNLOCK();
				irq_install_timerA(timerA_COVOX, hz);
				break;
		
			case kSoundPlayer_MV16:
				irq_install_timerA(timerA_MV16, hz);
				break;		

			case kSoundPlayer_Replay8:
				irq_install_timerA(timerA_Replay8, hz);
				break;		

			case kSoundPlayer_Replay8S:
				irq_install_timerA(timerA_Replay8S, hz);
				break;		

			case kSoundPlayer_Replay16:
				irq_install_timerA(timerA_Replay16, hz);
				break;		
		}
	}
	
	_soundQuality = quality;
	_soundPlayer = player;
	irq_enable();
}

//-------------------------------------------------------------------------------------
void OSystem_Atari::stopSound(bool forced)
{
	irq_disable();
	if (_soundActive)
	{
		switch (_soundPlayer)
		{
			case kSoundPlayer_DMA:
			{
				if (forced)
				{
				}
			}
			break;
			case kSoundPlayer_YM:
				MidiDriver_STCHIP_Mute(false, true);
				irq_enable_timerA(false);
				break;

			case kSoundPlayer_Covox:
			case kSoundPlayer_MV16:
			case kSoundPlayer_Replay8:
			case kSoundPlayer_Replay8S:
			case kSoundPlayer_Replay16:
				irq_enable_timerA(false);
				break;
		}

		_soundActive = false;
	}

	// kill YM for sound and/or music
	if (forced || (_soundPlayer == kSoundPlayer_YM))
	{
		YM2149_LOCK();
		YM2149_WR_MASKED(7, 0xFF, 0x3F);
		YM2149_UNLOCK();
	}

	irq_enable();
}

//-------------------------------------------------------------------------------------
void OSystem_Atari::updateSound_DMA()
{
	SoundMixer* mixer = g_engine->_mixer;
	_soundActive = mixer->hasActiveSFXChannel();

	uint32 curAddr = DMA_GetAddr();
	uint32 curPos  = (curAddr - (uint32)_soundBuffer) & _soundBufMask;
	if ((curPos > _soundBufWritten) && (curPos < (_soundBufWritten + _soundChunkSize)))
	{
		// refill next buffer
		_soundBufWritten = (_soundBufWritten + _soundChunkSize) & _soundBufMask;
		uint32 st = (uint32) &_soundBuffer[_soundBufWritten];
		uint32 en = (uint32) &_soundBuffer[_soundBufWritten + _soundChunkSize];

		if (_soundActive)
		{
			// temporarily point at silence in case there are hickups in the scummvm soundproc
			DMA_SetAddr((uint32)_tempSoundBuffer, (uint32)(_tempSoundBuffer + _soundChunkSize));
			// fetch samples from scummvm and set pointers for next loop
			_soundProc(_soundProcParam, (byte*)st, _soundChunkSize);
		}
		else
		{
			memset((void*)st, 0, _soundChunkSize);
		}
		DMA_SetAddr(st, en);
	}
} 

//-------------------------------------------------------------------------------------
void OSystem_Atari::updateSound_CPU()
{
	// enable disable sound playback
	SoundMixer* mixer = g_engine->_mixer;
	bool mixerActive = mixer->hasActiveSFXChannel();

	// mixer inactive or dummy sound player
	if (!mixerActive || (_soundPlayer == kSoundPlayer_Off))
	{
		// playback done?
		if (_soundActive && (_soundBufPlayed >= _soundBufWritten))
		{
			stopSound(false);
		}

		// drain mixer in case there is anything left
//		_soundProc(_soundProcParam, (byte*)_tempSoundBuffer, _soundChunkSize);

		return;
	}

	// initialize and start timer if necessary
	if (!_soundActive)
	{
		irq_disable();
		_soundBufPlayed = 0;
		_soundBufWritten = 0;
		memset((void*)_soundBuffer, 0, _soundBufSize);
		switch (_soundPlayer)
		{
			case kSoundPlayer_YM:
			{
				MidiDriver_STCHIP_Mute(true);
				YM2149_LOCK();
				YM2149_WR(0,0);
				YM2149_WR(1,0);
				YM2149_WR(2,0);
				YM2149_WR(3,0);
				YM2149_WR(4,0);
				YM2149_WR(5,0);
				YM2149_WR(6,0);
				YM2149_WR_MASKED(7, 0xFF, 0x3F);
				YM2149_WR(8,0);
				YM2149_WR(9,0);
				YM2149_WR(10,0);
				YM2149_WR(11,0);
				YM2149_WR(12,0);
				YM2149_WR(13,0);
				YM2149_UNLOCK();
				irq_enable_timerA(true);
			}
			break;
			case kSoundPlayer_Covox:
			case kSoundPlayer_MV16:
			case kSoundPlayer_Replay8:
			case kSoundPlayer_Replay8S:
			case kSoundPlayer_Replay16:
			{
				irq_enable_timerA(true);
			}
			break;
		}
		_soundActive = true;
		irq_enable();
	}

	// fetch samples from scummvm
	uint32 readPos = _soundBufPlayed & _soundBufMask;
	uint32 readPtr = (uint32) &_soundBuffer[readPos];
	uint32 writePos = _soundBufWritten & _soundBufMask;
	uint32 writePtr = (uint32) &_soundBuffer[writePos];

	// how many free samples in buffer
	int numSamples = (writePos < readPos) ? (readPos - writePos - 1) : (readPos + (_soundBufSize - writePos) - 1);
	if (numSamples > _soundChunkSize)
		numSamples = _soundChunkSize;

	// no space in player buffer, but we still need to empty the scumm buffer...
	if (numSamples <= 0)
	{
		_soundProc(_soundProcParam, (byte*)_tempSoundBuffer, _soundChunkSize);
		return;
	}

	uint32 samplesWritten = 0;
	int remain = _soundBufSize - writePos;
	if (remain > 0)
	{
		int numSamplesFirst = remain >= numSamples ? numSamples : remain;
		_soundProc(_soundProcParam, (byte*)writePtr, numSamplesFirst);
		samplesWritten += numSamplesFirst;
		remain = numSamples - numSamplesFirst;
	}
	if (remain > 0)
	{
		_soundProc(_soundProcParam, (byte*)_soundBuffer, remain);
		samplesWritten += remain;
	}

	_soundBufWritten += samplesWritten;
} 


//-------------------------------------------------------------------------------------
void OSystem_Atari::updateSound()
{
	static uint32 msec_last = 0;
	uint32 msec = get_msecs();
	_soundQuietDuration = _soundActive ? 0 : _soundQuietDuration + (msec - msec_last);
	msec_last = msec;

	if (!_soundProc)
		return;

	if (_soundPlayer == kSoundPlayer_DMA)
		updateSound_DMA();
	else
		updateSound_CPU();
}

uint32 OSystem_Atari::soundInactiveTime()
{
	return _soundActive ? 0 : _soundQuietDuration;
}


const uint16 _soundTable_S8[] =
{
	0x80C,0x90B,0xA09,0,0x80C,0x90B,0xA09,0,	0x80D,0x908,0xA08,0,0x80B,0x90B,0xA0B,0,
	0x80D,0x909,0xA05,0,0x80C,0x90B,0xA08,0,	0x80D,0x909,0xA02,0,0x80D,0x908,0xA06,0,
	0x80C,0x90B,0xA07,0,0x80D,0x907,0xA07,0,	0x80C,0x90B,0xA06,0,0x80C,0x90A,0xA09,0,
	0x80B,0x90B,0xA0A,0,0x80C,0x90B,0xA02,0,	0x80C,0x90B,0xA00,0,0x80C,0x90A,0xA08,0,
	0x80D,0x906,0xA04,0,0x80D,0x905,0xA05,0,	0x80D,0x905,0xA04,0,0x80C,0x909,0xA09,0,
	0x80D,0x904,0xA03,0,0x80B,0x90B,0xA09,0,	0x80C,0x90A,0xA05,0,0x80B,0x90A,0xA0A,0,
	0x80C,0x909,0xA08,0,0x80B,0x90B,0xA08,0,	0x80C,0x90A,0xA00,0,0x80C,0x90A,0xA00,0,
	0x80C,0x909,0xA07,0,0x80B,0x90B,0xA07,0,	0x80C,0x909,0xA06,0,0x80B,0x90B,0xA06,0,
	0x80B,0x90A,0xA09,0,0x80B,0x90B,0xA05,0,	0x80A,0x90A,0xA0A,0,0x80B,0x90B,0xA02,0,
	0x80B,0x90A,0xA08,0,0x80C,0x907,0xA07,0,	0x80C,0x908,0xA04,0,0x80C,0x907,0xA06,0,
	0x80B,0x909,0xA09,0,0x80C,0x906,0xA06,0,	0x80A,0x90A,0xA09,0,0x80C,0x907,0xA03,0,
	0x80B,0x90A,0xA05,0,0x80B,0x909,0xA08,0,	0x80B,0x90A,0xA03,0,0x80A,0x90A,0xA08,0,
	0x80B,0x90A,0xA00,0,0x80B,0x909,0xA07,0,	0x80B,0x908,0xA08,0,0x80A,0x90A,0xA07,0,
	0x80A,0x909,0xA09,0,0x80C,0x901,0xA01,0,	0x80A,0x90A,0xA06,0,0x80B,0x908,0xA07,0,
	0x80A,0x90A,0xA05,0,0x80A,0x909,0xA08,0,	0x80A,0x90A,0xA02,0,0x80A,0x90A,0xA01,0,
	0x80A,0x90A,0xA00,0,0x809,0x909,0xA09,0,	0x80A,0x908,0xA08,0,0x80B,0x908,0xA01,0,
	0x80A,0x909,0xA06,0,0x80B,0x907,0xA04,0,	0x80A,0x909,0xA05,0,0x809,0x909,0xA08,0,
	0x80A,0x909,0xA03,0,0x80A,0x908,0xA06,0,	0x80A,0x909,0xA00,0,0x809,0x909,0xA07,0,
	0x809,0x908,0xA08,0,0x80A,0x908,0xA04,0,	0x809,0x909,0xA06,0,0x80A,0x908,0xA01,0,
	0x809,0x909,0xA05,0,0x809,0x908,0xA07,0,	0x808,0x908,0xA08,0,0x809,0x909,0xA02,0,
	0x809,0x908,0xA06,0,0x809,0x909,0xA00,0,	0x809,0x907,0xA07,0,0x808,0x908,0xA07,0,
	0x809,0x907,0xA06,0,0x809,0x908,0xA02,0,	0x808,0x908,0xA06,0,0x809,0x906,0xA06,0,
	0x808,0x907,0xA07,0,0x808,0x908,0xA04,0,	0x808,0x907,0xA06,0,0x808,0x908,0xA02,0,
	0x807,0x907,0xA07,0,0x808,0x906,0xA06,0,	0x808,0x907,0xA04,0,0x807,0x907,0xA06,0,
	0x808,0x906,0xA05,0,0x808,0x906,0xA04,0,	0x807,0x906,0xA06,0,0x807,0x907,0xA04,0,
	0x808,0x905,0xA04,0,0x806,0x906,0xA06,0,	0x807,0x906,0xA04,0,0x807,0x905,0xA05,0,
	0x806,0x906,0xA05,0,0x806,0x906,0xA04,0,	0x806,0x905,0xA05,0,0x806,0x906,0xA02,0,
	0x806,0x905,0xA04,0,0x805,0x905,0xA05,0,	0x806,0x905,0xA02,0,0x805,0x905,0xA04,0,
	0x805,0x904,0xA04,0,0x805,0x905,0xA02,0,	0x804,0x904,0xA04,0,0x804,0x904,0xA03,0,
	0x804,0x904,0xA02,0,0x804,0x903,0xA03,0,	0x803,0x903,0xA03,0,0x803,0x903,0xA02,0,
	0x803,0x902,0xA02,0,0x802,0x902,0xA02,0,	0x802,0x902,0xA01,0,0x801,0x901,0xA01,0,
	0x802,0x901,0xA00,0,0x801,0x901,0xA00,0,	0x801,0x900,0xA00,0,0x800,0x900,0xA00,0,
	0x80E,0x90D,0xA0C,0,0x80F,0x903,0xA00,0,	0x80F,0x903,0xA00,0,0x80F,0x903,0xA00,0,
	0x80F,0x903,0xA00,0,0x80F,0x903,0xA00,0,	0x80F,0x903,0xA00,0,0x80E,0x90D,0xA0B,0,
	0x80E,0x90D,0xA0B,0,0x80E,0x90D,0xA0B,0,	0x80E,0x90D,0xA0B,0,0x80E,0x90D,0xA0B,0,
	0x80E,0x90D,0xA0B,0,0x80E,0x90D,0xA0B,0,	0x80E,0x90D,0xA0A,0,0x80E,0x90D,0xA0A,0,
	0x80E,0x90D,0xA0A,0,0x80E,0x90D,0xA0A,0,	0x80E,0x90C,0xA0C,0,0x80E,0x90D,0xA00,0,
	0x80D,0x90D,0xA0D,0,0x80D,0x90D,0xA0D,0,	0x80D,0x90D,0xA0D,0,0x80D,0x90D,0xA0D,0,
	0x80D,0x90D,0xA0D,0,0x80D,0x90D,0xA0D,0,	0x80E,0x90C,0xA0B,0,0x80E,0x90C,0xA0B,0,
	0x80E,0x90C,0xA0B,0,0x80E,0x90C,0xA0B,0,	0x80E,0x90C,0xA0B,0,0x80E,0x90C,0xA0B,0,
	0x80E,0x90C,0xA0B,0,0x80E,0x90C,0xA0B,0,	0x80E,0x90C,0xA0A,0,0x80E,0x90C,0xA0A,0,
	0x80E,0x90C,0xA0A,0,0x80E,0x90C,0xA0A,0,	0x80D,0x90D,0xA0C,0,0x80D,0x90D,0xA0C,0,
	0x80E,0x90C,0xA09,0,0x80E,0x90C,0xA09,0,	0x80E,0x90C,0xA05,0,0x80E,0x90C,0xA00,0,
	0x80E,0x90C,0xA00,0,0x80E,0x90B,0xA0B,0,	0x80E,0x90B,0xA0B,0,0x80E,0x90B,0xA0B,0,
	0x80E,0x90B,0xA0B,0,0x80E,0x90B,0xA0A,0,	0x80E,0x90B,0xA0A,0,0x80E,0x90B,0xA0A,0,
	0x80D,0x90D,0xA0B,0,0x80D,0x90D,0xA0B,0,	0x80D,0x90D,0xA0B,0,0x80E,0x90B,0xA09,0,
	0x80E,0x90B,0xA09,0,0x80E,0x90B,0xA09,0,	0x80D,0x90C,0xA0C,0,0x80D,0x90D,0xA0A,0,
	0x80E,0x90B,0xA07,0,0x80E,0x90B,0xA00,0,	0x80E,0x90B,0xA00,0,0x80D,0x90D,0xA09,0,
	0x80D,0x90D,0xA09,0,0x80E,0x90A,0xA09,0,	0x80D,0x90D,0xA08,0,0x80D,0x90D,0xA07,0,
	0x80D,0x90D,0xA04,0,0x80D,0x90D,0xA00,0,	0x80E,0x90A,0xA04,0,0x80E,0x909,0xA09,0,
	0x80E,0x909,0xA09,0,0x80D,0x90C,0xA0B,0,	0x80E,0x909,0xA08,0,0x80E,0x909,0xA08,0,
	0x80E,0x909,0xA07,0,0x80E,0x908,0xA08,0,	0x80E,0x909,0xA01,0,0x80C,0x90C,0xA0C,0,
	0x80D,0x90C,0xA0A,0,0x80E,0x908,0xA06,0,	0x80E,0x907,0xA07,0,0x80E,0x908,0xA00,0,
	0x80E,0x907,0xA05,0,0x80E,0x906,0xA06,0,	0x80D,0x90C,0xA09,0,0x80E,0x905,0xA05,0,
	0x80E,0x904,0xA04,0,0x80D,0x90C,0xA08,0,	0x80D,0x90B,0xA0B,0,0x80E,0x900,0xA00,0,
	0x80D,0x90C,0xA06,0,0x80D,0x90C,0xA05,0,	0x80D,0x90C,0xA02,0,0x80C,0x90C,0xA0B,0,
	0x80C,0x90C,0xA0B,0,0x80D,0x90B,0xA0A,0,	0x80D,0x90B,0xA0A,0,0x80D,0x90B,0xA0A,0,
	0x80D,0x90B,0xA0A,0,0x80C,0x90C,0xA0A,0,	0x80C,0x90C,0xA0A,0,0x80C,0x90C,0xA0A,0,
	0x80D,0x90B,0xA09,0,0x80D,0x90B,0xA09,0,	0x80D,0x90A,0xA0A,0,0x80D,0x90A,0xA0A,0,
	0x80D,0x90A,0xA0A,0,0x80C,0x90C,0xA09,0,	0x80C,0x90C,0xA09,0,0x80C,0x90C,0xA09,0,
	0x80D,0x90B,0xA06,0,0x80C,0x90B,0xA0B,0,	0x80C,0x90C,0xA08,0,0x80D,0x90B,0xA00,0,
	0x80D,0x90B,0xA00,0,0x80C,0x90C,0xA07,0,	0x80C,0x90C,0xA06,0,0x80C,0x90C,0xA05,0,
	0x80C,0x90C,0xA03,0,0x80C,0x90C,0xA01,0,	0x80C,0x90B,0xA0A,0,0x80D,0x90A,0xA05,0,
	0x80D,0x90A,0xA04,0,0x80D,0x90A,0xA02,0,	0x80D,0x909,0xA08,0,0x80D,0x909,0xA08,0,
};





//-------------------------------------------------------------------------------------
bool OSystem_Atari::poll_cdrom() {
	return false;
}

void OSystem_Atari::play_cdrom(int track, int num_loops, int start_frame, int duration) {
}

void OSystem_Atari::stop_cdrom() {
}

void OSystem_Atari::update_cdrom() {
}


