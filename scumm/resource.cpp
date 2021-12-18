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
 * $Header: /cvsroot/scummvm/scummvm/scumm/resource.cpp,v 1.188.2.1 2004/02/29 00:30:03 khalek Exp $
 *
 */

#include "stdafx.h"
#include "common/str.h"
#include "scumm/dialogs.h"
#include "scumm/imuse.h"
#include "scumm/object.h"
#include "scumm/resource.h"
#include "scumm/scumm.h"
#include "scumm/sound.h"
#include "scumm/verbs.h"
#include "sound/mididrv.h" // Need MD_ enum values

//#ifdef __ATARI__
#if 1
#define ATARI_RESOURCE_XOR			0x00
#else
#define ATARI_RESOURCE_XOR			0x69
#endif

namespace Scumm {

static const char *resTypeFromId(int id)
{
	static char buf[100];

	switch (id) {
	case rtRoom:
		return "Room";
	case rtScript:
		return "Script";
	case rtCostume:
		return "Costume";
	case rtSound:
		return "Sound";
	case rtInventory:
		return "Inventory";
	case rtCharset:
		return "Charset";
	case rtString:
		return "String";
	case rtVerb:
		return "Verb";
	case rtActorName:
		return "ActorName";
	case rtBuffer:
		return "Buffer";
	case rtScaleTable:
		return "ScaleTable";
	case rtTemp:
		return "Temp";
	case rtFlObject:
		return "FlObject";
	case rtMatrix:
		return "Matrix";
	case rtBox:
		return "Box";
	case rtLast:
		return "Last";
	case rtNumTypes:
		return "NumTypes";
	default:
		sprintf(buf, "%d", id);
		return buf;
	}
}

/* Open a room */
void ScummEngine::openRoom(int room) {
	int room_offs;
	bool result;
	char buf[128];
	byte encByte = 0;

	debugC(DEBUG_GENERAL, "openRoom(%d)", room);
	assert(room >= 0);

	/* Don't load the same room again */
	if (_lastLoadedRoom == room)
		return;
	_lastLoadedRoom = room;

	/* Room -1 means close file */
	if (room == -1) {
		deleteRoomOffsets();
		_fileHandle.close();
		return;
	}

	/* Either xxx.lfl or monkey.xxx file name */
	while (1) {
		room_offs = room ? _roomFileOffsets[room] : 0;

		if (room_offs == -1)
			break;

		if (room_offs != 0 && room != 0) {
			_fileOffset = _roomFileOffsets[room];
			return;
		}

		// try converted resource
		sprintf(buf, "%s.ST%01d", _gameName.c_str(), room == 0 ? 0 : res.roomno[rtRoom][room]);
		strupr(buf);

		result = openResourceFile(buf, ATARI_RESOURCE_XOR);
		/*
		if (!result)
		{
			// try v5-v6 resource
			sprintf(buf, "%s.%03d",  _gameName.c_str(), room == 0 ? 0 : res.roomno[rtRoom][room]);
			encByte = (_features & GF_USE_KEY) ? 0x69 : 0;
			result = openResourceFile(buf, encByte);
			if (!result && (_gameId == GID_SAMNMAX))
			{
				// try samnmax resource
				sprintf(buf, "%s.sm%01d",  _gameName.c_str(), room == 0 ? 0 : res.roomno[rtRoom][room]);
				result = openResourceFile(buf, encByte);
			}		
		}
		*/
			
		if (result)
		{
			if (room == 0)
				return;

			readRoomsOffsets();
			_fileOffset = _roomFileOffsets[room];

			if (_fileOffset != 8)
				return;

			error("Room %d not in %s", room, buf);
			return;
		}
		askForDisk(buf, room == 0 ? 0 : res.roomno[rtRoom][room]);
	}

	// try loose room files
	/*
	do {
		sprintf(buf, "%03d.lfl", room);
		if (openResourceFile(buf, 0))
			break;
		askForDisk(buf, room == 0 ? 0 : res.roomno[rtRoom][room]);
	} while (1);
	*/
	deleteRoomOffsets();
	_fileOffset = 0;		// start of file
}

void ScummEngine::closeRoom() {
	if (_lastLoadedRoom != -1) {
		_lastLoadedRoom = -1;
		deleteRoomOffsets();
		_fileHandle.close();
	}
}

/** Delete the currently loaded room offsets. */
void ScummEngine::deleteRoomOffsets() {
	if (!_dynamicRoomOffsets)
		return;

	for (int i = 0; i < _numRooms; i++) {
		if (_roomFileOffsets[i] != 0xFFFFFFFF)
			_roomFileOffsets[i] = 0;
	}
}

/** Read room offsets */
void ScummEngine::readRoomsOffsets() {
	int num, room;

	debug(9, "readRoomOffsets()");

	deleteRoomOffsets();

	if (!_dynamicRoomOffsets)
		return;

	_fileHandle.seek(16, SEEK_SET);

	num = _fileHandle.readByte();
	while (num--) {
		room = _fileHandle.readByte();
		if (_roomFileOffsets[room] != 0xFFFFFFFF) {
			_roomFileOffsets[room] = _fileHandle.readUint32LE();
		} else {
			_fileHandle.readUint32LE();
		}
	}
}

bool ScummEngine::openResourceFile(const char *filename, byte encByte) {
	if (_fileHandle.isOpen()) {
		_fileHandle.close();
	}

	_fileHandle.open(filename, getGameDataPath(), File::kFileReadMode, encByte);

	bool ret = _fileHandle.isOpen();
	if (ret)
	{
		debug(1, "openResourceFile(%s)(%02x)", filename,encByte);
		return true;
	}
	return false;
}

void ScummEngine::askForDisk(const char *filename, int disknum) {
	char buf[128];
	sprintf(buf, "Cannot find file: %s", filename);
	InfoDialog dialog(this, (char*)buf);
	runDialog(dialog);
	error("Cannot find file: '%s'", filename);
	g_system->quit();
}

void ScummEngine::readIndexFile() {
	uint32 blocktype, itemsize;
	int numblock = 0;
	int num, i;
	bool stop = false;

	debugC(DEBUG_GENERAL, "readIndexFile()");

	closeRoom();
	openRoom(0);

	uint16 resourceVersion = 0;

	if (_version == 5) {
		/* Figure out the sizes of various resources */
		while (!_fileHandle.eof()) {
			blocktype = fileReadDword();
			itemsize = _fileHandle.readUint32BE();
			if (_fileHandle.ioFailed())
				break;
			switch (blocktype) {
			case MKID('DOBJ'):
				_numGlobalObjects = _fileHandle.readUint16LE();
				itemsize -= 2;
				break;
			case MKID('DROO'):
				_numRooms = _fileHandle.readUint16LE();
				itemsize -= 2;
				break;

			case MKID('DSCR'):
				_numScripts = _fileHandle.readUint16LE();
				itemsize -= 2;
				break;

			case MKID('DCOS'):
				_numCostumes = _fileHandle.readUint16LE();
				itemsize -= 2;
				break;

			case MKID('DSOU'):
				_numSounds = _fileHandle.readUint16LE();
				itemsize -= 2;
				break;
			}
			_fileHandle.seek(itemsize - 8, SEEK_CUR);
		}
		_fileHandle.clearIOFailed();
		_fileHandle.seek(0, SEEK_SET);
	}

	while (!stop) {
		blocktype = fileReadDword();

		if (_fileHandle.ioFailed())
			break;
		itemsize = _fileHandle.readUint32BE();

		numblock++;

		debug(1, "block '%s' (%d)", tag2str(blocktype), itemsize);

		switch (blocktype) {

		case MKID('_ST_'):
			_resourceVersion = _fileHandle.readUint16BE();
			_fileHandle.readUint16BE();
			debug(1,"Atari resource v.: %d", i);
			break;

		case MKID('DCHR'):
		case MKID('DIRF'):
			readResTypeList(rtCharset, MKID('CHAR'), "charset");
			break;
		
		case MKID('DOBJ'):
			num = _fileHandle.readUint16LE();
			assert(num == _numGlobalObjects);

			_fileHandle.read(_objectOwnerTable, num);
			for (i = 0; i < num; i++) {
				_objectStateTable[i] = _objectOwnerTable[i] >> OF_STATE_SHL;
				_objectOwnerTable[i] &= OF_OWNER_MASK;
			}
			
			_fileHandle.read(_classData, num * sizeof(uint32));

			// Swap flag endian where applicable
#if defined(SCUMM_BIG_ENDIAN)
			for (i = 0; i != num; i++)
				_classData[i] = FROM_LE_32(_classData[i]);
#endif
			break;

		case MKID('RNAM'):
			_fileHandle.seek(itemsize - 8, SEEK_CUR);
			break;
		
		case MKID('DLFL'):
			i = _fileHandle.readUint16LE();
			_fileHandle.seek(-2, SEEK_CUR);
			_HEV7RoomOffsets = (byte *)calloc(2 + (i * 4), 1);
			_fileHandle.read(_HEV7RoomOffsets, (2 + (i * 4)) );
			break;

		case MKID('DIRM'):
			_fileHandle.seek(itemsize - 8, SEEK_CUR);
			break;
			
		case MKID('DIRI'):
			num = _fileHandle.readUint16LE();
			_fileHandle.seek(num + (8 * num), SEEK_CUR);
			break;

		case MKID('ANAM'):
			_numAudioNames = _fileHandle.readUint16LE();
			_audioNames = (char*)malloc(_numAudioNames * 9);
			_fileHandle.read(_audioNames, _numAudioNames * 9);
			break;

		case MKID('DROO'):
			readResTypeList(rtRoom, MKID('ROOM'), "room");
			break;

		case MKID('DIRR'):
			readResTypeList(rtRoom, MKID('RMDA'), "room");
			break;

		case MKID('DRSC'):					// FIXME: Verify
			readResTypeList(rtRoomScripts, MKID('RMSC'), "room script");
			break;

		case MKID('DSCR'):
		case MKID('DIRS'):
			readResTypeList(rtScript, MKID('SCRP'), "script");
			break;

		case MKID('DCOS'):
		case MKID('DIRC'):
			readResTypeList(rtCostume, MKID('COST'), "costume");
			break;

		case MKID('MAXS'):
			readMAXS();
			break;

		case MKID('DSOU'):
			readResTypeList(rtSound, MKID('SOUN'), "sound");
			break;

		case MKID('DIRN'):
			readResTypeList(rtSound, MKID('DIRN'), "sound");
			break;

		case MKID('AARY'):
			debug(1, "Going to call readArrayFromIndexFile (pos = 0x%08x)", _fileHandle.pos());
			readArrayFromIndexFile();
			debug(1, "After readArrayFromIndexFile (pos = 0x%08x)", _fileHandle.pos());
			break;

		default:
			error("Bad ID '%s' found in directory!", tag2str(blocktype));
			return;
		}
	}

//  if (numblock!=9)
//    error("Not enough blocks read from directory");

	closeRoom();
}

void ScummEngine::readArrayFromIndexFile() {
	error("readArrayFromIndexFile() not supported in pre-V6 games");
	// the code for v6 games is in script_v6.cpp
}

void ScummEngine::readResTypeList(int id, uint32 tag, const char *name) {
	int num;
	int i;

	num = _fileHandle.readUint16LE();

	debug(1, "readResTypeList(%d, %s,%x,%s) : %d", id, resTypeFromId(id), FROM_LE_32(tag), name, num);

	if (num != res.num[id]) {
		error("Invalid number of %ss (%d) in directory", name, num);
	}

	for (i = 0; i < num; i++) {
		res.roomno[id][i] = _fileHandle.readByte();
	}
	for (i = 0; i < num; i++) {
		res.roomoffs[id][i] = _fileHandle.readUint32LE();
	}
}

void ScummEngine::allocResTypeData(int id, uint32 tag, int num, const char *name, int mode) {
	debug(1, "allocResTypeData(%s/%s,%x,%d,%d)", resTypeFromId(id), name, FROM_LE_32(tag), num, mode);
	assert(id >= 0 && id < (int)(ARRAYSIZE(res.mode)));

	if (num >= 2000) {
		error("Too many %ss (%d) in directory", name, num);
	}

	res.mode[id] = mode;
	res.num[id] = num;
	res.tags[id] = tag;
	res.name[id] = name;
	res.address[id] = (byte **)calloc(num, sizeof(void *));
	res.flags[id] = (byte *)calloc(num, sizeof(byte));

	if (mode) {
		res.roomno[id] = (byte *)calloc(num, sizeof(byte));
		res.roomoffs[id] = (uint32 *)calloc(num, sizeof(uint32));
	}
}

void ScummEngine::loadCharset(int no) {
	int i;
	byte *ptr;

	debugC(DEBUG_GENERAL, "loadCharset(%d)", no);

	/* FIXME - hack around crash in Indy4 (occurs if you try to load after dieing) */
	if (_gameId == GID_INDY4 && no == 0)
		no = 1;

	assert(no < (int)sizeof(_charsetData) / 16);
	checkRange(_numCharsets - 1, 1, no, "Loading illegal charset %d");

//  ensureResourceLoaded(rtCharset, no);
	ptr = getResourceAddress(rtCharset, no);

	for (i = 0; i < 15; i++) {
		_charsetData[no][i + 1] = ptr[i + 14];
	}
}

void ScummEngine::nukeCharset(int i) {
	checkRange(_numCharsets - 1, 1, i, "Nuking illegal charset %d");
	nukeResource(rtCharset, i);
}

void ScummEngine::ensureResourceLoaded(int type, int i) {
	void *addr = NULL;

	debugC(DEBUG_RESOURCE, "ensureResourceLoaded(%s,%d)", resTypeFromId(type), i);

	if (type == rtRoom && i > 0x7F && _version < 7) {
		i = _resourceMapper[i & 0x7F];
	}

	// FIXME - TODO: This check used to be "i==0". However, that causes
	// problems when using this function to ensure charset 0 is loaded.
	// This is done for many games, e.g. Zak256 or Indy3 (EGA and VGA).
	// For now we restrict the check to anything which is not a charset.
	// Question: Why was this check like that in the first place?
	// Answer: costumes with an index of zero in the newer games at least.
	// TODO: determine why the heck anything would try to load a costume
	// with id 0. Is that "normal", or is it caused by yet another bug in
	// our code base? After all we also have to add special cases for many
	// of our script opcodes that check for the (invalid) actor 0... so 
	// maybe both issues are related...
	if (type != rtCharset && i == 0)
		return;

	if (i <= res.num[type])
		addr = res.address[type][i];

	if (addr)
		return;

	loadResource(type, i);

	if (type == rtRoom && i == _roomResource)
		VAR(VAR_ROOM_FLAG) = 1;
}

int ScummEngine::loadResource(int type, int idx) {
	int roomNr;
	uint32 fileOffs;
	uint32 size, tag;

	debug(1, "loadResource(%s,%d)", resTypeFromId(type),idx);

	roomNr = getResourceRoomNr(type, idx);

	if (idx >= res.num[type])
		error("%s %d undefined %d %d", res.name[type], idx, res.num[type], roomNr);

	if (roomNr == 0)
		roomNr = _roomResource;

	if (type == rtRoom) {
		fileOffs = 0;
	} else {
		fileOffs = res.roomoffs[type][idx];
		if (fileOffs == 0xFFFFFFFF)
			return 0;
	}

	openRoom(roomNr);

	_fileHandle.seek(fileOffs + _fileOffset, SEEK_SET);

	if (type == rtSound) {
		return readSoundResource(type, idx);
	}

	tag = fileReadDword();

	if (tag != res.tags[type]) {
		warning("%s %d not in room %d at %d+%d in file %s",
				res.name[type], idx, roomNr,
				_fileOffset, fileOffs, _fileHandle.name());
	}

	size = _fileHandle.readUint32BE();
	_fileHandle.seek(-8, SEEK_CUR);
	_fileHandle.read(createResource(type, idx, size), size);

	// dump the resource if requested
	if (!_fileHandle.ioFailed()) {
		return 1;
	}

	nukeResource(type, idx);

	error("Cannot read resource");
	return 0;
}

int ScummEngine::readSoundResource(int type, int idx) {
	uint32 pos, total_size, size, tag, basetag, max_total_size;
	int pri, best_pri;
	uint32 best_size = 0, best_offs = 0;

	debugC(DEBUG_RESOURCE, "readSoundResource(%s,%d)", resTypeFromId(type), idx);

	pos = 0;

	_fileHandle.readUint32LE();
	max_total_size = _fileHandle.readUint32BE() - 8;
	basetag = fileReadDword();
	total_size = _fileHandle.readUint32BE();

	debugC(DEBUG_RESOURCE, "  basetag: %s, total_size=%d", tag2str(TO_BE_32(basetag)), total_size);

	if (basetag == MKID('MIDI') || basetag == MKID('iMUS')) {
		if (_midiDriver != MD_PCSPK && _midiDriver != MD_PCJR) {
			_fileHandle.seek(-8, SEEK_CUR);
			_fileHandle.read(createResource(type, idx, total_size + 8), total_size + 8);
			return 1;
		}
	} else if (basetag == MKID('SOU ')) {
		best_pri = -1;
		while (pos < total_size) {
			tag = fileReadDword();
			size = _fileHandle.readUint32BE() + 8;
			pos += size;

			pri = -1;

			switch (tag) {
			case MKID('TOWS'):
				pri = 16;
				break;
			case MKID('SBL '):
				pri = 15;
				break;
			case MKID('ADL '):
				pri = 1;
				if (_midiDriver == MD_ADLIB)
					pri = 10;
				break;
			case MKID('AMI '):
				pri = 3;
				break;
			case MKID('ROL '):
				pri = 3;
				if (_native_mt32)
					pri = 5;
				break;
			case MKID('GMD '):
				pri = 4;
				break;
			case MKID('MAC '):
				pri = 2;
				break;
			case MKID('SPK '):
				pri = -1;
//				if (_midiDriver == MD_PCSPK)
//					pri = 11;
				break;
			}

			if ((_midiDriver == MD_PCSPK || _midiDriver == MD_PCJR) && pri != 11)
				pri = -1;

			debugC(DEBUG_RESOURCE, "    tag: %s, total_size=%d, pri=%d", tag2str(TO_BE_32(tag)), size, pri);


			if (pri > best_pri) {
				best_pri = pri;
				best_size = size;
				best_offs = _fileHandle.pos();
			}

			_fileHandle.seek(size - 8, SEEK_CUR);
		}

		if (best_pri != -1) {
			_fileHandle.seek(best_offs - 8, SEEK_SET);
			_fileHandle.read(createResource(type, idx, best_size), best_size);
			return 1;
		}
	} else if (basetag == MKID('Mac0')) {
		_fileHandle.seek(-12, SEEK_CUR);
		total_size = _fileHandle.readUint32BE() - 8;
		byte *ptr = (byte *)calloc(total_size, 1);
		_fileHandle.read(ptr, total_size);
		convertMac0Resource(type, idx, ptr, total_size);
		free(ptr);
		return 1;
	} else if (basetag == MKID('Mac1')) {
		_fileHandle.seek(-12, SEEK_CUR);
		total_size = _fileHandle.readUint32BE();
		_fileHandle.read(createResource(type, idx, total_size), total_size - 8);
		return 1;
	} else if (basetag == MKID('DIGI')) {
		// Use in Putt-Putt Demo
		debugC(DEBUG_SOUND, "Found base tag DIGI in sound %d, size %d", idx, total_size);
		debugC(DEBUG_SOUND, "It was at position %d", _fileHandle.pos());

		_fileHandle.seek(-12, SEEK_CUR);
		total_size = _fileHandle.readUint32BE();
		_fileHandle.read(createResource(type, idx, total_size), total_size - 8);
		return 1;
	} else if (basetag == MKID('FMUS')) {
		// Used in 3DO version of puttputt joins the parade and probably others
		// Specifies a separate file to be used for music from what I gather.
		int tmpsize;
		int i = 0;
		File dmuFile;
		char buffer[128];
		debugC(DEBUG_SOUND, "Found base tag FMUS in sound %d, size %d", idx, total_size);
		debugC(DEBUG_SOUND, "It was at position %d", _fileHandle.pos());
		
		_fileHandle.seek(4, SEEK_CUR);
		// HSHD size
		tmpsize = _fileHandle.readUint32BE();
		// skip to size part of the SDAT block
		_fileHandle.seek(tmpsize - 4, SEEK_CUR);
		// SDAT size
		tmpsize = _fileHandle.readUint32BE();
		
		// SDAT contains name of file we want
		for (i = 0; (buffer[i] != ' ') && (i < tmpsize - 8) ; i++) {
			buffer[i] = _fileHandle.readByte();
		}
		buffer[tmpsize - 11] = '\0';
		debugC(DEBUG_SOUND, "FMUS file %s", buffer);
		if (dmuFile.open(buffer, getGameDataPath()) == false) {
			warning("Can't open music file %s*", buffer);
			res.roomoffs[type][idx] = 0xFFFFFFFF;
			return 0;
		}
		dmuFile.seek(4, SEEK_SET);
		total_size = dmuFile.readUint32BE();
		debugC(DEBUG_SOUND, "dmu file size %d", total_size);
		dmuFile.seek(-8, SEEK_CUR);
		dmuFile.read(createResource(type, idx, total_size), total_size);
		dmuFile.close();
		return 1;
	} else if (basetag == MKID('Crea')) {
		_fileHandle.seek(-12, SEEK_CUR);
		total_size = _fileHandle.readUint32BE();
		_fileHandle.read(createResource(type, idx, total_size), total_size - 8);
		return 1;
	} else if (FROM_LE_32(basetag) == max_total_size) {
		_fileHandle.seek(-12, SEEK_CUR);
		total_size = _fileHandle.readUint32BE();
		_fileHandle.seek(-8, SEEK_CUR);
		_fileHandle.read(createResource(type, idx, total_size), total_size);
		return 1;
	} else {
		warning("Unrecognized base tag 0x%08x in sound %d", basetag, idx);
	}
	res.roomoffs[type][idx] = 0xFFFFFFFF;
	return 0;
}

// Adlib MIDI-SYSEX to set MIDI instruments for small header games.
static byte ADLIB_INSTR_MIDI_HACK[95] = {
	0x00, 0xf0, 0x14, 0x7d, 0x00,  // sysex 00: part on/off
	0x00, 0x00, 0x03,              // part/channel  (offset  5)
	0x00, 0x00, 0x07, 0x0f, 0x00, 0x00, 0x08, 0x00, 
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0xf7,
	0x00, 0xf0, 0x41, 0x7d, 0x10,  // sysex 16: set instrument
	0x00, 0x01,                    // part/channel  (offset 28)
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0xf7,
	0x00, 0xb0, 0x07, 0x64        // Controller 7 = 100 (offset 92)
};
			
static const byte map_param[7] = {
	0, 2, 3, 4, 8, 9, 0,
};

static const byte freq2note[128] = {
	/*128*/	6, 6, 6, 6,  
	/*132*/ 7, 7, 7, 7, 7, 7, 7,
	/*139*/ 8, 8, 8, 8, 8, 8, 8, 8, 8,
	/*148*/ 9, 9, 9, 9, 9, 9, 9, 9, 9,
	/*157*/ 10, 10, 10, 10, 10, 10, 10, 10, 10,
	/*166*/ 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	/*176*/ 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	/*186*/ 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	/*197*/ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	/*209*/ 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	/*222*/ 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	/*235*/ 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
	/*249*/ 18, 18, 18, 18, 18, 18, 18
};
			
static const uint16 num_steps_table[] = {
	1, 2, 4, 5,
	6, 7, 8, 9,
	10, 12, 14, 16,
	18, 21, 24, 30,
	36, 50, 64, 82,
	100, 136, 160, 192,
	240, 276, 340, 460,
	600, 860, 1200, 1600
};
int ScummEngine::convert_extraflags(byte * ptr, byte * src_ptr) {
	int flags = src_ptr[0];

	int t1, t2, t3, t4, time;
	int v1, v2, v3;

	if (!(flags & 0x80))
		return -1;

	t1 = (src_ptr[1] & 0xf0) >> 3;
	t2 = (src_ptr[2] & 0xf0) >> 3;
	t3 = (src_ptr[3] & 0xf0) >> 3 | (flags & 0x40 ? 0x80 : 0);
	t4 = (src_ptr[3] & 0x0f) << 1;
	v1 = (src_ptr[1] & 0x0f);
	v2 = (src_ptr[2] & 0x0f);
	v3 = 31;
	if ((flags & 0x7) == 0) {
	  v1 = v1 + 31 + 8;
	  v2 = v2 + 31 + 8;
	} else {
	  v1 = v1 * 2 + 31;
	  v2 = v2 * 2 + 31;
	}
	
	/* flags a */
	if ((flags & 0x7) == 6)
		ptr[0] = 0;
	else {
		ptr[0] = (flags >> 4) & 0xb;
		ptr[1] = map_param[flags & 0x7];
	}

	/* extra a */
	ptr[2] = 0;
	ptr[3] = 0;
	ptr[4] = t1 >> 4;
	ptr[5] = t1 & 0xf;
	ptr[6] = v1 >> 4;
	ptr[7] = v1 & 0xf;
	ptr[8] = t2 >> 4;
	ptr[9] = t2 & 0xf;
	ptr[10] = v2 >> 4;
	ptr[11] = v2 & 0xf;
	ptr[12] = t3 >> 4;
	ptr[13] = t3 & 0xf;
	ptr[14] = t4 >> 4;
	ptr[15] = t4 & 0xf;
	ptr[16] = v3 >> 4;
	ptr[17] = v3 & 0xf;

	time = num_steps_table[t1] + num_steps_table[t2]
		+ num_steps_table[t3 & 0x7f] + num_steps_table[t4];
	if (flags & 0x20) {
		int playtime = ((src_ptr[4] >> 4) & 0xf) * 118 + 
			(src_ptr[4] & 0xf) * 8;
		if (playtime > time)
			time = playtime;
	}
	/*
	time = ((src_ptr[4] >> 4) & 0xf) * 118 + 
		(src_ptr[4] & 0xf) * 8;
	*/
	return time;
}

#define kMIDIHeaderSize		46
static inline byte *writeMIDIHeader(byte *ptr, const char *type, int ppqn, int total_size) {
	uint32 dw = TO_BE_32(total_size);
	
	memcpy(ptr, type, 4); ptr += 4;
	memcpy(ptr, &dw, 4); ptr += 4;
	memcpy(ptr, "MDhd", 4); ptr += 4;
	ptr[0] = 0; ptr[1] = 0; ptr[2] = 0; ptr[3] = 8;
	ptr += 4;
	memset(ptr, 0, 8), ptr += 8;
	memcpy(ptr, "MThd", 4); ptr += 4;
	ptr[0] = 0; ptr[1] = 0; ptr[2] = 0; ptr[3] = 6;
	ptr += 4;
	ptr[0] = 0; ptr[1] = 0; ptr[2] = 0; ptr[3] = 1; // MIDI format 0 with 1 track
	ptr += 4;
	
	*ptr++ = ppqn >> 8;
	*ptr++ = ppqn & 0xFF;

	memcpy(ptr, "MTrk", 4); ptr += 4;
	memcpy(ptr, &dw, 4); ptr += 4;

	return ptr;
}

static inline byte *writeVLQ(byte *ptr, int value) {
	if (value > 0x7f) {
		if (value > 0x3fff) {
			*ptr++ = (value >> 14) | 0x80;
			value &= 0x3fff;
		}
		*ptr++ = (value >> 7) | 0x80;
		value &= 0x7f;
	}
	*ptr++ = value;
	return ptr;
}

static inline byte Mac0ToGMInstrument(uint32 type, int &transpose) {
	transpose = 0;
	switch (type) {
	case MKID('MARI'): return 12;
	case MKID('PLUC'): return 45;
	case MKID('HARM'): return 22;
	case MKID('PIPE'): return 19;
	case MKID('TROM'): transpose = -12; return 57;
	case MKID('STRI'): return 48;
	case MKID('HORN'): return 60;
	case MKID('VIBE'): return 11;
	case MKID('SHAK'): return 77;
	case MKID('PANP'): return 75;
	case MKID('WHIS'): return 76;
	case MKID('ORGA'): return 17;
	case MKID('BONG'): return 115;
	case MKID('BASS'): transpose = -24; return 35;
	default:
		error("Unknown Mac0 instrument %s found", tag2str(type));
		return 0;
	}
}

void ScummEngine::convertMac0Resource(int type, int idx, byte *src_ptr, int size) {
	/*
	From Markus Magnuson (superqult) we got this information:
	Mac0
	---
	   4 bytes - 'SOUN'
	BE 4 bytes - block length
	
		   4 bytes  - 'Mac0'
		BE 4 bytes  - (blockLength - 27)
		   28 bytes - ???
	
		   do this three times (once for each channel):
			  4 bytes  - 'Chan'
		   BE 4 bytes  - channel length
			  4 bytes  - instrument name (e.g. 'MARI')
	
			  do this for ((chanLength-24)/4) times:
				 2 bytes  - note duration
				 1 byte   - note value
				 1 byte   - note velocity
	
			  4 bytes - ???
			  4 bytes - 'Loop'/'Done'
			  4 bytes - ???
	
	   1 byte - 0x09
	---
	
	Instruments (General Midi):
	"MARI" - Marimba (12)
	"PLUC" - Pizzicato Strings (45)
	"HARM" - Harmonica (22)
	"PIPE" - Church Organ? (19) or Flute? (73) or Bag Pipe (109)
	"TROM" - Trombone (57)
	"STRI" - String Ensemble (48 or 49)
	"HORN" - French Horn? (60) or English Horn? (69)
	"VIBE" - Vibraphone (11)
	"SHAK" - Shakuhachi? (77)
	"PANP" - Pan Flute (75)
	"WHIS" - Whistle (78) / Bottle (76)
	"ORGA" - Drawbar Organ (16; but could also be 17-20)
	"BONG" - Woodblock? (115)
	"BASS" - Bass (32-39)
	
	
	Now the task could be to convert this into MIDI, to be fed into iMuse.
	Or we do something similiar to what is done in Player_V3, assuming
	we can identify SFX in the MI datafiles for each of the instruments
	listed above.
	*/

#if 0
	byte *ptr = createResource(type, idx, size);
	memcpy(ptr, src_ptr, size);
#else
	const int ppqn = 480;
	byte *ptr, *start_ptr;
	
	int total_size = 0;
	total_size += kMIDIHeaderSize; // Header
	total_size += 7;               // Tempo META
	total_size += 3 * 3;           // Three program change mesages
	total_size += 22;              // Possible jump SysEx
	total_size += 5;               // EOT META
	
	int i, len;
	byte track_instr[3];
	byte *track_data[3];
	int track_len[3];
	int track_transpose[3];
	bool looped = false;

	src_ptr += 8;
	// TODO: Decipher the unknown bytes in the header. For now, skip 'em
	src_ptr += 28;

	// Parse the three channels
	for (i = 0; i < 3; i++) {
		assert(*((uint32*)src_ptr) == MKID('Chan'));
		len = READ_BE_UINT32(src_ptr + 4);
		track_len[i] = len - 24;
		track_instr[i] = Mac0ToGMInstrument(*(uint32*)(src_ptr + 8), track_transpose[i]);
		track_data[i] = src_ptr + 12;
		src_ptr += len;
		looped = (*((uint32*)(src_ptr - 8)) == MKID('Loop'));
		
		// For each note event, we need up to 6 bytes for the
		// Note On (3 VLQ, 3 event), and 6 bytes for the Note
		// Off (3 VLQ, 3 event). So 12 bytes total.
		total_size += 12 * track_len[i];
	}
	assert(*src_ptr == 0x09);
	
	// Create sound resource
	start_ptr = createResource(type, idx, total_size);
	
	// Insert MIDI header
	ptr = writeMIDIHeader(start_ptr, "GMD ", ppqn, total_size);

	// Write a tempo change Meta event
	// 473 / 4 Hz, convert to micro seconds.
	uint32 dw = 1000000 * 437 / 4 / ppqn; // 1000000 * ppqn * 4 / 473;
	memcpy(ptr, "\x00\xFF\x51\x03", 4); ptr += 4;
	*ptr++ = (byte)((dw >> 16) & 0xFF);
	*ptr++ = (byte)((dw >> 8) & 0xFF);
	*ptr++ = (byte)(dw & 0xFF);

	// Insert program change messages
	*ptr++ = 0; // VLQ
	*ptr++ = 0xC0;
	*ptr++ = track_instr[0];
	*ptr++ = 0; // VLQ
	*ptr++ = 0xC1;
	*ptr++ = track_instr[1];
	*ptr++ = 0; // VLQ
	*ptr++ = 0xC2;
	*ptr++ = track_instr[2];
	
	// And now, the actual composition. Please turn all cell phones
	// and pagers off during the performance. Thank you.
	uint16 nextTime[3] = { 1, 1, 1 };
	int stage[3] = { 0, 0, 0 };

	while (track_len[0] | track_len[1] | track_len[2]) {
		int best = -1;
		uint16 bestTime = 0xFFFF;
		for (i = 0; i < 3; ++i) {
			if (track_len[i] && nextTime[i] < bestTime) {
				bestTime = nextTime[i];
				best = i;
			}
		}
		assert (best != -1);

		if (!stage[best]) {
			// We are STARTING this event.
			if (track_data[best][2] > 1) {
				// Note On
				ptr = writeVLQ(ptr, nextTime[best]);
				*ptr++ = 0x90 | best;
				*ptr++ = track_data[best][2] + track_transpose[best];
				*ptr++ = track_data[best][3] * 127 / 100; // Scale velocity
				for (i = 0; i < 3; ++i)
					nextTime[i] -= bestTime;
			}
			nextTime[best] += READ_BE_UINT16 (track_data[best]);
			stage[best] = 1;
		} else {
			// We are ENDING this event.
			if (track_data[best][2] > 1) {
				// There was a Note On, so do a Note Off
				ptr = writeVLQ(ptr, nextTime[best]);
				*ptr++ = 0x80 | best;
				*ptr++ = track_data[best][2] + track_transpose[best];
				*ptr++ = track_data[best][3] * 127 / 100; // Scale velocity
				for (i = 0; i < 3; ++i)
					nextTime[i] -= bestTime;
			}
			track_data[best] += 4;
			track_len[best] -= 4;
			stage[best] = 0;
		}
	}

	// Is this a looped song? If so, effect a loop by
	// using the S&M maybe_jump SysEx command.
	// FIXME: Jamieson630: The jump seems to be happening
	// too quickly! There should maybe be a pause after
	// the last Note Off? But I couldn't find one in the
	// MI1 Lookout music, where I was hearing problems.
	if (looped) {
		memcpy(ptr, "\x00\xf0\x13\x7d\x30\00", 6); ptr += 6; // maybe_jump
		memcpy(ptr, "\x00\x00", 2); ptr += 2;            // cmd -> 0 means always jump
		memcpy(ptr, "\x00\x00\x00\x00", 4); ptr += 4;    // track -> 0 (only track)
		memcpy(ptr, "\x00\x00\x00\x01", 4); ptr += 4;    // beat -> 1 (first beat)
		memcpy(ptr, "\x00\x00\x00\x01", 4); ptr += 4;    // tick -> 1
		memcpy(ptr, "\x00\xf7", 2); ptr += 2;            // SysEx end marker
	}

	// Insert end of song META
	memcpy(ptr, "\x00\xff\x2f\x00\x00", 5); ptr += 5;
	
	assert(ptr <= start_ptr + total_size);
	
	// Rewrite MIDI header, this time with true size
	total_size = ptr - start_ptr;
	ptr = writeMIDIHeader(start_ptr, "GMD ", ppqn, total_size);
#endif
}

void ScummEngine::convertADResource(int type, int idx, byte *src_ptr, int size) {

	// We will ignore the PPQN in the original resource, because
	// it's invalid anyway. We use a constant PPQN of 480.
	const int ppqn = 480;
	uint32 dw;
	int i, ch;
	byte *ptr;
	int total_size = kMIDIHeaderSize + 7 + 8 * sizeof(ADLIB_INSTR_MIDI_HACK) + size;
	total_size += 24;	// Up to 24 additional bytes are needed for the jump sysex
	
	ptr = createResource(type, idx, total_size);

	src_ptr += 2;
	size -= 2;

	// 0x80 marks a music resource. Otherwise it's a SFX
	if (*src_ptr == 0x80) {
		byte ticks, play_once;
		byte num_instr;
		byte *channel, *instr, *track;

		ptr = writeMIDIHeader(ptr, "ADL ", ppqn, total_size);

		// The "speed" of the song
		ticks = *(src_ptr + 1);
		
		// Flag that tells us whether we should loop the song (0) or play it only once (1)
		play_once = *(src_ptr + 2);
		
		// Number of instruments used
		num_instr = *(src_ptr + 8);	// Normally 8
		
		// copy the pointer to instrument data
		channel = src_ptr + 9;
		instr   = src_ptr + 0x11;
		
		// skip over the rest of the header and copy the MIDI data into a buffer
		src_ptr  += 0x11 + 8 * 16;
		size -= 0x11 + 8 * 16;

		CHECK_HEAP 
			
		track = src_ptr;
		
		// Convert the ticks into a MIDI tempo. 
		dw = 500000 * 256 / ticks;
		debugC(DEBUG_SOUND, "  ticks = %d, speed = %ld", ticks, dw);
			
		// Write a tempo change Meta event
		memcpy(ptr, "\x00\xFF\x51\x03", 4); ptr += 4;
		*ptr++ = (byte)((dw >> 16) & 0xFF);
		*ptr++ = (byte)((dw >> 8) & 0xFF);
		*ptr++ = (byte)(dw & 0xFF);
		
		// Copy our hardcoded instrument table into it
		// Then, convert the instrument table as given in this song resource
		// And write it *over* the hardcoded table.
		// Note: we deliberately.
		
		/* now fill in the instruments */
		for (i = 0; i < num_instr; i++) {
			ch = channel[i] - 1;
			if (ch < 0 || ch > 15)
				continue;

			if (instr[i*16 + 13])
				warning("Sound %d instrument %d uses percussion", idx, i);

			debugC(DEBUG_SOUND, "Sound %d: instrument %d on channel %d.", idx, i, ch);

			memcpy(ptr, ADLIB_INSTR_MIDI_HACK, sizeof(ADLIB_INSTR_MIDI_HACK));

			ptr[5]  += ch;
			ptr[28] += ch;
			ptr[92] += ch;

			/* flags_1 */
			ptr[30 + 0] = (instr[i * 16 + 3] >> 4) & 0xf;
			ptr[30 + 1] = instr[i * 16 + 3] & 0xf;

			/* oplvl_1 */
			ptr[30 + 2] = (instr[i * 16 + 4] >> 4) & 0xf;
			ptr[30 + 3] = instr[i * 16 + 4] & 0xf;

			/* atdec_1 */
			ptr[30 + 4] = ((~instr[i * 16 + 5]) >> 4) & 0xf;
			ptr[30 + 5] = (~instr[i * 16 + 5]) & 0xf;

			/* sustrel_1 */
			ptr[30 + 6] = ((~instr[i * 16 + 6]) >> 4) & 0xf;
			ptr[30 + 7] = (~instr[i * 16 + 6]) & 0xf;

			/* waveform_1 */
			ptr[30 + 8] = (instr[i * 16 + 7] >> 4) & 0xf;
			ptr[30 + 9] = instr[i * 16 + 7] & 0xf;

			/* flags_2 */
			ptr[30 + 10] = (instr[i * 16 + 8] >> 4) & 0xf;
			ptr[30 + 11] = instr[i * 16 + 8] & 0xf;

			/* oplvl_2 */
			ptr[30 + 12] = (instr[i * 16 + 9] >> 4) & 0xf;
			ptr[30 + 13] = instr[i * 16 + 9] & 0xf;

			/* atdec_2 */
			ptr[30 + 14] = ((~instr[i * 16 + 10]) >> 4) & 0xf;
			ptr[30 + 15] = (~instr[i * 16 + 10]) & 0xf;

			/* sustrel_2 */
			ptr[30 + 16] = ((~instr[i * 16 + 11]) >> 4) & 0xf;
			ptr[30 + 17] = (~instr[i * 16 + 11]) & 0xf;

			/* waveform_2 */
			ptr[30 + 18] = (instr[i * 16 + 12] >> 4) & 0xf;
			ptr[30 + 19] = instr[i * 16 + 12] & 0xf;

			/* feedback */
			ptr[30 + 20] = (instr[i * 16 + 2] >> 4) & 0xf;
			ptr[30 + 21] = instr[i * 16 + 2] & 0xf;
			ptr += sizeof(ADLIB_INSTR_MIDI_HACK);
		}

		// There is a constant delay of ppqn/3 before the music starts.
		if (ppqn / 3 >= 128)
			*ptr++ = (ppqn / 3 >> 7) | 0x80;
		*ptr++ = ppqn / 3 & 0x7f;

		// Now copy the actual music data 
		memcpy(ptr, track, size);
		ptr += size;

		if (!play_once) {
			// The song is meant to be looped. We achieve this by inserting just
			// before the song end a jump to the song start. More precisely we abuse
			// a S&M sysex, "maybe_jump" to achieve this effect. We could also
			// use a set_loop sysex, but it's a bit longer, a little more complicated,
			// and has no advantage either.

			// First, find the track end
			byte *end = ptr;
			ptr -= size;
			for (; ptr < end; ptr++) {
				if (*ptr == 0xff && *(ptr + 1) == 0x2f)
					break;
			}
			assert(ptr < end);

			// Now insert the jump. The jump offset is measured in ticks.
			// We have ppqn/3 ticks before the first note.

			const int jump_offset = ppqn / 3;
			memcpy(ptr, "\xf0\x13\x7d\x30\00", 5); ptr += 5;	// maybe_jump
			memcpy(ptr, "\x00\x00", 2); ptr += 2;			// cmd -> 0 means always jump
			memcpy(ptr, "\x00\x00\x00\x00", 4); ptr += 4;	// track -> there is only one track, 0
			memcpy(ptr, "\x00\x00\x00\x01", 4); ptr += 4;	// beat -> for now, 1 (first beat)
			// Ticks
			*ptr++ = (byte)((jump_offset >> 12) & 0x0F);
			*ptr++ = (byte)((jump_offset >> 8) & 0x0F);
			*ptr++ = (byte)((jump_offset >> 4) & 0x0F);
			*ptr++ = (byte)(jump_offset & 0x0F);
			memcpy(ptr, "\x00\xf7", 2); ptr += 2;	// sysex end marker
		}
	} else {

		/* This is a sfx resource.  First parse it quickly to find the parallel
		 * tracks.
		 */
		ptr = writeMIDIHeader(ptr, "ASFX", ppqn, total_size);

		byte current_instr[3][14];
		int  current_note[3];
		int track_time[3];
		byte *track_data[3];

		int track_ctr = 0;
		byte chunk_type = 0;
		int delay, delay2, olddelay;

		// Write a tempo change Meta event
		// 473 / 4 Hz, convert to micro seconds.
		dw = 1000000 * ppqn * 4 / 473;
		memcpy(ptr, "\x00\xFF\x51\x03", 4); ptr += 4;
		*ptr++ = (byte)((dw >> 16) & 0xFF);
		*ptr++ = (byte)((dw >> 8) & 0xFF);
		*ptr++ = (byte)(dw & 0xFF);

		for (i = 0; i < 3; i++) {
			track_time[i] = -1;
			current_note[i] = -1;
		}
		while (size > 0) {
			assert(track_ctr < 3);
			track_data[track_ctr] = src_ptr;
			track_time[track_ctr] = 0;
			track_ctr++;
			while (size > 0) {
				chunk_type = *(src_ptr);
				if (chunk_type == 1) {
					src_ptr += 15;
					size -= 15;
				} else if (chunk_type == 2) {
					src_ptr += 11;
					size -= 11;
				} else if (chunk_type == 0x80) {
					src_ptr ++;
					size --;
				} else {
					break;
				}
			}
			if (chunk_type == 0xff)
				break;
			src_ptr++;
		}

		int curtime = 0;
		for (;;) {
			int mintime = -1;
			ch = -1;
			for (i = 0; i < 3; i++) {
				if (track_time[i] >= 0 && 
					(mintime == -1 || mintime > track_time[i])) {
					mintime = track_time[i];
					ch = i;
				}
			}
			if (mintime < 0)
				break;

			src_ptr = track_data[ch];
			chunk_type = *src_ptr;

			if (current_note[ch] >= 0) {
				delay = mintime - curtime;
				curtime = mintime;
				ptr = writeVLQ(ptr, delay);
				*ptr++ = 0x80 + ch; // key off channel;
				*ptr++ = current_note[ch];
				*ptr++ = 0;
				current_note[ch] = -1;
			}

			switch (chunk_type) {
			case 1:
				/* Instrument definition */
				memcpy(current_instr[ch], src_ptr + 1, 14);
				src_ptr += 15;
				break;

			case 2:
				/* tone/parammodulation */
				memcpy(ptr, ADLIB_INSTR_MIDI_HACK, 
					   sizeof(ADLIB_INSTR_MIDI_HACK));

				ptr[5]  += ch;
				ptr[28] += ch;
				ptr[92] += ch;

				/* flags_1 */
				ptr[30 + 0] = (current_instr[ch][3] >> 4) & 0xf;
				ptr[30 + 1] = current_instr[ch][3] & 0xf;

				/* oplvl_1 */
				ptr[30 + 2] = (current_instr[ch][4] >> 4) & 0xf;
				ptr[30 + 3] = current_instr[ch][4] & 0xf;

				/* atdec_1 */
				ptr[30 + 4] = ((~current_instr[ch][5]) >> 4) & 0xf;
				ptr[30 + 5] = (~current_instr[ch][5]) & 0xf;

				/* sustrel_1 */
				ptr[30 + 6] = ((~current_instr[ch][6]) >> 4) & 0xf;
				ptr[30 + 7] = (~current_instr[ch][6]) & 0xf;

				/* waveform_1 */
				ptr[30 + 8] = (current_instr[ch][7] >> 4) & 0xf;
				ptr[30 + 9] = current_instr[ch][7] & 0xf;

				/* flags_2 */
				ptr[30 + 10] = (current_instr[ch][8] >> 4) & 0xf;
				ptr[30 + 11] = current_instr[ch][8] & 0xf;

				/* oplvl_2 */
				ptr[30 + 12] = ((current_instr[ch][9]) >> 4) & 0xf;
				ptr[30 + 13] = (current_instr[ch][9]) & 0xf;

				/* atdec_2 */
				ptr[30 + 14] = ((~current_instr[ch][10]) >> 4) & 0xf;
				ptr[30 + 15] = (~current_instr[ch][10]) & 0xf;

				/* sustrel_2 */
				ptr[30 + 16] = ((~current_instr[ch][11]) >> 4) & 0xf;
				ptr[30 + 17] = (~current_instr[ch][11]) & 0xf;

				/* waveform_2 */
				ptr[30 + 18] = (current_instr[ch][12] >> 4) & 0xf;
				ptr[30 + 19] = current_instr[ch][12] & 0xf;

				/* feedback */
				ptr[30 + 20] = (current_instr[ch][2] >> 4) & 0xf;
				ptr[30 + 21] = current_instr[ch][2] & 0xf;

				delay = mintime - curtime;
				curtime = mintime;

				{
					delay = convert_extraflags(ptr + 30 + 22, src_ptr + 1);
					delay2 = convert_extraflags(ptr + 30 + 40, src_ptr + 6);
					debugC(DEBUG_SOUND, "delays: %d / %d", delay, delay2);
					if (delay2 >= 0 && delay2 < delay)
						delay = delay2;
					if (delay == -1)
						delay = 0;
				}

				/* duration */
				ptr[30 + 58] = 0; // ((delay * 17 / 63) >> 4) & 0xf;
				ptr[30 + 59] = 0; // (delay * 17 / 63) & 0xf;

				ptr += sizeof(ADLIB_INSTR_MIDI_HACK);

				olddelay = mintime - curtime;
				curtime = mintime;
				ptr = writeVLQ(ptr, olddelay);

				{
					int freq = ((current_instr[ch][1] & 3) << 8)
						| current_instr[ch][0];
					if (!freq)
						freq = 0x80;
					freq <<= ((current_instr[ch][1] >> 2) & 7) + 1;
					int note = -11;
					while (freq >= 0x100) {
						note += 12;
						freq >>= 1;
					}
					debugC(DEBUG_SOUND, "Freq: %d (%x) Note: %d", freq, freq, note);
					if (freq < 0x80)
						note = 0;
					else
						note += freq2note[freq - 0x80];
	
					debugC(DEBUG_SOUND, "Note: %d", note);
					if (note <= 0)
						note = 1;
					else if (note > 127)
						note = 127;

					// Insert a note on event 
					*ptr++ = 0x90 + ch; // key on channel
					*ptr++ = note;
					*ptr++ = 63;
					current_note[ch] = note;
					track_time[ch] = curtime + delay;
				}
				src_ptr += 11;
				break;

			case 0x80:
				track_time[ch] = -1;
				src_ptr ++;
				break;

			default:
				track_time[ch] = -1;
			}
			track_data[ch] = src_ptr;
		}
	}

	// Insert end of song sysex
	memcpy(ptr, "\x00\xff\x2f\x00\x00", 5); ptr += 5;
}


int ScummEngine::getResourceRoomNr(int type, int idx) {
	if (type == rtRoom)
		return idx;
	return res.roomno[type][idx];
}

int ScummEngine::getResourceSize(int type, int idx) {
	byte *ptr = getResourceAddress(type, idx);
	MemBlkHeader *hdr = (MemBlkHeader *)(ptr - sizeof(MemBlkHeader));
	
	return hdr->size;
}

byte *ScummEngine::getResourceAddress(int type, int idx) {
	byte *ptr;

	// no nonsense for the mask buffers
	if (type == rtBuffer && idx == 9)
	{
		ptr = (byte *)res.address[type][idx];
		if (ptr)
			ptr += sizeof(MemBlkHeader);
		return ptr;
	}


	CHECK_HEAP
	if (!validateResource("getResourceAddress", type, idx))
		return NULL;

	if (!res.address[type]) {
		debugC(DEBUG_RESOURCE, "getResourceAddress(%s,%d), res.address[type] == NULL", resTypeFromId(type), idx);
		return NULL;
	}

	if (res.mode[type] && !res.address[type][idx]) {
		ensureResourceLoaded(type, idx);
	}

	if (!(ptr = (byte *)res.address[type][idx])) {
		debugC(DEBUG_RESOURCE, "getResourceAddress(%s,%d) == NULL", resTypeFromId(type), idx);
		return NULL;
	}

	setResourceCounter(type, idx, 1);

	debug(DEBUG_RESOURCE, "getResourceAddress(%s,%d) == %p", resTypeFromId(type), idx, ptr + sizeof(MemBlkHeader));
	return ptr + sizeof(MemBlkHeader);
}

byte *ScummEngine::getStringAddress(int i) {
	byte *b = getResourceAddress(rtString, i);
	if (!b)
		return NULL;

	if (_features & GF_NEW_OPCODES)
		return ((ArrayHeader *)b)->data;
	return b;
}

byte *ScummEngine::getStringAddressVar(int i) {
	byte *addr;

	addr = getResourceAddress(rtString, _scummVars[i]);
	if (addr == NULL)
		// as this is used for string mapping in the gui
		// it must be allowed to return NULL
		// error("NULL string var %d slot %d", i, _scummVars[i]);
		return NULL;

	if (_features & GF_NEW_OPCODES)
		return ((ArrayHeader *)addr)->data;

	return (addr);
}

void ScummEngine::setResourceCounter(int type, int idx, byte flag) {
	res.flags[type][idx] &= ~RF_USAGE;
	res.flags[type][idx] |= flag;
}

/* 2 bytes safety area to make "precaching" of bytes in the gdi drawer easier */
#define SAFETY_AREA 2

byte *ScummEngine::createResource(int type, int idx, uint32 size) {
	byte *ptr;

	CHECK_HEAP
	debug(1, "createResource(%s,%d,%d)", resTypeFromId(type), idx, size);

	if (!validateResource("allocating", type, idx))
		return NULL;
	nukeResource(type, idx);

	expireResources(size);

	CHECK_HEAP
	ptr = (byte *)calloc(size + sizeof(MemBlkHeader) + SAFETY_AREA, 1);
	if (ptr == NULL) {
		error("Out of memory while allocating %d", size);
	}

	_allocatedSize += size;

	res.address[type][idx] = ptr;
	((MemBlkHeader *)ptr)->size = size;
	setResourceCounter(type, idx, 1);
	return ptr + sizeof(MemBlkHeader);	/* skip header */
}

bool ScummEngine::validateResource(const char *str, int type, int idx) const {
	if (type < rtFirst || type > rtLast || (uint) idx >= (uint) res.num[type]) {
		warning("%s Illegal Glob type %s (%d) num %d", str, resTypeFromId(type), type, idx);
		return false;
	}
	return true;
}

void ScummEngine::nukeResource(int type, int idx) {
	byte *ptr;

	CHECK_HEAP
	if (!res.address[type])
		return;

	assert(idx >= 0 && idx < res.num[type]);

	if ((ptr = res.address[type][idx]) != NULL) {
		debugC(DEBUG_RESOURCE, "nukeResource(%s,%d)", resTypeFromId(type), idx);
		res.address[type][idx] = 0;
		res.flags[type][idx] = 0;
		_allocatedSize -= ((MemBlkHeader *)ptr)->size;
		free(ptr);
	}
}

const byte *ScummEngine::findResourceData(uint32 tag, const byte *ptr) {
	ptr = findResource(tag, ptr);
	if (ptr == NULL)
		return NULL;
	return ptr + _resourceHeaderSize;
}

int ScummEngine::getResourceDataSize(const byte *ptr) const {
	if (ptr == NULL)
		return 0;
	return READ_BE_UINT32(ptr - 4) - 8;
}

void ScummEngine::lock(int type, int i) {
	if (!validateResource("Locking", type, i))
		return;
	res.flags[type][i] |= RF_LOCK;
}

void ScummEngine::unlock(int type, int i) {
	if (!validateResource("Unlocking", type, i))
		return;
	res.flags[type][i] &= ~RF_LOCK;
}

bool ScummEngine::isResourceInUse(int type, int i) const {
	if (!validateResource("isResourceInUse", type, i))
		return false;
	switch (type) {
	case rtRoom:
		return _roomResource == (byte)i;
	case rtRoomScripts:
		return _roomResource == (byte)i;
	case rtScript:
		return isScriptInUse(i);
	case rtCostume:
		return isCostumeInUse(i);
	case rtSound:
		return _sound->isSoundInUse(i);
	default:
		return false;
	}
}

void ScummEngine::increaseResourceCounter() {
#if 0
	for (int16 i = rtFirst; i <= rtLast; i++)
	{
		byte* flags = res.flags[i];
		byte* end = flags + res.num[i];

		while((flags+8) < end)
		{
			byte flag, counter;
			flag = *flags; counter = (flag & RF_USAGE); if(counter && (counter != RF_USAGE_MAX)) { counter++; *flags = (flag & ~RF_USAGE) | counter; } flags++;
			flag = *flags; counter = (flag & RF_USAGE); if(counter && (counter != RF_USAGE_MAX)) { counter++; *flags = (flag & ~RF_USAGE) | counter; } flags++;
			flag = *flags; counter = (flag & RF_USAGE); if(counter && (counter != RF_USAGE_MAX)) { counter++; *flags = (flag & ~RF_USAGE) | counter; } flags++;
			flag = *flags; counter = (flag & RF_USAGE); if(counter && (counter != RF_USAGE_MAX)) { counter++; *flags = (flag & ~RF_USAGE) | counter; } flags++;
			flag = *flags; counter = (flag & RF_USAGE); if(counter && (counter != RF_USAGE_MAX)) { counter++; *flags = (flag & ~RF_USAGE) | counter; } flags++;
			flag = *flags; counter = (flag & RF_USAGE); if(counter && (counter != RF_USAGE_MAX)) { counter++; *flags = (flag & ~RF_USAGE) | counter; } flags++;
			flag = *flags; counter = (flag & RF_USAGE); if(counter && (counter != RF_USAGE_MAX)) { counter++; *flags = (flag & ~RF_USAGE) | counter; } flags++;
			flag = *flags; counter = (flag & RF_USAGE); if(counter && (counter != RF_USAGE_MAX)) { counter++; *flags = (flag & ~RF_USAGE) | counter; } flags++;
		}
		while((flags+4) < end)
		{
			byte flag, counter;
			flag = *flags; counter = (flag & RF_USAGE); if(counter && (counter != RF_USAGE_MAX)) { counter++; *flags = (flag & ~RF_USAGE) | counter; } flags++;
			flag = *flags; counter = (flag & RF_USAGE); if(counter && (counter != RF_USAGE_MAX)) { counter++; *flags = (flag & ~RF_USAGE) | counter; } flags++;
			flag = *flags; counter = (flag & RF_USAGE); if(counter && (counter != RF_USAGE_MAX)) { counter++; *flags = (flag & ~RF_USAGE) | counter; } flags++;
			flag = *flags; counter = (flag & RF_USAGE); if(counter && (counter != RF_USAGE_MAX)) { counter++; *flags = (flag & ~RF_USAGE) | counter; } flags++;
		}
		while(flags < end)
		{
			byte flag, counter;
			flag = *flags; counter = (flag & RF_USAGE); if(counter && (counter != RF_USAGE_MAX)) { counter++; *flags = (flag & ~RF_USAGE) | counter; } flags++;
		}
	}
#else
	int i, j;
	byte counter;

	for (i = rtFirst; i <= rtLast; i++) {
		for (j = res.num[i]; --j >= 0;) {
			counter = res.flags[i][j] & RF_USAGE;
			if (counter && counter < RF_USAGE_MAX) {
				setResourceCounter(i, j, counter + 1);
			}
		}
	}
#endif
}

void ScummEngine::expireResources(uint32 size) {
	int i, j;
	byte flag;
	byte best_counter;
	int best_type, best_res = 0;
	uint32 oldAllocatedSize;

	if (_expire_counter != 0xFF) {
		_expire_counter = 0xFF;
		increaseResourceCounter();
	}

	if (size + _allocatedSize < _maxHeapThreshold)
		return;

	oldAllocatedSize = _allocatedSize;

	do {
		best_type = 0;
		best_counter = 2;

		for (i = rtFirst; i <= rtLast; i++)
			if (res.mode[i]) {
				for (j = res.num[i]; --j >= 0;) {
					flag = res.flags[i][j];
					if (!(flag & RF_LOCK) && flag >= best_counter && res.address[i][j] && !isResourceInUse(i, j)) {
						best_counter = flag;
						best_type = i;
						best_res = j;
					}
				}
			}

		if (!best_type)
			break;
		nukeResource(best_type, best_res);
	} while (size + _allocatedSize > _minHeapThreshold);

	increaseResourceCounter();

	debugC(DEBUG_RESOURCE, "Expired resources, mem %d -> %d", oldAllocatedSize, _allocatedSize);
}

void ScummEngine::freeResources() {
	int i, j;
	for (i = rtFirst; i <= rtLast; i++) {
		for (j = res.num[i]; --j >= 0;) {
			if (isResourceLoaded(i, j))
				nukeResource(i, j);
		}
		free(res.address[i]);
		free(res.flags[i]);
		free(res.roomno[i]);
		free(res.roomoffs[i]);
	}
}

void ScummEngine::loadPtrToResource(int type, int resindex, const byte *source) {
	byte *alloced;
	int i, len;

	nukeResource(type, resindex);

	len = resStrLen(source) + 1;

	if (len <= 0)
		return;

	alloced = createResource(type, resindex, len);

	if (!source) {
		alloced[0] = fetchScriptByte();
		for (i = 1; i < len; i++)
			alloced[i] = *_scriptPointer++;
	} else {
		for (i = 0; i < len; i++)
			alloced[i] = source[i];
	}
}

bool ScummEngine::isResourceLoaded(int type, int idx) const {
	if (!validateResource("isLoaded", type, idx))
		return false;
	return res.address[type][idx] != NULL;
}

void ScummEngine::resourceStats() {
	int i, j;
	uint32 lockedSize = 0, lockedNum = 0;
	byte flag;

	for (i = rtFirst; i <= rtLast; i++)
		for (j = res.num[i]; --j >= 0;) {
			flag = res.flags[i][j];
			if (flag & RF_LOCK && res.address[i][j]) {
				lockedSize += ((MemBlkHeader *)res.address[i][j])->size;
				lockedNum++;
			}
		}

	debug(1, "Total allocated size=%d, locked=%d(%d)", _allocatedSize, lockedSize, lockedNum);
}

void ScummEngine::readMAXS() {
	if (_version == 6) {
		_numVariables = _fileHandle.readUint16LE();
		_fileHandle.readUint16LE();                      // 16 in Sam/DOTT
		_numBitVariables = _fileHandle.readUint16LE();
		_numLocalObjects = _fileHandle.readUint16LE();
		_numArray = _fileHandle.readUint16LE();
		_fileHandle.readUint16LE();                      // 0 in Sam/DOTT
		_numVerbs = _fileHandle.readUint16LE();
		_numFlObject = _fileHandle.readUint16LE();
		_numInventory = _fileHandle.readUint16LE();
		_numRooms = _fileHandle.readUint16LE();
		_numScripts = _fileHandle.readUint16LE();
		_numSounds = _fileHandle.readUint16LE();
		_numCharsets = _fileHandle.readUint16LE();
		_numCostumes = _fileHandle.readUint16LE();
		_numGlobalObjects = _fileHandle.readUint16LE();
		_numNewNames = 50;
		_objectRoomTable = NULL;
		_numGlobalScripts = 200;

	} else {
		_numVariables = _fileHandle.readUint16LE();      // 800
		_fileHandle.readUint16LE();                      // 16
		_numBitVariables = _fileHandle.readUint16LE();   // 2048
		_numLocalObjects = _fileHandle.readUint16LE();   // 200
		_numArray = 50;
		_numVerbs = 100;
		_numNewNames = 0;
		_objectRoomTable = NULL;

		_fileHandle.readUint16LE();                      // 50
		_numCharsets = _fileHandle.readUint16LE();       // 9
		_fileHandle.readUint16LE();                      // 100
		_fileHandle.readUint16LE();                      // 50
		_numInventory = _fileHandle.readUint16LE();      // 80
		_numGlobalScripts = 200;
		_numFlObject = 50;
	}

	allocateArrays();
	_dynamicRoomOffsets = true;
}

void ScummEngine::allocateArrays() {
	// Note: Buffers are now allocated in scummMain to allow for
	//     early GUI init.

	_objectOwnerTable = (byte *)calloc(_numGlobalObjects, 1);
	_objectStateTable = (byte *)calloc(_numGlobalObjects, 1);
	_classData = (uint32 *)calloc(_numGlobalObjects, sizeof(uint32));
	_newNames = (uint16 *)calloc(_numNewNames, sizeof(uint16));

	_inventory = (uint16 *)calloc(_numInventory, sizeof(uint16));
	_verbs = (VerbSlot *)calloc(_numVerbs, sizeof(VerbSlot));
	_objs = (ObjectData *)calloc(_numLocalObjects, sizeof(ObjectData));
	debug(2, "Allocated %d space in numObjects", _numLocalObjects);
	_scummVars = (int32 *)calloc(_numVariables, sizeof(int32));
	_bitVars = (byte *)calloc(_numBitVariables >> 3, 1);

	allocResTypeData(rtCostume, MKID('COST'), _numCostumes, "costume", 1);
	allocResTypeData(rtRoom, MKID('ROOM'), _numRooms, "room", 1);
	allocResTypeData(rtRoomScripts, MKID('RMSC'), _numRooms, "room script", 1);
	allocResTypeData(rtSound, MKID('SOUN'), _numSounds, "sound", 2);
	allocResTypeData(rtScript, MKID('SCRP'), _numScripts, "script", 1);
	allocResTypeData(rtCharset, MKID('CHAR'), _numCharsets, "charset", 1);
	allocResTypeData(rtObjectName, MKID('NONE'), _numNewNames, "new name", 0);
	allocResTypeData(rtInventory, MKID('NONE'), _numInventory, "inventory", 0);
	allocResTypeData(rtTemp, MKID('NONE'), 10, "temp", 0);
	allocResTypeData(rtScaleTable, MKID('NONE'), 5, "scale table", 0);
	allocResTypeData(rtActorName, MKID('NONE'), _numActors, "actor name", 0);
	allocResTypeData(rtVerb, MKID('NONE'), _numVerbs, "verb", 0);
	allocResTypeData(rtString, MKID('NONE'), _numArray, "array", 0);
	allocResTypeData(rtFlObject, MKID('NONE'), _numFlObject, "flobject", 0);
	allocResTypeData(rtMatrix, MKID('NONE'), 10, "boxes", 0);
}


bool ScummEngine::isGlobInMemory(int type, int idx) const{
	if (!validateResource("isGlobInMemory", type, idx))
		return false;

	return res.address[type][idx] != NULL;
}


ResourceIterator::ResourceIterator(const byte *searchin)
	: _ptr(searchin) {
	assert(searchin);
	_size = READ_BE_UINT32(searchin + 4);
	_pos = 8;
	_ptr = searchin + 8;
}

const byte *ResourceIterator::findNext(uint32 tag) {
	uint32 size = 0;
	const byte *result = 0;
	
	do {
		if (_pos >= _size)
			return 0;

		result = _ptr;
		size = READ_BE_UINT32(result + 4);
		if ((int32)size <= 0)
			return 0;	// Avoid endless loop
		
		_pos += size;
		_ptr += size;
	} while (READ_UINT32(result) != tag);

	return result;
}

const byte *findResource(uint32 tag, const byte *searchin) {
	uint32 curpos, totalsize, size;

	assert(searchin);

	searchin += 4;
	totalsize = READ_BE_UINT32(searchin);
	curpos = 8;
	searchin += 4;

	while (curpos < totalsize) {
		if (READ_UINT32(searchin) == tag)
			return searchin;

		size = READ_BE_UINT32(searchin + 4);
		if ((int32)size <= 0) {
			error("(%s) Not found in %d... illegal block len %d", tag2str(tag), 0, size);
			return NULL;
		}

		curpos += size;
		searchin += size;
	}

	return NULL;
}






} // End of namespace Scumm

