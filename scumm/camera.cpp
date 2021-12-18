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
 * $Header: /cvsroot/scummvm/scummvm/scumm/camera.cpp,v 2.23 2004/01/08 21:21:40 fingolfin Exp $
 *
 */

#include "stdafx.h"
#include "scumm/scumm.h"
#include "scumm/intern.h"
#include "scumm/actor.h"
#include "scumm/charset.h"

namespace Scumm {

void ScummEngine::setCameraAtEx(int at) {
	camera._mode = kNormalCameraMode;
	camera._cur.x = at;
	setCameraAt(at, 0);
	camera._movingToActor = false;
}

void ScummEngine::setCameraAt(int pos_x, int pos_y) {
	if (camera._mode != kFollowActorCameraMode || abs(pos_x - camera._cur.x) > (SCREEN_WIDTH / 2)) {
		camera._cur.x = pos_x;
	}
	camera._dest.x = pos_x;

	if (camera._cur.x < VAR(VAR_CAMERA_MIN_X))
		camera._cur.x = (short) VAR(VAR_CAMERA_MIN_X);

	if (camera._cur.x > VAR(VAR_CAMERA_MAX_X))
		camera._cur.x = (short) VAR(VAR_CAMERA_MAX_X);

	if (VAR_SCROLL_SCRIPT != 0xFF && VAR(VAR_SCROLL_SCRIPT)) {
		VAR(VAR_CAMERA_POS_X) = camera._cur.x;
		runScript(VAR(VAR_SCROLL_SCRIPT), 0, 0, 0);
	}

	// If the camera moved and text is visible, remove it
	if (camera._cur.x != camera._last.x && _charset->_hasMask && _version > 3)
		stopTalk();
}


void ScummEngine::setCameraFollows(Actor *a) {

	int t, i;

	camera._mode = kFollowActorCameraMode;
	camera._follows = a->number;

	if (!a->isInCurrentRoom()) {
		startScene(a->getRoom(), 0, 0);
		camera._mode = kFollowActorCameraMode;
		camera._cur.x = a->_pos.x;
		setCameraAt(camera._cur.x, 0);
	}

	t = SCREEN_TO_STRIP(a->_pos.x);

	if (t - _screenStartStrip < camera._leftTrigger || t - _screenStartStrip > camera._rightTrigger)
		setCameraAt(a->_pos.x, 0);

	for (i = 1; i < _numActors; i++) {
		if (_actors[i].isInCurrentRoom())
			_actors[i].needRedraw = true;
	}
	runInventoryScript(0);
}


void ScummEngine::clampCameraPos(Common::Point *pt) {
	if (pt->x < VAR(VAR_CAMERA_MIN_X))
		pt->x = (short) VAR(VAR_CAMERA_MIN_X);

	if (pt->x > VAR(VAR_CAMERA_MAX_X))
		pt->x = (short) VAR(VAR_CAMERA_MAX_X);

	if (pt->y < VAR(VAR_CAMERA_MIN_Y))
		pt->y = (short) VAR(VAR_CAMERA_MIN_Y);

	if (pt->y > VAR(VAR_CAMERA_MAX_Y))
		pt->y = (short) VAR(VAR_CAMERA_MAX_Y);
}

void ScummEngine::moveCamera() {
	int pos = camera._cur.x;
	int actorx, t;
	Actor *a = NULL;

	camera._cur.x &= 0xFFF8;

	if (camera._cur.x < VAR(VAR_CAMERA_MIN_X)) {
		if (VAR_CAMERA_FAST_X != 0xFF && VAR(VAR_CAMERA_FAST_X))
			camera._cur.x = (short) VAR(VAR_CAMERA_MIN_X);
		else
			camera._cur.x += SCREEN_STRIP_SIZE;
		cameraMoved();
		return;
	}

	if (camera._cur.x > VAR(VAR_CAMERA_MAX_X)) {
		if (VAR_CAMERA_FAST_X != 0xFF && VAR(VAR_CAMERA_FAST_X))
			camera._cur.x = (short) VAR(VAR_CAMERA_MAX_X);
		else
			camera._cur.x -= SCREEN_STRIP_SIZE;
		cameraMoved();
		return;
	}

	if (camera._mode == kFollowActorCameraMode) {
		a = derefActor(camera._follows, "moveCamera");

		actorx = a->_pos.x;
		t = SCREEN_TO_STRIP(actorx) - _screenStartStrip;

		if (t < camera._leftTrigger || t > camera._rightTrigger) {
			if (VAR_CAMERA_FAST_X != 0xFF && VAR(VAR_CAMERA_FAST_X)) {
#if (SCREEN_STRIP_SIZE == 8)
				if (t > 35)	camera._dest.x = actorx + 80;
				if (t < 5)	camera._dest.x = actorx - 80;
#else
				if (t > 17)	camera._dest.x = actorx + 80;
				if (t < 3)	camera._dest.x = actorx - 80;
#endif
			} else
				camera._movingToActor = true;
		}
	}

	if (camera._movingToActor) {
		a = derefActor(camera._follows, "moveCamera(2)");
		camera._dest.x = a->_pos.x;
	}

	if (camera._dest.x < VAR(VAR_CAMERA_MIN_X))
		camera._dest.x = (short) VAR(VAR_CAMERA_MIN_X);

	if (camera._dest.x > VAR(VAR_CAMERA_MAX_X))
		camera._dest.x = (short) VAR(VAR_CAMERA_MAX_X);

	if (VAR_CAMERA_FAST_X != 0xFF && VAR(VAR_CAMERA_FAST_X)) {
		camera._cur.x = camera._dest.x;
	} else {
		if (camera._cur.x < camera._dest.x)
			camera._cur.x += SCREEN_STRIP_SIZE;
		if (camera._cur.x > camera._dest.x)
			camera._cur.x -= SCREEN_STRIP_SIZE;
	}

	/* a is set a bit above */
	if (camera._movingToActor && SCREEN_TO_STRIP(camera._cur.x) == SCREEN_TO_STRIP(a->_pos.x)) {
		camera._movingToActor = false;
	}

	cameraMoved();

	if (VAR_SCROLL_SCRIPT != 0xFF && VAR(VAR_SCROLL_SCRIPT) && pos != camera._cur.x) {
		VAR(VAR_CAMERA_POS_X) = camera._cur.x;
		runScript(VAR(VAR_SCROLL_SCRIPT), 0, 0, 0);
	}
}


void ScummEngine::cameraMoved() {
	if (camera._cur.x < (SCREEN_WIDTH / 2)) {
		camera._cur.x = (SCREEN_WIDTH / 2);
	} else if (camera._cur.x > _roomWidth - (SCREEN_WIDTH / 2)) {
		camera._cur.x = _roomWidth - (SCREEN_WIDTH / 2);
	}

	_screenStartStrip = SCREEN_TO_STRIP(camera._cur.x) - (SCREEN_STRIP_COUNT / 2);
	_screenEndStrip = _screenStartStrip + SCREEN_STRIP_COUNT - 1;

	_screenTop = camera._cur.y - (SCREEN_HEIGHT / 2);
	_screenLeft = STRIP_TO_SCREEN(_screenStartStrip);

	virtscr[0].xstart = STRIP_TO_SCREEN(_screenStartStrip);

	if (_charset->_hasMask && _version > 3) {
		int dx = camera._cur.x - camera._last.x;
		int dy = camera._cur.y - camera._last.y;

		if (dx || dy) {
			_charset->_mask.left -= dx;
			_charset->_mask.right -= dx;
			_charset->_mask.top -= dy;
			_charset->_mask.bottom -= dy;
		}
	}
}

void ScummEngine::panCameraTo(int x, int y) {
	camera._dest.x = x;
	camera._mode = kPanningCameraMode;
	camera._movingToActor = false;
}


void ScummEngine::actorFollowCamera(int act) {
	int old;

	old = camera._follows;
	setCameraFollows(derefActor(act, "actorFollowCamera"));
	if (camera._follows != old)
		runInventoryScript(0);

	camera._movingToActor = false;
}

} // End of namespace Scumm
