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
 * $Header: /cvsroot/scummvm/scummvm/scumm/debugger.cpp,v 1.120 2004/02/13 10:26:40 ender Exp $
 *
 */
#include "stdafx.h"

#ifndef DISABLE_DEBUGGER

#include "common/file.h"
#include "common/str.h"
#include "common/util.h"

#include "scumm/actor.h"
#include "scumm/boxes.h"
#include "scumm/debugger.h"
#include "scumm/imuse.h"
#include "scumm/object.h"
#include "scumm/scumm.h"
#include "scumm/sound.h"

extern uint16 g_debugLevel;

namespace Scumm {
	
ScummDebugger::ScummDebugger(ScummEngine *s)
	: Common::Debugger<ScummDebugger>() {
	_vm = s;

	// Register variables
	DVar_Register("debug_countdown", &_frame_countdown, DVAR_INT, 0);

	DVar_Register("scumm_speed", &_vm->_fastMode, DVAR_INT, 0);
	DVar_Register("scumm_room", &_vm->_currentRoom, DVAR_INT, 0);
	DVar_Register("scumm_roomresource", &_vm->_roomResource, DVAR_INT, 0);
	DVar_Register("scumm_vars", &_vm->_scummVars, DVAR_INTARRAY, _vm->_numVariables);

	DVar_Register("scumm_gamename", &_vm->_targetName, DVAR_STRING, 0);
	DVar_Register("scumm_exename", &_vm->_gameName, DVAR_STRING, 0);
	DVar_Register("scumm_gameid", &_vm->_gameId, DVAR_INT, 0);

	// Register commands
	DCmd_Register("continue", &ScummDebugger::Cmd_Exit);
	DCmd_Register("exit", &ScummDebugger::Cmd_Exit);
	DCmd_Register("quit", &ScummDebugger::Cmd_Exit);
	DCmd_Register("restart", &ScummDebugger::Cmd_Restart);

	DCmd_Register("actor", &ScummDebugger::Cmd_Actor);
	DCmd_Register("actors", &ScummDebugger::Cmd_PrintActor);
	DCmd_Register("box", &ScummDebugger::Cmd_PrintBox);
	DCmd_Register("matrix", &ScummDebugger::Cmd_PrintBoxMatrix);
	DCmd_Register("room", &ScummDebugger::Cmd_Room);
	DCmd_Register("objects", &ScummDebugger::Cmd_PrintObjects);
	DCmd_Register("object", &ScummDebugger::Cmd_Object);
	DCmd_Register("script", &ScummDebugger::Cmd_Script);
	DCmd_Register("scr", &ScummDebugger::Cmd_Script);
	DCmd_Register("scripts", &ScummDebugger::Cmd_PrintScript);
	DCmd_Register("importres", &ScummDebugger::Cmd_ImportRes);

	DCmd_Register("loadgame", &ScummDebugger::Cmd_LoadGame);
	DCmd_Register("savegame", &ScummDebugger::Cmd_SaveGame);

	DCmd_Register("level", &ScummDebugger::Cmd_DebugLevel);
	DCmd_Register("debug", &ScummDebugger::Cmd_Debug);
	DCmd_Register("help", &ScummDebugger::Cmd_Help);

	DCmd_Register("show", &ScummDebugger::Cmd_Show);
	DCmd_Register("hide", &ScummDebugger::Cmd_Hide);

	DCmd_Register("imuse", &ScummDebugger::Cmd_IMuse);
}

void ScummDebugger::preEnter() {
	// Pause sound output
	_old_soundsPaused = _vm->_sound->_soundsPaused;
	_vm->_sound->pauseSounds(true);
}

void ScummDebugger::postEnter() {
	// Resume previous sound state
	_vm->_sound->pauseSounds(_old_soundsPaused);
}

///////////////////////////////////////////////////
// Now the fun stuff:

// Commands
bool ScummDebugger::Cmd_Exit(int argc, const char **argv) {
	_detach_now = true;
	return false;
}

bool ScummDebugger::Cmd_Restart(int argc, const char **argv) {
	_vm->restart();

	_detach_now = true;
	return false;
}

bool ScummDebugger::Cmd_IMuse(int argc, const char **argv) {
	if (!_vm->_imuse && !_vm->_musicEngine) {
		DebugPrintf("No iMuse engine is active.\n");
		return true;
	}

	if (argc > 1) {
		if (!strcmp(argv[1], "panic")) {
			_vm->_musicEngine->stopAllSounds();
			DebugPrintf("AAAIIIEEEEEE!\n");
			DebugPrintf("Shutting down all music tracks\n");
			return true;
		} else if (!strcmp (argv[1], "multimidi")) {
			if (argc > 2 && (!strcmp(argv[2], "on") || !strcmp(argv[2], "off"))) {
				if (_vm->_imuse)
					_vm->_imuse->property(IMuse::PROP_MULTI_MIDI, !strcmp(argv[2], "on"));
				DebugPrintf("MultiMidi mode switched %s.\n", argv[2]);
			} else {
				DebugPrintf("Specify \"on\" or \"off\" to switch.\n");
			}
			return true;
		} else if (!strcmp(argv[1], "play")) {
			if (argc > 2 && (!strcmp(argv[2], "random") || atoi(argv[2]) != 0)) {
				int sound = atoi(argv[2]);
				if (!strcmp(argv[2], "random")) {
					DebugPrintf("Selecting from %d songs...\n", _vm->getNumSounds());
					sound = _vm->_rnd.getRandomNumber(_vm->getNumSounds());
				}
				_vm->ensureResourceLoaded(rtSound, sound);
				_vm->_musicEngine->startSound(sound);

				DebugPrintf("Attempted to start music %d.\n", sound);
			} else {
				DebugPrintf("Specify a music resource # from 1-255.\n");
			}
			return true;
		} else if (!strcmp(argv[1], "stop")) {
			if (argc > 2 && (!strcmp(argv[2], "all") || atoi(argv[2]) != 0)) {
				if (!strcmp(argv[2], "all")) {
					_vm->_musicEngine->stopAllSounds();
					DebugPrintf("Shutting down all music tracks.\n");
				} else {
					_vm->_musicEngine->stopSound(atoi(argv[2]));
					DebugPrintf("Attempted to stop music %d.\n", atoi(argv[2]));
				}
			} else {
				DebugPrintf("Specify a music resource # or \"all\".\n");
			}
			return true;
		}
	}

	DebugPrintf("Available iMuse commands:\n");
	DebugPrintf("  panic - Stop all music tracks\n");
	DebugPrintf("  multimidi on/off - Toggle dual MIDI drivers\n");
	DebugPrintf("  play # - Play a music resource\n");
	DebugPrintf("  stop # - Stop a music resource\n");
	return true;
}

bool ScummDebugger::Cmd_Room(int argc, const char **argv) {
	if (argc > 1) {
		int room = atoi(argv[1]);
		_vm->_actors[_vm->VAR(_vm->VAR_EGO)].room = room;
		_vm->_sound->stopAllSounds();
		_vm->startScene(room, 0, 0);
		_vm->_fullRedraw = 1;
		return false;
	} else {
		DebugPrintf("Current room: %d [%d] - use 'room <roomnum>' to switch\n", _vm->_currentRoom, _vm->_roomResource);
		return true;
	}
}

bool ScummDebugger::Cmd_LoadGame(int argc, const char **argv) {
	if (argc > 1) {
		int slot = atoi(argv[1]);

		_vm->requestLoad(slot);

		_detach_now = true;
		return false;
	}

	DebugPrintf("Syntax: loadgame <slotnum>\n");
	return true;
}

bool ScummDebugger::Cmd_SaveGame(int argc, const char **argv) {
	if (argc > 2) {
		int slot = atoi(argv[1]);

		_vm->requestSave(slot, argv[2]);
	} else
		DebugPrintf("Syntax: savegame <slotnum> <name>\n");

	return true;
}

bool ScummDebugger::Cmd_Show(int argc, const char **argv) {

	if (argc != 2) {
		DebugPrintf("Syntax: show <parameter>\n");
		return true;
	}

	return true;
}

bool ScummDebugger::Cmd_Hide(int argc, const char **argv) {

	if (argc != 2) {
		DebugPrintf("Syntax: hide <parameter>\n");
		return true;
	}

	return true;
}

bool ScummDebugger::Cmd_Script(int argc, const char** argv) {
	int scriptnum;

	if (argc < 2) {
		DebugPrintf("Syntax: script <scriptnum> <command>\n");
		return true;
	}

	scriptnum = atoi(argv[1]);

	// FIXME: what is the max range on these?
	// if (scriptnum >= _vm->_numScripts) {
	//	DebugPrintf("Script number %d is out of range (range: 1 - %d)\n", scriptnum, _vm->_numScripts);
	//	return true;
	//}

	if ((!strcmp(argv[2], "kill")) || (!strcmp(argv[2], "stop"))) {
		_vm->stopScript(scriptnum);
	} else if ((!strcmp(argv[2], "run")) || (!strcmp(argv[2], "start"))) {
		_vm->runScript(scriptnum, 0, 0, 0);
		return false;
	} else {
		DebugPrintf("Unknown script command '%s'\nUse <kill/stop | run/start> as command\n", argv[2]);
	}

	return true;
}

bool ScummDebugger::Cmd_ImportRes(int argc, const char** argv) {
	File file;
	uint32 size;
	int resnum;

	if (argc != 4) {
		DebugPrintf("Syntax: importres <restype> <filename> <resnum>\n");
		return true;
	}

	resnum = atoi(argv[3]);
	// FIXME add bounds check

	if (!strncmp(argv[1], "scr", 3)) {
		file.open(argv[2], "");
		if (file.isOpen() == false) {
			DebugPrintf("Could not open file %s\n", argv[2]);
			return true;
		}
		file.readUint32BE();
		size = file.readUint32BE();
		file.seek(-8, SEEK_CUR);

		file.read(_vm->createResource(rtScript, resnum, size), size);

	} else
		DebugPrintf("Unknown importres type '%s'\n", argv[1]);
	return true;
}

bool ScummDebugger::Cmd_PrintScript(int argc, const char **argv) {
	int i;
	ScriptSlot *ss = _vm->vm.slot;
	DebugPrintf("+-----------------------------------+\n");
	DebugPrintf("|# | num|offst|sta|typ|fr|rec|fc|cut|\n");
	DebugPrintf("+--+----+-----+---+---+--+---+--+---+\n");
	for (i = 0; i < NUM_SCRIPT_SLOT; i++, ss++) {
		if (ss->number) {
			DebugPrintf("|%2d|%4d|%05x|%3d|%3d|%2d|%3d|%2d|%3d|\n",
					i, ss->number, ss->offs, ss->status, ss->where,
					ss->freezeResistant, ss->recursive,
					ss->freezeCount, ss->cutsceneOverride);
		}
	}
	DebugPrintf("+-----------------------------------+\n");

	return true;
}

bool ScummDebugger::Cmd_Actor(int argc, const char **argv) {
	Actor *a;
	int actnum;
	int value = 0;

	if (argc < 3) {
		DebugPrintf("Syntax: actor <actornum> <command> <parameter>\n");
		return true;
	}

	actnum = atoi(argv[1]);
	if (actnum >= _vm->_numActors) {
		DebugPrintf("Actor %d is out of range (range: 1 - %d)\n", actnum, _vm->_numActors);
		return true;
	}

	a = &_vm->_actors[actnum];
	if (argc > 3)
		value = atoi(argv[3]);

	if (!strcmp(argv[2], "ignoreboxes")) {
		a->ignoreBoxes = (value > 0);
		DebugPrintf("Actor[%d].ignoreBoxes = %d\n", actnum, a->ignoreBoxes);
	} else if (!strcmp(argv[2], "x")) {
		a->putActor(value, a->_pos.y, a->room);
		DebugPrintf("Actor[%d].x = %d\n", actnum, a->_pos.x);
		_vm->_fullRedraw = 1;
	} else if (!strcmp(argv[2], "y")) {
		a->putActor(a->_pos.x, value, a->room);
		DebugPrintf("Actor[%d].y = %d\n", actnum, a->_pos.y);
		_vm->_fullRedraw = 1;
	} else if (!strcmp(argv[2], "elevation")) {
		a->setElevation(value);
		DebugPrintf("Actor[%d].elevation = %d\n", actnum, a->getElevation());
		_vm->_fullRedraw = 1;
	} else if (!strcmp(argv[2], "costume")) {
		if (value >= _vm->res.num[rtCostume])
			DebugPrintf("Costume not changed as %d exceeds max of %d\n", value, _vm->res.num[rtCostume]);
		else {
			a->setActorCostume( value );
			_vm->_fullRedraw = 1;
			DebugPrintf("Actor[%d].costume = %d\n", actnum, a->costume);
		}
	} else if (!strcmp(argv[2], "name")) {
		DebugPrintf("Name of actor %d: %s\n", actnum, _vm->getObjOrActorName(actnum));
	} else {
		DebugPrintf("Unknown actor command '%s'\nUse <ignoreboxes |costume> as command\n", argv[2]);
	}

	return true;

}
bool ScummDebugger::Cmd_PrintActor(int argc, const char **argv) {
	int i;
	Actor *a;

	DebugPrintf("+----------------------------------------------------------------+\n");
	DebugPrintf("|# |room|  x |  y |elev|cos|width|box|mov| zp|frame|scale|dir|cls|\n");
	DebugPrintf("+--+----+----+----+----+---+-----+---+---+---+-----+-----+---+---+\n");
	for (i = 1; i < _vm->_numActors; i++) {
		a = &_vm->_actors[i];
		if (a->visible)
			DebugPrintf("|%2d|%4d|%4d|%4d|%4d|%3d|%5d|%3d|%3d|%3d|%5d|%5d|%3d|$%02x|\n",
						 a->number, a->room, a->_pos.x, a->_pos.y, a->getElevation(), a->costume,
						 a->width, a->walkbox, a->moving, a->forceClip, a->frame,
						 a->scalex, a->getFacing(), int(_vm->_classData[a->number]&0xFF));
	}
	DebugPrintf("+----------------------------------------------------------------+\n");
	return true;
}

bool ScummDebugger::Cmd_PrintObjects(int argc, const char **argv) {
	int i;
	ObjectData *o;
	DebugPrintf("Objects in current room\n");
	DebugPrintf("+---------------------------------+--+\n");
	DebugPrintf("|num |  x |  y |width|height|state|fl|\n");
	DebugPrintf("+----+----+----+-----+------+-----+--+\n");

	for (i = 1; (i < _vm->_numLocalObjects) && (_vm->_objs[i].obj_nr != 0) ; i++) {
		o = &(_vm->_objs[i]);
		DebugPrintf("|%4d|%4d|%4d|%5d|%6d|%5d|%2d|\n",
				o->obj_nr, o->x_pos, o->y_pos, o->width, o->height, o->state, o->fl_object_index);
	}
	DebugPrintf("\n");
	return true;
}

bool ScummDebugger::Cmd_Object(int argc, const char **argv) {
	int i;
	int obj;

	if (argc < 3) {
		DebugPrintf("Syntax: object <objectnum> <command> <parameter>\n");
		return true;
	}

	obj = atoi(argv[1]);
	if (obj >= _vm->_numGlobalObjects) {
		DebugPrintf("Object %d is out of range (range: 1 - %d)\n", obj, _vm->_numGlobalObjects);
		return true;
	}

	if (!strcmp(argv[2], "pickup")) {
		for (i = 0; i < _vm->_numInventory; i++) {
			if (_vm->_inventory[i] == (uint16)obj) {
				_vm->putOwner(obj, _vm->VAR(_vm->VAR_EGO));
				_vm->runInventoryScript(obj);
				return true;
			}
		}

		if (argc == 3)
			_vm->addObjectToInventory(obj, _vm->_currentRoom);
		else
			_vm->addObjectToInventory(obj, atoi(argv[3]));

		_vm->putOwner(obj, _vm->VAR(_vm->VAR_EGO));
		_vm->putClass(obj, kObjectClassUntouchable, 1);
		_vm->putState(obj, 1);
		_vm->markObjectRectAsDirty(obj);
		_vm->clearDrawObjectQueue();
		_vm->runInventoryScript(obj);
	} else if (!strcmp(argv[2], "state")) {
		_vm->putState(obj, atoi(argv[3]));
		//is BgNeedsRedraw enough?
		_vm->_BgNeedsRedraw = true;
	} else if (!strcmp(argv[2], "name")) {
		DebugPrintf("Name of object %d: %s\n", obj, _vm->getObjOrActorName(obj));
	} else {
		DebugPrintf("Unknown object command '%s'\nUse <pickup | state> as command\n", argv[2]);
	}

	return true;
}

bool ScummDebugger::Cmd_Help(int argc, const char **argv) {
	// console normally has 39 line width
	// wrap around nicely
	int width = 0, size, i;

	DebugPrintf("Commands are:\n");
	for (i = 0 ; i < _dcmd_count ; i++) {
		size = strlen(_dcmds[i].name) + 1;

		if ((width + size) >= 39) {
			DebugPrintf("\n");
			width = size;
		} else
			width += size;

		DebugPrintf("%s ", _dcmds[i].name);
	}

	width = 0;

	DebugPrintf("\n\nVariables are:\n");
	for (i = 0 ; i < _dvar_count ; i++) {
		size = strlen(_dvars[i].name) + 1;

		if ((width + size) >= 39) {
			DebugPrintf("\n");
			width = size;
		} else
			width += size;

		DebugPrintf("%s ", _dvars[i].name);
	}

	DebugPrintf("\n");

	return true;
}

bool ScummDebugger::Cmd_Debug(int argc, const char **argv) {
/*
	int numChannels = sizeof(debugChannels) / sizeof(dbgChannelDesc);

	bool setFlag = false;	// Remove or add debug channel?

	if ((argc == 1) && (_vm->_debugFlags == 0)) {
		DebugPrintf("No debug flags are enabled\n");
		DebugPrintf("Available Channels: ");
		for (int i = 0; i < numChannels; i++) {
			DebugPrintf("%s, ", debugChannels[i].channel);
		}
		DebugPrintf("\n");
		return true;
	}

	if ((argc == 1) && (_vm->_debugFlags > 0)) {
		for (int i = 0; i < numChannels; i++) {
			if(_vm->_debugFlags & debugChannels[i].flag)
				DebugPrintf("%s - %s\n", debugChannels[i].channel, 
							 debugChannels[i].desc);
		}
		return true;
	}

	// Enable or disable channel?
	if (argv[1][0] == '+') {
		setFlag = true;
	} else if (argv[1][0] == '-') {
		setFlag = false;
	} else {
		DebugPrintf("Syntax: Debug +CHANNEL, or Debug -CHANNEL\n");
		DebugPrintf("Available Channels: ");
		for (int i = 0; i < numChannels; i++) {
			DebugPrintf("%s, ", debugChannels[i].channel);
			DebugPrintf("\n");
		}
	}

	// Identify flag
	const char *realFlag = argv[1] + 1;
	for (int i = 0; i < numChannels; i++) {
		if((scumm_stricmp(debugChannels[i].channel, realFlag)) == 0) {
			if (setFlag) {
				_vm->_debugFlags |= debugChannels[i].flag;
				DebugPrintf("Enable ");
			} else {
				_vm->_debugFlags &= ~debugChannels[i].flag;
				DebugPrintf("Disable ");
			}

			DebugPrintf("%s\n", debugChannels[i].desc);
			return true;
		}
	}

	DebugPrintf("Unknown flag. Type 'Debug ?' for syntax\n");
*/
	return true;
};

bool ScummDebugger::Cmd_DebugLevel(int argc, const char **argv) {
	if (argc == 1) {
		if (_vm->_debugMode == false)
			DebugPrintf("Debugging is not enabled at this time\n");
		else
			DebugPrintf("Debugging is currently set at level %d\n", g_debugLevel);
	} else { // set level
		int level = atoi(argv[1]);
		g_debugLevel = level;
		if (level > 0) {
			_vm->_debugMode = true;
			DebugPrintf("Debug level set to level %d\n", level);
		} else if (level == 0) {
			_vm->_debugMode = false;
			DebugPrintf("Debugging is now disabled\n");
		} else
			DebugPrintf("Not a valid debug level\n");
	}

	return true;
}

bool ScummDebugger::Cmd_PrintBox(int argc, const char **argv) {
	int num, i = 0;

	if (argc > 1) {
		for (i = 1; i < argc; i++)
			printBox(atoi(argv[i]));
	} else {
		num = _vm->getNumBoxes();
		DebugPrintf("\nWalk boxes:\n");
		for (i = 0; i < num; i++)
			printBox(i);
	}
	return true;
}

bool ScummDebugger::Cmd_PrintBoxMatrix(int argc, const char **argv) {
	byte *boxm = _vm->getBoxMatrixBaseAddr();
	int num = _vm->getNumBoxes();
	int i, j;

	DebugPrintf("Walk matrix:\n");
	if (_vm->_version <= 2)
		boxm += num;
	for (i = 0; i < num; i++) {
		DebugPrintf("%d: ", i);
		if (_vm->_version <= 2) {
			for (j = 0; j < num; j++)
				DebugPrintf("[%d] ", *boxm++);
		} else {
			while (*boxm != 0xFF) {
				DebugPrintf("[%d-%d=>%d] ", boxm[0], boxm[1], boxm[2]);
				boxm += 3;
			}
			boxm++;
		}
		DebugPrintf("\n");
	}
	return true;
}

void ScummDebugger::printBox(int box) {
	if (box < 0 || box >= _vm->getNumBoxes()) {
		DebugPrintf("%d is not a valid box!\n", box);
		return;
	}
	BoxCoords coords;
	int flags = _vm->getBoxFlags(box);
	int mask = _vm->getMaskFromBox(box);
	int scale = _vm->getBoxScale(box);

	_vm->getBoxCoordinates(box, &coords);

	// Print out coords, flags, zbuffer mask
	DebugPrintf("%d: [%d x %d] [%d x %d] [%d x %d] [%d x %d], flags=0x%02x, mask=%d, scale=%d\n",
								box,
								coords.ul.x, coords.ul.y, coords.ll.x, coords.ll.y,
								coords.ur.x, coords.ur.y, coords.lr.x, coords.lr.y,
								flags, mask, scale);

	// Draw the box
	drawBox(box);
}

/************ ENDER: Temporary debug code for boxen **************/

static int gfxPrimitivesCompareInt(const void *a, const void *b);


static void hlineColor(ScummEngine *scumm, int x1, int x2, int y, byte color) {
	VirtScreen *vs = &scumm->virtscr[0];
	byte *ptr;

	// Clip y
	y += scumm->_screenTop;
	if (y < 0 || y >= SCREEN_HEIGHT)
		return;

	if (x2 < x1)
		SWAP(x2, x1);

	// Clip x1 / x2
	const int left = scumm->_screenStartStrip * 8;
	const int right = scumm->_screenEndStrip * 8;
	if (x1 < left)
		x1 = left;
	if (x2 >= right)
		x2 = right - 1;


	ptr = vs->screenPtr + x1 + y * vs->width;

	while (x1++ <= x2) {
		*ptr++ = color;
	}
}

static int gfxPrimitivesCompareInt(const void *a, const void *b) {
	return (*(const int *)a) - (*(const int *)b);
}

static void fillQuad(ScummEngine *scumm, Common::Point v[4], int color) {
	const int N = 4;
	int i;
	int y;
	int miny, maxy;
	Common::Point pt1, pt2;

	int polyInts[N];


	// Determine Y maxima
	miny = maxy = v[0].y;
	for (i = 1; i < N; i++) {
		if (v[i].y < miny) {
			miny = v[i].y;
		} else if (v[i].y > maxy) {
			maxy = v[i].y;
		}
	}

	// Draw, scanning y
	for (y = miny; y <= maxy; y++) {
		int ints = 0;
		for (i = 0; i < N; i++) {
			int ind1 = i;
			int ind2 = (i + 1) % N;
			pt1 = v[ind1];
			pt2 = v[ind2];
			if (pt1.y > pt2.y) {
				SWAP(pt1, pt2);
			}

			if (pt1.y <= y && y <= pt2.y) {
				if (y == pt1.y && y == pt2.y) {
					hlineColor(scumm, pt1.x, pt2.x, y, color);
				} else if ((y >= pt1.y) && (y < pt2.y)) {
					polyInts[ints++] = (y - pt1.y) * (pt2.x - pt1.x) / (pt2.y - pt1.y) + pt1.x;
				} else if ((y == maxy) && (y > pt1.y) && (y <= pt2.y)) {
					polyInts[ints++] = (y - pt1.y) * (pt2.x - pt1.x) / (pt2.y - pt1.y) + pt1.x;
				}
			}
		}
		qsort(polyInts, ints, sizeof(int), gfxPrimitivesCompareInt);

		for (i = 0; i < ints; i += 2) {
			hlineColor(scumm, polyInts[i], polyInts[i + 1], y, color);
		}
	}

	return;
}

void ScummDebugger::drawBox(int box) {
	BoxCoords coords;
	Common::Point r[4];

	_vm->getBoxCoordinates(box, &coords);

	r[0] = coords.ul;
	r[1] = coords.ur;
	r[2] = coords.lr;
	r[3] = coords.ll;

	// TODO - maybe use different colors for each box, and/or print the box number inside it?
	fillQuad(_vm, r, 13);

	VirtScreen *vs = _vm->findVirtScreen(coords.ul.y);
	if (vs != NULL)
		_vm->markRectAsDirty(vs->number, 0, vs->width, 0, vs->height);
	_vm->drawDirtyScreenParts();
	_vm->_system->update_screen();
}

bool ScummDebugger::Cmd_PrintDraft(int argc, const char **argv) {
/*
	const char *names[] = {
		"Opening",      "Straw to Gold", "Dyeing",
		"Night Vision",	"Twisting",      "Sleep",
		"Emptying",     "Invisibility",  "Terror",
		"Sharpening",   "Reflection",    "Healing",
		"Silence",      "Shaping",       "Unmaking",
		"Transcendence"
	};
	int odds[] = {
		15162, 15676, 16190,    64, 16961, 17475, 17989, 18503,
		   73, 19274,    76,    77, 20302, 20816, 21330,    84
	};

	const char *notes = "cdefgabC";
	int i, base, draft;

	if (_vm->_gameId != GID_LOOM && _vm->_gameId != GID_LOOM256) {
		DebugPrintf("Command only works with Loom/LoomCD\n");
		return true;
	}

	// There are 16 drafts, stored from variable 50 or 100 and upwards.
	// Each draft occupies two variables. Even-numbered variables contain
	// the notes for each draft, and a number of flags:
	//
	// +---+---+---+---+-----+-----+-----+-----+
	// | A | B | C | D | 444 | 333 | 222 | 111 |
	// +---+---+---+---+-----+-----+-----+-----+
	//
	// A   Unknown
	// B   The player has used the draft successfully at least once
	// C   The player has knowledge of the draft
	// D   Unknown
	// 444 The fourth note
	// 333 The third note
	// 222 The second note
	// 111 The first note
	//
	// I don't yet know what the odd-numbered variables are used for.
	// Possibly they store information on where and/or how the draft can
	// be used. They appear to remain constant throughout the game.

	base = (_vm->_gameId == GID_LOOM) ? 50 : 100;

	if (argc == 2) {
		// We had to debug a problem at the end of the game that only
		// happened if you interrupted the intro at a specific point.
		// That made it useful with a command to learn all the drafts
		// and notes.

		if (strcmp(argv[1], "learn") == 0) {
			for (i = 0; i < 16; i++)
				_vm->_scummVars[base + 2 * i] |= 0x2000;
			_vm->_scummVars[base + 72] = 8;

			// In theory, we could run script 18 here to redraw
			// the distaff, but I don't know if that's a safe
			// thing to do.

			DebugPrintf("Learned all drafts and notes.\n");
			return true;
		}

		// During the testing of EGA Loom we had some trouble with the
		// drafts data structure being overwritten. I don't expect
		// this command is particularly useful any more, but it will
		// attempt to repair the (probably) static part of it.

		if (strcmp(argv[1], "fix") == 0) {
			for (i = 0; i < 16; i++)
				_vm->_scummVars[base + 2 * i + 1] = odds[i];
			DebugPrintf(
				"An attempt has been made to repair\n"
				"the internal drafts data structure.\n"
				"Continue on your own risk.\n");
			return true;
		}
	}

	// Probably the most useful command for ordinary use: list the drafts.

	for (i = 0; i < 16; i++) {
		draft = _vm->_scummVars[base + i * 2];
		DebugPrintf("%d %-13s %c%c%c%c %c%c %5d %c\n",
			base + 2 * i,
			names[i],
			notes[draft & 0x0007],
			notes[(draft & 0x0038) >> 3],
			notes[(draft & 0x01c0) >> 6],
			notes[(draft & 0x0e00) >> 9],
			(draft & 0x2000) ? 'K' : ' ',
			(draft & 0x4000) ? 'U' : ' ',
			_vm->_scummVars[base + 2 * i + 1],
			(_vm->_scummVars[base + 2 * i + 1] != odds[i]) ? '!' : ' ');
	}
*/
	return true;
}

} // End of namespace Scumm



namespace Common {

template <class T>
Debugger<T>::Debugger() {
	_frame_countdown = 0;
	_dvar_count = 0;
	_dcmd_count = 0;
	_detach_now = false;
	_isAttached = false;
	_errStr = NULL;
	_firstTime = true;
}

template <class T>
Debugger<T>::~Debugger() {
}


// Initialisation Functions
template <class T>
int Debugger<T>::DebugPrintf(const char *format, ...) {
	va_list	argptr;

	va_start(argptr, format);
	int count;
	count = ::vprintf(format, argptr);
	va_end (argptr);
	return count;
}

template <class T>
void Debugger<T>::attach(const char *entry) {

	OSystem::Property prop;
	prop.show_keyboard = true;
	g_system->property(OSystem::PROP_TOGGLE_VIRTUAL_KEYBOARD, &prop);

	if (entry) {
		_errStr = strdup(entry);
	}

	_frame_countdown = 1;
	_detach_now = false;
	_isAttached = true;
}

template <class T>
void Debugger<T>::detach() {
	OSystem::Property prop;
	prop.show_keyboard = false;
	g_system->property(OSystem::PROP_TOGGLE_VIRTUAL_KEYBOARD, &prop);

	_detach_now = false;
	_isAttached = false;
}

// Temporary execution handler
template <class T>
void Debugger<T>::onFrame() {
	if (_frame_countdown == 0)
		return;
	--_frame_countdown;

	if (!_frame_countdown) {

		preEnter();
		enter();
		postEnter();

		// Detach if we're finished with the debugger
		if (_detach_now)
			detach();
	}
}

// Main Debugger Loop
template <class T>
void Debugger<T>::enter() {
	// TODO: compared to the console input, this here is very bare bone.
	// For example, no support for tab completion and no history. At least
	// we should re-add (optional) support for the readline library.
	// Or maybe instead of choosing between a console dialog and stdio,
	// we should move that choice into the ConsoleDialog class - that is,
	// the console dialog code could be #ifdef'ed to not print to the dialog
	// but rather to stdio. This way, we could also reuse the command history
	// and tab completion of the console. It would still require a lot of
	// work, but at least no dependency on a 3rd party library...

	printf("Debugger entered, please switch to this console for input.\n");

	int i;
	char buf[256];

	do {
		printf("debug> ");
		if (!fgets(buf, sizeof(buf), stdin))
			return;

		i = strlen(buf);
		while (i > 0 && buf[i - 1] == '\n')
			buf[--i] = 0;

		if (i == 0)
			continue;
	} while (RunCommand(buf));
}

// Command execution loop
template <class T>
bool Debugger<T>::RunCommand(const char *inputOrig) {
	int i = 0, num_params = 0;
	const char *param[256];
	char *input = strdup(inputOrig);	// One of the rare occasions using strdup is OK (although avoiding strtok might be more elegant here).

	// Parse out any params
	char *tok = strtok(input, " ");
	if (tok) {
		do {
			param[num_params++] = tok;
		} while ((tok = strtok(NULL, " ")) != NULL);
	} else {
		param[num_params++] = input;
	}

	for (i=0; i < _dcmd_count; i++) {
		if (!strcmp(_dcmds[i].name, param[0])) {
			bool result = (((T *)this)->*_dcmds[i].function)(num_params, param);
			free(input);
			return result;
		}
	}

	// It's not a command, so things get a little tricky for variables. Do fuzzy matching to ignore things like subscripts.
	for (i = 0; i < _dvar_count; i++) {
		if (!strncmp(_dvars[i].name, param[0], strlen(_dvars[i].name))) {
			if (num_params > 1) {
				// Alright, we need to check the TYPE of the variable to deref and stuff... the array stuff is a bit ugly :)
				switch(_dvars[i].type) {
				// Integer
				case DVAR_INT:
					*(int *)_dvars[i].variable = atoi(param[1]);
					DebugPrintf("(int)%s = %d\n", param[0], *(int *)_dvars[i].variable);
				break;
				// Integer Array
				case DVAR_INTARRAY: {
					char *chr = strchr(param[0], '[');
					if (!chr) {
						DebugPrintf("You must access this array as %s[element]\n", param[0]);
					} else {
						int element = atoi(chr+1);
						int32 *var = *(int32 **)_dvars[i].variable;
						if (element > _dvars[i].optional) {
							DebugPrintf("%s is out of range (array is %d elements big)\n", param[0], _dvars[i].optional);
						} else {
							var[element] = atoi(param[1]);
							DebugPrintf("(int)%s = %d\n", param[0], var[element]);
						}
					}
				}
				break;
				default:
					DebugPrintf("Failed to set variable %s to %s - unknown type\n", _dvars[i].name, param[1]);
					break;
				}
			} else {
				// And again, type-dependent prints/defrefs. The array one is still ugly.
				switch(_dvars[i].type) {
				// Integer
				case DVAR_INT:
					DebugPrintf("(int)%s = %d\n", param[0], *(int *)_dvars[i].variable);
				break;
				// Integer array
				case DVAR_INTARRAY: {
					char *chr = strchr(param[0], '[');
					if (!chr) {
						DebugPrintf("You must access this array as %s[element]\n", param[0]);
					} else {
						int element = atoi(chr+1);
						int16 *var = *(int16 **)_dvars[i].variable;
						if (element > _dvars[i].optional) {
							DebugPrintf("%s is out of range (array is %d elements big)\n", param[0], _dvars[i].optional);
						} else {
							DebugPrintf("(int)%s = %d\n", param[0], var[element]);
						}
					}
				}
				break;
				// String
				case DVAR_STRING:
					DebugPrintf("(string)%s = %s\n", param[0], ((Common::String *)_dvars[i].variable)->c_str());
					break;
				default:
					DebugPrintf("%s = (unknown type)\n", param[0]);
					break;
				}
			}

			free(input);
			return true;
		}
	}

	DebugPrintf("Unknown command or variable\n");
	free(input);
	return true;
}

// returns true if something has been completed
// completion has to be delete[]-ed then
template <class T>
bool Debugger<T>::TabComplete(const char *input, char*& completion) {
	// very basic tab completion
	// for now it just supports command completions

	// adding completions of command parameters would be nice (but hard) :-)
	// maybe also give a list of possible command completions?
	//   (but this will require changes to console)

	if (strchr(input, ' '))
		return false; // already finished the first word

	unsigned int inputlen = strlen(input);

	unsigned int matchlen = 0;
	char match[30]; // the max. command name is 30 chars

	for (int i = 0; i < _dcmd_count; i++) {
		if (!strncmp(_dcmds[i].name, input, inputlen)) {
			unsigned int commandlen = strlen(_dcmds[i].name);
			if (commandlen == inputlen) { // perfect match
				return false;
			}
			if (commandlen > inputlen) { // possible match
				// no previous match
				if (matchlen == 0) {
					strcpy(match, _dcmds[i].name + inputlen);
					matchlen = commandlen - inputlen;
				} else {
					// take common prefix of previous match and this command
					unsigned int j;
					for (j = 0; j < matchlen; j++) {
						if (match[j] != _dcmds[i].name[inputlen + j]) break;
					}
					matchlen = j;
				}
				if (matchlen == 0)
					return false;
			}
		}
	}
	if (matchlen == 0)
		return false;

	completion = new char[matchlen + 1];
	memcpy(completion, match, matchlen);
	completion[matchlen] = 0;
	return true;
}

// Variable registration function
template <class T>
void Debugger<T>::DVar_Register(const char *varname, void *pointer, int type, int optional) {
	assert(_dvar_count < (int)sizeof(_dvars));
	strcpy(_dvars[_dvar_count].name, varname);
	_dvars[_dvar_count].type = type;
	_dvars[_dvar_count].variable = pointer;
	_dvars[_dvar_count].optional = optional;

	_dvar_count++;
}

// Command registration function
template <class T>
void Debugger<T>::DCmd_Register(const char *cmdname, DebugProc pointer) {
	assert(_dcmd_count < (int)sizeof(_dcmds));
	strcpy(_dcmds[_dcmd_count].name, cmdname);
	_dcmds[_dcmd_count].function = pointer;

	_dcmd_count++;
}

}	// End of namespace Common



#endif 

