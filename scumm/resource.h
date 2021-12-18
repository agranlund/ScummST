/* ScummVM - Scumm Interpreter
 * Copyright (C) 2002-2004 The ScummVM project
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
 * $Header: /cvsroot/scummvm/scummvm/scumm/resource.h,v 1.9 2004/01/19 20:27:31 fingolfin Exp $
 */

#ifndef RESOURCE_H
#define RESOURCE_H

// versions
// 1: first release
// 2: new sprite format
// 3: new palette for Indy4
// 4: dithering + reverted the unique Indy4 palette
// 5: costume optimization
// 6: costume optimization
// 7: bomp and cursor stuff
// 8: with resource size fix
#define ATARI_RESOURCE_VERSION 8


// We have been saving images in twice the size needed. Fixing this breaks compatibility with old savegames
// because ScummVM, in its infinite wisdom, is saving the actual image resouces for inventory items into the savegame files...
//#define ATARI_OBIM_RESOURCE_SIZE_FIX_SAMNMAX_ONLY
//#define ATARI_OBIM_RESOURCE_SIZE_FIX_PARTIAL
#define ATARI_OBIM_RESOURCE_SIZE_FIX

// Insert dummy padding block between OBCD and OBIM so that the OBIM is word aligned.
// Not sure if this was causing any problems in the older games but it does fix alignment crashes in samnmax.
// flojects which are loaded from an old savegame (ie; inventory items) are not automatically fixed though.
// not an issue for samnmax since there are no saves out there yet, but if this is/was causing issues in
// older games we may want to either break save compatibility or auto-convert when loading from savegame.
//#ifdef GAME_SAMNMAX
#define ATARI_FLOBJECT_OBIM_FIX
//#endif

namespace Scumm {

#if !defined(__GNUC__)
	#pragma START_PACK_STRUCTS
#endif	

struct ResHdr {
	uint32 tag, size;
} GCC_PACK;

struct ArrayHeader {
	int16 dim1;
	int16 type;
	int16 dim2;
	byte data[1];
} GCC_PACK;

#if !defined(__GNUC__)
	#pragma END_PACK_STRUCTS
#endif

#define RES_DATA(x) (((const byte*)x) + sizeof(ResHdr))
#define RES_SIZE(x) (READ_BE_UINT32(&((const ResHdr* )x)->size))

enum {
	OF_OWNER_MASK = 0x0F,
	OF_STATE_MASK = 0xF0,
	
	OF_STATE_SHL = 4
};

enum {
	RF_LOCK = 0x80,
	RF_USAGE = 0x7F,
	RF_USAGE_MAX = RF_USAGE
};


const byte *findResource(uint32 tag, const byte *searchin);

class ResourceIterator {
	uint32 _size;
	uint32 _pos;
	const byte *_ptr;
public:
	ResourceIterator(const byte *searchin);
	const byte *findNext(uint32 tag);
};


} // End of namespace Scumm

#endif
