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
 * $Header: /cvsroot/scummvm/scummvm/scumm/script_v5.cpp,v 1.226 2004/01/31 22:11:01 fingolfin Exp $
 *
 */

#include "stdafx.h"

#include "scumm/actor.h"
#include "scumm/charset.h"
#include "scumm/intern.h"
#include "scumm/object.h"
#include "scumm/scumm.h"
#include "scumm/sound.h"
#include "scumm/verbs.h"

namespace Scumm {

#ifndef RELEASEBUILD
#define OPCODE(x)	{ &ScummEngine_v5::x, #x }
#else
#define OPCODE(x)	{ &ScummEngine_v5::x }
#endif

void ScummEngine_v5::setupOpcodes() {
	static const OpcodeEntryV5 opcodes[256] = {
		/* 00 */
		OPCODE(o5_stopObjectCode),
		OPCODE(o5_putActor),
		OPCODE(o5_startMusic),
		OPCODE(o5_getActorRoom),
		/* 04 */
		OPCODE(o5_isGreaterEqual),
		OPCODE(o5_drawObject),
		OPCODE(o5_getActorElevation),
		OPCODE(o5_setState),
		/* 08 */
		OPCODE(o5_isNotEqual),
		OPCODE(o5_faceActor),
		OPCODE(o5_startScript),
		OPCODE(o5_getVerbEntrypoint),
		/* 0C */
		OPCODE(o5_resourceRoutines),
		OPCODE(o5_walkActorToActor),
		OPCODE(o5_putActorAtObject),
		OPCODE(o5_getObjectState),
		/* 10 */
		OPCODE(o5_getObjectOwner),
		OPCODE(o5_animateActor),
		OPCODE(o5_panCameraTo),
		OPCODE(o5_actorOps),
		/* 14 */
		OPCODE(o5_print),
		OPCODE(o5_actorFromPos),
		OPCODE(o5_getRandomNr),
		OPCODE(o5_and),
		/* 18 */
		OPCODE(o5_jumpRelative),
		OPCODE(o5_doSentence),
		OPCODE(o5_move),
		OPCODE(o5_multiply),
		/* 1C */
		OPCODE(o5_startSound),
		OPCODE(o5_ifClassOfIs),
		OPCODE(o5_walkActorTo),
		OPCODE(o5_isActorInBox),
		/* 20 */
		OPCODE(o5_stopMusic),
		OPCODE(o5_putActor),
		OPCODE(o5_getAnimCounter),
		OPCODE(o5_getActorY),
		/* 24 */
		OPCODE(o5_loadRoomWithEgo),
		OPCODE(o5_pickupObject),
		OPCODE(o5_setVarRange),
		OPCODE(o5_stringOps),
		/* 28 */
		OPCODE(o5_equalZero),
		OPCODE(o5_setOwnerOf),
		OPCODE(o5_startScript),
		OPCODE(o5_delayVariable),
		/* 2C */
		OPCODE(o5_cursorCommand),
		OPCODE(o5_putActorInRoom),
		OPCODE(o5_delay),
		OPCODE(o5_ifNotState),
		/* 30 */
		OPCODE(o5_matrixOps),
		OPCODE(o5_getInventoryCount),
		OPCODE(o5_setCameraAt),
		OPCODE(o5_roomOps),
		/* 34 */
		OPCODE(o5_getDist),
		OPCODE(o5_findObject),
		OPCODE(o5_walkActorToObject),
		OPCODE(o5_startObject),
		/* 38 */
		OPCODE(o5_lessOrEqual),
		OPCODE(o5_doSentence),
		OPCODE(o5_subtract),
		OPCODE(o5_getActorScale),
		/* 3C */
		OPCODE(o5_stopSound),
		OPCODE(o5_findInventory),
		OPCODE(o5_walkActorTo),
		OPCODE(o5_drawBox),
		/* 40 */
		OPCODE(o5_cutscene),
		OPCODE(o5_putActor),
		OPCODE(o5_chainScript),
		OPCODE(o5_getActorX),
		/* 44 */
		OPCODE(o5_isLess),
		OPCODE(o5_drawObject),
		OPCODE(o5_increment),
		OPCODE(o5_setState),
		/* 48 */
		OPCODE(o5_isEqual),
		OPCODE(o5_faceActor),
		OPCODE(o5_startScript),
		OPCODE(o5_getVerbEntrypoint),
		/* 4C */
		OPCODE(o5_soundKludge),
		OPCODE(o5_walkActorToActor),
		OPCODE(o5_putActorAtObject),
		OPCODE(o5_ifState),
		/* 50 */
		OPCODE(o5_pickupObjectOld),
		OPCODE(o5_animateActor),
		OPCODE(o5_actorFollowCamera),
		OPCODE(o5_actorOps),
		/* 54 */
		OPCODE(o5_setObjectName),
		OPCODE(o5_actorFromPos),
		OPCODE(o5_getActorMoving),
		OPCODE(o5_or),
		/* 58 */
		OPCODE(o5_beginOverride),
		OPCODE(o5_doSentence),
		OPCODE(o5_add),
		OPCODE(o5_divide),
		/* 5C */
		OPCODE(o5_oldRoomEffect),
		OPCODE(o5_setClass),
		OPCODE(o5_walkActorTo),
		OPCODE(o5_isActorInBox),
		/* 60 */
		OPCODE(o5_freezeScripts),
		OPCODE(o5_putActor),
		OPCODE(o5_stopScript),
		OPCODE(o5_getActorFacing),
		/* 64 */
		OPCODE(o5_loadRoomWithEgo),
		OPCODE(o5_pickupObject),
		OPCODE(o5_getClosestObjActor),
		OPCODE(o5_getStringWidth),
		/* 68 */
		OPCODE(o5_isScriptRunning),
		OPCODE(o5_setOwnerOf),
		OPCODE(o5_startScript),
		OPCODE(o5_debug),
		/* 6C */
		OPCODE(o5_getActorWidth),
		OPCODE(o5_putActorInRoom),
		OPCODE(o5_stopObjectScript),
		OPCODE(o5_ifNotState),
		/* 70 */
		OPCODE(o5_lights),
		OPCODE(o5_getActorCostume),
		OPCODE(o5_loadRoom),
		OPCODE(o5_roomOps),
		/* 74 */
		OPCODE(o5_getDist),
		OPCODE(o5_findObject),
		OPCODE(o5_walkActorToObject),
		OPCODE(o5_startObject),
		/* 78 */
		OPCODE(o5_isGreater),
		OPCODE(o5_doSentence),
		OPCODE(o5_verbOps),
		OPCODE(o5_getActorWalkBox),
		/* 7C */
		OPCODE(o5_isSoundRunning),
		OPCODE(o5_findInventory),
		OPCODE(o5_walkActorTo),
		OPCODE(o5_drawBox),
		/* 80 */
		OPCODE(o5_breakHere),
		OPCODE(o5_putActor),
		OPCODE(o5_startMusic),
		OPCODE(o5_getActorRoom),
		/* 84 */
		OPCODE(o5_isGreaterEqual),
		OPCODE(o5_drawObject),
		OPCODE(o5_getActorElevation),
		OPCODE(o5_setState),
		/* 88 */
		OPCODE(o5_isNotEqual),
		OPCODE(o5_faceActor),
		OPCODE(o5_startScript),
		OPCODE(o5_getVerbEntrypoint),
		/* 8C */
		OPCODE(o5_resourceRoutines),
		OPCODE(o5_walkActorToActor),
		OPCODE(o5_putActorAtObject),
		OPCODE(o5_getObjectState),
		/* 90 */
		OPCODE(o5_getObjectOwner),
		OPCODE(o5_animateActor),
		OPCODE(o5_panCameraTo),
		OPCODE(o5_actorOps),
		/* 94 */
		OPCODE(o5_print),
		OPCODE(o5_actorFromPos),
		OPCODE(o5_getRandomNr),
		OPCODE(o5_and),
		/* 98 */
		OPCODE(o5_quitPauseRestart),
		OPCODE(o5_doSentence),
		OPCODE(o5_move),
		OPCODE(o5_multiply),
		/* 9C */
		OPCODE(o5_startSound),
		OPCODE(o5_ifClassOfIs),
		OPCODE(o5_walkActorTo),
		OPCODE(o5_isActorInBox),
		/* A0 */
		OPCODE(o5_stopObjectCode),
		OPCODE(o5_putActor),
		OPCODE(o5_getAnimCounter),
		OPCODE(o5_getActorY),
		/* A4 */
		OPCODE(o5_loadRoomWithEgo),
		OPCODE(o5_pickupObject),
		OPCODE(o5_setVarRange),
		OPCODE(o5_saveLoadVars),
		/* A8 */
		OPCODE(o5_notEqualZero),
		OPCODE(o5_setOwnerOf),
		OPCODE(o5_startScript),
		OPCODE(o5_saveRestoreVerbs),
		/* AC */
		OPCODE(o5_expression),
		OPCODE(o5_putActorInRoom),
		OPCODE(o5_wait),
		OPCODE(o5_ifNotState),
		/* B0 */
		OPCODE(o5_matrixOps),
		OPCODE(o5_getInventoryCount),
		OPCODE(o5_setCameraAt),
		OPCODE(o5_roomOps),
		/* B4 */
		OPCODE(o5_getDist),
		OPCODE(o5_findObject),
		OPCODE(o5_walkActorToObject),
		OPCODE(o5_startObject),
		/* B8 */
		OPCODE(o5_lessOrEqual),
		OPCODE(o5_doSentence),
		OPCODE(o5_subtract),
		OPCODE(o5_getActorScale),
		/* BC */
		OPCODE(o5_stopSound),
		OPCODE(o5_findInventory),
		OPCODE(o5_walkActorTo),
		OPCODE(o5_drawBox),
		/* C0 */
		OPCODE(o5_endCutscene),
		OPCODE(o5_putActor),
		OPCODE(o5_chainScript),
		OPCODE(o5_getActorX),
		/* C4 */
		OPCODE(o5_isLess),
		OPCODE(o5_drawObject),
		OPCODE(o5_decrement),
		OPCODE(o5_setState),
		/* C8 */
		OPCODE(o5_isEqual),
		OPCODE(o5_faceActor),
		OPCODE(o5_startScript),
		OPCODE(o5_getVerbEntrypoint),
		/* CC */
		OPCODE(o5_pseudoRoom),
		OPCODE(o5_walkActorToActor),
		OPCODE(o5_putActorAtObject),
		OPCODE(o5_ifState),
		/* D0 */
		OPCODE(o5_pickupObjectOld),
		OPCODE(o5_animateActor),
		OPCODE(o5_actorFollowCamera),
		OPCODE(o5_actorOps),
		/* D4 */
		OPCODE(o5_setObjectName),
		OPCODE(o5_actorFromPos),
		OPCODE(o5_getActorMoving),
		OPCODE(o5_or),
		/* D8 */
		OPCODE(o5_printEgo),
		OPCODE(o5_doSentence),
		OPCODE(o5_add),
		OPCODE(o5_divide),
		/* DC */
		OPCODE(o5_oldRoomEffect),
		OPCODE(o5_setClass),
		OPCODE(o5_walkActorTo),
		OPCODE(o5_isActorInBox),
		/* E0 */
		OPCODE(o5_freezeScripts),
		OPCODE(o5_putActor),
		OPCODE(o5_stopScript),
		OPCODE(o5_getActorFacing),
		/* E4 */
		OPCODE(o5_loadRoomWithEgo),
		OPCODE(o5_pickupObject),
		OPCODE(o5_getClosestObjActor),
		OPCODE(o5_getStringWidth),
		/* E8 */
		OPCODE(o5_isScriptRunning),
		OPCODE(o5_setOwnerOf),
		OPCODE(o5_startScript),
		OPCODE(o5_debug),
		/* EC */
		OPCODE(o5_getActorWidth),
		OPCODE(o5_putActorInRoom),
		OPCODE(o5_stopObjectScript),
		OPCODE(o5_ifNotState),
		/* F0 */
		OPCODE(o5_lights),
		OPCODE(o5_getActorCostume),
		OPCODE(o5_loadRoom),
		OPCODE(o5_roomOps),
		/* F4 */
		OPCODE(o5_getDist),
		OPCODE(o5_findObject),
		OPCODE(o5_walkActorToObject),
		OPCODE(o5_startObject),
		/* F8 */
		OPCODE(o5_isGreater),
		OPCODE(o5_doSentence),
		OPCODE(o5_verbOps),
		OPCODE(o5_getActorWalkBox),
		/* FC */
		OPCODE(o5_isSoundRunning),
		OPCODE(o5_findInventory),
		OPCODE(o5_walkActorTo),
		OPCODE(o5_drawBox)
	};

	_opcodesV5 = opcodes;
}

#define PARAM_1 0x80
#define PARAM_2 0x40
#define PARAM_3 0x20

void ScummEngine_v5::executeOpcode(byte i) {
	OpcodeProcV5 op = _opcodesV5[i].proc;
	(this->*op) ();
}

#ifndef RELEASEBUILD
const char *ScummEngine_v5::getOpcodeDesc(byte i) {
	return _opcodesV5[i].desc;
}
#endif

int ScummEngine_v5::getVar() {
	return readVar((int)fetchScriptWord());
}

int ScummEngine_v5::getVarOrDirectByte(byte mask) {
	if (_opcode & mask)
		return getVar();
	return fetchScriptByte();
}

int ScummEngine_v5::getVarOrDirectWord(byte mask) {
/*
	int w = fetchScriptWord();
	debug(9, "GetVarOrDirectword: %d, %d, %d", _opcode, mask, w);
	if (_opcode & mask)
		return readVar(w);
	return w;
*/

	if (_opcode & mask)
		return getVar();
	return (int16)fetchScriptWord();

}

void ScummEngine_v5::o5_actorFollowCamera() {
	actorFollowCamera(getVarOrDirectByte(0x80));
}

void ScummEngine_v5::o5_actorFromPos() {
	int x, y;
	getResultPos();
	x = getVarOrDirectWord(PARAM_1);
	y = getVarOrDirectWord(PARAM_2);
	setResult(getActorFromPos(x, y));
}

void ScummEngine_v5::o5_actorOps() {
	int act = getVarOrDirectByte(PARAM_1);
	Actor *a = derefActor(act, "o5_actorOps");
	int i, j;

	while ((_opcode = fetchScriptByte()) != 0xFF) {
		switch (_opcode & 0x1F) {
		case 0:										/* dummy case */
			getVarOrDirectByte(PARAM_1);
			break;
		case 1:			// SO_COSTUME
			a->setActorCostume(getVarOrDirectByte(PARAM_1));
			break;
		case 2:			// SO_STEP_DIST
			i = getVarOrDirectByte(PARAM_1);
			j = getVarOrDirectByte(PARAM_2);
			a->setActorWalkSpeed(i, j);
			break;
		case 3:			// SO_SOUND
			a->sound[0] = getVarOrDirectByte(PARAM_1);
			break;
		case 4:			// SO_WALK_ANIMATION
			a->walkFrame = getVarOrDirectByte(PARAM_1);
			break;
		case 5:			// SO_TALK_ANIMATION
			a->talkStartFrame = getVarOrDirectByte(PARAM_1);
			a->talkStopFrame = getVarOrDirectByte(PARAM_2);
			break;
		case 6:			// SO_STAND_ANIMATION
			a->standFrame = getVarOrDirectByte(PARAM_1);
			break;
		case 7:			// SO_ANIMATION
			getVarOrDirectByte(PARAM_1);
			getVarOrDirectByte(PARAM_2);
			getVarOrDirectByte(PARAM_3);
			break;
		case 8:			// SO_DEFAULT
			a->initActor(0);
			break;
		case 9:			// SO_ELEVATION
			a->setElevation(getVarOrDirectWord(PARAM_1));
			break;
		case 10:		// SO_ANIMATION_DEFAULT
			a->initFrame = 1;
			a->walkFrame = 2;
			a->standFrame = 3;
			a->talkStartFrame = 4;
			a->talkStopFrame = 5;
			break;
		case 11:		// SO_PALETTE
			i = getVarOrDirectByte(PARAM_1);
			j = getVarOrDirectByte(PARAM_2);
			checkRange(31, 0, i, "Illegal palette slot %d");
			a->setPalette(i, j);
			break;
		case 12:		// SO_TALK_COLOR

			// Zak256 (and possibly other games) uses actor 0 to
			// indicate that it's the default talk color that is
			// to be changed.

			if (act == 0)
				_string[0].color = getVarOrDirectByte(PARAM_1);
			else
				a->talkColor = getVarOrDirectByte(PARAM_1);
			break;
		case 13:		// SO_ACTOR_NAME
			loadPtrToResource(rtActorName, a->number, NULL);
			break;
		case 14:		// SO_INIT_ANIMATION
			a->initFrame = getVarOrDirectByte(PARAM_1);
			break;
		case 15:		// SO_PALETTE_LIST
			error("o5_actorOps:unk not implemented");
#if 0
			int args[16] =
				{
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				};
			getWordVararg(args);
			for (i = 0; i < 16; i++)
				if (args[i] != 0xFF)
					a->palette[i] = args[i];
#endif
			break;
		case 16:		// SO_ACTOR_WIDTH
			a->width = getVarOrDirectByte(PARAM_1);
			break;
		case 17:		// SO_ACTOR_SCALE
			i = getVarOrDirectByte(PARAM_1);
			j = getVarOrDirectByte(PARAM_2);
			a->setScale(i, j);
			break;
		case 18:		// SO_NEVER_ZCLIP
			a->forceClip = 0;
			break;
		case 19:		// SO_ALWAYS_ZCLIP
			a->forceClip = getVarOrDirectByte(PARAM_1);
			break;
		case 20:		// SO_IGNORE_BOXES
		case 21:		// SO_FOLLOW_BOXES
			a->ignoreBoxes = !(_opcode & 1);
			a->forceClip = 0;
			if (a->isInCurrentRoom())
				a->putActor(a->_pos.x, a->_pos.y, a->room);
			break;

		case 22:		// SO_ANIMATION_SPEED
			a->setAnimSpeed(getVarOrDirectByte(PARAM_1));
			break;
		case 23:		// SO_SHADOW
			i = getVarOrDirectByte(PARAM_1);
			//a->shadow_mode = i;
			break;
		default:
			warning("o5_actorOps: default case");
		}
	}
}

void ScummEngine_v5::o5_setClass() {
	int obj = getVarOrDirectWord(PARAM_1);
	int newClass;

	while ((_opcode = fetchScriptByte()) != 0xFF) {
		newClass = getVarOrDirectWord(PARAM_1);
		if (newClass == 0) {
			// Class '0' means: clean all class data
			_classData[obj] = 0;
		} else
			putClass(obj, newClass, (newClass & 0x80) ? true : false);
	}
}

void ScummEngine_v5::o5_add() {
	int a;
	getResultPos();
	a = getVarOrDirectWord(PARAM_1);
	setResult(readVar(_resultVarNumber) + a);
}

void ScummEngine_v5::o5_and() {
	int a;
	getResultPos();
	a = getVarOrDirectWord(PARAM_1);
	setResult(readVar(_resultVarNumber) & a);
}

void ScummEngine_v5::o5_animateActor() {
	int act = getVarOrDirectByte(PARAM_1);
	int anim = getVarOrDirectByte(PARAM_2);

	// WORKAROUND bug #820357: This seems to be yet another script bug which
	// the original engine let slip by. For details, refer to the tracker item.
	if (_gameId == GID_INDY4 && vm.slot[_currentScript].number == 206 && _currentRoom == 17 && (act == 31 || act == 86)) {
		return;
	}
	
	Actor *a = derefActor(act, "o5_animateActor");
	a->animateActor(anim);
}

void ScummEngine_v5::o5_breakHere() {
	updateScriptPtr();
	_currentScript = 0xFF;
}

void ScummEngine_v5::o5_chainScript() {
	int vars[16];
	int script;
	int cur;

	script = getVarOrDirectByte(PARAM_1);

	getWordVararg(vars);

	cur = _currentScript;

	vm.slot[cur].number = 0;
	vm.slot[cur].status = ssDead;
	_currentScript = 0xFF;

	runScript(script, vm.slot[cur].freezeResistant, vm.slot[cur].recursive, vars);
}

void ScummEngine_v5::o5_cursorCommand() {
	int i, j, k;
	int table[16];
	switch ((_opcode = fetchScriptByte()) & 0x1F) {
	case 1:			// SO_CURSOR_ON
		_cursor.state = 1;
		verbMouseOver(0);
		break;
	case 2:			// SO_CURSOR_OFF
		_cursor.state = 0;
		verbMouseOver(0);
		break;
	case 3:			// SO_USERPUT_ON
		_userPut = 1;
		break;
	case 4:			// SO_USERPUT_OFF
		_userPut = 0;
		break;
	case 5:			// SO_CURSOR_SOFT_ON
		_cursor.state++;
		verbMouseOver(0);
		break;
	case 6:			// SO_CURSOR_SOFT_OFF
		_cursor.state--;
		verbMouseOver(0);
		break;
	case 7:			// SO_USERPUT_SOFT_ON
		_userPut++;
		break;
	case 8:			// SO_USERPUT_SOFT_OFF
		_userPut--;
		break;
	case 10:		// SO_CURSOR_IMAGE
		i = getVarOrDirectByte(PARAM_1);
		j = getVarOrDirectByte(PARAM_2);
		// cursor image in both Looms is based on image from charset
		// omit for now.
		// FIXME: Actually: is this opcode ever called by a non-Loom game?
		// Which V3-V5 game besides Loom makes use of custom cursors, ever?
		warning("setCursorImg called - tell Fingolfin where you saw this!");
		setCursorImg(i, j, 1);
		break;
	case 11:		// SO_CURSOR_HOTSPOT
		i = getVarOrDirectByte(PARAM_1);
		j = getVarOrDirectByte(PARAM_2);
		k = getVarOrDirectByte(PARAM_3);
		setCursorHotspot(j, k);
		break;
	case 12:		// SO_CURSOR_SET
		setCursor(getVarOrDirectByte(PARAM_1));
		break;
	case 13:		// SO_CHARSET_SET
		initCharset(getVarOrDirectByte(PARAM_1));
		break;
	case 14:											/* unk */
		getWordVararg(table);
		for (i = 0; i < 16; i++)
		{
			byte c = (byte)table[i];
			_charsetColorMap[i] = _charsetData[_string[1].t_charset][i] = c;
		}
		break;
	}

	VAR(VAR_CURSORSTATE) = _cursor.state;
	VAR(VAR_USERPUT) = _userPut;
}

void ScummEngine_v5::o5_cutscene() {
	int args[16];
	getWordVararg(args);
	beginCutscene(args);
}

void ScummEngine_v5::o5_endCutscene() {
	endCutscene();
}

void ScummEngine_v5::o5_debug() {
	int a = getVarOrDirectWord(PARAM_1);
	debug(1, "o5_debug(%d)", a);
}

void ScummEngine_v5::o5_decrement() {
	getResultPos();
	setResult(readVar(_resultVarNumber) - 1);
}

void ScummEngine_v5::o5_delay() {
	int delay = fetchScriptByte();
	delay |= fetchScriptByte() << 8;
	delay |= fetchScriptByte() << 16;
	vm.slot[_currentScript].delay = delay;
	vm.slot[_currentScript].status = ssPaused;
	o5_breakHere();
}

void ScummEngine_v5::o5_delayVariable() {
	vm.slot[_currentScript].delay = getVar();
	vm.slot[_currentScript].status = ssPaused;
	o5_breakHere();
}

void ScummEngine_v5::o5_divide() {
	int a;
	getResultPos();
	a = getVarOrDirectWord(PARAM_1);
	if (a == 0) {
		error("Divide by zero");
		setResult(0);
	} else
		setResult(readVar(_resultVarNumber) / a);
}

void ScummEngine_v5::o5_doSentence() {
	int verb;
	SentenceTab *st;

	verb = getVarOrDirectByte(PARAM_1);
	if (verb == 0xFE) {
		_sentenceNum = 0;
		stopScript(VAR(VAR_SENTENCE_SCRIPT));
		clearClickedStatus();
		return;
	}

	st = &_sentence[_sentenceNum++];

	st->verb = verb;
	st->objectA = getVarOrDirectWord(PARAM_2);
	st->objectB = getVarOrDirectWord(PARAM_3);
	st->preposition = (st->objectB != 0);
	st->freezeCount = 0;
}

void ScummEngine_v5::o5_drawBox() {
	int x, y, x2, y2, color;

	x = getVarOrDirectWord(PARAM_1);
	y = getVarOrDirectWord(PARAM_2);

	_opcode = fetchScriptByte();
	x2 = getVarOrDirectWord(PARAM_1);
	y2 = getVarOrDirectWord(PARAM_2);
	color = getVarOrDirectByte(PARAM_3);

	drawBox(x, y, x2, y2, color);
}

void ScummEngine_v5::o5_drawObject() {
	int state, obj, idx, i;
	ObjectData *od;
	uint16 x, y, w, h;
	int xpos, ypos;

	state = 1;
	xpos = ypos = 255;
	obj = getVarOrDirectWord(PARAM_1);

	_opcode = fetchScriptByte();
	switch (_opcode & 0x1F) {
	case 1:										/* draw at */
		xpos = getVarOrDirectWord(PARAM_1);
		ypos = getVarOrDirectWord(PARAM_2);
		break;
	case 2:										/* set state */
		state = getVarOrDirectWord(PARAM_1);
		break;
	case 0x1F:									/* neither */
		break;
	default:
		error("o5_drawObject: unknown subopcode %d", _opcode & 0x1F);
	}

	idx = getObjectIndex(obj);
	if (idx == -1)
		return;

	od = &_objs[idx];
	if (xpos != 0xFF) {
		od->walk_x += (xpos * 8) - od->x_pos;
		od->x_pos = xpos * 8;
		od->walk_y += (ypos * 8) - od->y_pos;
		od->y_pos = ypos * 8;
	}
	addObjectToDrawQue(idx);

	x = od->x_pos;
	y = od->y_pos;
	w = od->width;
	h = od->height;

	i = _numLocalObjects - 1;
	do {
		if (_objs[i].obj_nr && _objs[i].x_pos == x && _objs[i].y_pos == y && _objs[i].width == w && _objs[i].height == h)
			putState(_objs[i].obj_nr, 0);
	} while (--i);

	putState(obj, state);
}

void ScummEngine_v5::o5_getStringWidth() {
	// TODO - not sure if this is correct... needs testing
	int string, width = 0;
	byte *ptr;
	
	getResultPos();
	string = getVarOrDirectByte(PARAM_1);
	ptr = getResourceAddress(rtString, string);
	assert(ptr);

	width = _charset->getStringWidth(0, ptr);
	
	setResult(width);
	warning("o5_getStringWidth, result %d", width);
}

void ScummEngine_v5::o5_saveLoadVars() {
	// TODO
	if (fetchScriptByte() == 1)
		saveVars();
	else
		loadVars();
}

void ScummEngine_v5::saveVars() {
	int a, b;

	while ((_opcode = fetchScriptByte()) != 0) {
		switch (_opcode & 0x1F) {
		case 0x01: // write a range of variables
			getResultPos();
		        a = _resultVarNumber;
			getResultPos();
		        b = _resultVarNumber;
			warning("stub saveVars: vars %d -> %d", a, b);
			break;
		case 0x02: // write a range of string variables
			a = getVarOrDirectByte(PARAM_1);
			b = getVarOrDirectByte(PARAM_2);
			warning("stub saveVars: strings %d -> %d", a, b);
			break;
		case 0x03: // open file
			a = resStrLen(_scriptPointer);
			warning("stub saveVars to %s", _scriptPointer);
			_scriptPointer += a + 1;
			break;
		case 0x04:
			return;
			break;
		case 0x1F: // close file
			warning("stub saveVars close file");
			return;
			break;
		}
	}
}

void ScummEngine_v5::loadVars() {
	int a, b;

	while ((_opcode = fetchScriptByte()) != 0) {
		switch (_opcode & 0x1F) {
		case 0x01: // read a range of variables
			getResultPos();
		        a = _resultVarNumber;
			getResultPos();
		        b = _resultVarNumber;
			warning("stub loadVars: vars %d -> %d", a, b);
			break;
		case 0x02: // read a range of string variables
			a = getVarOrDirectByte(0x80);
			b = getVarOrDirectByte(0x40);
			warning("stub loadVars: strings %d -> %d", a, b);
			break;
		case 0x03: // open file
			a = resStrLen(_scriptPointer);
			warning("stub loadVars from %s", _scriptPointer);
			_scriptPointer += a + 1;
			break;
		case 0x04:
			return;
			break;
		case 0x1F: // close file
			warning("stub loadVars close file");
			return;
			break;
		}
	}
}

void ScummEngine_v5::o5_expression() {
	int dst, i;

	_scummStackPos = 0;
	getResultPos();
	dst = _resultVarNumber;

	while ((_opcode = fetchScriptByte()) != 0xFF) {
		switch (_opcode & 0x1F) {
		case 1:										/* varordirect */
			push(getVarOrDirectWord(PARAM_1));
			break;
		case 2:										/* add */
			i = pop();
			push(i + pop());
			break;
		case 3:										/* sub */
			i = pop();
			push(pop() - i);
			break;
		case 4:										/* mul */
			i = pop();
			push(i * pop());
			break;
		case 5:										/* div */
			i = pop();
			if (i == 0)
				error("Divide by zero");
			push(pop() / i);
			break;
		case 6:										/* normal opcode */
			_opcode = fetchScriptByte();
			executeOpcode(_opcode);
			push(_scummVars[0]);
			break;
		}
	}

	_resultVarNumber = dst;
	setResult(pop());
}

void ScummEngine_v5::o5_faceActor() {
	int act = getVarOrDirectByte(PARAM_1);
	int obj = getVarOrDirectWord(PARAM_2);
	Actor *a = derefActor(act, "o5_faceActor");
	a->faceToObject(obj);
}

void ScummEngine_v5::o5_findInventory() {
	getResultPos();
	int x = getVarOrDirectByte(PARAM_1);
	int y = getVarOrDirectByte(PARAM_2);
	setResult(findInventory(x, y));
}

void ScummEngine_v5::o5_findObject() {
	getResultPos();
	int x = getVarOrDirectByte(PARAM_1);
	int y = getVarOrDirectByte(PARAM_2);
	setResult(findObject(x, y));
}

void ScummEngine_v5::o5_freezeScripts() {
	int scr = getVarOrDirectByte(PARAM_1);

	if (scr != 0)
		freezeScripts(scr);
	else
		unfreezeScripts();
}

void ScummEngine_v5::o5_getActorCostume() {
	getResultPos();
	int act = getVarOrDirectByte(PARAM_1);
	Actor *a = derefActor(act, "o5_getActorCostume");
	setResult(a->costume);
}

void ScummEngine_v5::o5_getActorElevation() {
	getResultPos();
	int act = getVarOrDirectByte(PARAM_1);
	Actor *a = derefActor(act, "o5_getActorElevation");
	setResult(a->getElevation());
}

void ScummEngine_v5::o5_getActorFacing() {
	getResultPos();
	int act = getVarOrDirectByte(PARAM_1);
	Actor *a = derefActor(act, "o5_getActorFacing");
	setResult(newDirToOldDir(a->getFacing()));
}

void ScummEngine_v5::o5_getActorMoving() {
	getResultPos();
	int act = getVarOrDirectByte(PARAM_1);
	Actor *a = derefActor(act, "o5_getActorMoving");
	setResult(a->moving);
}

void ScummEngine_v5::o5_getActorRoom() {
	getResultPos();
	int act = getVarOrDirectByte(PARAM_1);
	// WORKAROUND bug #746349. This is a really odd bug in either the script
	// or in our script engine. Might be a good idea to investigate this
	// further by e.g. looking at the FOA engine a bit closer.
	if (_gameId == GID_INDY4 && _roomResource == 94 && vm.slot[_currentScript].number == 206 && act > _numActors) {
		setResult(0);
		return;
	}
	
	Actor *a = derefActor(act, "o5_getActorRoom");
	setResult(a->room);
}

void ScummEngine_v5::o5_getActorScale() {
	Actor *a;
	getResultPos();
	int act = getVarOrDirectByte(PARAM_1);
	a = derefActor(act, "o5_getActorScale");
	setResult(a->scalex);
}

void ScummEngine_v5::o5_getActorWalkBox() {
	getResultPos();
	int act = getVarOrDirectByte(PARAM_1);
	Actor *a = derefActor(act, "o5_getActorWalkBox");
	setResult(a->walkbox);
}

void ScummEngine_v5::o5_getActorWidth() {
	getResultPos();
	int act = getVarOrDirectByte(PARAM_1);
	Actor *a = derefActor(act, "o5_getActorWidth");
	setResult(a->width);
}

void ScummEngine_v5::o5_getActorX() {
	int a;
	getResultPos();
	a = getVarOrDirectWord(PARAM_1);
	setResult(getObjX(a));
}

void ScummEngine_v5::o5_getActorY() {
	int a;
	getResultPos();
	a = getVarOrDirectWord(PARAM_1);
	setResult(getObjY(a));
}

void ScummEngine_v5::o5_saveLoadGame() {
	getResultPos();
	byte a = getVarOrDirectByte(PARAM_1);
	byte slot = (a & 0x1F) + 1;
	byte result = 0;
	
	_opcode = a & 0xE0;

	switch (_opcode) {
	case 0x00: // num slots available
		result = 100;
		break;
	case 0x20: // drive
		// 0 = hard drive
		// 1 = disk drive
		result = 0;
		break;
	case 0x40: // load
		if (loadState(slot, _saveTemporaryState))
			result = 3; // sucess
		else
			result = 5; // failed to load
		break;
	case 0x80: // save
		if (saveState(slot, _saveTemporaryState))
			result = 0;
		else
			result = 2;
		break;
	case 0xC0: // test if save exists
		SaveFile *file;
		bool avail_saves[100];
		char filename[256];
		SaveFileManager *mgr = _system->get_savefile_manager();
		listSavegames(avail_saves, ARRAYSIZE(avail_saves), mgr);
		makeSavegameName(filename, slot, false);
		if (avail_saves[slot]) {
			file = mgr->open_savefile(filename, getSavePath(), false);
			if (file)
			{
				result = 6; // save file exists
				delete file;
			}
		}
		else
			result = 7; // save file does not exist
		break;
	}
	setResult(result);
}
		
void ScummEngine_v5::o5_getAnimCounter() {
	getResultPos();

	int act = getVarOrDirectByte(PARAM_1);
	Actor *a = derefActor(act, "o5_getAnimCounter");
	setResult(a->cost.animCounter);
}

void ScummEngine_v5::o5_getClosestObjActor() {
	int obj;
	int act;
	int dist;

	// This code can't detect any actors farther away than 255 units
	// (pixels in newer games, characters in older ones.) But this is
	// perfectly OK, as it is exactly how the original behaved.

	int closest_obj = 0xFF, closest_dist = 0xFF;

	getResultPos();

	act = getVarOrDirectWord(PARAM_1);
	obj = VAR(VAR_ACTOR_RANGE_MAX);

	do {
		dist = getObjActToObjActDist(act, obj);
		if (dist < closest_dist) {
			closest_dist = dist;
			closest_obj = obj;
		}
	} while (--obj >= VAR(VAR_ACTOR_RANGE_MIN));

	setResult(closest_obj);
}

void ScummEngine_v5::o5_getDist() {
	int o1, o2;
	int r;
	getResultPos();
	o1 = getVarOrDirectWord(PARAM_1);
	o2 = getVarOrDirectWord(PARAM_2);
	r = getObjActToObjActDist(o1, o2);

	// FIXME: MI2 race workaround, see bug #597022
	if (_gameId == GID_MONKEY2 && vm.slot[_currentScript].number == 40 && r < 60)
		r = 60;

	setResult(r);
}

void ScummEngine_v5::o5_getInventoryCount() {
	getResultPos();
	setResult(getInventoryCount(getVarOrDirectByte(PARAM_1)));
}

void ScummEngine_v5::o5_getObjectOwner() {
	getResultPos();
	setResult(getOwner(getVarOrDirectWord(PARAM_1)));
}

void ScummEngine_v5::o5_getObjectState() {
	getResultPos();
	setResult(getState(getVarOrDirectWord(PARAM_1)));
}

void ScummEngine_v5::o5_ifState() {
	int a = getVarOrDirectWord(PARAM_1);
	int b = getVarOrDirectByte(PARAM_2);

	if (getState(a) != b)
		o5_jumpRelative();
	else
		ignoreScriptWord();
}

void ScummEngine_v5::o5_ifNotState() {
	int a = getVarOrDirectWord(PARAM_1);
	int b = getVarOrDirectByte(PARAM_2);

	if (getState(a) == b)
		o5_jumpRelative();
	else
		ignoreScriptWord();
}

void ScummEngine_v5::o5_getRandomNr() {
	getResultPos();
	setResult(_rnd.getRandomNumber(getVarOrDirectByte(PARAM_1)));
}

void ScummEngine_v5::o5_isScriptRunning() {
	getResultPos();
	setResult(isScriptRunning(getVarOrDirectByte(PARAM_1)));
}

void ScummEngine_v5::o5_getVerbEntrypoint() {
	int a, b;
	getResultPos();
	a = getVarOrDirectWord(PARAM_1);
	b = getVarOrDirectWord(PARAM_2);

	setResult(getVerbEntrypoint(a, b));
}

void ScummEngine_v5::o5_ifClassOfIs() {
	int act, cls, b = 0;
	bool cond = true;

	act = getVarOrDirectWord(PARAM_1);

	while ((_opcode = fetchScriptByte()) != 0xFF) {
		cls = getVarOrDirectWord(PARAM_1);

		if (!cls) // FIXME: Ender can't remember why this is here,
			b = false;  // but it fixes an oddball zak256 crash
		else
			b = getClass(act, cls);

		if (((cls & 0x80) && !b) || (!(cls & 0x80) && b))		// FIX_ATARI, was: if (cls & 0x80 && !b || !(cls & 0x80) && b)
			cond = false;
	}
	if (cond)
		ignoreScriptWord();
	else
		o5_jumpRelative();
}

void ScummEngine_v5::o5_increment() {
	getResultPos();
	setResult(readVar(_resultVarNumber) + 1);
}

void ScummEngine_v5::o5_isActorInBox() {
	int act = getVarOrDirectByte(PARAM_1);
	int box = getVarOrDirectByte(PARAM_2);
	Actor *a = derefActor(act, "o5_isActorInBox");

	if (!checkXYInBoxBounds(box, a->_pos.x, a->_pos.y))
		o5_jumpRelative();
	else
		ignoreScriptWord();
}

void ScummEngine_v5::o5_isEqual() {
	int var = fetchScriptWord();
	int16 a = readVar(var);
	int16 b = getVarOrDirectWord(PARAM_1);

	// HACK: See bug report #602348. The sound effects for Largo's screams
	// are only played on type 5 soundcards. However, there is at least one
	// other sound effect (the bartender spitting) which is only played on
	// type 3 soundcards.

	if (_gameId == GID_MONKEY2 && var == VAR_SOUNDCARD && b == 5)
		b = a;

	if (b == a)
		ignoreScriptWord();
	else
		o5_jumpRelative();

}

void ScummEngine_v5::o5_isGreater() {
	int16 a = getVar();
	int16 b = getVarOrDirectWord(PARAM_1);
	if (b > a)
		ignoreScriptWord();
	else
		o5_jumpRelative();
}

void ScummEngine_v5::o5_isGreaterEqual() {
	int16 a = getVar();
	int16 b = getVarOrDirectWord(PARAM_1);
	if (b >= a)
		ignoreScriptWord();
	else
		o5_jumpRelative();
}

void ScummEngine_v5::o5_isLess() {
	int16 a = getVar();
	int16 b = getVarOrDirectWord(PARAM_1);
	if (b < a)
		ignoreScriptWord();
	else
		o5_jumpRelative();
}

void ScummEngine_v5::o5_lessOrEqual() {
	int16 a = getVar();
	int16 b = getVarOrDirectWord(PARAM_1);

	if (b <= a)
		ignoreScriptWord();
	else
		o5_jumpRelative();
}

void ScummEngine_v5::o5_isNotEqual() {
	int16 a = getVar();
	int16 b = getVarOrDirectWord(PARAM_1);
	if (b != a)
		ignoreScriptWord();
	else
		o5_jumpRelative();
}

void ScummEngine_v5::o5_notEqualZero() {
	int a = getVar();
	if (a != 0)
		ignoreScriptWord();
	else
		o5_jumpRelative();
}

void ScummEngine_v5::o5_equalZero() {
	int a = getVar();
	if (a == 0)
		ignoreScriptWord();
	else
		o5_jumpRelative();
}

void ScummEngine_v5::o5_jumpRelative() {
	int16 w = fetchScriptWord();
//	debug(9,"jumprelative = %d, %d", w);
	_scriptPointer += w;
}

void ScummEngine_v5::o5_lights() {
	int a, b, c;

	a = getVarOrDirectByte(PARAM_1);
	b = fetchScriptByte();
	c = fetchScriptByte();

	if (c == 0)
		VAR(VAR_CURRENT_LIGHTS) = a;
	else if (c == 1) {
		_flashlight.xStrips = a;
		_flashlight.yStrips = b;
	}
	_fullRedraw = 1;
}

void ScummEngine_v5::o5_loadRoom() {
	int room;

	room = getVarOrDirectByte(PARAM_1);
	startScene(room, 0, 0);
	_fullRedraw = 1;
}

void ScummEngine_v5::o5_loadRoomWithEgo() {
	Actor *a;
	int obj, room, x, y;

	obj = getVarOrDirectWord(PARAM_1);
	room = getVarOrDirectByte(PARAM_2);

	a = derefActor(VAR(VAR_EGO), "o5_loadRoomWithEgo");

	a->putActor(0, 0, room);
	_egoPositioned = false;

	x = (int16)fetchScriptWord();
	y = (int16)fetchScriptWord();

	VAR(VAR_WALKTO_OBJ) = obj;
	startScene(a->room, a, obj);
	VAR(VAR_WALKTO_OBJ) = 0;

	// FIXME: Can this be removed?
	camera._cur.x = a->_pos.x;

	setCameraAt(a->_pos.x, a->_pos.y);
	setCameraFollows(a);

	_fullRedraw = 1;

	if (x != -1) {
		a->startWalkActor(x, y, -1);
	}
}

void ScummEngine_v5::o5_matrixOps() {
	int a, b;

	_opcode = fetchScriptByte();
	switch (_opcode & 0x1F) {
	case 1:
		a = getVarOrDirectByte(PARAM_1);
		b = getVarOrDirectByte(PARAM_2);
		setBoxFlags(a, b);
		break;
	case 2:
		a = getVarOrDirectByte(PARAM_1);
		b = getVarOrDirectByte(PARAM_2);
		setBoxScale(a, b);
		break;
	case 3:
		a = getVarOrDirectByte(PARAM_1);
		b = getVarOrDirectByte(PARAM_2);
		setBoxScale(a, (b - 1) | 0x8000);
		break;
	case 4:
		createBoxMatrix();
		break;
	}
}

void ScummEngine_v5::o5_move() {
	getResultPos();
	setResult(getVarOrDirectWord(PARAM_1));
}

void ScummEngine_v5::o5_multiply() {
	int a;
	getResultPos();
	a = getVarOrDirectWord(PARAM_1);
	setResult(readVar(_resultVarNumber) * a);
}

void ScummEngine_v5::o5_or() {
	int a;
	getResultPos();
	a = getVarOrDirectWord(PARAM_1);
	setResult(readVar(_resultVarNumber) | a);
}

void ScummEngine_v5::o5_beginOverride() {
	if (fetchScriptByte() != 0)
		beginOverride();
	else
		endOverride();
}

void ScummEngine_v5::o5_panCameraTo() {
	panCameraTo(getVarOrDirectWord(PARAM_1), 0);
}

void ScummEngine_v5::o5_pickupObject() {
	int obj, room;
	obj = getVarOrDirectWord(PARAM_1);
	room = getVarOrDirectByte(PARAM_2);
	if (room == 0)
		room = _roomResource;
	addObjectToInventory(obj, room);
	putOwner(obj, VAR(VAR_EGO));
	putClass(obj, kObjectClassUntouchable, 1);
	putState(obj, 1);
	markObjectRectAsDirty(obj);
	clearDrawObjectQueue();
	runInventoryScript(1);
}

void ScummEngine_v5::o5_print() {
	_actorToPrintStrFor = getVarOrDirectByte(PARAM_1);
	decodeParseString();
}

void ScummEngine_v5::o5_printEgo() {
	_actorToPrintStrFor = (byte)VAR(VAR_EGO);
	decodeParseString();
}

void ScummEngine_v5::o5_pseudoRoom() {
	int i = fetchScriptByte(), j;
	while ((j = fetchScriptByte()) != 0) {
		if (j >= 0x80) {
			_resourceMapper[j & 0x7F] = i;
		}
	}
}

void ScummEngine_v5::o5_putActor() {
	int x, y;
	Actor *a;

	a = derefActor(getVarOrDirectByte(PARAM_1), "o5_putActor");
	x = getVarOrDirectWord(PARAM_2);
	y = getVarOrDirectWord(PARAM_3);
	a->putActor(x, y, a->room);
}

void ScummEngine_v5::o5_putActorAtObject() {
	int obj, x, y;
	Actor *a;

	a = derefActor(getVarOrDirectByte(PARAM_1), "o5_putActorAtObject");
	obj = getVarOrDirectWord(PARAM_2);
	if (whereIsObject(obj) != WIO_NOT_FOUND)
		getObjectXYPos(obj, x, y);
	else {
		x = 240;
		y = 120;
	}
	a->putActor(x, y, a->room);
}

void ScummEngine_v5::o5_putActorInRoom() {
	Actor *a;
	int act = getVarOrDirectByte(PARAM_1);
	int room = getVarOrDirectByte(PARAM_2);

	a = derefActor(act, "o5_putActorInRoom");
	
	if (a->visible && _currentRoom != room && talkingActor() == a->number) {
		clearMsgQueue();
	}
	a->room = room;
	if (!room)
		a->putActor(0, 0, 0);
}

void ScummEngine_v5::o5_quitPauseRestart() {
	byte subOp = fetchScriptByte();
	switch (subOp) {
	case 1:		// SO_RESTART
		restart();
		break;
	case 2:		// SO_PAUSE
		pauseGame();
		break;
	case 3:		// SO_QUIT
		shutDown();
		break;
	default:
		error("o5_quitPauseRestart: unknown subopcode %d", subOp);
	}
}

void ScummEngine_v5::o5_resourceRoutines() {
	const ResTypes resType[4] = { rtScript, rtSound, rtCostume, rtRoom };
	int resid = 0;
	int foo, bar;

	_opcode = fetchScriptByte();
	if (_opcode != 17)
		resid = getVarOrDirectByte(PARAM_1);

	if ((_opcode & 0x3F) != (_opcode & 0x1F))
		error("Oops, this shouldn't happen: o5_resourceRoutines opcode %d", _opcode);

	int op = _opcode & 0x3F;

	switch (_opcode & 0x3F) {
	case 1:			// SO_LOAD_SCRIPT
	case 2:			// SO_LOAD_SOUND
	case 3:			// SO_LOAD_COSTUME
		ensureResourceLoaded(resType[op - 1], resid);
		break;
	case 4:			// SO_LOAD_ROOM
		ensureResourceLoaded(rtRoom, resid);
		break;

	case 5:			// SO_NUKE_SCRIPT
	case 6:			// SO_NUKE_SOUND
	case 7:			// SO_NUKE_COSTUME
	case 8:			// SO_NUKE_ROOM
		setResourceCounter(resType[op-5], resid, 0x7F);
		break;
	case 9:			// SO_LOCK_SCRIPT
		if (resid >= _numGlobalScripts)
			break;
		lock(rtScript, resid);
		break;
	case 10:		// SO_LOCK_SOUND
		lock(rtSound, resid);
		break;
	case 11:		// SO_LOCK_COSTUME
		lock(rtCostume, resid);
		break;
	case 12:		// SO_LOCK_ROOM
		if (resid > 0x7F)
			resid = _resourceMapper[resid & 0x7F];
		lock(rtRoom, resid);
		break;

	case 13:		// SO_UNLOCK_SCRIPT
		if (resid >= _numGlobalScripts)
			break;
		unlock(rtScript, resid);
		break;
	case 14:		// SO_UNLOCK_SOUND
		unlock(rtSound, resid);
		break;
	case 15:		// SO_UNLOCK_COSTUME
		unlock(rtCostume, resid);
		break;
	case 16:		// SO_UNLOCK_ROOM
		if (resid > 0x7F)
			resid = _resourceMapper[resid & 0x7F];
		unlock(rtRoom, resid);
		break;

	case 17:		// SO_CLEAR_HEAP
		//heapClear(0);
		//unkHeapProc2(0, 0);
		break;
	case 18:		// SO_LOAD_CHARSET
		loadCharset(resid);
		break;
	case 19:		// SO_NUKE_CHARSET
		nukeCharset(resid);
		break;
	case 20:		// SO_LOAD_OBJECT
		loadFlObject(getVarOrDirectWord(PARAM_2), resid);
		break;

	// TODO: For the following see also Hibarnatus' information on bug #805691.
	case 32:
		// TODO (apparently never used in FM Towns)
		warning("o5_resourceRoutines %d not yet handled (script %d)", _opcode & 0x3F,  vm.slot[_currentScript].number);
		break;
	case 33:
		// TODO (apparently never used in FM Towns)
		warning("o5_resourceRoutines %d not yet handled (script %d)", _opcode & 0x3F,  vm.slot[_currentScript].number);
		break;
	case 35:
		// TODO: Might be used to set CD volume in FM Towns Loom
		foo = getVarOrDirectByte(PARAM_2);
		warning("o5_resourceRoutines %d not yet handled (script %d)", _opcode & 0x3F,  vm.slot[_currentScript].number);
		break;
	case 36:
		// TODO: Sets the loudness of a sound resource. Used in Indy3 and Zak. 
		foo = getVarOrDirectByte(PARAM_2);
		bar = fetchScriptByte();
		warning("o5_resourceRoutines %d not yet handled (script %d)", _opcode & 0x3F,  vm.slot[_currentScript].number);
		break;
	case 37:
		// TODO: Sets the pitch of a sound resource (pitch = foo - center semitones.
		// "center" is at 0x32 in the sfx resource (always 0x3C in zak256, but sometimes different in Indy3). 
		foo = getVarOrDirectByte(PARAM_2);
		warning("o5_resourceRoutines %d not yet handled (script %d)", _opcode & 0x3F,  vm.slot[_currentScript].number);
		break;

	default:
		warning("Unknown o5_resourceRoutines: %d", _opcode & 0x3F);
		break;
	}
}

void ScummEngine_v5::o5_roomOps() {
	int a = 0, b = 0, c, d, e;

	_opcode = fetchScriptByte();
	switch (_opcode & 0x1F) {
	case 1:		// SO_ROOM_SCROLL
		a = getVarOrDirectWord(PARAM_1);
		b = getVarOrDirectWord(PARAM_2);

		if (a < (SCREEN_WIDTH / 2))
			a = (SCREEN_WIDTH / 2);
		if (b < (SCREEN_WIDTH / 2))
			b = (SCREEN_WIDTH / 2);
		if (a > _roomWidth - (SCREEN_WIDTH / 2))
			a = _roomWidth - (SCREEN_WIDTH / 2);
		if (b > _roomWidth - (SCREEN_WIDTH / 2))
			b = _roomWidth - (SCREEN_WIDTH / 2);
		VAR(VAR_CAMERA_MIN_X) = a;
		VAR(VAR_CAMERA_MAX_X) = b;
		break;
	case 2:		// SO_ROOM_COLOR
		error("room-color is no longer a valid command");
		break;

	case 3:		// SO_ROOM_SCREEN
		a = getVarOrDirectWord(PARAM_1);
		b = getVarOrDirectWord(PARAM_2);
		initScreens(a, b);
		break;
	case 4:		// SO_ROOM_PALETTE
		a = getVarOrDirectWord(PARAM_1);
		b = getVarOrDirectWord(PARAM_2);
		c = getVarOrDirectWord(PARAM_3);
		_opcode = fetchScriptByte();
		d = getVarOrDirectByte(PARAM_1);
		setPalColor(d, a, b, c);	/* index, r, g, b */
		break;
	case 5:		// SO_ROOM_SHAKE_ON
		setShake(1);
		break;
	case 6:		// SO_ROOM_SHAKE_OFF
		setShake(0);
		break;
	case 7:		// SO_ROOM_SCALE
		a = getVarOrDirectByte(PARAM_1);
		b = getVarOrDirectByte(PARAM_2);
		_opcode = fetchScriptByte();
		c = getVarOrDirectByte(PARAM_1);
		d = getVarOrDirectByte(PARAM_2);
		_opcode = fetchScriptByte();
		e = getVarOrDirectByte(PARAM_2);
		setScaleSlot(e - 1, 0, b, a, 0, d, c);
		break;
	case 8:		// SO_ROOM_INTENSITY
		a = getVarOrDirectByte(PARAM_1);
		b = getVarOrDirectByte(PARAM_2);
		c = getVarOrDirectByte(PARAM_3);
		darkenPalette(a, a, a, b, c);
		break;
	case 9:		// SO_ROOM_SAVEGAME
		_saveLoadFlag = getVarOrDirectByte(PARAM_1);
		_saveLoadSlot = getVarOrDirectByte(PARAM_2);
		_saveLoadSlot = 99;					/* use this slot */
		_saveTemporaryState = true;
		break;
	case 10:	// SO_ROOM_FADE
		a = getVarOrDirectWord(PARAM_1);
		if (a) {
			_switchRoomEffect = (byte)(a & 0xFF);
			_switchRoomEffect2 = (byte)(a >> 8);
		} else {
			fadeIn(_newEffect);
		}
		break;
	case 11:	// SO_RGB_ROOM_INTENSITY
		a = getVarOrDirectWord(PARAM_1);
		b = getVarOrDirectWord(PARAM_2);
		c = getVarOrDirectWord(PARAM_3);
		_opcode = fetchScriptByte();
		d = getVarOrDirectByte(PARAM_1);
		e = getVarOrDirectByte(PARAM_2);
		darkenPalette(a, b, c, d, e);
		break;
	case 12:	// SO_ROOM_SHADOW
		a = getVarOrDirectWord(PARAM_1);
		b = getVarOrDirectWord(PARAM_2);
		c = getVarOrDirectWord(PARAM_3);
		_opcode = fetchScriptByte();
		d = getVarOrDirectByte(PARAM_1);
		e = getVarOrDirectByte(PARAM_2);
		setupShadowPalette(a, b, c, d, e);
		break;

	case 13:	// SO_SAVE_STRING
		{
			SaveFile *file;
			char filename[256], *s;

			a = getVarOrDirectByte(PARAM_1);
			s = filename;
			while ((*s++ = fetchScriptByte()));

			SaveFileManager *mgr = _system->get_savefile_manager();
			file = mgr->open_savefile(filename, getSavePath(), true);
			if (file != NULL) {
				byte *ptr;
				ptr = getResourceAddress(rtString, a);
				file->write(ptr, resStrLen(ptr) + 1);
				delete file;
			}
			delete mgr;
			break;
		}
	case 14:	// SO_LOAD_STRING
		{
			SaveFile *file;
			char filename[256], *s;

			a = getVarOrDirectByte(PARAM_1);
			s = filename;
			while ((*s++ = fetchScriptByte()));

			SaveFileManager *mgr = _system->get_savefile_manager();
			file = mgr->open_savefile(filename, getSavePath(), false);
			if (file != NULL) {
				byte *ptr;
				int len = 256, cnt = 0;
				ptr = (byte *)malloc(len);
				while (ptr) {
				  int r = file->read(ptr + cnt, len - cnt);
				  if ((cnt += r) < len) break;
				  ptr = (byte *)realloc(ptr, len *= 2);
				}
				ptr[cnt] = '\0';
				loadPtrToResource(rtString, a, ptr);
				free(ptr);
				delete file;
			}
			delete mgr;
			break;
		}
	case 15:	// SO_ROOM_TRANSFORM
		a = getVarOrDirectByte(PARAM_1);
		_opcode = fetchScriptByte();
		b = getVarOrDirectByte(PARAM_1);
		c = getVarOrDirectByte(PARAM_2);
		_opcode = fetchScriptByte();
		d = getVarOrDirectByte(PARAM_1);
		palManipulateInit(a, b, c, d);
		break;

	case 16:	// SO_CYCLE_SPEED
		a = getVarOrDirectByte(PARAM_1);
		b = getVarOrDirectByte(PARAM_2);
		if (a < 1)
			a = 1;										/* FIXME: ZAK256 */
		checkRange(16, 1, a, "o5_roomOps: 16: color cycle out of range (%d)");
		_colorCycle[a - 1].delay = (b != 0) ? 0x4000 / (b * 0x4C) : 0;
		break;
	default:
		error("o5_roomOps: unknown subopcode %d", _opcode & 0x1F);
	}
}

void ScummEngine_v5::o5_saveRestoreVerbs() {
	int a, b, c, slot, slot2;

	_opcode = fetchScriptByte();

	a = getVarOrDirectByte(PARAM_1);
	b = getVarOrDirectByte(PARAM_2);
	c = getVarOrDirectByte(PARAM_3);

	switch (_opcode) {
	case 1:		// SO_SAVE_VERBS
		while (a <= b) {
			slot = getVerbSlot(a, 0);
			if (slot && _verbs[slot].saveid == 0) {
				_verbs[slot].saveid = c;
				drawVerb(slot, 0);
				verbMouseOver(0);
			}
			a++;
		}
		break;
	case 2:		// SO_RESTORE_VERBS
		while (a <= b) {
			slot = getVerbSlot(a, c);
			if (slot) {
				slot2 = getVerbSlot(a, 0);
				if (slot2)
					killVerb(slot2);
				slot = getVerbSlot(a, c);
				_verbs[slot].saveid = 0;
				drawVerb(slot, 0);
				verbMouseOver(0);
			}
			a++;
		}
		break;
	case 3:		// SO_DELETE_VERBS
		while (a <= b) {
			slot = getVerbSlot(a, c);
			if (slot)
				killVerb(slot);
			a++;
		}
		break;
	default:
		error("o5_saveRestoreVerbs: unknown subopcode %d", _opcode);
	}
}

void ScummEngine_v5::o5_setCameraAt() {
	setCameraAtEx(getVarOrDirectWord(PARAM_1));
}

void ScummEngine_v5::o5_setObjectName() {
	int obj = getVarOrDirectWord(PARAM_1);
	int size;
	int a;
	int i = 0;
	byte *name = NULL;
	unsigned char work[256];
	
	// Read in new name
	while ((a = fetchScriptByte()) != 0) {
		work[i++] = a;
		if (a == 0xFF) {
			work[i++] = fetchScriptByte();
			work[i++] = fetchScriptByte();
			work[i++] = fetchScriptByte();
		}
	}
	work[i++] = 0;

	if (obj < _numActors)
		error("Can't set actor %d name with new-name-of", obj);

	// TODO: Would be nice if we used rtObjectName resource for pre-V6
	// games, too. The only problem with that which I can see is that this
	// would break savegames. I.e. it would require yet another change to
	// the save/load system.

	byte *objptr;
	objptr = getOBCDFromObject(obj);
	if (objptr == NULL) {
		// WORKAROUND bug #587553: This is an odd one and looks more like
		// an actual bug in the original script. Usually we would error
		warning("Can't find OBCD to rename object %d to %s", obj, work);
		return;
	}

	// FIXME: we can't use findResourceData anymore, because it returns const
	// data, while this function *must* return a non-const pointer. That is so
	// because in o2_setObjectName / o5_setObjectName we directly modify this
	// data. Now, we could add a non-const version of findResourceData, too
	// (C++ makes that easy); but this here is really the *only* place in all
	// of ScummVM where it wold be needed! That seems kind of a waste...
	//
	// So for now, I duplicate some code from findResourceData / findResource
	// here. However, a much nicer solution might be (with stress on "might")
	// to use the same technique as in V6 games: that is, use a separate
	// resource for changed names. That would be the cleanest solution, but
	// might proof to be infeasible, as it might lead to unforseen regressions.
	
	name = 0;
	const uint32 tag = MKID('OBNA');
	byte *searchin = objptr;
	uint32 curpos, totalsize;

	assert(searchin);

	searchin += 4;
	totalsize = READ_BE_UINT32(searchin);
	curpos = 8;
	searchin += 4;

	while (curpos < totalsize) {
		if (READ_UINT32(searchin) == tag) {
			name = searchin + _resourceHeaderSize;
			break;
		}

		size = READ_BE_UINT32(searchin + 4);
		if ((int32)size <= 0) {
			error("(OBNA) Not found... illegal block len %d", size);
		}

		curpos += size;
		searchin += size;
	}
	size = getResourceDataSize(name);
	
	if (name == 0)
		return;		// Silently bail out
	

	if (i > size) {
		warning("New name of object %d too long: old 's' (%d), new '%s' (%d))",
				obj, name, i, work, size);
		i = size;
	}

	memcpy(name, work, i);
	runInventoryScript(0);
}

void ScummEngine_v5::o5_setOwnerOf() {
	int obj, owner;

	obj = getVarOrDirectWord(0x80);
	owner = getVarOrDirectByte(0x40);

	setOwnerOf(obj, owner);
}

void ScummEngine_v5::o5_setState() {
	int obj, state;
	obj = getVarOrDirectWord(PARAM_1);
	state = getVarOrDirectByte(PARAM_2);
	putState(obj, state);
	markObjectRectAsDirty(obj);
	if (_BgNeedsRedraw)
		clearDrawObjectQueue();
}

void ScummEngine_v5::o5_setVarRange() {
	int a, b;

	getResultPos();
	a = fetchScriptByte();
	do {
		if (_opcode & 0x80)
			b = fetchScriptWordSigned();
		else
			b = fetchScriptByte();

		setResult(b);
		_resultVarNumber++;
	} while (--a);
}

void ScummEngine_v5::o5_startMusic() {
	_sound->addSoundToQueue(getVarOrDirectByte(PARAM_1));
}

void ScummEngine_v5::o5_startSound() {
	VAR(VAR_MUSIC_TIMER) = 0;
	_sound->addSoundToQueue(getVarOrDirectByte(PARAM_1));
}

void ScummEngine_v5::o5_stopMusic() {
	_sound->stopAllSounds();
}

void ScummEngine_v5::o5_stopSound() {
	_sound->stopSound(getVarOrDirectByte(PARAM_1));
}

void ScummEngine_v5::o5_isSoundRunning() {
	int snd;
	getResultPos();
	snd = getVarOrDirectByte(PARAM_1);
	if (snd)
		snd = _sound->isSoundRunning(snd);
	setResult(snd);
}

void ScummEngine_v5::o5_soundKludge() {
	int items[16];
	int num = getWordVararg(items);
	_sound->soundKludge(items, num);
}

void ScummEngine_v5::o5_startObject() {
	int obj, script;
	int data[16];

	obj = getVarOrDirectWord(PARAM_1);
	script = getVarOrDirectByte(PARAM_2);

	getWordVararg(data);
	runObjectScript(obj, script, 0, 0, data);
}

void ScummEngine_v5::o5_startScript() {
	int op, script;
	int data[16];

	op = _opcode;
	script = getVarOrDirectByte(PARAM_1);

	getWordVararg(data);

	runScript(script, (op & 0x20) != 0, (op & 0x40) != 0, data);
}

void ScummEngine_v5::o5_stopObjectCode() {
	stopObjectCode();
}

void ScummEngine_v5::o5_stopObjectScript() {
	stopObjectScript(getVarOrDirectWord(PARAM_1));
}

void ScummEngine_v5::o5_stopScript() {
	int script;

	script = getVarOrDirectByte(PARAM_1);
	if (!script)
		stopObjectCode();
	else
		stopScript(script);
}

void ScummEngine_v5::o5_stringOps() {
	int a, b, c, i;
	byte *ptr;

	_opcode = fetchScriptByte();
	switch (_opcode & 0x1F) {
	case 1:											/* loadstring */
		loadPtrToResource(rtString, getVarOrDirectByte(PARAM_1), NULL);
		break;
	case 2:											/* copystring */
		a = getVarOrDirectByte(PARAM_1);
		b = getVarOrDirectByte(PARAM_2);
		nukeResource(rtString, a);
		ptr = getResourceAddress(rtString, b);
		if (ptr)
			loadPtrToResource(rtString, a, ptr);
		break;
	case 3:											/* set string char */
		a = getVarOrDirectByte(PARAM_1);
		b = getVarOrDirectByte(PARAM_2);
		c = getVarOrDirectByte(PARAM_3);
		ptr = getResourceAddress(rtString, a);
		if (ptr == NULL)
			error("String %d does not exist", a);
		ptr[b] = c;
		break;

	case 4:											/* get string char */
		getResultPos();
		a = getVarOrDirectByte(PARAM_1);
		b = getVarOrDirectByte(PARAM_2);
		ptr = getResourceAddress(rtString, a);
		if (ptr == NULL)
			error("String %d does not exist", a);
		setResult(ptr[b]);
		break;

	case 5:											/* create empty string */
		a = getVarOrDirectByte(PARAM_1);
		b = getVarOrDirectByte(PARAM_2);
		nukeResource(rtString, a);
		if (b) {
			ptr = createResource(rtString, a, b);
			if (ptr) {
				for (i = 0; i < b; i++)
					ptr[i] = 0;
			}
		}
		break;
	}
}

void ScummEngine_v5::o5_subtract() {
	int a;
	getResultPos();
	a = getVarOrDirectWord(PARAM_1);
	setResult(readVar(_resultVarNumber) - a);
}

void ScummEngine_v5::o5_verbOps() {
	int verb, slot;
	VerbSlot *vs;
	int a, b;
	byte *ptr;

	verb = getVarOrDirectByte(PARAM_1);

	slot = getVerbSlot(verb, 0);
	checkRange(_numVerbs - 1, 0, slot, "Illegal new verb slot %d");

	vs = &_verbs[slot];
	vs->verbid = verb;

	while ((_opcode = fetchScriptByte()) != 0xFF) {
		switch (_opcode & 0x1F) {
		case 1:		// SO_VERB_IMAGE
			a = getVarOrDirectWord(PARAM_1);
			if (slot) {
				setVerbObject(_roomResource, a, slot);
				vs->type = kImageVerbType;
			}
			break;
		case 2:		// SO_VERB_NAME
			loadPtrToResource(rtVerb, slot, NULL);
			if (slot == 0)
				nukeResource(rtVerb, slot);
			vs->type = kTextVerbType;
			vs->imgindex = 0;
			break;
		case 3:		// SO_VERB_COLOR
			vs->color = getVarOrDirectByte(PARAM_1);
			break;
		case 4:		// SO_VERB_HICOLOR
			vs->hicolor = getVarOrDirectByte(PARAM_1);
			break;
		case 5:		// SO_VERB_AT
			vs->curRect.left = getVarOrDirectWord(PARAM_1);
			vs->curRect.top = getVarOrDirectWord(PARAM_2);
			break;
		case 6:		// SO_VERB_ON
			vs->curmode = 1;
			break;
		case 7:		// SO_VERB_OFF
			vs->curmode = 0;
			break;
		case 8:		// SO_VERB_DELETE
			killVerb(slot);
			break;
		case 9:		// SO_VERB_NEW
			slot = getVerbSlot(verb, 0);
			if (slot == 0) {
				for (slot = 1; slot < _numVerbs; slot++) {
					if (_verbs[slot].verbid == 0)
						break;
				}
				if (slot == _numVerbs)
					error("Too many verbs");
			}
			vs = &_verbs[slot];
			vs->verbid = verb;
			vs->color = 2;
			vs->hicolor = 0;
			vs->dimcolor = 8;
			vs->type = kTextVerbType;
			vs->charset_nr = _string[0].t_charset;
			vs->curmode = 0;
			vs->saveid = 0;
			vs->key = 0;
			vs->center = 0;
			vs->imgindex = 0;
			break;

		case 16:	// SO_VERB_DIMCOLOR
			vs->dimcolor = getVarOrDirectByte(PARAM_1);
			break;
		case 17:	// SO_VERB_DIM
			vs->curmode = 2;
			break;
		case 18:	// SO_VERB_KEY
			vs->key = getVarOrDirectByte(PARAM_1);
			break;
		case 19:	// SO_VERB_CENTER
			vs->center = 1;
			break;
		case 20:	// SO_VERB_NAME_STR
			ptr = getResourceAddress(rtString, getVarOrDirectWord(PARAM_1));
			if (!ptr)
				nukeResource(rtVerb, slot);
			else {
				loadPtrToResource(rtVerb, slot, ptr);
			}
			if (slot == 0)
				nukeResource(rtVerb, slot);
			vs->type = kTextVerbType;
			vs->imgindex = 0;
			break;
		case 22:										/* assign object */
			a = getVarOrDirectWord(PARAM_1);
			b = getVarOrDirectByte(PARAM_2);
			if (slot && vs->imgindex != a) {
				setVerbObject(b, a, slot);
				vs->type = kImageVerbType;
				vs->imgindex = a;
			}
			break;
		case 23:										/* set back color */
			{
				byte b = getVarOrDirectByte(PARAM_1);
				debug(1, "*** backcolor[%d] = %d - %d", slot, b, getSystemPal(b));
				vs->bkcolor = b; //getSystemPal(b);
			}
			break;
		default:
			error("o5_verbOps: unknown subopcode %d", _opcode & 0x1F);
		}
	}

	// Force redraw of the modified verb slot
	drawVerb(slot, 0);
	verbMouseOver(0);
}

void ScummEngine_v5::o5_wait() {
	const byte *oldaddr = _scriptPointer - 1;
	_opcode = fetchScriptByte();

	switch (_opcode & 0x1F) {
	case 1:		// SO_WAIT_FOR_ACTOR
		{
			Actor *a = derefActorSafe(getVarOrDirectByte(PARAM_1), "o5_wait");
			if (a && a->isInCurrentRoom() && a->moving)
				break;
			return;
		}
	case 2:		// SO_WAIT_FOR_MESSAGE
		if (VAR(VAR_HAVE_MSG))
			break;
		return;
	case 3:		// SO_WAIT_FOR_CAMERA
		if (camera._cur.x / 8 != camera._dest.x / 8)
			break;
		return;
	case 4:		// SO_WAIT_FOR_SENTENCE
		if (_sentenceNum) {
			if (_sentence[_sentenceNum - 1].freezeCount && !isScriptInUse(VAR(VAR_SENTENCE_SCRIPT)))
				return;
			break;
		}
		if (!isScriptInUse(VAR(VAR_SENTENCE_SCRIPT)))
			return;
		break;
	default:
		error("o5_wait: unknown subopcode %d", _opcode & 0x1F);
		return;
	}

	_scriptPointer = oldaddr;
	o5_breakHere();
}

void ScummEngine_v5::o5_walkActorTo() {
	int x, y;
	Actor *a;

	a = derefActor(getVarOrDirectByte(PARAM_1), "o5_walkActorTo");
	x = getVarOrDirectWord(PARAM_2);
	y = getVarOrDirectWord(PARAM_3);
	a->startWalkActor(x, y, -1);
}

void ScummEngine_v5::o5_walkActorToActor() {
	int x, y;
	Actor *a, *a2;
	int nr = getVarOrDirectByte(PARAM_1);
	int nr2 = getVarOrDirectByte(PARAM_2);
	int dist = fetchScriptByte();

	if (nr == 106 && _gameId == GID_INDY4) {
		warning("Bypassing Indy4 bug");
		return;
	}
	
	if (_gameId == GID_INDY4 && nr == 1 && nr2 == 106 &&
		dist == 255 && vm.slot[_currentScript].number == 210) {
		// WORKAROUND bug: Work around an invalid actor bug when using the
		// camel in Fate of Atlantis, the "wits" path. The room-65-210 script
		// contains this:
		//   walkActorToActor(1,106,255)
		// Once again this is either a script bug, or there is some hidden
		// or unknown meaning to this odd walk request...
		return;
	}

	a = derefActor(nr, "o5_walkActorToActor");
	if (!a->isInCurrentRoom())
		return;

	a2 = derefActor(nr2, "o5_walkActorToActor(2)");
	if (!a2->isInCurrentRoom())
		return;

	if (dist == 0xFF) {
		dist = a->scalex * a->width / 0xFF;
		dist += (a2->scalex * a2->width / 0xFF) / 2;
	}
	x = a2->_pos.x;
	y = a2->_pos.y;
	if (x < a->_pos.x)
		x += dist;
	else
		x -= dist;

	a->startWalkActor(x, y, -1);
}

void ScummEngine_v5::o5_walkActorToObject() {
	int obj;
	Actor *a;

	a = derefActor(getVarOrDirectByte(PARAM_1), "o5_walkActorToObject");
	obj = getVarOrDirectWord(PARAM_2);
	if (whereIsObject(obj) != WIO_NOT_FOUND) {
		int x, y, dir;
		getObjectXYPos(obj, x, y, dir);
		a->startWalkActor(x, y, dir);
	}
}

int ScummEngine_v5::getWordVararg(int *ptr) {
	int i;

	for (i = 0; i < 16; i++)
		ptr[i] = 0;

	i = 0;
	while ((_opcode = fetchScriptByte()) != 0xFF) {
		ptr[i++] = getVarOrDirectWord(PARAM_1);
	}
	return i;
}

void ScummEngine_v5::decodeParseString() {
	int textSlot;

	switch (_actorToPrintStrFor) {
	case 252:
		textSlot = 3;
		break;
	case 253:
		textSlot = 2;
		break;
	case 254:
		textSlot = 1;
		break;
	default:
		textSlot = 0;
	}

	setStringVars(textSlot);

	while ((_opcode = fetchScriptByte()) != 0xFF) {
		switch (_opcode & 0xF) {
		case 0:		// SO_AT
			_string[textSlot].xpos = getVarOrDirectWord(PARAM_1);
			_string[textSlot].ypos = getVarOrDirectWord(PARAM_2);
			_string[textSlot].overhead = false;
			break;
		case 1:		// SO_COLOR
			_string[textSlot].color = getVarOrDirectByte(PARAM_1);
			break;
		case 2:		// SO_CLIPPED
			_string[textSlot].right = getVarOrDirectWord(PARAM_1);
			break;
		case 3:		// SO_ERASE
			{
			int a = getVarOrDirectWord(PARAM_1);
			int b = getVarOrDirectWord(PARAM_2);
			warning("ScummEngine_v5::decodeParseString: Unhandled case 3: %d, %d", a, b);
			}
			break;
		case 4:		// SO_CENTER
			_string[textSlot].center = true;
			_string[textSlot].overhead = false;
			break;
		case 6:		// SO_LEFT
			_string[textSlot].center = false;
			_string[textSlot].overhead = false;
			break;
		case 7:		// SO_OVERHEAD
			_string[textSlot].overhead = true;
			break;
		case 8:{	// SO_SAY_VOICE
				int offset, delay;
				offset = (uint16) getVarOrDirectWord(PARAM_1);
				delay = (uint16) getVarOrDirectWord(PARAM_2);
				warning("ScummEngine_v5::decodeParseString: Unhandled case 8");
			}
			break;
		case 15:	// SO_TEXTSTRING
			_messagePtr = _scriptPointer;
			switch (textSlot) {
			case 0:
				actorTalk();
				break;
			case 1:
				drawString(1);
				break;
			case 2:
				unkMessage1();
				break;
			case 3:
				unkMessage2();
				break;
			}

			_scriptPointer = _messagePtr;
			return;
		default:
			warning("ScummEngine_v5::decodeParseString: Unhandled case %d", _opcode & 0xF);
			return;
		}
	}

	_string[textSlot].t_xpos = _string[textSlot].xpos;
	_string[textSlot].t_ypos = _string[textSlot].ypos;
	_string[textSlot].t_center = _string[textSlot].center;
	_string[textSlot].t_overhead = _string[textSlot].overhead;
	_string[textSlot].t_right = _string[textSlot].right;
	_string[textSlot].t_color = _string[textSlot].color;
	_string[textSlot].t_charset = _string[textSlot].charset;
}

void ScummEngine_v5::o5_oldRoomEffect() {
	int a;

	_opcode = fetchScriptByte();
	if ((_opcode & 0x1F) == 3) {
		a = getVarOrDirectWord(PARAM_1);
		if (a) {
			_switchRoomEffect = (byte)(a & 0xFF);
			_switchRoomEffect2 = (byte)(a >> 8);
		} else {
			fadeIn(_newEffect);
		}
	}
}

void ScummEngine_v5::o5_pickupObjectOld() {
	int obj = getVarOrDirectWord(PARAM_1);

	if (obj < 1) {
		error("pickupObjectOld received invalid index %d (script %d)", obj, vm.slot[_currentScript].number);
	}

	if (getObjectIndex(obj) == -1)
		return;

	if (whereIsObject(obj) == WIO_INVENTORY)	/* Don't take an */
		return;											/* object twice */

	// warning("adding %d from %d to inventoryOld", obj, _currentRoom);
	addObjectToInventory(obj, _roomResource);
	markObjectRectAsDirty(obj);
	putOwner(obj, VAR(VAR_EGO));
	putClass(obj, kObjectClassUntouchable, 1);
	putState(obj, 1);
	clearDrawObjectQueue();
	runInventoryScript(1);
}

#undef PARAM_1
#undef PARAM_2
#undef PARAM_3

} // End of namespace Scumm
