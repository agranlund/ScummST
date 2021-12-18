/* ScummVM - Scumm Interpreter
 * Copyright (C) 2002-2004 The ScummVM project
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
 * $Header: /cvsroot/scummvm/scummvm/common/file.h,v 1.21 2004/01/06 12:45:27 fingolfin Exp $
 *
 */

#ifndef COMMON_FILE_H
#define COMMON_FILE_H

#include "stdafx.h"
#include "common/scummsys.h"
#include "common/str.h"

class File {
private:

	FILE * _handle;
	bool _ioFailed;
	byte _encbyte;
	char *_name;	// For debugging
	uintptr_t _tempBuf;
	uintptr_t _tempBufPtr;
	uintptr_t _tempBufEnd;
	bool _tempBufWriting;
	uint32 _fileCacheSize;

	static FILE *fopenNoCase(const char *filename, const char *directory, const char *mode);
	
	static Common::String _defaultDirectory;

public:
	enum AccessMode {
		kFileReadMode = 1,
		kFileWriteMode = 2
	};
	
	static void setDefaultDirectory(const Common::String &directory);
	
	File();
	virtual ~File();
	void enableFileCache(uint32 size);
	bool open(const char *filename, const Common::String &directory) { return open(filename, directory.c_str()); }
	bool open(const char *filename, const char *directory = NULL, AccessMode mode = kFileReadMode, byte encbyte = 0);
	void close();
	bool isOpen() const;
	bool ioFailed() const;
	void clearIOFailed();
	bool eof();
	uint32 pos();
	uint32 size();
	const char *name() const { return _name; }
	void seek(int32 offs, int whence = SEEK_SET);
	uint32 read(void *ptr, uint32 size);
	byte readByte();
	uint16 readUint16LE();
	uint32 readUint32LE();
	uint16 readUint16BE();
	uint32 readUint32BE();
	uint32 write(const void *ptr, uint32 size);
	void writeByte(byte value);
	void writeUint16LE(uint16 value);
	void writeUint32LE(uint32 value);
	void writeUint16BE(uint16 value);
	void writeUint32BE(uint32 value);
	void setEnc(byte value) { _encbyte = value; }

#ifdef SCUMM_LITTLE_ENDIAN
	void writeUint32(uint32 value)	{ writeUint32LE(value);	 }
	uint32 readUint32()				{ return readUint32LE(); }
#else
	void writeUint32(uint32 value)	{ writeUint32BE(value);	 }
	uint32 readUint32()				{ return readUint32BE(); }
#endif

	void createTempBuf(bool writemode);
	void writeTempBuf(bool force);
	void readTempBuf();
};

#endif
