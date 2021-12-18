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
 * $Header: /cvsroot/scummvm/scummvm/base/engine.cpp,v 1.14.2.1 2004/03/04 21:13:38 arisme Exp $
 */

#include "stdafx.h"
#if defined(_MSC_VER)
#include <malloc.h>
#endif
#include "common/engine.h"
#include "common/gameDetector.h"
#include "common/config-manager.h"
#include "common/file.h"
#include "common/timer.h"
#include "sound/mixer.h"

/* FIXME - BIG HACK for MidiEmu */
Engine *g_engine = 0;

bool g_slowMachine = true;
uint16 g_debugLevel = 0;

Engine::Engine(OSystem *syst)
	: _system(syst), _gameDataPath(_conf_gamepath) {
	g_engine = this;
	_mixer = GameDetector::createMixer();

	_timer = g_timer;

	// Set default file directory
	File::setDefaultDirectory(_gameDataPath);

	g_debugLevel = GetConfig(kConfig_DebugLevel);
}

Engine::~Engine() {
	delete _mixer;
	delete _timer;
}

const char *Engine::getSavePath() const {
	return _gameDataPath.c_str();
}

const char *Engine::getGameDataPath() const {
	return _gameDataPath.c_str();
}

#ifndef RELEASEBUILD

static char buf[1024];

void NORETURN CDECL error(const char *s, ...) {
	va_list va;
	va_start(va, s);
	vsprintf(buf, s, va);
	va_end(va);
/*
	if (g_engine) {
		g_engine->errorString(buf_input, buf_output);
	} else {
		strcpy(buf_output, buf_input);
	}
*/
	printf("ERROR: %s\n", buf);
	fflush(0);

	// Finally exit. quit() will terminate the program if g_system iss present
	if (g_system)
		g_system->quit();

	exit(1);
}

void CDECL warning(const char *s, ...) {
	va_list va;
	va_start(va, s);
	vsprintf(buf, s, va);
	va_end(va);
	printf("WARNING: %s\n\r", buf);
	fflush(0);
}

void CDECL debug(int level, const char *s, ...) {
	if (level > g_debugLevel)
		return;
	va_list va;
	va_start(va, s);
	vsprintf(buf, s, va);
	va_end(va);
	printf("%s\n\r", buf);
	fflush(0);
}

void CDECL debug(const char *s, ...) {
	va_list va;
	va_start(va, s);
	vsprintf(buf, s, va);
	va_end(va);
	printf("%s\n\r", buf);
	fflush(0);
}


void checkHeap() {
}

#endif

