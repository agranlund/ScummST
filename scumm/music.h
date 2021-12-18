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
 * /scummvm/scummvm/scumm/player_v3a.h
 *
 */

#ifndef SCUMM_MUSIC_H
#define SCUMM_MUSIC_H

#include "common/scummsys.h"
#include "common/system.h"

namespace Scumm {

/**
 * Pure virtual base class for the various music/sound engines used in Scumm
 * games. In particular, the iMuse code provides a subclass of this. There are
 * several other subclasses providing music and sound capabilities for
 * several Scumm games.
 * Having this base class for all music engines allows uniform access to the
 * core music/sound functionality, thus simplifying the client code.
 *
 * Instantiated by class Scumm.
 */
class MusicEngine {
public:
	virtual ~MusicEngine() {}

	/**
	 * Set the output volume.
	 * @param vol		the new output volume
	 */
	virtual void setMasterVolume(int vol) = 0;

	/**
	 * Start playing the sound with the given id.
	 * @param sound		the sound to start
	 */
	virtual void startSound(int sound) = 0;

	/**
	 * Stop playing the sound with the given id.
	 * @param sound		the sound to stop
	 */
	virtual void stopSound(int sound) = 0;

	/**
	 * Start playing all currently playing sounds.
	 */
	virtual void stopAllSounds() = 0;

	/**
	 * Query the status of the sound with the given id. Usually this is just
	 * a boolean telling us whether the sound is playing or not.
	 * @param sound		the sound to for which we want the status
	 * @return the status of the specified sound
	 */
	virtual int  getSoundStatus(int sound) const = 0;

	/**
	 * Get the value of the music timer. Used for synchronising scripts with
	 * the music/sound.
	 * @return the music timer
	 */
	virtual int  getMusicTimer() const { return 0; }

	/**
	 * Terminate the music engine. Called just before the music engine
	 * is deleted.
	 */
	virtual void terminate() {}
};

} // End of namespace Scumm

#endif
