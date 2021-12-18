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
 * $Header: /cvsroot/scummvm/scummvm/sound/mididrv.cpp,v 1.50 2004/02/05 13:56:39 chrilith Exp $
 *
 */

#include "stdafx.h"
#include "sound/mididrv.h"
#include "common/str.h"


/** Internal list of all available 'midi' drivers. */
static const struct MidiDriverDescription midiDrivers[] = {
	{"auto", 	"Default", MD_AUTO},
	{"null", 	"No music", MD_NULL},
	{"ym2149", 	"Atari", MD_STCHIP},
	{"gm", 		"General Midi", MD_STMIDI_GM},
	{"mt32", 	"Roland MT32", MD_STMIDI_MT32},
#ifndef DISABLE_ADLIB
	{"adlib", "Adlib", MD_ADLIB},
#endif
	{0, 0, 0}
};


const MidiDriverDescription *getAvailableMidiDrivers() {
	return midiDrivers;
}

int parseMusicDriver(const Common::String &str) {
	if (str.isEmpty())
		return -1;

	const char *s = str.c_str();
	const MidiDriverDescription *md = getAvailableMidiDrivers();

	while (md->name) {
		if (!scumm_stricmp(md->name, s)) {
			return md->id;
		}
		md++;
	}

	return -1;
}
