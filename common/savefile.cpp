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
 * $Header: /cvsroot/scummvm/scummvm/common/savefile.cpp,v 1.9 2004/01/06 12:45:28 fingolfin Exp $
 *
 */

#include "stdafx.h"
#include "common/util.h"
#include "common/savefile.h"

#define SAVEFILE_CACHESIZE	(16 * 1024)

static uintptr_t _tempBuf = 0;
static bool _fileIsOpen = false;

SaveFile::SaveFile(const char* filename, bool saveOrLoad) {
	assert(!_fileIsOpen);
	_f = fopen(filename, (saveOrLoad ? "wb" : "rb"));
	if (_f) {
		if (!_tempBuf) {
#ifdef __ATARI__
			// prefer ST ram because file operations would have to go through FRB otherwise
			_tempBuf = (uintptr_t) atari_alloc(SAVEFILE_CACHESIZE, ALLOC_STRAM_PREFER);
#else
			_tempBuf = (uintptr_t) malloc(SAVEFILE_CACHESIZE);
#endif
		}
		mode = saveOrLoad;
		buf = _tempBuf;
		bufPtr = buf;
		_fileIsOpen = true;
	}
}

SaveFile::~SaveFile() {
	if(_f) {
		if (mode) {
			flushWrites(true);
		}
		fclose(_f);
		_fileIsOpen = false;
	}
}

bool SaveFile::isOpen() const
{
	return (_f != 0);
}

uint32 SaveFile::read(void *ptr, uint32 size) {
	if (size == 0)
		return 0;

	cacheReads();
	uint32 remaining = SAVEFILE_CACHESIZE - (bufPtr - buf);
	if (remaining >= size)
	{
		memcpy(ptr, (void*)bufPtr, size);
		bufPtr += size;
		return size;
	}

	byte* dst = (byte*)ptr;
	if (remaining > 0)
	{
		memcpy(dst, (void*)bufPtr, remaining);
		bufPtr += remaining;
		size -= remaining;
		dst += remaining;
	}
	return fread(dst, 1, size, _f) + remaining;
}

byte SaveFile::readByte() {
	cacheReads();
	byte val = *((byte*)bufPtr++);
	return val;
}

uint16 SaveFile::readUint16() {
	uint16 a = readByte();
	uint16 b = readByte();
	return a | (b << 8);
}

uint32 SaveFile::readUint32() {
	uint32 a = readByte();
	uint32 b = readByte();
	uint32 c = readByte();
	uint32 d = readByte();
	return a | (b << 8) | (c << 16) | (d << 24);
}

uint32 SaveFile::write(const void *ptr, uint32 size) {
	flushWrites(true);
	return fwrite(ptr, 1, size, _f);
}

void SaveFile::writeByte(byte value) {
	*((byte*)bufPtr++) = value;
	flushWrites(false);
}

void SaveFile::writeUint16(uint16 value) {
	writeByte(value & 0xFF); value >>= 8;
	writeByte(value & 0xFF);
}

void SaveFile::writeUint32(uint32 value) {
	writeByte(value & 0xFF); value >>= 8;
	writeByte(value & 0xFF); value >>= 8;
	writeByte(value & 0xFF); value >>= 8;
	writeByte(value & 0xFF);
}

void SaveFile::flushWrites(bool forceFlush)
{
	uint32 size = bufPtr - buf;
	if ((size >= SAVEFILE_CACHESIZE) || (forceFlush && (size > 0)))
	{
		fwrite((void*)buf, 1, size, _f);
		bufPtr = buf;
	}
}

void SaveFile::cacheReads()
{
	uint usedSize = bufPtr - buf;
	if ((usedSize == 0) || (usedSize >= SAVEFILE_CACHESIZE))
	{
		fread((void*)buf, 1, SAVEFILE_CACHESIZE, _f);
		bufPtr = buf;
	}
}


SaveFile *SaveFileManager::open_savefile(const char *filename, const char *directory, bool saveOrLoad) {
	char buf[256];
	join_paths(filename, directory, buf, sizeof(buf));
	SaveFile *sf = makeSaveFile(buf, saveOrLoad);
	if (!sf->isOpen()) {
		delete sf;
		sf = 0;
	}
	return sf;
}

void SaveFileManager::join_paths(const char *filename, const char *directory,
								 char *buf, int bufsize) {
	buf[bufsize-1] = '\0';
	strncpy(buf, directory, bufsize-1);

#ifdef WIN32
	// Fix for Win98 issue related with game directory pointing to root drive ex. "c:\"
	if ((buf[0] != 0) && (buf[1] == ':') && (buf[2] == '\\') && (buf[3] == 0)) {
		buf[2] = 0;
	}
#endif

	const int dirLen = strlen(buf);

	if (dirLen > 0) {
#if !defined(__ATARI__)
		strncat(buf, "/", bufsize-1);	// prevent double /
#endif
	}
	strncat(buf, filename, bufsize-1);
}

SaveFile *SaveFileManager::makeSaveFile(const char *filename, bool saveOrLoad) {
	return new SaveFile(filename, saveOrLoad);
}
