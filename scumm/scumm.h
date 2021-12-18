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
 * $Header: /cvsroot/scummvm/scummvm/scumm/scumm.h,v 1.369.2.1 2004/02/24 00:44:16 kirben Exp $
 *
 */

#ifndef SCUMM_H
#define SCUMM_H

#include "common/engine.h"
#include "common/file.h"
#include "common/rect.h"
#include "common/str.h"

#include "scumm/gfx.h"
#include "scumm/script.h"

namespace GUI {
	class Dialog;
	class OptionsDialog;
}
using GUI::Dialog;
class GameDetector;


namespace Scumm {

class Actor;
class BaseCostumeRenderer;
class CharsetRenderer;
class IMuse;
class Insane;
class MusicEngine;
class ScummEngine;
#ifndef DISABLE_DEBUGGER
class ScummDebugger;
#endif
class Serializer;
class Sound;

struct Box;
struct BoxCoords;
struct FindObjectInRoom;
struct ScummGameSettings;

// Use g_scumm from error() ONLY
extern ScummEngine *g_scumm;

/* System Wide Constants */
enum {
	NUM_LOCALSCRIPT = 60,
	NUM_SENTENCE = 6,
	KEY_SET_OPTIONS = 3456, // WinCE
	KEY_ALL_SKIP = 3457   // WinCE
};

/** SCUMM feature flags. */
enum GameFeatures {
	GF_NEW_OPCODES         = 1 << 0,
//	GF_NEW_CAMERA          = 1 << 1,
//	GF_NEW_COSTUMES        = 1 << 2,
//	GF_DIGI_IMUSE          = 1 << 3,
	GF_USE_KEY             = 1 << 4,
	GF_DRAWOBJ_OTHER_ORDER = 1 << 5,
//	GF_SMALL_HEADER        = 1 << 6,
//	GF_SMALL_NAMES         = 1 << 7,
//	GF_OLD_BUNDLE          = 1 << 8,
//	GF_16COLOR             = 1 << 9,
//	GF_OLD256              = 1 << 10,
	GF_AUDIOTRACKS         = 1 << 11,
//	GF_NO_SCALING          = 1 << 12,
//	GF_FEW_LOCALS          = 1 << 13,
//	GF_HUMONGOUS           = 1 << 14,
//	GF_AFTER_HEV7          = 1 << 15,
	
//	GF_FMTOWNS             = 1 << 16,
	GF_AMIGA               = 1 << 17,
//	GF_NES                 = 1 << 18,
//	GF_ATARI_ST            = 1 << 19,
//	GF_MACINTOSH           = 1 << 20,
	GF_PC                  = 1 << 21,
//	GF_DEMO                = 1 << 22,
	
//	GF_EXTERNAL_CHARSET    = GF_SMALL_HEADER
};

struct ScummGameSettings {
	const char *name;
	const char *description;
	byte id, version;
	int midi; // MidiDriverType values
	uint32 features;
	const char *baseFilename;
};


extern const ScummGameSettings scumm_settings[];


enum ObjectClass {
	kObjectClassNeverClip = 20,
	kObjectClassAlwaysClip = 21,
	kObjectClassIgnoreBoxes = 22,
	kObjectClassYFlip = 29,
	kObjectClassXFlip = 30,
	kObjectClassPlayer = 31,	// Actor is controlled by the player
	kObjectClassUntouchable = 32
};

/* SCUMM Debug Channels */
#ifndef RELEASEBUILD
void CDECL debugC(int level, const char *s, ...);
#else
#define debugC(a,b,...)
#endif



enum {
	DEBUG_GENERAL	=	1 << 0,		// General debug
	DEBUG_SCRIPTS	=	1 << 2,		// Track script execution (start/stop/pause)
	DEBUG_OPCODES	=	1 << 3,		// Track opcode invocations
	DEBUG_VARS	=	1 << 4,		// Track variable changes
	DEBUG_RESOURCE	=	1 << 5,		// Track resource loading / allocation
	DEBUG_IMUSE	=	1 << 6,		// Track iMUSE events
	DEBUG_SOUND	=	1 << 7,		// General Sound Debug
	DEBUG_ACTORS	=	1 << 8		// General Actor Debug
};


struct MemBlkHeader {
	uint32 size;
};

struct VerbSlot;
struct ObjectData;

struct BlastText {
	int16 xpos, ypos;
	Common::Rect rect;
	byte color;
	byte charset;
	bool center;
	byte text[256];
};

struct V2MouseoverBox {
	Common::Rect rect;
	byte color;
	byte hicolor;
};

enum ResTypes {
	rtFirst = 1,
	rtRoom = 1,
	rtScript = 2,
	rtCostume = 3,
	rtSound = 4,
	rtInventory = 5,
	rtCharset = 6,
	rtString = 7,
	rtVerb = 8,
	rtActorName = 9,
	rtBuffer = 10,
	rtScaleTable = 11,
	rtTemp = 12,
	rtFlObject = 13,
	rtMatrix = 14,
	rtBox = 15,
	rtObjectName = 16,
	rtRoomScripts = 17,
	rtLast = 17,
	rtNumTypes = 18
};

enum {
	LIGHTMODE_dark			= 0,
	LIGHTMODE_actor_base	= 1,
	LIGHTMODE_screen		= 2,
	LIGHTMODE_flashlight	= 4,
	LIGHTMODE_actor_color	= 8
};

enum {
	MBS_LEFT_CLICK = 0x8000,
	MBS_RIGHT_CLICK = 0x4000,
	MBS_MOUSE_MASK = (MBS_LEFT_CLICK | MBS_RIGHT_CLICK),
	MBS_MAX_KEY	= 0x0200
};

enum ScummGameId {
	GID_TENTACLE,
	GID_MONKEY2,
	GID_INDY4,
	GID_MONKEY,
	GID_SAMNMAX,
//	GID_MONKEY_EGA,
//	GID_PASS,
//	GID_LOOM256,
//	GID_ZAK256,
//	GID_INDY3,
//	GID_LOOM,
//	GID_FT,
//	GID_DIG,
//	GID_MONKEY_VGA,
//	GID_CMI,
//	GID_MANIAC,
//	GID_ZAK,
//	GID_PUTTPUTT,
//	GID_PUTTDEMO,
//	GID_PUTTMOON,
//	GID_FBEAR,
//	GID_PJSDEMO,
//	GID_MONKEY_SEGA,
	GID_FBPACK
};

#ifdef GAME_SAMNMAX
#define MAX_NUM_ACTORS	30
#else
#define MAX_NUM_ACTORS	15
#endif

#define _baseRooms res.address[rtRoom]
#define _baseScripts res.address[rtScript]
#define _baseInventoryItems res.address[rtInventory]
#define _baseFLObject res.address[rtFlObject]
#define _baseArrays res.address[rtString]

#define _roomFileOffsets res.roomoffs[rtRoom]

struct SentenceTab {
	byte verb;
	byte preposition;
	uint16 objectA;
	uint16 objectB;
	uint8 freezeCount;
};

// TODO / FIXME: next time save game format changes, Fingolfin would like to
// revise StringTab. In particular, all t_* fields can be removed, making some
// code a bit cleaner & easier to understand.
struct StringTab {
	int16 t_xpos, t_ypos;
	int16 t_right;
	int16 xpos, ypos;
	int16 right;
	byte color, t_color;
	byte charset, t_charset;
	bool center, t_center;
	bool overhead, t_overhead;
	bool no_talk_anim, t_no_talk_anim;
};

enum WhereIsObject {
	WIO_NOT_FOUND = -1,
	WIO_INVENTORY = 0,
	WIO_ROOM = 1,
	WIO_GLOBAL = 2,
	WIO_LOCAL = 3,
	WIO_FLOBJECT = 4
};

struct LangIndexNode {
	char tag[12+1];
	int32 offset;
};




class ScummEngine : public Engine {
	friend class GUI::OptionsDialog;
	friend class AtariResourceConverter;
	friend class AtariSoundConverter;

#ifndef DISABLE_DEBUGGER
	friend class ScummDebugger;
#endif
	friend class SmushPlayer;
	friend class Insane;
	friend class CharsetRenderer;
	friend class CostumeRenderer;
	
#ifndef RELEASEBUILD
	void errorString(const char *buf_input, char *buf_output);
#endif
public:
	/* Put often used variables at the top.
	 * That results in a shorter form of the opcode
	 * on some architectures. */
	IMuse *_imuse;
	MusicEngine *_musicEngine;
	Sound *_sound;

	VerbSlot *_verbs;
	ObjectData *_objs;
#ifndef DISABLE_DEBUGGER
	ScummDebugger *_debugger;
#endif

	// Core variables
	byte _gameId;
	byte _version;
	uint32 _features;

	/** Random number generator */
	Common::RandomSource _rnd;

	/** Graphics manager */
	Gdi gdi;

protected:
	/** Central resource data. */
	struct ResData {
		byte mode[rtNumTypes];
		uint16 num[rtNumTypes];
		uint32 tags[rtNumTypes];
		const char *name[rtNumTypes];
		byte **address[rtNumTypes];
		byte *flags[rtNumTypes];
		byte *roomno[rtNumTypes];
		uint32 *roomoffs[rtNumTypes];
	};

	ResData res;

	VirtualMachineState vm;

public:
	// Constructor / Destructor
	ScummEngine(GameDetector *detector, OSystem *syst, const ScummGameSettings &gs);
	virtual ~ScummEngine();

	// Init functions
	void scummInit();
	void initScummVars();
	virtual void setupScummVars();

	// Startup functions
	void launch();
	void go();

	// Scumm main loop
	void mainRun();
	int scummLoop(int delta);

	// Event handling
	void parseEvents();
	void waitForTimer(int msec_delay);
	void processKbd(bool smushMode);
	void clearClickedStatus();

	// Misc utility functions
	uint32 _debugFlags;
	const char *getGameName() const { return _gameName.c_str(); }
	const char *getGameDataPath() const;

	// Cursor/palette
	void updateCursor();
	void animateCursor();
	void updatePalette();

	void pauseGame();
	void restart();
	void shutDown();

	/** We keep running until this is set to true. */
	bool _quit;

protected:
	Dialog *_pauseDialog;
	Dialog *_mainMenuDialog;

protected:
	int runDialog(Dialog &dialog);
	void confirmexitDialog();
	void confirmrestartDialog();
	void pauseDialog();
	void mainMenuDialog();

protected:
	char displayError(const char *altButton, const char *message/*, ...*/);

protected:
	byte _fastMode;

	Actor *_actors;	// Has _numActors elements
	
	uint16 *_inventory;
	uint16 *_newNames;
public:
	// VAR is a wrapper around scummVar, which attempts to include additional
	// useful information should an illegal var access be detected.
	#define VAR(x)	scummVar(x, #x, __FILE__, __LINE__)
	int32& scummVar(byte var, const char *varName, const char *file, int line)
	{
		if (var == 0xFF) {
			warning("Illegal access to variable %s in file %s, line %d", varName, file, line);
			// Return a fake memory location, so that at least no innocent variable
			// gets overwritten.
			static int32 fake;
			fake = 0;
			return fake;
		}
		return _scummVars[var];
	}
	int32 scummVar(byte var, const char *varName, const char *file, int line) const
	{
		if (var == 0xFF) {
			warning("Illegal access to variable %s in file %s, line %d", varName, file, line);
		}
		return _scummVars[var];
	}

protected:
	int16 _varwatch;
	int32 *_scummVars;
	byte *_bitVars;

	/* Global resource tables */
	int _numVariables, _numBitVariables, _numLocalObjects;
	int _numGlobalObjects, _numArray, _numVerbs, _numFlObject;
	int _numInventory, _numRooms, _numScripts, _numSounds;
	int _numNewNames, _numGlobalScripts;
	int _numActors;
public:
	int _numCostumes;	// FIXME - should be protected, used by Actor::remapActorPalette
	int _numCharsets;	// FIXME - should be protected, used by CharsetRenderer

	int getNumSounds() const { return _numSounds; }
	BaseCostumeRenderer* _costumeRenderer;
	
	char *_audioNames;
	int32 _numAudioNames;

protected:
	/* Current objects - can go in their respective classes */
	byte _curActor;
	int _curVerb;
	int _curVerbSlot;
	int _curPalIndex;

public:
	byte _currentRoom;	// FIXME - should be protected but Actor::isInCurrentRoom uses it
	int _roomResource;  // FIXME - should be protected but Sound::pauseSounds uses it
	bool _egoPositioned;	// Used by Actor::putActor, hence public

protected:
	int _keyPressed;
	uint16 _lastKeyHit;

	Common::Point _mouse;
	Common::Point _virtualMouse;

	uint16 _mouseButStat;
	byte _leftBtnPressed, _rightBtnPressed;

	/** The bootparam, to be passed to the script 1, the bootscript. */
	int _bootParam;
	
	// Various options useful for debugging
	uint16 _debugMode;

	// Save/Load class - some of this may be GUI
	byte _saveLoadFlag, _saveLoadSlot;
	uint32 _lastSaveTime;
	bool _saveTemporaryState;
	char _saveLoadName[32];

	bool saveState(int slot, bool compat, SaveFileManager *mgr);
	bool loadState(int slot, bool compat, SaveFileManager *mgr);
	bool saveState(int slot, bool compat) {
		SaveFileManager *mgr = _system->get_savefile_manager();
		bool result = saveState(slot, compat, mgr);
		delete mgr;
		return result;
	}
	bool loadState(int slot, bool compat) {
		SaveFileManager *mgr = _system->get_savefile_manager();
		bool result = loadState(slot, compat, mgr);
		delete mgr;
		return result;
	}
	void saveOrLoad(Serializer *s, uint32 savegameVersion);
	void saveLoadResource(Serializer *ser, int type, int index);	// "Obsolete"
	void saveResource(Serializer *ser, int type, int index);
	void loadResource(Serializer *ser, int type, int index);
	void makeSavegameName(char *out, int slot, bool compatible);

	int getKeyState(int key);

public:
	bool getSavegameName(int slot, char *desc, SaveFileManager *mgr);
	void listSavegames(bool *marks, int num, SaveFileManager *mgr);
	
	void requestSave(int slot, const char *name, bool compatible = false);
	void requestLoad(int slot);

protected:
	/* Heap and memory management */
	uint32 _maxHeapThreshold, _minHeapThreshold;
	void lock(int type, int i);
	void unlock(int type, int i);

	/* Script VM - should be in Script class */
	uint32 _localScriptList[NUM_LOCALSCRIPT];
	const byte *_scriptPointer, *_scriptOrgPointer;
	byte _opcode, _currentScript;
	uint16 _curExecScript;
	byte **_lastCodePtr;
	int _resultVarNumber, _scummStackPos;
	int _vmStack[150];
	int _keyScriptKey, _keyScriptNo;
	
	virtual void setupOpcodes() = 0;
	virtual void executeOpcode(byte i) = 0;
#ifndef RELEASEBUILD
	virtual const char *getOpcodeDesc(byte i) = 0;
#endif
	void initializeLocals(int slot, int *vars);
	int	getScriptSlot();

	void startScene(int room, Actor *a, int b);

public:
	void runScript(int script, bool freezeResistant, bool recursive, int *lvarptr);
	void stopScript(int script);
	bool isScriptRunning(int script) const;	// FIXME - should be protected, used by Sound::startTalkSound

protected:
	void runObjectScript(int script, int entry, bool freezeResistant, bool recursive, int *vars, int slot = -1);
	void runScriptNested(int script);
	void executeScript();
	void updateScriptPtr();
	void runInventoryScript(int i);
	void checkAndRunSentenceScript();
	void runExitScript();
	void runEntryScript();
	void runAllScripts();
	void freezeScripts(int scr);
	void unfreezeScripts();

	bool isScriptInUse(int script) const;
	bool isRoomScriptRunning(int script) const;

	void killAllScriptsExceptCurrent();
	void killScriptsAndResources();
	void decreaseScriptDelay(int amount);

	void stopObjectCode();
	void stopObjectScript(int script);

	void getScriptBaseAddress();
	void getScriptEntryPoint();
	int getVerbEntrypoint(int obj, int entry);

	byte fetchScriptByte();
	virtual uint fetchScriptWord();
	virtual int fetchScriptWordSigned();
	void ignoreScriptWord() { fetchScriptWord(); }
	void ignoreScriptByte() { fetchScriptByte(); }
	virtual void getResultPos();
	void setResult(int result);
	void push(int a);
	int pop();
	virtual int readVar(uint var);
	virtual void writeVar(uint var, int value);
	
	void beginCutscene(int *args);
	void endCutscene();
	void abortCutscene();
	void beginOverride();
	void endOverride();

	void copyScriptString(byte *dst);
	int resStrLen(const byte *src) const;
	void doSentence(int c, int b, int a);
	void setStringVars(int i);

	/* Should be in Resource class */
	File _fileHandle;
	uint32 _fileOffset;
	int _resourceHeaderSize;
	int16 _resourceVersion;
	Common::String _gameName;	// This is the name we use for opening resource files
	Common::String _targetName;	// This is the game the user calls it, so use for saving
	bool _dynamicRoomOffsets;
	byte _resourceMapper[128];
	uint32 _allocatedSize;
	byte _expire_counter;
	byte *_HEV7RoomOffsets;

	void allocateArrays();
	void openRoom(int room);
	void closeRoom();
	void deleteRoomOffsets();
	void readRoomsOffsets();
	void askForDisk(const char *filename, int disknum);
	bool openResourceFile(const char *filename, byte encByte);

	void loadPtrToResource(int type, int i, const byte *ptr);
	void readResTypeList(int id, uint32 tag, const char *name);
	void allocResTypeData(int id, uint32 tag, int num, const char *name, int mode);
	byte *createResource(int type, int index, uint32 size);
	int loadResource(int type, int i);
	void nukeResource(int type, int i);	
	int getResourceSize(int type, int idx);


public:
	bool convertResourcesForAtari();
	bool isGlobInMemory(int type, int index) const;
	bool isResourceLoaded(int type, int index) const;
	byte *getResourceAddress(int type, int i);
	byte *getStringAddress(int i);
	byte *getStringAddressVar(int i);
	void ensureResourceLoaded(int type, int i);
	int getResourceRoomNr(int type, int index);

protected:
	int readSoundResource(int type, int index);
	int convert_extraflags(byte *ptr, byte * src_ptr);
	void convertMac0Resource(int type, int index, byte *ptr, int size);
	void convertADResource(int type, int index, byte *ptr, int size);
	void setResourceCounter(int type, int index, byte flag);
	bool validateResource(const char *str, int type, int index) const;
	void increaseResourceCounter();
	bool isResourceInUse(int type, int i) const;
	void initRoomSubBlocks();
	void clearRoomObjects();
	void loadRoomObjects();

	virtual void readArrayFromIndexFile();
	void readMAXS();
	void readIndexFile();
	void loadCharset(int i);
	void nukeCharset(int i);

	int _lastLoadedRoom;
public:
	const byte *findResourceData(uint32 tag, const byte *ptr);
	int getResourceDataSize(const byte *ptr) const;

protected:
	void resourceStats();
	void expireResources(uint32 size);
	void freeResources();

public:
	/* Should be in Object class */
	byte OF_OWNER_ROOM;
	int getInventorySlot();
	int findInventory(int owner, int index);
	int getInventoryCount(int owner);

protected:
	byte *_objectOwnerTable, *_objectRoomTable, *_objectStateTable;
	byte _numObjectsInRoom;

	void setupRoomObject(ObjectData *od, const byte *room, const byte *searchptr = NULL);
	void markObjectRectAsDirty(int obj);
	void loadFlObject(uint object, uint room);
	void nukeFlObjects(int min, int max);
	int findFlObjectSlot();
	int findLocalObjectSlot();
	void addObjectToInventory(uint obj, uint room);
	void fixObjectFlags();
public:
	bool getClass(int obj, int cls) const;		// Used in actor.cpp, hence public
protected:
	void putClass(int obj, int cls, bool set);
	int getState(int obj);
	void putState(int obj, int state);
	void setObjectState(int obj, int state, int x, int y);
	int getOwner(int obj) const;
	void putOwner(int obj, int owner);
	void setOwnerOf(int obj, int owner);
	void clearOwnerOf(int obj);
	int getObjectRoom(int obj) const;
	int getObjX(int obj);
	int getObjY(int obj);
	void getObjectXYPos(int object, int &x, int &y)	{ int dir; getObjectXYPos(object, x, y, dir); }
	void getObjectXYPos(int object, int &x, int &y, int &dir);
	int getObjOldDir(int obj);
	int getObjNewDir(int obj);
	int getObjectIndex(int object) const;
	int whereIsObject(int object) const;
	int findObject(int x, int y);
	void findObjectInRoom(FindObjectInRoom *fo, byte findWhat, uint object, uint room);	
public:
	int getObjectOrActorXY(int object, int &x, int &y);	// Used in actor.cpp, hence public
protected:
	int getObjActToObjActDist(int a, int b); // Not sure how to handle
	const byte *getObjOrActorName(int obj);		 // these three..

	void addObjectToDrawQue(int object);
	void clearDrawObjectQueue();
	void processDrawQue();

	uint32 getOBCDOffs(int object) const;
	byte *getOBCDFromObject(int obj);
	const byte *getOBIMFromObject(const ObjectData &od);
	const byte *getObjectImage(const byte *ptr, int state);

	int getDistanceBetween(bool is_obj_1, int b, int c, bool is_obj_2, int e, int f);


protected:
	/* Should be in Verb class */
	uint16 _verbMouseOver;
	int _inventoryOffset;
	int8 _userPut;
	uint16 _userState;

	void redrawVerbs();
	void checkExecVerbs();
	void verbMouseOver(int verb);
	int checkMouseOver(int x, int y) const;
	void drawVerb(int verb, int mode);
	void runInputScript(int a, int cmd, int mode);
	void restoreVerbBG(int verb);
	void drawVerbBitmap(int verb, int x, int y);
	int getVerbSlot(int id, int mode) const;
	void killVerb(int slot);
	void setVerbObject(uint room, uint object, uint verb);

public:
	void gotoRoom(uint room);

	/* Should be in Actor class */
#ifndef RELEASEBUILD
	Actor *derefActor(int id, const char *errmsg = 0) const;
	Actor *derefActorSafe(int id, const char *errmsg) const;
#else
	Actor *derefActorNoMsg(int id) const;
#endif

	uint32 *_classData;

	int getAngleFromPos(int x, int y) const;

protected:
	void walkActors();
	void playActorSounds();
	void setActorRedrawFlags();
	void showActors();
	void resetActorBgs();
	void processActors();
	void processUpperActors();
	int getActorFromPos(int x, int y);
	
	bool isCostumeInUse(int i) const;

public:
	/* Actor talking stuff */
	byte _actorToPrintStrFor, _V1_talkingActor;
	int _sentenceNum;
	SentenceTab _sentence[NUM_SENTENCE];
	StringTab _string[6];
	int16 _talkDelay;
	void actorTalk();
	void stopTalk();
	int talkingActor();		// Wrapper around VAR_TALK_ACTOR for V1 Maniac
	void talkingActor(int variable);

	// Costume class
	void cost_decodeData(Actor *a, int frame, uint usemask);
	int cost_frameToAnim(Actor *a, int frame);

protected:
	/* Should be in Graphics class? */
	uint16 _screenB, _screenH;
	int _roomHeight, _roomWidth;
public:
//	int SCREEN_HEIGHT, SCREEN_WIDTH;
	VirtScreen virtscr[4];		// Virtual screen areas
	CameraData camera;			// 'Camera' - viewport
	bool _fullRedraw, _BgNeedsRedraw, _verbRedraw, _verbColorsDirty;
	bool _screenEffectFlag, _completeScreenRedraw;

protected:
	ColorCycle _colorCycle[16];	// Palette cycles

	uint32 _ENCD_offs, _EXCD_offs;
	uint32 _CLUT_offs;
	uint32 _IM00_offs, _PALS_offs;

	struct {
		int16 hotspotX, hotspotY, width, height;
		byte animate, animateIndex;
		int8 state;
	} _cursor;

	uint32 _grabbedCursorId;	
	byte _grabbedCursor[8192];
	byte _currentCursor;
	byte _grabbedCursorTransp[16];

	byte _newEffect, _switchRoomEffect2, _switchRoomEffect;
	bool _doEffect;
	
	struct {
		int x, y, w, h;
		byte *buffer;
		uint16 xStrips, yStrips;
		bool isDrawn;
	} _flashlight;
	
public:
	bool isLightOn() const;
	
protected:
	void initScreens(int b, int h);
	void initVirtScreen(VirtScreenNumber slot, int number, int top, int width, int height, bool twobufs, bool scrollable);
	void initBGBuffers(int height);
	void initCycl(const byte *ptr);	// Color cycle

	void drawObject(int obj, int arg);	
	void drawRoomObjects(int arg);
	void drawRoomObject(int i, int arg);
	void drawBox(int x, int y, int x2, int y2, int color);

	void restoreCharsetBG(Common::Rect rect);
	void restoreBG(Common::Rect rect, byte backColor = 0);
	void redrawBGStrip(int start, int num, int16 y, int16 h);	
	void redrawBGAreas();	
	
	void cameraMoved();
	void setCameraAtEx(int at);
	virtual void setCameraAt(int pos_x, int pos_y);
	virtual void setCameraFollows(Actor *a);
	virtual void moveCamera();
	virtual void panCameraTo(int x, int y);
	void clampCameraPos(Common::Point *pt);
	void actorFollowCamera(int act);

	void mapSystemPalette(int start, int num);

	const byte *getPalettePtr(int palindex);
	void setPalette(int pal);
	void setPaletteFromPtr(const byte *ptr);
	void setPaletteFromRes();
	void setPalColor(int index, int r, int g, int b);
	void setDirtyColors(int min, int max);
	const byte *findPalInPals(const byte *pal, int index);
	void swapPalColors(int a, int b);
	void copyPalColor(int dst, int src);
	void cyclePalette();
	void stopCycle(int i);
	virtual void palManipulateInit(int resID, int start, int end, int time);
	void palManipulate();
public:
	byte getSystemPal(byte idx);

	int remapPaletteColor(int r, int g, int b, uint threshold);		// Used by Actor::remapActorPalette
protected:
	byte findClosestGameColor(int r, int g, int b);
	byte findClosestSystemColor(int r, int g, int b);

	void moveMemInPalRes(int start, int end, byte direction);
	void setupShadowPalette(int redScale, int greenScale, int blueScale, int startColor, int endColor);
	void darkenPalette(int redScale, int greenScale, int blueScale, int startColor, int endColor);

	void setCursor(int16 cursor);
	void setCursorImg(int16 img, int16 room, int16 imgindex, byte var = 0);
	void setCursorHotspot(int16 x, int16 y);
	void grabCursor(uint32 id, int16 x, int16 y, int16 width, int16 height);
	void grabCursor(uint32 id, byte *ptr, int16 width, int16 height, int16 pitch);
	void makeCursorColorTransparent(int16 a);
	void setupCursor();
	void decompressDefaultCursor(int16 index);
	void useIm01Cursor(uint32 id, const byte *im, int16 width, int16 height);
#ifdef ENGINE_SCUMM6
	void useBompCursor(uint32 id, const byte *im, int16 width, int16 height);
#endif


public:
	void markRectAsDirty(VirtScreenNumber virt, int left, int right, int top, int bottom, int dirtybit = 0);
	void markRectAsDirty(VirtScreenNumber virt, Common::Rect rect, int dirtybit = 0) {
		markRectAsDirty(virt, rect.left, rect.right, rect.top, rect.bottom, dirtybit);
	}
protected:
	void drawDirtyScreenParts();
	void updateDirtyScreen(VirtScreenNumber slot);

public:
	VirtScreen *findVirtScreen(int y);
	byte *getMaskBuffer(int x, int y, int z);
	bool hasMask(int x0, int y0, int x1, int y1, byte* ptr, int zbuf);

protected:
	void drawFlashlight();
	
	void fadeIn(int effect);
	void fadeOut(int effect);

	void unkScreenEffect5(int a);
	void unkScreenEffect6();
	void transitionEffect(int a);
	void dissolveEffect(int width, int height);
	void scrollEffect(int dir);

	void blit(byte *dst, const byte *src, int w, int h);


protected:
	bool _shakeEnabled;
	uint _shakeFrame;
	void setShake(int mode);

public:
	int _screenStartStrip, _screenEndStrip;
	int _screenLeft, _screenTop;


#ifdef ENGINE_SCUMM6
	// bomp
public:
	byte *_bompActorPalettePtr;
	void drawBomp(const BompDrawData &bd, bool mirror);
protected:
	int _blastObjectQueuePos; 
	BlastObject _blastObjectQueue[128];

	int _blastTextQueuePos;
	BlastText _blastTextQueue[35];	// FIXME - how many blast texts can there be at once? The Dig needs 33 for its end credits.

	void enqueueText(const byte *text, int x, int y, byte color, byte charset, bool center);
	void drawBlastTexts();
	void removeBlastTexts();

	void enqueueObject(int objectNumber, int objectX, int objectY, int objectWidth,
	                   int objectHeight, int scaleX, int scaleY, int image, int mode);
	void clearEnqueue() { _blastObjectQueuePos = 0; }

	void drawBlastObjects();
	void drawBlastObject(BlastObject *eo);
	void removeBlastObjects();
	void removeBlastObject(BlastObject *eo);
#endif //ENGINE_SCUMM6

protected:
	int _drawObjectQueNr;
	byte _drawObjectQue[200];
	
	/* For each of the 200 screen strips, gfxUsageBits contains a
	 * bitmask. The lower 30 bits each correspond to one actor and
	 * signify if any part of that actor is currently contained in
	 * that strip.
	 * 
	 * If the leftmost bit is set, the strip (background) is dirty
	 * needs to be redrawn.
	 * 
	 * The second leftmost bit is set by removeBlastObject() and
	 * restoreBG(), but I'm not yet sure why.
	 */
	enum {
		USAGE_BIT_DIRTY = 31,
		USAGE_BIT_RESTORED = 30,
	};

	uint32 gfxUsageBits[256];

	FORCEINLINE void setGfxUsageBit(int strip, int bit) {
		assert((bit >= 0) && (bit < 32));
		assert((strip >= 0) && (strip < 200));
		if (strip >= 0 && strip < 200)
			gfxUsageBits[strip] |= (1 << bit);
	}

	FORCEINLINE void clearGfxUsageBit(int strip, int bit) {
		assert((bit >= 0) && (bit < 32));
		assert((strip >= 0) && (strip < 200));
		if (strip >= 0 && strip < 200)
			gfxUsageBits[strip] &= ~(1 << bit);
	}

	FORCEINLINE bool testGfxUsageBit(int strip, int bit) {
		assert((bit >= 0) && (bit < 32));
		assert((strip >= 0) && (strip < 200));
		if (strip >= 0 && strip < 200)
			return (gfxUsageBits[strip] & (1 << bit)) != 0;
		return false;
	}

	FORCEINLINE bool testGfxAnyUsageBits(int strip) {
		assert((strip >= 0) && (strip < 200));
		return (gfxUsageBits[strip] & 0x3FFFFFFF) != 0;
	}

	FORCEINLINE bool testGfxOtherUsageBits(int strip, int bit) {
		assert((strip >= 0) && (strip < 200));
		return (gfxUsageBits[strip] & ~(1<<bit)) != 0;
	}

public:
	byte _roomPalette[256];

protected:
	byte _currentPalette[3 * 256];

	int _palDirtyMin, _palDirtyMax;

	byte _palManipStart, _palManipEnd;
	uint16 _palManipCounter;
	byte *_palManipPalette;
	byte *_palManipIntermediatePal;

	byte _haveMsg;
	bool _useTalkAnims;
	uint16 _defaultTalkDelay;
	int tempMusic;
	int _saveSound;
	bool _native_mt32;
	int _midiDriver; // Use the MD_ values from mididrv.h
	bool _copyProtection;
	bool _subtitles;

	Insane *_insane;

public:
	uint16 _extraBoxFlags[65];

	byte getNumBoxes();
	byte *getBoxMatrixBaseAddr();
	int getPathToDestBox(byte from, byte to);
	bool inBoxQuickReject(int box, int x, int y, int threshold);
	int getClosestPtOnBox(int box, int x, int y, int16& outX, int16& outY);
	int getSpecialBox(int param1, int param2);
	
	void setBoxFlags(int box, int val);
	void setBoxScale(int box, int b);

	bool checkXYInBoxBounds(int box, int x, int y);
	uint distanceFromPt(int x, int y, int ptx, int pty);
	void getBoxCoordinates(int boxnum, BoxCoords *bc);
	byte getMaskFromBox(int box);
	Box *getBoxBaseAddr(int box);
	byte getBoxFlags(int box);
	int getBoxScale(int box);

	int getScale(int box, int x, int y);

protected:
	// Scaling slots/items
	struct ScaleSlot {
		int x1, y1, scale1;
		int x2, y2, scale2;
	};
	ScaleSlot _scaleSlots[20];
	void setScaleSlot(int slot, int x1, int y1, int scale1, int x2, int y2, int scale2);
	void setBoxScaleSlot(int box, int slot);
	//void convertScaleTableToScaleSlot(int slot);

	void createBoxMatrix();
	bool areBoxesNeighbours(int i, int j);

	/* String class */
public:
	CharsetRenderer *_charset;
	byte _charsetColorMap[16];
protected:
	byte _charsetColor;
	byte _charsetData[15][16];

	int _charsetBufPos;
	byte _charsetBuffer[512];

protected:
	void initCharset(int charset);

	void CHARSET_1();
	void drawString(int a);
	const byte *addMessageToStack(const byte *msg);
	void addIntToStack(int var);
	void addVerbToStack(int var);
	void addNameToStack(int var);
	void addStringToStack(int var);
	void unkMessage1();
	void unkMessage2();
public:
	void clearMsgQueue();	// Used by Actor::putActor
protected:
	byte *_msgPtrToAdd;
	const byte *_messagePtr;
	bool _keepText;
public:
	Common::Language _language;
protected:

	const byte *translateTextAndPlaySpeech(const byte *ptr) { return ptr; }
	
#if defined(SCUMM_LITTLE_ENDIAN)
	uint32 fileReadDword() { return _fileHandle.readUint32LE(); }
#elif defined(SCUMM_BIG_ENDIAN)
	uint32 fileReadDword() { return _fileHandle.readUint32BE(); }
#endif

public:

	/* Scumm Vars */
	byte VAR_LANGUAGE;
	byte VAR_KEYPRESS;
	byte VAR_SYNC;
	byte VAR_EGO;
	byte VAR_CAMERA_POS_X;
	byte VAR_HAVE_MSG;
	byte VAR_ROOM;
	byte VAR_OVERRIDE;
	byte VAR_MACHINE_SPEED;
	byte VAR_ME;
	byte VAR_NUM_ACTOR;
	byte VAR_CURRENT_LIGHTS;
	byte VAR_CURRENTDRIVE;	// How about merging this with VAR_CURRENTDISK?
	byte VAR_CURRENTDISK;
	byte VAR_TMR_1;
	byte VAR_TMR_2;
	byte VAR_TMR_3;
	byte VAR_MUSIC_TIMER;
	byte VAR_ACTOR_RANGE_MIN;
	byte VAR_ACTOR_RANGE_MAX;
	byte VAR_CAMERA_MIN_X;
	byte VAR_CAMERA_MAX_X;
	byte VAR_TIMER_NEXT;
	byte VAR_VIRT_MOUSE_X;
	byte VAR_VIRT_MOUSE_Y;
	byte VAR_ROOM_RESOURCE;
	byte VAR_LAST_SOUND;
	byte VAR_CUTSCENEEXIT_KEY;
	byte VAR_OPTIONS_KEY;
	byte VAR_TALK_ACTOR;
	byte VAR_CAMERA_FAST_X;
	byte VAR_SCROLL_SCRIPT;
	byte VAR_ENTRY_SCRIPT;
	byte VAR_ENTRY_SCRIPT2;
	byte VAR_EXIT_SCRIPT;
	byte VAR_EXIT_SCRIPT2;
	byte VAR_VERB_SCRIPT;
	byte VAR_SENTENCE_SCRIPT;
	byte VAR_INVENTORY_SCRIPT;
	byte VAR_CUTSCENE_START_SCRIPT;
	byte VAR_CUTSCENE_END_SCRIPT;
	byte VAR_CHARINC;
	byte VAR_WALKTO_OBJ;
	byte VAR_DEBUGMODE;
	byte VAR_HEAPSPACE;
	byte VAR_RESTART_KEY;
	byte VAR_PAUSE_KEY;
	byte VAR_MOUSE_X;
	byte VAR_MOUSE_Y;
	byte VAR_TIMER;
	byte VAR_TMR_4;
	byte VAR_SOUNDCARD;
	byte VAR_VIDEOMODE;
	byte VAR_MAINMENU_KEY;
	byte VAR_FIXEDDISK;
	byte VAR_CURSORSTATE;
	byte VAR_USERPUT;
	byte VAR_SOUNDRESULT;
	byte VAR_TALKSTOP_KEY;
	byte VAR_NOSUBTITLES;

	byte VAR_SOUNDPARAM;
	byte VAR_SOUNDPARAM2;
	byte VAR_SOUNDPARAM3;
	byte VAR_MOUSEPRESENT;
	byte VAR_PERFORMANCE_1;
	byte VAR_PERFORMANCE_2;
	byte VAR_ROOM_FLAG;
	byte VAR_GAME_LOADED;
	byte VAR_NEW_ROOM;
	byte VAR_VERSION;

	byte VAR_V5_TALK_STRING_Y;

	byte VAR_V6_SCREEN_WIDTH;
	byte VAR_V6_SCREEN_HEIGHT;
	byte VAR_V6_EMSSPACE;

	byte VAR_CAMERA_POS_Y;

	byte VAR_CAMERA_MIN_Y;
	byte VAR_CAMERA_MAX_Y;
	byte VAR_CAMERA_THRESHOLD_X;
	byte VAR_CAMERA_THRESHOLD_Y;
	byte VAR_CAMERA_SPEED_X;
	byte VAR_CAMERA_SPEED_Y;
	byte VAR_CAMERA_ACCEL_X;
	byte VAR_CAMERA_ACCEL_Y;

	byte VAR_CAMERA_DEST_X;

	byte VAR_CAMERA_DEST_Y;

	byte VAR_CAMERA_FOLLOWED_ACTOR;

	byte VAR_LEFTBTN_DOWN;
	byte VAR_RIGHTBTN_DOWN;
	byte VAR_LEFTBTN_HOLD;
	byte VAR_RIGHTBTN_HOLD;
	byte VAR_MOUSE_BUTTONS;
	byte VAR_MOUSE_HOLD;
	byte VAR_SAVELOAD_SCRIPT;
	byte VAR_SAVELOAD_SCRIPT2;

	byte VAR_DEFAULT_TALK_DELAY;
	byte VAR_CHARSET_MASK;

	byte VAR_CUSTOMSCALETABLE;
	byte VAR_V6_SOUNDMODE;

	byte VAR_CHARCOUNT;
	byte VAR_VERB_ALLOWED;
	byte VAR_ACTIVE_VERB;
	byte VAR_ACTIVE_OBJECT1;
	byte VAR_ACTIVE_OBJECT2;
	byte VAR_CLICK_AREA;

	byte VAR_SCREENSAVER_TIME;
};

// This is a constant lookup table of reverse bit masks
extern const byte revBitMask[8];

/* Direction conversion functions (between old dir and new dir format) */
int newDirToOldDir(int dir);
int oldDirToNewDir(int dir);

int normalizeAngle(int angle);
int fromSimpleDir(int dirtype, int dir);
int toSimpleDir(int dirtype, int dir);
#ifndef RELEASEBUILD
void checkRange(int max, int min, int no, const char *str);
#endif

const char *tag2str(uint32 tag);

} // End of namespace Scumm


#if defined(RELEASEBUILD)
#define checkRange(max, min, no, str)
#define derefActor(id, ...)		derefActorNoMsg(id)
#define derefActorSafe(id, msg)	derefActorNoMsg(id)
#endif


#endif
