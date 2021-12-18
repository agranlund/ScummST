/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001/2002 The ScummVM project
 * Copyright (C) 2002 ph0x (GP32 port)
 * Copyright (C) 2021 Anders Granlund (ScummST Atari port)
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
 */


#include "atari.h"
#include "cursor.h"
#include "irq.h"
#include "sound/mixer.h"
#include "common/config-manager.h"
#include "common/sound/mididrv.h"
#include "common/util.h"

extern bool g_slowMachine;

#define USE_EXCEPTION_HANDLER		1
#define USE_TERMINATION_HANDLER		1
#ifdef GAME_SAMNMAX
#define USE_BUSY_CURSOR				0
#else
#define USE_BUSY_CURSOR				0
#endif

#define SCREEN_SHAKE_MIN	-8
#define SCREEN_SHAKE_MAX	8
#define SCREEN_SHAKE_SIZE	((SCREEN_SHAKE_MAX - SCREEN_SHAKE_MIN) / 2)
#define REPT20(q)			{ q q q q q q q q q q q q q q q q q q q q }
#define REPT40(q)			{ q q q q q q q q q q q q q q q q q q q q q q q q q q q q q q q q q q q q q q q q }


// Define to make front+backbuffer controllable using only the MID shifter register.
// Trying to prevent a rare and random glitch, without resorting to Vsync(), which I suspect
// occurs if the MMU/Shifter reads out the registers after we updated one but before we've updated the second.
// It's a horrible waste of 64Kb but this thing wont runt with anything less than 2MB anyway...
#define SCREEN_ALIGN_MIDREG 1


//#define DEBUG_COPYRECT

// TODO_ATARI: should probably remove linea eventually
#include <mint/linea.h>
__LINEA *__aline = NULL;
__FONT  **__fonts;
short  (**__funcs) (void);

#define Linea0() 										\
({														\
	register __LINEA *__xaline __asm__ ("a0");			\
	register __FONT **__xfonts __asm__ ("a1");			\
	register short (**__xfuncs) (void) __asm__ ("a2");	\
														\
	__asm__ volatile									\
	(													\
		"	.word	0xA000"								\
	: "=g"(__xaline), "=g"(__xfonts), "=g"(__xfuncs) 	\
	: 													\
	: __CLOBBER_RETURN("a0") __CLOBBER_RETURN("a1") __CLOBBER_RETURN("a2") "d0", "d1", "d2" AND_MEMORY	\
	);													\
	__aline = __xaline;									\
	__fonts = __xfonts;									\
	__funcs = __xfuncs;									\
})

#define Linea9() 					\
({									\
	__asm__ volatile				\
	(								\
		"	.word	0xA009"			\
	: : : "d0", "d1", "d2", "a0", "a1", "a2" AND_MEMORY	\
	);								\
})

#define Linea10() 					\
({									\
	__asm__ volatile				\
	(								\
		"	.word	0xA00A"			\
	: : : "d0", "d1", "d2", "a0", "a1", "a2" AND_MEMORY	\
	);								\
})


// not really a mutex, but we use it to prevent VBL from
// updating audio while scummvm is doing audio work.
struct Mutex {
	Mutex() { lock = 0; }
	volatile uint32 lock;
};

static Mutex _audioMutex;
static OSystem_Atari* _this;


#ifndef Setscreen
#define Setscreen(a,b,c) trap_14_wllw(0x5,(short)a,(long)b,(long)c)
#endif
#ifndef Logbase
#define Logbase() trap_14_w(0x3)
#endif
#ifndef Physbase
#define Physbase() trap_14_w(0x2)
#endif
#ifndef Getrez
#define Physbase() trap_14_w(0x4)
#endif

uint16 _cursorData[4*16] = 
{
	0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100,
	0x0000, 0x0000, 0x0000, 0x0000,
	0xfc7f, 0xfc7f, 0xfc7f, 0xfc7f,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100,
};
uint16 _cursorMask[16] = 
{
	0x0380,	0x0380,	0x0380,	0x0380,
	0x0380,	0x0380,	0xfc7f,	0xfc7f,
	0xfc7f,	0x0380,	0x0380,	0x0380,
	0x0380,	0x0380,	0x0380,	0x0380,
};
#if USE_BUSY_CURSOR
uint16 _cursorBusyData[4*16] = 
{
	0x0000, 0x0000, 0x0000, 0x0000,
	0x7ffe, 0x7ffe, 0x7ffe, 0x7ffe,
	0x6006, 0x6006, 0x6006, 0x6006,
	0x300c, 0x300c, 0x300c, 0x300c,
	0x1818, 0x1818, 0x1818, 0x1818,
	0x0c30, 0x0c30, 0x0c30, 0x0c30,
	0x0660, 0x0660, 0x0660, 0x0660,
	0x03c0, 0x03c0, 0x03c0, 0x03c0,
	0x0660, 0x0660, 0x0660, 0x0660,
	0x0c30, 0x0c30, 0x0c30, 0x0c30,
	0x1998, 0x1998, 0x1998, 0x1998,
	0x33cc, 0x33cc, 0x33cc, 0x33cc,
	0x67e6, 0x67e6, 0x67e6, 0x67e6,
	0x7ffe, 0x7ffe, 0x7ffe, 0x7ffe,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
};
uint16 _cursorBusyMask[16] = 
{
	0x0000, 0x7ffe, 0x6006, 0x300c,
	0x1818, 0x0c30, 0x0660, 0x03c0,
	0x0660, 0x0c30, 0x1998, 0x33cc,
	0x67e6, 0x7ffe, 0x0000, 0x0000,
};
#endif //USE_BUSY_CURSOR
Cursor _cursor;

#define MAKE_PC_COLOR(_r,_g,_b) _r,_g,_b,0

static uint16 _palST[16];

static byte _palPC[16 * 4] = {
	MAKE_PC_COLOR(48,52,109), MAKE_PC_COLOR(68,36,52), MAKE_PC_COLOR(48,52,109), MAKE_PC_COLOR(78,74,78),
	MAKE_PC_COLOR(133,76,48), MAKE_PC_COLOR(52,101,36), MAKE_PC_COLOR(208,70,72), MAKE_PC_COLOR(117,113,97),
	MAKE_PC_COLOR(89,125,206), MAKE_PC_COLOR(210,125,44), MAKE_PC_COLOR(133,149,161), MAKE_PC_COLOR(109,170,44),
	MAKE_PC_COLOR(210,170,153), MAKE_PC_COLOR(109,194,202), MAKE_PC_COLOR(218,212,94), MAKE_PC_COLOR(255,255,255)
};


const OSystem_Atari::DriverCfg allMusicDrivers[4] = {
	{ "Off",						1 },
	{ "Atari YM",					2 },
	{ "General Midi",				3 },
	{ "Roland MT32",				4 }
};

const OSystem_Atari::DriverCfg allSoundDriversDrivers[8] = {
	{ "Off",						OSystem_Atari::kSoundPlayer_Off 		},
	{ "Atari YM",					OSystem_Atari::kSoundPlayer_YM 			},
	{ "Atari DMA",					OSystem_Atari::kSoundPlayer_DMA 		},
	{ "Covox Speech Thing",			OSystem_Atari::kSoundPlayer_Covox 		},
	{ "Ubisoft MV16",				OSystem_Atari::kSoundPlayer_MV16 		},
	{ "Microdeal Replay 8",			OSystem_Atari::kSoundPlayer_Replay8 	},
	{ "Microdeal Replay 16",		OSystem_Atari::kSoundPlayer_Replay16 	},
	{ "Microdeal Stereo Replay",	OSystem_Atari::kSoundPlayer_Replay8S 	}
};


void enable_VBL()
{
	(*((volatile int16*)0x00000452))++;
}

void disable_VBL()
{
	(*((volatile int16*)0x00000452))--;
}

uintptr_t atari_alloc(uint32 size, uint16 mode)
{
	// todo: we may need to append 0x20 to flags for MiNT:
	// http://toshyp.atari.org/en/00500c.html

	uintptr_t ret = Mxalloc(size, mode | 0x20);
	if (ret & 0x80000000)
		ret = (uintptr_t) malloc(size);
	return ret;
}

#if USE_EXCEPTION_HANDLER
extern "C" void InstallExceptionHandler(int vec, int(*)(int));
extern "C" void RemoveExceptionHandler(int vec);

static const uint32 exception_list[] =
{
	0x08,							// bus error
	0x0C,							// address error
	0x10,							// illegal instruction
	0x14,							// division by zero
	//0x18,							// chk instruction
	//0x1C,							// trapv instruction
	0x20,							// privilege violation
	//0x24,							// trace instruction
	//0x28,							// line-a
	//0x2C,							// line-f
	0x30, 0x34, 0x38, 0x3C,			// exception 12-23
	0x40, 0x44, 0x48, 0x4C,
	0x50, 0x54, 0x58, 0x5C,
	// 0x60-0x7C 					// interrupts
	// 0x80-0xBC 					// traps
	0xC0, 0xC4, 0xC8, 0xCC,			// exception 48-63
	0xD0, 0xD4, 0xD8, 0xDC,
	0xE0, 0xE4, 0xE8, 0xEC,
	0xF0, 0xF4, 0xF8, 0xFC,
};
#endif //USE_EXCEPTION_HANDLER

#define UNSET_TERMINATION_HANDLER (void(*)(void))-1
void (*_oldTerminationHandler)(void) = UNSET_TERMINATION_HANDLER;

void OSystem_Atari::TerminationHandler(void)
{
	ExceptionHandler(0x102);
}

int OSystem_Atari::ExceptionHandler(int vec)
{
	// ignore division by zero
	switch (vec)
	{
		// ignore and keep going
		case 0x14:		// division by zero
			return 1;
			
		// no worries, pass it onto TOS
		case 0x18:		// chk
		case 0x1C:		// trapv
		case 0x24:		// trace
		case 0x28:		// line-a
		case 0x2c:		// line-f
			return 0;

		case 0x102:		// termination handler
			_this->cleanup(true);
			return 0;

		// panic, restore screen and let TOS deal with us
		default:
			// we are going to rely on TOS to terminate and show bombs
			// or some other kind of crash information.
			// set a _crashed flag just in case we aren't being terminated.
			_this->_crashed = true;		
			_this->cleanup(true);
			return 0;
	}
}

void OSystem_Atari::installExceptionHandlers()
{
#if USE_EXCEPTION_HANDLER
	for (int i=0; i<sizeof(exception_list)/exception_list[0]; i++)
		InstallExceptionHandler(exception_list[i], OSystem_Atari::ExceptionHandler);
#endif
#if USE_TERMINATION_HANDLER
	_oldTerminationHandler = Setexc(0x102, OSystem_Atari::TerminationHandler);
#endif
}

void OSystem_Atari::removeExceptionHandlers()
{
#if USE_EXCEPTION_HANDLER
	for (int i=0; i<sizeof(exception_list)/exception_list[0]; i++)
		RemoveExceptionHandler(exception_list[i]);
#endif

#if USE_TERMINATION_HANDLER
	if (_oldTerminationHandler != UNSET_TERMINATION_HANDLER)
		Setexc(0x102, _oldTerminationHandler);
#endif
}

void OSystem_Atari::VBL()
{
	// do nothing until we are properly up and running
	if (_this->_frameCount == 0)
		return;

	// update audio unless we interrupted scummvm in
	// the middle of audio work.
	if (_this->try_lock_mutex(&_audioMutex))
	{
		if (_this->_midiCallback)
			_this->_midiCallback();

		_this->updateTimers();
		
		//if (_this->_isLoading == 0)
			_this->updateSound();

		_this->unlock_mutex(&_audioMutex);
	}

	_this->update_mouse();
	_this->_flipPending = false;
}

OSystem_Atari::OSystem_Atari()
{
	_this = this;
	_crashed = false;
	_frameCount = 0;
	_midiCallback = NULL;
	_timerCallback = NULL;
	_timerDuration = 0;
	_timerExpiry = 0;
	_soundActive = false;
	_soundQuietDuration = 0;
	_soundProc = NULL;
	_soundProcParam = NULL;
	_mouseBufSrc = NULL;
	_mouseBuf = NULL;
	_mouseBak = NULL;
	_mouseVisible = false;
	_mouseDrawn = false;
	_flipPending = false;
	_isLoading = 0;
	_shakePos = 0;
	_shakePosPrev = 0;
	_soundPlayer = kSoundPlayer_Off;
	_mch = 0;
	_vdo = 0;
	_snd = 0;

	_oldLogBase = 0;
	_oldPhysBase = 0;
	_oldRez = 0;

	_dirtyTop = SCREEN_HEIGHT;
	_dirtyBottom = 0;
	_prevFrameDirtyMask.clear();
}


uint32 OSystem_Atari::get_cookie(uint32 cookie)
{
	typedef struct
	{
	    uint32 id;
	    uint32 value;
	} Cookie;

    Cookie* jar = (Cookie*)(Setexc(0x05A0/4,(const void (*)(void))-1));
	while (jar && jar->id) {
		if (jar->id == cookie)
			return jar->value;
		jar++;
	}
	return 0;
}


void OSystem_Atari::drawConsoleScreen()
{
	uint16* tosScreenPtr = (uint16*) *((volatile uint32*)0x44e);
	Setscreen(-1, tosScreenPtr, -1);
	extern Common::String gGameName;
	extern const char* gScummVMBuildDate;
	Cconws("\33E"); Cconws("\33p");
	Cconws("\33Y"); Cconout(32+0); Cconout(32+0);
	int left = (40 - gGameName.size()) >> 1;
	for (int i=0; i<left; i++)
		Cconws(" ");
	Cconws(gGameName.c_str());
	int right = 40 - (gGameName.size() + left);
	for (int i=0; i<right; i++)
		Cconws(" ");
	Cconws("\n\r");
	Cconws("\33q"); Cconws("\n\r");
	Cconws("\33Y"); Cconout(32+24); Cconout(32+0);
	Cconws("Build: "); Cconws(__DATE__);
}

void OSystem_Atari::installStart()
{
	drawConsoleScreen();
	Cconws("\33Y"); Cconout(32+2); Cconout(32+0);	
	Cconws("Installing: [..........................]\n\r");
}

void OSystem_Atari::installDone()
{
	Cconws("\33E");
	// back to game screen
	Setscreen(-1, _screen, -1);
}

void OSystem_Atari::installUpdate(byte progCur, byte progMax)
{
	const char* progchars = "##########################\n\r";
	Cconws("\33Y"); Cconout(32+2); Cconout(32+13);
	int cnt = (progCur * 27) / progMax;
	if (cnt < 0)
		cnt = 0;
	if (cnt > 26)
		cnt = 26;
	Cconws(progchars + 26 - cnt);
}

void OSystem_Atari::setIsLoading(bool isloading)
{
	if (isloading)
	{
		#if USE_BUSY_CURSOR
		if (_isLoading == 0)
			_cursor.SetOverrideFrame(1);
		#endif
		_isLoading++;
	}
	else
	{
		_isLoading--;
		#if USE_BUSY_CURSOR
		if (_isLoading == 0)
			_cursor.ClearOverrideFrame();
		#endif
	}
}

bool OSystem_Atari::init()
{
	// hardware detect
	_mch = get_cookie('_MCH');
	_vdo = get_cookie('_VDO');
	_snd = get_cookie('_SND');
	_cpu = get_cookie('_CPU');

	if (_cpu > 30)				g_slowMachine = false;	// 040 or 060
	else if (MCH_IS_FALCON)		g_slowMachine = true;	// 030 falcon
	else if (_cpu == 30)		g_slowMachine = false;	// 030 TT or accelerated machine
	else if (MCH_IS_MEGA_STE)	g_slowMachine = true;	// mega STE
	else						g_slowMachine = true;	// standard ST/STE

	// assume YM is available if no cookie jar is present
	if (_snd == 0)
		_snd = 1;

	// disable TOS sounds
	byte _conterm = *((volatile byte*)0x484);
	_conterm &= ~(1 << 0);	// disable click
	_conterm &= ~(1 << 2);	// disable bell
	*((volatile byte*)0x484) = _conterm;

	// initialize video
	initGfx();

	// install exception handler
	installExceptionHandlers();

	// system init
	irq_init();
	irq_install_vbl(OSystem_Atari::VBL);


	// sound and music drivers
	memset(musicDrivers, 0, 16 * sizeof(DriverCfg*));
	memset(soundDrivers, 0 , 16 * sizeof(DriverCfg*));

	for (int i=0,j=0; i<4; i++)
		musicDrivers[j++] = &allMusicDrivers[i];

	bool haveDMA = (_snd & 2) != 0;
	for (int i=0,j=0; i<8; i++)
	{
		if ((allSoundDriversDrivers[i].id == kSoundPlayer_DMA) && !haveDMA)
			continue;

		soundDrivers[j++] = &allSoundDriversDrivers[i];
	}

	// first time config
	if (GetConfig(kConfig_SoundDriver) == kSoundPlayer_Auto)
	{
		if (_snd & 2)
			SetConfig(kConfig_SoundDriver, kSoundPlayer_DMA);
		else	
			SetConfig(kConfig_SoundDriver, kSoundPlayer_YM);

		SetConfig(kConfig_MusicDriver, MD_STCHIP);
		SaveConfig();
	}

	// sound init
	byte quality = 1;	
	byte soundPlayer = GetConfig(kConfig_SoundDriver);
	initSound((SoundPlayer)soundPlayer, quality);
	if (_soundPlayer != soundPlayer)
		SetConfig(kConfig_SoundDriver, _soundPlayer);

	return true;
}

NewGuiColor OSystem_Atari::RGBToColor(uint8 r, uint8 g, uint8 b)
{
	NewGuiColor bestitem = 0;
	uint bestsum = (uint) - 1;
	byte *pal = _palPC;
	r &= ~3; g &= ~3; b &= ~3;
	for (NewGuiColor i = 0; i < 16; i++, pal += 4)
	{
		int ar = pal[0] & ~3;
		int ag = pal[1] & ~3;
		int ab = pal[2] & ~3;
		if (ar == r && ag == g && ab == b)
			return i;

		int r1 = ar - r;
		int r2 = r1 * r1;
		int g1 = ag - g;
		int g2 = g1 * g1;
		int b1 = ab - b;
		int b2 = b1 * b1;
		uint sum = (MUL3(r2) + MUL6(g2) + MUL2(b2));

		if (sum < bestsum) {
			bestsum = sum;
			bestitem = i;
		}
	}
	return bestitem;
}

void OSystem_Atari::initGfx()
{
	_prevFrameDirtyMask.clear();
	_dirtyTop = SCREEN_HEIGHT;
	_dirtyBottom = 0;

	_oldLogBase = Logbase();
	_oldPhysBase = Physbase();
	_oldRez = Getrez();
	for (int i=0; i<16; i++)
		_oldPalette[i] = Setcolor(i, -1);

	Linea0();		// init linea
	Linea10();		// hide mouse

	Vsync();
	set_palette(_palPC, 0, 16);

	// create cursor
	_cursor.CreateFromPlanar(-1, 0, (const byte*)_cursorData, (const byte*)_cursorMask, 16, 16, 7, 7, 16);
	#if USE_BUSY_CURSOR
	_cursor.CreateFromPlanar(-1, 1, (const byte*)_cursorBusyData, (const byte*)_cursorBusyMask, 16, 16, 7, 7, 16);
	#endif
	_cursor.SetFrame(0);

	// backup screen can be in ST or TT ram, don't care
	_screenBackup = (byte*) malloc((SCREEN_WIDTH >> 1) * SCREEN_HEIGHT);

	// front and backbuffers must be in ST Ram and aligned to 256 bytes
	const uint32 screenSize = (SCREEN_WIDTH >> 1) * SCREEN_HEIGHT;
	const uint32 shakeSize = (SCREEN_WIDTH >> 1) * SCREEN_SHAKE_SIZE;
	const uint32 totalSize = shakeSize + screenSize + shakeSize + screenSize + shakeSize;
#if SCREEN_ALIGN_MIDREG
	byte* screenMem = (byte*) ((atari_alloc(totalSize + 0x10000, ALLOC_STRAM_ONLY) + 0xFFFF) & 0x00FF0000);
#else
	byte* screenMem = (byte*) ((atari_alloc(totalSize + 0x100, ALLOC_STRAM_ONLY) + 0xFF) & 0x00FFFF00);
#endif
	_screen = screenMem + shakeSize;
	_backbuffer = _screen + screenSize + shakeSize;
	memset(screenMem, 0, totalSize);

	Vsync();
	Setscreen(-1, _screen, 0);
}

void OSystem_Atari::init_size(uint w, uint h)
{
}


void OSystem_Atari::set_palette(const byte *colors, uint start, uint num)
{
	if (start >= 16)
		return;

	if (start + num > 16)
		num = 16 - start;

	const byte* src = colors;
	uint16* dst = &_palST[start];
	memcpy(&_palPC[start<<2], colors, num<<2);
	for (int i=0; i<num; i++)
	{
		uint16 r = *src++;
		uint16 g = *src++;
		uint16 b = *src++;
		src++;

		uint16 c = ((r << 3) & 0x0700) | ((g >> 1) & 0x0070) | ((b >> 5) & 0x0007);
		//c |= ((r << 7) & 0x0800) | ((g << 3) & 0x0080) | ((b >> 1) & 0x0008);
		*dst++ = c;
	}
	Setpalette(_palST);
 }

void _copy_scanlines(byte* dst, const byte* src, int16 yh)
{
	__asm__ volatile					\
	(									\
		"movem.l d0-d6,-(sp)\n\t"		\
		"bra _cpsll1\n\t"				\
		"_cpsll0:\n\t"					\
		"movem.l (%0)+,d0-d6\n\t"		\
		"movep.l d0,0(%1)\n\t"			\
		"movep.l d1,1(%1)\n\t"			\
		"movep.l d2,8(%1)\n\t"			\
		"movep.l d3,9(%1)\n\t"			\
		"movep.l d4,16(%1)\n\t"			\
		"movep.l d5,17(%1)\n\t"			\
		"movep.l d6,24(%1)\n\t"			\
		"movem.l (%0)+,d0-d6\n\t"		\
		"movep.l d0,25(%1)\n\t"			\
		"movep.l d1,32(%1)\n\t"			\
		"movep.l d2,33(%1)\n\t"			\
		"movep.l d3,40(%1)\n\t"			\
		"movep.l d4,41(%1)\n\t"			\
		"movep.l d5,48(%1)\n\t"			\
		"movep.l d6,49(%1)\n\t"			\
		"movem.l (%0)+,d0-d6\n\t"		\
		"movep.l d0,56(%1)\n\t"			\
		"movep.l d1,57(%1)\n\t"			\
		"movep.l d2,64(%1)\n\t"			\
		"movep.l d3,65(%1)\n\t"			\
		"movep.l d4,72(%1)\n\t"			\
		"movep.l d5,73(%1)\n\t"			\
		"movep.l d6,80(%1)\n\t"			\
		"movem.l (%0)+,d0-d6\n\t"		\
		"movep.l d0,81(%1)\n\t"			\
		"movep.l d1,88(%1)\n\t"			\
		"movep.l d2,89(%1)\n\t"			\
		"movep.l d3,96(%1)\n\t"			\
		"movep.l d4,97(%1)\n\t"			\
		"movep.l d5,104(%1)\n\t"		\
		"movep.l d6,105(%1)\n\t"		\
		"movem.l (%0)+,d0-d6\n\t"		\
		"movep.l d0,112(%1)\n\t"		\
		"movep.l d1,113(%1)\n\t"		\
		"movep.l d2,120(%1)\n\t"		\
		"movep.l d3,121(%1)\n\t"		\
		"movep.l d4,128(%1)\n\t"		\
		"movep.l d5,129(%1)\n\t"		\
		"movep.l d6,136(%1)\n\t"		\
		"movem.l (%0)+,d0-d4\n\t"		\
		"movep.l d0,137(%1)\n\t"		\
		"movep.l d1,144(%1)\n\t"		\
		"movep.l d2,145(%1)\n\t"		\
		"movep.l d3,152(%1)\n\t"		\
		"movep.l d4,153(%1)\n\t"		\
		"lea 160(%1),%1\n\t" 			\
		"_cpsll1:\n\t"					\
		"dbra %2,_cpsll0\n\t"			\
		"movem.l (sp)+,d0-d6\n\t"		\
	: : "a"(src), "a"(dst), "d"(yh) : "d0","d1","d2","d3","d4","d5","d6","memory" );
}

#ifdef DEBUG_COPYRECT
static uint8 _debugRectColor;
#endif

template <uint16 height> void _blit_screen(byte* &dst, const uint32*& src, const uint32*& mask, int16& yh)
{
	// 8 pixels per column
	#ifdef DEBUG_COPYRECT	
		#define WRLINE(_doff,_soff) { uint32 px = c2pfill16[_debugRectColor & 15]; __asm__ volatile ( "movep.l %1," #_doff "(%0)\n\t" : : "a"(dst), "d"(px) : "memory" ); }
	#else
		#define WRLINE(_doff,_soff) { __asm__ volatile ( "movep.l %1," #_doff "(%0)\n\t" : : "a"(dst), "d"(src[_soff]) : "memory" ); }
	#endif

	// 8 lines per row
	#define WRTILE(_x,_y) { \
		if (m & (1 << _y)) \
		{ \
			WRLINE( ((((_y*8)+0)*160)+_x), (((_y*8)+0)*40) ); \
			WRLINE( ((((_y*8)+1)*160)+_x), (((_y*8)+1)*40) ); \
			WRLINE( ((((_y*8)+2)*160)+_x), (((_y*8)+2)*40) ); \
			WRLINE( ((((_y*8)+3)*160)+_x), (((_y*8)+3)*40) ); \
			WRLINE( ((((_y*8)+4)*160)+_x), (((_y*8)+4)*40) ); \
			WRLINE( ((((_y*8)+5)*160)+_x), (((_y*8)+5)*40) ); \
			WRLINE( ((((_y*8)+6)*160)+_x), (((_y*8)+6)*40) ); \
			WRLINE( ((((_y*8)+7)*160)+_x), (((_y*8)+7)*40) ); \
		} \
	}

	// 20 rows
	#define WRCOL(_x) { \
		uint32 m = *mask++; \
		if (m) { \
			if (height != 0) { \
				if (height >=   8) WRTILE(_x, 0); \
				if (height >=  16) WRTILE(_x, 1); \
				if (height >=  24) WRTILE(_x, 2); \
				if (height >=  32) WRTILE(_x, 3); \
				if (height >=  40) WRTILE(_x, 4); \
				if (height >=  48) WRTILE(_x, 5); \
				if (height >=  56) WRTILE(_x, 6); \
				if (height >=  64) WRTILE(_x, 7); \
				if (height >=  72) WRTILE(_x, 8); \
				if (height >=  80) WRTILE(_x, 9); \
				if (height >=  88) WRTILE(_x,10); \
				if (height >=  96) WRTILE(_x,11); \
				if (height >= 104) WRTILE(_x,12); \
				if (height >= 112) WRTILE(_x,13); \
				if (height >= 120) WRTILE(_x,14); \
				if (height >= 128) WRTILE(_x,15); \
				if (height >= 136) WRTILE(_x,16); \
				if (height >= 144) WRTILE(_x,17); \
				if (height >= 152) WRTILE(_x,18); \
				if (height >= 160) WRTILE(_x,19); \
				if (height >= 168) WRTILE(_x,20); \
				if (height >= 176) WRTILE(_x,21); \
				if (height >= 184) WRTILE(_x,22); \
				if (height >= 192) WRTILE(_x,23); \
				if (height >= 200) WRTILE(_x,24); \
			} else { \
				switch(25 - (yh >> 3)) { \
					case  0: WRTILE(_x,  0); \
					case  1: WRTILE(_x,  1); \
					case  2: WRTILE(_x,  2); \
					case  3: WRTILE(_x,  3); \
					case  4: WRTILE(_x,  4); \
					case  5: WRTILE(_x,  5); \
					case  6: WRTILE(_x,  6); \
					case  7: WRTILE(_x,  7); \
					case  8: WRTILE(_x,  8); \
					case  9: WRTILE(_x,  9); \
					case 10: WRTILE(_x, 10); \
					case 11: WRTILE(_x, 11); \
					case 12: WRTILE(_x, 12); \
					case 13: WRTILE(_x, 13); \
					case 14: WRTILE(_x, 14); \
					case 15: WRTILE(_x, 15); \
					case 16: WRTILE(_x, 16); \
					case 17: WRTILE(_x, 17); \
					case 18: WRTILE(_x, 18); \
					case 19: WRTILE(_x, 19); \
					case 20: WRTILE(_x, 20); \
					case 21: WRTILE(_x, 21); \
					case 22: WRTILE(_x, 22); \
					case 23: WRTILE(_x, 23); \
					case 24: WRTILE(_x, 24); \
					default: break; \
				} \
			} \
		} \
		src++; \
		if (_x) \
			dst += 8;\
	}

	// 40 columns
#if 0
	// unrolling everything costs an additional 300kb memory
	REPT20(WRCOL(0); WRCOL(1););
#else
	int16 count = 20;
	while (count)
	{
		WRCOL(0);
		WRCOL(1);
		count--;
	}
#endif
}


void OSystem_Atari::copy_screen(const byte *buf, int y, int h, const uint32* mask, bool doublebuffer)
{
	waitForPresent();

#ifdef DEBUG_COPYRECT
	_debugRectColor++;
#endif

	int16 y0 = y;
	int16 y1 = y + h;

	// clip
	if (y0 >= SCREEN_HEIGHT)
		return;
	if (y0 < 0)
		y0 = 0;
	if (y1 > SCREEN_HEIGHT)
		y1 = SCREEN_HEIGHT;

	int16 yh = y1 - y0;
	if (yh <= 0)
		return;

	if (y0 < _dirtyTop)
		_dirtyTop = y0;
	if (y1 > _dirtyBottom)
		_dirtyBottom = y1;

	// unmasked copy, used when scrolling
	if (mask == 0) {
		const byte* src = buf;
		byte* dst = _backbuffer + MUL160(y0);
		_copy_scanlines(dst, src, yh);
		_prevFrameDirtyMask.updateRange(y0,y1);
		if (!doublebuffer) {
			disable_VBL();
			undraw_mouse();
			memcpy(_screen, _backbuffer, SCREEN_HEIGHT * 160);
			_prevFrameDirtyMask &= ~Common::DirtyMask::bitMask(y0,y1);
			_dirtyTop = SCREEN_HEIGHT;
			_dirtyBottom = 0;
			draw_mouse();
			enable_VBL();
		}
		return;
	}

	// make combined dirtymask from this and previous frame
	Common::DirtyMask combinedMask;
	uint32* omptr = _prevFrameDirtyMask.maskBuf();
	uint32* cmptr = combinedMask.maskBuf();
	const uint32* nmptr = mask;
	uint32 lim = Common::DirtyMask::bitMask(0,yh);
	uint32 nmask = ~Common::DirtyMask::bitMask(y0,y1);
	if (!doublebuffer)
		_prevFrameDirtyMask &= nmask;

	uint32 om, nm;
	if (y0 == 0) {
		if (y1 == SCREEN_HEIGHT) {
			REPT40(om = *omptr; nm = lim & *nmptr++; *cmptr++ = om | nm; *omptr++ = nm;);
		} else {
			REPT40(om = *omptr; nm = lim & *nmptr++; *cmptr++ = om | nm; *omptr++ = (om & nmask) | nm;);
		}
	} else{
		int16 mshift = (y0 >> 3);
		uint32 som;
		if (mshift < 16) {
			REPT40(om = *omptr; som = om >> mshift; nm = lim & *nmptr++; *cmptr++ = som | nm; nm <<= mshift; *omptr++ = (om & nmask) | nm;);
		} else {
			mshift -= 16;
			REPT40(om = *omptr; som = om; M68K_SWAP(som); som >>= mshift; som &= 0x0000FFFF; nm = lim & *nmptr++; *cmptr++ = som | nm; M68K_SWAP(nm); nm &= 0xFFFF0000; nm <<= mshift; *omptr++ = (om & nmask) | nm;);
		}
	}

	// and draw to backbuffer
	byte* dst = _backbuffer + MUL160(y0);
	const uint32* src = (const uint32*)buf;
	const uint32* msk = (const uint32*)combinedMask.maskBuf();
	switch(yh)
	{
		case 200: _blit_screen<200>(dst,src,msk,yh); break;	// main screen
		case 144: _blit_screen<144>(dst,src,msk,yh); break; // main screen
		case  56: _blit_screen< 56>(dst,src,msk,yh); break; // verb screen
		default:  _blit_screen<  0>(dst,src,msk,yh); break; // fallback
	}

	// special case for transition effects
	if (!doublebuffer) {
		disable_VBL();
		undraw_mouse();
		memcpy(_screen, _backbuffer, SCREEN_HEIGHT * 160);
		_prevFrameDirtyMask.clear();
		_dirtyTop = SCREEN_HEIGHT;
		_dirtyBottom = 0;
		draw_mouse();
		enable_VBL();
	}
}


void OSystem_Atari::copy_rect(const byte *buf, int pitch, int x, int y, int w, int h)
{
}

// Moves the screen content around by the given amount of pixels
// but only the top height pixel rows, the rest stays untouched
void OSystem_Atari::move_screen(int dx, int dy, int height)
{ 
}

void OSystem_Atari::waitForPresent(){
	int16 timeout = 1000;
	while (_flipPending && timeout)
	{
		Vsync();
		timeout--;
	}
}

// Update the dirty areas of the screen
void OSystem_Atari::update_screen()
{
	waitForPresent();
	disable_VBL();
	if (_dirtyBottom)
	{
	#if 1
		// copy missing updates from current screenbuffer
		if ((_dirtyTop > 0) || (_dirtyBottom < SCREEN_HEIGHT))
		{   
			undraw_mouse();
			if (_dirtyTop > 0) {
				memcpy(_backbuffer, _screen, MUL160(_dirtyTop));
			}
			if (_dirtyBottom < SCREEN_HEIGHT) {
				uint32 yoffs = MUL160(_dirtyBottom);
				uint32 size = (SCREEN_HEIGHT * 160) - yoffs;
				memcpy(_backbuffer + yoffs, _screen + yoffs, size);
			}
			draw_mouse();
		}
	#endif
		byte* oldscreen = _screen;
		_screen = _backbuffer;
		_backbuffer = oldscreen;
		_dirtyTop = SCREEN_HEIGHT;
		_dirtyBottom = 0;

		//
		// Set new physical screen
		//
		// As far as I understand, the MMU reads these regs at the beginning of VBlank
		// which is an extremely unfortunate design choice.
		// This was possibly rectified on the Falcon reading it at the end of Vblank instead (?)
		//
		// Since there are multiple screen registers that needs to update the only safe way to do
		// so would be if we know there is enough time until Vblank, or after VBlank has triggered
		// and the MMU has just fetched the values.
		// Otherwise we run the risk of it happening in the middle of us updating them, possibly ending
		// up with corrupt frame pointers sometimes.
		// Setting the values after a Vsync(), or in the VBL handler would introduce 1 frame of delay
		// since the change would not happen until the end of the frame after that.
		//
		//
		// I opted to sacrifice 64KB RAM to let me control the screen pointer with a single register.
		// This lets us safely trigger the flip whenever we want and without delays.
		//
		// _flipPending then signals to our VBL handler that the mouse needs to be undrawn from
		// what is now the backbuffer and redrawn again onto what is now the frontbuffer.
		// This redrawing will happen in the same vertical blank as the MMU is refreshing the physical screen address
		//
		// Furthermore, we use this boolean as a way to wait until the VBL has happened before
		// allowing another flip or drawing operation - in case we're running super fast (yeah right, as if..)
		//
		_flipPending = true;
		byte* screenPtr = _screen - MUL160(_shakePos);
#if SCREEN_ALIGN_MIDREG
		*(volatile byte*)0xFFFF8203L = ((uint32)screenPtr) >> 8;
#else
	#if 0
		// This is likely the safe option, at the cost of a delay we really don't want...
		Vsync();
		Setscreen(-1, screenPtr, -1);
	#else
		byte screenHI	= ((uint32)screenPtr)>>16;
		byte screenMID	= ((uint32)screenPtr)>>8; 
		irq_disable();
		*(volatile byte*)0xFFFF8201L = screenHI;
		// disabling interrupts will probably not prevent the MMU from being able to fetch the regs when the PC is in this location.
		// but, at the very least it should prevent an interrupt handler from stalling us at this unfortunate location.
		*(volatile byte*)0xFFFF8203L = screenLO;
		irq_enable();
	#endif
#endif
	}
	else if (_shakePos != _shakePosPrev)
	{
		_shakePosPrev = _shakePos;
		_flipPending = true;
		byte* screenPtr = _screen - MUL160(_shakePos);
		Vsync();
		Setscreen(-1, screenPtr, -1);
	}
	enable_VBL();
}

// update mouse cursor, called from VBL handler
void OSystem_Atari::update_mouse()
{
	static short _mousePosLastX = -1;
	static short _mousePosLastY = -1;

	if (__aline)
	{
		short mx = CLAMP<short>(GCURX, 0, 319);
		short my = CLAMP<short>(GCURY, 0, 199);

		_mousePosLastX = _mousePosX;
		_mousePosLastY = _mousePosY;

		_mousePosX = mx;
		_mousePosY = my;

		if (_flipPending)
		{
			undraw_mouse();
			if (_mouseVisible)
				draw_mouse();
		}
		else
		{
			if (_mouseVisible)
			{
				if (!_mouseDrawn || (_mousePosX != _mousePosLastX) || (_mousePosX != _mousePosLastY))
				{
					undraw_mouse();
					draw_mouse();
				}
			}
			else
			{
				undraw_mouse();
			}
		}
	}
}

// Either show or hide the mouse cursor
bool OSystem_Atari::show_mouse(bool visible)
{
	if (_mouseVisible == visible)
		return visible;

	disable_VBL();
	bool last = _mouseVisible;
	_mouseVisible = visible;
	enable_VBL();
	return last;
}


void OSystem_Atari::draw_mouse()
{
	disable_VBL();
	if (!_mouseDrawn && _mouseVisible)
	{
		_cursor.Draw((byte*)_screen, (int16)_mousePosX, (int16)_mousePosY);
		_mouseDrawn = true;
	}
	enable_VBL();
}

void OSystem_Atari::undraw_mouse(bool restore)
{
	disable_VBL();
	if (_mouseDrawn)
	{
		if (restore)
			_cursor.Undraw();
		_mouseDrawn = false;
	}
	enable_VBL();
}

void OSystem_Atari::warp_mouse(int x, int y)
{
	//set_mouse_pos(x, y);	
}

// Set the bitmap that's used when drawing the cursor.
void OSystem_Atari::set_mouse_cursor(uint32 id, const byte *buf, const byte* mask, uint w, uint h, int hotspot_x, int hotspot_y)
{ 
#ifdef GAME_SAMNMAX
	disable_VBL();
	undraw_mouse();

	// id = ((room & 0xFF) << 24) | ((img & 0xFFFF) << 8) | ((imgindex & 0xF) << 4) | ((var & 1) << 1) | bomp
	// default cursor
	if ((id & 0xFF000000) == 0)
	{
		id = 0;
	}
	else
	{
		bool cacheable = (id != 0xFFFFFFFF);
		if (cacheable && _cursor.SetFrame(id))
		{
			draw_mouse();
			enable_VBL();			
			return;
		}
		// generate cursor data
		const bool bomp = (id & 1);
		const int16 variation = (id >> 1) & 1;
		const int16 tempIdx = (CURSOR_MAX_FRAMES - 2) + variation;

		bool success = bomp ? _cursor.CreateFromPlanar(-1, id, buf, mask, w, h, hotspot_x, hotspot_y, w) : false;
		if (!success && !_cursor.CreateFromPlanar(tempIdx, id, buf, mask, w, h, hotspot_x, hotspot_y, w))
			id = 0;
	}

	// and activate
	_cursor.SetFrame(id);
	draw_mouse();
	enable_VBL();
#endif // GAME_SAMNMAX
}

bool OSystem_Atari::mouse_cursor_cached(uint32 id)
{
	bool cacheable = (id != 0xFFFFFFFF);
	return cacheable && _cursor.HasFrame(id);
}


// Shaking is used in SCUMM. Set current shake position.
void OSystem_Atari::set_shake_pos(int shake_pos)
{ 
	// clamp shake position to min/max allowed.
	// make sure sahke position is in steps of 8 pixels due
	// to how the screen pointer must be 256 byte aligned.
	_shakePos = CLAMP(shake_pos, SCREEN_SHAKE_MIN, SCREEN_SHAKE_MAX);
	_shakePos &= ~7;
}

void OSystem_Atari::show_overlay()
{
	waitForPresent();
	disable_VBL();
	lock_mutex(&_audioMutex);
	MidiDriver_STCHIP_Mute(true);
	stopSound(true);
	unlock_mutex(&_audioMutex);

	const uint32* src = (uint32*)_screen;
	uint32* dst0 = (uint32*)_backbuffer;
	uint32* dst1 = (uint32*)_screenBackup;
	undraw_mouse();
	for(int i=0; i<SCREEN_WIDTH * SCREEN_HEIGHT / 128; ++i)
	{
		uint32 px;
		px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; 
		px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; 
		px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; 
		px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; 
	}
	draw_mouse();
	_prevFrameDirtyMask.clear();
	enable_VBL();
}

void OSystem_Atari::hide_overlay()
{
	waitForPresent();
	disable_VBL();
	MidiDriver_STCHIP_Mute(false);
	clear_overlay();
	enable_VBL();
}

void OSystem_Atari::clear_overlay()
{ 
	waitForPresent();
	disable_VBL();
	undraw_mouse(false);
	const uint32* src = (uint32*)_screenBackup;
	uint32* dst0 = (uint32*)_screen;
	uint32* dst1 = (uint32*)_backbuffer;
	for(int i=0; i<SCREEN_WIDTH * SCREEN_HEIGHT / 128; ++i)
	{
		uint32 px;
		px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; 
		px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; 
		px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; 
		px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; px = *src++; *dst0++ = px; *dst1++ = px; 
	}
	draw_mouse();
	_prevFrameDirtyMask.clear();
	enable_VBL();
}

void OSystem_Atari::grab_overlay(byte *buf, int pitch)
{ 
	const byte* src = (const byte*) _screenBackup;
	uint32* dst = (uint32*) buf;
	for (int y=0; y<SCREEN_HEIGHT; y++)
	{
		for (int x=0; x<SCREEN_WIDTH>>4; x++)
		{
			__asm__ volatile (
				"movep.l 0(%0),d1\n\t"
				"move.l	 d1,(%1)+\n\t"
				"movep.l 1(%0),d1\n\t"
				"move.l  d1,(%1)+\n\t"
				"lea     8(%0),%0\n\t"	
			: : "a"(src), "a"(dst) : "d1","memory" );
		}
	}
}


// ------------------------------------------------------------------------------------------------------
//
//	Misc
//
// ------------------------------------------------------------------------------------------------------

void OSystem_Atari::updateTimers()
{
	uint32 msec = get_msecs();
	if(_timerCallback && (msec >= _timerExpiry))
	{
		_timerDuration = _timerCallback(_timerDuration);
		_timerExpiry =  _timerDuration + msec;
	}
}
		
// Get the number of milliseconds since the program was started.
uint32 OSystem_Atari::get_msecs()
{
//	clock_t clocks = clock();
	#define _hz_200 ((unsigned long *) 0x4baL)
	clock_t clocks = *(volatile clock_t *) _hz_200;

	uint32 msec = (uint32) ((clocks * 1000) / 200);
	return msec;
}
	
// Delay for a specified amount of milliseconds
void OSystem_Atari::delay_msecs(uint msecs)
{ 
	uint32 target = get_msecs() + msecs;
	while(1)
	{
		uint32 now = get_msecs();
		if (now >= target)
			break;
	}
}
	
// Set the function to be invoked whenever samples need to be generated
bool OSystem_Atari::set_sound_proc(SoundProc proc, void *param, SoundFormat format)
{ 
	_soundProc = proc;
	_soundProcParam = param;
	return true;
}

void OSystem_Atari::clear_sound_proc()
{
	_soundProc = NULL;
	_soundProcParam = NULL;
}
	
// Get or set a property
uint32 OSystem_Atari::property(int param, Property *value)
{
	switch(param) {
		case PROP_GET_SAMPLE_RATE:
			return soundFreq(_soundPlayer, _soundQuality);
	}
	return 0;
}

// Add a new callback timer
void OSystem_Atari::set_timer(TimerProc callback, int timer)
{
	if (callback != NULL) {
		_timerDuration = timer;
		_timerExpiry = get_msecs() + _timerDuration;
		_timerCallback = callback;
	} else {
		_timerCallback = NULL;
	}
}

void OSystem_Atari::cleanup(bool panic)
{
	// no interruptions please
	irq_disable();
	// remove exception handler
	removeExceptionHandlers();
	// remove VBL handler
	irq_remove_vbl(OSystem_Atari::VBL);
	// kill sound
	if (panic)
	{
		// kill timer
		irq_enable_timerA(false);
		// kill YM
		YM2149_WR_MASKED(7, 0xFF, 0x3F);
		// kill DMA
		if (_this->_soundPlayer == kSoundPlayer_DMA)
			*((volatile uint8*)0x00FF8901) = 0;
	}
	else
	{
		initSound(kSoundPlayer_Off, 0);
	}
	// restore video + mouse
	if (_oldLogBase != 0)
	{
		Setscreen(_oldLogBase, _oldPhysBase, _oldRez);
		Setpalette(_oldPalette);
		Linea9();
	}
	irq_enable();
}

void OSystem_Atari::quit()
{ 
	cleanup(false);
	exit(0);
}

// Mutex handling

OSystem::MutexRef OSystem_Atari::create_mutex() {
	return &_audioMutex;
}

void OSystem_Atari::delete_mutex(MutexRef mutex) {
}

bool OSystem_Atari::try_lock_mutex(MutexRef mutex) {
	Mutex* m = (Mutex*) mutex;	
	if (m->lock > 0)
		return false;
	m->lock++;
	return true;
}


OSystem *OSystem_Atari::create()
{
	OSystem_Atari *syst = new OSystem_Atari();
	return syst;
}

OSystem *OSystem_Atari_create()
{ 
	return OSystem_Atari::create();
}



// ------------------------------------------------------------------------------------------------------
//
//	Input and events
//
// ------------------------------------------------------------------------------------------------------

extern void SCUMM_Goto_Room(int room);

bool OSystem_Atari::poll_event(Event *event)
{ 
	if(_crashed) {
		// we should't really get here, but if we do, just quit.
		exit(0);
	}

	_frameCount++;

	// check keyboard and mouse
	static byte ms_last = 0;
	static short mx_last = 0;
	static short my_last = 0;

	short mx = 160;
	short my = 100;
	byte ms = 0;
	if (__aline)
	{
		disable_VBL();
		ms = (CUR_MS_STAT) & 3;
		mx = GCURX;
		my = GCURY;
		enable_VBL();
		if (mx < 0)		mx = 0;
		if (mx > 319)	mx = 319;
		if (my < 0)		my = 0;
		if (my > 199)	my = 199;
	}

	short key_ascii = 0;
	if (Cconis() < 0)
	{
		uint32 v = Cconin();
		key_ascii = (v & 0xFF);

		// function keys
		short key_code = (v >> 16) & 0xFF;
		if (key_code >= 0x3b && key_code <= 0x44)
			key_ascii = 315 + (key_code - 0x3b);
	}

	if (key_ascii != 0)
	{
		event->event_code = EVENT_KEYDOWN;
		event->kbd.flags = 0;
		event->kbd.keycode = key_ascii;
		event->kbd.ascii = key_ascii;
		return true;
	}

	if ((ms & 3) != (ms_last & 3))
	{
/*
		if ((ms & 3) == 3)
		{
			#if 0
			if (keyhack == 10)
			{
				SCUMM_Goto_Room(53);
			}
			else if (keyhack == 11)
			{
				SCUMM_Goto_Room(48);
			}
			else
			#endif
			if ((keyhack & 1) == 0)
			{
				event->event_code = EVENT_KEYDOWN;
				event->kbd.flags = 0;
				event->kbd.keycode = 0;
				event->kbd.ascii = 27;
			}
			else
			{
				event->event_code = EVENT_KEYDOWN;
				event->kbd.flags = 0;
				event->kbd.keycode = '1';
				event->kbd.ascii = '1';
			}
			keyhack++;

		}
		else*/
		if ((ms & 1) != (ms_last & 1))
		{
			ms_last = ms;
			event->event_code = (ms & 1) ? EVENT_LBUTTONDOWN : EVENT_LBUTTONUP;
			event->mouse.x = mx;
			event->mouse.y = my;
		}
		else if ((ms & 2) != (ms_last & 2))
		{
			event->event_code = (ms & 2) ? EVENT_RBUTTONDOWN : EVENT_RBUTTONUP;
			event->mouse.x = mx;
			event->mouse.y = my;
		}

		ms_last = ms;
		mx_last = mx;
		my_last = my;
		return true;
	}

	else if ((mx != mx_last) || (my != my_last))
	{
		event->event_code = EVENT_MOUSEMOVE;
		event->mouse.x = mx;
		event->mouse.y = my;	
		mx_last = mx;
		my_last = my;
		//set_mouse_pos(mx, my);
		return true;
	}

	event->event_code = (EventCode) 0;
	return false;
}


