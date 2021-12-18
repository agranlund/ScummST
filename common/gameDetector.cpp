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
 * $Header: /cvsroot/scummvm/scummvm/base/gameDetector.cpp,v 1.69.2.1 2004/03/08 02:42:37 kirben Exp $
 *
 */

#include "stdafx.h"

#include "common/engine.h"
#include "common/gameDetector.h"
#include "common/version.h"
#include "common/util.h"

#include "common/config-manager.h"
#include "common/scaler.h"	// Only for gfx_modes

#include "sound/mididrv.h"
#include "sound/mixer.h"

extern bool g_slowMachine;

#if defined(ENGINE_SCUMM5) || defined(ENGINE_SCUMM6)
#include "scumm/scumm.h"
extern Engine *Engine_SCUMM_create(GameDetector *detector, OSystem *syst);
#endif

GameDetector::GameDetector() {
#ifdef SINGLEGAME
#if defined(GAME_MONKEY1)
	_targetName = "monkey";
#elif defined(GAME_MONKEY2)
	_targetName = "monkey2";
#elif defined(GAME_ATLANTIS)
	_targetName = "atlantis";
#elif defined(GAME_TENTACLE)
	_targetName = "tentacle";
#elif defined(GAME_SAMNMAX)
	_targetName = "samnmax";
#elif defined(GAME_RESOURCECONVERTER)
	_targetName = "resourceconverter";
#elif defined(GAME_SIERRA)
	_targetName = "sierra";
#else
	#error "Built as single game exe but no game specified"
#endif
	_conf_filename = _targetName;
	_conf_filename += ".inf";
#endif
	LoadConfig();
	memset(&_game, 0, sizeof(_game));
}


// DONT FIXME: DO NOT ORDER ALPHABETICALLY, THIS IS ORDERED BY IMPORTANCE/CATEGORY! :)
#ifndef __ATARI__
static const char USAGE_STRING[] = 
	"Usage: [OPTIONS]... [GAME]\n"
	"  -p   Path to where the game is installed\n"
	"  -d   Set debug verbosity level\n"
	"\n"
;
#endif

//
// Various macros used by the command line parser.
//

#define DO_OPTION_OPT(shortCmd) \
	if ((shortCmdLower == shortCmd)) { \
		if ((*s != '\0') && (current_option != NULL)) goto ShowHelpAndExit; \
		option = (*s != '\0') ? s : current_option; \
		current_option = NULL;

#define DO_OPTION(shortCmd) \
	DO_OPTION_OPT(shortCmd) \
	if (option == NULL) goto ShowHelpAndExit;

#define DO_OPTION_BOOL(shortCmd) \
	if ((shortCmdLower == shortCmd)) { \
		if ((*s != '\0') || (current_option != NULL)) goto ShowHelpAndExit;

#define DO_OPTION_CMD(shortCmd) \
	if ((shortCmdLower == shortCmd)) { \
		if ((*s != '\0') || (current_option != NULL)) goto ShowHelpAndExit;


// End an option handler
#define END_OPTION \
		continue; \
	}


void GameDetector::parseCommandLine(int argc, char **argv) {

#ifndef __ATARI__
	int i;
	char *s;
	char *current_option = NULL;
	char *option = NULL;
	char shortCmdLower;
	bool isLongCmd, cmdValue;

	// Iterate over all command line arguments, backwards.
	for (i = argc - 1; i >= 1; i--) {
		s = argv[i];

		if (s[0] != '-' || s[1] == '\0') {
			if (i == (argc - 1) && findGame(s).name) {
				_targetName = s;
			} else {
				if (current_option == NULL)
					current_option = s;
				else
					goto ShowHelpAndExit;
			}
		} else {

			shortCmdLower = tolower(s[1]);
			isLongCmd = (s[0] == '-' && s[1] == '-');
			cmdValue = (shortCmdLower == s[1]);
			s += 2;

			DO_OPTION_OPT('s')
				if (option != NULL)
					g_slowMachine = (bool)atoi(option);
			END_OPTION

			DO_OPTION_OPT('d')
				if (option != NULL)
					SetConfig(kConfig_DebugLevel, (byte)atoi(option));
			END_OPTION
			
			DO_OPTION_OPT('e')				
				if (option != NULL)
					SetConfig(kConfig_MusicDriver, (byte)atoi(option));
			END_OPTION

			DO_OPTION('p')
				_conf_gamepath = option;
			END_OPTION

			DO_OPTION_CMD('v')
				printf("%s\n", gScummVMFullVersion);
				exit(0);
			END_OPTION

			goto ShowHelpAndExit;
		}
	}

	if (current_option) {
ShowHelpAndExit:
		printf(USAGE_STRING);
		exit(1);
	}
#endif
}



GameSettings GameDetector::findGame(const String &gameName) {
	GameSettings result = {NULL, NULL, 0};
#if defined(ENGINE_SCUMM5) || defined(ENGINE_SCUMM6)
	const Scumm::ScummGameSettings* setting = Scumm::scumm_settings;
	while (setting->name != NULL)
	{
		if (gameName == setting->name)
		{
			result.name = setting->name;
			result.description = setting->description;
			result.features = setting->features;
			break;
		}
		setting++;
	}
#endif
	return result;
}


bool GameDetector::detectGame() {
	String realGame;

	realGame = _targetName;

	_game = findGame(realGame);

	if (_game.name) {
		return true;
	} else {
		return false;
	}
}

int GameDetector::detectMusicDriver(int midiFlags) {
	int musicDriver = (int) GetConfig(kConfig_MusicDriver);

	if (musicDriver == MD_AUTO || musicDriver < 0)
	{
		#ifdef __ATARI__
			//musicDriver = MD_STCHIP;
			//musicDriver = MD_STMIDI_GM;
			musicDriver = MD_STMIDI_MT32;
			//musicDriver = MD_NULL;
		#else
			musicDriver = MD_ADLIB;
		#endif
	}

	bool nativeMidiDriver =	(musicDriver != MD_NULL && musicDriver != MD_ADLIB);

	if (nativeMidiDriver && !(midiFlags & MDT_NATIVE))
		musicDriver = MD_ADLIB;

#ifdef DISABLE_ADLIB
	if (musicDriver == MD_ADLIB)
		musicDriver = MD_NULL;
#endif

	return musicDriver;
}

bool GameDetector::detectMain() {
	if (_targetName.isEmpty()) {
		warning("No game was specified...");
		return false;
	}

	if (!detectGame()) {
		warning("%s is an invalid target. Use the --list-targets option to list targets", _targetName.c_str());
		return false;
	}
	return true;
}


Engine *GameDetector::createEngine(OSystem *sys) {
	debug(1, "createEngine");
#if defined(ENGINE_SCUMM5) || defined(ENGINE_SCUMM6)
	return Engine_SCUMM_create(this, sys);
#else
	#error Unknown engine
	return 0;
#endif
}

SoundMixer *GameDetector::createMixer() {
	debug(1, "createMixer");
	return new SoundMixer();
}

MidiDriver *GameDetector::createMidi(int midiDriver) {
	debug(1, "CreateMidi: %d", midiDriver);
	switch(midiDriver) {
	case MD_NULL:
		return MidiDriver_NULL_create();

#ifdef __ATARI__
	case MD_STMIDI_GM:
	case MD_STMIDI_MT32:
		return MidiDriver_STMIDI_create();

	case MD_STCHIP:
		return MidiDriver_STCHIP_create();
#endif

	// In the case of Adlib, we won't specify anything.
	// IMuse is designed to set up its own Adlib driver
	// if need be, and we only have to specify a native
	// driver.
	case MD_ADLIB:
	default:
		return NULL;
	}

	error("Invalid midi driver selected");
	return NULL;
}
