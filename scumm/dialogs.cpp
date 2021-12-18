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
 * $Header: /cvsroot/scummvm/scummvm/scumm/dialogs.cpp,v 1.103 2004/01/26 07:40:14 arisme Exp $
 */

#include "stdafx.h"

#include "common/config-manager.h"

#include "gui/chooser.h"
#include "gui/newgui.h"
#include "gui/options.h"
#include "gui/ListWidget.h"

#include "scumm/dialogs.h"
#include "scumm/sound.h"
#include "scumm/scumm.h"
#include "scumm/imuse.h"
#include "scumm/verbs.h"
#include "sound/mididrv.h"
#include "sound/mixer.h"


using GUI::CommandSender;
using GUI::StaticTextWidget;
using GUI::kButtonWidth;
using GUI::kCloseCmd;
using GUI::kTextAlignCenter;
using GUI::kTextAlignLeft;
using GUI::WIDGET_ENABLED;

typedef GUI::OptionsDialog GUI_OptionsDialog;

namespace Scumm {

struct ResString {
	int num;
	char string[80];
};

static ResString string_map_table_v5[] = {
	{28, "How may I serve you?"}, 
	{20, "Select a game to LOAD"},
	{19, "Name your SAVE game"},
	{7, "Save"},
	{8, "Load"},
	{9, "Play"},
	{10, "Cancel"},
	{11, "Quit"},
	{12, "OK"},
	{4, "  Game paused  "}
};


#pragma mark -


const Common::String ScummDialog::queryResString(int stringno) {

	if (stringno == 0)
		return String();

	return string_map_table_v5[stringno - 1].string;
}

#pragma mark -

enum {
	kSaveCmd = 'SAVE',
	kLoadCmd = 'LOAD',
	kPlayCmd = 'PLAY',
	kOptionsCmd = 'OPTN',
	kHelpCmd = 'HELP',
	kAboutCmd = 'ABOU',
	kQuitCmd = 'QUIT'
};

class SaveLoadChooser : public GUI::ChooserDialog {
	typedef Common::String String;
	typedef Common::StringList StringList;
protected:
	bool _saveMode;

public:
	SaveLoadChooser(const String &title, const String &buttonLabel, bool saveMode);
	
	virtual void handleCommand(CommandSender *sender, uint32 cmd, uint32 data);
	const String &getResultString() const;
};

SaveLoadChooser::SaveLoadChooser(const String &title, const String &buttonLabel, bool saveMode)
	: ChooserDialog(title, buttonLabel, 182), _saveMode(saveMode) {

	_list->setEditable(saveMode);
	_list->setNumberingMode(saveMode ? GUI::kListNumberingOne : GUI::kListNumberingZero);
}

const Common::String &SaveLoadChooser::getResultString() const {
	return _list->getSelectedString();
}

void SaveLoadChooser::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	int selItem = _list->getSelected();
	switch (cmd) {
	case GUI::kListItemActivatedCmd:
	case GUI::kListItemDoubleClickedCmd:
		if (selItem >= 0) {
			if (_saveMode || !getResultString().isEmpty()) {
				setResult(selItem);
				close();
			}
		}
		break;
	case GUI::kListSelectionChangedCmd:
		if (_saveMode) {
			_list->startEditMode();
		}
		// Disable button if nothing is selected, or (in load mode) if an empty
		// list item is selected. We allow choosing an empty item in save mode
		// because we then just assign a default name.
		_chooseButton->setEnabled(selItem >= 0 && (_saveMode || !getResultString().isEmpty()));
		_chooseButton->draw();
		break;
	default:
		ChooserDialog::handleCommand(sender, cmd, data);
	}
}

Common::StringList generateSavegameList(ScummEngine *scumm, bool saveMode) {
	// Get savegame names
	Common::StringList l;
	char name[32];
	uint i = saveMode ? 1 : 0;
	bool avail_saves[13];

	SaveFileManager *mgr = OSystem::instance()->get_savefile_manager();

	scumm->listSavegames(avail_saves, ARRAYSIZE(avail_saves), mgr);
	for (; i < ARRAYSIZE(avail_saves); i++) {
		if (avail_saves[i])
			scumm->getSavegameName(i, name, mgr);
		else
			name[0] = 0;
		l.push_back(name);
	}

	delete mgr;

	return l;
}

enum {
	kRowHeight = 18,
	kBigButtonWidth = 90,
	kMainMenuWidth 	= ((kBigButtonWidth + 2 * 8) + 31) & ~31,
	kMainMenuHeight = (6 * kRowHeight) + (2 * 8) + 7 + 5
};

#define addBigButton(label, cmd, hotkey) \
	new GUI::ButtonWidget(this, x, y, kBigButtonWidth, 16, label, cmd, hotkey); y += kRowHeight

MainMenuDialog::MainMenuDialog(ScummEngine *scumm)
	: ScummDialog(scumm, (320 - kMainMenuWidth) / 2, (200 - kMainMenuHeight)/2, kMainMenuWidth, kMainMenuHeight) {
	int y = 7;

	const int x = (_w - kBigButtonWidth) / 2;

	addBigButton("About", kAboutCmd, 'A');
	addBigButton("Options", kOptionsCmd, 'O');
	y += 8;

	addBigButton("Load", kLoadCmd, 'L');
	addBigButton("Save", kSaveCmd, 'S');
	addBigButton("Quit", kQuitCmd, 'Q');
	y += 8;

	addBigButton("Resume", kPlayCmd, 'P');


	//
	// Create the sub dialog(s)
	//
	_aboutDialog = new GUI::AboutDialog();

	_saveDialog = new SaveLoadChooser("Save game:", "Save", true);
	_loadDialog = new SaveLoadChooser("Load game:", "Load", false);
	_optionsDialog = new GUI::OptionsDialog(16, 24, 320 - 32, 200 - 48);
}

MainMenuDialog::~MainMenuDialog() {
	delete _aboutDialog;
	delete _saveDialog;
	delete _loadDialog;
}

void MainMenuDialog::open() {
	OSystem::Property prop;
	prop.show_keyboard = true;
	g_system->property(OSystem::PROP_TOGGLE_VIRTUAL_KEYBOARD, &prop);

	ScummDialog::open();
}

void MainMenuDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	switch (cmd) {
	case kSaveCmd:
		save();
		break;
	case kLoadCmd:
		load();
		break;
	case kPlayCmd:
		close();
		break;
	case kOptionsCmd:
		_optionsDialog->runModal();
		break;		
	case kAboutCmd:
		_aboutDialog->runModal();
		break;
	case kQuitCmd:
		_vm->_quit = true;
		close();
		break;
	default:
		ScummDialog::handleCommand(sender, cmd, data);
	}
}

void MainMenuDialog::close() {
	OSystem::Property prop;

	ScummDialog::close();

	prop.show_keyboard = false;
	g_system->property(OSystem::PROP_TOGGLE_VIRTUAL_KEYBOARD, &prop);
}

void MainMenuDialog::save() {
	int idx;
	_saveDialog->setList(generateSavegameList(_vm, true));
	idx = _saveDialog->runModal();
	if (idx >= 0) {
		const String &result = _saveDialog->getResultString();
		char buffer[20];
		const char *str;
		if (result.isEmpty()) {
			// If the user was lazy and entered no save name, come up with a default name.
			sprintf(buffer, "Save %d", idx + 1);
			str = buffer;
		} else
			str = result.c_str();
		_vm->requestSave(idx + 1, str);
		close();
	}
}

void MainMenuDialog::load() {
	int idx;
	_loadDialog->setList(generateSavegameList(_vm, false));
	idx = _loadDialog->runModal();
	if (idx >= 0) {
		_vm->requestLoad(idx);
		close();
	}
}

#pragma mark -

enum {
	kOKCmd					= 'ok  '
};

enum {
	kKeysCmd = 'KEYS'
};


#pragma mark -

InfoDialog::InfoDialog(ScummEngine *scumm, int res)
: ScummDialog(scumm, 0, 80, 0, 16) { // dummy x and w
	setInfoText(queryResString (res));
}

InfoDialog::InfoDialog(ScummEngine *scumm, const String& message)
: ScummDialog(scumm, 0, 80, 0, 16) { // dummy x and w
	setInfoText(message);
}

void InfoDialog::setInfoText(const String& message) {
	int width = (g_gui.getStringWidth(message) + 16 + 31) & ~31;

	_x = ((SCREEN_WIDTH - width) >> 1);
	_w = width;

	new StaticTextWidget(this, 4, 4, _w - 8, _h - 4, message, kTextAlignCenter);
}

#pragma mark -

PauseDialog::PauseDialog(ScummEngine *scumm)
	: InfoDialog(scumm, 10) {
}

void PauseDialog::handleKeyDown(uint16 ascii, int keycode, int modifiers) {
	if (ascii == ' ')  // Close pause dialog if space key is pressed
		close();
	else
		ScummDialog::handleKeyDown(ascii, keycode, modifiers);
}

ConfirmDialog::ConfirmDialog(ScummEngine *scumm, const String& message)
	: InfoDialog(scumm, message) {
}

void ConfirmDialog::handleKeyDown(uint16 ascii, int keycode, int modifiers) {
	if (tolower(ascii) == 'n') {
		setResult(0);
		close();
	} else if (tolower(ascii) == 'y') {
		setResult(1);
		close();
	} else
		ScummDialog::handleKeyDown(ascii, keycode, modifiers);
}

} // End of namespace Scumm
