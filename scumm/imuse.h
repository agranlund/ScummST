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
 * $Header: /cvsroot/scummvm/scummvm/scumm/imuse.h,v 1.53 2004/01/06 12:45:30 fingolfin Exp $
 *
 */

#ifndef IMUSE_H
#define IMUSE_H

#include "common/scummsys.h"
#include "common/system.h"
#include "scumm/music.h"

namespace GUI {
	class OptionsDialog;
};

class MidiDriver;
class OSystem;
class SoundMixer;

namespace Scumm {

class IMuseInternal;
class ScummEngine;
class Serializer;

class IMuse : public MusicEngine {
	friend class GUI::OptionsDialog;
private:
	OSystem *_system;
	IMuseInternal *_target;
	mutable OSystem::MutexRef _mutex;

	IMuse(OSystem *system, IMuseInternal *target);
	void in() const;
	void out() const;

public:
	~IMuse();

	enum {
		PROP_TEMPO_BASE,
		PROP_NATIVE_MT32,
		PROP_MULTI_MIDI,
		PROP_OLD_ADLIB_INSTRUMENTS,
		PROP_LIMIT_PLAYERS,
		PROP_RECYCLE_PLAYERS,
		PROP_DIRECT_PASSTHROUGH
	};

	void on_timer(MidiDriver *midi);
	void pause(bool paused);
	int save_or_load(Serializer *ser, ScummEngine *scumm);
	int set_music_volume(uint vol);
	void setMasterVolume(int vol);
	void startSound(int sound);
	void stopSound(int sound);
	void stopAllSounds();
	int getSoundStatus(int sound) const;
	bool get_sound_active(int sound) const;
	int getMusicTimer() const;
	int32 doCommand (int a, int b, int c, int d, int e, int f, int g, int h);
	int32 doCommand (int numargs, int args[]);
	int clear_queue();
	void setBase(byte **base);
	uint32 property(int prop, uint32 value);
	void terminate();
	void changeDriver(MidiDriver *midi, bool native_mt32);

	// Factory methods
	static IMuse *create(OSystem *syst, SoundMixer *mixer, MidiDriver *midi);
};

} // End of namespace Scumm

#endif
