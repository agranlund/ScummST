/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001/2002 The ScummVM project
 * Copyright (C) 2002 ph0x (GP32 port)
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
 * Standalone DOS->Atari resource converter for PC.
 * Anders Granlund
 * 
 */


#include "converter.h"
#include "scumm/scumm.h"
#include "common/gameDetector.h"
#include "common/config-manager.h"
#include "common/timer.h"
extern byte _atari_res_version;

int __dummy_mutex;

void ScummResourceConvert()
{
	printf("ScummST Resource converter v%d\n", _atari_res_version);
	Common::String path = _conf_gamepath;
	byte conf[kConfigCount];
	for (int i=0; i<kConfigCount; i++)
		conf[i] = _config[i];

	GameDetector detector;
	_conf_gamepath = path;
	for (int i=0; i<kConfigCount; i++)
		_config[i] = conf[i];

	const Scumm::ScummGameSettings* setting = Scumm::scumm_settings;
	while (setting->name != NULL)
	{
		detector._targetName = setting->name;
		if (!detector.detectMain())
			continue;

		g_timer = new Timer(g_system);
		Engine *engine = detector.createEngine(g_system);
		if (!engine || !Scumm::g_scumm)
			continue;

		if (Scumm::g_scumm->convertResourcesForAtari())
			printf("Converted: [Scumm%d] %s\n", setting->version, setting->description);

		delete engine;
		Scumm::g_scumm = 0;
		setting++;
	}
}

uint32 OSystem_Converter::property(int param, Property *value)
{
	switch(param) {
		case PROP_GET_SAMPLE_RATE:
			return 11025;
	}
	return 0;
}

OSystem *OSystem_Converter_create()
{ 
	return OSystem_Converter::create();
}

OSystem *OSystem_Converter::create()
{
	g_system = new OSystem_Converter();
	ScummResourceConvert();
	delete g_system;
	g_system = 0;
	exit(0);
}
