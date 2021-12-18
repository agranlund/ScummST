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
 * $Header: /cvsroot/scummvm/scummvm/scumm/dialogs.h,v 1.38 2004/01/26 07:40:14 arisme Exp $
 */

#ifndef SCUMM_DIALOGS_H
#define SCUMM_DIALOGS_H

#include "common/str.h"
#include "gui/about.h"
#include "gui/options.h"
#include "gui/dialog.h"
#include "gui/widget.h"


namespace GUI {
	class ListWidget;
}


namespace Scumm {

class ScummEngine;

class ScummDialog : public GUI::Dialog {
public:
	ScummDialog(ScummEngine *scumm, int x, int y, int w, int h)
		: GUI::Dialog(x, y, w, h), _vm(scumm) {}
	
protected:
	typedef Common::String String;

	ScummEngine *_vm;

	// Query a string from the resources
	const String queryResString(int stringno);
};

class SaveLoadChooser;

class MainMenuDialog : public ScummDialog {
public:
	MainMenuDialog(ScummEngine *scumm);
	~MainMenuDialog();
	virtual void handleCommand(GUI::CommandSender *sender, uint32 cmd, uint32 data);
	virtual void open();	
	virtual void close();

protected:
	GUI::Dialog			*_aboutDialog;
	SaveLoadChooser		*_saveDialog;
	SaveLoadChooser		*_loadDialog;
	GUI::OptionsDialog	*_optionsDialog;

	void save();
	void load();
};


class ConfigDialog : public ScummDialog {
protected:
	ScummEngine *_vm;

public:
	ConfigDialog(ScummEngine *scumm);
	~ConfigDialog();

	virtual void open();
	virtual void close();
	virtual void handleCommand(GUI::CommandSender *sender, uint32 cmd, uint32 data);

protected:
	GUI::CheckboxWidget *subtitlesCheckbox;
};

class InfoDialog : public ScummDialog {
public:
	// arbitrary message
	InfoDialog(ScummEngine *scumm, const String& message);
	// from resources
	InfoDialog(ScummEngine *scumm, int res);

	virtual void handleMouseDown(int x, int y, int button, int clickCount) { 
		close();
	}
	virtual void handleKeyDown(uint16 ascii, int keycode, int modifiers) {
		setResult(ascii);
		close();
	}

protected:
	void setInfoText (const String& message);
};

class PauseDialog : public InfoDialog {
public:
	PauseDialog(ScummEngine *scumm);
	virtual void handleKeyDown(uint16 ascii, int keycode, int modifiers);
};

class ConfirmDialog : public InfoDialog {
public:
	ConfirmDialog(ScummEngine *scumm, const String& message);
	virtual void handleKeyDown(uint16 ascii, int keycode, int modifiers);
};

} // End of namespace Scumm

#endif
