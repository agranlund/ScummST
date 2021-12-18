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
 * $Header: /cvsroot/scummvm/scummvm/common/system.cpp,v 1.4 2004/01/06 12:45:28 fingolfin Exp $
 *
 */

#include "stdafx.h"
#include "backends/intern.h"
#include "common/gameDetector.h"
#include "common/system.h"

OSystem *g_system = 0;

static OSystem *createSystem() {
#if defined(__ATARI__)
	return OSystem_Atari_create();
#elif defined(GAME_RESOURCECONVERTER)
	return OSystem_Converter_create();
#else
	return OSystem_SDL_create(1);
#endif
}

OSystem *OSystem::instance() {
	if (!g_system)
		g_system = createSystem();
	return g_system;
}
