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
 * $Header: /cvsroot/scummvm/scummvm/scumm/vars.cpp,v 1.71 2004/02/11 15:59:25 kirben Exp $
 *
 */


#include "stdafx.h"
#include "scumm/scumm.h"
#include "scumm/intern.h"

namespace Scumm {

void ScummEngine::setupScummVars() {
	VAR_KEYPRESS = 0;
	VAR_EGO = 1;
	VAR_CAMERA_POS_X = 2;
	VAR_HAVE_MSG = 3;
	VAR_ROOM = 4;
	VAR_OVERRIDE = 5;
	VAR_MACHINE_SPEED = 6;
	VAR_ME = 7;
	VAR_NUM_ACTOR = 8;
	VAR_CURRENTDRIVE = 10;
	VAR_TMR_1 = 11;
	VAR_TMR_2 = 12;
	VAR_TMR_3 = 13;
	VAR_MUSIC_TIMER = 14;
	VAR_ACTOR_RANGE_MIN = 15;
	VAR_ACTOR_RANGE_MAX = 16;
	VAR_CAMERA_MIN_X = 17;
	VAR_CAMERA_MAX_X = 18;
	VAR_TIMER_NEXT = 19;
	VAR_VIRT_MOUSE_X = 20;
	VAR_VIRT_MOUSE_Y = 21;
	VAR_ROOM_RESOURCE = 22;
	VAR_LAST_SOUND = 23;
	VAR_CUTSCENEEXIT_KEY = 24;
	VAR_TALK_ACTOR = 25;
	VAR_CAMERA_FAST_X = 26;
	VAR_SCROLL_SCRIPT = 27;
	VAR_ENTRY_SCRIPT = 28;
	VAR_ENTRY_SCRIPT2 = 29;
	VAR_EXIT_SCRIPT = 30;
	VAR_EXIT_SCRIPT2 = 31;
	VAR_VERB_SCRIPT = 32;
	VAR_SENTENCE_SCRIPT = 33;
	VAR_INVENTORY_SCRIPT = 34;
	VAR_CUTSCENE_START_SCRIPT = 35;
	VAR_CUTSCENE_END_SCRIPT = 36;
	VAR_CHARINC = 37;
	VAR_WALKTO_OBJ = 38;
	VAR_DEBUGMODE = 39;
	VAR_HEAPSPACE = 40;
	VAR_RESTART_KEY = 42;
	VAR_PAUSE_KEY = 43;
	VAR_MOUSE_X = 44;
	VAR_MOUSE_Y = 45;
	VAR_TIMER = 46;
	VAR_TMR_4 = 47;
	VAR_SOUNDCARD = 48;
	VAR_VIDEOMODE = 49;
	VAR_MAINMENU_KEY = 50;
	VAR_FIXEDDISK = 51;
	VAR_CURSORSTATE = 52;
	VAR_USERPUT = 53;
	VAR_SOUNDRESULT = 56;
	VAR_TALKSTOP_KEY = 57;
	VAR_NOSUBTITLES = 60; // for loomcd

	VAR_SOUNDPARAM = 64;
	VAR_SOUNDPARAM2 = 65;
	VAR_SOUNDPARAM3 = 66;
	VAR_MOUSEPRESENT = 67;
	VAR_PERFORMANCE_1 = 68;
	VAR_PERFORMANCE_2 = 69;
	VAR_ROOM_FLAG = 70;
	VAR_GAME_LOADED = 71;
	VAR_NEW_ROOM = 72;

	VAR_VERSION = 75;
}

#ifdef ENGINE_SCUMM5
void ScummEngine_v5::setupScummVars() {
	// Many vars are the same as in V5 & V6 games, so just call the inherited method first
	ScummEngine::setupScummVars();

	VAR_CURRENT_LIGHTS = 9;
	VAR_V5_TALK_STRING_Y = 54;
}
#endif //ENGINE_SCUMM5


#ifdef ENGINE_SCUMM6
void ScummEngine_v6::setupScummVars() {
	// Many vars are the same as in V5 & V6 games, so just call the inherited method first
	ScummEngine::setupScummVars();

	VAR_V6_SCREEN_WIDTH = 41;
	VAR_V6_SCREEN_HEIGHT = 54;
	VAR_V6_EMSSPACE = 76;
	VAR_RANDOM_NR = 118;
	
	VAR_V6_SOUNDMODE = 9;

	VAR_TIMEDATE_YEAR = 119;
	VAR_TIMEDATE_MONTH = 129;
	VAR_TIMEDATE_DAY = 128;
	VAR_TIMEDATE_HOUR = 125;
	VAR_TIMEDATE_MINUTE = 126;

	VAR_SCREENSAVER_TIME = 132;	// samnmax
}
#endif //ENGINE_SCUMM6


} // End of namespace Scumm
