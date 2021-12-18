/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001-2004 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /cvsroot/scummvm/scummvm/backends/midi/null.cpp,v 1.8 2004/01/06 12:45:27 fingolfin Exp $
 */

#include "stdafx.h"
#include "common/util.h"
#include "sound/mpu401.h"

/* NULL driver */
class MidiDriver_NULL : public MidiDriver_MPU401 {
public:
	int open() { return 0; }
	void send(uint32 b) { }
};

MidiDriver *MidiDriver_NULL_create() {
	debug(9, " - MidiDriver_NULL_create - ");
	return new MidiDriver_NULL();
}
