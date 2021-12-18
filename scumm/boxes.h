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
 * $Header: /cvsroot/scummvm/scummvm/scumm/boxes.h,v 1.10 2004/01/06 12:45:30 fingolfin Exp $
 *
 */

#ifndef BOXES_H
#define BOXES_H

#include "common/rect.h"

namespace Scumm {

#define SIZEOF_BOX_V2 8
#define SIZEOF_BOX_V3 18
#define SIZEOF_BOX 20
#define SIZEOF_BOX_V8 52

typedef enum {
	kBoxXFlip		= 0x08,
	kBoxYFlip		= 0x10,
	kBoxIgnoreScale	= 0x20,
	kBoxPlayerOnly	= 0x20,
	kBoxLocked		= 0x40,
	kBoxInvisible	= 0x80
} BoxFlags;

struct BoxCoords {			/* Box coordinates */
	Common::Point ul;
	Common::Point ur;
	Common::Point ll;
	Common::Point lr;
};

} // End of namespace Scumm

#endif
