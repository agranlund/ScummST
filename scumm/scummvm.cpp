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
 * $Header: /cvsroot/scummvm/scummvm/scumm/Attic/scummvm.cpp,v 2.577.2.7 2004/03/11 03:37:43 ender Exp $
 *
 */

#include "stdafx.h"

#include "common/gameDetector.h"

#include "common/config-manager.h"

#include "gui/message.h"
#include "gui/newgui.h"

#include "scumm/actor.h"
#include "scumm/boxes.h"
#include "scumm/charset.h"
#include "scumm/costume.h"
#include "scumm/dialogs.h"
#include "scumm/imuse.h"
#include "scumm/intern.h"
#include "scumm/object.h"
#include "scumm/resource.h"
#include "scumm/scumm.h"
#include "scumm/sound.h"
#include "scumm/verbs.h"
#ifndef DISABLE_DEBUGGER
#include "scumm/debugger.h"
#endif

#include "sound/mididrv.h"
#include "sound/mixer.h"


#ifdef MACOSX
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef _WIN32_WCE
extern bool isSmartphone(void);
#endif


extern uint16 g_debugLevel;
extern bool g_slowMachine;

void SCUMM_Goto_Room(int aRoom) {
	Scumm::g_scumm->gotoRoom(aRoom);
}


namespace Scumm {

enum MouseButtonStatus {
	msDown = 1,
	msClicked = 2
};

// Use g_scumm from error() ONLY
ScummEngine *g_scumm = 0;

#ifndef RELEASEBUILD
void CDECL debugC(int channel, const char *s, ...)
{

	char buf[256];
	va_list va;

	// FIXME: Still spew all debug at -d9, for crashes in startup etc.
	//	  Add setting from commandline ( / abstract channel interface)
	if (!(g_scumm->_debugFlags & channel) && (g_debugLevel < 9))
		return;

	va_start(va, s);
	vsprintf(buf, s, va);
	va_end(va);

	debug(buf);
};
#endif

void ScummEngine::gotoRoom(uint room)
{
	_actors[VAR(VAR_EGO)].room = room;
	_sound->stopAllSounds();
	startScene(room, 0, 0);
	_fullRedraw = 1;	
}


const ScummGameSettings scumm_settings[] = {
	// Scumm version 5 
	{"monkey", "The Secret of Monkey Island", GID_MONKEY, 5, /*MDT_PCSPK |*/ MDT_ADLIB | MDT_NATIVE, GF_USE_KEY /*| GF_AUDIOTRACKS*/, 0},
	{"monkey2", "Monkey Island 2: LeChuck's revenge", GID_MONKEY2, 5, /*MDT_PCSPK |*/ MDT_ADLIB | MDT_NATIVE, GF_USE_KEY, 0},
	{"atlantis", "Indiana Jones and the Fate of Atlantis", GID_INDY4, 5, MDT_ADLIB | MDT_NATIVE, GF_USE_KEY, 0},
	// Scumm Version 6
	{"tentacle", "Day Of The Tentacle", GID_TENTACLE, 6, /*MDT_PCSPK |*/ MDT_ADLIB | MDT_NATIVE, GF_NEW_OPCODES | GF_USE_KEY, 0},
	{"samnmax", "Sam & Max: Hit the Road", GID_SAMNMAX, 6, /*MDT_PCSPK |*/ MDT_ADLIB | MDT_NATIVE, GF_NEW_OPCODES | GF_USE_KEY | GF_DRAWOBJ_OTHER_ORDER, 0},
	{NULL, NULL, 0, 0, MDT_NONE, 0, 0}
};


ScummEngine::ScummEngine(GameDetector *detector, OSystem *syst, const ScummGameSettings &gs)
	: Engine(syst),
	  _gameId(gs.id),
	  _version(gs.version),
	  _features(gs.features),
	  gdi(this), _pauseDialog(0), _mainMenuDialog(0),
	  _targetName(detector->_targetName) {

	//OSystem::Property prop;

	// Init all vars - maybe now we can get rid of our custom new/delete operators?
	_imuse = NULL;
	_musicEngine = NULL;
	_verbs = NULL;
	_objs = NULL;
#ifndef DISABLE_DEBUGGER
	_debugger = NULL;
#endif
	_debugFlags = 0;
	_sound = NULL;
	memset(&res, 0, sizeof(res));
	memset(&vm, 0, sizeof(vm));
	_quit = false;
	_pauseDialog = NULL;
	_mainMenuDialog = NULL;
	_fastMode = 0;
	_actors = NULL;
	_inventory = NULL;
	_newNames = NULL;
	_scummVars = NULL;
	_varwatch = 0;
	_bitVars = NULL;
	_numVariables = 0;
	_numBitVariables = 0;
	_numLocalObjects = 0;
	_numGlobalObjects = 0;
	_numArray = 0;
	_numVerbs = 0;
	_numFlObject = 0;
	_numInventory = 0;
	_numRooms = 0;
	_numScripts = 0;
	_numSounds = 0;
	_numCharsets = 0;
	_numNewNames = 0;
	_numGlobalScripts = 0;
	_numActors = 0;
	_numCostumes = 0;
	_audioNames = NULL;
	_numAudioNames = 0;
	_curActor = 0;
	_curVerb = 0;
	_curVerbSlot = 0;
	_curPalIndex = 0;
	_currentRoom = 0;
	_egoPositioned = false;
	_keyPressed = 0;
	_lastKeyHit = 0;
	_mouseButStat = 0;
	_leftBtnPressed = 0;
	_rightBtnPressed = 0;
	_bootParam = 0;
	_debugMode = 0;
	_objectOwnerTable = NULL;
	_objectRoomTable = NULL;
	_objectStateTable = NULL;
	_numObjectsInRoom = 0;
	_userPut = 0;
	_userState = 0;
	_resourceHeaderSize = 0;
	_resourceVersion = 0;
	_saveLoadFlag = 0;
	_saveLoadSlot = 0;
	_lastSaveTime = 0;
	_saveTemporaryState = false;
	memset(_saveLoadName, 0, sizeof(_saveLoadName));
	_maxHeapThreshold = 0;
	_minHeapThreshold = 0;
	memset(_localScriptList, 0, sizeof(_localScriptList));
	_scriptPointer = NULL;
	_scriptOrgPointer = NULL;
	_opcode = 0;
	vm.numNestedScripts = 0;
	_currentScript = 0;
	_curExecScript = 0;
	_lastCodePtr = NULL;
	_resultVarNumber = 0;
	_scummStackPos = 0;
	memset(_vmStack, 0, sizeof(_vmStack));
	_keyScriptKey = 0;
	_keyScriptNo = 0;
	_fileOffset = 0;
	_dynamicRoomOffsets = false;
	memset(_resourceMapper, 0, sizeof(_resourceMapper));
	_allocatedSize = 0;
	_expire_counter = 0;
	_lastLoadedRoom = 0;
	_roomResource = 0;
	OF_OWNER_ROOM = 0;
	_verbMouseOver = 0;
	_inventoryOffset = 0;
	_classData = NULL;
	_actorToPrintStrFor = 0;
	_sentenceNum = 0;
	memset(_sentence, 0, sizeof(_sentence));
	memset(_string, 0, sizeof(_string));
	_screenB = 0;
	_screenH = 0;
	_roomHeight = 0;
	_roomWidth = 0;
	memset(virtscr, 0, sizeof(virtscr));
	memset(&camera, 0, sizeof(CameraData));
	memset(_colorCycle, 0, sizeof(_colorCycle));
	_ENCD_offs = 0;
	_EXCD_offs = 0;
	_CLUT_offs = 0;
	_IM00_offs = 0;
	_PALS_offs = 0;
	_fullRedraw = false;
	_BgNeedsRedraw = false;
	_verbRedraw = false;
	_verbColorsDirty = false;
	_screenEffectFlag = false;
	_completeScreenRedraw = false;
	memset(&_cursor, 0, sizeof(_cursor));
	memset(_grabbedCursor, 0, sizeof(_grabbedCursor));
	memset(_grabbedCursorTransp, 0xFF, sizeof(_grabbedCursorTransp));
	_currentCursor = 0;
	_grabbedCursorId = 0;
	_newEffect = 0;
	_switchRoomEffect2 = 0;
	_switchRoomEffect = 0;
	_doEffect = false;
	memset(&_flashlight, 0, sizeof(_flashlight));
	_shakeEnabled= false;
	_shakeFrame = 0;
	_screenStartStrip = 0;
	_screenEndStrip = 0;
	_screenLeft = 0;
	_screenTop = 0;
#ifdef ENGINE_SCUMM6
	_bompActorPalettePtr = NULL;
	_blastObjectQueuePos = 0;
	memset(_blastObjectQueue, 0, sizeof(_blastObjectQueue));
	_blastTextQueuePos = 0;
	memset(_blastTextQueue, 0, sizeof(_blastTextQueue));
	_drawObjectQueNr = 0;
	memset(_drawObjectQue, 0, sizeof(_drawObjectQue));
#endif //ENGINE_SCUMM6
	_palManipStart = 0;
	_palManipEnd = 0;
	_palManipCounter = 0;
	_palManipPalette = NULL;
	_palManipIntermediatePal = NULL;
	memset(gfxUsageBits, 0, sizeof(gfxUsageBits));
	memset(_currentPalette, 0, sizeof(_currentPalette));
	_palDirtyMin = 0;
	_palDirtyMax = 0;
	_haveMsg = 0;
	_useTalkAnims = false;
	_defaultTalkDelay = 0;
	_midiDriver = MD_NULL;
	tempMusic = 0;
	_saveSound = 0;
	memset(_extraBoxFlags, 0, sizeof(_extraBoxFlags));
	memset(_scaleSlots, 0, sizeof(_scaleSlots));
	_charset = NULL;
	_charsetColor = 0;
	memset(_charsetColorMap, 0, sizeof(_charsetColorMap));
	memset(_charsetData, 0, sizeof(_charsetData));
	_charsetBufPos = 0;
	memset(_charsetBuffer, 0, sizeof(_charsetBuffer));
	_copyProtection = false;
	_msgPtrToAdd = NULL;
	_messagePtr = NULL;
	_talkDelay = 0;
	_keepText = false;
	_costumeRenderer = NULL;
	_V1_talkingActor = 0;

	//
	// Init all VARS to 0xFF
	//
	VAR_LANGUAGE = 0xFF;
	VAR_KEYPRESS = 0xFF;
	VAR_SYNC = 0xFF;
	VAR_EGO = 0xFF;
	VAR_CAMERA_POS_X = 0xFF;
	VAR_HAVE_MSG = 0xFF;
	VAR_ROOM = 0xFF;
	VAR_OVERRIDE = 0xFF;
	VAR_MACHINE_SPEED = 0xFF;
	VAR_ME = 0xFF;
	VAR_NUM_ACTOR = 0xFF;
	VAR_CURRENT_LIGHTS = 0xFF;
	VAR_CURRENTDRIVE = 0xFF;	// How about merging this with VAR_CURRENTDISK?
	VAR_CURRENTDISK = 0xFF;
	VAR_TMR_1 = 0xFF;
	VAR_TMR_2 = 0xFF;
	VAR_TMR_3 = 0xFF;
	VAR_MUSIC_TIMER = 0xFF;
	VAR_ACTOR_RANGE_MIN = 0xFF;
	VAR_ACTOR_RANGE_MAX = 0xFF;
	VAR_CAMERA_MIN_X = 0xFF;
	VAR_CAMERA_MAX_X = 0xFF;
	VAR_TIMER_NEXT = 0xFF;
	VAR_VIRT_MOUSE_X = 0xFF;
	VAR_VIRT_MOUSE_Y = 0xFF;
	VAR_ROOM_RESOURCE = 0xFF;
	VAR_LAST_SOUND = 0xFF;
	VAR_CUTSCENEEXIT_KEY = 0xFF;
	VAR_OPTIONS_KEY = 0xFF;
	VAR_TALK_ACTOR = 0xFF;
	VAR_CAMERA_FAST_X = 0xFF;
	VAR_SCROLL_SCRIPT = 0xFF;
	VAR_ENTRY_SCRIPT = 0xFF;
	VAR_ENTRY_SCRIPT2 = 0xFF;
	VAR_EXIT_SCRIPT = 0xFF;
	VAR_EXIT_SCRIPT2 = 0xFF;
	VAR_VERB_SCRIPT = 0xFF;
	VAR_SENTENCE_SCRIPT = 0xFF;
	VAR_INVENTORY_SCRIPT = 0xFF;
	VAR_CUTSCENE_START_SCRIPT = 0xFF;
	VAR_CUTSCENE_END_SCRIPT = 0xFF;
	VAR_CHARINC = 0xFF;
	VAR_CHARCOUNT = 0xFF;
	VAR_WALKTO_OBJ = 0xFF;
	VAR_DEBUGMODE = 0xFF;
	VAR_HEAPSPACE = 0xFF;
	VAR_RESTART_KEY = 0xFF;
	VAR_PAUSE_KEY = 0xFF;
	VAR_MOUSE_X = 0xFF;
	VAR_MOUSE_Y = 0xFF;
	VAR_TIMER = 0xFF;
	VAR_TMR_4 = 0xFF;
	VAR_SOUNDCARD = 0xFF;
	VAR_VIDEOMODE = 0xFF;
	VAR_MAINMENU_KEY = 0xFF;
	VAR_FIXEDDISK = 0xFF;
	VAR_CURSORSTATE = 0xFF;
	VAR_USERPUT = 0xFF;
	VAR_SOUNDRESULT = 0xFF;
	VAR_TALKSTOP_KEY = 0xFF;
	VAR_NOSUBTITLES = 0xFF;

	VAR_SOUNDPARAM = 0xFF;
	VAR_SOUNDPARAM2 = 0xFF;
	VAR_SOUNDPARAM3 = 0xFF;
	VAR_MOUSEPRESENT = 0xFF;
	VAR_PERFORMANCE_1 = 0xFF;
	VAR_PERFORMANCE_2 = 0xFF;
	VAR_ROOM_FLAG = 0xFF;
	VAR_GAME_LOADED = 0xFF;
	VAR_NEW_ROOM = 0xFF;
	VAR_VERSION = 0xFF;

	VAR_V5_TALK_STRING_Y = 0xFF;

	VAR_V6_SCREEN_WIDTH = 0xFF;
	VAR_V6_SCREEN_HEIGHT = 0xFF;
	VAR_V6_EMSSPACE = 0xFF;

	VAR_CAMERA_POS_Y = 0xFF;

	VAR_CAMERA_MIN_Y = 0xFF;
	VAR_CAMERA_MAX_Y = 0xFF;
	VAR_CAMERA_THRESHOLD_X = 0xFF;
	VAR_CAMERA_THRESHOLD_Y = 0xFF;
	VAR_CAMERA_SPEED_X = 0xFF;
	VAR_CAMERA_SPEED_Y = 0xFF;
	VAR_CAMERA_ACCEL_X = 0xFF;
	VAR_CAMERA_ACCEL_Y = 0xFF;

	VAR_CAMERA_DEST_X = 0xFF;

	VAR_CAMERA_DEST_Y = 0xFF;

	VAR_CAMERA_FOLLOWED_ACTOR = 0xFF;

	VAR_LEFTBTN_DOWN = 0xFF;
	VAR_RIGHTBTN_DOWN = 0xFF;
	VAR_LEFTBTN_HOLD = 0xFF;
	VAR_RIGHTBTN_HOLD = 0xFF;
	VAR_MOUSE_BUTTONS = 0xFF;
	VAR_MOUSE_HOLD = 0xFF;
	VAR_SAVELOAD_SCRIPT = 0xFF;
	VAR_SAVELOAD_SCRIPT2 = 0xFF;

	VAR_DEFAULT_TALK_DELAY = 0xFF;
	VAR_CHARSET_MASK = 0xFF;

	VAR_CUSTOMSCALETABLE = 0xFF;
	VAR_V6_SOUNDMODE = 0xFF;

	VAR_ACTIVE_VERB = 0xFF;
	VAR_ACTIVE_OBJECT1 = 0xFF;
	VAR_ACTIVE_OBJECT2 = 0xFF;
	VAR_VERB_ALLOWED = 0xFF;
	VAR_CLICK_AREA = 0xFF;

	// Use g_scumm from error() ONLY
	g_scumm = this;

	// Read settings from the detector & config manager
	_debugMode = GetConfig(kConfig_DebugLevel);
	_bootParam = 0;

	// Allow the user to override the game name with a custom string.
	// This allows some game versions to work which use filenames
	// differing from the regular version(s) of that game.
	_gameName = gs.baseFilename ? gs.baseFilename : gs.name;

	_midiDriver = GameDetector::detectMusicDriver(gs.midi);

	_copyProtection = false;
	_subtitles = GetConfig(kConfig_Subtitles);

	_defaultTalkDelay = 60;
	

	_native_mt32 = false;
	if (_midiDriver == MD_STMIDI_MT32)
		_native_mt32 = true;


	_language = (Common::Language) GetConfig(kConfig_Language);
	memset(&res, 0, sizeof(res));

	// Initialize backend
	syst->init_size(SCREEN_WIDTH, SCREEN_HEIGHT);
	//prop.cd_num = 0;

	_sound = new Sound(this);

	/* Bind the mixer to the system => mixer will be invoked
	 * automatically when samples need to be generated */
	if (!_mixer->isReady()) {
		//warning("Sound mixer initialization failed");
		if (_midiDriver == MD_ADLIB ) {
			_midiDriver = MD_NULL;
			//warning("MIDI driver depends on sound mixer, switching to null MIDI driver");
		}
	}
	_mixer->setVolume(GetConfig(kConfig_SfxVolume) * GetConfig(kConfig_MasterVolume) / 255);
	_mixer->setMusicVolume(GetConfig(kConfig_MusicVolume));

	// Init iMuse
	MidiDriver *driver = GameDetector::createMidi(_midiDriver);
	if (driver && _native_mt32)
		driver->property (MidiDriver::PROP_CHANNEL_MASK, 0x03FE);
	_musicEngine = _imuse = IMuse::create(syst, _mixer, driver);
	if (_imuse) {
		_imuse->property(IMuse::PROP_OLD_ADLIB_INSTRUMENTS, 0);
		_imuse->property(IMuse::PROP_MULTI_MIDI, GetConfig(kConfig_MultiMidi) && _midiDriver != MD_NULL && (gs.midi & MDT_ADLIB));
		_imuse->property(IMuse::PROP_NATIVE_MT32, _native_mt32);
		_imuse->set_music_volume(GetConfig(kConfig_MusicVolume));
	}

	// Create the charset renderer
	_charset = new CharsetRenderer(this);

	// Create the costume renderer
	_costumeRenderer = new CostumeRenderer(this);
}

ScummEngine::~ScummEngine() {
	if (_musicEngine) {
		_musicEngine->terminate();
		delete _musicEngine;
	}

	_mixer->stopAll();

	delete [] _actors;

	delete _charset;
	delete _pauseDialog;
	delete _mainMenuDialog;

	delete _sound;
	free(_audioNames);

	delete _costumeRenderer;

	freeResources();

	free(_objectStateTable);
	free(_objectRoomTable);
	free(_objectOwnerTable);
	free(_inventory);
	free(_verbs);
	free(_objs);
	free(_scummVars);
	free(_bitVars);
	free(_newNames);
	free(_classData);
}

void ScummEngine::go() {
	launch();
	mainRun();
}

#pragma mark -
#pragma mark --- Initialization ---
#pragma mark -

void ScummEngine::launch() {


	// TEMP -- not sure where this will go
	convertResourcesForAtari();
	// TEMP -- not sure where this will go


	_maxHeapThreshold = 550000;
	_minHeapThreshold = 400000;

	_verbRedraw = false;

	allocResTypeData(rtBuffer, MKID('NONE'), 10, "buffer", 0);

	setupScummVars();

	setupOpcodes();

	_numActors = MAX_NUM_ACTORS;

	OF_OWNER_ROOM = 0x0F;

	// if (_gameId==GID_MONKEY2 && _bootParam == 0)
	//	_bootParam = 10001;

	if (_gameId == GID_INDY4 && _bootParam == 0) {
		_bootParam = -7873;
	}

	_resourceHeaderSize = 8;

	readIndexFile();

	if (_resourceVersion != ATARI_RESOURCE_VERSION)
	{
		const char* msg = "Incompatible data, please reinstall.";
		InfoDialog dialog(this, msg);
		runDialog(dialog);
		g_system->quit();
	}

	scummInit();

	if (_version > 2) {
		if (_version < 7)
			VAR(VAR_VERSION) = 21;
	}

	if (_gameId == GID_MONKEY)
		_scummVars[74] = 1225;

	if (_imuse) {
		_imuse->setBase(res.address[rtSound]);

		_imuse->setMasterVolume(GetConfig(kConfig_MasterVolume));
		_imuse->set_music_volume(GetConfig(kConfig_MusicVolume));
	}
	_sound->setupSound();

	// Create debugger
#ifndef DISABLE_DEBUGGER
	if (!_debugger)
		_debugger = new ScummDebugger(this);
#endif

	// If requested, load a save game instead of running the boot script
	if (_saveLoadFlag != 2 || !loadState(_saveLoadSlot, _saveTemporaryState)) {
		int args[16];
		memset(args, 0, sizeof(args));
		args[0] = _bootParam;	

		_saveLoadFlag = 0;
		runScript(1, 0, 0, args);
	} else {
		_saveLoadFlag = 0;
	}
}

void ScummEngine::scummInit() {
	int i;

	tempMusic = 0;
	debug(9, "scummInit");

	initScreens(16, 144);

	for (i = 0; i < 256; i++)
		_roomPalette[i] = i;

	loadCharset(1);
	setShake(0);
	setupCursor();
	
	// Allocate and Initialize actors
	Actor::initActorClass(this);
	_actors = new Actor[_numActors];
	for (i = 0; i < _numActors; i++) {
		_actors[i].number = i;
		_actors[i].initActor(1);
	}

	vm.numNestedScripts = 0;
	vm.cutSceneStackPointer = 0;

	memset(vm.cutScenePtr, 0, sizeof(vm.cutScenePtr));
	memset(vm.cutSceneData, 0, sizeof(vm.cutSceneData));

	for (i = 0; i < _numVerbs; i++) {
		_verbs[i].verbid = 0;
		_verbs[i].curRect.right = SCREEN_WIDTH - 1;
		_verbs[i].oldRect.left = -1;
		_verbs[i].type = 0;
		_verbs[i].color = 2;
		_verbs[i].hicolor = 0;
		_verbs[i].charset_nr = 1;
		_verbs[i].curmode = 0;
		_verbs[i].saveid = 0;
		_verbs[i].center = 0;
		_verbs[i].key = 0;
	}

	camera._leftTrigger = 10;
	camera._rightTrigger = 30;
	camera._mode = 0;
	camera._follows = 0;

	virtscr[0].xstart = 0;

	if (VAR_CURRENT_LIGHTS != 0xFF) {
		// Setup light
		_flashlight.xStrips = 7;
		_flashlight.yStrips = 7;
		_flashlight.buffer = NULL;
	}

	_mouse.x = 104;
	_mouse.y = 56;

	_ENCD_offs = 0;
	_EXCD_offs = 0;

	_currentScript = 0xFF;
	_sentenceNum = 0;

	_currentRoom = 0;
	_numObjectsInRoom = 0;
	_actorToPrintStrFor = 0;

	_charsetBufPos = 0;
	_haveMsg = 0;

	_varwatch = -1;
	_screenStartStrip = 0;

	_talkDelay = 0;
	_keepText = false;

	_currentCursor = 0;
	_cursor.state = 0;
	_userPut = 0;

	_newEffect = 129;
	_fullRedraw = true;

	clearDrawObjectQueue();

	for (i = 0; i < 6; i++) {
		_string[i].t_xpos = 2;
		_string[i].t_ypos = 5;
		_string[i].t_right = SCREEN_WIDTH - 1;
		_string[i].t_color = 0xF;
		_string[i].t_center = 0;
		_string[i].t_charset = 0;
	}

	initScummVars();

	_lastSaveTime = _system->get_msecs();
}


void ScummEngine::initScummVars() {

	if (_version < 6)
		VAR(VAR_V5_TALK_STRING_Y) = -0x50;

	VAR(VAR_CURRENTDRIVE) = 0;
	VAR(VAR_FIXEDDISK) = true;
	switch (_midiDriver) {
	case MD_NULL:  VAR(VAR_SOUNDCARD) = 0; break;
	case MD_ADLIB: VAR(VAR_SOUNDCARD) = 3; break;
	case MD_PCSPK:
	case MD_PCJR:  VAR(VAR_SOUNDCARD) = 1; break;
	default:       
		VAR(VAR_SOUNDCARD) = 3;
	}

	if (g_slowMachine)
	{
		VAR(VAR_MACHINE_SPEED) = 1;	// slow machine
#if defined(GAME_MONKEY2)
		VAR(VAR_VIDEOMODE) = 13;	// EGA
#else
		VAR(VAR_VIDEOMODE) = 19;	// VGA
#endif
	}
	else
	{
		VAR(VAR_MACHINE_SPEED) = 2;	// fast machine
		VAR(VAR_VIDEOMODE) = 19;	// VGA
	}

	VAR(VAR_HEAPSPACE) = 1400;
	VAR(VAR_MOUSEPRESENT) = true; // FIXME - used to be 0, but that seems odd?!?
	VAR(VAR_SOUNDPARAM) = 0;
	VAR(VAR_SOUNDPARAM2) = 0;
	VAR(VAR_SOUNDPARAM3) = 0;
	if (_version == 6 && VAR_V6_EMSSPACE != 0xFF)
		VAR(VAR_V6_EMSSPACE) = 10000;

	VAR(59) = 3;	// FIXME: What is this good for?
	
	if (VAR_CURRENT_LIGHTS != 0xFF) {
		// Setup light
		VAR(VAR_CURRENT_LIGHTS) = LIGHTMODE_actor_base | LIGHTMODE_actor_color | LIGHTMODE_screen;
	}
	
	VAR(VAR_CHARINC) = 4;
	talkingActor(0);
}

#pragma mark -
#pragma mark --- Main loop ---
#pragma mark -

void ScummEngine::mainRun() {
	int delta = 0;
	int diff = _system->get_msecs();

	while (!_quit) {

		if (_mixer)
			_mixer->collectGarbage();

		updatePalette();
		_system->update_screen();

		diff -= _system->get_msecs();
		waitForTimer(delta * 15 + diff);
		diff = _system->get_msecs();
		delta = scummLoop(delta);

		if (delta < 1)	// Ensure we don't get into a loop
			delta = 1;  // by not decreasing sleepers.

		if (_quit) {
			confirmexitDialog();
		}
	}
}

void ScummEngine::waitForTimer(int msec_delay) {
	uint32 start_time;

	if (_fastMode & 2)
		msec_delay = 0;
	else if (_fastMode & 1)
		msec_delay = 10;

	uint32 end_time = _system->get_msecs();
	if (msec_delay >= 0)
		end_time += msec_delay;

	while (!_quit)
	{
		parseEvents();

		uint32 now_time = _system->get_msecs();
		if (now_time >= end_time)
			break;

		uint32 d = end_time - now_time;
		if (d > 10) d = 10;
		_system->delay_msecs(d);
	}
}

int ScummEngine::scummLoop(int delta) {
#ifndef DISABLE_DEBUGGER
	if (_debugger->isAttached())
		_debugger->onFrame();
#endif

	// Randomize the PRNG by calling it at regular intervals. This ensures
	// that it will be in a different state each time you run the program.
	_rnd.getRandomNumber(2);

	if (_version > 2) {
		VAR(VAR_TMR_1) += delta;
		VAR(VAR_TMR_2) += delta;
		VAR(VAR_TMR_3) += delta;
	}
	if (VAR_TMR_4 != 0xFF)
		VAR(VAR_TMR_4) += delta;

	//if (delta > 15)
	//	delta = 15;

	if (delta > 60)
		delta = 60;

	decreaseScriptDelay(delta);

	// If _talkDelay is -1, that means the text should never time out.
	// This is used for drawing verb texts, e.g. the Full Throttle
	// dialogue choices.

	if (_talkDelay != -1) {
		_talkDelay -= delta;
		if (_talkDelay < 0)
			_talkDelay = 0;
	}

	// Record the current ego actor before any scripts (including input scripts)
	// get a chance to run.
//	int oldEgo = 0;
//	if (VAR_EGO != 0xFF)
//		oldEgo = VAR(VAR_EGO);

	processKbd(false);

	VAR(VAR_CAMERA_POS_X) = camera._cur.x;

	VAR(VAR_HAVE_MSG) = (_haveMsg == 0xFE) ? 0xFF : _haveMsg;
	VAR(VAR_VIRT_MOUSE_X) = _virtualMouse.x;
	VAR(VAR_VIRT_MOUSE_Y) = _virtualMouse.y;
	VAR(VAR_MOUSE_X) = _mouse.x;
	VAR(VAR_MOUSE_Y) = _mouse.y;

	if (_features & GF_AUDIOTRACKS) {
		// Covered automatically by the Sound class
	} else 	if (_musicEngine && VAR_MUSIC_TIMER != 0xFF) {
		// The music engine generates the timer data for us.
		VAR(VAR_MUSIC_TIMER) = _musicEngine->getMusicTimer();
	}

	// Trigger autosave all 5 minutes.
	/*
	if (!_saveLoadFlag && _system->get_msecs() > _lastSaveTime + 5 * 60 * 1000) {
		_saveLoadSlot = 0;
		sprintf(_saveLoadName, "Autosave %d", _saveLoadSlot);
		_saveLoadFlag = 1;
		_saveTemporaryState = false;
	}
	*/

	if (VAR_GAME_LOADED != 0xFF)
		VAR(VAR_GAME_LOADED) = 0;
	if (_saveLoadFlag) {
load_game:
		bool success;
		const char *errMsg = 0;
		char filename[256];

		if (_saveLoadFlag == 1) {
			success = saveState(_saveLoadSlot, _saveTemporaryState);
			if (!success)
				errMsg = "Failed to save game\n\n";

			if (success && _saveTemporaryState)
				VAR(VAR_GAME_LOADED) = 201;
		} else {
			success = loadState(_saveLoadSlot, _saveTemporaryState);
			if (!success)
				errMsg = "Failed to load game\n\n";

			if (success && _saveTemporaryState)
				VAR(VAR_GAME_LOADED) = 203;
		}

		makeSavegameName(filename, _saveLoadSlot, _saveTemporaryState);
		if (!success) {
			displayError(0, errMsg);
		} else if (_saveLoadFlag == 1 && _saveLoadSlot != 0 && !_saveTemporaryState) {
			// Display "Save successful" message, except for auto saves
			const char* msg = "Successfully saved game\n\n";
	
			GUI::TimedMessageDialog dialog(msg, 1500);
			runDialog(dialog);
		}
		if (success && _saveLoadFlag != 1)
			clearClickedStatus();

		_saveLoadFlag = 0;
		_lastSaveTime = _system->get_msecs();
	}

	if (_verbColorsDirty)
	{
		debug(2, "Redraw _verbColorsDirty");
		_verbColorsDirty = false;
		debug(2, "%d,%d, %d,%d", virtscr[0].topline, virtscr[0].height, virtscr[2].topline, virtscr[2].height);
		if ((virtscr[kMainVirtScreen].height < SCREEN_HEIGHT) && (virtscr[kVerbVirtScreen].topline > 0) && (virtscr[kVerbVirtScreen].height < SCREEN_HEIGHT))
		{
			for (int i = 0; i < _numVerbs; i++)
				drawVerb(i, 0);
			verbMouseOver(0);
			_verbRedraw = false;
		}
	}

	if (_completeScreenRedraw) {
		debug(2, "Redraw _completeScreenRedraw");
		_completeScreenRedraw = false;
		_charset->clearCharsetMask();
		_charset->_hasMask = false;

		for (int i = 0; i < _numVerbs; i++)
			drawVerb(i, 0);

		verbMouseOver(0);

		_verbRedraw = false;
		_fullRedraw = true;
	}

	runAllScripts();
	checkExecVerbs();
	checkAndRunSentenceScript();

	if (_quit)
		return 0;

	// HACK: If a load was requested, immediately perform it. This avoids
	// drawing the current room right after the load is request but before
	// it is performed. That was annoying esp. if you loaded while a SMUSH
	// cutscene was playing.
	if (_saveLoadFlag && _saveLoadFlag != 1) {
		goto load_game;
	}
	
	static uint32 framenum = 0;
	framenum++;
	debug(2, "-- frame %d start -- %d,%d,%d\n\r", framenum, _fullRedraw, _BgNeedsRedraw, _verbRedraw);

	if (_currentRoom == 0) {
		CHARSET_1();
		drawDirtyScreenParts();
	} else {
		walkActors();
		moveCamera();
		fixObjectFlags();
		CHARSET_1();

		if (camera._cur.x != camera._last.x || _BgNeedsRedraw || _fullRedraw) {
			redrawBGAreas();
		}

		processDrawQue();

		if (_verbRedraw) {
			redrawVerbs();
		}
	
		setActorRedrawFlags();
		resetActorBgs();

		if (VAR_CURRENT_LIGHTS != 0xFF &&
		    !(VAR(VAR_CURRENT_LIGHTS) & LIGHTMODE_screen) &&
		      VAR(VAR_CURRENT_LIGHTS) & LIGHTMODE_flashlight) {
			drawFlashlight();
			setActorRedrawFlags();
		}

		processActors();
		_fullRedraw = false;
		cyclePalette();
		palManipulate();

		if (_doEffect) {
			_doEffect = false;
			fadeIn(_newEffect);
			clearClickedStatus();
		}


		if (!_verbRedraw && _cursor.state > 0) {
			verbMouseOver(checkMouseOver(_mouse.x, _mouse.y));
		}
		_verbRedraw = false;

#ifdef ENGINE_SCUMM6
		drawBlastObjects();
		drawBlastTexts();
#endif

		drawDirtyScreenParts();

#ifdef ENGINE_SCUMM6
		removeBlastTexts();
		removeBlastObjects();
#endif

		if (_version == 5)
			playActorSounds();
	}

	debug(2, "-- frame end --\n\r");

	_sound->processSoundQues();
	camera._last = camera._cur;

	if (!(++_expire_counter)) {
		increaseResourceCounter();
	}

	animateCursor();
	
	/* show or hide mouse */
	_system->show_mouse(_cursor.state > 0);

	if (VAR_TIMER != 0xFF)
		VAR(VAR_TIMER) = 0;
	return VAR(VAR_TIMER_NEXT);

}

#pragma mark -
#pragma mark --- Events / Input ---
#pragma mark -

void ScummEngine::parseEvents()
{
	OSystem::Event event;
	while (_system->poll_event(&event))
	{
		switch(event.event_code)
		{
		case OSystem::EVENT_KEYDOWN:
#ifndef __ATARI__
			if (event.kbd.keycode >= '0' && event.kbd.keycode <= '9'
				&& (event.kbd.flags == OSystem::KBD_ALT ||
					event.kbd.flags == OSystem::KBD_CTRL)) {
				_saveLoadSlot = event.kbd.keycode - '0';

				//  don't overwrite autosave (slot 0)
				if (_saveLoadSlot == 0)
					_saveLoadSlot = 10;

				sprintf(_saveLoadName, "Quicksave %d", _saveLoadSlot);
				_saveLoadFlag = (event.kbd.flags == OSystem::KBD_ALT) ? 1 : 2;
				_saveTemporaryState = false;
			}
			else if (event.kbd.flags == OSystem::KBD_CTRL)
			{
				if (event.kbd.keycode == 't')
				{
					SCUMM_Goto_Room(27);	// ball of twine
					//SCUMM_Goto_Room(37);	// vortex outside
					//SCUMM_Goto_Room(79);	// vortex snow globe
				}
				else if (event.kbd.keycode == 'v')
					VAR(VAR_SCREENSAVER_TIME) = 0;
				else if (event.kbd.keycode == 'f')
					_fastMode ^= 1;
				else if (event.kbd.keycode == 'g')
					_fastMode ^= 2;
#ifndef DISABLE_DEBUGGER
				else if (event.kbd.keycode == 'd')
					_debugger->attach();
#endif
				else if (event.kbd.keycode == 's')
					resourceStats();
				else
					_keyPressed = event.kbd.ascii;	// Normal key press, pass on to the game.
			}
			else if (event.kbd.flags & OSystem::KBD_ALT)
			{
				// The result must be 273 for Alt-W
				// because that's what MI2 looks for in
				// its "instant win" cheat.
				_keyPressed = event.kbd.keycode + 154;
			}
			else
#endif
			if (_gameId == GID_INDY4 && event.kbd.ascii >= '0' && event.kbd.ascii <= '9')
			{
				// To support keyboard fighting in FOA, we need to remap the number keys.
				// FOA apparently expects PC scancode values (see script 46 if you want
				// to know where I got these numbers from).
				static const int numpad[10] = {
						'0',
						335, 336, 337,
						331, 332, 333,
						327, 328, 329
					};
				_keyPressed = numpad[event.kbd.ascii - '0'];
			} else if (event.kbd.ascii < 273 || event.kbd.ascii > 276) {
				// don't let game have arrow keys as we currently steal them
				// for keyboard cursor control
				// this fixes bug with up arrow (273) corresponding to
				// "instant win" cheat in MI2 mentioned above
				//
				// This is not applicable to Full Throttle as it processes keyboard
				// cursor control by itself. Also it fixes derby scene
				_keyPressed = event.kbd.ascii;	// Normal key press, pass on to the game.
			}

			break;

		case OSystem::EVENT_KEYUP:
			break;

		case OSystem::EVENT_MOUSEMOVE:
			_mouse.x = event.mouse.x;
			_mouse.y = event.mouse.y;
			break;

		case OSystem::EVENT_LBUTTONDOWN:
			_leftBtnPressed |= msClicked|msDown;
#if defined(_WIN32_WCE) || defined(__PALM_OS__)
			_mouse.x = event.mouse.x;
			_mouse.y = event.mouse.y;
#endif
			break;

		case OSystem::EVENT_RBUTTONDOWN:
			_rightBtnPressed |= msClicked|msDown;
#if defined(_WIN32_WCE) || defined(__PALM_OS__)
			_mouse.x = event.mouse.x;
			_mouse.y = event.mouse.y;
#endif
			break;

		case OSystem::EVENT_LBUTTONUP:
			_leftBtnPressed &= ~msDown;
			break;

		case OSystem::EVENT_RBUTTONUP:
			_rightBtnPressed &= ~msDown;
			break;
	
		case OSystem::EVENT_QUIT:
			//confirmexitDialog();
			_quit = true;
			break;
	
		default:
			break;
		}
	}
}

void ScummEngine::clearClickedStatus() {
	_keyPressed = 0;
	_mouseButStat = 0;
	_leftBtnPressed &= ~msClicked;
	_rightBtnPressed &= ~msClicked;
}

void ScummEngine::processKbd(bool smushMode) {
	int saveloadkey;

	_lastKeyHit = _keyPressed;
	_keyPressed = 0;
	
	//
	// Clip the mouse coordinates, and compute _virtualMouse.x (and clip it, too)
	//
	if (_mouse.x < 0)
		_mouse.x = 0;
	if (_mouse.x > SCREEN_WIDTH-1)
		_mouse.x = SCREEN_WIDTH-1;
	if (_mouse.y < 0)
		_mouse.y = 0;
	if (_mouse.y > SCREEN_HEIGHT-1)
		_mouse.y = SCREEN_HEIGHT-1;

	_virtualMouse.x = _mouse.x + virtscr[0].xstart;
	_virtualMouse.y = _mouse.y - virtscr[0].topline;

	if (_virtualMouse.y < 0)
		_virtualMouse.y = -1;
	if (_virtualMouse.y >= virtscr[0].height)
		_virtualMouse.y = -1;

	//
	// Determine the mouse button state.
	//
	_mouseButStat = 0;

	// Interpret 'return' as left click and 'tab' as right click
	if (_lastKeyHit && _cursor.state > 0) {
		if (_lastKeyHit == 9) {
			_mouseButStat = MBS_RIGHT_CLICK;
			_lastKeyHit = 0;
		} else if (_lastKeyHit == 13) {
			_mouseButStat = MBS_LEFT_CLICK;
			_lastKeyHit = 0;
		}
	}

	if (_leftBtnPressed & msClicked && _rightBtnPressed & msClicked && _version > 3) {
		// Pressing both mouse buttons is treated as if you pressed
		// the cutscene exit key (i.e. ESC in most games). That mimicks
		// the behaviour of the original engine where pressing both
		// mouse buttons also skips the current cutscene.
		_mouseButStat = 0;
		_lastKeyHit = (uint)VAR(VAR_CUTSCENEEXIT_KEY);
	} else if (_leftBtnPressed & msClicked) {
		_mouseButStat = MBS_LEFT_CLICK;
	} else if (_rightBtnPressed & msClicked) {
		_mouseButStat = MBS_RIGHT_CLICK;
	}

	_leftBtnPressed &= ~msClicked;
	_rightBtnPressed &= ~msClicked;

	if (!_lastKeyHit)
		return;

/*
	// If a key script was specified (a V8 feature), and it's trigger
	// key was pressed, run it.
	if (_keyScriptNo && (_keyScriptKey == _lastKeyHit)) {
		runScript(_keyScriptNo, 0, 0, 0);
		return;
	}
*/

	if (VAR_RESTART_KEY != 0xFF && _lastKeyHit == VAR(VAR_RESTART_KEY)) {
		confirmrestartDialog();
		return;
	}

	if ((VAR_PAUSE_KEY != 0xFF && _lastKeyHit == VAR(VAR_PAUSE_KEY)) ||
		(VAR_PAUSE_KEY == 0xFF && _lastKeyHit == ' ')) {
		pauseGame();
		return;
	}

	if (_gameId == GID_SAMNMAX)
		saveloadkey = 319;	// F5
	else
		saveloadkey = VAR(VAR_MAINMENU_KEY);

	if (_lastKeyHit == VAR(VAR_CUTSCENEEXIT_KEY) ||
		(VAR(VAR_CUTSCENEEXIT_KEY) == 4 && _lastKeyHit == 27)) {
		// Skip cutscene (or active SMUSH video). For the V2 games, which
		// normally use F4 for this, we add in a hack that makes escape work,
		// too (just for convenience).
		abortCutscene();

	} else if (_lastKeyHit == saveloadkey) {
		if (VAR_SAVELOAD_SCRIPT != 0xFF && _currentRoom != 0)
			runScript(VAR(VAR_SAVELOAD_SCRIPT), 0, 0, 0);

		mainMenuDialog();		// Display NewGui

		if (VAR_SAVELOAD_SCRIPT != 0xFF && _currentRoom != 0)
			runScript(VAR(VAR_SAVELOAD_SCRIPT2), 0, 0, 0);
		return;
	} else if (VAR_TALKSTOP_KEY != 0xFF && _lastKeyHit == VAR(VAR_TALKSTOP_KEY)) {
		// Some text never times out, and should never be skipped. The
		// Full Throttle conversation menus is the main - perhaps the
		// only - example of this.

		if (_talkDelay != -1) {
			_talkDelay = 0;
			if (_sound->_sfxMode & 2)
				stopTalk();
		}
		return;
	} else if (_lastKeyHit == '[') { // [ Music volume down
		int vol = GetConfig(kConfig_MusicVolume);
		if (!(vol & 0xF) && vol)
			vol -= 16;
		vol = vol & 0xF0;
		SetConfig(kConfig_MusicVolume, vol);
		if (_imuse)
			_imuse->set_music_volume (vol);
	} else if (_lastKeyHit == ']') { // ] Music volume up
		int vol = GetConfig(kConfig_MusicVolume);
		vol = (vol + 16) & 0xFF0;
		if (vol > 255) vol = 255;
		SetConfig(kConfig_MusicVolume, vol);
		if (_imuse)
			_imuse->set_music_volume (vol);
	} else if (_lastKeyHit == '-') { // - text speed down
		_defaultTalkDelay += 5;
		if (_defaultTalkDelay > 90)
			_defaultTalkDelay = 90;

		VAR(VAR_CHARINC) = _defaultTalkDelay / 20;
	} else if (_lastKeyHit == '+') { // + text speed up
		_defaultTalkDelay -= 5;
		if (_defaultTalkDelay < 5)
			_defaultTalkDelay = 5;

		VAR(VAR_CHARINC) = _defaultTalkDelay / 20;
	}
#ifndef DISABLE_DEBUGGER
	else if (_lastKeyHit == '~' || _lastKeyHit == '#') { // Debug console
		_debugger->attach();
	}
#endif

	_mouseButStat = _lastKeyHit;
}

#pragma mark -
#pragma mark --- SCUMM ---
#pragma mark -

/**
 * Start a 'scene' by loading the specified room with the given main actor.
 * The actor is placed next to the object indicated by objectNr.
 */
void ScummEngine::startScene(int room, Actor *a, int objectNr) {
	int i, where;

#ifdef GAME_SAMNMAX
	extern bool _hack_samnmax_intro;
	_hack_samnmax_intro = false;
#endif

	CHECK_HEAP;
	debugC(DEBUG_GENERAL, "Loading room %d", room);

	_system->setIsLoading(true);

	clearMsgQueue();

	fadeOut(_switchRoomEffect2);
	_newEffect = _switchRoomEffect;

	ScriptSlot *ss =  &vm.slot[_currentScript];

	if (_currentScript != 0xFF) {
		if (ss->where == WIO_ROOM || ss->where == WIO_FLOBJECT) {
			if (ss->cutsceneOverride != 0)
				error("Object %d stopped with active cutscene/override in exit", ss->number);
			_currentScript = 0xFF;
		} else if (ss->where == WIO_LOCAL) {
			if (ss->cutsceneOverride != 0) {
				error("Script %d stopped with active cutscene/override in exit", ss->number);
			}
			_currentScript = 0xFF;
		}
	}

	if (VAR_NEW_ROOM != 0xFF)
		VAR(VAR_NEW_ROOM) = room;

	runExitScript();
	killScriptsAndResources();
#ifdef ENGINE_SCUMM6
	clearEnqueue();
#endif
	stopCycle(0);
	_sound->processSoundQues();

	for (i = 1; i < _numActors; i++) {
		_actors[i].hideActor();
	}

	for (i = 0; i < 256; i++) {
		_roomPalette[i] = i;
	}

	clearDrawObjectQueue();

	VAR(VAR_ROOM) = room;
	_fullRedraw = true;

	increaseResourceCounter();

	_currentRoom = room;
	VAR(VAR_ROOM) = room;

	if (room >= 0x80 && _version < 7)
		_roomResource = _resourceMapper[room & 0x7F];
	else
		_roomResource = room;

	if (VAR_ROOM_RESOURCE != 0xFF)
		VAR(VAR_ROOM_RESOURCE) = _roomResource;

	if (room != 0)
		ensureResourceLoaded(rtRoom, room);

	clearRoomObjects();

	if (_currentRoom == 0) {
		_ENCD_offs = _EXCD_offs = 0;
		_numObjectsInRoom = 0;
		_system->setIsLoading(false);
		return;
	}

	initRoomSubBlocks();
	loadRoomObjects();

	if (VAR_V6_SCREEN_WIDTH != 0xFF && VAR_V6_SCREEN_HEIGHT != 0xFF) {
		VAR(VAR_V6_SCREEN_WIDTH) = _roomWidth;
		VAR(VAR_V6_SCREEN_HEIGHT) = _roomHeight;
	}

	VAR(VAR_CAMERA_MIN_X) = SCREEN_WIDTH / 2;
	VAR(VAR_CAMERA_MAX_X) = _roomWidth - (SCREEN_WIDTH / 2);

	camera._mode = kNormalCameraMode;
	camera._cur.x = camera._dest.x = SCREEN_WIDTH / 2;
	camera._cur.y = camera._dest.y = SCREEN_HEIGHT / 2;

	if (_roomResource == 0)
	{
		_system->setIsLoading(false);
		return;
	}

	memset(gfxUsageBits, 0, sizeof(gfxUsageBits));

	if (a) {
		where = whereIsObject(objectNr);
		if (where != WIO_ROOM && where != WIO_FLOBJECT)
			error("startScene: Object %d is not in room %d", objectNr,
						_currentRoom);
		int x, y, dir;
		getObjectXYPos(objectNr, x, y, dir);
		a->putActor(x, y, _currentRoom);
		a->setDirection(dir + 180);
		a->moving = 0;
	}

	showActors();

	_egoPositioned = false;
	runEntryScript();
	if (a && !_egoPositioned) {
		int x, y;
		getObjectXYPos(objectNr, x, y);
		a->putActor(x, y, _currentRoom);
		a->moving = 0;
	}

	_doEffect = true;

	_system->setIsLoading(false);

	CHECK_HEAP;
}

void ScummEngine::initRoomSubBlocks() {
	int i;
	const byte *ptr;
	byte *roomptr, *searchptr, *roomResPtr;
	const RoomHeader *rmhd;

	_ENCD_offs = 0;
	_EXCD_offs = 0;
	_CLUT_offs = 0;
	_PALS_offs = 0;

	nukeResource(rtMatrix, 1);
	nukeResource(rtMatrix, 2);

	for (i = 1; i < res.num[rtScaleTable]; i++)
		nukeResource(rtScaleTable, i);

	memset(_localScriptList, 0, sizeof(_localScriptList));

	memset(_extraBoxFlags, 0, sizeof(_extraBoxFlags));

	// Determine the room and room script base address
	roomResPtr = roomptr = getResourceAddress(rtRoom, _roomResource);
	if (!roomptr || !roomResPtr)
		error("Room %d: data not found (" __FILE__  ":%d)", _roomResource, __LINE__);

	//
	// Determine the room dimensions (width/height)
	//
	rmhd = (const RoomHeader *)findResourceData(MKID('RMHD'), roomptr);
	_roomWidth = READ_LE_UINT16(&(rmhd->old.width));
	_roomHeight = READ_LE_UINT16(&(rmhd->old.height));

	//
	// Find the room image data
	//
	_IM00_offs = findResource(MKID('IM00'), findResource(MKID('RMIM'), roomptr)) - roomptr;

	//
	// Look for an exit script
	//
	ptr = findResourceData(MKID('EXCD'), roomResPtr);
	if (ptr)
		_EXCD_offs = ptr - roomResPtr;

	//
	// Look for an entry script
	//
	ptr = findResourceData(MKID('ENCD'), roomResPtr);
	if (ptr)
		_ENCD_offs = ptr - roomResPtr;

	//
	// Load box data
	//
	ptr = findResourceData(MKID('BOXD'), roomptr);
	if (ptr) {
		int size = getResourceDataSize(ptr);
		createResource(rtMatrix, 2, size);
		roomptr = getResourceAddress(rtRoom, _roomResource);
		ptr = findResourceData(MKID('BOXD'), roomptr);
		memcpy(getResourceAddress(rtMatrix, 2), ptr, size);
	}

	ptr = findResourceData(MKID('BOXM'), roomptr);
	if (ptr) {
		int size = getResourceDataSize(ptr);
		createResource(rtMatrix, 1, size);
		roomptr = getResourceAddress(rtRoom, _roomResource);
		ptr = findResourceData(MKID('BOXM'), roomptr);
		memcpy(getResourceAddress(rtMatrix, 1), ptr, size);
	}

	//
	// Load scale data
	//
	ptr = findResourceData(MKID('SCAL'), roomptr);
	if (ptr) {
		int s1, s2, y1, y2;
		for (i = 1; i < res.num[rtScaleTable]; i++, ptr += 8) {
			s1 = READ_LE_UINT16(ptr);
			y1 = READ_LE_UINT16(ptr + 2);
			s2 = READ_LE_UINT16(ptr + 4);
			y2 = READ_LE_UINT16(ptr + 6);
			if (s1 || y1 || s2 || y2) {
				setScaleSlot(i, 0, y1, s1, 0, y2, s2);
			}
		}
	}

	//
	// Setup local scripts
	//

	// Determine the room script base address
	roomResPtr = roomptr = getResourceAddress(rtRoom, _roomResource);
	searchptr = roomResPtr;

	ResourceIterator localScriptIterator(searchptr);
	while ((ptr = localScriptIterator.findNext(MKID('LSCR'))) != NULL) {
		int id = 0;

		ptr += _resourceHeaderSize;	/* skip tag & size */
		id = ptr[0];
		_localScriptList[id - _numGlobalScripts] = ptr + 1 - roomResPtr;
	}

	ptr = findResourceData(MKID('CLUT'), roomptr);
	if (ptr) {
		_CLUT_offs = ptr - roomptr;
		setPaletteFromRes();
	}

	if (_version == 6) {
		ptr = findResource(MKID('PALS'), roomptr);
		if (ptr) {
			_PALS_offs = ptr - roomptr;
			setPalette(0);
		}
	}

	// Color cycling
	ptr = findResourceData(MKID('CYCL'), roomptr);
	if (ptr) {
		initCycl(ptr);
	}

	// Transparent color
	ptr = findResourceData(MKID('TRNS'), roomptr);
	if (ptr)
		gdi._transparentColor = ptr[0];
	else
		gdi._transparentColor = 255;

	initBGBuffers(_roomHeight);
}

void ScummEngine::pauseGame() {
	pauseDialog();
}

void ScummEngine::shutDown() {
	_quit = true;
}

void ScummEngine::restart() {
// TODO: Check this function - we should probably be reinitting a lot more stuff, and I suspect
//	 this leaks memory like a sieve

	int i;

	// Reset some stuff
	_currentRoom = 0;
	_currentScript = 0xFF;
	killAllScriptsExceptCurrent();
	setShake(0);
	_sound->stopAllSounds();

	// Clear the script variables
	for (i = 0; i < 255; i++)
		_scummVars[i] = 0;

	// Empty inventory
	for (i = 0; i < _numGlobalObjects; i++)
		clearOwnerOf(i);

	// Reinit things
	allocateArrays();                   // Reallocate arrays
	readIndexFile();                    // Reread index (reset objectstate etc)
	scummInit();                        // Reinit scumm variables
	if (_imuse) {
		_imuse->setBase(res.address[rtSound]);
	}
	_sound->setupSound();               // Reinit sound engine

	if (_gameId == GID_MONKEY)
		_scummVars[74] = 1225;

	// Re-run bootscript
	int args[16];
	memset(args, 0, sizeof(args));
	args[0] = _bootParam;	
	runScript(1, 0, 0, args);
}


#pragma mark -
#pragma mark --- GUI ---
#pragma mark -

int ScummEngine::runDialog(Dialog &dialog) {
	// Pause sound & video
	bool old_soundsPaused = _sound->_soundsPaused;
	_sound->pauseSounds(true);

	// Open & run the dialog
	int result = dialog.runModal();

	// full redraw necessary
	_fullRedraw = true;

	// Restore old cursor
	updateCursor();

	// Resume sound & video
	_sound->pauseSounds(old_soundsPaused);
	
	// Return the result
	return result;
}

void ScummEngine::pauseDialog() {
	if (!_pauseDialog)
		_pauseDialog = new PauseDialog(this);
	runDialog(*_pauseDialog);
}

void ScummEngine::mainMenuDialog() {
	if (!_mainMenuDialog)
		_mainMenuDialog = new MainMenuDialog(this);
	runDialog(*_mainMenuDialog);
}

void ScummEngine::confirmexitDialog() {
	ConfirmDialog confirmExitDialog(this, "Do you really want to quit (y/n)?");
	_quit = runDialog(confirmExitDialog);
}

void ScummEngine::confirmrestartDialog() {
	ConfirmDialog confirmRestartDialog(this, "Do you really want to restart (y/n)?");

	if (runDialog(confirmRestartDialog)) {
		restart();
	}
}

char ScummEngine::displayError(const char *altButton, const char *message/*, ...*/) {
/*
#ifdef __PALM_OS__
	char buf[256]; // 1024 is too big overflow the stack
#else
	char buf[1024];
#endif
	va_list va;

	va_start(va, message);
	vsprintf(buf, message, va);
	va_end(va);
*/
	GUI::MessageDialog dialog(message, "OK", altButton);
	return runDialog(dialog);
}

#pragma mark -
#pragma mark --- Miscellaneous ---
#pragma mark -

int SJIStoFMTChunk(int f, int s) //convert sjis code to fmt font offset
{
	enum {
		KANA = 0,
		KANJI = 1,
		EKANJI = 2
	};
	int base = s - (s % 32) - 1;
	int c = 0, p = 0, chunk_f = 0, chunk = 0, cr, kanjiType = KANA;

	if (f >= 0x81 && f <= 0x84) kanjiType = KANA;
	if (f >= 0x88 && f <= 0x9f) kanjiType = KANJI;
	if (f >= 0xe0 && f <= 0xea) kanjiType = EKANJI;

	if ((f > 0xe8 || (f == 0xe8 && base >= 0x9f)) || (f > 0x90 || (f == 0x90 && base >= 0x9f))) {
		c = 48; //correction
		p = -8; //correction
	}

	if (kanjiType == KANA) {//Kana
		chunk_f = (f - 0x81) * 2;
	} else if (kanjiType == KANJI) {//Standard Kanji
		p += f - 0x88;
		chunk_f = c + 2 * p;
	} else if (kanjiType == EKANJI) {//Enhanced Kanji
		p += f - 0xe0;
		chunk_f = c + 2 * p;
	}

	if (base == 0x7f && s == 0x7f)
		base -= 0x20; //correction
	if ((base == 0x7f && s == 0x9e) || (base == 0x9f && s == 0xbe) || (base == 0xbf && s == 0xde))
		base += 0x20; //correction

	switch(base) {
	case 0x3f:
		cr = 0; //3f
		if (kanjiType == KANA) chunk = 1;
		else if (kanjiType == KANJI) chunk = 31;
		else if (kanjiType == EKANJI) chunk = 111;
		break;
	case 0x5f:
		cr = 0; //5f
		if (kanjiType == KANA) chunk = 17;
		else if (kanjiType == KANJI) chunk = 47;
		else if (kanjiType == EKANJI) chunk = 127;
		break;
	case 0x7f:
		cr = -1; //80
		if (kanjiType == KANA) chunk = 9;
		else if (kanjiType == KANJI) chunk = 63;
		else if (kanjiType == EKANJI) chunk = 143;
		break;
	case 0x9f:
		cr = 1; //9e
		if (kanjiType == KANA) chunk = 2;
		else if (kanjiType == KANJI) chunk = 32;
		else if (kanjiType == EKANJI) chunk = 112;
		break;
	case 0xbf:
		cr = 1; //be
		if (kanjiType == KANA) chunk = 18;
		else if (kanjiType == KANJI) chunk = 48;
		else if (kanjiType == EKANJI) chunk = 128;
		break;
	case 0xdf:
		cr = 1; //de
		if (kanjiType == KANA) chunk = 10;
		else if (kanjiType == KANJI) chunk = 64;
		else if (kanjiType == EKANJI) chunk = 144;
		break;
	default:
		return 0;
	}
	
	return ((chunk_f + chunk) * 32 + (s - base)) + cr;
}


const char *ScummEngine::getGameDataPath() const {
	return _gameDataPath.c_str();
}

#ifndef RELEASEBUILD
void ScummEngine::errorString(const char *buf1, char *buf2)
{
	if (_currentScript != 0xFF)
	{
		ScriptSlot *ss = &vm.slot[_currentScript];
		sprintf(buf2, "(%d:%d:0x%X): %s", _roomResource,
			ss->number, _scriptPointer - _scriptOrgPointer, buf1);
	} else {
		strcpy(buf2, buf1);
	}
#ifndef DISABLE_DEBUGGER
	// Unless an error -originated- within the debugger, spawn the debugger. Otherwise
	// exit out normally.
	if (_debugger && !_debugger->isAttached()) {
		printf("%s\n", buf2);	// (Print it again in case debugger segfaults)
		_debugger->attach(buf2);
		_debugger->onFrame();
	}
#endif
}
#endif

#pragma mark -
#pragma mark --- Utilities ---
#pragma mark -

#ifndef RELEASEBUILD
void checkRange(int max, int min, int no, const char *str) {
	if (no < min || no > max) {
		char buf[256]; // 1024 is too big overflow the stack
		sprintf(buf, str, no);
		error("Value %d is out of bounds (%d,%d) (%s)", no, min, max, buf);
	}
}
#endif

/**
 * Convert an old style direction to a new style one (angle),
 */
int newDirToOldDir(int dir) {
	if (dir >= 71 && dir <= 109)
		return 1;
	if (dir >= 109 && dir <= 251)
		return 2;
	if (dir >= 251 && dir <= 289)
		return 0;
	return 3;
}

/**
 * Convert an new style (angle) direction to an old style one.
 */
int oldDirToNewDir(int dir) {
	assert(0 <= dir && dir <= 3);
	const int new_dir_table[4] = { 270, 90, 180, 0 };
	return new_dir_table[dir];
}

/**
 * Convert an angle to a simple direction.
 */
int toSimpleDir(int dirType, int dir) {
	if (dirType) {
		const int16 directions[] = { 22,  72, 107, 157, 202, 252, 287, 337 };
		for (int i = 0; i < 7; i++)
			if (dir >= directions[i] && dir <= directions[i+1])
				return i+1;
	} else {
		const int16 directions[] = { 71, 109, 251, 289 };
		for (int i = 0; i < 3; i++)
			if (dir >= directions[i] && dir <= directions[i+1])
				return i+1;
	}

	return 0;
}

/**
 * Convert a simple direction to an angle.
 */
int fromSimpleDir(int dirType, int dir) {
	if (dirType)
		return dir * 45;
	else
		return dir * 90;
}

/**
 * Normalize the given angle - that means, ensure it is positive, and
 * change it to the closest multiple of 45 degree by abusing toSimpleDir.
 */
int normalizeAngle(int angle) {
	int temp;

	temp = (angle + 360) % 360;

	return toSimpleDir(1, temp) * 45;
}

const char *tag2str(uint32 tag) {
	static char str[5];
	str[0] = (char)(tag >> 24);
	str[1] = (char)(tag >> 16);
	str[2] = (char)(tag >> 8);
	str[3] = (char)tag;
	str[4] = '\0';
	return str;
}

} // End of namespace Scumm

using namespace Scumm;



Engine *Engine_SCUMM_create(GameDetector *detector, OSystem *syst) {
	Engine *engine;

	
	const ScummGameSettings *g = scumm_settings;
	while (g->name) {
		if (!scumm_stricmp(detector->_game.name, g->name))
			break;
		g++;
	}
	if (!g->name)
		error("Invalid game '%s'\n", detector->_game.name);

	ScummGameSettings game = *g;

	byte platform = GetConfig(kConfig_Platform);
	switch (platform)
	{
	case Common::kPlatformAmiga:
		game.features |= GF_AMIGA;
		break;
	default:
		game.features |= GF_PC;
		break;
	}

	switch (game.version) {
#ifdef ENGINE_SCUMM5
	case 5:
		engine = new ScummEngine_v5(detector, syst, game);
		break;
#endif //ENGINE_SCUMM5

#ifdef ENGINE_SCUMM6
	case 6:
		engine = new ScummEngine_v6(detector, syst, game);
		break;
#endif //ENGINE_SCUMM6

	default:
		error("Engine_SCUMM_create(): Unknown version of game engine");
	}

	return engine;
}

