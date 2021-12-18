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
 * $Header: /cvsroot/scummvm/scummvm/gui/about.cpp,v 1.12.2.1 2004/03/13 14:11:45 eriktorbjorn Exp $
 */

#include "stdafx.h"
#include "common/engine.h"
#include "common/version.h"
#include "gui/about.h"
#include "gui/newgui.h"
#include "gui/widget.h"

extern Common::String gGameName;


namespace GUI {

AboutDialog::AboutDialog()
	: Dialog(8, 8, 320-16, 188/*174*/) {
	
	addButton((_w - kButtonWidth) / 2, 160/*150*/, "OK", kCloseCmd, '\r');	// Close dialog - FIXME

	new StaticTextWidget(this, 0, 15, _w, kLineHeight, gGameName, kTextAlignCenter);
	new StaticTextWidget(this, 0, 30, _w, kLineHeight, "(c) LucasArts", kTextAlignCenter);
	new StaticTextWidget(this, 0, 50, _w, kLineHeight, "-----------------------------------------", kTextAlignCenter);

	Common::String version("ScummVM "); version += gScummVMVersion;
	new StaticTextWidget(this, 0, 70, _w, kLineHeight, version, kTextAlignCenter);

	new StaticTextWidget(this, 0, 80, _w, kLineHeight, "Copyright (C) 2002-2004 The ScummVM project", kTextAlignCenter);
	new StaticTextWidget(this, 0, 90, _w, kLineHeight, "http://www.scummvm.org", kTextAlignCenter);

	new StaticTextWidget(this, 0, 110, _w, kLineHeight, "Atari ST version by Anders Granlund", kTextAlignCenter);
	new StaticTextWidget(this, 0, 120, _w, kLineHeight, "http://www.happydaze.se", kTextAlignCenter);
	Common::String date("(built on "); date += gScummVMBuildDate; date += ')';
	new StaticTextWidget(this, 0, 140, _w, kLineHeight, date, kTextAlignCenter);


/*
	// TODO: Probably should display something regarding GPL
	new StaticTextWidget(this, 0, 45, _w, kLineHeight, "Copyright (C) 2002-2020 The ScummVM project", kTextAlignCenter);
	new StaticTextWidget(this, 0, 55, _w, kLineHeight, "http://www.scummvm.org", kTextAlignCenter);
	new StaticTextWidget(this, 0, 75, _w, kLineHeight, "LucasArts SCUMM Games (C) LucasArts", kTextAlignCenter);
	new StaticTextWidget(this, 0, 85, _w, kLineHeight, "Humongous SCUMM Games (C) Humongous", kTextAlignCenter);
	new StaticTextWidget(this, 0, 95, _w, kLineHeight, "Simon the Sorcerer (C) Adventure Soft", kTextAlignCenter);
	new StaticTextWidget(this, 0, 105, _w, kLineHeight, "Beneath a Steel Sky (C) Revolution", kTextAlignCenter);
	new StaticTextWidget(this, 0, 115, _w, kLineHeight, "Broken Sword Games (C) Revolution", kTextAlignCenter);
	new StaticTextWidget(this, 0, 125, _w, kLineHeight, "Flight of the Amazon Queen (C) John Passfield", kTextAlignCenter);
	new StaticTextWidget(this, 0, 135, _w, kLineHeight, "and Steve Stamatiadis", kTextAlignCenter);
*/
}

} // End of namespace GUI
