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


#include "common/stdafx.h"
#include "common/scummsys.h"
#include "common/system.h"
#include "common/engine.h"
#include "common/util.h"
#include "common/dirty.h"
#include "common/gui/options.h"
#include "backends/intern.h"
#include "portdefs.h"

#define ATARI_SNDFREQ_YM		
#define ATARI_SNDFREQ_DMA

#define MCH_IS_STE			((_mch & 0xFFFF0000) == 0x00010000)
#define MCH_IS_MEGA_STE		(_mch == 0x00010010)
#define MCH_IS_TT			((_mch & 0xFFFF0000) == 0x00020000)
#define MCH_IS_FALCON		((_mch & 0xFFFF0000) == 0x00030000)


class OSystem_Atari : public OSystem {
public:

	bool init();
	void cleanup(bool panic = false);

	void installStart();
	void installDone();
	void installUpdate(byte progCur, byte progMax);

	void setIsLoading(bool isloading);
	
	// Set colors of the palette
	void set_palette(const byte *colors, uint start, uint num);

	// Set the size of the video bitmap.
	// Typically, 320x200
	void init_size(uint w, uint h);
	int16 get_height() { return SCREEN_HEIGHT; }
	int16 get_width() { return SCREEN_WIDTH; }
	
	// Draw a bitmap to screen.
	void copy_rect(const byte *buf, int pitch, int x, int y, int w, int h);
	void copy_screen(const byte *buf, int y, int h, const uint32* mask, bool doublebuffer);

	// Moves the screen content around by the given amount of pixels
	// but only the top height pixel rows, the rest stays untouched
	void move_screen(int dx, int dy, int height);

	// Update the dirty areas of the screen
	void update_screen();

	// Either show or hide the mouse cursor
	bool show_mouse(bool visible);
	void warp_mouse(int x, int y);
	
	// Set the bitmap that's used when drawing the cursor.
	void set_mouse_cursor(uint32 id, const byte *buf, const byte* mask, uint w, uint h, int hotspot_x, int hotspot_y);
	bool mouse_cursor_cached(uint32 id);


	// Shaking is used in SCUMM. Set current shake position.
	void set_shake_pos(int shake_pos);
		
	// Get the number of milliseconds since the program was started.
	uint32 get_msecs();
	
	// Delay for a specified amount of milliseconds
	void delay_msecs(uint msecs);
	
	// Get the next event.
	// Returns true if an event was retrieved.	
	bool poll_event(Event *event);


	
	// Get or set a property
	uint32 property(int param, Property *value);
		
	// Add a new callback timer
	void set_timer(TimerProc callback, int timer);

	// Mutex handling (not really, but we use it to prevent
	// VBL from updating audio while scummvm does audio
	struct Mutex {
		Mutex() { lock = 0; }
		volatile uint16 lock;
	};

	OSystem::MutexRef create_mutex();
	bool try_lock_mutex(MutexRef mutex);
	void lock_mutex(MutexRef mutex) 	{ ((Mutex*)mutex)->lock++; }
	void unlock_mutex(MutexRef mutex)	{ ((Mutex*)mutex)->lock--; }
	void delete_mutex(MutexRef mutex);

	// Quit
	void quit();
	
	// Overlay
	void show_overlay();
	void hide_overlay();
	void clear_overlay();
	void grab_overlay(byte *buf, int pitch);
	static OSystem *create();


	bool set_sound_proc(SoundProc proc, void *param, SoundFormat format);
	void clear_sound_proc();

	bool poll_cdrom();
	void play_cdrom(int track, int num_loops, int start_frame, int duration);
	void stop_cdrom();
	void update_cdrom();

	enum SoundPlayer
	{
		kSoundPlayer_Auto = 0,
		kSoundPlayer_Off,
		kSoundPlayer_YM,
		kSoundPlayer_DMA,
		kSoundPlayer_Covox,
		kSoundPlayer_MV16,			//12bit mono		(MV16, Replay Pro?)
		kSoundPlayer_Replay8,		// 8bit mono		(ST Replay 8)
		kSoundPlayer_Replay16,		//16bit mono		(ST Replay 16)
		kSoundPlayer_Replay8S,		// 8bit stereo		(Stereo Replay, Playback, etc..)
	};

	struct DriverCfg
	{
		const char* name;
		const int id;
	};

	const DriverCfg*	musicDrivers[16];
	const DriverCfg*	soundDrivers[16];

private:
	friend class GUI::OptionsDialog;
	friend class MidiDriver_STCHIP;

	static void VBL();
	static void TerminationHandler();
	static int ExceptionHandler(int no);
	void installExceptionHandlers();
	void removeExceptionHandlers();

	void setMidiUpdateFunc(void (*func)()) { _midiCallback = func; }
	uint32 get_cookie(uint32 cookie);

	void update_mouse();

	void draw_mouse();
	void undraw_mouse(bool restore = true);

	void waitForPresent();
	void updateTimers();

	void drawConsoleScreen();

	void initGfx();
	void initSound(SoundPlayer player, byte quality);
	void stopSound(bool forced);
	void updateSound();
	void updateSound_DMA();
	void updateSound_CPU();
	uint32 soundInactiveTime();

	uint32 soundFreq(SoundPlayer player, byte quality);
	NewGuiColor RGBToColor(uint8 r, uint8 g, uint8 b);

	uint32		_mch;		// machine
	uint32		_vdo;		// video hw
	uint32		_snd;		// sound hw
	uint32		_cpu;		// cpu

	void		(*_midiCallback) (void);
	int 		(*_timerCallback) (int);
	uint32		_timerDuration;
	uint32		_timerExpiry;
	bool		_soundActive;
	int			_isLoading;
	uint32		_soundQuietDuration;
	SoundProc	_soundProc;
	void*		_soundProcParam;
	byte*		_mouseBufSrc;
	byte*		_mouseBuf;
	byte*		_mouseBak;
	short		_mouseMask[16];
	volatile bool		_mouseDrawn;
	volatile bool		_mouseVisible;
	volatile short		_mousePosX;
	volatile short		_mousePosY;
	volatile int		_shakePos;
	volatile int		_shakePosPrev;
	volatile bool		_flipPending;

	SoundPlayer	_soundPlayer;
	byte		_soundQuality;

	uint32		_frameCount;

	Common::DirtyMask _prevFrameDirtyMask;

	int16		_dirtyTop;
	int16		_dirtyBottom;

	byte*		_screen;			// physical screen
	byte*		_backbuffer;		// backbuffer
	byte*		_screenBackup;		// screen backup for overlay
	void* 		_oldLogBase;
	void* 		_oldPhysBase;
	int16		_oldRez;
	int16		_oldPalette[16];
	bool		_crashed;

	OSystem_Atari();
};
