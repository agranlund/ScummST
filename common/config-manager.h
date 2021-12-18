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
 * $Header: /cvsroot/scummvm/scummvm/common/config-manager.h,v 1.11 2004/02/07 04:53:59 ender Exp $
 *
 */

#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H


#include "stdafx.h"
#include "common/str.h"

enum ConfigId
{
	kConfig_Subtitles,
	kConfig_MultiMidi,
	kConfig_MasterVolume,
	kConfig_SfxVolume,
	kConfig_MusicVolume,
	kConfig_GfxMode,
	kConfig_MusicDriver,
	kConfig_SoundDriver,
	kConfig_DebugLevel,
	kConfig_Platform,
	kConfig_Language,
	kConfigCount
};


extern byte _config[kConfigCount];
extern Common::String _conf_gamepath;
extern Common::String _conf_filename;

extern void LoadConfig();
extern void SaveConfig();
extern int  GetConfig(ConfigId id);
extern void SetConfig(ConfigId id, byte val);


#endif
