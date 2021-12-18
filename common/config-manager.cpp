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
 * $Header: /cvsroot/scummvm/scummvm/common/config-manager.cpp,v 1.17 2004/02/07 04:53:59 ender Exp $
 *
 */

#include "common/config-manager.h"
#include "common/file.h"

#define CONFIG_VERSION 	1

byte _config[kConfigCount];
Common::String _conf_gamepath;
Common::String _conf_filename = "settings.inf";

void LoadConfig()
{
	// set defaults
	memset(_config, 0, kConfigCount);
	SetConfig(kConfig_Subtitles,		true);
	SetConfig(kConfig_MultiMidi,		false);
	SetConfig(kConfig_MasterVolume,		255);
	SetConfig(kConfig_SfxVolume,		240);
	SetConfig(kConfig_MusicVolume,		192);
	SetConfig(kConfig_GfxMode,			0);
	SetConfig(kConfig_MusicDriver,		0);			// auto
	SetConfig(kConfig_SoundDriver,		0);			// auto
#if defined(__ATARI__) && !defined(RELEASEBUILD)
	SetConfig(kConfig_DebugLevel,		1);
#else
	SetConfig(kConfig_DebugLevel,		0);
#endif
	SetConfig(kConfig_Platform,			0);
	SetConfig(kConfig_Language,			0);

	_conf_gamepath = "";

	// load from file
	File f;
	if (f.open(_conf_filename.c_str()))
	{
		if (CONFIG_VERSION == f.readByte())
		{
			SetConfig(kConfig_SoundDriver,	f.readByte());
			SetConfig(kConfig_MusicDriver,	f.readByte());
			SetConfig(kConfig_MusicVolume,	f.readByte());
			SetConfig(kConfig_Subtitles,	f.readByte());
		}
		f.close();
	}
}

void SaveConfig()
{
	File f;
	if (f.open(_conf_filename.c_str(), 0, File::kFileWriteMode))
	{
		f.writeByte(CONFIG_VERSION);
		f.writeByte(GetConfig(kConfig_SoundDriver));
		f.writeByte(GetConfig(kConfig_MusicDriver));
		f.writeByte(GetConfig(kConfig_MusicVolume));
		f.writeByte(GetConfig(kConfig_Subtitles));
		f.close();
	}
}

int GetConfig(ConfigId id)
{
	return (int) (_config[id]);
}

void SetConfig(ConfigId id, byte val)
{
	_config[id] = val;
}

