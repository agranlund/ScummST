# Target: debug/profile/release

ifeq ($(target),debug)
TARGETDEF=-DEBUGBUILD
TARGETFLG=-g
STRIPX=
STRIP=

else ifeq ($(target),profile)
TARGETDEF=-DRELEASEBUILD
TARGETFLG=-g -O3
STRIPX=./backends/atari/tools/gst2ascii$(NATIVE_EXT)
STRIP=$(STRIPX) -l -o $(EXECUTABLE)> $(SYMBOLS) 2>/dev/null

else
TARGETDEF=-DRELEASEBUILD
TARGETFLG=-O3
STRIPX=./backends/atari/tools/stripx$(NATIVE_EXT)
STRIP=$(STRIPX) -s -v -f $(EXECUTABLE)
endif

ATARI_GCC		= m68k-atari-mint-gcc
ATARI_EXT		= .PRG

NATIVE_GCC		= gcc
NATIVE_EXT 		= 

PARCP			= ./backends/atari/tools/parcp421/apple/parcp
PARCP_DST		= E:\CODE\SCUMMVM

LIBCMINI_INC	= -Ibackends/atari/libcmini/include
LIBCMINI_LIB	= -Lbackends/atari/libcmini/build
LIBCMINI_CRT	= backends/atari/libcmini/build/minicrt0.o
LIBCMINI_CFL	= -nostdlib -ffast-math
LIBCMINI_LFL	= -nodefaultlibs -nostdlib -nostartfiles -lcmini

INCLUDES		= $(LIBCMINI_INC) -Ibackends/atari
LDFLAGS 		= $(LIBCMINI_CRT) $(TARGETFLG) $(LIBCMINI_LIB) -Wl,-Map=backends/atari/mapfile -Wl,--traditional-format
# Thorsten's gcc 9.3.1
CXXFLAGS 		= $(TARGETFLG) -Wno-multichar -Wno-invalid-offsetof -Wno-narrowing -fpermissive $(LIBCMINI_CFL) -std=c++11 -m68000 -fomit-frame-pointer -fno-rtti -fno-exceptions
# Vincent's gcc 4.6.3
#CXXFLAGS 		= $(TARGETFLG) -Wno-multichar -Wno-invalid-offsetof -fpermissive $(LIBCMINI_CFL) -std=c++0x -m68000 -fomit-frame-pointer

CXX 			= $(ATARI_GCC)
EXECUTABLE_EXT	= $(ATARI_EXT)

DEFINES			= -D__ATARI__ -DNONSTANDARD_PORT -DDISABLE_DEBUGGER -DDISABLE_ADLIB -DNDEBUG $(TARGETDEF) $(GAMEDEF)

LIBS			= $(LIBCMINI_LFL)
OBJS 			+= backends/atari/lgcc.o backends/atari/lstdc++.o backends/atari/exception.o backends/atari/atari.o backends/atari/cursor.o backends/atari/sound.o backends/atari/irq.o backends/midi/stmidi.o backends/midi/stchip.o
DISASM			= 
SYMBOLS			= $(EXEPATH)/$(GAME).SYM
PROFILEDATA		= $(EXEPATH)/$(GAME).profile.txt

INCS = backends/atari/portdefs.h Makefile
include Makefile.common

%.o : %.S
	$(CXX) $(INCLUDE) $(SFLAGS) -c $< -o $@

%.disasm : %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -fverbose-asm -g -S $< -o $@

$(STRIPX): $(STRIPX).c
	$(NATIVE_GCC) $(STRIPX).c -o $(STRIPX)

profile:
	@iconv -f ISO-8859-1 -t UTF8 $(PROFILEDATA)> profile.tmp
	./backends/atari/tools/hatari_profile.py -st -i -r $(SYMBOLS) profile.tmp

go:
	echo "CD $(PARCP_DST)\$(GAME)" > parcp.cmd
	echo "LCD bin" >> parcp.cmd
	echo "PUT $(GAME).$(EXECUTABLE_EXT)" >> parcp.cmd
	echo "EXEC -n $(GAME).$(EXECUTABLE_EXT)" >> parcp.cmd
	echo "QUIT" >> parcp.cmd
	$(PARCP) -b parcp.cmd

