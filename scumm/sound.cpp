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
 * $Header: /cvsroot/scummvm/scummvm/scumm/sound.cpp,v 1.320 2004/02/14 04:12:22 kirben Exp $
 *
 */

#include "stdafx.h"
#include "scumm/actor.h"
#include "scumm/imuse.h"
#include "scumm/scumm.h"
#include "scumm/sound.h"

#include "common/config-manager.h"
#include "common/timer.h"
#include "common/util.h"

#include "sound/mididrv.h"
#include "sound/mixer.h"
#include "sound/voc.h"


namespace Scumm {

struct MP3OffsetTable {					/* Compressed Sound (.SO3) */
	int org_offset;
	int new_offset;
	int num_tags;
	int compressed_size;
};


Sound::Sound(ScummEngine *parent) {
	memset(this,0,sizeof(Sound));	// palmos
	
	_vm = parent;

	_sfxFile = 0;
}

Sound::~Sound() {
	delete _sfxFile;
}

void Sound::addSoundToQueue(int sound) {
	_vm->VAR(_vm->VAR_LAST_SOUND) = sound;
	_vm->ensureResourceLoaded(rtSound, sound);
	addSoundToQueue2(sound);
}

void Sound::addSoundToQueue2(int sound) {
	assert(_soundQue2Pos < ARRAYSIZE(_soundQue2));
	_soundQue2[_soundQue2Pos++] = sound;
}

void Sound::processSoundQues() {
	int i = 0, d, num;
	int data[16];

	processSfxQueues();

	while (_soundQue2Pos) {
		d = _soundQue2[--_soundQue2Pos];
		if (d)
			playSound(d);
	}

	while (i < _soundQuePos) {
		num = _soundQue[i++];
		if (i + num > _soundQuePos) {
			warning("processSoundQues: invalid num value");
			break;
		}
		memset(data, 0, sizeof(data));
		if (num > 0) {
			for (int j = 0; j < num; j++)
				data[j] = _soundQue[i + j];
			i += num;

			debugC(DEBUG_IMUSE, "processSoundQues(%d,%d,%d,%d,%d,%d,%d,%d,%d)",
						data[0] >> 8, data[0] & 0xFF,
						data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

			if (_vm->_imuse) {
				_vm->VAR(_vm->VAR_SOUNDRESULT) = (short)_vm->_imuse->doCommand (num, data);
			}
		}
	}
	_soundQuePos = 0;
}

void Sound::playSound(int soundID) {
	byte *ptr;
//	char *sound;
//	int size;
//	int rate;
//	byte flags = SoundMixer::FLAG_UNSIGNED | SoundMixer::FLAG_AUTOFREE;
	
	debug(1, "playSound #%d (room %d)", soundID, 
		_vm->getResourceRoomNr(rtSound, soundID));

	ptr = _vm->getResourceAddress(rtSound, soundID);
	if (!ptr) {
		return;
	}

	if (_vm->_musicEngine) {
		_vm->_musicEngine->startSound(soundID);
	}
}

void Sound::processSfxQueues() {

	if (_talk_sound_mode != 0) {
		if (_talk_sound_mode & 1)
			startTalkSound(_talk_sound_a1, _talk_sound_b1, 1);
		if (_talk_sound_mode & 2)
			startTalkSound(_talk_sound_a2, _talk_sound_b2, 2, &_talkChannelHandle);
		_talk_sound_mode = 0;
	}

	const int act = _vm->talkingActor();
	if ((_sfxMode & 2) && act != 0) {
		Actor *a;
		bool b, finished;

		finished = !_talkChannelHandle.isActive();

		if ((uint) act < 0x80 && !_vm->_string[0].no_talk_anim && (finished || !_endOfMouthSync)) {
			a = _vm->derefActor(act, "processSfxQueues");
			if (a->isInCurrentRoom()) {
				b = finished || isMouthSyncOff(_curSoundPos);
				if (_mouthSyncMode != b) {
					_mouthSyncMode = b;
					if (_talk_sound_frame != -1) {
						a->startAnimActor(_talk_sound_frame);
						_talk_sound_frame = -1;
					} else
						a->startAnimActor(b ? a->talkStopFrame : a->talkStartFrame);
				}
			}
		}

		if (finished && _vm->_talkDelay == 0) {
			_vm->stopTalk();
		}
	}

	if (_sfxMode & 1) {
		if (isSfxFinished()) {
			_sfxMode &= ~1;
		}
	}
}
/*
static int compareMP3OffsetTable(const void *a, const void *b) {
	return ((const MP3OffsetTable *)a)->org_offset - ((const MP3OffsetTable *)b)->org_offset;
}
*/
void Sound::startTalkSound(uint32 offset, uint32 b, int mode, PlayingSoundHandle *handle) {
	int num = 0, i;
	int size = 0;
	int id = -1;

	if (!_sfxFile->isOpen()) {
		warning("startTalkSound: SFX file is not open");
		return;
	}

	if (mode == 1 && (_vm->_gameId == GID_TENTACLE
		|| (_vm->_gameId == GID_SAMNMAX && !_vm->isScriptRunning(99)))) {
		id = 777777;
		_vm->_mixer->stopID(id);
	}

	if (b > 8) {
		num = (b - 8) >> 1;
	}

	offset += 8;
	size = -1;

	_sfxFile->seek(offset, SEEK_SET);

	assert(num + 1 < (int)ARRAYSIZE(_mouthSyncTimes));
	for (i = 0; i < num; i++)
		_mouthSyncTimes[i] = _sfxFile->readUint16BE();

	_mouthSyncTimes[i] = 0xFFFF;
	_sfxMode |= mode;
	_curSoundPos = 0;
	_mouthSyncMode = true;

	if (!_soundsPaused && _vm->_mixer->isReady())
		startSfxSound(_sfxFile, size, handle, id);
}

void Sound::stopTalkSound() {
	if (_sfxMode & 2) {
		_vm->_mixer->stopHandle(_talkChannelHandle);
		_sfxMode &= ~2;
	}
}

bool Sound::isMouthSyncOff(uint pos) {
	uint j;
	bool val = true;
	uint16 *ms = _mouthSyncTimes;

	_endOfMouthSync = false;
	do {
		val = !val;
		j = *ms++;
		if (j == 0xFFFF) {
			_endOfMouthSync = true;
			break;
		}
	} while (pos > j);
	return val;
}


int Sound::isSoundRunning(int sound) const {
	if (isSoundInQueue(sound))
		return 1;

	if (!_vm->isResourceLoaded(rtSound, sound))
		return 0;

	if (_vm->_musicEngine)
		return _vm->_musicEngine->getSoundStatus(sound);

	return 0;
}

/**
 * Check whether the sound resource with the specified ID is still
 * used. This is invoked by ScummEngine::isResourceInUse, to determine
 * which resources can be expired from memory.
 * Technically, this works very similar to isSoundRunning, however it
 * calls IMuse::get_sound_active() instead of IMuse::getSoundStatus().
 * The difference between those two is in how they treat sounds which
 * are being faded out: get_sound_active() returns true even when the
 * sound is being faded out, while getSoundStatus() returns false in
 * that case.
 */
bool Sound::isSoundInUse(int sound) const {

	if (isSoundInQueue(sound))
		return true;

	if (!_vm->isResourceLoaded(rtSound, sound))
		return false;

	if (_vm->_imuse)
		return _vm->_imuse->get_sound_active(sound);

	return false;
}

bool Sound::isSoundInQueue(int sound) const {
	int i, num;

	i = _soundQue2Pos;
	while (i--) {
		if (_soundQue2[i] == sound)
			return true;
	}

	i = 0;
	while (i < _soundQuePos) {
		num = _soundQue[i++];

		if (num > 0) {
			if (_soundQue[i + 0] == 0x10F && _soundQue[i + 1] == 8 && _soundQue[i + 2] == sound)
				return true;
			i += num;
		}
	}
	return false;
}

void Sound::stopSound(int a) {
	int i;

	if (_vm->_musicEngine)
		_vm->_musicEngine->stopSound(a);

	for (i = 0; i < ARRAYSIZE(_soundQue2); i++)
		if (_soundQue2[i] == a)
			_soundQue2[i] = 0;
}

void Sound::stopAllSounds() {

	// Clear the (secondary) sound queue
	_soundQue2Pos = 0;
	memset(_soundQue2, 0, sizeof(_soundQue2));

	if (_vm->_musicEngine) {
		_vm->_musicEngine->stopAllSounds();
	}
	if (_vm->_imuse) {
		// FIXME: Maybe we could merge this call to clear_queue()
		// into IMuse::stopAllSounds() ?
		_vm->_imuse->clear_queue();
	}

	// Stop all SFX
	_vm->_mixer->stopAll();
}

void Sound::soundKludge(int *list, int num) {
	int i;

	if (list[0] == -1) {
		processSoundQues();
	} else {
		_soundQue[_soundQuePos++] = num;
		
		for (i = 0; i < num; i++) {
			_soundQue[_soundQuePos++] = list[i];
		}
	}
}

void Sound::talkSound(uint32 a, uint32 b, int mode, int frame) {
	if (mode == 1) {
		_talk_sound_a1 = a;
		_talk_sound_b1 = b;
	} else {
		_talk_sound_a2 = a;
		_talk_sound_b2 = b;
	}

	_talk_sound_frame = frame;
	_talk_sound_mode |= mode;
}

/* The sound code currently only supports General Midi.
 * General Midi is used in Day Of The Tentacle.
 * Roland music is also playable, but doesn't sound well.
 * A mapping between roland instruments and GM instruments
 * is needed.
 */

void Sound::setupSound() {
	delete _sfxFile;
	_sfxFile = openSfxFile();
}

void Sound::pauseSounds(bool pause) {
	if (_vm->_imuse)
		_vm->_imuse->pause(pause);

	// Don't pause sounds if the game isn't active
	// FIXME - this is quite a nasty hack, replace with something cleaner, and w/o
	// having to access member vars directly!
	if (!_vm->_roomResource)
		return;

	_soundsPaused = pause;

	_vm->_mixer->pauseAll(pause);
/*
	if ((_vm->_features & GF_AUDIOTRACKS) && _vm->VAR(_vm->VAR_MUSIC_TIMER) > 0) {
		if (pause)
			stopCDTimer();
		else
			startCDTimer();
	}
*/		
}

void Sound::startSfxSound(File *file, int file_size, PlayingSoundHandle *handle, int id) {
#ifndef DISABLE_VOC
	AudioStream *input = 0;
	input = makeVOCStream(_sfxFile);
	if (!input) {
		warning("startSfxSound failed to load sound");
		return;
	}

	_vm->_mixer->playInputStream(handle, input, false, 256, 0, id);
#endif
}

File *Sound::openSfxFile() {
	File *file = new File();
#ifndef DISABLE_VOC
	char buf[256];
	sprintf(buf, "%s.sou", _vm->getGameName());
	if (!file->open(buf, _vm->getGameDataPath())) {
		file->open("monster.sou", _vm->getGameDataPath());
	}

	if (!file->isOpen()) {
		sprintf(buf, "%s.tlk", _vm->getGameName());
		file->open(buf, _vm->getGameDataPath(), File::kFileReadMode, 0x69);
	}
#endif
	return file;
}

bool Sound::isSfxFinished() const {
	return !_vm->_mixer->hasActiveSFXChannel();
}



} // End of namespace Scumm
