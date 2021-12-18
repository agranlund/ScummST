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
 * $Header: /cvsroot/scummvm/scummvm/gui/options.cpp,v 1.47.2.1 2004/02/24 21:54:09 arisme Exp $
 */

#include "stdafx.h"
//#include "gui/browser.h"
#include "gui/chooser.h"
#include "gui/newgui.h"
#include "gui/options.h"
#include "gui/PopUpWidget.h"

#ifdef __ATARI__
#include "backends/atari/atari.h"
#endif

//#include "base/gameDetector.h"
#include "common/config-manager.h"
#include "common/scaler.h"
#include "sound/mididrv.h"
#include "sound/mixer.h"

#if defined(ENGINE_SCUMM5) || defined(ENGINE_SCUMM6)
#include "scumm/scumm.h"
#include "scumm/imuse.h"
#include "scumm/imuse_internal.h"
#endif

namespace GUI {

enum {
	kMasterVolumeChanged	= 'mavc',
	kMusicVolumeChanged		= 'muvc',
	kSfxVolumeChanged		= 'sfvc',
};

struct DriverOption
{
	const char* Name;
	int			Id;
};

OptionsDialog::OptionsDialog(int x, int y, int w, int h)
	: Dialog(x, y, w, h),
	_subCheckbox(0),
	_musicVolumeSlider(0), _musicVolumeLabel(0)
	{

	const int wx = 8;
	const int ww = w - 16;
	const int vBorder = 5;
	int yoffset = vBorder;

	#ifdef __ATARI__

	OSystem_Atari* sys = g_engine ? (OSystem_Atari*)g_engine->_system : 0;

	// sound
	_sfxPopUp = new PopUpWidget(this, wx, yoffset, ww, kLineHeight, "Sound driver: ", 96);
	const OSystem_Atari::DriverCfg** soundDrivers = sys->soundDrivers;
	int oldSoundDriver = GetConfig(kConfig_SoundDriver);
	for (int i=0; i<16; i++)
	{
		if (soundDrivers[i] == 0)
			break;
		
		_sfxPopUp->appendEntry(soundDrivers[i]->name, soundDrivers[i]->id);
		if (soundDrivers[i]->id == oldSoundDriver)
			_oldSoundSelection = i;
	}
	_sfxPopUp->setSelected(_oldSoundSelection);
	yoffset += 16;

	// music
	_midiPopUp = new PopUpWidget(this, wx, yoffset, ww, kLineHeight, "Music driver: ", 96);
	const OSystem_Atari::DriverCfg** musicDrivers = sys->musicDrivers;
	int oldMusicDriver = GetConfig(kConfig_MusicDriver);
	for (int i=0; i<16; i++)
	{
		if (musicDrivers[i] == 0)
			break;
		
		_midiPopUp->appendEntry(musicDrivers[i]->name, musicDrivers[i]->id);
		if (musicDrivers[i]->id == oldMusicDriver)
			_oldMusicSelection = i;
	}
	_midiPopUp->setSelected(_oldMusicSelection);

	yoffset += 16 + 8;

	#endif //__ATARI__

	// music volume
	_musicVolumeSlider = new SliderWidget(this, wx, yoffset, 185, 12, "Music volume: ", 96, kMusicVolumeChanged);
	_musicVolumeLabel = new StaticTextWidget(this, 200, yoffset + 2, 24, kLineHeight, "100%", kTextAlignLeft);
	_musicVolumeSlider->setMinValue(0); _musicVolumeSlider->setMaxValue(255);
	_musicVolumeLabel->setFlags(WIDGET_CLEARBG);
	yoffset += 16 + 8;	

	// Subtitles on/off
	_subCheckbox = new CheckboxWidget(this, wx, yoffset, ww, 16, "Display subtitles");
	yoffset += 16;

	addButton((w - kButtonWidth) / 2, h - kButtonHeight - 8, "OK", kOKCmd, '\r');
}

void OptionsDialog::open() {
	Dialog::open();
#ifdef __ATARI__
	_oldMusicSelection = _midiPopUp->getSelected();
	_oldSoundSelection = _sfxPopUp->getSelected();
#endif //__ATARI__
	_subCheckbox->setState(GetConfig(kConfig_Subtitles));

	byte vol = GetConfig(kConfig_MusicVolume);
	_musicVolumeSlider->setValue(vol);
	_musicVolumeLabel->setValue(vol);

	// Reset result value
	setResult(0);
}

void OptionsDialog::close() {
	if (getResult() > 0) {

		#ifdef __ATARI__

		OSystem_Atari* sys = (OSystem_Atari*)g_engine->_system;
		#if defined(ENGINE_SCUMM5) || defined(ENGINE_SCUMM6)
		Scumm::IMuse* imuse = Scumm::g_scumm->_imuse;
		#endif

		// subtitles
		SetConfig(kConfig_Subtitles, _subCheckbox->getState());
		#if defined(ENGINE_SCUMM5) || defined(ENGINE_SCUMM6)
		Scumm::g_scumm->_subtitles = _subCheckbox->getState();
		#endif

		// volume
		byte newVolume = _musicVolumeSlider->getValue();
		if (newVolume != GetConfig(kConfig_MusicVolume))
		{
			SetConfig(kConfig_MusicVolume, _musicVolumeSlider->getValue());
			#if defined(ENGINE_SCUMM5) || defined(ENGINE_SCUMM6)
			imuse->set_music_volume(GetConfig(kConfig_MusicVolume));
			#endif
		}

		// music
		if (_midiPopUp->getSelected() != _oldMusicSelection)
		{
			int newMusicDriver = _midiPopUp->getSelectedTag();
			int oldMusicDriver = GetConfig(kConfig_MusicDriver);
			SetConfig(kConfig_MusicDriver, newMusicDriver);

			bool native_mt32 = false;
			MidiDriver* midi = 0;
			switch(newMusicDriver)
			{
				case MD_NULL:
					midi = MidiDriver_NULL_create();
					break;
				case MD_STMIDI_MT32:
					native_mt32 = true;
				case MD_STMIDI_GM:
					midi = MidiDriver_STMIDI_create();
					break;
				default:
					midi = MidiDriver_STCHIP_create();
					break;
			}
			#if defined(ENGINE_SCUMM5) || defined(ENGINE_SCUMM6)
			imuse->changeDriver(midi, native_mt32);
			#endif
			MidiDriver_STCHIP_Mute(true);
		}

		// sound
		if (_sfxPopUp->getSelected() != _oldSoundSelection)
		{
			int newSoundDriver = _sfxPopUp->getSelectedTag();
			SetConfig(kConfig_SoundDriver, newSoundDriver);
			sys->initSound(OSystem_Atari::kSoundPlayer_Off, 0);
			sys->initSound((OSystem_Atari::SoundPlayer)newSoundDriver, 1);
			if (g_engine->_mixer)
				g_engine->_mixer->stopAll();
		}

		#endif //__ATARI__
	
		// Save config file
		SaveConfig();
	}
	else
	{
		#ifdef __ATARI__
		_sfxPopUp->setSelected(_oldSoundSelection);
		_midiPopUp->setSelected(_oldMusicSelection);
		#endif //__ATARI__
	}

	Dialog::close();
}

void OptionsDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	switch (cmd) {
	case kMusicVolumeChanged:
		_musicVolumeLabel->setValue(_musicVolumeSlider->getValue());
		_musicVolumeLabel->draw();
		break;
	case kOKCmd:
		setResult(1);
		close();
		break;
	default:
		Dialog::handleCommand(sender, cmd, data);
	}
}

} // End of namespace GUI
