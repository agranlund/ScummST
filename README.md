------------------------------------------------------------------------
ScummST, build 211218
A ScummVM port for stock Atari ST/STE/Falcon/TT
------------------------------------------------------------------------

This is an unofficial, stripped down and quite heavily modified version
of ScummVM targetting low-end unaccelerated Atari computers.
It is mostly, but not exclusively, based on version 0.6.0

This is not meant as a fully featured port of ScummVM, so
grab the official version instead if you have a (very fast) Atari.
Consider this a port of the Scumm5 + Scumm6 games rather than
a port of a generic ScummVM.

------------------------------------------------------------------------
Supported games:
------------------------------------------------------------------------
ATLANTIS.PRG  - Indiana Jones and the fate of Atlantis
MONKEY.PRG    - The Secret of Monkey Island
MONKEY2.PRG   - Monkey Island 2: Le'Chucks Revenge
TENTACLE.PRG  - Day of the Tentacle
SAMNMAX.PRG   - Sam & Max: Hit the Road

------------------------------------------------------------------------
Hardware requirements:
------------------------------------------------------------------------
Hard drive and minimum 2MB RAM
(Sam & Max requires 4MB RAM, unsure about DOTT)

Monkey Island 1, Monkey Island 2, Fate of Atlantis:
  An 8mhz machine will play these games fine.

Day of the Tentacle:
  This game is playable on 8Mhz machines but a Mega STE or better
  will play them fine even by modern standards.

Sam & Max:
  It is playable on a Mega STE, with slowdowns in certain scenes.
  A faster machine such as a TT is recommended.


------------------------------------------------------------------------
Usage:
------------------------------------------------------------------------
Put the executable in the same folder as your game files
and run it from there.

First run will trigger a rather lengthy installation process where
it converts some resources into formats better suited for the
Atari. This will take upward of 30 minutes on a stock 8Mhz ST.

The new files are called <game>.ST0 and <game>.ST1 and once you
have these it will no longer need the old <game>.000/001/002 files.
I suggest keeping these around anyway since it's likely that
future releases may trigger reinstallation if data formats change.

Press F5 in game to access the main menu for options and the
ability to load/save your game.


------------------------------------------------------------------------
Pre-convert data files on a PC:
------------------------------------------------------------------------
You can pre-convert the game resources on a PC to avoid the lengthy
installation step on the Atari.

Run convert.exe in the same folder as your data files and it
should output the ST0 + ST1 files you need.
These two files, MONSTER.SOU and the Atari executable are the only
files you need to play the game on your Atari.


------------------------------------------------------------------------
Where to legally get game data files:
------------------------------------------------------------------------


------------------------------------------------------------------------
* Monkey Island 1:
------------------------------------------------------------------------
ScummST only supports the 256color CD-ROM release for DOS.

This version can be extracted from the Special Edition and is
avilable on Steam and GOG.com

I strongly recommend using "Monkey Island Ultimate Talkie Edition Builder"
to generate a full talkie version, with music, from Special Edition:
http://www.gratissaugen.de/ultimatetalkies/
Copy the "monkey.000", "monkey.001" and "monster.sou" which the
talkie builder generates.


Alternatively you can use "Monkey Island Explorer" and extract just
the original data files (monkey1.000 and monkey1.001 - rename these
 to monkey.000 and monkey.001)
This version will be complete silent for the same reasons as the
original MS-DOS version.

The original MS-DOS CD-ROM version should be compatible but it too
will be completely silent since it does not have speech, nor MIDI music
since it relies on music being played from the CD.


------------------------------------------------------------------------
* Monkey Island 2:
------------------------------------------------------------------------
Special Edition contains the original DOS files and is available on
Steam and GOG.com

I recommend using the "Monkey Island Ultimate Talkie Edition Builder"
to generate a full talkie version version from the Special Edition:
http://www.gratissaugen.de/ultimatetalkies/
Copy "monkey2.001", "monkey2.001" and "monster.sou" which the
talkie builder generates.


Alternatively, you can use "Monkey Island Explorer" and extract
just the original data files listed below.
This version will not have speech.
classic/en/monkey2.000
classic/en/monkey2.001
https://quickandeasysoftware.net/software/monkey-island-explorer



------------------------------------------------------------------------
* Fate of Atlantis:
------------------------------------------------------------------------
The DOS talkie edition can be bought from GOG.com.
You need to take following files from the game folder:
ATLANTIS.000
ATLANTIS.001
MONSTER.SOU   (optional, if you want speech)


------------------------------------------------------------------------
* Day of the Tentacle:
------------------------------------------------------------------------
The remastered edition contains the original DOS talkie edition files
and is available to purchase on Steam and GOG.com

Use "DoubleFine Explorer" to extract the following files from the game:
classic/en/tentacle.000
classic/en/tentacle.001
classic/en/monster.sou   (optional, if you want speech)

https://quickandeasysoftware.net/software/doublefine-explorer


------------------------------------------------------------------------
* Sam & Max: Hit the Road
------------------------------------------------------------------------
This game is available to purchase on Steam on GOG.com
You need to take the following files from the game folder:
SAMNMAX.000
SAMNMAX.001
MONSTER.SOU  (optional, if you want speech)



------------------------------------------------------------------------
Important info and issues
------------------------------------------------------------------------
This is a work in progress build so don't be surprised if there
are bugs and issues.

- This version is not compatible with savegames from earlier versions!

- This version may not be compatible with 060 equipeed machines
  unless the the MOVEP instruction is emulated by the OS.

- You need PC MS-DOS release of these games.
  Other platforms are not supported.

- Only the CD-ROM version of Monkey Island 1 will work.
  I recommend the "Ultimate Talkie Edition" which can be
  generated from the modern "Special Edition" - this is the
  only way to get speech and music in this game.
  Normal CD-ROM version will be completely silent.

- Demo versions are not supported (yet)
 
- You need original uncompressed sound files, it will NOT play
  audio which has been compressed with the scummvm tools.
  (the file monster.SOF or monster.SOG will not work.
  You need monster.SOU as it came with the game originally)

- If you are crashing you could try different options for sound/music.



------------------------------------------------------------------------
Options
------------------------------------------------------------------------

Graphics:
It runs in 16 color ST-Low resolution and wont work with fancy
graphics cards. There may or may not be options for TT and Falcon
video modes in the future.

Music:
It sounds best if you have a real MT32 hooked up.
General Midi sounds ok if you have that type of synthesizer.
The YM sound chip will try its hardest to play the tunes if you
lack midi equipment :)

Sound:
DMA sound is the best option for machines that support it

For plain-old ST you can choose to play speech and sound effects
through the YM chip. It doesn't sound all that great and
consumes quite a bit of CPU.

A sound cartridge or parallel port DAC are better options
for DMA-less computers in terms of quality and CPU usage.
The CVX4 from Serdashop has been tested to work on my stock ST.

Most sound cartridges are quite similar so even if you have a
brand that is not listed it may be worth testing with an
existing option of similar spec:
  Ubisoft MV16:       12bit unsigned mono
  Microdeal Replay8:   8bit unsigned mono
  Microdeal Replay16: 16bit unsigned mono
  Microdeal Playback:  8bit unsigned stereo



------------------------------------------------------------------------
Info
------------------------------------------------------------------------
Homepage for this project: http://www.happydaze.se/

Sourcecode is here: https://github.com/agranlund/ScummST

Progress is mainly posted in this thread on the Exxos forums:
https://www.exxoshost.co.uk/forum/viewtopic.php?f=61&t=2644




------------------------------------------------------------------------
Many thanks to everyone who contributes by testing this
on various Atari's with different configurations.

Atarian Computing, DoG, Heiko Poppen, JezC,
kodak80, mouse_master, nemo, stephen_usher, Steve,
Sturm, wietze + everyone else who I may have forgotten :)

------------------------------------------------------------------------
This version is mostly based on ScummVM v0.6.0 and older source code,
with changes to tailor it specifically for the Atari ST and
a very limited number of games.

------------------------------------------------------------------------
For information about the official ScummVM, compatibility lists, details
on donating, the latest release, progress reports and more,
please visit the ScummVM home page at: http://www.scummvm.org/
