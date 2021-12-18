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
 * $Header: /cvsroot/scummvm/scummvm/scumm/script.cpp,v 1.149 2004/02/11 05:38:15 kirben Exp $
 *
 */

#include "stdafx.h"

#include "common/config-manager.h"
#include "common/util.h"

#include "scumm/actor.h"
#include "scumm/object.h"
#include "scumm/resource.h"
#include "scumm/scumm.h"

extern bool g_slowMachine;

namespace Scumm {

/* Start executing script 'script' with the given parameters */
void ScummEngine::runScript(int script, bool freezeResistant, bool recursive, int *lvarptr) {
	ScriptSlot *s;
	//byte *scriptPtr;
	uint32 scriptOffs;
	byte scriptType;
	int slot;

	if (!script)
		return;

	if (!recursive)
		stopScript(script);

	if (script < _numGlobalScripts) {
		//scriptPtr = getResourceAddress(rtScript, script);
		scriptOffs = _resourceHeaderSize;
		scriptType = WIO_GLOBAL;

		debugC(DEBUG_SCRIPTS, "runScript(Global-%d) from %d-%d", script, 
				       vm.slot[_currentScript].number, _roomResource);
	} else {
		scriptOffs = _localScriptList[script - _numGlobalScripts];
		if (scriptOffs == 0)
			error("Local script %d is not in room %d", script, _roomResource);
		scriptType = WIO_LOCAL;

		debugC(DEBUG_SCRIPTS, "runScript(%d) from %d-%d", script, 
				       vm.slot[_currentScript].number, _roomResource);
	}

	slot = getScriptSlot();

	s = &vm.slot[slot];
	s->number = script;
	s->offs = scriptOffs;
	s->status = ssRunning;
	s->where = scriptType;
	s->freezeResistant = freezeResistant;
	s->recursive = recursive;
	s->freezeCount = 0;
	s->delayFrameCount = 0;

	initializeLocals(slot, lvarptr);

	runScriptNested(slot);
}

void ScummEngine::runObjectScript(int object, int entry, bool freezeResistant, bool recursive, int *vars, int slot) {
	ScriptSlot *s;
	uint32 obcd;
	int where, offs;

	if (!object)
		return;

	if (!recursive)
		stopObjectScript(object);

	where = whereIsObject(object);

	if (where == WIO_NOT_FOUND) {
		warning("Code for object %d not in room %d", object, _roomResource);
		return;
	}

	obcd = getOBCDOffs(object);
	
	// Find a free object slot, unless one was specified
	if (slot == -1)
		slot = getScriptSlot();

	offs = getVerbEntrypoint(object, entry);
	if (offs == 0)
		return;

	s = &vm.slot[slot];
	s->number = object;
	s->offs = obcd + offs;
	s->status = ssRunning;
	s->where = where;
	s->freezeResistant = freezeResistant;
	s->recursive = recursive;
	s->freezeCount = 0;
	s->delayFrameCount = 0;

	initializeLocals(slot, vars);

	runScriptNested(slot);
}

void ScummEngine::initializeLocals(int slot, int *vars) {
	int i;
	if (!vars) {
		for (i = 0; i < 16; i++)
			vm.localvar[slot][i] = 0;
	} else {
		for (i = 0; i < 16; i++)
			vm.localvar[slot][i] = vars[i];
	}
}

int ScummEngine::getVerbEntrypoint(int obj, int entry) {
	const byte *objptr, *verbptr;
	int verboffs;

	if (whereIsObject(obj) == WIO_NOT_FOUND)
		return 0;

	objptr = getOBCDFromObject(obj);
	assert(objptr);

	verbptr = findResource(MKID('VERB'), objptr);
	assert(verbptr);

	verboffs = verbptr - objptr;
	verbptr += _resourceHeaderSize;

	do {
		if (!*verbptr)
			return 0;
		if (*verbptr == entry || *verbptr == 0xFF)
			break;
		verbptr += 3;
	} while (1);
	
	return verboffs + READ_LE_UINT16(verbptr + 1);
}

/* Stop script 'script' */
void ScummEngine::stopScript(int script) {
	ScriptSlot *ss;
	NestedScript *nest;
	int i, num;

	if (script == 0)
		return;

	ss = vm.slot;
	for (i = 0; i < NUM_SCRIPT_SLOT; i++, ss++) {
		if (script == ss->number && ss->status != ssDead &&
			(ss->where == WIO_GLOBAL || ss->where == WIO_LOCAL)) {
			if (ss->cutsceneOverride)
				error("Script %d stopped with active cutscene/override", script);
			ss->number = 0;
			ss->status = ssDead;
			if (_currentScript == i)
				_currentScript = 0xFF;
		}
	}

	nest = vm.nest;
	num = vm.numNestedScripts;

	while (num > 0) {
		if (nest->number == script &&
				(nest->where == WIO_GLOBAL || nest->where == WIO_LOCAL)) {
			nest->number = 0xFF;
			nest->slot = 0xFF;
			nest->where = 0xFF;
		}
		nest++;
		num--;
	}
}

/* Stop an object script 'script'*/
void ScummEngine::stopObjectScript(int script) {
	ScriptSlot *ss;
	NestedScript *nest;
	int i, num;

	if (script == 0)
		return;

	ss = vm.slot;
	for (i = 0; i < NUM_SCRIPT_SLOT; i++, ss++) {
		if (script == ss->number && ss->status != ssDead &&
		    (ss->where == WIO_ROOM || ss->where == WIO_INVENTORY || ss->where == WIO_FLOBJECT)) {
			if (ss->cutsceneOverride)
				error("Object %d stopped with active cutscene/override", script);
			ss->number = 0;
			ss->status = ssDead;
			if (_currentScript == i)
				_currentScript = 0xFF;
		}
	}

	nest = vm.nest;
	num = vm.numNestedScripts;

	while (num > 0) {
		if (nest->number == script &&
				(nest->where == WIO_ROOM || nest->where == WIO_INVENTORY || nest->where == WIO_FLOBJECT)) {
			nest->number = 0xFF;
			nest->slot = 0xFF;
			nest->where = 0xFF;
		}
		nest++;
		num--;
	}
}

/* Return a free script slot */
int ScummEngine::getScriptSlot() {
	ScriptSlot *ss;
	int i;

	ss = vm.slot;
	for (i = 0; i < NUM_SCRIPT_SLOT; i++, ss++) {
		if (ss->status == ssDead)
			return i;
	}
	error("Too many scripts running, %d max", NUM_SCRIPT_SLOT);
	return -1;
}

/* Run script 'script' nested - eg, within the parent script.*/
void ScummEngine::runScriptNested(int script) {
	NestedScript *nest;
	ScriptSlot *slot;

	updateScriptPtr();

	nest = &vm.nest[vm.numNestedScripts];

	if (_currentScript == 0xFF) {
		nest->number = 0xFF;
		nest->where = 0xFF;
	} else {
		// Store information about the currently running script
		slot = &vm.slot[_currentScript];
		nest->number = slot->number;
		nest->where = slot->where;
		nest->slot = _currentScript;
	}

	vm.numNestedScripts++;
	
	if (vm.numNestedScripts > ARRAYSIZE(vm.nest))
		error("Too many nested scripts");

	_currentScript = script;
	getScriptBaseAddress();
	getScriptEntryPoint();
	executeScript();

	vm.numNestedScripts--;

	if (nest->number != 0xFF) {
		// Try to resume the script which called us, if its status has not changed
		// since it invoked us. In particular, we only resume it if it hasn't been
		// stopped in the meantime, and if it did not already move on.
		slot = &vm.slot[nest->slot];
		if (slot->number == nest->number && slot->where == nest->where &&
				slot->status != ssDead && slot->freezeCount == 0) {
			_currentScript = nest->slot;
			getScriptBaseAddress();
			getScriptEntryPoint();
			return;
		}
	}
	_currentScript = 0xFF;
}

void ScummEngine::updateScriptPtr() {
	if (_currentScript == 0xFF)
		return;

	vm.slot[_currentScript].offs = _scriptPointer - _scriptOrgPointer;
}

/* Get the code pointer to a script */
void ScummEngine::getScriptBaseAddress() {
	ScriptSlot *ss;
	int idx;

	if (_currentScript == 0xFF)
		return;

	ss = &vm.slot[_currentScript];
	switch (ss->where) {
	case WIO_INVENTORY:					/* inventory script * */
		for (idx = 0; idx < _numInventory; idx++)
			if (_inventory[idx] == ss->number)
				break;
		_scriptOrgPointer = getResourceAddress(rtInventory, idx);
		assert(idx < _numInventory);
		_lastCodePtr = &_baseInventoryItems[idx];
		break;

	case WIO_LOCAL:
	case WIO_ROOM:								/* room script */
		_scriptOrgPointer = getResourceAddress(rtRoom, _roomResource);
		assert(_roomResource < _numRooms);
		_lastCodePtr = &_baseRooms[_roomResource];
		break;

	case WIO_GLOBAL:							/* global script */
		_scriptOrgPointer = getResourceAddress(rtScript, ss->number);
		assert(ss->number < _numScripts);
		_lastCodePtr = &_baseScripts[ss->number];
		break;

	case WIO_FLOBJECT:						/* flobject script */
		idx = getObjectIndex(ss->number);
		_scriptOrgPointer = getResourceAddress(rtFlObject, _objs[idx].fl_object_index);
		assert(idx < _numFlObject);
		_lastCodePtr = &_baseFLObject[idx];
		break;
	default:
		error("Bad type while getting base address");
	}
}


void ScummEngine::getScriptEntryPoint() {
	if (_currentScript == 0xFF)
		return;
	_scriptPointer = _scriptOrgPointer + vm.slot[_currentScript].offs;
}

/* Execute a script - Read opcode, and execute it from the table */
void ScummEngine::executeScript() {
	while (_currentScript != 0xFF) {
		
		_opcode = fetchScriptByte();
		vm.slot[_currentScript].didexec = 1;
		debugC(DEBUG_OPCODES, "Script %d, offset 0x%x: [%X] %s()",
				vm.slot[_currentScript].number,
				_scriptPointer - _scriptOrgPointer,
				_opcode,
				getOpcodeDesc(_opcode));

		executeOpcode(_opcode);
	}
	CHECK_HEAP;
}

byte ScummEngine::fetchScriptByte() {
	if (*_lastCodePtr + sizeof(MemBlkHeader) != _scriptOrgPointer) {
		uint32 oldoffs = _scriptPointer - _scriptOrgPointer;
		getScriptBaseAddress();
		_scriptPointer = _scriptOrgPointer + oldoffs;
	}
	return *_scriptPointer++;
}

uint ScummEngine::fetchScriptWord() {
	uint16 a;
	//int a;
	if (*_lastCodePtr + sizeof(MemBlkHeader) != _scriptOrgPointer) {
		uint32 oldoffs = _scriptPointer - _scriptOrgPointer;
		getScriptBaseAddress();
		_scriptPointer = _scriptOrgPointer + oldoffs;
	}
	a = READ_LE_UINT16(_scriptPointer);
	_scriptPointer += 2;

//	int b = a;
//	debug(9," fetchscriptword: %d, %d", a, b);
	return a;
}

int ScummEngine::fetchScriptWordSigned() {
//	return (int)((int16)fetchScriptWord());
	return (int16)fetchScriptWord();
}

int ScummEngine::readVar(uint var) {
	int a;
	static byte copyprotbypassed;
	if (!_copyProtection)
		copyprotbypassed = false;
	else
		copyprotbypassed = true;

	debug(DEBUG_VARS, "readvar(%d)", var);

	if (var & 0x2000 && !(_features & GF_NEW_OPCODES)) {
		a = fetchScriptWord();
		if (a & 0x2000)
			var += readVar(a & ~0x2000);
			//var += readVar(a & 0x1FFF);
		else
			var += a & 0xFFF;
		var &= ~0x2000;
		//var &= ~0x1FFF;
	}

	if (!(var & 0xF000)) {
		if (!_copyProtection) {
			if (var == 490 && _gameId == GID_MONKEY2 && !copyprotbypassed) {
				copyprotbypassed = true;
				var = 518;
			} /*else if (var == 179 && (_gameId == GID_MONKEY_VGA || _gameId == GID_MONKEY_EGA) && !copyprotbypassed) {
				copyprotbypassed = true;
				var = 266;
			} */			
		}

		checkRange(_numVariables - 1, 0, var, "Variable %d out of range(r)");
		return _scummVars[var];
	}

	if (var & 0x8000) {
		var &= 0x7FFF;
		checkRange(_numBitVariables - 1, 0, var, "Bit variable %d out of range(r)");
		return (_bitVars[var >> 3] & (1 << (var & 7))) ? 1 : 0;
	}

	if (var & 0x4000) {
		var &= 0xFFF;
		checkRange(20, 0, var, "Local variable %d out of range(r)");
		return vm.localvar[_currentScript][var];
	}

	error("Illegal varbits (r)");
	return -1;
}

void ScummEngine::writeVar(uint var, int value) {
	if (!(var & 0xF000)) {
		checkRange(_numVariables - 1, 0, var, "Variable %d out of range(w)");

		// FIXME: Find some better place to put this.
		if (var == VAR_CHARINC)
			VAR(VAR_CHARINC) = _defaultTalkDelay / 20;
		else
		{
			if (!g_slowMachine || (var != VAR_MACHINE_SPEED))
				_scummVars[var] = value;
		}

		if ((_varwatch == (int)var) || (_varwatch == 0)) {
			if (vm.slot[_currentScript].number < 100)
				debug(1, "vars[%d] = %d (via script-%d)", var, value, vm.slot[_currentScript].number);
			else
				debug(1, "vars[%d] = %d (via room-%d-%d)", var, value, _currentRoom,
							vm.slot[_currentScript].number);
		}
		return;
	}

	if (var & 0x8000) {
		var &= 0x7FFF;
		checkRange(_numBitVariables - 1, 0, var, "Bit variable %d out of range(w)");

		if (value)
			_bitVars[var >> 3] |= (1 << (var & 7));
		else
			_bitVars[var >> 3] &= ~(1 << (var & 7));
		return;
	}

	if (var & 0x4000) {
		var &= 0xFFF;
		checkRange(20, 0, var, "Local variable %d out of range(w)");
		vm.localvar[_currentScript][var] = value;
		return;
	}

	error("Illegal varbits (w)");
}

void ScummEngine::getResultPos() {
	int a;

	_resultVarNumber = fetchScriptWord();
	if (_resultVarNumber & 0x2000) {
		a = fetchScriptWord();
		if (a & 0x2000) {
			_resultVarNumber += readVar(a & ~0x2000);
		} else {
			_resultVarNumber += a & 0xFFF;
		}
		_resultVarNumber &= ~0x2000;
	}
}

void ScummEngine::setResult(int value) {
	writeVar(_resultVarNumber, value);
}

void ScummEngine::push(int a) {
	assert(_scummStackPos >= 0 && _scummStackPos < ARRAYSIZE(_vmStack));
	_vmStack[_scummStackPos++] = a;
}

int ScummEngine::pop() {
	if (_scummStackPos < 1 || _scummStackPos > ARRAYSIZE(_vmStack)) {
		error("No items on stack to pop() for %s (0x%X) at [%d-%d]", getOpcodeDesc(_opcode), _opcode, _roomResource, vm.slot[_currentScript].number);
	}

	return _vmStack[--_scummStackPos];
}

void ScummEngine::stopObjectCode() {
	ScriptSlot *ss;

	ss = &vm.slot[_currentScript];
	if (ss->cutsceneOverride == 255) {	/* FIXME: What does this? */
		warning("Cutscene for script %d has overflown. Resetting.", ss->number);
		ss->cutsceneOverride = 0;
	}

	if (ss->where != WIO_GLOBAL && ss->where != WIO_LOCAL) {
		if (ss->cutsceneOverride) {
			warning("Object %d ending with active cutscene/override (%d)", ss->number, ss->cutsceneOverride);
			ss->cutsceneOverride = 0;
		}
	} else {
		if (ss->cutsceneOverride) {
			warning("Script %d ending with active cutscene/override (%d)", ss->number, ss->cutsceneOverride);
			ss->cutsceneOverride = 0;
		}
	}
	ss->number = 0;
	ss->status = ssDead;
	_currentScript = 0xFF;
}

void ScummEngine::runInventoryScript(int i) {
	int args[16];
	memset(args, 0, sizeof(args));
	args[0] = i;
	if (VAR(VAR_INVENTORY_SCRIPT)) {
		runScript(VAR(VAR_INVENTORY_SCRIPT), 0, 0, args);
	}
}

void ScummEngine::freezeScripts(int flag) {
	int i;

	for (i = 0; i < NUM_SCRIPT_SLOT; i++) {
		if (_currentScript != i && vm.slot[i].status != ssDead && (!vm.slot[i].freezeResistant || flag >= 0x80)) {
			vm.slot[i].status |= 0x80;
			vm.slot[i].freezeCount++;
		}
	}

	for (i = 0; i < NUM_SENTENCE; i++)
		_sentence[i].freezeCount++;

	if (vm.cutSceneScriptIndex != 0xFF) {
		vm.slot[vm.cutSceneScriptIndex].status &= 0x7F;
		vm.slot[vm.cutSceneScriptIndex].freezeCount = 0;
	}
}

void ScummEngine::unfreezeScripts() {
	int i;

	for (i = 0; i < NUM_SCRIPT_SLOT; i++) {
		if (vm.slot[i].status & 0x80) {
			if (!--vm.slot[i].freezeCount) {
				vm.slot[i].status &= 0x7F;
			}
		}
	}

	for (i = 0; i < NUM_SENTENCE; i++) {
		if (_sentence[i].freezeCount > 0)
			_sentence[i].freezeCount--;
	}
}


void ScummEngine::runAllScripts() {
	int i;

	for (i = 0; i < NUM_SCRIPT_SLOT; i++)
		vm.slot[i].didexec = 0;

	// FIXME - why is _curExecScript?!? The only place it is ever set is here.
	// The outer world will only see it as consequence of the calls made in the following
	// for loop. But in that case, _curExecScript will be equal to _currentScript. Hence
	// it would seem we can replace all occurances of _curExecScript by _currentScript.
	_currentScript = 0xFF;
	for (_curExecScript = 0; _curExecScript < NUM_SCRIPT_SLOT; _curExecScript++) {
		if (vm.slot[_curExecScript].status == ssRunning && vm.slot[_curExecScript].didexec == 0) {
			_currentScript = (byte)_curExecScript;
			getScriptBaseAddress();
			getScriptEntryPoint();
			executeScript();
		}
	}
}

void ScummEngine::runExitScript() {
	if (_version > 2 && VAR(VAR_EXIT_SCRIPT))
		runScript(VAR(VAR_EXIT_SCRIPT), 0, 0, 0);
	if (_EXCD_offs) {
		int slot = getScriptSlot();
		vm.slot[slot].status = ssRunning;
		vm.slot[slot].number = 10001;
		vm.slot[slot].where = WIO_ROOM;
		vm.slot[slot].offs = _EXCD_offs;
		vm.slot[slot].freezeResistant = false;
		vm.slot[slot].recursive = false;
		vm.slot[slot].freezeCount = 0;

		vm.slot[slot].delayFrameCount = 0;

		runScriptNested(slot);
	}
	if (_version > 2 && VAR(VAR_EXIT_SCRIPT2))
		runScript(VAR(VAR_EXIT_SCRIPT2), 0, 0, 0);
}

void ScummEngine::runEntryScript() {
	if (_version > 2 && VAR(VAR_ENTRY_SCRIPT))
		runScript(VAR(VAR_ENTRY_SCRIPT), 0, 0, 0);
	if (_ENCD_offs) {
		int slot = getScriptSlot();
		vm.slot[slot].status = ssRunning;
		vm.slot[slot].number = 10002;
		vm.slot[slot].where = WIO_ROOM;
		vm.slot[slot].offs = _ENCD_offs;
		vm.slot[slot].freezeResistant = false;
		vm.slot[slot].recursive = false;
		vm.slot[slot].freezeCount = 0;
		vm.slot[slot].delayFrameCount = 0;
		runScriptNested(slot);
	}
	if (_version > 2 && VAR(VAR_ENTRY_SCRIPT2))
		runScript(VAR(VAR_ENTRY_SCRIPT2), 0, 0, 0);
}

void ScummEngine::killScriptsAndResources() {
	ScriptSlot *ss;
	int i;

	ss = vm.slot;
	for (i = 0; i < NUM_SCRIPT_SLOT; i++, ss++) {
		if (ss->where == WIO_ROOM || ss->where == WIO_FLOBJECT) {
			if (ss->cutsceneOverride != 0) {
				warning("Object %d stopped with active cutscene/override in exit", ss->number);
				ss->cutsceneOverride = 0;
			}
			ss->status = ssDead;
		} else if (ss->where == WIO_LOCAL) {
			if (ss->cutsceneOverride != 0) {
				warning("Script %d stopped with active cutscene/override in exit", ss->number);
				ss->cutsceneOverride = 0;
			}
			ss->status = ssDead;
		}
	}

	/* Nuke local object names */
	if (_newNames) {
		for (i = 0; i < _numNewNames; i++) {
			int j = _newNames[i];
			if (j && getOwner(j) == 0) {
				_newNames[i] = 0;
				nukeResource(rtObjectName, i);
			}
		}
	}
}

void ScummEngine::killAllScriptsExceptCurrent() {
	for (int i = 0; i < NUM_SCRIPT_SLOT; i++) {
		if (i != _currentScript)
			vm.slot[i].status = ssDead;
	}
}

void ScummEngine::doSentence(int verb, int objectA, int objectB) {
	SentenceTab *st;

	st = &_sentence[_sentenceNum++];

	st->verb = verb;
	st->objectA = objectA;
	st->objectB = objectB;
	st->preposition = (objectB != 0);
	st->freezeCount = 0;
}

void ScummEngine::checkAndRunSentenceScript() {
	int i;
	int localParamList[16];
	const ScriptSlot *ss;
	int sentenceScript;
	sentenceScript = VAR(VAR_SENTENCE_SCRIPT);

	memset(localParamList, 0, sizeof(localParamList));
	if (isScriptInUse(sentenceScript)) {
		ss = vm.slot;
		for (i = 0; i < NUM_SCRIPT_SLOT; i++, ss++)
			if (ss->number == sentenceScript && ss->status != ssDead && ss->freezeCount == 0)
				return;
	}

	if (!_sentenceNum || _sentence[_sentenceNum - 1].freezeCount)
		return;

	_sentenceNum--;

	if (_sentence[_sentenceNum].preposition && _sentence[_sentenceNum].objectB == _sentence[_sentenceNum].objectA)
		return;

	localParamList[0] = _sentence[_sentenceNum].verb;
	localParamList[1] = _sentence[_sentenceNum].objectA;
	localParamList[2] = _sentence[_sentenceNum].objectB;

	_currentScript = 0xFF;
	if (sentenceScript)
		runScript(sentenceScript, 0, 0, localParamList);
}

void ScummEngine::runInputScript(int a, int cmd, int mode) {
	int args[16];
	int verbScript;

	verbScript = VAR(VAR_VERB_SCRIPT);

	memset(args, 0, sizeof(args));
	args[0] = a;
	args[1] = cmd;
	args[2] = mode;
	if (verbScript)
		runScript(verbScript, 0, 0, args);
}

void ScummEngine::decreaseScriptDelay(int amount) {
	ScriptSlot *ss = vm.slot;
	int i;
	for (i = 0; i < NUM_SCRIPT_SLOT; i++, ss++) {
		if (ss->status == ssPaused) {
			ss->delay -= amount;
			if (ss->delay < 0) {
				ss->status = ssRunning;
				ss->delay = 0;
			}
		}
	}
}

bool ScummEngine::isScriptInUse(int script) const {
	int i;
	const ScriptSlot *ss = vm.slot;
	for (i = 0; i < NUM_SCRIPT_SLOT; i++, ss++)
		if (ss->number == script)
			return true;
	return false;
}

bool ScummEngine::isScriptRunning(int script) const {
	int i;
	const ScriptSlot *ss = vm.slot;
	for (i = 0; i < NUM_SCRIPT_SLOT; i++, ss++)
		if (ss->number == script && (ss->where == WIO_GLOBAL || ss->where == WIO_LOCAL) && ss->status != ssDead)
			return true;
	return false;
}

bool ScummEngine::isRoomScriptRunning(int script) const {
	int i;
	const ScriptSlot *ss = vm.slot;
	for (i = 0; i < NUM_SCRIPT_SLOT; i++, ss++)
		if (ss->number == script && ss->where == WIO_ROOM && ss->status != ssDead)
			return true;
	return false;
}

void ScummEngine::copyScriptString(byte *dst) {
	int len = resStrLen(_scriptPointer) + 1;
	while (len--)
		*dst++ = fetchScriptByte();
}

//
// Given a pointer to a Scumm string, this function returns the total byte length
// of the string data in that resource. To do so it has to understand certain
// special characters embedded into the string. The reason for this function is that
// sometimes this embedded data contains zero bytes, thus we can't just use strlen.
//
int ScummEngine::resStrLen(const byte *src) const {
	int num = 0;
	byte chr;
	if (src == NULL)
		src = _scriptPointer;
	while ((chr = *src++) != 0) {
		num++;
		if (chr == 255) {
			chr = *src++;
			num++;

			if (chr != 1 && chr != 2 && chr != 3 && chr != 8) {
				src += 2;
				num += 2;
			}
		}
	}
	return num;
}

void ScummEngine::beginCutscene(int *args) {
	int scr = _currentScript;
	vm.slot[scr].cutsceneOverride++;

	if (++vm.cutSceneStackPointer > ARRAYSIZE(vm.cutSceneData))
		error("Cutscene stack overflow");

	vm.cutSceneData[vm.cutSceneStackPointer] = args[0];
	vm.cutSceneScript[vm.cutSceneStackPointer] = 0;
	vm.cutScenePtr[vm.cutSceneStackPointer] = 0;

	vm.cutSceneScriptIndex = scr;
	if (VAR(VAR_CUTSCENE_START_SCRIPT))
		runScript(VAR(VAR_CUTSCENE_START_SCRIPT), 0, 0, args);
	vm.cutSceneScriptIndex = 0xFF;
}

void ScummEngine::endCutscene() {
	ScriptSlot *ss = &vm.slot[_currentScript];
	int args[16];

	memset(args, 0, sizeof(args));

	if (ss->cutsceneOverride > 0)	// Only terminate if active
		ss->cutsceneOverride--;

	args[0] = vm.cutSceneData[vm.cutSceneStackPointer];
	VAR(VAR_OVERRIDE) = 0;

	if (vm.cutScenePtr[vm.cutSceneStackPointer] && (ss->cutsceneOverride > 0))	// Only terminate if active
		ss->cutsceneOverride--;

	vm.cutSceneScript[vm.cutSceneStackPointer] = 0;
	vm.cutScenePtr[vm.cutSceneStackPointer] = 0;
	vm.cutSceneStackPointer--;

	if (VAR(VAR_CUTSCENE_END_SCRIPT))
		runScript(VAR(VAR_CUTSCENE_END_SCRIPT), 0, 0, args);
}

void ScummEngine::abortCutscene() {
	uint32 offs = vm.cutScenePtr[vm.cutSceneStackPointer];
	if (offs) {
		ScriptSlot *ss = &vm.slot[vm.cutSceneScript[vm.cutSceneStackPointer]];
		ss->offs = offs;
		ss->status = ssRunning;
		ss->freezeCount = 0;

		if (ss->cutsceneOverride > 0)
			ss->cutsceneOverride--;

		VAR(VAR_OVERRIDE) = 1;
		vm.cutScenePtr[vm.cutSceneStackPointer] = 0;
	}
}

void ScummEngine::beginOverride() {
	int idx;

	idx = vm.cutSceneStackPointer;
	assert(idx < 5);

	vm.cutScenePtr[idx] = _scriptPointer - _scriptOrgPointer;
	vm.cutSceneScript[idx] = _currentScript;

	// Skip the jump instruction following the override instruction
	// (the jump is responsible for "skipping" cutscenes, and the reason
	// why we record the current script position in vm.cutScenePtr).
	fetchScriptByte();
	fetchScriptWord();
	
	// FIXME: why is this here? it doesn't seem to belong here?
	VAR(VAR_OVERRIDE) = 0;
}

void ScummEngine::endOverride() {
	int idx;

	idx = vm.cutSceneStackPointer;
	assert(idx < 5);

	vm.cutScenePtr[idx] = 0;
	vm.cutSceneScript[idx] = 0;
	
	if (_version > 3)
		VAR(VAR_OVERRIDE) = 0;
}

} // End of namespace Scumm
