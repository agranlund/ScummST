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
 * $Header: /cvsroot/scummvm/scummvm/common/file.cpp,v 1.56 2004/02/10 10:56:16 kirben Exp $
 *
 */

#include "common/file.h"
#include "common/util.h"

Common::String File::_defaultDirectory;


FILE *File::fopenNoCase(const char *filename, const char *directory, const char *mode) {
	FILE *file;
	char buf[512];
	char *ptr;

	assert(directory);
	strcpy(buf, directory);

#ifdef WIN32
	// Fix for Win98 issue related with game directory pointing to root drive ex. "c:\"
	if ((buf[0] != 0) && (buf[1] == ':') && (buf[2] == '\\') && (buf[3] == 0)) {
		buf[2] = 0;
	}
#endif

	// Record the length of the dir name (so we can cut of anything trailing it
	// later, when we try with different file names).
	const int dirLen = strlen(buf);

	if (dirLen > 0) {
#ifdef __MORPHOS__
		if (buf[dirLen-1] != ':' && buf[dirLen-1] != '/')
#endif

#if !defined(__GP32__) && !defined(__PALM_OS__)
		strcat(buf, "/");	// prevent double /
#endif
	}
	strcat(buf, filename);

	file = fopen(buf, mode);
	if (file)
		return file;

	// FIXME this should probably be engine specific...
	const char *dirs[] = {
		"",
		"rooms/",
		"ROOMS/",
		"Rooms 1/",
		"Rooms 2/",
		"Rooms 3/",
		"video/",
		"VIDEO/",
		"data/",
		"DATA/",
		"resource/",
		"RESOURCE/",
		// Simon the Sorcerer 1 Acorn
		"execute/",
		"EXECUTE/",
		// Simon the Sorcerer 2 Amiga/Mac
		"../",
		"voices/",
		"VOICES/",
		// sword1/2 stuff if user just copied files without putting
		// them all into the same dir like original installer did
		"CLUSTERS/",
		"clusters/",
		"SPEECH/",
		"speech/",
		"SWORD2/",
		"sword2/"
	};

	for (int dirIdx = 0; dirIdx < ARRAYSIZE(dirs); dirIdx++) {
		buf[dirLen] = 0;
		if (buf[0] != 0) {
#ifdef __MORPHOS__
			if (buf[strlen(buf) - 1] != ':' && buf[strlen(buf) - 1] != '/')
#endif
#ifndef __PALM_OS__
			strcat(buf, "/");	// PALMOS
#endif
		}
		strcat(buf, dirs[dirIdx]);
		int8 len = strlen(buf);
		strcat(buf, filename);

		ptr = buf + len;
		do
			*ptr = toupper(*ptr);
		while (*ptr++);
		file = fopen(buf, mode);
		if (file)
			return file;

		ptr = buf + len;
		do
			*ptr = tolower(*ptr);
		while (*ptr++);
		file = fopen(buf, mode);
		if (file)
			return file;
	}

	return NULL;
}

void File::setDefaultDirectory(const Common::String &directory) {
	_defaultDirectory = directory;
}

File::File() {
	_handle = NULL;
	_ioFailed = false;
	_encbyte = 0;
	_name = 0;
	_fileCacheSize = 0;
	_tempBuf = 0;
	_tempBufPtr = 0;
	_tempBufEnd = 0;
	_tempBufWriting = false;
}

File::~File() {
	close();
	delete [] _name;
}

void File::enableFileCache(uint32 size)
{
	if (!isOpen() && (size > 0))
	{
		_fileCacheSize = size;
	}
}

bool File::open(const char *filename, const char *directory, AccessMode mode, byte encbyte) {
	if (_handle) {
		debug(2, "File %s already opened", filename);
		return false;
	}

	if (filename == NULL || *filename == 0)
		return false;
	
	// If no directory was specified, use the default directory (if any).
	if (directory == NULL)
		directory = _defaultDirectory.isEmpty() ? "" : _defaultDirectory.c_str();

	clearIOFailed();

	if (mode == kFileReadMode) {
		_handle = fopenNoCase(filename, directory, "rb");
		if (_handle == NULL) {
			debug(2, "File %s not found", filename);
			return false;
		}
	}
	else if (mode == kFileWriteMode) {
		_handle = fopenNoCase(filename, directory, "wb");
		if (_handle == NULL) {
			debug(2, "File %s not opened", filename);
			return false;
		}
	}	else {
		warning("Only read/write mode supported!");
		return false;
	}

	_encbyte = encbyte;

	int len = strlen(filename);
	if (_name != 0)
		delete [] _name;
	_name = new char[len+1];
	memcpy(_name, filename, len+1);
	if (_fileCacheSize > 0)
		createTempBuf(mode == kFileWriteMode ? true : false);
	return true;
}

void File::close() {
	if (_handle)
	{
		if (_fileCacheSize > 0)
		{
			if (_tempBufWriting && (_tempBufPtr > _tempBuf))
				fwrite((void*)_tempBuf, (_tempBufPtr - _tempBuf), 1, _handle);
			free((void*)_tempBuf);
		}
		fclose(_handle);
	}
	_handle = NULL;
}

bool File::isOpen() const {
	return _handle != NULL;
}

bool File::ioFailed() const {
	return _ioFailed != 0;
}

void File::clearIOFailed() {
	_ioFailed = false;
}

bool File::eof() {
	if (_handle == NULL) {
		error("File is not open!");
		return false;
	}
	if (_fileCacheSize > 0)
	{
		if (_tempBufWriting)
			return feof(_handle) != 0;
		else
			return (feof(_handle) != 0) && (_tempBufPtr >= _tempBufEnd);
	}
	else
	{
		return feof(_handle) != 0;
	}
}

uint32 File::pos() {
	if (_handle == NULL) {
		error("File is not open!");
		return 0;
	}

	if (_fileCacheSize > 0)
	{
		if (_tempBufWriting)
			return ftell(_handle) + (_tempBufPtr - _tempBuf);
		else
			return ftell(_handle) - (_tempBufEnd - _tempBufPtr);
	}
	else
	{
		return ftell(_handle);
	}
}

uint32 File::size() {
	if (_handle == NULL) {
		error("File is not open!");
		return 0;
	}

	uint32 oldPos = ftell(_handle);
	fseek(_handle, 0, SEEK_END);
	uint32 length = ftell(_handle);
	fseek(_handle, oldPos, SEEK_SET);

	return length;
}

void File::seek(int32 offs, int whence) {
	if (_handle == NULL) {
		error("File is not open!");
		return;
	}
	if (_fileCacheSize > 0)
	{
		if (_tempBufWriting)
		{
			writeTempBuf(true);
		}
		else
		{
			if (whence == SEEK_CUR)
				offs -= (_tempBufEnd - _tempBufPtr);

			_tempBufPtr = _tempBuf;
			_tempBufEnd = _tempBuf;
		}
	}

	if (fseek(_handle, offs, whence) != 0)
		clearerr(_handle);
}

uint32 File::read(void *ptr, uint32 len) {
	byte *ptr2 = (byte *)ptr;

	if (_handle == NULL) {
		error("File is not open!");
		return 0;
	}

	if (len == 0)
		return 0;

	uint32 real_len = 0;
	if (_fileCacheSize > 0)
	{
		readTempBuf();
		uint32 remaining = _tempBufEnd - _tempBufPtr;
		if (remaining >= len)
		{
			memcpy(ptr2, (void*)_tempBufPtr, len);
			_tempBufPtr += len;
			ptr2 += len;
			real_len = len;
			len = 0;
		}
		else
		{
			if (remaining > 0)
			{
				memcpy(ptr2, (void*)_tempBufPtr, remaining);
				_tempBufPtr += remaining;
				ptr2 += remaining;
				real_len += remaining;
				len -= remaining;
			}
		}
	}

	if (len > 0)
	{
		// P.Putnik drivers appears to silently fail on certain partitions when reading more than 128Kb in a single go.
		// May or may not be related to files larger than 16MB.
		// It triggers on Indy4 when reading >128KB that crosses the 16MB mark of the file.
		// Let's read in 64KB chunks since that is the usual size of the _FRB anyway.
		const uint32 maxlen = 64*1024;

		while(len > maxlen)
		{
			uint32 retlen = fread(ptr2, 1, maxlen, _handle);
			if (retlen < maxlen)
			{
				clearerr(_handle);
				_ioFailed = true;
				break;
			}
			len -= maxlen;
			ptr2 += maxlen;
			real_len += maxlen;
		}

		real_len += fread(ptr2, 1, len, _handle);
		if (real_len < len)
		{
			clearerr(_handle);
			_ioFailed = true;
		}	
	}

	if (_encbyte != 0) {
		ptr2 = (byte *)ptr;
		uint32 t_size = real_len;
		do {
			*ptr2++ ^= _encbyte;
		} while (--t_size);
	}

	return real_len;
}

byte File::readByte() {
	byte b = 0;

	if (_handle == NULL) {
		error("File is not open!");
		return 0;
	}
	if (_fileCacheSize > 0)
	{
		readTempBuf();
		if (_tempBufPtr >= _tempBufEnd)
		{
			clearerr(_handle);
			_ioFailed = true;
		}
		else
		{
			b = *((byte*)_tempBufPtr++);
		}
	}
	else
	{
		if (fread(&b, 1, 1, _handle) != 1) {
			clearerr(_handle);
			_ioFailed = true;
		}	
	}
	return b ^ _encbyte;
}

uint16 File::readUint16LE() {
	uint16 a = readByte();
	uint16 b = readByte();
	return a | (b << 8);
}

uint32 File::readUint32LE() {
	uint32 a = readUint16LE();
	uint32 b = readUint16LE();
	return (b << 16) | a;
}

uint16 File::readUint16BE() {
	uint16 b = readByte();
	uint16 a = readByte();
	return a | (b << 8);
}

uint32 File::readUint32BE() {
	uint32 b = readUint16BE();
	uint32 a = readUint16BE();
	return (b << 16) | a;
}

uint32 File::write(const void *ptr, uint32 len) {
	byte *tmp = 0;
	
	if (_handle == NULL) {
		error("File is not open!");
		return 0;
	}

	if (len == 0)
		return 0;

	if (_encbyte != 0) {
		// Maybe FIXME: while it's efficient to do the encoding here,
		// it not really nice for a write function to modify its input.
		// Maybe we should work on a copy here...
		tmp = (byte *)malloc(len);
		for (uint32 i = 0; i < len; i ++) {
			tmp[i] = ((const byte *)ptr)[i] ^ _encbyte;
		}
		ptr = tmp;
	}

	if (_fileCacheSize > 0)
		writeTempBuf(true);

	// P.Putnik drivers appears to silently fail on certain partitions when reading more than 128Kb in a single go.
	// May or may not be related to files larger than 16MB.
	// It triggers on Indy4 when reading >128KB that crosses the 16MB mark of the file.
	// Let's read in 64KB chunks since that is the usual size of the _FRB anyway.
	const uint32 maxlen = 64*1024;
	const byte* ptr2 = (const byte*) ptr;
	uint32 real_len = 0;
	while(len > maxlen)
	{
		uint32 retlen = fwrite(ptr2, 1, maxlen, _handle);
		if (retlen != maxlen)
		{
			clearerr(_handle);
			_ioFailed = true;
			break;
		}
		len -= maxlen;
		ptr2 += maxlen;
		real_len += maxlen;
	}
	if (len > 0)
	{
		real_len += fwrite(ptr2, 1, len, _handle);
		if (real_len != len)
		{
			clearerr(_handle);
			_ioFailed = true;
		}
	}

	if (_encbyte != 0) {
		free(tmp);
	}

	return len;
}

void File::writeByte(byte value) {
	value ^= _encbyte;

	if (_handle == NULL) {
		error("File is not open!");
	}
	if (_fileCacheSize > 0)
	{
		*((byte*)_tempBufPtr++) = value;
		writeTempBuf(false);
	}
	else
	{
		if (fwrite(&value, 1, 1, _handle) != 1) {
			clearerr(_handle);
			_ioFailed = true;
		}
	}	
}

void File::writeUint16LE(uint16 value) {
	writeByte((byte)(value & 0xff));
	writeByte((byte)(value >> 8));
}

void File::writeUint32LE(uint32 value) {
	writeUint16LE((uint16)(value & 0xffff));
	writeUint16LE((uint16)(value >> 16));
}

void File::writeUint16BE(uint16 value) {
	writeByte((byte)(value >> 8));
	writeByte((byte)(value & 0xff));
}

void File::writeUint32BE(uint32 value) {
	writeUint16BE((uint16)(value >> 16));
	writeUint16BE((uint16)(value & 0xffff));
}


void File::createTempBuf(bool writemode)
{
	if (_fileCacheSize > 0)
	{
	#ifdef __ATARI__
		// prefer ST ram because file operations would have to go through FRB otherwise
		_tempBuf = (uintptr_t) atari_alloc(_fileCacheSize, ALLOC_STRAM_PREFER);
	#else
		_tempBuf = (uintptr_t) malloc(_fileCacheSize);
	#endif
		_tempBufPtr = _tempBuf;
		_tempBufEnd = _tempBuf;
		_tempBufWriting = writemode;	
	}
}

void File::writeTempBuf(bool force)
{
	if (_fileCacheSize > 0)
	{
		uint32 size = _tempBufPtr - _tempBuf;
		if ((size >= _fileCacheSize) || (force && (size > 0)))
		{
			if (fwrite((void*)_tempBuf, 1, size, _handle) != 1)
			{
				clearerr(_handle);
				_ioFailed = true;
			}
			_tempBufPtr = _tempBuf;
		}
	}
}

void File::readTempBuf()
{
	if (_fileCacheSize > 0)
	{
		uint usedSize = _tempBufPtr - _tempBuf;
		if ((usedSize == 0) || (usedSize >= _fileCacheSize))
		{
			int count = fread((void*)_tempBuf, 1, _fileCacheSize, _handle);
			_tempBufEnd = _tempBuf + count;
			_tempBufPtr = _tempBuf;
		}	
	}
}





