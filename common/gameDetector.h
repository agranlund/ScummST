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
 * $Header: /cvsroot/scummvm/scummvm/base/gameDetector.h,v 1.26 2004/01/18 20:46:11 fingolfin Exp $
 *
 */

#ifndef GAMEDETECTOR_H
#define GAMEDETECTOR_H

#include "common/str.h"
#include "common/scaler.h"

class Engine;
class GameDetector;
class MidiDriver;
class OSystem;
class SoundMixer;

/** Global (shared) game feature flags. */
enum {
//	GF_HAS_SPEECH = 1 << 28,
//	GF_HAS_SUBTITLES = 1 << 29,
	GF_DEFAULT_TO_1X_SCALER = 1 << 30
};

enum MidiDriverType {
	MDT_NONE   = 0,
	MDT_PCSPK  = 1, // MD_PCSPK and MD_PCJR
	MDT_ADLIB  = 2, // MD_ADLIB
	MDT_TOWNS  = 4, // MD_TOWNS
	MDT_NATIVE = 8, // Everything else
	MDT_PREFER_NATIVE = 16
};

struct GameSettings {
	const char *name;
	const char *description;
	uint32 features;
};


class GameDetector {
	typedef Common::String String;

public:
	GameDetector();

	void parseCommandLine(int argc, char **argv);
	bool detectMain();

	String _targetName;
	GameSettings _game;	// TODO: Eventually get rid of this?!

public:
	void setTarget(const String &name);

	Engine *createEngine(OSystem *system);

	static SoundMixer *createMixer();
	static MidiDriver *createMidi(int midiDriver);

	static int detectMusicDriver(int midiFlags);

	static GameSettings findGame(const String &gameName);

protected:
	bool detectGame(void);
};

#endif
