/* ScummVM - Scumm Interpreter
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
 * $Header: /cvsroot/scummvm/scummvm/backends/intern.h,v 1.7 2004/01/26 07:57:25 arisme Exp $
 *
 */

#ifndef BACKENDS_INTERN_H
#define BACKENDS_INTERN_H

#include "common/system.h"

/* Factory functions. This means we don't have to include the headers for
 * all backends.
 */
#if defined(__ATARI__)
extern OSystem *OSystem_Atari_create();
#elif defined(GAME_RESOURCECONVERTER)
extern OSystem *OSystem_Converter_create();
#else
extern OSystem *OSystem_SDL_create(int gfx_driver);
#endif

#ifndef __ATARI__
#define SAMPLES_PER_SEC 11025
#endif

#endif
