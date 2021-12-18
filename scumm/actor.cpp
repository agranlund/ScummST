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
 * $Header: /cvsroot/scummvm/scummvm/scumm/actor.cpp,v 1.229.2.2 2004/02/25 04:37:28 kirben Exp $
 *
 */

#include "stdafx.h"
#include "scumm/scumm.h"
#include "scumm/actor.h"
#include "scumm/boxes.h"
#include "scumm/charset.h"
#include "scumm/costume.h"
#include "scumm/resource.h"
#include "scumm/saveload.h"
#include "scumm/sound.h"

extern bool g_slowMachine;

namespace Scumm {

byte Actor::kInvalidBox = 0;
ScummEngine *Actor::_vm = 0;

void Actor::initActorClass(ScummEngine *scumm) {
	_vm = scumm;
}

Actor::Actor() {
	assert(_vm != 0);
	top = bottom = 0;
	left = right = 0;
	number = 0;
	needRedraw = needBgReset = costumeNeedsInit = visible = false;
	customPalette = true;
	flip = false;
	speedx = 8;
	speedy = 2;
	frame = 0;
	walkbox = 0;
	animProgress = 0;
	skipLimb = false;
	memset(animVariable, 0, sizeof(animVariable));
	memset(palette, 0, sizeof(palette));
	memset(sound, 0, sizeof(sound));
	memset(&cost, 0, sizeof(CostumeData));
	memset(&walkdata, 0, sizeof(ActorWalkData));
	walkdata.point3.x = 32000;

	walkScript = 0;

	initActor(1);
}

void Actor::initActor(int mode) {
	if (mode == 1) {
		costume = 0;
		room = 0;
		_pos.x = 0;
		_pos.y = 0;
		facing = 180;
	} else if (mode == 2) {
		facing = 180;
	}

	elevation = 0;
	skipLimb = false;
	width = 24;
	talkColor = 15;
	talkPosX = 0;
	talkPosY = -80;
	scaley = scalex = 0xFF;
	charset = 0;
	memset(sound, 0, sizeof(sound));
	targetFacing = facing;

	if (walkScript)
		_vm->stopScript(walkScript);
	moving = 0;

	shadow_mode = 0;
	layer = 0;

	setActorWalkSpeed(8, 2);
	animSpeed = 0;

	ignoreBoxes = false;
	forceClip = 0;
	ignoreTurns = false;
	flip = 0;

	talkFrequency = 256;
	talkPan = 64;
	talkVolume = 127;

	initFrame = 1;
	walkFrame = 2;
	standFrame = 3;
	talkStartFrame = 4;
	talkStopFrame = 5;

	walkScript = 0;
	talkScript = 0;

	_vm->_classData[number] = 0;
}

void Actor::stopActorMoving() {
	if (walkScript)
		_vm->stopScript(walkScript);
	moving = 0;
	startAnimActor(standFrame);
}

void Actor::setActorWalkSpeed(uint newSpeedX, uint newSpeedY) {
	if (newSpeedX == speedx && newSpeedY == speedy)
		return;

	speedx = newSpeedX;
	speedy = newSpeedY;

	if (moving) {
		calcMovementFactor(walkdata.next);
	}
}

int ScummEngine::getAngleFromPos(int x, int y) const {
	if (abs(y) * 2 < abs(x)) {
		if (x > 0)
			return 90;
		return 270;
	} else {
		if (y > 0)
			return 180;
		return 0;
	}
}

int Actor::calcMovementFactor(Common::Point next) {
	Common::Point actorPos(_pos);
	int diffX, diffY;
	int32 deltaXFactor, deltaYFactor;

	if (actorPos == next)
		return 0;

	diffX = next.x - actorPos.x;
	diffY = next.y - actorPos.y;
	deltaYFactor = speedy << 16;

	if (diffY < 0)
		deltaYFactor = -deltaYFactor;

	deltaXFactor = deltaYFactor * diffX;
	if (diffY != 0) {
		deltaXFactor /= diffY;
	} else {
		deltaYFactor = 0;
	}

	if ((uint) abs((int)(deltaXFactor >> 16)) > speedx) {
		deltaXFactor = speedx << 16;
		if (diffX < 0)
			deltaXFactor = -deltaXFactor;

		deltaYFactor = deltaXFactor * diffY;
		if (diffX != 0) {
			deltaYFactor /= diffX;
		} else {
			deltaXFactor = 0;
		}
	}

	walkdata.cur = actorPos;
	walkdata.next = next;
	walkdata.deltaXFactor = deltaXFactor;
	walkdata.deltaYFactor = deltaYFactor;
	walkdata.xfrac = 0;
	walkdata.yfrac = 0;

	targetFacing = _vm->getAngleFromPos(deltaXFactor, deltaYFactor);

	return actorWalkStep();
}

int Actor::remapDirection(int dir, bool is_walking) {
	int specdir;
	byte flags;
	bool flipX;
	bool flipY;

	// FIXME - It seems that at least in The Dig the original code does
	// check ignoreBoxes here. However, it breaks some animations in Loom,
	// causing Bobbin to face towards the camera instead of away from it
	// in some places: After the tree has been destroyed by lightning, and
	// when entering the dark tunnels beyond the dragon's lair at the very
	// least. Possibly other places as well.
	//
	// The Dig also checks if the actor is in the current room, but that's
	// not necessary here because we never call the function unless the
	// actor is in the current room anyway.
	
	if (!ignoreBoxes) {
		specdir = _vm->_extraBoxFlags[walkbox];
		if (specdir) {
			if (specdir & 0x8000) {
				dir = specdir & 0x3FFF;
			} else {
				specdir = specdir & 0x3FFF;
				if (specdir - 90 < dir && dir < specdir + 90)
					dir = specdir;
				else
					dir = specdir + 180;
			}
		}

		flags = _vm->getBoxFlags(walkbox);

		flipX = (walkdata.deltaXFactor > 0);
		flipY = (walkdata.deltaYFactor > 0);

		// FIXME - this special cases for the class might be necessary
		// for other games besides Loom!

		// Check for X-Flip
		if ((flags & kBoxXFlip) || isInClass(kObjectClassXFlip)) {
			dir = 360 - dir;
			flipX = !flipX;
		}
		// Check for Y-Flip
		if ((flags & kBoxYFlip) || isInClass(kObjectClassYFlip)) {
			dir = 180 - dir;
			flipY = !flipY;
		}

		switch (flags & 7) {
		case 1:
			if (is_walking)	                       // Actor is walking
				return flipX ? 90 : 270;
			else	                               // Actor is standing/turning
				return (dir == 90) ? 90 : 270;
		case 2:
			if (is_walking)	                       // Actor is walking
				return flipY ? 180 : 0;
			else	                               // Actor is standing/turning
				return (dir == 0) ? 0 : 180;
		case 3:
			return 270;
		case 4:
			return 90;
		case 5:
			return 0;
		case 6:
			return 180;
		}
	}
	// OR 1024 in to signal direction interpolation should be done
	return normalizeAngle(dir) | 1024;
}

int Actor::updateActorDirection(bool is_walking) {
	int from;
	int dirType;
	int dir;
	bool shouldInterpolate;

	if ((_vm->_version == 6) && ignoreTurns)
		return facing;

	dirType = 0;
	from = toSimpleDir(dirType, facing);
	dir = remapDirection(targetFacing, is_walking);

	shouldInterpolate = (dir & 1024) ? true : false;
	dir &= 1023;

	if (shouldInterpolate) {
		int to = toSimpleDir(dirType, dir);
		int num = dirType ? 8 : 4;

		// Turn left or right, depending on which is shorter.
		int diff = to - from;
		if (abs(diff) > (num >> 1))
			diff = -diff;

		if (diff > 0) {
			to = from + 1;
		} else if (diff < 0){
			to = from - 1;
		}

		dir = fromSimpleDir(dirType, (to + num) % num);
	}

	return dir;
}

void Actor::setBox(int box) {
	walkbox = box;
	setupActorScale();
}

int Actor::actorWalkStep() {
	int tmpX, tmpY;
	Common::Point actorPos;
	int distX, distY;
	int nextFacing;

	debug(2, "actorWalkStep %d\n\r", number);
	needRedraw = true;

	nextFacing = updateActorDirection(true);
	if (!(moving & MF_IN_LEG) || facing != nextFacing) {
		if (walkFrame != frame || facing != nextFacing) {
			startWalkAnim(1, nextFacing);
		}
		moving |= MF_IN_LEG;
	}

	actorPos = _pos;

	if (walkbox != walkdata.curbox && _vm->checkXYInBoxBounds(walkdata.curbox, actorPos.x, actorPos.y)) {
		setBox(walkdata.curbox);
	}

	distX = abs(walkdata.next.x - walkdata.cur.x);
	distY = abs(walkdata.next.y - walkdata.cur.y);

	if (abs(actorPos.x - walkdata.cur.x) >= distX && abs(actorPos.y - walkdata.cur.y) >= distY) {
		moving &= ~MF_IN_LEG;
		return 0;
	}

	tmpX = (actorPos.x << 16) + walkdata.xfrac + (walkdata.deltaXFactor >> 8) * scalex;
	walkdata.xfrac = (uint16)tmpX;
	actorPos.x = (tmpX >> 16);

	tmpY = (actorPos.y << 16) + walkdata.yfrac + (walkdata.deltaYFactor >> 8) * scaley;
	walkdata.yfrac = (uint16)tmpY;
	actorPos.y = (tmpY >> 16);

	if (abs(actorPos.x - walkdata.cur.x) > distX) {
		actorPos.x = walkdata.next.x;
	}

	if (abs(actorPos.y - walkdata.cur.y) > distY) {
		actorPos.y = walkdata.next.y;
	}

	_pos = actorPos;
	return 1;
}


void Actor::setupActorScale() {

	if (ignoreBoxes)
		return;

	// For some boxes, we ignore the scaling and use whatever values the
	// scripts set. This is used e.g. in the Mystery Vortex in Sam&Max.
	// Older games used the flag 0x20 differently, though.
	if (_vm->_version == 6 && (_vm->getBoxFlags(walkbox) & kBoxIgnoreScale))
		return;

	uint16 scale = _vm->getScale(walkbox, _pos.x, _pos.y);
	assert(scale <= 0xFF);

	scalex = scaley = (byte)scale;
}

void Actor::startAnimActor(int f) {

	switch (f) {
	case 0x38:
		f = initFrame;
		break;
	case 0x39:
		f = walkFrame;
		break;
	case 0x3A:
		f = standFrame;
		break;
	case 0x3B:
		f = talkStartFrame;
		break;
	case 0x3C:
		f = talkStopFrame;
		break;
	}

	assert(f != 0x3E);
	frame = f;

	if (isInCurrentRoom() && costume != 0) {
		animProgress = 0;
		cost.animCounter = 0;
		needRedraw = true;
		if (f == initFrame)
			cost.reset();
		_vm->cost_decodeData(this, f, (uint) - 1);
	}
}

void Actor::animateActor(int anim) {
	int cmd, dir;


	cmd = anim / 4;
	dir = oldDirToNewDir(anim % 4);

	// Convert into old cmd code
	cmd = 0x3F - cmd + 2;

	switch (cmd) {
	case 2:				// stop walking
		stopActorMoving();
		break;
	case 3:				// change direction immediatly
		moving &= ~MF_TURN;
		setDirection(dir);
		break;
	case 4:				// turn to new direction
		turnToDirection(dir);
		break;
	default:
		startAnimActor(anim);
	}
}

void Actor::setDirection(int direction) {
	uint aMask;
	int i;
	uint16 vald;

	// Do nothing if actor is already facing in the given direction
	if (facing == direction)
		return;

	// Normalize the angle
	facing = normalizeAngle(direction);

	// If there is no costume set for this actor, we are finished
	if (costume == 0)
		return;

	// Update the costume for the new direction (and mark the actor for redraw)
	aMask = 0x8000;
	for (i = 0; i < 16; i++, aMask >>= 1) {
		vald = cost.frame[i];
		if (vald == 0xFFFF)
			continue;

		_vm->cost_decodeData(this, vald, aMask);
	}

	needRedraw = true;
}

void Actor::putActor(int dstX, int dstY, byte newRoom) {
	if (visible && _vm->_currentRoom != newRoom && _vm->talkingActor() == number) {
		_vm->clearMsgQueue();
	}

	// HACK: The green transparency of the tank in the Hall of Oddities is
	// is positioned one pixel too far to the left. This appears to be a
	// bug in the original game as well.

	if (_vm->_gameId == GID_SAMNMAX && newRoom == 16 && number == 5 && dstX == 235 && dstY == 236)
		dstX++;

	_pos.x = dstX;
	_pos.y = dstY;
	room = newRoom;
	needRedraw = true;

#if defined(GAME_ATLANTIS)
	if (g_slowMachine)
	{
		// HACK: hide expensive lamp in New York
		if ((newRoom == 10 && number == 11 && dstY == 142)
		// HACK: hide expensive tree in Azores
		//|| (newRoom == 19 && (number == 11 || number == 12) && dstY == 160)
		)
		{
			_pos.y = 512;
			hideActor();
			needRedraw = false;
			needBgReset = false;
			return;
		}
	}
#endif


	if (_vm->VAR(_vm->VAR_EGO) == number) {
		_vm->_egoPositioned = true;
	}

	if (visible) {
		if (isInCurrentRoom()) {
			if (moving) {
				startAnimActor(standFrame);
				moving = 0;
			}
			adjustActorPos();
		} else {
			hideActor();
		}
	} else {
		if (isInCurrentRoom())
			showActor();
	}
}

int Actor::getActorXYPos(int &xPos, int &yPos) const {
	if (!isInCurrentRoom())
		return -1;

	xPos = _pos.x;
	yPos = _pos.y;
	return 0;
}

AdjustBoxResult Actor::adjustXYToBeInBox(int dstX, int dstY) {
	const uint thresholdTable[] = { 30, 80, 0 };
	AdjustBoxResult abr;
	int16 tmpX, tmpY;
	int tmpDist, bestDist, threshold, numBoxes;
	byte flags, bestBox;
	int box;
	const int firstValidBox = 1;

	abr.x = dstX;
	abr.y = dstY;
	abr.box = kInvalidBox;

	if (ignoreBoxes)
		return abr;

	for (int tIdx = 0; tIdx < ARRAYSIZE(thresholdTable); tIdx++) {
		threshold = thresholdTable[tIdx];

		numBoxes = _vm->getNumBoxes() - 1;
		if (numBoxes < firstValidBox)
			return abr;

		bestDist = 0xFFFF;
		bestBox = kInvalidBox;

		// We iterate (backwards) over all boxes, searching the one closest
		// to the desired coordinates.
		for (box = numBoxes; box >= firstValidBox; box--) {
			flags = _vm->getBoxFlags(box);

			// Skip over invisible boxes
			if (flags & kBoxInvisible && !(flags & kBoxPlayerOnly && !isPlayer()))
				continue;
			
			// For increased performance, we perform a quick test if
			// the coordinates can even be within a distance of 'threshold'
			// pixels of the box.
			if (threshold > 0 && _vm->inBoxQuickReject(box, dstX, dstY, threshold))
				continue;

			// Check if the point is contained in the box. If it is,
			// we don't have to search anymore.
			if (_vm->checkXYInBoxBounds(box, dstX, dstY)) {
				abr.x = dstX;
				abr.y = dstY;
				abr.box = box;
				return abr;
			}

			// Find the point in the box which is closest to our point.
			tmpDist = _vm->getClosestPtOnBox(box, dstX, dstY, tmpX, tmpY);

			// Check if the box is closer than the previous boxes.
			if (tmpDist < bestDist) {
				abr.x = tmpX;
				abr.y = tmpY;
	
				if (tmpDist == 0) {
					abr.box = box;
					return abr;
				}
				bestDist = tmpDist;
				bestBox = box;
			}
		}

		// If the closest ('best') box we found is within the threshold, or if
		// we are on the last run (i.e. threshold == 0), return that box.
		if (threshold == 0 || threshold * threshold >= bestDist) {
			abr.box = bestBox;
			return abr;
		}
	}

	return abr;
}

void Actor::adjustActorPos() {
	AdjustBoxResult abr;

	abr = adjustXYToBeInBox(_pos.x, _pos.y);

	_pos.x = abr.x;
	_pos.y = abr.y;
	walkdata.destbox = abr.box;

	setBox(abr.box);

	walkdata.dest.x = -1;

	moving = 0;
	cost.soundCounter = 0;

	if (walkScript)
		_vm->stopScript(walkScript);

	if (walkbox != kInvalidBox) {
		byte flags = _vm->getBoxFlags(walkbox);
		if (flags & 7) {
			turnToDirection(facing);
		}
	}
}

void Actor::faceToObject(int obj) {
	int x2, y2, dir;
	
	if (!isInCurrentRoom())
		return;

	if (_vm->getObjectOrActorXY(obj, x2, y2) == -1)
		return;

	dir = (x2 > _pos.x) ? 90 : 270;
	turnToDirection(dir);
}

void Actor::turnToDirection(int newdir) {
	if (newdir == -1 || ignoreTurns)
		return;

	moving &= ~MF_TURN;

	if (newdir != facing) {
		moving |= MF_TURN;
		targetFacing = newdir;
	}
}

void Actor::hideActor() {
	if (!visible)
		return;

	if (moving) {
		startAnimActor(standFrame);
		moving = 0;
	}
	visible = false;
	cost.soundCounter = 0;
	needRedraw = false;
	needBgReset = true;
}

void Actor::showActor() {
	if (_vm->_currentRoom == 0 || visible)
		return;

	adjustActorPos();

	_vm->ensureResourceLoaded(rtCostume, costume);

	if (costumeNeedsInit) {
		startAnimActor(initFrame);
		costumeNeedsInit = false;
	}

	debug(2, "showActor %d\n\r", number);
	moving = 0;
	visible = true;
	needRedraw = true;
}

// V1 Maniac doesn't have a ScummVar for VAR_TALK_ACTOR, and just uses
// an internal variable. Emulate this to prevent overwriting script vars...
int ScummEngine::talkingActor() {
	return VAR(VAR_TALK_ACTOR);
};

void ScummEngine::talkingActor(int value) {
	VAR(VAR_TALK_ACTOR) = value;
};

void ScummEngine::showActors() {
	int i;

	for (i = 1; i < _numActors; i++) {
		if (_actors[i].isInCurrentRoom())
			_actors[i].showActor();
	}
}

void ScummEngine::walkActors() {
	int i;

	for (i = 1; i < _numActors; i++) {
		if (_actors[i].isInCurrentRoom())
			_actors[i].walkActor();
	}
}

/* Used in Scumm v5 only. Play sounds associated with actors */
void ScummEngine::playActorSounds() {
	int i;

	for (i = 1; i < _numActors; i++) {
		if (_actors[i].cost.soundCounter && _actors[i].isInCurrentRoom() && _actors[i].sound) {
			_currentScript = 0xFF;
			_sound->addSoundToQueue(_actors[i].sound[0]);
			for (i = 1; i < _numActors; i++) {
				_actors[i].cost.soundCounter = 0;
			}
			return;
		}
	}
}

#ifndef RELEASEBUILD
Actor *ScummEngine::derefActor(int id, const char *errmsg) const {
	if (id == 0)
		debugC(DEBUG_ACTORS, "derefActor(0, \"%s\") in script %d, opcode 0x%x", 
			errmsg, vm.slot[_curExecScript].number, _opcode);

	if (id < 0 || id >= _numActors || _actors[id].number != id) {
		if (errmsg)
			error("Invalid actor %d in %s", id, errmsg);
		else
			error("Invalid actor %d", id);
	}
	return &_actors[id];
}

Actor *ScummEngine::derefActorSafe(int id, const char *errmsg) const {
	if (id == 0)
		debugC(DEBUG_ACTORS, "derefActorSafe(0, \"%s\") in script %d, opcode 0x%x", 
			errmsg, vm.slot[_curExecScript].number, _opcode);

	if (id < 0 || id >= _numActors || _actors[id].number != id) {
		debugC(DEBUG_ACTORS, "Invalid actor %d in %s (script %d, opcode 0x%x)",
			 id, errmsg, vm.slot[_curExecScript].number, _opcode);
		return NULL;
	}
	return &_actors[id];
}

#else

Actor *ScummEngine::derefActorNoMsg(int id) const {
	return &_actors[id];
}

#endif

static int compareDrawOrder(const void* a, const void* b)
{
	const Actor* actor1 = *(const Actor *const*)a;
	const Actor* actor2 = *(const Actor *const*)b;
	int diff;

	// The actor in the higher layer is ordered lower
	diff = actor1->layer - actor2->layer;
	if (diff < 0)
		return +1;
	if (diff > 0)
		return -1;

	// The actor with higher y value is ordered higher
	diff = actor1->_pos.y - actor2->_pos.y;
	if (diff < 0)
		return -1;
	if (diff > 0)
		return +1;

	// The qsort() function is not guaranteed to be stable (i.e. it may
	// re-order "equal" elements in an array it sorts). Hence we use the
	// actor number as tie-breaker. This is needed for the Sam & Max intro,
	// and possibly other cases as well. See bug #758167.

	return actor1->number - actor2->number;
}

void ScummEngine::processActors() {
	int numactors = 0;

	// TODO : put this actors as a member array. It never has to grow or shrink
	// since _numActors is constant within a game.
	//Actor** actors = new Actor * [_numActors];

	// Make a list of all actors in this room
	Actor* actors[MAX_NUM_ACTORS];
	for (int i = 1; i < _numActors; i++) {
		if (_actors[i].isInCurrentRoom() && _actors[i].costume)
			actors[numactors++] = &_actors[i];
	}
	if (!numactors) {
		//delete [] actors;
		return;
	}

	// Sort actors by position before we draw them (to ensure that actors in
	// front are drawn after those "behind" them).
	qsort(actors, numactors, sizeof (Actor*), compareDrawOrder);

	Actor** end = actors + numactors;

	// Finally draw the now sorted actors
	for (Actor** ac = actors; ac != end; ++ac) {
		Actor* a = *ac;
		CHECK_HEAP
		a->drawActorCostume();
		CHECK_HEAP
		a->animateCostume();
	}
	
	//delete [] actors;
}

// Used in Scumm v8, to allow the verb coin to be drawn over the inventory
// chest. I'm assuming that draw order won't matter here.
void ScummEngine::processUpperActors() {
	int i;

	for (i = 1; i < _numActors; i++) {
		if (_actors[i].isInCurrentRoom() && _actors[i].costume && _actors[i].layer < 0) {
			CHECK_HEAP
			_actors[i].drawActorCostume();
			CHECK_HEAP
			_actors[i].animateCostume();
		}
	}
}

void Actor::drawActorCostume() {
	if (!needRedraw)
		return;

//	debug(1, "drawActorCostume");

	needRedraw = false;

	setupActorScale();

	BaseCostumeRenderer* bcr = _vm->_costumeRenderer;

	bcr->_actorID = number;

	bcr->_actorX = _pos.x - _vm->virtscr[0].xstart;
	bcr->_actorY = _pos.y - elevation;
	bcr->_scaleX = scalex;
	bcr->_scaleY = scaley;

	bcr->setCostume(costume);
	bcr->setPalette(customPalette ? palette : 0);
	bcr->setFacing(this);

	if (forceClip)
		bcr->_zbuf = forceClip;
	else if (isInClass(kObjectClassNeverClip))
		bcr->_zbuf = 0;
	else {
		bcr->_zbuf = _vm->getMaskFromBox(walkbox);
		if (bcr->_zbuf > _vm->gdi._numZBuffer-1)
			bcr->_zbuf = _vm->gdi._numZBuffer-1;
	}

	bcr->_draw_top = top = 0xFF;
	bcr->_draw_left = left = 0x7FFF;
	bcr->_draw_bottom = bottom = 0;
	bcr->_draw_right = right = 0;
	bcr->_skipLimb = (skipLimb != 0);

#if 1
	bcr->drawCostume(_vm->virtscr[0], cost);
#else
	// If the actor is partially hidden, redraw it next frame.
	// Only done for pre-AKOS, though.
	if (bcr->drawCostume(_vm->virtscr[0], cost) & 1)
	{
		debug(2, "drawActorCostume ph %d\n\r", number);
		needRedraw = true;
	}
#endif
	

	// Record the vertical extent of the drawn actor
	top = bcr->_draw_top;
	bottom = bcr->_draw_bottom;
	left = ((bcr->_draw_left >> 3) + _vm->_screenStartStrip) << 3;
	right = (((bcr->_draw_right+7) >> 3) + _vm->_screenStartStrip) << 3;
}

void Actor::animateCostume() {
	if (costume == 0)
		return;

	animProgress++;
	if (animProgress >= animSpeed) {
		animProgress = 0;

		LoadedCostume lc(_vm);
		lc.loadCostume(costume);
		if (lc.increaseAnims(this))
		{
			debug(2, "animatecostume %d\n\r", number);
			needRedraw = true;
		}
	}
}

void ScummEngine::setActorRedrawFlags() {
	static bool wasFullRedraw = false;
	if (_fullRedraw || wasFullRedraw)
	{
		for (int16 j = 1; j < _numActors; j++) {
			_actors[j].needRedraw = true;
		}
		wasFullRedraw = _fullRedraw;
	}
	else
	{
	#if 1

		bool cameraMoved = (camera._last.x != camera._cur.x);

		// actors which has not yet been drawn will do a rough check against gfxUsageBits.
		// redraw if BG is marked as dirty or if another actor occupies the same strips.
		for (int16 strip = _screenStartStrip; strip < _screenStartStrip + SCREEN_STRIP_COUNT; strip++) {
			uint32 usageBits = gfxUsageBits[strip];
			if (usageBits & 0x3FFFFFFF) {
				uint32 actorBit = 2;
				for (int16 j = 1; j < _numActors; j++) {
					if ((usageBits & actorBit) && (usageBits & ~actorBit)) {
						if ((_actors[j].right <= _actors[j].left) || cameraMoved) {
							_actors[j].needRedraw = true;
						}
					}
					actorBit <<= 1;
				}
			}
		}

		// fetch actors we care about
		Actor* activeActors[30];
		int16 numActiveActors = 0;
		for (int16 j=1; j<_numActors; j++) {
			Actor* a = &_actors[j];
			if ((a->right > a->left) && a->isInCurrentRoom() && a->costume)
				activeActors[numActiveActors++] = a;
		}

		// mark actors for redraw if background is dirty
		VirtScreen* vs = &virtscr[0];
		int16 startx = (_screenStartStrip << 3);
		for (int16 j = 0; j < numActiveActors; j++) {
			Actor* a = activeActors[j];
			if (!a->needRedraw) {
				int16 l = a->left-startx; int16 r = a->right-startx;
				if(vs->dirty.isDirty(l, a->top, r, a->bottom)) {
					a->needRedraw = true;
				}
			}
		}	

		// mark actors for redraw if any intersecting actor is redrawing
		Common::DirtyMask dirty;
		dirty.clear();
		for (int16 j = 0; j < numActiveActors; j++) {
			Actor* a = activeActors[j];
			if (a->needRedraw) {
				int16 l = a->left-startx; int16 r = a->right-startx;
				dirty.updateRect(l, a->top, r, a->bottom);
			}
		}
		for (int16 j = 0; j < numActiveActors; j++) {
			Actor* a = activeActors[j];
			if (!a->needRedraw) {
				int16 l = a->left-startx; int16 r = a->right-startx;
				if (dirty.isDirty(l, a->top, r, a->bottom)) {
					dirty.updateRect(l, a->top, r, a->bottom);
					a->needRedraw = true;
					j = 0;
					continue;
				}
			}
		}

	#else

		for (int16 i = 0; i < SCREEN_STRIP_COUNT; i++)
		{
			int strip = _screenStartStrip + i;
			uint32 usageBits = gfxUsageBits[strip];

			// ignore strip if no actors are here
			if ((usageBits & 0x3FFFFFFF) == 0)
				continue;

			uint32 actorBit = 2;
			for (int16 j = 1; j < _numActors; j++)
			{
				// is actor in this strip?
				if (usageBits & actorBit)
				{
					// is background dirty, or is another actor in the same strip?
					if (usageBits & ~actorBit)
					{
						// redraw
						_actors[j].needRedraw = true;
					}
				}
				actorBit <<= 1;
			}
		}

	#endif
	}
}

void ScummEngine::resetActorBgs() {
	uint16 resetList[30];
	for (int16 i = 0; i < SCREEN_STRIP_COUNT; i++)
	{
		// find spans to reset
		uint16 resetCount = 0;
		int strip = _screenStartStrip + i;
		clearGfxUsageBit(strip, USAGE_BIT_DIRTY);
		clearGfxUsageBit(strip, USAGE_BIT_RESTORED);

		for (int16 j = 1; j < _numActors; j++)
		{
			if (testGfxUsageBit(strip, j) && (_actors[j].needRedraw || _actors[j].needBgReset))
			{
				clearGfxUsageBit(strip, j);

				if (_actors[j].bottom <= 0)
					continue;
				if (_actors[j].top >= SCREEN_HEIGHT)
					continue;
				if (_actors[j].bottom < _actors[j].top)
					continue;

				resetList[resetCount++] = _actors[j].top << 8 | _actors[j].bottom;
				//gdi.resetBackground(_actors[j].top, _actors[j].bottom, i);
			}
		}

		// merge intersecting spans
		for (int16 j = resetCount-1; j > 0; j--)
		{
			uint16 t0 = resetList[j] & 0xFF00;
			uint16 b0 = resetList[j] & 0x00FF;
			for (int16 k = j - 1; k >= 0; k--)
			{
				uint16 t1 = resetList[k] & 0xFF00;
				uint16 b1 = resetList[k] & 0x00FF;
				if (((t0>>8) < b1) && (b0 > (t1>>8)))
				{
					if (t0 < t1) t1 = t0;
					if (b0 > b1) b1 = b0;
					resetList[k] = t1 | b1;
					resetList[j] = 0;
					break;
				}
			} 
		}

		// reset background
		for (int16 j = 0; j < resetCount; j++)
		{
			uint16 t = resetList[j];
			if ((t & 0xFF) != 0)
			{
				gdi.resetBackground(t>>8, t&0xFF, i);
			}
		}
	}

	for (int16 i = 1; i < _numActors; i++)
	{
		_actors[i].needBgReset = false;
	}
}

int ScummEngine::getActorFromPos(int x, int y) {
	if (!testGfxAnyUsageBits(x / 8))
		return 0;

	for (int16 i = 1; i < _numActors; i++)
	{
		if (testGfxUsageBit(x / 8, i) && !getClass(i, kObjectClassUntouchable) && y >= _actors[i].top && y <= _actors[i].bottom)
		{
//			if (_version > 2 || i != VAR(VAR_EGO))
				return i;
		}
	}
	return 0;
}

void ScummEngine::actorTalk(){
	Actor *a;

	_msgPtrToAdd = _charsetBuffer;
	_messagePtr = addMessageToStack(_messagePtr);
	assert((int)(_msgPtrToAdd - _charsetBuffer) < (int)(sizeof(_charsetBuffer)));

	if (_actorToPrintStrFor == 0xFF) {
		if (!_keepText)
			stopTalk();
		talkingActor(0xFF);
	} else {
		int oldact;
		
		a = derefActor(_actorToPrintStrFor, "actorTalk");
		if (!a->isInCurrentRoom()) {
			oldact = 0xFF;
		} else {
			if (!_keepText)
				stopTalk();
			talkingActor(a->number);
			if (!_string[0].no_talk_anim) {
				a->startAnimActor(a->talkStartFrame);
				_useTalkAnims = true;
			}
			oldact = talkingActor();
		}
		if (oldact >= 0x80)
			return;
	}

	if (talkingActor() > 0x7F) {
		_charsetColor = (byte)_string[0].color;
	} else {
		a = derefActor(talkingActor(), "actorTalk(2)");
		_charsetColor = a->talkColor;
	}
	_charsetBufPos = 0;
	_talkDelay = 0;
	_haveMsg = 0xFF;
	VAR(VAR_HAVE_MSG) = 0xFF;
	if (VAR_CHARCOUNT != 0xFF)
		VAR(VAR_CHARCOUNT) = 0;
	CHARSET_1();
}

void ScummEngine::stopTalk() {
	int act;

	_sound->stopTalkSound();

	_haveMsg = 0;
	_talkDelay = 0;

	act = talkingActor();
	if (act && act < 0x80) {
		Actor *a = derefActor(act, "stopTalk");
		if (a->isInCurrentRoom() && _useTalkAnims) {
			a->startAnimActor(a->talkStopFrame);
			_useTalkAnims = false;
		}
		talkingActor(0xFF);
	}
	_keepText = false;
	_charset->restoreCharsetBg();
}

void ScummEngine::clearMsgQueue() {
	_messagePtr = (const byte *)" ";
	stopTalk();
}

void Actor::setActorCostume(int c) {
	int i;

	costumeNeedsInit = true;
	
	if (visible) {
		hideActor();
		cost.reset();
		costume = c;
		showActor();
	} else {
		costume = c;
		cost.reset();
	}

	customPalette = false;
	for (i = 0; i < 32; i++)
		palette[i] = 0xFF;
}

void Actor::startWalkActor(int destX, int destY, int dir) {
	AdjustBoxResult abr;
	abr = adjustXYToBeInBox(destX, destY);

	if (!isInCurrentRoom()) {
		_pos.x = abr.x;
		_pos.y = abr.y;
		if (!(_vm->_version == 6 && ignoreTurns) && dir != -1)
			setDirection(dir);
		return;
	}

	if (ignoreBoxes) {
		abr.box = kInvalidBox;
		walkbox = kInvalidBox;
	} else {
		if (_vm->checkXYInBoxBounds(walkdata.destbox, abr.x, abr.y)) {
			abr.box = walkdata.destbox;
		} else {
			abr = adjustXYToBeInBox(abr.x, abr.y);
		}
		if (moving && walkdata.destdir == dir && walkdata.dest.x == abr.x && walkdata.dest.y == abr.y)
			return;
	}

	if (_pos.x == abr.x && _pos.y == abr.y) {
		turnToDirection(dir);
		return;
	}

	walkdata.dest.x = abr.x;
	walkdata.dest.y = abr.y;
	walkdata.destbox = abr.box;
	walkdata.destdir = dir;
	moving = (moving & MF_IN_LEG) | MF_NEW_LEG;
	walkdata.point3.x = 32000;

	walkdata.curbox = walkbox;
}

void Actor::startWalkAnim(int cmd, int angle) {
	if (angle == -1)
		angle = facing;

	/* FIXME: (yazoo/fingolfin): using the walk script is buggy in Dig,
	 * troubles while walking. It's disabled until we can figure out how
	 * to fix this properly.
	 * Note: walk scripts aren't required to make the game
	 * work as usual
	 */
#if 1
	if (walkScript != 0) {
		int args[16];
		memset(args, 0, sizeof(args));
		args[0] = number;
		args[1] = cmd;
		args[2] = angle;
		_vm->runScript(walkScript, 1, 0, args);
	} else
#endif
	{
		switch (cmd) {
		case 1:										/* start walk */
			setDirection(angle);
			startAnimActor(walkFrame);
			break;
		case 2:										/* change dir only */
			setDirection(angle);
			break;
		case 3:										/* stop walk */
			turnToDirection(angle);
			startAnimActor(standFrame);
			break;
		}
	}
}

void Actor::walkActor() {
	int new_dir, next_box;
	Common::Point foundPath;

	if (!moving)
		return;

	if (!(moving & MF_NEW_LEG)) {
		if (moving & MF_IN_LEG && actorWalkStep())
			return;

		if (moving & MF_LAST_LEG) {
			moving = 0;
			setBox(walkdata.destbox);
			startWalkAnim(3, walkdata.destdir);
			return;
		}

		if (moving & MF_TURN) {
			new_dir = updateActorDirection(false);
			if (facing != new_dir)
				setDirection(new_dir);
			else
				moving = 0;
			return;
		}

		setBox(walkdata.curbox);
		moving &= MF_IN_LEG;
	}

	moving &= ~MF_NEW_LEG;
	do {

		if (walkbox == kInvalidBox) {
			setBox(walkdata.destbox);
			walkdata.curbox = walkdata.destbox;
			break;
		}

		if (walkbox == walkdata.destbox)
			break;

		next_box = _vm->getPathToDestBox(walkbox, walkdata.destbox);
		if (next_box < 0) {
			walkdata.destbox = walkbox;
			moving |= MF_LAST_LEG;
			return;
		}

		walkdata.curbox = next_box;
		
		if (findPathTowards(walkbox, next_box, walkdata.destbox, foundPath))
			break;

		if (calcMovementFactor(foundPath))
			return;

		setBox(walkdata.curbox);
	} while (1);

	moving |= MF_LAST_LEG;
	calcMovementFactor(walkdata.dest);
}

/*
void Actor::walkActorV12() {
	Common::Point foundPath, tmp;
	int new_dir, next_box;

	if (moving & MF_TURN) {
		new_dir = updateActorDirection(false);
		if (facing != new_dir)
			setDirection(new_dir);
		else
			moving = 0;
		return;
	}
	
	if (!moving)
		return;
	
	if (moving & MF_IN_LEG) {
		actorWalkStep();
	} else {
		if (moving & MF_LAST_LEG) {
			moving = 0;
			startWalkAnim(3, walkdata.destdir);
		} else {
			setBox(walkdata.curbox);
			if (walkbox == walkdata.destbox) {
				foundPath = walkdata.dest;
				moving |= MF_LAST_LEG;
			} else {
				next_box = _vm->getPathToDestBox(walkbox, walkdata.destbox);
				if (next_box < 0) {
					moving |= MF_LAST_LEG;
					return;
				}
		
				// Can't walk through locked boxes
				int flags = _vm->getBoxFlags(next_box);
				if (flags & kBoxLocked && !(flags & kBoxPlayerOnly && !isPlayer())) {
					moving |= MF_LAST_LEG;
				}

				walkdata.curbox = next_box;

				_vm->getClosestPtOnBox(walkdata.curbox, x, y, tmp.x, tmp.y);
				_vm->getClosestPtOnBox(walkbox, tmp.x, tmp.y, foundPath.x, foundPath.y);
			}
			calcMovementFactor(foundPath);
		}
	}
}
*/


byte *Actor::getActorName() {
	byte *ptr = _vm->getResourceAddress(rtActorName, number);
	if (ptr == NULL) {
		warning("Failed to find name of actor %d", number);
	}
	return ptr;
}

void Actor::classChanged(int cls, bool value) {
	if (cls == kObjectClassAlwaysClip)
		forceClip = value;
	if (cls == kObjectClassIgnoreBoxes)
		ignoreBoxes = value;
}

bool Actor::isInClass(int cls) {
	return _vm->getClass(number, cls);
}

bool Actor::isPlayer() {
	return isInClass(kObjectClassPlayer);
}


const SaveLoadEntry *Actor::getSaveLoadEntries() {
	static const SaveLoadEntry actorEntries[] = {
		MKLINE(Actor, _pos.x, sleInt16, VER(8)),
		MKLINE(Actor, _pos.y, sleInt16, VER(8)),
		MKLINE(Actor, top, sleInt16, VER(8)),
		MKLINE(Actor, bottom, sleInt16, VER(8)),
		MKLINE(Actor, elevation, sleInt16, VER(8)),
		MKLINE(Actor, width, sleUint16, VER(8)),
		MKLINE(Actor, facing, sleUint16, VER(8)),
		MKLINE(Actor, costume, sleUint16, VER(8)),
		MKLINE(Actor, room, sleByte, VER(8)),
		MKLINE(Actor, talkColor, sleByte, VER(8)),
		MKLINE(Actor, talkFrequency, sleInt16, VER(16)),
		MKLINE(Actor, talkPan, sleInt16, VER(24)),
		MKLINE(Actor, talkVolume, sleInt16, VER(29)),
		MKLINE(Actor, scalex, sleByte, VER(8)),
		MKLINE(Actor, scaley, sleByte, VER(8)),
		MKLINE(Actor, charset, sleByte, VER(8)),
		MKARRAY(Actor, sound[0], sleByte, 8, VER(8)),
		MKARRAY(Actor, animVariable[0], sleUint16, 8, VER(8)),
		MKLINE(Actor, targetFacing, sleUint16, VER(8)),
		MKLINE(Actor, moving, sleByte, VER(8)),
		MKLINE(Actor, ignoreBoxes, sleByte, VER(8)),
		MKLINE(Actor, forceClip, sleByte, VER(8)),
		MKLINE(Actor, initFrame, sleByte, VER(8)),
		MKLINE(Actor, walkFrame, sleByte, VER(8)),
		MKLINE(Actor, standFrame, sleByte, VER(8)),
		MKLINE(Actor, talkStartFrame, sleByte, VER(8)),
		MKLINE(Actor, talkStopFrame, sleByte, VER(8)),
		MKLINE(Actor, speedx, sleUint16, VER(8)),
		MKLINE(Actor, speedy, sleUint16, VER(8)),
		MKLINE(Actor, cost.animCounter, sleUint16, VER(8)),
		MKLINE(Actor, cost.soundCounter, sleByte, VER(8)),
	
		// Actor palette grew from 64 to 256 bytes
		MKLINE(Actor, customPalette, sleByte, VER(33)),
		MKARRAY_OLD(Actor, palette[0], sleByte, 64, VER(8), VER(9)),
		MKARRAY(Actor, palette[0], sleByte, 256, VER(10)),
	
		MK_OBSOLETE(Actor, mask, sleByte, VER(8), VER(9)),
		MKLINE(Actor, shadow_mode, sleByte, VER(8)),
		MKLINE(Actor, visible, sleByte, VER(8)),
		MKLINE(Actor, frame, sleByte, VER(8)),
		MKLINE(Actor, animSpeed, sleByte, VER(8)),
		MKLINE(Actor, animProgress, sleByte, VER(8)),
		MKLINE(Actor, walkbox, sleByte, VER(8)),
		MKLINE(Actor, needRedraw, sleByte, VER(8)),
		MKLINE(Actor, needBgReset, sleByte, VER(8)),
		MKLINE(Actor, costumeNeedsInit, sleByte, VER(8)),
	
		MKLINE(Actor, talkPosY, sleInt16, VER(8)),
		MKLINE(Actor, talkPosX, sleInt16, VER(8)),
		MKLINE(Actor, ignoreTurns, sleByte, VER(8)),
	
		MKLINE(Actor, layer, sleByte, VER(8)),
	
		MKLINE(Actor, talkScript, sleUint16, VER(8)),
		MKLINE(Actor, walkScript, sleUint16, VER(8)),
	
		MKLINE(Actor, walkdata.dest.x, sleInt16, VER(8)),
		MKLINE(Actor, walkdata.dest.y, sleInt16, VER(8)),
		MKLINE(Actor, walkdata.destbox, sleByte, VER(8)),
		MKLINE(Actor, walkdata.destdir, sleUint16, VER(8)),
		MKLINE(Actor, walkdata.curbox, sleByte, VER(8)),
		MKLINE(Actor, walkdata.cur.x, sleInt16, VER(8)),
		MKLINE(Actor, walkdata.cur.y, sleInt16, VER(8)),
		MKLINE(Actor, walkdata.next.x, sleInt16, VER(8)),
		MKLINE(Actor, walkdata.next.y, sleInt16, VER(8)),
		MKLINE(Actor, walkdata.deltaXFactor, sleInt32, VER(8)),
		MKLINE(Actor, walkdata.deltaYFactor, sleInt32, VER(8)),
		MKLINE(Actor, walkdata.xfrac, sleUint16, VER(8)),
		MKLINE(Actor, walkdata.yfrac, sleUint16, VER(8)),
	
		MKARRAY(Actor, cost.active[0], sleByte, 16, VER(8)),
		MKLINE(Actor, cost.stopped, sleUint16, VER(8)),
		MKARRAY(Actor, cost.curpos[0], sleUint16, 16, VER(8)),
		MKARRAY(Actor, cost.start[0], sleUint16, 16, VER(8)),
		MKARRAY(Actor, cost.end[0], sleUint16, 16, VER(8)),
		MKARRAY(Actor, cost.frame[0], sleUint16, 16, VER(8)),
		MKEND()
	};
	
	return actorEntries;
}

} // End of namespace Scumm
