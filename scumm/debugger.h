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
 * $Header: /cvsroot/scummvm/scummvm/scumm/debugger.h,v 1.34 2004/01/10 05:20:15 ender Exp $
 */

#ifndef DEBUGGER_H
#define DEBUGGER_H

#ifndef DISABLE_DEBUGGER

namespace Common {

template <class T>
class Debugger {
public:
	Debugger();
	virtual ~Debugger();
	
	int DebugPrintf(const char *format, ...);

	virtual void onFrame();

	virtual void attach(const char *entry = 0);
	bool isAttached() const { return _isAttached; }

protected:
	typedef bool (T::*DebugProc)(int argc, const char **argv);
	
	enum {
		DVAR_INT,
		DVAR_BOOL,
		DVAR_INTARRAY,
		DVAR_STRING
	};
	
	struct DVar {
		char name[30];
		void *variable;
		int type, optional;
	};
	
	struct DCmd {
		char name[30];
		DebugProc function;
	};
	
	int _frame_countdown, _dvar_count, _dcmd_count;
	DVar _dvars[256];
	DCmd _dcmds[256];
	bool _detach_now;

private:
	bool _isAttached;
	char *_errStr;
	bool _firstTime;

protected:
	void detach();
	void enter();

	virtual void preEnter() = 0;
	virtual void postEnter() = 0;

	bool RunCommand(const char *input);
	bool TabComplete(const char *input, char*& completion);

	void DVar_Register(const char *varname, void *pointer, int type, int optional);
	void DCmd_Register(const char *cmdname, DebugProc pointer);
};

}	// End of namespace Common


namespace Scumm {

class ScummEngine;

class ScummDebugger : public Common::Debugger<ScummDebugger> {
public:
	ScummDebugger(ScummEngine *s);
	
protected:
	ScummEngine *_vm;
	bool  _old_soundsPaused;

	virtual void preEnter();
	virtual void postEnter();

	// Commands
	bool Cmd_Exit(int argc, const char **argv);
	bool Cmd_Room(int argc, const char **argv);
	bool Cmd_LoadGame(int argc, const char **argv);
	bool Cmd_SaveGame(int argc, const char **argv);
	bool Cmd_Restart(int argc, const char **argv);

	bool Cmd_PrintActor(int argc, const char **argv);
	bool Cmd_PrintBox(int argc, const char **argv);
	bool Cmd_PrintBoxMatrix(int argc, const char **argv);
	bool Cmd_PrintObjects(int argc, const char **argv);
	bool Cmd_Actor(int argc, const char **argv);
	bool Cmd_Object(int argc, const char **argv);
	bool Cmd_Script(int argc, const char **argv);
	bool Cmd_PrintScript(int argc, const char **argv);
	bool Cmd_ImportRes(int argc, const char **argv);

	bool Cmd_PrintDraft(int argc, const char **argv);

	bool Cmd_Debug(int argc, const char **argv);
	bool Cmd_DebugLevel(int argc, const char **argv);
	bool Cmd_Help(int argc, const char **argv);

	bool Cmd_Show(int argc, const char **argv);
	bool Cmd_Hide(int argc, const char **argv);

	bool Cmd_IMuse (int argc, const char **argv);
	
	void printBox(int box);
	void drawBox(int box);
};

} // End of namespace Scumm

#endif
#endif