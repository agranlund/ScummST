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
 * $Header: /cvsroot/scummvm/scummvm/sound/mixer.cpp,v 1.157 2004/02/12 16:25:28 eriktorbjorn Exp $
 *
 */

#include "stdafx.h"
#include "common/file.h"
#include "common/util.h"

#include "sound/mixer.h"
#include "sound/rate.h"
#include "sound/audiostream.h"


#pragma mark -
#pragma mark --- Channel classes ---
#pragma mark -


/**
 * Channels used by the sound mixer.
 */
class Channel {
private:
	SoundMixer *_mixer;
	PlayingSoundHandle *_handle;
	bool _autofreeStream;
	const bool _isMusic;
	uint16 _volume;
	int8 _balance;
	bool _paused;
	bool _garbage;
	int _id;
	uint32 _samplesConsumed;
	uint32 _samplesDecoded;
	uint32 _mixerTimeStamp;

protected:
	RateConverter *_converter;
	AudioStream *_input;

public:

	Channel(SoundMixer *mixer, PlayingSoundHandle *handle, bool isMusic, int id = -1);
	Channel(SoundMixer *mixer, PlayingSoundHandle *handle, AudioStream *input, bool autofreeStream, bool isMusic, bool reverseStereo = false, int id = -1);
	virtual ~Channel();
	void makeGarbage() { _garbage = true; }

	void mix(uint8 *data, uint len);

	bool isFinished() const {
		return _input->endOfStream();
	}
	bool isMusicChannel() const {
		return _isMusic;
	}
	void pause(bool paused) {
		_paused = paused;
	}
	bool isPaused() {
		return _paused;
	}
	void setVolume(const uint16 volume) {
		_volume = volume;
	}
	void setBalance(const int8 balance) {
		_balance = balance;
	}
	int getId() const {
		return _id;
	}
	bool isGarbage() {
		return _garbage;
	}
	uint32 getElapsedTime();
};


#pragma mark -
#pragma mark --- SoundMixer ---
#pragma mark -


SoundMixer::SoundMixer() {
	_syst = OSystem::instance();
	_mutex = _syst->create_mutex();

	_premixParam = 0;
	_premixProc = 0;
	int i = 0;

	_outputRate = (uint) _syst->property(OSystem::PROP_GET_SAMPLE_RATE, 0);

	if (_outputRate == 0)
		error("OSystem returned invalid sample rate");

	_globalVolume = 0;
	_musicVolume = 0;

	_paused = false;
	
	for (i = 0; i != NUM_CHANNELS; i++)
		_channels[i] = 0;

	_mixerReady = _syst->set_sound_proc(mixCallback, this, OSystem::SOUND_8BIT);
}

SoundMixer::~SoundMixer() {
	_syst->clear_sound_proc();
	stopAll();
	_syst->delete_mutex(_mutex);
}

void SoundMixer::setupPremix(PremixProc *proc, void *param) {
	Common::StackLock lock(_mutex);
	_premixParam = param;
	_premixProc = proc;
}


void SoundMixer::collectGarbage()
{
	Common::StackLock lock(_mutex);
	for (int i = 0; i != NUM_CHANNELS; i++) {
		if (_channels[i] && _channels[i]->isGarbage()) {
			delete _channels[i];
			_channels[i] = 0;
		}
	}
}

void SoundMixer::insertChannel(PlayingSoundHandle *handle, Channel *chan) {

	int index = -1;
	for (int i = 0; i != NUM_CHANNELS; i++) {
		if (_channels[i] == 0) {
			index = i;
			break;
		}
	}

	if(index == -1) {
		warning("SoundMixer::out of mixer slots");
		delete chan;
		return;
	}

	_channels[index] = chan;
	if (handle)
		handle->setIndex(index);
}

void SoundMixer::playInputStream(PlayingSoundHandle *handle, AudioStream *input, bool isMusic, uint16 volume, int8 balance, int id, bool autofreeStream) {
	Common::StackLock lock(_mutex);

	if (input == 0) {
		warning("input stream is 0");
		return;
	}

	collectGarbage();
	
	// Prevent duplicate sounds
	if (id != -1) {
		for (int i = 0; i != NUM_CHANNELS; i++)
			if (_channels[i] != 0 && _channels[i]->getId() == id) {
				if (autofreeStream)
					delete input;
				return;
			}
	}

	// Create the channel
	Channel *chan = new Channel(this, handle, input, autofreeStream, isMusic, false, id);
	chan->setVolume(volume);
	chan->setBalance(balance);
	insertChannel(handle, chan);
}

void SoundMixer::mix(uint8 *buf, uint len) {
	Common::StackLock lock(_mutex);

	//  zero the buf
	memset(buf, 0, len * sizeof(uint8));

	if (!_paused) {
		if (_premixProc)
			_premixProc(_premixParam, buf, len);

		// now mix all channels
		for (int i = 0; i != NUM_CHANNELS; i++) {
			if (_channels[i] && !_channels[i]->isGarbage()) {
				if (_channels[i]->isFinished()) {
					_channels[i]->makeGarbage();
				} else if (!_channels[i]->isPaused())
					_channels[i]->mix(buf, len);
			}
		}
	}
}

void SoundMixer::mixCallback(void *s, byte *samples, int len) {
	assert(s);
	assert(samples);
	// Len is the number of bytes in the buffer; (mono 8 bit).
	((SoundMixer *)s)->mix((uint8 *)samples, len);
}

void SoundMixer::stopAll() {
	Common::StackLock lock(_mutex);
	for (int i = 0; i != NUM_CHANNELS; i++)
		if (_channels[i] != 0) {
			delete _channels[i];
			_channels[i] = 0;
		}
}

void SoundMixer::stopID(int id) {
	Common::StackLock lock(_mutex);
	for (int i = 0; i != NUM_CHANNELS; i++) {
		if (_channels[i] != 0 && _channels[i]->getId() == id) {
			delete _channels[i];
			_channels[i] = 0;
		}
	}
}

void SoundMixer::stopHandle(PlayingSoundHandle handle) {
	Common::StackLock lock(_mutex);

	// Simply ignore stop requests for handles of sounds that already terminated
	if (!handle.isActive())
		return;

	int index = handle.getIndex();

	if ((index < 0) || (index >= NUM_CHANNELS)) {
		warning("soundMixer::stopHandle has invalid index %d", index);
		return;
	}

	if (_channels[index]) {
		delete _channels[index];
		_channels[index] = 0;
	}
}

void SoundMixer::pauseAll(bool paused) {
	_paused = paused;
}

bool SoundMixer::hasActiveSFXChannel() {
	// FIXME/TODO: We need to distinguish between SFX and music channels
	// (and maybe also voice) here to work properly in iMuseDigital
	// games. In the past that was achieve using the _beginSlots hack.
	// Since we don't have that anymore, it's not that simple anymore.
	Common::StackLock lock(_mutex);
	for (int i = 0; i != NUM_CHANNELS; i++)
		if (_channels[i] && !_channels[i]->isGarbage() && !_channels[i]->isMusicChannel())
			return true;
	return false;
}

void SoundMixer::setVolume(int volume) {
	// Check range
	if (volume > 256)
		volume = 256;
	else if (volume < 0)
		volume = 0;

	_globalVolume = volume;
}

void SoundMixer::setMusicVolume(int volume) {
	// Check range
	if (volume > 256)
		volume = 256;
	else if (volume < 0)
		volume = 0;

	_musicVolume = volume;
}


#pragma mark -
#pragma mark --- Channel implementations ---
#pragma mark -


Channel::Channel(SoundMixer *mixer, PlayingSoundHandle *handle, bool isMusic, int id)
	: _mixer(mixer), _handle(handle), _autofreeStream(true), _isMusic(isMusic),
	  _volume(256), _balance(0), _paused(false), _garbage(false), _id(id), _samplesConsumed(0),
	  _samplesDecoded(0), _mixerTimeStamp(0), _converter(0), _input(0) {
	assert(mixer);
}

Channel::Channel(SoundMixer *mixer, PlayingSoundHandle *handle, AudioStream *input,
				bool autofreeStream, bool isMusic, bool reverseStereo, int id)
	: _mixer(mixer), _handle(handle), _autofreeStream(autofreeStream), _isMusic(isMusic),
	  _volume(256), _balance(0), _paused(false), _garbage(false), _id(id), _samplesConsumed(0),
	  _samplesDecoded(0), _mixerTimeStamp(0), _converter(0), _input(input) {
	assert(mixer);
	assert(input);

	// Get a rate converter instance
	_converter = makeRateConverter(_input->getRate(), mixer->getOutputRate());
}

Channel::~Channel() {
	delete _converter;
	if (_autofreeStream)
		delete _input;
	if (_handle)
		_handle->resetIndex();
}

/* len indicates the number of sample *pairs*. So a value of
   10 means that the buffer contains 10 sample, each
   8 bits, for a total of 10 bytes.
 */
void Channel::mix(uint8 *data, uint len) {
	assert(_input);

	if (_input->endOfData()) {
		// TODO: call drain method
	} else {
		assert(_converter);

		// vol_l/r becomes 8x8 fixed point
		uint32 vol = ((isMusicChannel() ? _mixer->getMusicVolume() : _mixer->getVolume()) * _volume);
		st_volume_t vol_l, vol_r;
		vol_l = vol_r = (vol >> 8);

		_samplesConsumed = _samplesDecoded;
		_mixerTimeStamp = g_system->get_msecs();

		_converter->flow(*_input, data, len, vol_l, vol_r);

		_samplesDecoded += len;
	}
}

uint32 Channel::getElapsedTime() {
	if (_mixerTimeStamp == 0)
		return 0;

	// Convert the number of samples into a time duration. To avoid
	// overflow, this has to be done in a somewhat non-obvious way.

	uint rate = _mixer->getOutputRate();

	uint32 seconds = _samplesConsumed / rate;
	uint32 milliseconds = (1000 * (_samplesConsumed % rate)) / rate;

	uint32 delta = g_system->get_msecs() - _mixerTimeStamp;

	// In theory it would seem like a good idea to limit the approximation
	// so that it never exceeds the theoretical upper bound set by
	// _samplesDecoded. Meanwhile, back in the real world, doing so makes
	// the Broken Sword cutscenes noticeably jerkier. I guess the mixer
	// isn't invoked at the regular intervals that I first imagined.

	// FIXME: This won't work very well if the sound is paused.
	return 1000 * seconds + milliseconds + delta;
}
