//-------------------------------------------------------------------------
// PC -> Atari resource converter
//
// This file is distributed under the GPL v2, or at your option any
// later version.  Read COPYING for details.
// (c)2021, Anders Granlund
//-------------------------------------------------------------------------

#include "stdafx.h"
#include "common/str.h"
#include "scumm/gfx.h"
#include "scumm/dialogs.h"
#include "scumm/imuse.h"
#include "scumm/object.h"
#include "scumm/resource.h"
#include "scumm/scumm.h"
#include "scumm/sound.h"
#include "scumm/verbs.h"

//#define ATARI_SOUND_CONVERTER


// atari sprite ver:
//	0 : no conversion
//	1 : copy original data
//	2 : horizontal rle
#define ATARI_SPRITE_VER		2

// atari sound ver:
//	0 : invalid
//	1 : 
#define ATARI_SOUND_VER			1


#ifndef __ATARI__
byte _atari_res_version = ATARI_RESOURCE_VERSION;
#endif

//#ifdef __ATARI__
#if 1
#define ATARI_RESOURCE_XOR			0x00
#define ATARI_RESOURCE_HEADER		1
#else
#define ATARI_RESOURCE_XOR			0x69
#define ATARI_RESOURCE_HEADER		0
#endif

extern uint32 c2ptable256[];

namespace Scumm {


extern uint8 system_colors[16*4];

static const uint32 imnn_tags[] = {
	MKID('IM00'), MKID('IM01'), MKID('IM02'), MKID('IM03'), MKID('IM04'), MKID('IM05'), MKID('IM06'), MKID('IM07'),
	MKID('IM08'), MKID('IM09'), MKID('IM0A'), MKID('IM0B'), MKID('IM0C'), MKID('IM0D'), MKID('IM0E'), MKID('IM0F'),
	MKID('IM10'), MKID('IM11'), MKID('IM12'), MKID('IM13'), MKID('IM14'), MKID('IM15'), MKID('IM16'), MKID('IM17'),
	MKID('IM18'), MKID('IM19'), MKID('IM1A'), MKID('IM1B'), MKID('IM1C'), MKID('IM1D'), MKID('IM1E'), MKID('IM1F'),
	MKID('IM20'), MKID('IM21'), MKID('IM22'), MKID('IM23'), MKID('IM24'), MKID('IM25'), MKID('IM26'), MKID('IM27'),
	MKID('IM28'), MKID('IM29'), MKID('IM2A'), MKID('IM2B'), MKID('IM2C'), MKID('IM2D'), MKID('IM2E'), MKID('IM2F'),
	MKID('IM30'), MKID('IM31'), MKID('IM32'), MKID('IM33'), MKID('IM34'), MKID('IM35'), MKID('IM36'), MKID('IM37'),
	MKID('IM38'), MKID('IM39'), MKID('IM3A'), MKID('IM3B'), MKID('IM3C'), MKID('IM3D'), MKID('IM3E'), MKID('IM3F'),
};

static const uint32 zpnn_tags[] = {
	MKID('ZP00'), MKID('ZP01'), MKID('ZP02'), MKID('ZP03'),
	MKID('ZP04'), MKID('ZP05'), MKID('ZP06'), MKID('ZP07'),
};


//-----------------------------------------------------------------
//
// resource converter
//
//-----------------------------------------------------------------
class AtariResourceConverter
{
public:
	AtariResourceConverter();
	~AtariResourceConverter();
	bool GenerateResourceFiles();

private:

	enum ResDirTypes
	{
		rtResDirRoom = 0,
		rtResDirScript,
		rtResDirSound,
		rtResDirCostume,
		rtResDirCharset,
		rtResDirCOUNT
	};

	struct ResDir
	{
		uint16	count;
		byte*	room;
		uint32*	offset;
		byte*	valid;
	};

	struct ConvCost
	{
		uint16 oldOffs;
		uint32 newOffs;
		uint32 newSize;
		byte*  oldData;
		byte*  newData;
	};

	byte	progCur;
	byte	progMax;
	byte	numRooms;
	byte	roomRes[128];
	byte 	roomId[128];
	uint32 	roomOffs[128];
	ResDir	resDirs[rtResDirCOUNT];
	File	f1, f2;
	byte*	tempScreen;
	ConvCost* convCost;
	byte*	virtualPal;
	bool	ditherEnabled;

#ifndef __ATARI__
	int		numCostumesPerFormat[4];
	int 	numBomps;
	int		totalBompSizeOld;
	int		totalBompSizeNew;
#endif

	bool openInFile(byte resNo);
	bool openOutFile(byte resNo);
	void closeInFile();
	void closeOutFile();
	void closeFiles();
	void writeBlockSize(uint32 blockStart, bool align=false);
	bool readBlock(uint32& blockType, uint32& blockSize);
	bool copyFileContent(uint32 len);
	bool copyFileBlock(uint32 type, uint32 size, bool align=false);

	bool wasAlreadyConverted();
	bool deleteAtariResources();

	uint32 encodeAtariCost2(byte* dst, const byte* src, const byte* pal, uint16 width,  uint16 height);

	bool convertPICT(ConvCost* pict, byte format);
	uint32 convertBOMP(byte roomNo, byte* dst, const byte* src, uint16 width, uint16 height);

	bool writeGENERIC(byte* ptrBlock, bool align=false);
	bool writeCOST(byte roomNo, uint32 blocktype, uint32 blocksize);
	bool writeBOMP(byte roomNo, uint32 blocktype, uint32 blocksize);
	bool writeIMnn(byte roomNo, byte imNo, const byte* ptrIMnn, uint16 stripCount, uint16 stripHeight, uint16 zCount);
	bool writeOBIM(byte roomNo, byte* ptrOBIM);
	bool writeRMIM(byte roomNo, byte* ptrROOM, uint16 width, uint16 height);
	bool writeROOM(byte roomNo, uint32 blocktype, uint32 blocksize);
	bool writeLFLF(uint16 resNo, uint32 blocktype, uint32 blocksize);
	bool readIndexFile();
	bool writeIndexFile();
	bool writeResFile(uint16 resNo);
	byte getRoomId(byte resNo, uint32 resOffs);
	byte getRoomNo(byte resNo, uint32 resOffs);
	bool writeRoomOffset(byte resNo, byte roomNo, byte roomId, uint32 newOffs);
	bool updateResDir(ResDirTypes type, byte resNo, byte roomNo, uint32 oldOffs, uint32 newOffs);

	uint32 colorWeight(int16 r, int16 g, int16 b);
	void findClosestColors(byte r, byte g, byte b, byte* c0, byte* c1);
	void buildC2PTable();
	void enableDither(bool enable);
};

AtariResourceConverter::AtariResourceConverter()
{
	for (int i=0; i<rtResDirCOUNT; i++)
		resDirs[i].count = 0;

#ifndef __ATARI__
	numBomps = 0;
	totalBompSizeOld = 0;
	totalBompSizeNew = 0;
	for (int i=0; i<4; i++)
		numCostumesPerFormat[i] = 0;
#endif

	tempScreen = (byte*) malloc(320*200*8);	// 512kb temp data
	numRooms = 0;
	for (int i=0; i<128; i++)
	{
		roomRes[i] = 0;
		roomId[i] = 0;
		roomOffs[i] = 0;
	}
	progCur = 0;
	progMax = 128;

	ditherEnabled = false;

	virtualPal = (byte*) malloc(256 * 4);
	convCost = new ConvCost[256];
}

AtariResourceConverter::~AtariResourceConverter()
{
	free(virtualPal);
	free(tempScreen);
	for (int i=0; i<rtResDirCOUNT; i++)
	{
		ResDir* dir = &resDirs[i];
		if (dir->count > 0)
		{
			free(dir->room);
			free(dir->offset);
		}
	}
	delete[] convCost;
	closeFiles();
}

bool AtariResourceConverter::deleteAtariResources()
{
	return true;
}

bool AtariResourceConverter::wasAlreadyConverted()
{
#ifdef GAME_RESOURCECONVERTER
	return false;
#endif

	char fname[128]; sprintf(fname, "%s.ST0", g_scumm->_gameName.c_str());
	if (f1.open(fname, g_scumm->getGameDataPath(), File::kFileReadMode, ATARI_RESOURCE_XOR))
	{
#if ATARI_RESOURCE_HEADER
		uint32 blocktype, blocksize;
		while (1)
		{
			if (!readBlock(blocktype, blocksize))
				break;

			if (blocktype == MKID('_ST_'))
			{
				blocksize -= 4;
				uint16 ver1 = f1.readUint16BE();
				//uint16 ver2 = f2.readUint16BE();
				if (ver1 == ATARI_RESOURCE_VERSION)
				{
					f1.close();
					return true;
				}
				debug(1, "ATARI resource files out of date");
				break;
			}
			f1.seek(blocksize - 8, SEEK_CUR);
		}
		f1.close();
#else
		f1.close();
		return true;
#endif
	}
	return false;
}

bool AtariResourceConverter::GenerateResourceFiles()
{
	// check if valid resources already exist
	if (wasAlreadyConverted())
		return true;

	// create virtual palete
	for (int i=0; i<16; i++)
	{
		int16 c0r = system_colors[(i*4) + 0];
		int16 c0g = system_colors[(i*4) + 1];
		int16 c0b = system_colors[(i*4) + 2];
		for (int j=0; j<16; j++)
		{
			int16 c1r = system_colors[(j*4) + 0];
			int16 c1g = system_colors[(j*4) + 1];
			int16 c1b = system_colors[(j*4) + 2];
			
			int16 car = ((c0r + c1r) / 2);
			int16 cag = ((c0g + c1g) / 2);
			int16 cab = ((c0b + c1b) / 2);

			int idx = (i + (j << 4)) << 2;
			virtualPal[idx + 0] = car;
			virtualPal[idx + 1] = cag;
			virtualPal[idx + 2] = cab;

			byte cost = 0;
			if (i != j)
			{
				uint32 w = colorWeight(c1r - c0r, c1g - c0g, c1b - c0b);
				cost = (byte) CLAMP<uint32>((w >> 11), 1, 255);
			}
			virtualPal[idx + 3] = cost;
		}
	}

	// read original index file
	if (!readIndexFile())
		return false;
	
	// converts resource files
	if (!openOutFile(1))
		return false;

	progCur = 0;
	progMax = MAX<byte>(resDirs[rtResDirRoom].count, 1);
	g_scumm->_system->installStart();

	// write placeholder LECF block 
	f2.writeUint32(MKID('LECF'));
	f2.writeUint32BE(8);

	// write placeholder LOFF block

	f2.writeUint32(MKID('LOFF'));
	f2.writeUint32BE((8 + 1 + (5 * 127)));
	f2.writeByte(0);			// placeholder room count
	for(uint i=0; i<127; i++)
	{
		f2.writeByte(0);		// placeholder room id
		f2.writeUint32(0);		// placeholder room offset
	}

	// write all resources
	int resNo = 1;
	while(1)
	{
		if (!openInFile(resNo))
			break;

		bool result = writeResFile(resNo);
		closeInFile();
		if (!result)
			break;
		resNo++;
	}

	// finish LOFF block
	uint32 pos = f2.pos();
	f2.seek(16, SEEK_SET);
	f2.writeByte(numRooms);
	f2.seek(pos, SEEK_SET);

	// finish LECF block
	writeBlockSize(0);
	closeOutFile();

	if (resNo < 2)
	{
		debug(1,"FAILED");
		g_scumm->_system->installDone();
		return false;
	}

	// write new index file
	writeIndexFile();
	debug(1,"done");
	g_scumm->_system->installDone();

#ifndef __ATARI__
	debug (1,"num bomp %d (%d --> %d)", numBomps, totalBompSizeOld, totalBompSizeNew);
	for(int i=0; i<4; i++)
		debug(1,"num cost %02x : %d", 0x40+i, numCostumesPerFormat[i]);
#endif


	return true;
}

bool AtariResourceConverter::updateResDir(ResDirTypes type, byte resNo, byte roomNo, uint32 oldOffs, uint32 newOffs)
{
	ResDir* dir = &resDirs[type];
	byte oldRoom = roomNo;
	byte newRoom = roomNo;

	int16 found = 0;
	for (uint16 i=0; i<dir->count; i++)
	{
		if ((dir->room[i] == oldRoom) && (dir->offset[i] == oldOffs))
		{
			debug(1, "update resdir %d(%d). %d,%d -> %d,%d", type, i, oldRoom, oldOffs, newRoom, newOffs);
			dir->room[i] = newRoom;
			dir->offset[i] = newOffs;
			dir->valid[i] = 1;
			found++;
			// ok, so we can't return here, at least in samnmax there are multiple entries for the same thing..
			//return true;
		}
	}

	if (found == 0)
	{
		debug (1, "### unable to find resource id for %d,%d:%d:%d", type, resNo, roomNo, oldOffs);
		//exit(0);
	}
	return true;
}

byte AtariResourceConverter::getRoomNo(byte resNo, uint32 oldOffs)
{
	for (uint i=0; i<numRooms; i++)
		if((roomRes[i] == resNo) && (roomOffs[i] == oldOffs))
			return i;
	return 0xFF;
}

byte AtariResourceConverter::getRoomId(byte resNo, uint32 oldOffs)
{
	for (uint i=0; i<numRooms; i++)
		if((roomRes[i] == resNo) && (roomOffs[i] == oldOffs))
			return roomId[i];
	return 0xFF;
}

bool AtariResourceConverter::writeRoomOffset(byte resNo, byte roomNo, byte roomId, uint32 newOffs)
{
	debug(1, "update room offs: %x : %d, %d", newOffs, roomNo, roomId);

	roomOffs[roomNo] = newOffs;

	// update resource directory
	ResDir* dir = &resDirs[rtResDirRoom];
	dir->room[roomId] = (resNo > 0) ? 1 : 0;
	dir->offset[roomId] = 0;
	dir->valid[roomId] = 1;

	// update LOFF block
	uint32 pos = f2.pos();
	f2.seek(17 + ((roomNo) * 5), SEEK_SET);
	f2.writeByte(roomId);
	f2.writeUint32LE(newOffs);
	f2.seek(pos, SEEK_SET);
	return true;
}

uint32 AtariResourceConverter::convertBOMP(byte roomNo, byte* dst, const byte* src, uint16 width, uint16 height)
{
#if defined(GAME_SAMNMAX) || defined(ENGINE_SCUMMALL)
	byte prescale = 1;
	byte preshift = 1;
	byte minscale = 16;
	byte maxscale = 255;
	byte hwdepend = 0;

	if (g_scumm->_gameId == GID_SAMNMAX)
	{
		switch (roomNo)
		{
			// logo screen
			case 0:
				// small moving logo
				if (width < 256) {
					prescale = 4;
					preshift = 8;
					hwdepend = 1;
				}
				break;

			// mystery vortex
			case 33:
				prescale = 1;
				preshift = 8;
				break;

			// highway surfing minigame
			case 61:
			case 62:
				if ((width == 48 && height == 56) || (width == 40 && height == 72))
				{	// max
					preshift = 1;
				}
				else if (width == 168 && height == 64)
				{	// car
					preshift = 1;
				}
				else if (
					(width == 24 && height > width) ||
					(width == 136 && height == 56) ||
					(width == 200 && height == 88) ||
					(width == 296 && height == 16))
				{
					// scaling signs
					prescale = 8;
					preshift = 1;
				}
				break;

			// vortex snow globe
			case 69:
				prescale = 1;
				preshift = 8;
				break;

			// cursors
			case 83:
				break;
		}
	}

#ifndef __ATARI__
	numBomps++;
	totalBompSizeOld += (width * height);
	uint32 baseSize = (((width * height) >> 1) + ((width * height) >> 1));
	if ((prescale != 1) && (prescale != 2) && (prescale != 4) && (prescale != 8))
	{
		debug(1,"invalid prescale: %d", prescale);
		exit(0);
	}
	for (int16 i=0; i<prescale; i++)
	{
		totalBompSizeNew += (baseSize * preshift);
		baseSize >>= 1;
	}
	if (width & 7)
	{
		debug(1,"bomp width is not multiple of 8 (%dx&d)", width, height);
		exit(0);
	}
#endif

	// decode original bomp data
	static const uint32 maxConvertedSize = (320*200*4);
	byte* decodedBomp = dst + maxConvertedSize;
	memset(decodedBomp, 0xFF, ((width + 16) * height));
	decodedBomp += 8;
	byte* temp = decodedBomp;
	uint16 h = height;
	while (h)
	{
		const byte* lsrc = src + 2;
		byte* ldst = temp;
		uint16 lw = width;
		while (lw != 0)
		{
			byte code = *lsrc++;
			byte num = (code >> 1) + 1;
			if (num > lw)
				num = lw;
			lw -= num;
			if (code & 1) {
				byte color = *lsrc++;
				memset(ldst, color, num);
			} else {
				memcpy(ldst, lsrc, num);
				lsrc += num;
			}
			ldst += num;
		}
		src += READ_LE_UINT16(src) + 2;
		temp += (width + 16);
		h--;
	}

	debug(1, "Converting BOMP: %d,%d", width, height);

	static const uint32 baseHdrSize  = 4;
	static const uint32 scaleHdrSize = (8 * 2);
	static const uint32 frameHdrSize = 4 + 4 + (8 * 4);
	static const uint32 totalHdrSize = baseHdrSize + scaleHdrSize + (frameHdrSize * 8);

	// base header
	memset(dst, 0, totalHdrSize);
	byte flags = (hwdepend << 7) | (((prescale-1) & 0x7) << 3) | (((preshift-1) & 0x7) << 0);
	*(dst + 0) = 0;								// reserved
	*(dst + 1) = flags;							// flags
	*(dst + 2) = minscale;						// min scale
	*(dst + 3) = maxscale;						// max scale

	// init scale table
	uint16* scaleHdr = (uint16*)(dst + baseHdrSize);
	{
		static const int16 scaleStepMasks[8] = { 0x00, 0x10, 0x54, 0x54, 0x54, 0x54, 0xFE, 0xFE };
		const int16 scaleStepMask = scaleStepMasks[prescale - 1];
		uint16 offs = baseHdrSize + scaleHdrSize;
		for (int16 i = 0; i < 8; i++)
		{
			if (scaleStepMask & (1 << i))
				offs += frameHdrSize;
			WRITE_BE_UINT16(&scaleHdr[i], offs);
		}
	}

	// encode new bomp
	static const byte shiftSelectMasks[8] = { 0x01, 0x11, 0x49, 0x55, 0x5D, 0xDD, 0xDF, 0xFF};
	const byte shiftSelectMask = shiftSelectMasks[preshift - 1];
	uint32* curDataPtr = (uint32*)(dst + totalHdrSize);
	uint32* lastDataPtr = curDataPtr;
	uint32* frameHdr = (uint32*)(dst + baseHdrSize + scaleHdrSize);


	uint32 scaledWidth = width << 8;
	uint16 scaledHeight = height << 8;
	uint32 scaledWidthStep = scaledWidth / prescale;
	uint16 scaledHeightStep = scaledHeight / prescale;
	for (int16 scaleIdx = 0; scaleIdx < prescale; scaleIdx++)
	{
		debug(1,"scaleIdx = %d, height = %d, scaledHeight = %d", scaleIdx, height, scaledHeight);

		if (scaleIdx != 0)
		{
			scaledWidth -= scaledWidthStep;
			if ((int32)scaledWidth < (1 << 8))
				scaledWidth = (1 << 8);
			scaledHeight -= scaledHeightStep;
			if ((int32)scaledHeight < (1 << 8))
				scaledHeight = (1 << 8);
		}
		uint16 w = (scaledWidth >> 8);
		uint16 h = (scaledHeight >> 8);
		uint16 stepx = (width << 16) / scaledWidth;
		uint16 stepy = (height << 16) / scaledHeight;

		// update frame header
		int16 xtiles = (w >> 3) + 1;
		uint32 dataSize = ((xtiles * h) << 3);	// data+mask interleaved
		byte* inf = (byte*)frameHdr;
		*(inf + 0) = xtiles;
		*(inf + 1) = h;
		*(inf + 2) = 0;
		*(inf + 3) = 0;

		debug(1,"xtiles = %d, height = %d", xtiles, h);

		WRITE_BE_UINT32(inf + 4, dataSize);
		// write frame data
		for (uint16 shiftIdx = 0; shiftIdx < 8; shiftIdx++)
		{
			if (shiftSelectMask & (1 << shiftIdx))
			{
				lastDataPtr = curDataPtr;
				uint32* encData = curDataPtr;
				uint32 srcPitch = width + 16;
				int16 lines = h;
				uint32 sy = 0;
				while(lines)
				{
					byte* spix = decodedBomp + ((sy>>8) * srcPitch);
					uint32 sx = 0;
					uint32 pm = 0;
					uint32 px = 0;
					// first tile, adjust for preshift
					{
						uint32 mask = 0x80808080 >> shiftIdx;
						for (int16 i = shiftIdx; i < 8; i++ )
						{
							byte c = spix[sx>>8];
							sx += stepx;
							px |= c2ptable256[(i << 8) + c];
							if (c != 0xFF)
								pm |= mask;
							mask >>= 1;
						}
						*encData++ = px;
						*encData++ = pm;
					}
					// remaining tiles
					for (uint16 i = 1; i < xtiles; i++)
					{
						pm = 0;
						px = 0;
						byte c;
						c = spix[sx>>8]; sx += stepx; px |= c2ptable256[0x000 + c]; if (c != 0xFF) { pm |= 0x80808080; }
						c = spix[sx>>8]; sx += stepx; px |= c2ptable256[0x100 + c]; if (c != 0xFF) { pm |= 0x40404040; }
						c = spix[sx>>8]; sx += stepx; px |= c2ptable256[0x200 + c]; if (c != 0xFF) { pm |= 0x20202020; }
						c = spix[sx>>8]; sx += stepx; px |= c2ptable256[0x300 + c]; if (c != 0xFF) { pm |= 0x10101010; }
						c = spix[sx>>8]; sx += stepx; px |= c2ptable256[0x400 + c]; if (c != 0xFF) { pm |= 0x08080808; }
						c = spix[sx>>8]; sx += stepx; px |= c2ptable256[0x500 + c]; if (c != 0xFF) { pm |= 0x04040404; }
						c = spix[sx>>8]; sx += stepx; px |= c2ptable256[0x600 + c]; if (c != 0xFF) { pm |= 0x02020202; }
						c = spix[sx>>8]; sx += stepx; px |= c2ptable256[0x700 + c]; if (c != 0xFF) { pm |= 0x01010101; }
						*encData++ = px;
						*encData++ = pm;
					}
					sy += stepy;
					lines--;
				}
				curDataPtr += (dataSize >> 2);
			}
			// update data offset in frame header
			WRITE_BE_UINT32(&frameHdr[2+shiftIdx], ((uint32)lastDataPtr)-((uint32)dst));
		}
		// this frame is done
		frameHdr += (frameHdrSize >> 2);
	}

	uint32 convertedSize = (uint32)curDataPtr - (uint32)dst;
#ifndef __ATARI__
	if (convertedSize > maxConvertedSize)
	{
		debug(1, "ERROR: converted bomp too large (%d / %d)", convertedSize, maxConvertedSize);
		exit(0);
	}
#endif
	return convertedSize;
#else // defined(GAME_SAMNMAX) || defined(ENGINE_SCUMMALL)
	return 0;
#endif
}

uint32 AtariResourceConverter::encodeAtariCost2(byte* dst, const byte* src, const byte* pal, uint16 width,  uint16 height)
{
	// line-by-line horizontal rle encoding.
	// line header:
	//		xxxxxxxx : bytes until next line
	//      xxxxxxxx : line width in pixels
	//      xxxxxxxy : initial pixel skip + transparent flag
	#define COST_ATARI2_LINEHEADER_SIZE	3

	#ifndef __ATARI__
		#define COST_ATARI2_VALIDATE(a,b) { if (a > b) { debug ("Cost (" #a ") larger than %d : %d", b, a); exit(0); } }
	#endif

	#define COST_ATARI2_GETCOL() *src++

	// yyyyyxxx	: x = length, y = color
	// if (length is 0, next byte is length)
	#define WRITECOL() \
			if(rep < 8) \
				*dst++ = rep | (curcol << 3); \
			else { \
				if (rep > 255) \
				{ \
					*dst++ = (curcol << 3); \
					*dst++ = rep - 255; \
					*dst++ = (curcol << 3); \
					*dst++ = 255; \
				} \
				else \
				{ \
					*dst++ = (curcol << 3); \
					*dst++ = rep; \
				} \
			} \
			pixelcount += rep; \
		rep = 1;

	byte* dstStart = dst;
	for (uint16 y=0; y<height; y++)
	{
		uint16 pixelcount = 0;
		byte transp = 0;
		byte* lineptr = dst;
		dst += COST_ATARI2_LINEHEADER_SIZE;
		byte curcol = COST_ATARI2_GETCOL();
		byte pixelstart = (curcol == 0) ? 0x80 : 0;
		uint16 rep = 1;
		for (uint16 x=0; x<width-1; x++)
		{
			byte newcol = COST_ATARI2_GETCOL();
			if (newcol != curcol)
			{
				if (curcol == 0)
				{
					if (pixelstart & 0x80)
					{
						// skip first transparent
						if (x < 128)
						{
							curcol = newcol;
							pixelstart = x;
							rep = 1;
							continue;
						}
						else
						{
							// skip as many as we can, add dummy.
							pixelstart = 127;
							rep = x - 127;
							transp = 1;
						}
					}
					else
					{
						transp = 1;
					}
				}
				WRITECOL();
				curcol = newcol;
			}
			else
			{
				rep++;
			}
		}

		// skip last transparent
		if (curcol != 0)
		{
			WRITECOL();
		}

		// line header
		uint16 bytelen = (uint32)dst - (uint32)lineptr - COST_ATARI2_LINEHEADER_SIZE;
		*lineptr++ = bytelen;						// length in bytes
		*lineptr++ = pixelcount; 					// line width
		*lineptr++ = (pixelstart << 1) | transp;	// line start + transparent flag
		#ifndef __ATARI__
		COST_ATARI2_VALIDATE(bytelen, 254);
		COST_ATARI2_VALIDATE(pixelcount, 255);
		COST_ATARI2_VALIDATE(pixelcount, width);
		#endif	
	}
	*dst++ = 255;	// end of data
	*dst++ = 0;
	*dst++ = 0;
	return (uint32)(dst - dstStart);
}

bool AtariResourceConverter::convertPICT(ConvCost* pict, byte format)
{
	pict->newOffs = pict->oldOffs;
	pict->newData = 0;
	pict->newSize = 64;

	byte* oldPtr = pict->oldData;
	uint16 width  = READ_LE_UINT16(oldPtr + 0);
	uint16 height = READ_LE_UINT16(oldPtr + 2);

	int16  rel_x  = (int16) READ_LE_UINT16(oldPtr + 4);
	int16  rel_y  = (int16) READ_LE_UINT16(oldPtr + 6);
	int16  move_x = (int16) READ_LE_UINT16(oldPtr + 8);
	int16  move_y = (int16) READ_LE_UINT16(oldPtr + 10);

	debug(1,"pict: %d,%d,%d,%d,%d,%d, %d,%d, 0x%x", width, height, rel_x, rel_y, move_x, move_y, oldPtr[13], oldPtr[14], format);
	debug(1,"off: %x (%d), ptr: %x (%d)", pict->oldOffs, pict->oldOffs, pict->oldData, pict->oldData);

	if((format & 0x7E) == 0x60)
	{
		if ((oldPtr[12] != 0xFF) || (oldPtr[13] != 0xFF))
		{
			pict->newSize = 14;
			pict->newData = (byte*)malloc(pict->newSize);
			memcpy(pict->newData, pict->oldData, pict->newSize);
			return true;
		}
		oldPtr += 2;
	}
	oldPtr += 12;

	// decode picture
	byte shift,mask,maxrep;
	if (format & 1)
	{
		// 32 color mode
		shift  = 3;
		mask   = 0x7;
		maxrep = 8;
	}
	else
	{
		// 16 color mode
		shift = 4;
		mask  = 0xF;
		maxrep = 16;
	}

	byte* src = oldPtr;
	byte* dst = tempScreen;
	uint16 x = 0;
	uint16 y = 0;
	while(x < width)
	{
		byte rep = *src++;
		byte col = rep >> shift;
		rep &= mask;
		if (rep == 0)
			rep = *src++;
		while (rep > 0)
		{
			*dst = col;
			dst += width;
			rep--;
			y++;
			if (y >= height)
			{
				x++;
				if (x >= width)
					break;

				dst = tempScreen + x;
				y = 0;
			}
		}
	}

#if (ATARI_SPRITE_VER == 2)
	{
		src = tempScreen;
		byte* rle = tempScreen + (320 * 200);
		uint32 rleSize;

		rleSize = encodeAtariCost2(rle, src, 0, width, height);

		uint32 hdrSize;
		if (g_scumm->_version >= 6)
			hdrSize = 14;
		else
			hdrSize = oldPtr - pict->oldData; 

		uint32 padSize = (2 - ((hdrSize + rleSize) & 1)) & 1;
		pict->newSize = hdrSize + rleSize + padSize;
		pict->newData = (byte*)malloc(pict->newSize);
		debug(1, "newsize = %d+%d = %d. mem = %x", hdrSize, rleSize, pict->newSize, pict->newData);
		memcpy(pict->newData, pict->oldData, hdrSize);
		if (g_scumm->_version >= 6)
		{
			if((format & 0x7E) != 0x60)
			{
				pict->newData[12] = 0xFF;
				pict->newData[13] = 0xFF;
			}
		}
		memcpy(pict->newData+hdrSize, rle, rleSize);
	}
#endif // ATARI_SPRITE_VER

	debug(1, "done");
	return true;
}

bool AtariResourceConverter::writeCOST(byte roomNo, uint32 blocktype, uint32 blocksize)
{
	#define writeUnmodifiedInputCost()	{ f2.write(blockPtr+8, blocksize-8); free(blockPtr); writeBlockSize(costBlockPos); }

	uint32 costBlockPos = f2.pos();
	f2.writeUint32(MKID('COST'));
	f2.writeUint32(0);

	// load cost to memory
	byte* blockPtr = (byte*)malloc(blocksize);
	f1.seek(-8, SEEK_CUR);
	f1.read(blockPtr, blocksize);

	byte* ptr;
	byte* costPtr = blockPtr + 8;
	//uint32 costSize = READ_LE_UINT32(costPtr + 0);
	//uint16 costHdr  = READ_LE_UINT16(costPtr + 4);
	byte* basePtr = costPtr + ((g_scumm->_version == 6) ? 0 : -6);
	//debug(1,"costPtr=%x, basePtr=%x", costPtr, basePtr);

	byte numColors = 16;
	//byte numAnims = basePtr[6];
	byte format = basePtr[7] & 0x7F;
	//bool mirror = (basePtr[7] & 0x80) != 0;
	switch (format) {
		case 0x58:	numColors = 16;	break;
		case 0x59:	numColors = 32;	break;
		case 0x60:	numColors = 16;	break;
		case 0x61:	numColors = 32;	break;
	}


	//byte* palPtr 			= basePtr + 8;											// 8 * numColors
	byte* limbsOffsetsPtr	= basePtr + 8 + numColors + 2;							// 16le * 16
	//byte* animOffsetsPtr	= basePtr + 8 + numColors + 2 + (16 * 2);				// 16le * numAnims
	//uint32 cmdOffs 			= READ_LE_UINT16(basePtr + 8 + numColors);
	//uint32 cmdSize 			= READ_LE_UINT16(limbsOffsetsPtr) - cmdOffs;
	//byte* cmdPtr 			= basePtr + READ_LE_UINT16(basePtr + 8 + numColors);	// ptr to anim cmds array


	//debug(1, "\n\rloadCostum '%s':%d, %d, '%c', '%c'", tag2str(READ_UINT32(blockPtr)), READ_BE_UINT32(blockPtr+4), READ_LE_UINT32(costPtr), basePtr[4], basePtr[5]);
	//debug(1, "  f:0x%02x, a:%d, c:%d", format, numAnims, numColors);
	//debug(1, "cmdOffs=%d, cmdSize=%d", cmdOffs, cmdSize);

	// check used limbs
/*
	uint16 allLimbsMask = 0;
	byte* ptr = animOffsetsPtr;
	for (uint16 i=0; i<numAnims; i++)
	{
		uint16 animOffset = READ_LE_UINT16(ptr);
		uint16 limbMask = READ_LE_UINT16(basePtr + animOffset);
		ptr += 2;
		if (animOffset != 0)
			allLimbsMask |= limbMask;
	}

	// ignore if no limbs
	if (allLimbsMask == 0)
	{
		writeUnmodifiedInputCost();
		return true;
	}

	byte numLimbs = 0;
	byte firstLimb = 0;
	for (uint16 i=0; i<16; i++)
	{
		if (allLimbsMask & (1<<i))
		{
			firstLimb = (15-i);
			numLimbs++;
		}
	}
	debug(1, "allLimbsMask = 0x%04x (%d). First=%d, Num=%d", allLimbsMask, allLimbsMask, firstLimb, numLimbs);
*/
	uint16 picTableFirst = 0xFFFF;
	uint16 picTableLast	 = 0;
	ptr = limbsOffsetsPtr;
	for (byte i=0; i<16; i++)
	{
		uint16 offs    = READ_LE_UINT16(ptr); ptr += 2;
		picTableFirst  = MIN<uint16>(picTableFirst, offs);
		picTableLast   = MAX<uint16>(picTableLast, offs);
	}
	//debug(1,"first = %d, last = %d", picTableFirst, picTableLast);

	// check first pic offset for first limb
	uint16 picDataStart = READ_LE_UINT16(basePtr + picTableFirst);
	//debug(1, "First pic of first limb = 0x%04x, %d", picDataStart, picDataStart);

	// first pic offset was invalid
	if (picDataStart == 0)
	{
		byte* ptr = basePtr + picTableFirst;
		if (picTableLast > picTableFirst)
		{
			// check known table size
			for (uint16 i=0; i<(picTableLast - picTableFirst) >> 1; i++)
			{
				uint16 offs = READ_LE_UINT16(ptr); ptr += 2;
				if (offs != 0)
					picDataStart = (picDataStart == 0) ? offs : MIN<uint16>(picDataStart, offs);
			}
		}
		else
		{
			// there's only one table and we don't know the size...
			// check max 8 entries until we hit a valid pic offset
			for(uint16 i=0; i<8; i++)
			{
				uint16 offs = READ_LE_UINT16(ptr); ptr += 2;
				if (offs != 0)
				{
					picDataStart = offs;
					break;
				}
			}
		}
	}


	picTableLast = picDataStart;
	debug(1, "PicDataStart = 0x%04x, %d", picDataStart, picDataStart);
	debug(1, "pict offsets = %d - %d", picTableFirst, picDataStart);

	if ((picDataStart == 0) || (picTableFirst >= picTableLast))
	{
		debug(1, "failed pic offset");
		exit(0);
		writeUnmodifiedInputCost();
		return true;
	}

	// align
	uint32 maxCostOffs = (1<<16)-1;
	if (g_scumm->_gameId == GID_SAMNMAX)
	{
		picDataStart = (picDataStart + 1) & ~1;
		maxCostOffs = (1<<17)-1;
	}

	// convert all costumes
	bool failed = false;
	byte numConvertedCostumes = 0;
	ptr = basePtr + picTableFirst;
	uint32 offsCounter = picDataStart;

	debug(1, "PicTable: %x,%x - %d, %d", picTableFirst, picTableLast, picTableLast-picTableFirst, (picTableLast-picTableFirst)>>1 );

	for (uint16 i=0; i<(picTableLast - picTableFirst)>>1; i++)
	{
		ConvCost* c = 0;
		uint16 offs = READ_LE_UINT16(ptr);
		if (offs == 0)
		{
			ptr += 2;
			continue;
		}

		for (uint16 j=0; j<numConvertedCostumes; j++)
		{
			if (convCost[j].oldOffs == offs)
			{
				c = &convCost[j];
				break;
			}
		}

		if (c == 0)
		{
			c = &convCost[numConvertedCostumes];
			c->oldOffs = offs;
			c->oldData = basePtr + offs;
			c->newData = 0;


			debug(1,"Converting cost %d at offs %d format %d", numConvertedCostumes, c->oldOffs, format);
			if (!convertPICT(c, format))
			{
				debug(0,"Failed converting cost %d (%x) at offs %d", numConvertedCostumes, format, c->oldOffs);
				failed = true;
				break;
			}
			debug(1,"Converted.");

			c->newOffs = offsCounter;
			offsCounter += c->newSize;

			if (offsCounter >= maxCostOffs)
			{
				debug(0,"Failed converting cost %d (%x), because offsCounter %d >= %d", numConvertedCostumes, format, offsCounter, maxCostOffs);
				failed = true;
				break;
			}

			debug(1,"* converted pict %d. %d -> %d", numConvertedCostumes, c->oldOffs, c->newOffs);
			numConvertedCostumes++;
		}
		ptr += 2;
	}

	// use old data on failures
	if (failed)
	{
//#ifndef GAME_SAMNMAX
#if 1
		debug(0," Fatal failure converting costume for %s", g_scumm->_gameName);
		exit(0);
#else
		debug(0," (Fallback to non-optimized costume format)");
		writeUnmodifiedInputCost();

		// free pict datas
		debug(1,"free picts %d", numConvertedCostumes);
		for (int i=numConvertedCostumes-2; i>=0; i--)
		{
			if(convCost[i].newData)
			{
				free(convCost[i].newData);
				convCost[i].newData = 0;
			}
		}
		return true;
#endif
	}

	// update pict offset tables
	ptr = basePtr + picTableFirst;
	for (uint16 i=0; i<(picTableLast - picTableFirst)>>1; i++)
	{
		uint16 offs = READ_LE_UINT16(ptr);
		if (offs != 0)
		{
			for (uint16 j=0; j<numConvertedCostumes; j++)
			{
				if (convCost[j].oldOffs == offs)
				{
					if (g_scumm->_gameId == GID_SAMNMAX)
					{
						#ifndef __ATARI__
						if (convCost[j].newOffs & 1)
						{
							error("cost offset is not aligned (%d)", convCost[j].newOffs);
							exit(0);
						}
						#endif
						WRITE_LE_UINT16(ptr, convCost[j].newOffs >> 1);
					}
					else
						WRITE_LE_UINT16(ptr, convCost[j].newOffs);
					break;
				}
			}
		}
		ptr += 2;
	}

	// update format
	basePtr[7] = (basePtr[7] & 0x81) | 0x40;

	// write data up until picts
	uint32 baseSize = (basePtr + picDataStart) - blockPtr;
	f2.write(blockPtr+8, baseSize-8);
	
	// write picts
	for (byte i=0; i<numConvertedCostumes; i++)
		f2.write(convCost[i].newData, convCost[i].newSize);

	// update block size
	writeBlockSize(costBlockPos);

	// free pict datas
	for (int16 i=numConvertedCostumes-1; i >= 0; i--)
		if (convCost[i].newData)
			free(convCost[i].newData);

	// free original block
	free(blockPtr);
	return true;
}

bool AtariResourceConverter::writeIMnn(byte roomNo, byte imNo, const byte* ptrIMnn, uint16 stripCount, uint16 stripHeight, uint16 zCount)
{
	// so.. ScummVM saves the actual image resources for the inventory into the savegame
	// files -> we can't easily fix this issue without invalidating old savegame files.
	bool wrongStripSize = true;
#if defined(ATARI_OBIM_RESOURCE_SIZE_FIX)
	wrongStripSize = false;
#elif defined(ATARI_OBIM_RESOURCE_SIZE_FIX_PARTIAL)
	if (imNo == 0)
		wrongStripSize = false;
#elif defined(ATARI_OBIM_RESOURCE_SIZE_FIX_SAMNMAX_ONLY)
	if (g_scumm->_gameId == GID_SAMNMAX)
		wrongStripSize = false;
#endif

	const uint32 maskSize  = (stripHeight + 3) & ~3;
	const uint32 stripSize = wrongStripSize ? (stripHeight << 3) : (stripHeight << 2);

	const byte* ptrSMAP = findResource(MKID('SMAP'), ptrIMnn);
	const byte* ptrBOMP = findResource(MKID('BOMP'), ptrIMnn);
	if(ptrSMAP == 0 && ptrBOMP == 0)
	{
		debug(1,"NullPtr SMAP/BOMP %d, %d, %d, %d", imNo, stripCount, stripHeight, zCount);
		return false;
	}

	byte* tempData = tempScreen;
	byte* tempPtr = tempData;
	WRITE_UINT32(tempPtr, imnn_tags[imNo]);
	tempPtr += 8;
	// BOMP
	
	if (ptrBOMP)
	{
		const byte* bompData = ptrBOMP + 18;
		uint32 bompDataSize  = READ_BE_UINT32(ptrBOMP + 4) - 18;
		uint16 bompHdrUnk  	 = READ_LE_UINT16(ptrBOMP + 8);
		uint16 bompHdrWidth  = READ_LE_UINT16(ptrBOMP + 10);
		uint16 bompHdrHeight = READ_LE_UINT16(ptrBOMP + 12);
		uint16 bompHdrPad0	 = READ_LE_UINT16(ptrBOMP + 14);
		uint16 bompHdrPad1	 = READ_LE_UINT16(ptrBOMP + 16);
		byte* bompBlockPtr = tempPtr;
		tempPtr += 18;
		uint32 atariSize = convertBOMP(roomNo, tempPtr, bompData, bompHdrWidth, bompHdrHeight);
		tempPtr += atariSize;
		debug(1, "Convert BOMP: %d,%d  %d->%d", bompHdrWidth, bompHdrHeight, bompDataSize, atariSize);
		assertAligned(tempPtr);
		WRITE_UINT32(bompBlockPtr +  0, MKID('BOMP'));
		WRITE_BE_UINT32(bompBlockPtr +  4, tempPtr - bompBlockPtr);
		WRITE_BE_UINT16(bompBlockPtr +  8, bompHdrUnk);
		WRITE_BE_UINT16(bompBlockPtr + 10, bompHdrWidth);
		WRITE_BE_UINT16(bompBlockPtr + 12, bompHdrHeight);
		WRITE_BE_UINT16(bompBlockPtr + 14, bompHdrPad0);
		WRITE_BE_UINT16(bompBlockPtr + 16, bompHdrPad1);
	}
	else
	{
		// SMAP
		{
			byte palBackup[256*3];
			bool specialCursor = false;
			if (g_scumm->_gameId == GID_SAMNMAX && roomNo == 84 && stripCount < 8)
			{
				specialCursor = true;
				memcpy(palBackup, g_scumm->_currentPalette, 256*3);
			}

			byte* smapBlockPtr = tempPtr;
			WRITE_UINT32(tempPtr, MKID('SMAP'));
			tempPtr += 8;
			byte* stripOffsPtr = tempPtr;
			for (int i=0; i<stripCount; i++)
			{
				WRITE_LE_UINT32(tempPtr, 0);
				tempPtr += 4;
			}
			uint32 stripOffset = 8 + (4 * stripCount) + 3;
			for (int i=0; i<stripCount; i++)
			{
				// decode strip
				*tempPtr++ = 0;
				*tempPtr++ = 0;
				*tempPtr++ = specialCursor ? 2 : 0;
				byte codec = g_scumm->gdi.convertBitmap(tempPtr+1, (const byte*) (ptrSMAP + READ_LE_UINT32(ptrSMAP + (i << 2) + 8)), (int)stripHeight);
				bool transp = (codec & 1) ? true : false;
				uint32 size = transp ? (stripSize + maskSize) : (stripSize);
				*tempPtr++ = codec;
				tempPtr += size;
				assertAligned(size);
				assertAligned(tempPtr);

				if (specialCursor)
				{
					// build cursor mask (normal)
					bool ditherWasEnabled = ditherEnabled;
					memset(g_scumm->_currentPalette, 255, 256*3);
					g_scumm->_currentPalette[(255*3)+0] = 0; g_scumm->_currentPalette[(255*3)+1] = 0; g_scumm->_currentPalette[(255*3)+2] = 0;
					g_scumm->_currentPalette[(  0*3)+0] = 0; g_scumm->_currentPalette[(  0*3)+1] = 0; g_scumm->_currentPalette[(  0*3)+2] = 0;
					g_scumm->_currentPalette[(  6*3)+0] = 0; g_scumm->_currentPalette[(  6*3)+1] = 0; g_scumm->_currentPalette[(  6*3)+2] = 0;
					g_scumm->_currentPalette[(176*3)+0] = 0; g_scumm->_currentPalette[(176*3)+1] = 0; g_scumm->_currentPalette[(176*3)+2] = 0;
					g_scumm->_currentPalette[(178*3)+0] = 0; g_scumm->_currentPalette[(178*3)+1] = 0; g_scumm->_currentPalette[(178*3)+2] = 0;
					g_scumm->_currentPalette[(180*3)+0] = 0; g_scumm->_currentPalette[(180*3)+1] = 0; g_scumm->_currentPalette[(180*3)+2] = 0;
					enableDither(false);
					g_scumm->gdi.convertBitmap(tempPtr, (const byte*) (ptrSMAP + READ_LE_UINT32(ptrSMAP + (i << 2) + 8)), (int)stripHeight);
					for (int j=0; j<stripHeight; j++)
						tempPtr[j] = tempPtr[j<<2];
					tempPtr += maskSize;
					size += maskSize;
					assertAligned(maskSize);
					assertAligned(tempPtr);

					// build cursor mask (interaction)
					g_scumm->_currentPalette[(180*3)+0] = 255; g_scumm->_currentPalette[(180*3)+1] = 255; g_scumm->_currentPalette[(180*3)+2] = 255;
					enableDither(false);
					g_scumm->gdi.convertBitmap(tempPtr, (const byte*) (ptrSMAP + READ_LE_UINT32(ptrSMAP + (i << 2) + 8)), (int)stripHeight);
					for (int j=0; j<stripHeight; j++)
						tempPtr[j] = tempPtr[j<<2];
					tempPtr += maskSize;
					size += maskSize;
					assertAligned(maskSize);
					assertAligned(tempPtr);

					// restore palette
					memcpy(g_scumm->_currentPalette, palBackup, 256*3);
					enableDither(ditherWasEnabled);			
				}

				assertAligned(size);
				WRITE_LE_UINT32(stripOffsPtr + (i << 2), stripOffset);
				stripOffset += (size + 4);
			}
			assertAligned(tempPtr);
			WRITE_BE_UINT32(&smapBlockPtr[4], tempPtr - smapBlockPtr);
		}
		// ZPNN
		for (int i=1; i<=zCount; i++)
		{
			uint32 tag = zpnn_tags[i];
			const byte* zplane_ptr = findResource(tag, ptrIMnn);
			uint32 size = READ_BE_UINT32(zplane_ptr + 4);
			WRITE_UINT32(&tempPtr[0], tag);
			WRITE_BE_UINT32(&tempPtr[4], size);
			memcpy(&tempPtr[8], zplane_ptr+8, size-8);
			tempPtr += size;
			//assertAligned(size);
			//assertAligned(tempPtr);
		}
	}
	uint32 size = tempPtr - tempData;
	//assertAligned(size);
	//assertAligned(tempPtr);
	WRITE_BE_UINT32(&tempData[4], size);


	uint32 offs = f2.pos() - roomOffs[roomNo];
	assertAligned(offs);
	//assertAligned(size);
	f2.write(tempData, size);

	debug(1, "*** size = %d", size);
	if (size > (320*200*7) /*+ (320*200) - (8 * 200)*/)
		exit(0);

	return true;
}

bool AtariResourceConverter::writeOBIM(byte roomNo, byte* ptrOBIM)
{
	// OBIM
	/*
	const byte* ptrBOMP = findResource(MKID('BOMP'), ptrOBIM);
	if (ptrBOMP)
		return false;
	*/
	uint32 obimBlockPos = f2.pos();
	f2.writeUint32(MKID('OBIM'));
	f2.writeUint32BE(0);

	// IMHD
	// not according to spec, but we pad the header to make the rest
	// of the OBIM 32bit aligned to start of room block
	const byte* ptrIMHD = findResource(MKID('IMHD'), ptrOBIM);
	uint32 hdrsize = READ_BE_UINT32(ptrIMHD+4);

	uint32 startoffs = (f2.pos() + hdrsize) - roomOffs[roomNo];
	uint32 padding = (4 - (startoffs & 3)) & 3;
	f2.writeUint32(MKID('IMHD'));
	f2.writeUint32BE(hdrsize + padding);
	f2.write(ptrIMHD + 8, hdrsize - 8);
	for(uint i=0; i<padding; i++)
		f2.writeByte(0);

	uint16 numIMnn = READ_LE_UINT16(ptrIMHD + 10);
	uint16 numZPnn = READ_LE_UINT16(ptrIMHD + 12);
	uint16 width   = READ_LE_UINT16(ptrIMHD + 20);
	uint16 height  = READ_LE_UINT16(ptrIMHD + 22);

	bool oldDitherEnabled = ditherEnabled;
	switch(g_scumm->_gameId)
	{
		case GID_MONKEY:
			// inventory items in 98
			if (roomNo == 98)
				enableDither(false);
			break;
		case GID_MONKEY2:
			// inventory items in 93
			if (roomNo == 93)
				enableDither(false);
			break;
		case GID_INDY4:
			// inventory items in 95
			if (roomNo == 95)
				enableDither(false);
			break;
		case GID_TENTACLE:
			// inventory items in 59
			if (roomNo == 59)
				enableDither(false);
			break;
		case GID_SAMNMAX:
			// inventory items in 84, other cursors in 83
			if (roomNo == 83 || roomNo == 84)
				enableDither(false);
			break;
	};

	for (int i=1; i<numIMnn+1; i++)
	{
		const byte* ptrIMnn = findResource(imnn_tags[i], ptrOBIM);
		writeIMnn(roomNo, i, ptrIMnn, width>>3, height, numZPnn);
	}

	if (ditherEnabled != oldDitherEnabled)
		enableDither(oldDitherEnabled);

	writeBlockSize(obimBlockPos);
	return true;
}

bool AtariResourceConverter::writeRMIM(byte roomNo, byte* ptrROOM, uint16 width, uint16 height)
{
	const byte* ptrRMIM = findResource(MKID('RMIM'), ptrROOM);
	const byte* ptrRMIH = findResource(MKID('RMIH'), ptrRMIM);
	const byte* ptrIM00 = findResource(MKID('IM00'), ptrRMIM);
	byte zbufCount = READ_LE_UINT16(ptrRMIH + 8);
	byte stripCount = (width >> 3);
	uint32 stripHeight = height;

	// RMIM
	{
		uint32 rmimBlockPos = f2.pos();
		f2.writeUint32(MKID('RMIM'));
		f2.writeUint32BE(0);

		// RMIH
		{
			// not according to spec, but we pad the header to make the rest
			// of the RMIM 32bit aligned to start of room block
			uint32 roomStartOffs = (f2.pos() + 10) - roomOffs[roomNo];
			uint32 padding = (4 - (roomStartOffs & 3)) & 3;
			f2.writeUint32(MKID('RMIH'));
			f2.writeUint32BE(10 + padding);
			f2.writeUint16LE(zbufCount);
			for(uint i=0; i<padding; i++)
				f2.writeByte(0);
		}

		// write IM00 block
		writeIMnn(roomNo, 0, ptrIM00, stripCount, stripHeight, zbufCount);

		// update RMIM block size
		writeBlockSize(rmimBlockPos);
	}
	return true;
}

uint32 AtariResourceConverter::colorWeight(int16 r, int16 g, int16 b)
{
	int r2 = r < 0 ? -r : r;
	int g2 = g < 0 ? -g : g;
	int b2 = b < 0 ? -b : b;
	if(r2 > 255) r = 255;
	if(g2 > 255) g = 255;
	if(b2 > 255) b = 255;
	r2 = MULSQUARE(r2);
	g2 = MULSQUARE(g2);
	b2 = MULSQUARE(b2);
	return MUL3(r2) + MUL6(g2) + MUL2(b2);	
}

void AtariResourceConverter::findClosestColors(byte r, byte g, byte b, byte* c0, byte* c1)
{
	byte bestitem = 0;
	uint bestsum = (uint) - 1;
	int cost = 0;

	for (uint16 i = 0; i < 256; i++) {
		int ar = virtualPal[(i*4) + 0];
		int ag = virtualPal[(i*4) + 1];
		int ab = virtualPal[(i*4) + 2];
		cost = virtualPal[(i*4) + 3];
		if (ar == r && ag == g && ab == b)
		{
			bestitem = i;
			break;
		}
	
		int r2 = ar > r ? ar - r : r - ar;
		int g2 = ag > g ? ag - g : g - ag;
		int b2 = ab > b ? ab - b : b - ab;
		r2 = MULSQUARE(r2);
		g2 = MULSQUARE(g2);
		b2 = MULSQUARE(b2);

		// color diff
		uint32 sum = MUL3(r2) + MUL6(g2) + MUL2(b2);

		// dither cost
		sum += (cost << 8);
	
		if (sum < bestsum) {
			bestsum = sum;
			bestitem = i;
		}
	}

	*c0 = (bestitem & 0xF);
	*c1 = (bestitem >> 4) & 0xF;
}


void AtariResourceConverter::buildC2PTable()
{
	byte* data = g_scumm->_currentPalette;
	uint32* c2p0 = (uint32*) &c2ptable256[0];
	uint32* c2p1 = (uint32*) &c2ptable256[256*8];

	for (int i=0; i<256; ++i)
	{
		byte r = *data++;
		byte g = *data++;
		byte b = *data++;

		byte c0, c1;
		findClosestColors(r, g, b, &c0, &c1);

#ifdef __ATARI__
		c2p0[0 * 256] = ((c0 & 1) << 31) | ((c0 & 2) << 22) | ((c0 & 4) << 13) | ((c0 & 8) << 4);
		c2p0[1 * 256] = ((c0 & 1) << 30) | ((c0 & 2) << 21) | ((c0 & 4) << 12) | ((c0 & 8) << 3);
		c2p0[2 * 256] = ((c0 & 1) << 29) | ((c0 & 2) << 20) | ((c0 & 4) << 11) | ((c0 & 8) << 2);
		c2p0[3 * 256] = ((c0 & 1) << 28) | ((c0 & 2) << 19) | ((c0 & 4) << 10) | ((c0 & 8) << 1);
		c2p0[4 * 256] = ((c0 & 1) << 27) | ((c0 & 2) << 18) | ((c0 & 4) <<  9) | ((c0 & 8)     );
		c2p0[5 * 256] = ((c0 & 1) << 26) | ((c0 & 2) << 17) | ((c0 & 4) <<  8) | ((c0 & 8) >> 1);
		c2p0[6 * 256] = ((c0 & 1) << 25) | ((c0 & 2) << 16) | ((c0 & 4) <<  7) | ((c0 & 8) >> 2);
		c2p0[7 * 256] = ((c0 & 1) << 24) | ((c0 & 2) << 15) | ((c0 & 4) <<  6) | ((c0 & 8) >> 3);
		c2p1[0 * 256] = ((c1 & 1) << 31) | ((c1 & 2) << 22) | ((c1 & 4) << 13) | ((c1 & 8) << 4);
		c2p1[1 * 256] = ((c1 & 1) << 30) | ((c1 & 2) << 21) | ((c1 & 4) << 12) | ((c1 & 8) << 3);
		c2p1[2 * 256] = ((c1 & 1) << 29) | ((c1 & 2) << 20) | ((c1 & 4) << 11) | ((c1 & 8) << 2);
		c2p1[3 * 256] = ((c1 & 1) << 28) | ((c1 & 2) << 19) | ((c1 & 4) << 10) | ((c1 & 8) << 1);
		c2p1[4 * 256] = ((c1 & 1) << 27) | ((c1 & 2) << 18) | ((c1 & 4) <<  9) | ((c1 & 8)     );
		c2p1[5 * 256] = ((c1 & 1) << 26) | ((c1 & 2) << 17) | ((c1 & 4) <<  8) | ((c1 & 8) >> 1);
		c2p1[6 * 256] = ((c1 & 1) << 25) | ((c1 & 2) << 16) | ((c1 & 4) <<  7) | ((c1 & 8) >> 2);
		c2p1[7 * 256] = ((c1 & 1) << 24) | ((c1 & 2) << 15) | ((c1 & 4) <<  6) | ((c1 & 8) >> 3);
#else
		c2p0[0 * 256] = ((c0 & 1) << 7) | ((c0 & 2) << 14) | ((c0 & 4) << 21) | ((c0 & 8) << 28);
		c2p0[1 * 256] = ((c0 & 1) << 6) | ((c0 & 2) << 13) | ((c0 & 4) << 20) | ((c0 & 8) << 27);
		c2p0[2 * 256] = ((c0 & 1) << 5) | ((c0 & 2) << 12) | ((c0 & 4) << 19) | ((c0 & 8) << 26);
		c2p0[3 * 256] = ((c0 & 1) << 4) | ((c0 & 2) << 11) | ((c0 & 4) << 18) | ((c0 & 8) << 25);
		c2p0[4 * 256] = ((c0 & 1) << 3) | ((c0 & 2) << 10) | ((c0 & 4) << 17) | ((c0 & 8) << 24);
		c2p0[5 * 256] = ((c0 & 1) << 2) | ((c0 & 2) <<  9) | ((c0 & 4) << 16) | ((c0 & 8) << 23);
		c2p0[6 * 256] = ((c0 & 1) << 1) | ((c0 & 2) <<  8) | ((c0 & 4) << 15) | ((c0 & 8) << 22);
		c2p0[7 * 256] = ((c0 & 1)     ) | ((c0 & 2) <<  7) | ((c0 & 4) << 14) | ((c0 & 8) << 21);	
		c2p1[0 * 256] = ((c1 & 1) << 7) | ((c1 & 2) << 14) | ((c1 & 4) << 21) | ((c1 & 8) << 28);
		c2p1[1 * 256] = ((c1 & 1) << 6) | ((c1 & 2) << 13) | ((c1 & 4) << 20) | ((c1 & 8) << 27);
		c2p1[2 * 256] = ((c1 & 1) << 5) | ((c1 & 2) << 12) | ((c1 & 4) << 19) | ((c1 & 8) << 26);
		c2p1[3 * 256] = ((c1 & 1) << 4) | ((c1 & 2) << 11) | ((c1 & 4) << 18) | ((c1 & 8) << 25);
		c2p1[4 * 256] = ((c1 & 1) << 3) | ((c1 & 2) << 10) | ((c1 & 4) << 17) | ((c1 & 8) << 24);
		c2p1[5 * 256] = ((c1 & 1) << 2) | ((c1 & 2) <<  9) | ((c1 & 4) << 16) | ((c1 & 8) << 23);
		c2p1[6 * 256] = ((c1 & 1) << 1) | ((c1 & 2) <<  8) | ((c1 & 4) << 15) | ((c1 & 8) << 22);
		c2p1[7 * 256] = ((c1 & 1)     ) | ((c1 & 2) <<  7) | ((c1 & 4) << 14) | ((c1 & 8) << 21);	
#endif
		c2p0++;
		c2p1++;
	}
}

void AtariResourceConverter::enableDither(bool enable)
{
	if (enable)
	{
		buildC2PTable();
	}
	else
	{
		g_scumm->mapSystemPalette(0, 255);
		uint32* c2p0 = (uint32*) &c2ptable256[0];
		uint32* c2p1 = (uint32*) &c2ptable256[256*8];		
		memcpy(c2p1, c2p0, 256*8*4);
	}
	ditherEnabled = enable;
}

bool AtariResourceConverter::writeROOM(byte roomNo, uint32 blocktype, uint32 blocksize)
{
	uint32 roomBlockFileSizeOld = blocksize;
	uint32 roomBlockFilePos = f2.pos();
	f2.writeUint32(MKID('ROOM'));
	f2.writeUint32BE(0);

	// load room
	byte* ptrROOM = (byte*) malloc(roomBlockFileSizeOld);
	if (!ptrROOM)
		return false;

	f1.seek(-8, SEEK_CUR);
	f1.read(ptrROOM, blocksize);

	const byte* ptrRMHD = findResource(MKID('RMHD'), ptrROOM);
	uint16 roomWidth  = READ_LE_UINT16(ptrRMHD + 8 + 0);
	uint16 roomHeight = READ_LE_UINT16(ptrRMHD + 8 + 2);
	uint16 numObjs    = READ_LE_UINT16(ptrRMHD + 8 + 4);

	debug(1, "room:%d : %dx%d, %d", roomNo, roomWidth, roomHeight, numObjs);

	// Transparent color
	const byte* ptrTRNS = findResource(MKID('TRNS'), ptrROOM);
	g_scumm->gdi._transparentColor = ptrTRNS ? ptrTRNS[8] : 255;

	// Palette
	for (uint i = 0; i < 256; i++) {
		g_scumm->_roomPalette[i] = i;
	}

	const byte* ptrCLUT = findResource(MKID('CLUT'), ptrROOM);
	if (ptrCLUT)
	{
		g_scumm->setPaletteFromPtr(&ptrCLUT[8]);
	}
	else if (g_scumm->_version == 6)
	{
		const byte* ptrPALS = findResource(MKID('PALS'), ptrROOM);
		if (ptrPALS)
		{
			const byte* ptrWRAP = findResource(MKID('WRAP'), ptrPALS);
			if(ptrWRAP)
			{
				const byte* ptrAPAL = findResource(MKID('APAL'), ptrWRAP);
				{
					const byte* ptr = ptrAPAL + 8;
					byte* dst = g_scumm->_currentPalette;
					for (uint32 i = 0; i < 256; i++) {
						byte r = *ptr++; byte g = *ptr++; byte b = *ptr++;
						if (i <= 15 || r < 252 || g < 252 || b < 252) {
							*dst++ = r; *dst++ = g; *dst++ = b;
						} else {
							dst += 3;
						}
					}
				}
				/*
				const byte* ptrAPAL2 = ptrAPAL + READ_BE_UINT32(ptrAPAL+4);
				if (READ_UINT32(ptrAPAL2) == MKID('APAL'))
				{
					pal1 = secondaryPalette;
					memcpy(pal1, g_scumm->_currentPalette, 256*3);
					byte* dst = pal1;
					const byte* ptr = ptrAPAL2 + 8;
					for (uint32 i = 0; i < 256; i++) {
						byte r = *ptr++; byte g = *ptr++; byte b = *ptr++;
						if (i <= 15 || r < 252 || g < 252 || b < 252) {
							*dst++ = r; *dst++ = g; *dst++ = b;
						} else {
							dst += 3;
						}
					}
				}
				*/
			}
		}
	}

//#if defined(GAME_MONKEY2)
	if (g_scumm->_gameId == GID_MONKEY2)
	{
		g_scumm->setPalColor(2,  83,   0,  83);
		g_scumm->setPalColor(3, 223,  83, 223);
		g_scumm->setPalColor(1,  23,   0,  23);
		g_scumm->setPalColor(6, 127,  47, 127);
	}
//#elif defined(GAME_MONKEY1)
//#elif defined(GAME_ATLANTIS)
//#elif defined(GAME_TENTACLE)
//#elif defined(GAME_SAMNMAX)
	else if (g_scumm->_gameId == GID_SAMNMAX)
	{
		g_scumm->setPalColor(2,  71,   0,  71);
		g_scumm->setPalColor(3, 255, 139,  39);
		g_scumm->setPalColor(1,   0,   0,   0);
		g_scumm->setPalColor(6,   0,  71, 143);
	}
//#endif	

	bool ditherRoom = true;
 	if (g_scumm->_gameId == GID_SAMNMAX)
	{
		if (roomNo < 6)	// all intro screens
			ditherRoom = false;
	}
	else if (g_scumm->_gameId == GID_TENTACLE)
	{
		if (roomNo == 2 || roomNo == 4 || roomNo == 5 ||	// intro screens
			roomNo == 70)	// cutscene room
			ditherRoom = false;
	}

	g_scumm->mapSystemPalette(0, 255);
	enableDither(ditherRoom);

	byte* readPtr = ptrROOM + 8;
	while(1)
	{
		if (readPtr >= (ptrROOM + roomBlockFileSizeOld))
			break;

		uint32 blocktype = READ_UINT32(readPtr);
		uint32 blocksize = READ_BE_UINT32(readPtr + 4);

		//debug(1, "ROOM block '%s', %d  (%d / %d)", tag2str(blocktype), blocksize, readPtr - ptrROOM, roomBlockFileSizeOld);
		
		switch(blocktype)
		{
			case MKID('RMIM'):
				writeRMIM(roomNo, ptrROOM, roomWidth, roomHeight);
				break;
			
			case MKID('OBIM'):
				if (!writeOBIM(roomNo, readPtr))
					f2.write(readPtr, blocksize);
				break;

			case MKID('OBCD'):
				//assertAligned(blocksize);
				f2.write(readPtr, blocksize);
				break;

			default:
				f2.write(readPtr, blocksize);
				break;
		}
		readPtr += blocksize;
	}

	// update ROOM block size
	writeBlockSize(roomBlockFilePos);
	free(ptrROOM);


	// update progress
	progCur++;
	if (progCur > progMax)
		progCur = progMax;

	g_scumm->_system->installUpdate(progCur, progMax);

	return true;
}

bool AtariResourceConverter::writeLFLF(uint16 resNo, uint32 blocktype, uint32 blocksize)
{
	byte roomNo = getRoomNo(resNo, f1.pos());
	byte roomId = getRoomId(resNo, f1.pos());
	if (roomNo == 0xFF)
		return false;

	uint32 lflfBlockFilePosOld = f1.pos();
	uint32 lflfBlockFileSizeOld = blocksize;
	uint32 lflfBlockFilePos = f2.pos();
	uint32 lflfBlockReadCount = 8;
	f2.writeUint32(MKID('LFLF'));
	f2.writeUint32BE(8);

	// process sub blocks
	while(1)
	{
		if ((lflfBlockReadCount >= lflfBlockFileSizeOld - 8) || !readBlock(blocktype, blocksize))
			break;
		lflfBlockReadCount += blocksize;

		debug(1, "LFLF block '%s'", tag2str(blocktype), blocksize);

		uint32 resDirOldOffs = f1.pos() - 8 - lflfBlockFilePosOld;
		uint32 resDirNewOffs = f2.pos() - 8 - lflfBlockFilePos;

		switch (blocktype)
		{
			case MKID('ROOM'):
				if (!writeRoomOffset(resNo, roomNo, roomId, f2.pos()))
					return false;
				writeROOM(roomNo, blocktype, blocksize);
				break;

			case MKID('COST'):
				if (!updateResDir(rtResDirCostume, resNo, roomId, resDirOldOffs, resDirNewOffs))
					return false;
#if (ATARI_SPRITE_VER == 0)
				copyFileBlock(blocktype, blocksize);
#else
				writeCOST(roomNo, blocktype, blocksize);
#endif				
				break;

			case MKID('SCRP'):
				if (!updateResDir(rtResDirScript, resNo, roomId, resDirOldOffs, resDirNewOffs))
					return false;
				copyFileBlock(blocktype, blocksize);
				break;

			case MKID('SOUN'):
				if (!updateResDir(rtResDirSound, resNo, roomId, resDirOldOffs, resDirNewOffs))
					return false;
				copyFileBlock(blocktype, blocksize);
				break;

			case MKID('CHAR'):
				if (!updateResDir(rtResDirCharset, resNo, roomId, resDirOldOffs, resDirNewOffs))
					return false;
				copyFileBlock(blocktype, blocksize);
				break;

			default:
				copyFileBlock(blocktype, blocksize);
				break;
		};

	}

	// update LFLF block size
	writeBlockSize(lflfBlockFilePos);
	return true;
}

bool AtariResourceConverter::writeResFile(uint16 resNo)
{
	uint32 blocktype, blocksize;

	// read LECF
	if (!readBlock(blocktype, blocksize) || (blocktype != MKID('LECF')))
		return false;

	// read LOFF
	if (!readBlock(blocktype, blocksize) || (blocktype != MKID('LOFF')))
		return false;

	uint32 pos = f1.pos();
	uint32 numNewRooms = f1.readByte();
	for (uint i=numRooms; i<numRooms + numNewRooms; i++)
	{
		roomRes[i] = resNo;
		roomId[i] = f1.readByte();
		roomOffs[i] = f1.readUint32LE();
	}
	numRooms += numNewRooms;
	f1.seek(pos + blocksize - 8, SEEK_SET);

	debug(1, "res %d has %d rooms", resNo, numNewRooms);
	
	// process other blocks
	while(1)
	{
		if (!readBlock(blocktype, blocksize))
			break;

		debug(1, "block '%s', %d", tag2str(blocktype), blocksize);
		if (blocktype == MKID('LFLF'))
		{
			uint32 pos = f1.pos();
			if (!writeLFLF(resNo, blocktype, blocksize))
			{
				closeFiles();
				return false;
			}
			f1.seek(pos + blocksize - 8, SEEK_SET);
		}
		else
		{
			copyFileBlock(blocktype, blocksize);
		}
	}

	closeInFile();
	return true;
}


bool AtariResourceConverter::readIndexFile()
{
	if (!openInFile(0))
		return false;

	uint32 blocktype, blocksize;
	while(1)
	{
		if (!readBlock(blocktype, blocksize))
			break;

		uint32 pos = f1.pos();
		int resDirType = -1;
		switch (blocktype)
		{
			case MKID('DROO'):	resDirType = rtResDirRoom; 		break;
			case MKID('DSCR'):	resDirType = rtResDirScript;	break;
			case MKID('DSOU'):	resDirType = rtResDirSound;		break;
			case MKID('DCOS'):	resDirType = rtResDirCostume;	break;
			case MKID('DCHR'):	resDirType = rtResDirCharset;	break;
			default:											break;
		}
		if (resDirType >= 0)
		{
			ResDir* dir = &resDirs[resDirType];
			dir->count = f1.readUint16LE();
			dir->room 	= (byte*)calloc(dir->count, 1);
			dir->offset = (uint32*)calloc(dir->count, 4);
			dir->valid  = (byte*)calloc(dir->count, 1);
			f1.read(dir->room, dir->count);
			memset(dir->valid, 0, dir->count);
			if (resDirType == rtResDirRoom)
			{
				memset(dir->offset, 0,	dir->count * 4);

				for (uint i=0; i<dir->count; i++)
				{
					debug(1,"## ROOM %d, %d", i, dir->room[i]);
				}
			}
			else
			{
				f1.read(dir->offset, dir->count * 4);
				for(int i=0; i<dir->count; i++)
					dir->offset[i] = FROM_LE_32(dir->offset[i]);
			}
		}
		f1.seek(pos + blocksize - 8, SEEK_SET);
	}
	closeInFile();
	return true;
}


bool AtariResourceConverter::writeIndexFile()
{
	debug(1,"writeIndexFile");
	closeFiles();
	if (!openInFile(0) || !openOutFile(0))
	{
		closeFiles();
		return false;
	}

#if ATARI_RESOURCE_HEADER
	f2.writeUint32(MKID('_ST_'));
	f2.writeUint32BE(12);
	f2.writeUint16BE(ATARI_RESOURCE_VERSION);
	f2.writeUint16BE(0);
#endif
	uint32 blocktype, blocksize;
	while(1)
	{
		if (!readBlock(blocktype, blocksize))
			break;

		int resDirType = -1;
		switch (blocktype)
		{
			case MKID('DROO'):	resDirType = rtResDirRoom; 		break;
			case MKID('DSCR'):	resDirType = rtResDirScript;	break;
			case MKID('DSOU'):	resDirType = rtResDirSound;		break;
			case MKID('DCOS'):	resDirType = rtResDirCostume;	break;
			case MKID('DCHR'):	resDirType = rtResDirCharset;	break;
			default:											break;
		}
		if (resDirType >= 0)
		{
			ResDir* dir = &resDirs[resDirType];
			uint16 count = dir->count;
			uint32 size  = 10 + (count * 5);
			f2.writeUint32(blocktype);
			f2.writeUint32BE(blocksize);
			f2.writeUint16LE(count);

			debug(1, "DIR %d: %d (%d)", resDirType, size, count);
			for (int i=0; i<count; i++)
			{
				debug(1, "  %d, r:%d, o:%d, v:%d", i, dir->room[i], dir->offset[i], dir->valid[i]);
			}

			for (uint16 i=0; i<count; i++)
				f2.writeByte((dir->valid[i] != 0) ? dir->room[i] : 0);

			for (uint16 i=0; i<count; i++)
				f2.writeUint32LE((dir->valid[i] != 0) ? dir->offset[i] : 0);

			for (uint32 i=size; i<blocksize; i++)
				f2.writeByte(0);

			f1.seek(blocksize - 8, SEEK_CUR);
		}
		else
		{
			copyFileBlock(blocktype, blocksize, false);
		}
	}

	closeFiles();
	return true;
}


bool AtariResourceConverter::openInFile(byte resNo)
{
	char fname1[128];
	sprintf(fname1, "%s.%03d", g_scumm->_gameName.c_str(), resNo);
	closeInFile();
	f1.enableFileCache(4096);
	if (!f1.open(fname1, g_scumm->getGameDataPath(), File::kFileReadMode, 0x69))
	{
		//debug("failed opening input resource %d (%s | %s)", resNo, fname1, g_scumm->getGameDataPath());
		return false;
	}
	return true;
}

bool AtariResourceConverter::openOutFile(byte resNo)
{
	char fname2[128];
	sprintf(fname2, "%s.ST%d", g_scumm->_gameName.c_str(), resNo);
	closeOutFile();
	f2.enableFileCache(256);
	if (!f2.open(fname2, g_scumm->getGameDataPath(), File::kFileWriteMode, ATARI_RESOURCE_XOR))
	{
		debug("failed opening output resource %d", resNo);
		return false;
	}
	return true;
}

void AtariResourceConverter::closeInFile()
{
	if (f1.isOpen())
		f1.close();
}

void AtariResourceConverter::closeOutFile()
{
	if(f2.isOpen())
		f2.close();
}


void AtariResourceConverter::closeFiles()
{
	closeInFile();
	closeOutFile();
}

void AtariResourceConverter::writeBlockSize(uint32 blockstart, bool align)
{
	uint32 pos = f2.pos();
	if (align && (pos & 3))
	{
		while (f2.pos() & 3)
		{
			f2.writeByte(0);
		}
		pos = f2.pos();
	}
	uint32 size = pos - blockstart;
	f2.seek(blockstart + 4, SEEK_SET);
	f2.writeUint32BE(size);
	f2.seek(pos, SEEK_SET);
}


bool AtariResourceConverter::readBlock(uint32& blockType, uint32& blockSize)
{
	blockType = f1.readUint32();
	blockSize = f1.readUint32BE();
	return !f1.ioFailed();
} 

bool AtariResourceConverter::copyFileContent(uint32 len)
{
	const uint32 bufSize = 16 * 1024;
	byte* buf = tempScreen;
	while (len > 0)
	{
		uint32 bytes = MIN<uint32>(len, bufSize);
		uint32 bytes_read = f1.read(buf, bytes);
		if (bytes_read < bytes)
		{
			debug(1, "Err: failed read (%d) (%d)", bytes, bytes_read);
			return false;
		}
		if (!f2.write(buf, bytes_read))
		{
			debug(1, "Err: failed write (%d)", bytes);
			return false;
		}

		len -= bytes_read;
	}
	return true;
}


bool AtariResourceConverter::copyFileBlock(uint32 type, uint32 size, bool align)
{
	f2.writeUint32(type);
	f2.writeUint32BE(size);
	copyFileContent(size - 8);
	return true;
}

//-----------------------------------------------------------------
//
// sound converter
//
//-----------------------------------------------------------------
#ifdef ATARI_SOUND_CONVERTER
class AtariSoundConverter
{
public:
	AtariSoundConverter();
	~AtariSoundConverter();
	bool GenerateSoundFile();
private:
	bool writeSounds();
	bool writeMappingTable();
	File f1;
	File f2;
};

AtariSoundConverter::AtariSoundConverter()
{
}

AtariSoundConverter::~AtariSoundConverter()
{
	if (f1.isOpen())
		f1.close();
	if (f2.isOpen())
		f2.close();
}

bool AtariSoundConverter::writeSounds()
{
	// todo:
	// 	unsigned->signed conversion
	// 	convert all sounds to 12517hz ?
	return true;
}

bool AtariSoundConverter::writeMappingTable()
{
	// align
	if (f2.pos() & 1)
		f2.writeByte(0);
	// todo: write sound offset remapping table
	f2.writeUint16BE(0);
	return true;
}

bool AtariSoundConverter::GenerateSoundFile()
{
	static const char magic[4] = {'S','T','A',ATARI_SOUND_VER};

	char outputFilename[64];
	char inputFilename[64];
	sprintf(inputFilename, "%s.SOU", g_scumm->_gameName.c_str());
	sprintf(outputFilename, "%s.STA", g_scumm->_gameName.c_str());

	// already converted?
#ifndef GAME_RESOURCECONVERTER
	if (f1.open(outputFilename, g_scumm->getGameDataPath(), File::kFileReadMode))
	{
		char hdr[4];
		f1.read(hdr, 4);
		f1.close();
		if (memcmp(hdr, magic, 4) == 0)
			return true;
	}
#endif

	// open input file
	{
		// try "<game>.sou"
		f1.enableFileCache(4096);
		if (!f1.open(inputFilename, g_scumm->getGameDataPath(), File::kFileReadMode))
		{
			// try "monster.sou"
			sprintf(inputFilename, "MONSTER.SOU");
			if (!f1.open(inputFilename, g_scumm->getGameDataPath(), File::kFileReadMode))
			{
				debug(1, "Failed opening input audio file");
				return false;
			}
		}
	}

	// verify input file
	{
		char hdr[8];
		f1.read(hdr, 8);
		if (strcmp(hdr, "SOU ") != 0)
		{
			debug(1,"Invalid input file");
			return false;
		}
	}

	// open output file
	{
		f2.enableFileCache(4096);
		if (!f2.open(outputFilename, g_scumm->getGameDataPath(), File::kFileWriteMode));
		{
			debug(1,"Failed opening output file");
			return false;
		}
	}

	// dummy header
	f2.writeUint32BE(0);
	f2.writeUint32BE(0);

	// sounds
	if (!writeSounds())
	{
		debug(1, "Failed to write sounds");
		return false;
	}

	// mapping table
	if (!writeMappingTable())
	{
		debug(1, "Failed to write mapping table");
		return false;
	}

	// update header
	uint32 tablePos = f2.pos();
	f2.seek(0, SEEK_SET);
	f2.write(magic, 4);
	f2.writeUint32BE(tablePos);
	return true;
}
#endif //ATARI_SOUND_CONVERTER

bool ScummEngine::convertResourcesForAtari()
{
	// resources
	{
		AtariResourceConverter resConverter;
		if (!resConverter.GenerateResourceFiles())
			return false;
	}

	// sounds
	{
		#ifdef ATARI_SOUND_CONVERTER
		AtariSoundConverter souConverter;
		if (!souConverter.GenerateSoundFile())
			return false;
		#endif
	}

	return true;
}


} // End of namespace Scumm

