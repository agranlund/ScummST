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
 * $Header: /cvsroot/scummvm/scummvm/sound/midiparser.cpp,v 1.20 2004/01/06 12:45:33 fingolfin Exp $
 *
 */

#include "stdafx.h"
#include "midiparser.h"
#include "mididrv.h"
#include "common/util.h"



//////////////////////////////////////////////////
//
// MidiParser implementation
//
//////////////////////////////////////////////////

MidiParser::MidiParser() :
_hanging_notes_count (0),
_driver (0),
_timer_rate (0x4A0000),
_ppqn (96),
_tempo (500000),
_psec_per_tick (5208), // 500000 / 96
_autoLoop (false),
_smartJump (false),
_num_tracks (0),
_active_track (255),
_abort_parse (0) {
	memset (_active_notes, 0, sizeof(_active_notes));
}

void MidiParser::property (int prop, int value) {
	switch (prop) {
	case mpAutoLoop:
		_autoLoop = (value != 0);
	case mpSmartJump:
		_smartJump = (value != 0);
	}
}

void MidiParser::setTempo (uint32 tempo) {
	_tempo = tempo;
	if (_ppqn)
		_psec_per_tick = (tempo + (_ppqn >> 2)) / _ppqn;
}

// This is the conventional (i.e. SMF) variable length quantity
uint32 MidiParser::readVLQ (byte * &data) {
	byte str;
	uint32 value = 0;
	int i;

	for (i = 0; i < 4; ++i) {
		str = data[0];
		++data;
		value = (value << 7) | (str & 0x7F);
		if (!(str & 0x80))
			break;
	}
	return value;
}

void MidiParser::activeNote (byte channel, byte note, bool active) {
	if (note >= 128 || channel >= 16)
		return;

	if (active)
		_active_notes[note] |= (1 << channel);
	else
		_active_notes[note] &= ~(1 << channel);

	// See if there are hanging notes that we can cancel
	NoteTimer *ptr = _hanging_notes;
	int i;
	for (i = ARRAYSIZE(_hanging_notes); i; --i, ++ptr) {
		if (ptr->channel == channel && ptr->note == note && ptr->time_left) {
			ptr->time_left = 0;
			--_hanging_notes_count;
			break;
		}
	}
}

void MidiParser::hangingNote (byte channel, byte note, uint32 time_left, bool recycle) {
	NoteTimer *best = 0;
	NoteTimer *ptr = _hanging_notes;
	int i;

	if (_hanging_notes_count >= ARRAYSIZE(_hanging_notes)) {
		warning("WARNING! MidiParser::hangingNote(): Exceeded polyphony!");
		return;
	}

	for (i = ARRAYSIZE(_hanging_notes); i; --i, ++ptr) {
		if (ptr->channel == channel && ptr->note == note) {
			if (ptr->time_left && ptr->time_left < time_left && recycle)
				return;
			best = ptr;
			if (ptr->time_left) {
				if (recycle) _driver->send (0x80 | channel | note << 8);
				--_hanging_notes_count;
			}
			break;
		} else if (!best && ptr->time_left == 0) {
			best = ptr;
		}
	}

	// Occassionally we might get a zero or negative note
	// length, if the note should be turned on and off in
	// the same iteration. For now just set it to 1 and
	// we'll turn it off in the next cycle.
	if (!time_left || time_left & 0x80000000)
		time_left = 1;

	if (best) {
		best->channel = channel;
		best->note = note;
		best->time_left = time_left;
		++_hanging_notes_count;
	} else {
		// We checked this up top. We should never get here!
		warning("WARNING! MidiParser::hangingNote(): Internal error!");
	}
}

void MidiParser::onTimer() {
	uint32 end_time;
	uint32 event_time;

	if (!_position._play_pos || !_driver)
		return;

	_abort_parse = false;
	end_time = _position._play_time + _timer_rate;

	// Scan our hanging notes for any
	// that should be turned off.
	if (_hanging_notes_count) {
		NoteTimer *ptr = &_hanging_notes[0];
		int i;
		for (i = ARRAYSIZE(_hanging_notes); i; --i, ++ptr) {
			if (ptr->time_left) {
				if (ptr->time_left <= _timer_rate) {
					_driver->send (0x80 | ptr->channel | ptr->note << 8);
					ptr->time_left = 0;
					--_hanging_notes_count;
				} else {
					ptr->time_left -= _timer_rate;
				}
			}
		}
	}

	while (!_abort_parse) {
		EventInfo &info = _next_event;

		event_time = _position._last_event_time + info.delta * _psec_per_tick;
		if (event_time > end_time)
			break;

		// Process the next info.
		_position._last_event_tick += info.delta;
		if (info.event < 0x80) {
			warning("ERROR! Bad command or running status %02X", info.event);
			_position._play_pos = 0;
			return;
		}

		if (info.event == 0xF0) {
			// SysEx event
			_driver->sysEx (info.ext.data, (uint16) info.length);
		} else if (info.event == 0xFF) {
			// META event
			if (info.ext.type == 0x2F) {
				// End of Track must be processed by us,
				// as well as sending it to the output device.
				if (_autoLoop) {
					jumpToTick (0);
					parseNextEvent (_next_event);
				} else {
					allNotesOff();
					resetTracking();
					_driver->metaEvent (info.ext.type, info.ext.data, (uint16) info.length);
				}
				return;
			} else if (info.ext.type == 0x51) {
				if (info.length >= 3) {
					setTempo (info.ext.data[0] << 16 | info.ext.data[1] << 8 | info.ext.data[2]);
				}
			}
			_driver->metaEvent (info.ext.type, info.ext.data, (uint16) info.length);
		} else {
			if (info.command() == 0x8) {
				activeNote (info.channel(), info.basic.param1, false);
			} else if (info.command() == 0x9) {
				if (info.length > 0)
					hangingNote (info.channel(), info.basic.param1, info.length * _psec_per_tick - (end_time - event_time));
				else
					activeNote (info.channel(), info.basic.param1, true);
			}
			_driver->send (info.event | info.basic.param1 << 8 | info.basic.param2 << 16);
		}


		if (!_abort_parse) {
			_position._last_event_time = event_time;
			parseNextEvent (_next_event);
		}
	}

	if (!_abort_parse) {
		_position._play_time = end_time;
		_position._play_tick = (_position._play_time - _position._last_event_time) / _psec_per_tick + _position._last_event_tick;
	}
}

void MidiParser::allNotesOff() {
	if (!_driver)
		return;

	int i;

	// todo: would be good to remove this if "All Note Off" is supported for anything relevant to Atari
	// Turn off all active notes
	/*
	int j;
	for (i = 0; i < 128; ++i)
	{
		uint32 cmd = 0x80 | (i << 8);
		uint16 notes = _active_notes[i];
		if (notes & (1 <<  0)) _driver->send(cmd |  0);
		if (notes & (1 <<  1)) _driver->send(cmd |  1);
		if (notes & (1 <<  2)) _driver->send(cmd |  2);
		if (notes & (1 <<  3)) _driver->send(cmd |  3);
		if (notes & (1 <<  4)) _driver->send(cmd |  4);
		if (notes & (1 <<  5)) _driver->send(cmd |  5);
		if (notes & (1 <<  6)) _driver->send(cmd |  6);
		if (notes & (1 <<  7)) _driver->send(cmd |  7);
		if (notes & (1 <<  8)) _driver->send(cmd |  8);
		if (notes & (1 <<  9)) _driver->send(cmd |  9);
		if (notes & (1 << 10)) _driver->send(cmd | 10);
		if (notes & (1 << 11)) _driver->send(cmd | 11);
		if (notes & (1 << 12)) _driver->send(cmd | 12);
		if (notes & (1 << 13)) _driver->send(cmd | 13);
		if (notes & (1 << 14)) _driver->send(cmd | 14);
		if (notes & (1 << 15)) _driver->send(cmd | 15);
		_active_notes[i] = 0;
	}
	*/

	// To be sure, send an "All Note Off" event (but not all MIDI devices support this...)
	{
		_driver->send (0x007BB0 |  0);
		_driver->send (0x007BB0 |  1);
		_driver->send (0x007BB0 |  2);
		_driver->send (0x007BB0 |  3);
		_driver->send (0x007BB0 |  4);
		_driver->send (0x007BB0 |  5);
		_driver->send (0x007BB0 |  6);
		_driver->send (0x007BB0 |  7);
		_driver->send (0x007BB0 |  8);
		_driver->send (0x007BB0 |  9);
		_driver->send (0x007BB0 | 10);
		_driver->send (0x007BB0 | 11);
		_driver->send (0x007BB0 | 12);
		_driver->send (0x007BB0 | 13);
		_driver->send (0x007BB0 | 14);
		_driver->send (0x007BB0 | 15);
	}

	for (i = 0; i < ARRAYSIZE(_hanging_notes); ++i)
		_hanging_notes[i].time_left = 0;
	_hanging_notes_count = 0;
}

void MidiParser::resetTracking() {
	_position.clear();
}

bool MidiParser::setTrack (int track) {
	if (track < 0 || track >= _num_tracks)
		return false;
	else if (track == _active_track)
		return true;

	if (_smartJump)
		hangAllActiveNotes();
	else
		allNotesOff();

	resetTracking();
	memset (_active_notes, 0, sizeof(_active_notes));
	_active_track = track;
	_position._play_pos = _tracks[track];
	parseNextEvent (_next_event);
	return true;
}

void MidiParser::hangAllActiveNotes() {
	// Search for note off events until we have
	// accounted for every active note.
	uint16 temp_active [128];
	memcpy (temp_active, _active_notes, sizeof (temp_active));

	uint32 advance_tick = _position._last_event_tick;
	while (true) {
		int i, j;
		for (i = 0; i < 128; ++i)
			if (temp_active[i] != 0) break;
		if (i == 128) break;
		parseNextEvent (_next_event);
		advance_tick += _next_event.delta;
		if (_next_event.command() == 0x8) {
			if (temp_active[_next_event.basic.param1] & (1 << _next_event.channel())) {
				hangingNote (_next_event.channel(), _next_event.basic.param1, (advance_tick - _position._last_event_tick) * _psec_per_tick, false);
				temp_active[_next_event.basic.param1] &= ~ (1 << _next_event.channel());
			}
		} else if (_next_event.event == 0xFF && _next_event.ext.type == 0x2F) {
			// printf ("MidiParser::hangAllActiveNotes(): Hit End of Track with active notes left!\n");
			for (i = 0; i < 128; ++i) {
				for (j = 0; j < 16; ++j) {
					if (temp_active[i] & (1 << j)) {
						activeNote (j, i, false);
						_driver->send (0x80 | j | i << 8);
					}
				}
			}
			break;
		}
	}
}

bool MidiParser::jumpToTick (uint32 tick, bool fireEvents) {
	if (_active_track >= _num_tracks)
		return false;

	Tracker currentPos (_position);
	EventInfo currentEvent (_next_event);

	resetTracking();
	_position._play_pos = _tracks[_active_track];
	parseNextEvent (_next_event);
	if (tick > 0) {
		while (true) {
			EventInfo &info = _next_event;
			if (_position._last_event_tick + info.delta >= tick) {
				_position._play_time += (tick - _position._last_event_tick) * _psec_per_tick;
				_position._play_tick = tick;
				break;
			}

			_position._last_event_tick += info.delta;
			_position._last_event_time += info.delta * _psec_per_tick;
			_position._play_tick = _position._last_event_tick;
			_position._play_time = _position._last_event_time;

			if (info.event == 0xFF) {
				if (info.ext.type == 0x2F) { // End of track
					_position = currentPos;
					_next_event = currentEvent;
					return false;
				} else {
					if (info.ext.type == 0x51 && info.length >= 3) // Tempo
						setTempo (info.ext.data[0] << 16 | info.ext.data[1] << 8 | info.ext.data[2]);
					if (fireEvents)
						_driver->metaEvent (info.ext.type, info.ext.data, (uint16) info.length);
				}
			} else if (fireEvents) {
				if (info.event == 0xF0)
					_driver->sysEx (info.ext.data, (uint16) info.length);
				else
					_driver->send (info.event | info.basic.param1 << 8 | info.basic.param2 << 16);
			}

			parseNextEvent (_next_event);
		}
	}

	if (!_smartJump || !currentPos._play_pos) {
		allNotesOff();
	} else {
		EventInfo targetEvent (_next_event);
		Tracker targetPosition (_position);

		_position = currentPos;
		_next_event = currentEvent;
		hangAllActiveNotes();

		_next_event = targetEvent;
		_position = targetPosition;
	}

	_abort_parse = true;
	return true;
}

void MidiParser::unloadMusic() {
	resetTracking();
	allNotesOff();
	_num_tracks = 0;
	_active_track = 255;
	_abort_parse = true;
}
