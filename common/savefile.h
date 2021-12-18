/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001/2002 The ScummVM project
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
 * $Header: /cvsroot/scummvm/scummvm/common/savefile.h,v 1.9 2003/11/30 00:41:19 fingolfin Exp $
 *
 */

#ifndef COMMON_SAVEFILE_H
#define COMMON_SAVEFILE_H

#include "stdafx.h"
#include "common/scummsys.h"

#include <stdio.h>
#include <string.h>


class SaveFile {
public:
	SaveFile(const char* filename, bool saveOrLoad);
	~SaveFile();

	uint32 read(void *ptr, uint32 size);
	byte readByte();
	uint16 readUint16();
	uint32 readUint32();
	uint32 write(const void *ptr, uint32 size);
	void writeByte(byte value);
	void writeUint16(uint16 value);
	void writeUint32(uint32 value);

	bool isOpen() const;

private:
	void flushWrites(bool forceFlush);
	void cacheReads();

	bool mode;
	uintptr_t buf;
	uintptr_t bufPtr;
	FILE* _f;
};

class SaveFileManager {

public:
	~SaveFileManager() {}

	SaveFile *open_savefile(const char *filename, const char *directory, bool saveOrLoad);
	void list_savefiles(const char * /* prefix */,  const char *directory, bool *marks, int num) {
		memset(marks, true, num * sizeof(bool));
	}

protected:
	void join_paths(const char *filename, const char *directory, char *buf, int bufsize);
	SaveFile *makeSaveFile(const char *filename, bool saveOrLoad);
};

#endif
