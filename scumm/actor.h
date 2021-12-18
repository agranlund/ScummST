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
 * $Header: /cvsroot/scummvm/scummvm/scumm/actor.h,v 1.51 2004/02/09 08:05:23 aquadran Exp $
 *
 */


#ifndef ACTOR_H
#define ACTOR_H

#include "common/scummsys.h"
#include "scumm/scumm.h"
namespace Scumm {

enum MoveFlags {
	MF_NEW_LEG = 1,
	MF_IN_LEG = 2,
	MF_TURN = 4,
	MF_LAST_LEG = 8,
	MF_FROZEN = 0x80
};

struct ActorWalkData {
	Common::Point dest;						// Final destination
	byte destbox;
	int16 destdir;
	Common::Point cur;										// Current position
	byte curbox;
	Common::Point next;							// Next position on our way to the destination
	Common::Point point3;
	int32 deltaXFactor, deltaYFactor;
	uint16 xfrac, yfrac;
};

struct CostumeData {
	byte active[16];
	uint16 animCounter;
	byte soundCounter;
	uint16 stopped;
	uint16 curpos[16];
	uint16 start[16];
	uint16 end[16];
	uint16 frame[16];

	void reset() {
		stopped = 0;
		for (int i = 0; i < 16; i++) {
			active[i] = 0;
			curpos[i] = start[i] = end[i] = frame[i] = 0xFFFF;
		}
	}
};

struct AdjustBoxResult {	/* Result type of AdjustBox functions */
	int16 x, y;
	byte box;
};

struct SaveLoadEntry;

class Actor {

public:
	static byte kInvalidBox;
	
	static void initActorClass(ScummEngine *scumm);

public:
	Common::Point _pos;
	int top, bottom;
	int left, right;
	uint width;
	byte number;
	uint16 costume;
	byte room;
	byte talkColor;
	int talkFrequency;
	byte talkPan;
	byte talkVolume;
	byte scalex, scaley;
	byte charset;
	byte moving;
	bool ignoreBoxes;
	byte forceClip;
	byte initFrame, walkFrame, standFrame, talkStartFrame, talkStopFrame;
	bool needRedraw, needBgReset, visible;
	bool customPalette;
	byte shadow_mode;
	bool flip;
	byte frame;
	byte walkbox;
	int16 talkPosX, talkPosY;
	uint16 talkScript, walkScript;
	bool ignoreTurns;
	bool skipLimb;
	int8 layer;
	uint16 sound[8];
	CostumeData cost;
protected:
	byte palette[256];
	int elevation;
	uint16 facing;
	uint16 targetFacing;
	uint speedx, speedy;
	byte animProgress, animSpeed;
	bool costumeNeedsInit;
	ActorWalkData walkdata;
	int16 animVariable[16];

	static ScummEngine *_vm;

public:

	Actor();
	
//protected:
	void hideActor();
	void showActor();

	void initActor(int mode);
	void putActor(int x, int y, byte room);
	void setActorWalkSpeed(uint newSpeedX, uint newSpeedY);
protected:
	int calcMovementFactor(Common::Point next);
	int actorWalkStep();
	int remapDirection(int dir, bool is_walking);
	void setupActorScale();

	void setBox(int box);
	int updateActorDirection(bool is_walking);

public:
	void adjustActorPos();
	AdjustBoxResult adjustXYToBeInBox(int dstX, int dstY);

	void setDirection(int direction);
	void faceToObject(int obj);
	void turnToDirection(int newdir);
	void walkActor();
	void drawActorCostume();
	void animateCostume();
	void setActorCostume(int c);
	
	byte *getActorName();
	void startWalkActor(int x, int y, int dir);
	void stopActorMoving();
protected:
	void startWalkAnim(int cmd, int angle);
public:
	void startAnimActor(int frame);

	void animateActor(int anim);

	bool isInCurrentRoom() const {
		return room == _vm->_currentRoom;
	}
	
	int getActorXYPos(int &x, int &y) const;

	int getRoom() const {
		return room;
	}
	
	int getFacing() const {
		return facing;
	}

	int getAnimVar(byte var) const {
		return animVariable[var];
	}
	void setAnimVar(byte var, int value) {
		animVariable[var] = value;
	}
	
	void setAnimSpeed(byte newAnimSpeed) {
		animSpeed = newAnimSpeed;
		animProgress = 0;
	}

	int getElevation() const {
		return elevation;
	}
	
	void setElevation(int newElevation) {
		if (elevation != newElevation) {
			elevation = newElevation;
			needRedraw = true;
		}
	}
	
	void setPalette(int idx, int val) {
		debug(2, "actor %d - setPalette %d, %d\n\r", number, idx, val);
		palette[idx] = val;
		needRedraw = true;
		customPalette = true;
	}

	byte getPalette(int idx) {
		return palette[idx];
	}
	
	void setScale(int sx, int sy) {
		debug(2, "actor %d - setScale %d, %d\n\r", number, sx, sy);
		if (sx != -1)
			scalex = sx;
		if (sy != -1)
			scaley = sy;
		needRedraw = true;
	}

	void classChanged(int cls, bool value);
	
	// Used by the save/load syste:
	static const SaveLoadEntry *getSaveLoadEntries();
	
protected:
	bool isInClass(int cls);
	
	bool isPlayer();

	bool findPathTowards(byte box, byte box2, byte box3, Common::Point &foundPath);
};

} // End of namespace Scumm

#endif
