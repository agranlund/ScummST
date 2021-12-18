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
 * $Header: /cvsroot/scummvm/scummvm/scumm/saveload.cpp,v 1.140.2.1 2004/02/17 00:44:11 fingolfin Exp $
 *
 */

#include "stdafx.h"

#include "common/config-manager.h"

#include "scumm/actor.h"
#include "scumm/charset.h"
#include "scumm/imuse.h"
#include "scumm/object.h"
#include "scumm/resource.h"
#include "scumm/saveload.h"
#include "scumm/scumm.h"
#include "scumm/sound.h"
#include "scumm/verbs.h"
#include "sound/mixer.h"


namespace Scumm {

struct SaveGameHeader {
	uint32 type;
	uint32 size;
	uint32 ver;
	char name[32];
};


void ScummEngine::requestSave(int slot, const char *name, bool compatible) {
	_saveLoadSlot = slot;
	_saveTemporaryState = compatible;
	_saveLoadFlag = 1;		// 1 for save
	assert(name);
	strcpy(_saveLoadName, name);
}

void ScummEngine::requestLoad(int slot) {
	_saveLoadSlot = slot;
	_saveTemporaryState = false;
	_saveLoadFlag = 2;		// 2 for load
}

bool ScummEngine::saveState(int slot, bool compat, SaveFileManager *mgr) {
	char filename[256];
	SaveFile *out;
	SaveGameHeader hdr;

	makeSavegameName(filename, slot, compat);

	if (!(out = mgr->open_savefile(filename, getSavePath(), true)))
		return false;

	memcpy(hdr.name, _saveLoadName, sizeof(hdr.name));

	hdr.type = MKID('SCST');
	hdr.size = 0;
	hdr.ver = TO_LE_32(CURRENT_VER);

	out->write(&hdr, sizeof(hdr));

	Serializer ser(out, true, CURRENT_VER);
	saveOrLoad(&ser, CURRENT_VER);
	delete out;
	debug(1, "State saved as '%s'", filename);
	return true;
}

bool ScummEngine::loadState(int slot, bool compat, SaveFileManager *mgr) {
	char filename[256];
	SaveFile *in;
	int i, j;
	SaveGameHeader hdr;

	makeSavegameName(filename, slot, compat);
	if (!(in = mgr->open_savefile(filename, getSavePath(), false)))
		return false;

	in->read(&hdr, sizeof(hdr));
	if (hdr.type != MKID('SCST')) {
		warning("Invalid savegame '%s'", filename);
		delete in;
		return false;
	}

	// In older versions of ScummVM, the header version was not endian safe.
	// We account for that by retrying once with swapped byte order.
	if (hdr.ver > CURRENT_VER)
		hdr.ver = SWAP_BYTES_32(hdr.ver);
	if (hdr.ver < MIN_VALID_VER || hdr.ver > CURRENT_VER)
	{
		warning("Invalid version of '%s'", filename);
		delete in;
		return false;
	}

	// Due to a bug in scummvm up to and including 0.3.0, save games could be saved
	// in the V8/V9 format but were tagged with a V7 mark. Ouch. So we just pretend V7 == V8 here
	if (hdr.ver == VER(7))
		hdr.ver = VER(8);

	memcpy(_saveLoadName, hdr.name, sizeof(hdr.name));

	// Unless specifically requested with _saveSound, we do not save the iMUSE
	// state for temporary state saves - such as certain cutscenes in DOTT,
	// FOA, Sam and Max, etc.
	//
	// Thusly, we should probably not stop music when restoring from one of 
	// these saves. This change stops the Mole Man theme from going quiet in
	// Sam & Max when Doug tells you about the Ball of Twine, as mentioned in
	// patch #886058.
	//
	// If we don't have iMUSE at all we may as well stop the sounds. The previous
	// default behavior here was to stopAllSounds on all state restores.

	if (!_imuse || _saveSound || !_saveTemporaryState)
		_sound->stopAllSounds();

	_sound->pauseSounds(true);

	CHECK_HEAP

	closeRoom();

	memset(_inventory, 0, sizeof(_inventory[0]) * _numInventory);
	memset(_newNames, 0, sizeof(_newNames[0]) * _numNewNames);
	
	// Because old savegames won't fill the entire gfxUsageBits[] array,
	// clear it here just to be sure it won't hold any unforseen garbage.
	memset(gfxUsageBits, 0, sizeof(gfxUsageBits));

	// Nuke all resources
	for (i = rtFirst; i <= rtLast; i++)
		if (i != rtTemp && i != rtBuffer && (i != rtSound || _saveSound || !compat))
			for (j = 0; j < res.num[i]; j++) {
				nukeResource(i, j);
				res.flags[i][j] = 0;
			}

	initScummVars();

	//
	// Now do the actual loading
	//
	Serializer ser(in, false, hdr.ver);
	saveOrLoad(&ser, hdr.ver);
	delete in;
	
	// Normally, _vm->_screenTop should always be >= 0, but for some old save games
	// it is not, hence we check & correct it here.
	if (_screenTop < 0)
		_screenTop = 0;
	
	setDirtyColors(0, 255);

	camera._last.x = camera._cur.x;

	// verb bkcolor was changed from system_pal to game_pal in version 32.
	// workaround: force to 0 in old versions
	if (hdr.ver < VER(32))
		for (int i=0; i<_numVerbs; ++i)
			_verbs[i].bkcolor = 0;

	// Restore the virtual screens and force a fade to black.
	initScreens(_screenB, _screenH);
	VirtScreen *vs = &virtscr[0];
	memset(vs->screenPtr + vs->xstart, 0, (vs->width * vs->height) >> 1);
	vs->setDirtyRange(0, vs->height);
	updateDirtyScreen(kMainVirtScreen);
	updatePalette();
	_completeScreenRedraw = true;

	// Reset charset mask
	_charset->_mask.top = _charset->_mask.left = 32767;
	_charset->_mask.right = _charset->_mask.bottom = 0;
	_charset->_hasMask = false;

	// With version 22, we replaced the scale items with scale slots. So when
	// loading such an old save game, try to upgrade the old to new format.
	/*
	if (hdr.ver < VER(22)) {
		// Convert all rtScaleTable resources to matching scale items
		for (i = 1; i < res.num[rtScaleTable]; i++) {
			convertScaleTableToScaleSlot(i);
		}
	}
	*/

	_lastCodePtr = NULL;
	_drawObjectQueNr = 0;
	_verbMouseOver = 0;

	cameraMoved();

	initBGBuffers(_roomHeight);

#ifdef GAME_SAMNMAX
	if (hdr.ver >= VER(34))
	{
		int16 room = (_grabbedCursorId & 0xFF000000) >> 24;
		int16 img  = (_grabbedCursorId & 0x00FFFF00) >> 8;
		int16 idx  = (_grabbedCursorId & 0x000000F0) >> 4;
		debug(1,"_grabbedCursorId = 0x%08x (%d,%d,%d)", _grabbedCursorId, room, img, idx);
		if (room != 0x00 && room != 0xFF && img != 0x0000)
		{
			byte transpColors[16];
			memcpy(transpColors, _grabbedCursorTransp, 16);
			setCursorImg(img, room, idx);
			for (int i=0; i!=16; i++)
			{
				if (transpColors[i] == 0xFF)
					break;
				makeCursorColorTransparent(transpColors[i]);
			}
		}
		else
		{
			// use saved _grabbedCursor
			updateCursor();
		}
		_system->warp_mouse(_mouse.x, _mouse.y);
	}	
#endif

	CHECK_HEAP
	debug(1, "State loaded from '%s'", filename);

	_sound->pauseSounds(false);

	return true;
}

void ScummEngine::makeSavegameName(char *out, int slot, bool compatible) {
	sprintf(out, "%s.%c%02d", _targetName.c_str(), compatible ? 'c' : 's', slot);
}

void ScummEngine::listSavegames(bool *marks, int num, SaveFileManager *mgr) {
	char prefix[256];
	makeSavegameName(prefix, 99, false);
	prefix[strlen(prefix)-2] = 0;
	mgr->list_savefiles(prefix, getSavePath(), marks, num);
}

bool ScummEngine::getSavegameName(int slot, char *desc, SaveFileManager *mgr) {
	char filename[256];
	SaveFile *out;
	SaveGameHeader hdr;
	int len;

	makeSavegameName(filename, slot, false);
	if (!(out = mgr->open_savefile(filename, getSavePath(), false))) {
		strcpy(desc, "");
		return false;
	}
	len = out->read(&hdr, sizeof(hdr));
	delete out;

	if (len != sizeof(hdr) || hdr.type != MKID('SCST')) {
		strcpy(desc, "Invalid savegame");
		return false;
	}

	if (hdr.ver > CURRENT_VER)
		hdr.ver = TO_LE_32(hdr.ver);
	if (hdr.ver < MIN_VALID_VER || hdr.ver > CURRENT_VER) {
		strcpy(desc, "Invalid version");
		return false;
	}

	memcpy(desc, hdr.name, sizeof(hdr.name));
	desc[sizeof(hdr.name) - 1] = 0;
	return true;
}

void ScummEngine::saveOrLoad(Serializer *s, uint32 savegameVersion) {
	const SaveLoadEntry objectEntries[] = {
		MKLINE(ObjectData, OBIMoffset, sleUint32, VER(8)),
		MKLINE(ObjectData, OBCDoffset, sleUint32, VER(8)),
		MKLINE(ObjectData, walk_x, sleUint16, VER(8)),
		MKLINE(ObjectData, walk_y, sleUint16, VER(8)),
		MKLINE(ObjectData, obj_nr, sleUint16, VER(8)),
		MKLINE(ObjectData, x_pos, sleInt16, VER(8)),
		MKLINE(ObjectData, y_pos, sleInt16, VER(8)),
		MKLINE(ObjectData, width, sleUint16, VER(8)),
		MKLINE(ObjectData, height, sleUint16, VER(8)),
		MKLINE(ObjectData, actordir, sleByte, VER(8)),
		MKLINE(ObjectData, parentstate, sleByte, VER(8)),
		MKLINE(ObjectData, parent, sleByte, VER(8)),
		MKLINE(ObjectData, state, sleByte, VER(8)),
		MKLINE(ObjectData, fl_object_index, sleByte, VER(8)),
		MKEND()
	};

	const SaveLoadEntry *actorEntries = Actor::getSaveLoadEntries();

	const SaveLoadEntry verbEntries[] = {
		MKLINE(VerbSlot, curRect.left, sleInt16, VER(8)),
		MKLINE(VerbSlot, curRect.top, sleInt16, VER(8)),
		MKLINE(VerbSlot, curRect.right, sleInt16, VER(8)),
		MKLINE(VerbSlot, curRect.bottom, sleInt16, VER(8)),
		MKLINE(VerbSlot, oldRect.left, sleInt16, VER(8)),
		MKLINE(VerbSlot, oldRect.top, sleInt16, VER(8)),
		MKLINE(VerbSlot, oldRect.right, sleInt16, VER(8)),
		MKLINE(VerbSlot, oldRect.bottom, sleInt16, VER(8)),
		MKLINE(VerbSlot, verbid, sleInt16, VER(12)),
		MKLINE(VerbSlot, color, sleByte, VER(8)),
		MKLINE(VerbSlot, hicolor, sleByte, VER(8)),
		MKLINE(VerbSlot, dimcolor, sleByte, VER(8)),
		MKLINE(VerbSlot, bkcolor, sleByte, VER(8)),
		MKLINE(VerbSlot, type, sleByte, VER(8)),
		MKLINE(VerbSlot, charset_nr, sleByte, VER(8)),
		MKLINE(VerbSlot, curmode, sleByte, VER(8)),
		MKLINE(VerbSlot, saveid, sleByte, VER(8)),
		MKLINE(VerbSlot, key, sleByte, VER(8)),
		MKLINE(VerbSlot, center, sleByte, VER(8)),
		MKLINE(VerbSlot, prep, sleByte, VER(8)),
		MKLINE(VerbSlot, imgindex, sleUint16, VER(8)),
		MKEND()
	};

	const SaveLoadEntry mainEntries[] = {
		MKLINE(ScummEngine, _roomWidth, sleUint16, VER(8)),
		MKLINE(ScummEngine, _roomHeight, sleUint16, VER(8)),
		MKLINE(ScummEngine, _ENCD_offs, sleUint32, VER(8)),
		MKLINE(ScummEngine, _EXCD_offs, sleUint32, VER(8)),
		MKLINE(ScummEngine, _IM00_offs, sleUint32, VER(8)),
		MKLINE(ScummEngine, _CLUT_offs, sleUint32, VER(8)),
		MKLINE(ScummEngine, _PALS_offs, sleUint32, VER(8)),
		MKLINE(ScummEngine, _curPalIndex, sleByte, VER(8)),
		MKLINE(ScummEngine, _currentRoom, sleByte, VER(8)),
		MKLINE(ScummEngine, _roomResource, sleByte, VER(8)),
		MKLINE(ScummEngine, _numObjectsInRoom, sleByte, VER(8)),
		MKLINE(ScummEngine, _currentScript, sleByte, VER(8)),
		MKARRAY(ScummEngine, _localScriptList[0], sleUint32, NUM_LOCALSCRIPT, VER(8)),

		MKARRAY2(ScummEngine, vm.localvar[0][0], sleUint32, 26, NUM_SCRIPT_SLOT, (byte*)vm.localvar[1] - (byte*)vm.localvar[0], VER(20)),

		MKARRAY(ScummEngine, _resourceMapper[0], sleByte, 128, VER(8)),
		MKARRAY(ScummEngine, _charsetColorMap[0], sleByte, 16, VER(8)),
		
		MKARRAY(ScummEngine, _charsetData[0][0], sleByte, 15 * 16, VER(10)),

		MKLINE(ScummEngine, _curExecScript, sleUint16, VER(8)),

		MKLINE(ScummEngine, camera._dest.x, sleInt16, VER(8)),
		MKLINE(ScummEngine, camera._dest.y, sleInt16, VER(8)),
		MKLINE(ScummEngine, camera._cur.x, sleInt16, VER(8)),
		MKLINE(ScummEngine, camera._cur.y, sleInt16, VER(8)),
		MKLINE(ScummEngine, camera._last.x, sleInt16, VER(8)),
		MKLINE(ScummEngine, camera._last.y, sleInt16, VER(8)),
		MKLINE(ScummEngine, camera._accel.x, sleInt16, VER(8)),
		MKLINE(ScummEngine, camera._accel.y, sleInt16, VER(8)),
		MKLINE(ScummEngine, _screenStartStrip, sleInt16, VER(8)),
		MKLINE(ScummEngine, _screenEndStrip, sleInt16, VER(8)),
		MKLINE(ScummEngine, camera._mode, sleByte, VER(8)),
		MKLINE(ScummEngine, camera._follows, sleByte, VER(8)),
		MKLINE(ScummEngine, camera._leftTrigger, sleInt16, VER(8)),
		MKLINE(ScummEngine, camera._rightTrigger, sleInt16, VER(8)),
		MKLINE(ScummEngine, camera._movingToActor, sleUint16, VER(8)),

		MKLINE(ScummEngine, _actorToPrintStrFor, sleByte, VER(8)),
		MKLINE(ScummEngine, _charsetColor, sleByte, VER(8)),

		MKLINE(ScummEngine, _charsetBufPos, sleInt16, VER(10)),

		MKLINE(ScummEngine, _haveMsg, sleByte, VER(8)),
		MKLINE(ScummEngine, _useTalkAnims, sleByte, VER(8)),

		MKLINE(ScummEngine, _talkDelay, sleInt16, VER(8)),
		MKLINE(ScummEngine, _defaultTalkDelay, sleInt16, VER(8)),
		MKLINE(ScummEngine, _sentenceNum, sleByte, VER(8)),

		MKLINE(ScummEngine, vm.cutSceneStackPointer, sleByte, VER(8)),
		MKARRAY(ScummEngine, vm.cutScenePtr[0], sleUint32, 5, VER(8)),
		MKARRAY(ScummEngine, vm.cutSceneScript[0], sleByte, 5, VER(8)),
		MKARRAY(ScummEngine, vm.cutSceneData[0], sleInt16, 5, VER(8)),
		MKLINE(ScummEngine, vm.cutSceneScriptIndex, sleInt16, VER(8)),

		MKLINE(ScummEngine, vm.numNestedScripts, sleByte, VER(8)),
		MKLINE(ScummEngine, _userPut, sleByte, VER(8)),
		MKLINE(ScummEngine, _userState, sleUint16, VER(17)),
		MKLINE(ScummEngine, _cursor.state, sleByte, VER(8)),
		MKLINE(ScummEngine, _currentCursor, sleByte, VER(8)),
		MKLINE(ScummEngine, _grabbedCursorId, sleUint32, VER(34)),
		MKARRAY(ScummEngine, _grabbedCursorTransp[0], sleByte, 16, VER(34)),
		MKARRAY(ScummEngine, _grabbedCursor[0], sleByte, 8192, VER(20)),
		MKLINE(ScummEngine, _cursor.width, sleInt16, VER(20)),
		MKLINE(ScummEngine, _cursor.height, sleInt16, VER(20)),
		MKLINE(ScummEngine, _cursor.hotspotX, sleInt16, VER(20)),
		MKLINE(ScummEngine, _cursor.hotspotY, sleInt16, VER(20)),
		MKLINE(ScummEngine, _cursor.animate, sleByte, VER(20)),
		MKLINE(ScummEngine, _cursor.animateIndex, sleByte, VER(20)),
		MKLINE(ScummEngine, _mouse.x, sleInt16, VER(20)),
		MKLINE(ScummEngine, _mouse.y, sleInt16, VER(20)),

		MKLINE(ScummEngine, _doEffect, sleByte, VER(8)),
		MKLINE(ScummEngine, _switchRoomEffect, sleByte, VER(8)),
		MKLINE(ScummEngine, _newEffect, sleByte, VER(8)),
		MKLINE(ScummEngine, _switchRoomEffect2, sleByte, VER(8)),
		MKLINE(ScummEngine, _BgNeedsRedraw, sleByte, VER(8)),

		// The state of palManipulate is stored only since V10
		MKLINE(ScummEngine, _palManipStart, sleByte, VER(10)),
		MKLINE(ScummEngine, _palManipEnd, sleByte, VER(10)),
		MKLINE(ScummEngine, _palManipCounter, sleUint16, VER(10)),

		MKARRAY(ScummEngine, gfxUsageBits[0], sleUint32, 200, VER(8)),

		MKLINE(ScummEngine, gdi._transparentColor, sleByte, VER(8)),
		MKARRAY(ScummEngine, _currentPalette[0], sleByte, 768, VER(8)),

		MKARRAY(ScummEngine, _charsetBuffer[0], sleByte, 256, VER(8)),

		MKLINE(ScummEngine, _egoPositioned, sleByte, VER(8)),

		MKLINE(ScummEngine, _screenEffectFlag, sleByte, VER(8)),

		MKLINE(ScummEngine, _shakeEnabled, sleByte, VER(10)),
		MKLINE(ScummEngine, _shakeFrame, sleUint32, VER(10)),

		MKLINE(ScummEngine, _keepText, sleByte, VER(8)),

		MKLINE(ScummEngine, _screenB, sleUint16, VER(8)),
		MKLINE(ScummEngine, _screenH, sleUint16, VER(8)),
		
		MKEND()
	};

	const SaveLoadEntry scriptSlotEntries[] = {
		MKLINE(ScriptSlot, offs, sleUint32, VER(8)),
		MKLINE(ScriptSlot, delay, sleInt32, VER(8)),
		MKLINE(ScriptSlot, number, sleUint16, VER(8)),
		MKLINE(ScriptSlot, delayFrameCount, sleUint16, VER(8)),
		MKLINE(ScriptSlot, status, sleByte, VER(8)),
		MKLINE(ScriptSlot, where, sleByte, VER(8)),
		MKLINE(ScriptSlot, freezeResistant, sleByte, VER(8)),
		MKLINE(ScriptSlot, recursive, sleByte, VER(8)),
		MKLINE(ScriptSlot, freezeCount, sleByte, VER(8)),
		MKLINE(ScriptSlot, didexec, sleByte, VER(8)),
		MKLINE(ScriptSlot, cutsceneOverride, sleByte, VER(8)),
		MKEND()
	};

	const SaveLoadEntry nestedScriptEntries[] = {
		MKLINE(NestedScript, number, sleUint16, VER(8)),
		MKLINE(NestedScript, where, sleByte, VER(8)),
		MKLINE(NestedScript, slot, sleByte, VER(8)),
		MKEND()
	};

	const SaveLoadEntry sentenceTabEntries[] = {
		MKLINE(SentenceTab, verb, sleUint8, VER(8)),
		MKLINE(SentenceTab, preposition, sleUint8, VER(8)),
		MKLINE(SentenceTab, objectA, sleUint16, VER(8)),
		MKLINE(SentenceTab, objectB, sleUint16, VER(8)),
		MKLINE(SentenceTab, freezeCount, sleUint8, VER(8)),
		MKEND()
	};

	const SaveLoadEntry stringTabEntries[] = {
		// TODO - It makes no sense to have all these t_* fields in StringTab
		// Rather let's dump them all when the save game format changes, and 
		// keep two StringTab objects where we have one now: a "normal" one,
		// and a temporar y"t_" one.
		// Then backup/restore of a StringTab entry becomes a one liner.
		MKLINE(StringTab, xpos, sleInt16, VER(8)),
		MKLINE(StringTab, t_xpos, sleInt16, VER(8)),
		MKLINE(StringTab, ypos, sleInt16, VER(8)),
		MKLINE(StringTab, t_ypos, sleInt16, VER(8)),
		MKLINE(StringTab, right, sleInt16, VER(8)),
		MKLINE(StringTab, t_right, sleInt16, VER(8)),
		MKLINE(StringTab, color, sleInt8, VER(8)),
		MKLINE(StringTab, t_color, sleInt8, VER(8)),
		MKLINE(StringTab, charset, sleInt8, VER(8)),
		MKLINE(StringTab, t_charset, sleInt8, VER(8)),
		MKLINE(StringTab, center, sleByte, VER(8)),
		MKLINE(StringTab, t_center, sleByte, VER(8)),
		MKLINE(StringTab, overhead, sleByte, VER(8)),
		MKLINE(StringTab, t_overhead, sleByte, VER(8)),
		MKLINE(StringTab, no_talk_anim, sleByte, VER(8)),
		MKLINE(StringTab, t_no_talk_anim, sleByte, VER(8)),
		MKEND()
	};

	const SaveLoadEntry colorCycleEntries[] = {
		MKLINE(ColorCycle, delay, sleUint16, VER(8)),
		MKLINE(ColorCycle, counter, sleUint16, VER(8)),
		MKLINE(ColorCycle, flags, sleUint16, VER(8)),
		MKLINE(ColorCycle, start, sleByte, VER(8)),
		MKLINE(ColorCycle, end, sleByte, VER(8)),
		MKEND()
	};

	const SaveLoadEntry scaleSlotsEntries[] = {
		MKLINE(ScaleSlot, x1, sleUint16, VER(13)),
		MKLINE(ScaleSlot, y1, sleUint16, VER(13)),
		MKLINE(ScaleSlot, scale1, sleUint16, VER(13)),
		MKLINE(ScaleSlot, x2, sleUint16, VER(13)),
		MKLINE(ScaleSlot, y2, sleUint16, VER(13)),
		MKLINE(ScaleSlot, scale2, sleUint16, VER(13)),
		MKEND()
	};


	int i, j;
	int var120Backup;
	int var98Backup;

	s->saveLoadEntries(this, mainEntries);
#ifndef GAME_SAMNMAX
	if (s->isLoading() && savegameVersion >= VER(20)) {
		updateCursor();
		_system->warp_mouse(_mouse.x, _mouse.y);
	}
#endif

	s->saveLoadArrayOf(_actors, _numActors, sizeof(_actors[0]), actorEntries);
	if (s->isLoading() && savegameVersion < VER(33)) {
		for (i = 0; i < _numActors; i++) {
			Actor* actor = &_actors[i];
			actor->customPalette = false;
			for (j = 0; j < 32 && actor->customPalette == false; j++) {
				if (actor->getPalette(j) != 0xFF) {
					actor->customPalette = true;
				}
			}
		}
	}

	if (savegameVersion < VER(9))
		s->saveLoadArrayOf(vm.slot, 25, sizeof(vm.slot[0]), scriptSlotEntries);
	else if (savegameVersion < VER(20))
		s->saveLoadArrayOf(vm.slot, 40, sizeof(vm.slot[0]), scriptSlotEntries);
	else
		s->saveLoadArrayOf(vm.slot, NUM_SCRIPT_SLOT, sizeof(vm.slot[0]), scriptSlotEntries);

	s->saveLoadArrayOf(_objs, _numLocalObjects, sizeof(_objs[0]), objectEntries);
	if (s->isLoading() && savegameVersion < VER(13)) {
		// Since roughly v13 of the save games, the objs storage has changed a bit
		for (i = _numObjectsInRoom; i < _numLocalObjects; i++) {
			_objs[i].obj_nr = 0;
		}
	}
	s->saveLoadArrayOf(_verbs, _numVerbs, sizeof(_verbs[0]), verbEntries);
	s->saveLoadArrayOf(vm.nest, 16, sizeof(vm.nest[0]), nestedScriptEntries);
	s->saveLoadArrayOf(_sentence, 6, sizeof(_sentence[0]), sentenceTabEntries);
	s->saveLoadArrayOf(_string, 6, sizeof(_string[0]), stringTabEntries);
	s->saveLoadArrayOf(_colorCycle, 16, sizeof(_colorCycle[0]), colorCycleEntries);

	if (savegameVersion >= VER(13))
		s->saveLoadArrayOf(_scaleSlots, 20, sizeof(_scaleSlots[0]), scaleSlotsEntries);

	// Save all resource.
	int type, idx;
	if (savegameVersion >= VER(26)) {
		// New, more robust resource save/load system. This stores the type
		// and index of each resource. Thus if we increase e.g. the maximum
		// number of script resources, savegames won't break.
		if (s->isSaving()) {
			for (type = rtFirst; type <= rtLast; type++) {
				if (res.mode[type] != 1 && type != rtTemp && type != rtBuffer) {
					s->saveUint16(type);	// Save the res type...
					for (idx = 0; idx < res.num[type]; idx++) {
						// Only save resources which actually exist...
						if (res.address[type][idx]) {
							s->saveUint16(idx);	// Save the index of the resource
							saveResource(s, type, idx);
						}
					}
					s->saveUint16(0xFFFF);	// End marker
				}
			}
			s->saveUint16(0xFFFF);	// End marker
		} else {
			while ((type = s->loadUint16()) != 0xFFFF) {
				while ((idx = s->loadUint16()) != 0xFFFF) {
					assert(0 <= idx && idx < res.num[type]);
					loadResource(s, type, idx);
				}
			}
		}
	} else {
		// Old, fragile resource save/load system. Doesn't save resources
		// with index 0, and breaks whenever we change the limit on a given
		// resource type.
		for (type = rtFirst; type <= rtLast; type++)
			if (res.mode[type] != 1)
				for (idx = 1; idx < res.num[type]; idx++)
					if (type != rtTemp && type != rtBuffer)
						saveLoadResource(s, type, idx);
	}

	s->saveLoadArrayOf(_objectOwnerTable, _numGlobalObjects, sizeof(_objectOwnerTable[0]), sleByte);
	s->saveLoadArrayOf(_objectStateTable, _numGlobalObjects, sizeof(_objectStateTable[0]), sleByte);
	if (_objectRoomTable)
		s->saveLoadArrayOf(_objectRoomTable, _numGlobalObjects, sizeof(_objectRoomTable[0]), sleByte);

	// PalManip data was not saved before V10 save games
	if (savegameVersion < VER(10))
		_palManipCounter = 0;
	if (_palManipCounter) {
		if (!_palManipPalette)
			_palManipPalette = (byte *)calloc(0x300, 1);
		if (!_palManipIntermediatePal)
			_palManipPalette = (byte *)calloc(0x300, 1);
		s->saveLoadArrayOf(_palManipPalette, 0x300, 1, sleByte);
		s->saveLoadArrayOf(_palManipIntermediatePal, 0x600, 1, sleByte);
	}

	s->saveLoadArrayOf(_classData, _numGlobalObjects, sizeof(_classData[0]), sleUint32);

	var120Backup = _scummVars[120];
	var98Backup = _scummVars[98];

	byte varVideoMode = VAR(VAR_VIDEOMODE);
	byte varMachineSpeed = VAR(VAR_MACHINE_SPEED);

	// The variables grew from 16 to 32 bit.
	if (savegameVersion < VER(15))
		s->saveLoadArrayOf(_scummVars, _numVariables, sizeof(_scummVars[0]), sleInt16);
	else
		s->saveLoadArrayOf(_scummVars, _numVariables, sizeof(_scummVars[0]), sleInt32);

	if (_gameId == GID_TENTACLE)	// Maybe misplaced, but that's the main idea
		_scummVars[120] = var120Backup;
	if (_gameId == GID_INDY4)
		_scummVars[98] = var98Backup;;

	VAR(VAR_VIDEOMODE) = varVideoMode;
	VAR(VAR_MACHINE_SPEED) = varMachineSpeed;

	s->saveLoadArrayOf(_bitVars, _numBitVariables >> 3, 1, sleByte);

	/* Save or load a list of the locked objects */
	if (s->isSaving()) {
		for (i = rtFirst; i <= rtLast; i++)
			for (j = 1; j < res.num[i]; j++) {
				if (res.flags[i][j] & RF_LOCK) {
					s->saveByte(i);
					s->saveUint16(j);
				}
			}
		s->saveByte(0xFF);
	} else {
		int r;
		while ((r = s->loadByte()) != 0xFF) {
			res.flags[r][s->loadUint16()] |= RF_LOCK;
		}
	}

	if (_imuse && (_saveSound || !_saveTemporaryState)) {
		_imuse->save_or_load(s, this);
		_imuse->setMasterVolume(GetConfig(kConfig_MasterVolume));
		_imuse->set_music_volume(GetConfig(kConfig_MusicVolume));
	}
}

void ScummEngine::saveLoadResource(Serializer *ser, int type, int idx) {
	byte *ptr;
	uint32 size;

	if (!res.mode[type]) {
		if (ser->isSaving()) {
			ptr = res.address[type][idx];
			if (ptr == NULL) {
				ser->saveUint32(0);
				return;
			}

			size = ((MemBlkHeader *)ptr)->size;

			ser->saveUint32(size);
			ser->saveBytes(ptr + sizeof(MemBlkHeader), size);

			if (type == rtInventory) {
				ser->saveUint16(_inventory[idx]);
			}
			if (type == rtObjectName && ser->getVersion() >= VER(25)) {
				ser->saveUint16(_newNames[idx]);
			}
		} else {
			size = ser->loadUint32();
			if (size) {
				createResource(type, idx, size);
				ser->loadBytes(getResourceAddress(type, idx), size);
				if (type == rtInventory) {
					_inventory[idx] = ser->loadUint16();
				}
				if (type == rtObjectName && ser->getVersion() >= VER(25)) {
					_newNames[idx] = ser->loadUint16();
				}
			}
		}
	} else if (res.mode[type] == 2 && ser->getVersion() >= VER(23)) {
		// Save/load only a list of resource numbers that need reloaded.
		if (ser->isSaving()) {
			ser->saveUint16(res.address[type][idx] ? 1 : 0);
		} else {
			if (ser->loadUint16())
				ensureResourceLoaded(type, idx);
		}
	}
}

void ScummEngine::saveResource(Serializer *ser, int type, int idx) {
	assert(res.address[type][idx]);

	if (res.mode[type] == 0) {
		byte *ptr = res.address[type][idx];
		uint32 size = ((MemBlkHeader *)ptr)->size;

		ser->saveUint32(size);
		ser->saveBytes(ptr + sizeof(MemBlkHeader), size);

		if (type == rtInventory) {
			ser->saveUint16(_inventory[idx]);
		}
		if (type == rtObjectName) {
			ser->saveUint16(_newNames[idx]);
		}
	}
}

void ScummEngine::loadResource(Serializer *ser, int type, int idx) {
	if (res.mode[type] == 0) {
		uint32 size = ser->loadUint32();
		assert(size);
		createResource(type, idx, size);
		ser->loadBytes(getResourceAddress(type, idx), size);

		if (type == rtInventory) {
			_inventory[idx] = ser->loadUint16();
		}
		if (type == rtObjectName) {
			_newNames[idx] = ser->loadUint16();
		}
	} else if (res.mode[type] == 2) {
		ensureResourceLoaded(type, idx);
	}
}

void Serializer::saveBytes(void *b, int len) {
	_saveLoadStream->write(b, len);
}

void Serializer::loadBytes(void *b, int len) {
	_saveLoadStream->read(b, len);
}

void Serializer::saveUint32(uint32 d) {
	_saveLoadStream->writeUint32(d);
}

void Serializer::saveUint16(uint16 d) {
	_saveLoadStream->writeUint16(d);
}

void Serializer::saveByte(byte b) {
	_saveLoadStream->writeByte(b);
}

uint32 Serializer::loadUint32() {
	return _saveLoadStream->readUint32();
}

uint16 Serializer::loadUint16() {
	return _saveLoadStream->readUint16();
}

byte Serializer::loadByte() {
	return _saveLoadStream->readByte();
}

void Serializer::saveArrayOf(void *b, int len, int datasize, byte filetype) {
	byte *at = (byte *)b;
	uint32 data;

	// speed up byte arrays
	if (datasize == 1 && filetype == sleByte) {
		saveBytes(b, len);
		return;
	}

	while (--len >= 0) {
		if (datasize == 0) {
			// Do nothing for obsolete data
			data = 0;
		} else if (datasize == 1) {
			data = *(byte *)at;
			at += 1;
		} else if (datasize == 2) {
			data = *(uint16 *)at;
			at += 2;
		} else if (datasize == 4) {
			data = *(uint32 *)at;
			at += 4;
		} else {
			error("saveLoadArrayOf: invalid size %d", datasize);
		}
		switch (filetype) {
		case sleByte:
			saveByte((byte)data);
			break;
		case sleUint16:
		case sleInt16:
			saveUint16((int16)data);
			break;
		case sleInt32:
		case sleUint32:
			saveUint32(data);
			break;
		default:
			error("saveLoadArrayOf: invalid filetype %d", filetype);
		}
	}
}

void Serializer::loadArrayOf(void *b, int len, int datasize, byte filetype) {
	byte *at = (byte *)b;
	uint32 data;

	// speed up byte arrays
	if (datasize == 1 && filetype == sleByte) {
		loadBytes(b, len);
		return;
	}

	while (--len >= 0) {
		switch (filetype) {
		case sleByte:
			data = loadByte();
			break;
		case sleUint16:
			data = loadUint16();
			break;
		case sleInt16:
			data = (int16)loadUint16();
			break;
		case sleUint32:
			data = loadUint32();
			break;
		case sleInt32:
			data = (int32)loadUint32();
			break;
		default:
			error("saveLoadArrayOf: invalid filetype %d", filetype);
		}
		if (datasize == 0) {
			// Do nothing for obsolete data
		} else if (datasize == 1) {
			*(byte *)at = (byte)data;
			at += 1;
		} else if (datasize == 2) {
			*(uint16 *)at = (uint16)data;
			at += 2;
		} else if (datasize == 4) {
			*(uint32 *)at = data;
			at += 4;
		} else {
			error("saveLoadArrayOf: invalid size %d", datasize);
		}
	}
}

void Serializer::saveLoadArrayOf(void *b, int num, int datasize, const SaveLoadEntry *sle) {
	byte *data = (byte *)b;

	if (isSaving()) {
		while (--num >= 0) {
			saveEntries(data, sle);
			data += datasize;
		}
	} else {
		while (--num >= 0) {
			loadEntries(data, sle);
			data += datasize;
		}
	}
}

void Serializer::saveLoadArrayOf(void *b, int len, int datasize, byte filetype) {
	if (isSaving())
		saveArrayOf(b, len, datasize, filetype);
	else
		loadArrayOf(b, len, datasize, filetype);
}

void Serializer::saveLoadEntries(void *d, const SaveLoadEntry *sle) {
	if (isSaving())
		saveEntries(d, sle);
	else
		loadEntries(d, sle);
}

void Serializer::saveEntries(void *d, const SaveLoadEntry *sle) {
	byte type;
	byte *at;
	int size;

	while (sle->offs != 0xFFFF) {
		at = (byte *)d + sle->offs;
		size = sle->size;
		type = (byte) sle->type;

		if (sle->maxVersion != CURRENT_VER) {
			// Skip obsolete entries
			if (type & 128)
				sle++;
		} else if (size == 0xFF) {
			// save reference
			void *ptr = *((void **)at);
			saveUint16(ptr ? ((*_save_ref) (_ref_me, type, ptr) + 1) : 0);
		} else {
			// save entry
			int columns = 1;
			int rows = 1;
			int rowlen = 0;
			if (type & 128) {
				sle++;
				columns = sle->offs;
				rows = sle->type;
				rowlen = sle->size;
				type &= ~128;
			}
			while (rows--) {
				saveArrayOf(at, columns, size, type);
				at += rowlen;
			}
		}
		sle++;
	}
}

void Serializer::loadEntries(void *d, const SaveLoadEntry *sle) {
	byte type;
	byte *at;
	int size;

	while (sle->offs != 0xFFFF) {
		at = (byte *)d + sle->offs;
		size = sle->size;
		type = (byte) sle->type;

		if (_savegameVersion < sle->minVersion || _savegameVersion > sle->maxVersion) {
			// Skip entries which are not present in this save game version
			if (type & 128)
				sle++;
		} else if (size == 0xFF) {
			// load reference...
			int num = loadUint16();
			// ...but only use it if it's still there in CURRENT_VER
			if (sle->maxVersion == CURRENT_VER)
				*((void **)at) = num ? (*_load_ref) (_ref_me, type, num - 1) : NULL;
		} else {
			// load entry
			int columns = 1;
			int rows = 1;
			int rowlen = 0;

			if (type & 128) {
				sle++;
				columns = sle->offs;
				rows = sle->type;
				rowlen = sle->size;
				type &= ~128;
			}
			while (rows--) {
				loadArrayOf(at, columns, size, type);
				at += rowlen;
			}
		}
		sle++;
	}
}

} // End of namespace Scumm
