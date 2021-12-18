/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
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
 * $Header: /cvsroot/scummvm/scummvm/base/main.cpp,v 1.35.2.2 2004/03/14 13:45:12 aquadran Exp $
 *
 */

/*! \mainpage %ScummVM Source Reference
 *
 * These pages contains a cross referenced documentation for the %ScummVM source code,
 * generated with Doxygen (http://www.doxygen.org) directly from the source.
 * Currently not much is actually properly documented, but at least you can get an overview
 * of almost all the classes, methods and variables, and how they interact.
 */
 
#include "stdafx.h"
#include "common/engine.h"
#include "common/gameDetector.h"
#include "common/version.h"
#include "common/config-manager.h"
#include "common/scaler.h"	// For GFX_NORMAL
#include "common/timer.h"
#include "common/util.h"
#include "gui/newgui.h"
#include "gui/message.h"

/*
 * Version string and build date string. These can be used by anything that
 * wants to display this information to the user (e.g. about dialog).
 *
 * Note: it would be very nice if we could instead of (or in addition to) the
 * build date present a date which corresponds to the date our source files
 * were last changed. To understand the difference, imagine that a user
 * makes a checkout of CVS on January 1, then after a week compiles it
 * (e.g. after doing a 'make clean'). The build date then will say January 8
 * even though the files were last changed on January 1.
 *
 * Another problem is that __DATE__/__TIME__ depend on the local time zone.
 *
 * It's clear that such a "last changed" date would be much more useful to us
 * for feedback purposes. After all, when somebody files a bug report, we
 * don't care about the build date, we want to know which date their checkout
 * was made. This is even more important now since anon CVS lags a few
 * days behind developer CVS.
 *
 * So, how could we implement this? At least on unix systems, a special script
 * could do it. Basically, that script would run over all .cpp/.h files and
 * parse the CVS 'Header' keyword we have in our file headers.
 * That line contains a date/time in GMT. Now, the script just has to collect
 * all these times and find the latest. This time then would be inserted into
 * a header file or so (common/date.h ?) which engine.cpp then could
 * include and put into a global variable analog to gScummVMBuildDate.
 *
 * Drawback: scanning all source/header files will be rather slow. Also, this
 * only works on systems which can run powerful enough scripts (so I guess
 * Visual C++ would be out of the game here? don't know VC enough to be sure).
 *
 * Another approach would be to somehow get CVS to update a global file
 * (e.g. LAST_CHANGED) whenever any checkins are made. That would be
 * faster and work w/o much "logic" on the client side, in particular no
 * scripts have to be run. The problem with this is that I am not even
 * sure it's actually possible! Modifying files during commit time is trivial
 * to setup, but I have no idea if/how one can also change files which are not
 * currently being commit'ed.
 */
const char *gScummVMVersion = "0.6.0";
const char *gScummVMBuildDate = __DATE__ " " __TIME__;
const char *gScummVMFullVersion = "ScummVM 0.6.0 (" __DATE__ " " __TIME__ ")";

Common::String gGameName;

#undef main


#if defined(UNIX)
#include <signal.h>
#ifndef SCUMM_NEED_ALIGNMENT
static void handle_errors(int sig_num) {
	error("Your system does not support unaligned memory accesses. Please rebuild with SCUMM_NEED_ALIGNMENT (signal %d)", sig_num);
}
#endif


// This function is here to test if the endianness / alignement compiled it is matching with the one at run-time.
static void do_memory_test(void) {
	unsigned char test[8] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
	unsigned int value;
	/* First test endianness */
#ifdef SCUMM_LITTLE_ENDIAN
	if (*((int *) test) != 0x44332211) {
		error("Compiled as LITTLE_ENDIAN on a big endian system. Please rebuild ");
	}
	value = 0x55443322;
#else
	if (*((int *) test) != 0x11223344) {
		error("Compiled as BIG_ENDIAN on a little endian system. Please rebuild ");
	}
	value = 0x22334455;
#endif
	/* Then check if one really supports unaligned memory accesses */
#ifndef SCUMM_NEED_ALIGNMENT
	signal(SIGBUS, handle_errors);
	signal(SIGABRT, handle_errors);
	signal(SIGSEGV, handle_errors);
	if (*((unsigned int *) ((char *) test + 1)) != value) {
		error("Your system does not support unaligned memory accesses. Please rebuild with SCUMM_NEED_ALIGNMENT ");
	}
	signal(SIGBUS, SIG_DFL);
	signal(SIGABRT, SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
#endif
}
#endif // UNIX


static void runGame(GameDetector &detector, OSystem *system) {

	OSystem::Property prop;

	// Set the window caption to the game name

#ifndef __ATARI__
	if (!gGameName.isEmpty())	{
		prop.caption = gGameName.c_str();
		system->property(OSystem::PROP_SET_WINDOW_CAPTION, &prop);
	}
#endif

	// Create the game engine
	Engine *engine = detector.createEngine(system);
	assert(engine);

	// Run the game engine
	engine->go();

	// Stop all sound processing now (this prevents some race conditions later on)
	system->clear_sound_proc();

	// Free up memory
	delete engine;
};

static int _argc;
static char** _argv;


int super_main() {
	OSystem::Property prop;

#if defined(UNIX)
	/* On Unix, do a quick endian / alignement check before starting */
	do_memory_test();
#endif

	// Load the plugins
	//PluginManager::instance().loadPlugins();

	// Init utils
	Common::Util::Init();

	// Parse the command line information
	GameDetector detector;
	detector.parseCommandLine(_argc, _argv);

	// Create the system object
	OSystem *system = OSystem::instance();

	// Create the timer services
	g_timer = new Timer(system);

	// Set initial window caption
	prop.caption = gScummVMFullVersion;
	system->property(OSystem::PROP_SET_WINDOW_CAPTION, &prop);

	if (detector.detectMain())
	{
		if (gGameName.isEmpty() && detector._game.description)
			gGameName = detector._game.description;
		if (gGameName.isEmpty())	
			gGameName = detector._targetName;
		if (gGameName.isEmpty())
			gGameName = "ScummVM";

		if (system->init())
		{
			runGame(detector, system);
		}
	}

	// ...and quit (the return 0 should never be reached)
	system->quit();
	delete system;
	return 0;
}


#ifdef __ATARI__

#define S_SUPER_SSP(_sz_) \
static const int ssp_size = _sz_>>2; \
uint32 s_new_ssp[ssp_size]; \
uint32 s_old_ssp; \
uint32 s_old_usp;

S_SUPER_SSP(1024 * 8)

void start_super()             \
{                                    \
   __asm__ __volatile__                  \
   (                                 \
      "                              \
      move.l      %0,%%a0;               \
      move.l      %%a0,%%d0;               \
      subq.l      #4,%%d0;               \
      and.w      #-16,%%d0;               \
      move.l      %%d0,%%a0;               \
      move.l      %%sp,-(%%a0);            \
      move.l      %%usp,%%a1;               \
      move.l      %%a1,-(%%a0);            \
      move.l      %%a0,%%sp;               \
      movem.l      %%d1-%%d7/%%a2-%%a6,-(%%sp);   \
      jsr         (%1);                     \
      movem.l      (%%sp)+,%%d1-%%d7/%%a2-%%a6;   \
      move.l      (%%sp)+,%%a0;            \
      move.l      %%a0,%%usp;               \
      move.l      (%%sp)+,%%sp;            \
      "                              \
      :                              \
      : "p"(&s_new_ssp[ssp_size-16]), "a"(&super_main) \
      : "%%d0", "%%a0", "%%a1", "cc"         \
   );                                 \
} 


int main(int argc, char* argv[])
{
	_argc = argc;
	_argv = argv;
	Supexec(&start_super);
	return 0;
}


#else

int main(int argc, char* argv[])
{
	_argc = argc;
	_argv = argv;
	return super_main();
}

#endif

