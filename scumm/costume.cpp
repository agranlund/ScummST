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
 * $Header: /cvsroot/scummvm/scummvm/scumm/costume.cpp,v 1.126 2004/01/08 03:10:16 fingolfin Exp $
 *
 */

#include "stdafx.h"
#include "scumm/scumm.h"
#include "scumm/actor.h"
#include "scumm/costume.h"
#include "scumm/sound.h"
#include "scumm/charset.h"

extern uint32 rptTable[256];

namespace Scumm
{
	#define _outwidth	320


	#define TABLE_8_REPT_40(_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7) { \
			_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7, \
			_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7, \
			_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7, \
			_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7, \
			_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7, \
			_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7, \
			_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7, \
			_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7, \
			_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7, \
			_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7,_v0,_v1,_v2,_v3,_v4,_v5,_v6,_v7 \
		}

	uint32 proc2mask_SingleBit[8*40] 		= TABLE_8_REPT_40(0x80808080, 0x40404040, 0x20202020, 0x10101010, 0x08080808, 0x04040404, 0x02020202, 0x01010101);
	uint32 proc2mask_SingleBit_Rev[8*40] 	= TABLE_8_REPT_40(0x01010101, 0x02020202, 0x04040404, 0x08080808, 0x10101010, 0x20202020, 0x40404040, 0x80808080);
	uint32 proc2mask_SingleBit_Inv[8*40] 	= TABLE_8_REPT_40(0x7F7F7F7F, 0xBFBFBFBF, 0xDFDFDFDF, 0xEFEFEFEF, 0xF7F7F7F7, 0xFBFBFBFB, 0xFDFDFDFD, 0xFEFEFEFE);
	uint32 proc2mask_LeftBits[8*40] 		= TABLE_8_REPT_40(0xFFFFFFFF, 0x7F7F7F7F, 0x3F3F3F3F, 0x1F1F1F1F, 0x0F0F0F0F, 0x07070707, 0X03030303, 0x01010101);
	uint32 proc2mask_LeftBits_Rev[8*40] 	= TABLE_8_REPT_40(0x01010101, 0x03030303, 0x07070707, 0x0F0F0F0F, 0x1F1F1F1F, 0x3F3F3F3F, 0x7F7F7F7F, 0xFFFFFFFF);
	uint32 proc2mask_RightBits[8*40] 		= TABLE_8_REPT_40(0x80808080, 0xC0C0C0C0, 0xE0E0E0E0, 0xF0F0F0F0, 0xF8F8F8F8, 0xFCFCFCFC, 0xFEFEFEFE, 0xFFFFFFFF);
	uint32 proc2mask_RightBit_Rev[8*40]		= TABLE_8_REPT_40(0xFFFFFFFF, 0xFEFEFEFE, 0xFCFCFCFC, 0x0F8F8F8F, 0xF0F0F0F0, 0xE0E0E0E0, 0xC0C0C0C0, 0x80808080);

	const byte revBitMask[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

	const byte cost_scaleTable[256*4] = {
		0xFF, 0xFD, 0x7D, 0xBD, 0x3D, 0xDD, 0x5D, 0x9D, 0x1D, 0xED, 0x6D, 0xAD, 0x2D, 0xCD, 0x4D, 0x8D, 0x0D, 0xF5, 0x75, 0xB5, 0x35, 0xD5, 0x55, 0x95, 0x15, 0xE5, 0x65, 0xA5, 0x25, 0xC5, 0x45, 0x85,
		0x05, 0xF9, 0x79, 0xB9, 0x39, 0xD9, 0x59, 0x99,	0x19, 0xE9, 0x69, 0xA9, 0x29, 0xC9, 0x49, 0x89,	0x09, 0xF1, 0x71, 0xB1, 0x31, 0xD1, 0x51, 0x91,	0x11, 0xE1, 0x61, 0xA1, 0x21, 0xC1, 0x41, 0x81,
		0x01, 0xFB, 0x7B, 0xBB, 0x3B, 0xDB, 0x5B, 0x9B,	0x1B, 0xEB, 0x6B, 0xAB, 0x2B, 0xCB, 0x4B, 0x8B,	0x0B, 0xF3, 0x73, 0xB3, 0x33, 0xD3, 0x53, 0x93,	0x13, 0xE3, 0x63, 0xA3, 0x23, 0xC3, 0x43, 0x83,
		0x03, 0xF7, 0x77, 0xB7, 0x37, 0xD7, 0x57, 0x97,	0x17, 0xE7, 0x67, 0xA7, 0x27, 0xC7, 0x47, 0x87,	0x07, 0xEF, 0x6F, 0xAF, 0x2F, 0xCF, 0x4F, 0x8F,	0x0F, 0xDF, 0x5F, 0x9F, 0x1F, 0xBF, 0x3F, 0x7F,
		0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,	0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,	0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,	0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
		0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,	0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,	0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
		0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,	0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,	0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,	0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
		0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,	0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,	0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,	0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,

		0xFF, 0xFD, 0x7D, 0xBD, 0x3D, 0xDD, 0x5D, 0x9D, 0x1D, 0xED, 0x6D, 0xAD, 0x2D, 0xCD, 0x4D, 0x8D, 0x0D, 0xF5, 0x75, 0xB5, 0x35, 0xD5, 0x55, 0x95, 0x15, 0xE5, 0x65, 0xA5, 0x25, 0xC5, 0x45, 0x85,
		0x05, 0xF9, 0x79, 0xB9, 0x39, 0xD9, 0x59, 0x99,	0x19, 0xE9, 0x69, 0xA9, 0x29, 0xC9, 0x49, 0x89,	0x09, 0xF1, 0x71, 0xB1, 0x31, 0xD1, 0x51, 0x91,	0x11, 0xE1, 0x61, 0xA1, 0x21, 0xC1, 0x41, 0x81,
		0x01, 0xFB, 0x7B, 0xBB, 0x3B, 0xDB, 0x5B, 0x9B,	0x1B, 0xEB, 0x6B, 0xAB, 0x2B, 0xCB, 0x4B, 0x8B,	0x0B, 0xF3, 0x73, 0xB3, 0x33, 0xD3, 0x53, 0x93,	0x13, 0xE3, 0x63, 0xA3, 0x23, 0xC3, 0x43, 0x83,
		0x03, 0xF7, 0x77, 0xB7, 0x37, 0xD7, 0x57, 0x97,	0x17, 0xE7, 0x67, 0xA7, 0x27, 0xC7, 0x47, 0x87,	0x07, 0xEF, 0x6F, 0xAF, 0x2F, 0xCF, 0x4F, 0x8F,	0x0F, 0xDF, 0x5F, 0x9F, 0x1F, 0xBF, 0x3F, 0x7F,
		0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,	0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,	0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,	0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
		0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,	0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,	0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
		0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,	0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,	0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,	0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
		0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,	0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,	0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,	0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,

		0xFF, 0xFD, 0x7D, 0xBD, 0x3D, 0xDD, 0x5D, 0x9D, 0x1D, 0xED, 0x6D, 0xAD, 0x2D, 0xCD, 0x4D, 0x8D, 0x0D, 0xF5, 0x75, 0xB5, 0x35, 0xD5, 0x55, 0x95, 0x15, 0xE5, 0x65, 0xA5, 0x25, 0xC5, 0x45, 0x85,
		0x05, 0xF9, 0x79, 0xB9, 0x39, 0xD9, 0x59, 0x99,	0x19, 0xE9, 0x69, 0xA9, 0x29, 0xC9, 0x49, 0x89,	0x09, 0xF1, 0x71, 0xB1, 0x31, 0xD1, 0x51, 0x91,	0x11, 0xE1, 0x61, 0xA1, 0x21, 0xC1, 0x41, 0x81,
		0x01, 0xFB, 0x7B, 0xBB, 0x3B, 0xDB, 0x5B, 0x9B,	0x1B, 0xEB, 0x6B, 0xAB, 0x2B, 0xCB, 0x4B, 0x8B,	0x0B, 0xF3, 0x73, 0xB3, 0x33, 0xD3, 0x53, 0x93,	0x13, 0xE3, 0x63, 0xA3, 0x23, 0xC3, 0x43, 0x83,
		0x03, 0xF7, 0x77, 0xB7, 0x37, 0xD7, 0x57, 0x97,	0x17, 0xE7, 0x67, 0xA7, 0x27, 0xC7, 0x47, 0x87,	0x07, 0xEF, 0x6F, 0xAF, 0x2F, 0xCF, 0x4F, 0x8F,	0x0F, 0xDF, 0x5F, 0x9F, 0x1F, 0xBF, 0x3F, 0x7F,
		0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,	0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,	0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,	0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
		0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,	0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,	0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
		0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,	0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,	0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,	0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
		0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,	0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,	0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,	0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,

		0xFF, 0xFD, 0x7D, 0xBD, 0x3D, 0xDD, 0x5D, 0x9D, 0x1D, 0xED, 0x6D, 0xAD, 0x2D, 0xCD, 0x4D, 0x8D, 0x0D, 0xF5, 0x75, 0xB5, 0x35, 0xD5, 0x55, 0x95, 0x15, 0xE5, 0x65, 0xA5, 0x25, 0xC5, 0x45, 0x85,
		0x05, 0xF9, 0x79, 0xB9, 0x39, 0xD9, 0x59, 0x99,	0x19, 0xE9, 0x69, 0xA9, 0x29, 0xC9, 0x49, 0x89,	0x09, 0xF1, 0x71, 0xB1, 0x31, 0xD1, 0x51, 0x91,	0x11, 0xE1, 0x61, 0xA1, 0x21, 0xC1, 0x41, 0x81,
		0x01, 0xFB, 0x7B, 0xBB, 0x3B, 0xDB, 0x5B, 0x9B,	0x1B, 0xEB, 0x6B, 0xAB, 0x2B, 0xCB, 0x4B, 0x8B,	0x0B, 0xF3, 0x73, 0xB3, 0x33, 0xD3, 0x53, 0x93,	0x13, 0xE3, 0x63, 0xA3, 0x23, 0xC3, 0x43, 0x83,
		0x03, 0xF7, 0x77, 0xB7, 0x37, 0xD7, 0x57, 0x97,	0x17, 0xE7, 0x67, 0xA7, 0x27, 0xC7, 0x47, 0x87,	0x07, 0xEF, 0x6F, 0xAF, 0x2F, 0xCF, 0x4F, 0x8F,	0x0F, 0xDF, 0x5F, 0x9F, 0x1F, 0xBF, 0x3F, 0x7F,
		0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,	0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,	0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,	0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
		0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,	0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,	0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
		0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,	0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,	0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,	0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
		0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,	0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,	0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,	0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
};

	byte CostumeRenderer::mainRoutine(int xmoveCur, int ymoveCur)
	{
		int i, skip;
		byte drawFlag = 1;
		uint scal;
		bool use_scaling;
		byte startScaleIndexX;
		Common::Rect rect;
		int step;

		byte atariFormat = 0;
		bool horizontalEncoding = false;

		CHECK_HEAP

		v1.scaletable = cost_scaleTable;
		if (_loaded._numColors == 32)
		{
			v1.mask = 7;
			v1.shr = 3;
		}
		else
		{
			v1.mask = 0xF;
			v1.shr = 4;
		}

		switch (_loaded._format)
		{
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
			atariFormat = 2;
			horizontalEncoding = true;
			#ifdef ENGINE_SCUMM6
			{
				int ex1 = _srcptr[0];
				int ex2 = _srcptr[1];
				_srcptr += 2;
				if (ex1 != 0xFF || ex2 != 0xFF)
				{
					ex1 = READ_LE_UINT16(_loaded._frameOffsets + ex1 * 2);
					#ifdef GAME_SAMNMAX
					_srcptr = _loaded._baseptr + (READ_LE_UINT16(_loaded._baseptr + ex1 + ex2 * 2) << 1) + 14;
					#else
					_srcptr = _loaded._baseptr + READ_LE_UINT16(_loaded._baseptr + ex1 + ex2 * 2) + 14;
					#endif
				}
			}
			#endif			
			break;
			/*
		case 0x60:
		case 0x61:
			// This format is used e.g. in the Sam&Max intro
			ex1 = _srcptr[0];
			ex2 = _srcptr[1];
			_srcptr += 2;
			if (ex1 != 0xFF || ex2 != 0xFF)
			{
				ex1 = READ_LE_UINT16(_loaded._frameOffsets + ex1 * 2);
				#ifdef GAME_SAMNMAX
				_srcptr = _loaded._baseptr + (READ_LE_UINT16(_loaded._baseptr + ex1 + ex2 * 2) << 1) + 14;
				#else
				_srcptr = _loaded._baseptr + READ_LE_UINT16(_loaded._baseptr + ex1 + ex2 * 2) + 14;
				#endif
			}
			*/
			default:
				return 1;
		}

		v1.x = _actorX;
		v1.y = _actorY;
		use_scaling = (_scaleX != 0xFF) || (_scaleY != 0xFF);
		skip = 0;

		if (use_scaling)
		{
			// scaling
			v1.scaleXstep = -1;
			if (xmoveCur < 0)
			{
				xmoveCur = -xmoveCur;
				v1.scaleXstep = 1;
			}

			if (_mirror)
			{
				// mirrored
				startScaleIndexX = _scaleIndexX = 128 - xmoveCur;
				for (i = 0; i < xmoveCur; i++)
				{
					if (cost_scaleTable[_scaleIndexX++] < _scaleX)
						v1.x -= v1.scaleXstep;
				}
				rect.right = rect.left = v1.x;
				_scaleIndexX = startScaleIndexX;
				for (i = 0; i < _width; i++)
				{
					if (rect.right < 0)
					{
						skip++;
						startScaleIndexX = _scaleIndexX;
					}
					scal = cost_scaleTable[_scaleIndexX++];
					if (scal < _scaleX)
						rect.right++;
				}
			}
			else
			{
				// not mirrored
				startScaleIndexX = _scaleIndexX = xmoveCur + 128;
				for (i = 0; i < xmoveCur; i++)
				{
					scal = cost_scaleTable[_scaleIndexX--];
					if (scal < _scaleX)
						v1.x += v1.scaleXstep;
				}
				rect.right = rect.left = v1.x;
				_scaleIndexX = startScaleIndexX;
				for (i = 0; i < _width; i++)
				{
					if (rect.left >= (int)_outwidth)
					{
						skip++;
						startScaleIndexX = _scaleIndexX;
					}
					scal = cost_scaleTable[_scaleIndexX--];
					if (scal < _scaleX)
						rect.left--;
				}
			}

			_scaleIndexX = startScaleIndexX;
			if (skip)
				skip--;

			step = -1;
			if (ymoveCur < 0)
			{
				ymoveCur = -ymoveCur;
				step = 1;
			}
			_scaleIndexY = 128 - ymoveCur;
			for (i = 0; i < ymoveCur; i++)
			{
				scal = cost_scaleTable[_scaleIndexY++];
				if (scal < _scaleY)
					v1.y -= step;
			}
			rect.top = rect.bottom = v1.y;
			_scaleIndexY = 128 - ymoveCur;
			for (i = 0; i < _height; i++)
			{
				scal = cost_scaleTable[_scaleIndexY++];
				if (scal < _scaleY)
					rect.bottom++;
			}
			_scaleIndexY = 128 - ymoveCur;
		}
		else
		{
			// no scaling
			if (!_mirror)
				xmoveCur = -xmoveCur;
			v1.x += xmoveCur;
			v1.y += ymoveCur;
			if (_mirror)
			{
				rect.left = v1.x;
				rect.right = v1.x + _width;
			}
			else
			{
				rect.left = v1.x - _width;
				rect.right = v1.x;
			}
			rect.top = v1.y;
			rect.bottom = rect.top + _height;
		}

		v1.skip_width = _width;
		v1.scaleXstep = _mirror ? 1 : -1;

		_vm->markRectAsDirty(kMainVirtScreen, rect.left, rect.right/* + 1*/, rect.top, rect.bottom, _actorID);

		if (rect.top >= (int)_outheight || rect.bottom <= 0)
			return 0;

		if (rect.left >= (int)_outwidth || rect.right <= 0)
			return 0;

		v1.replen = 0;

		if (_mirror)
		{
			if (!use_scaling)
				skip = -v1.x;

			if (/*(atariFormat == 0) &&*/ (skip > 0))
			{
				if (atariFormat == 0)
				{
					v1.skip_width -= skip;
					codec1_ignorePakCols(skip);
					v1.x = 0;
				}
			}
			else
			{
				skip = rect.right - _outwidth;
				if (skip <= 0)
					drawFlag = 2;
				else
					v1.skip_width -= skip;
			}
		}
		else
		{
			if (!use_scaling)
				skip = rect.right - _outwidth;

			if (/*(atariFormat == 0) &&*/ (skip > 0))
			{
				if (atariFormat == 0)
				{
					v1.skip_width -= skip;
					codec1_ignorePakCols(skip);
					v1.x = _outwidth - 1;
				}
			}
			else
			{
				skip = -1 - rect.left;
				if (skip <= 0)
					drawFlag = 2;
				else
					v1.skip_width -= skip;
			}
		}

		if (v1.skip_width <= 0)
			return 0;

		//_vm->markRectAsDirty(kMainVirtScreen, rect.left, rect.right/* + 1*/, rect.top, rect.bottom, _actorID);

		if (rect.left < 0)
			rect.left = 0;
		if (rect.right > _outwidth)
			rect.right = _outwidth;
		if (rect.top < 0)
			rect.top = 0;
		if (rect.bottom > _outheight)
			rect.bottom = _outheight;
		if (rect.top + _height >= 256)
		{
			CHECK_HEAP
			return 2;
		}

		if (_draw_left > rect.left)
			_draw_left = rect.left;
		if (_draw_right < rect.right)
			_draw_right = rect.right;
		if (_draw_top > rect.top)
			_draw_top = rect.top;
		if (_draw_bottom < rect.bottom)
			_draw_bottom = rect.bottom;

		v1.destptr = _outptr + (v1.y * (_outwidth >> 1));
		v1.mask_ptr = _vm->getMaskBuffer(0, v1.y, 0);
		v1.imgbufoffs = _vm->gdi._imgBufOffs[_zbuf];

		CHECK_HEAP

		if (atariFormat == 2)
		{
#ifdef ENABLE_ATARI_COST_FORMAT_2
			byte masking = 0;

			// todo: investigate masking
			//	keep a has-mask value for each row and column for faster lookup ?
			//	separate line drawers depending on masking ?
			//  want to at least avoid having to look up 2 masks, so maybe do extra detection on zbuffer if text is active?

			if (v1.mask_ptr)
			{
				if (_zbuf != 0)
				{
					masking |= 1;
				}
			#ifdef GAME_SAMNMAX
				else
				{
					// todo: detect when dbDrawMaskOnAll has been used, and when it's cleared again.
					masking |= 1;
				}
			#endif					

				if (_vm->_charset->hasCharsetMask(rect.left, rect.top, rect.right, rect.bottom))
					masking |= 2;
			}
			
			int16 endy = rect.bottom;
			bool mirror = (v1.scaleXstep < 0) ? true : false;
			bool use_clipping = mirror ? ((v1.x >= (int)_outwidth) || ((v1.x - _width) < 0)) : ((v1.x < 0) || ((v1.x + _width) > (int)_outwidth));
			byte flag = (use_scaling << 4) | (mirror << 3) | (masking << 1) | use_clipping;

			switch (flag)
			{
				// mirror, masking, clipping
				case  0:	procAtari2<1, 0, false>(endy);		break;
				case  1:	procAtari2<1, 0, true>(endy);		break;
				case  2:	procAtari2<1, 1, false>(endy);		break;
				case  3:	procAtari2<1, 1, true>(endy);		break;
				case  4:	procAtari2<1, 2, false>(endy);		break;
				case  5:	procAtari2<1, 2, true>(endy);		break;
				case  6:	procAtari2<1, 3, false>(endy);		break;
				case  7:	procAtari2<1, 3, true>(endy);		break;
				case  8:	procAtari2<-1, 0, false>(endy);		break;
				case  9:	procAtari2<-1, 0, true>(endy);		break;
				case 10:	procAtari2<-1, 1, false>(endy);		break;
				case 11:	procAtari2<-1, 1, true>(endy);		break;
				case 12:	procAtari2<-1, 2, false>(endy);		break;
				case 13:	procAtari2<-1, 2, true>(endy);		break;
				case 14:	procAtari2<-1, 3, false>(endy);		break;
				case 15:	procAtari2<-1, 3, true>(endy);		break;
				case 16:	procAtari2_Scaled<1, 0, false>(endy);	break;
				case 17:	procAtari2_Scaled<1, 0, true>(endy);	break;
				case 18:	procAtari2_Scaled<1, 1, false>(endy);	break;
				case 19:	procAtari2_Scaled<1, 1, true>(endy);	break;
				case 20:	procAtari2_Scaled<1, 2, false>(endy);	break;
				case 21:	procAtari2_Scaled<1, 2, true>(endy);	break;
				case 22:	procAtari2_Scaled<1, 3, false>(endy);	break;
				case 23:	procAtari2_Scaled<1, 3, true>(endy);	break;
				case 24:	procAtari2_Scaled<-1, 0, false>(endy);	break;
				case 25:	procAtari2_Scaled<-1, 0, true>(endy);	break;
				case 26:	procAtari2_Scaled<-1, 1, false>(endy);	break;
				case 27:	procAtari2_Scaled<-1, 1, true>(endy);	break;
				case 28:	procAtari2_Scaled<-1, 2, false>(endy);	break;
				case 29:	procAtari2_Scaled<-1, 2, true>(endy);	break;
				case 30:	procAtari2_Scaled<-1, 3, false>(endy);	break;
				case 31:	procAtari2_Scaled<-1, 3, true>(endy);	break;
			}
#endif //ENABLE_ATARI_COST_FORMAT_2
		}

		CHECK_HEAP
		return drawFlag;
	}

#ifdef ENABLE_ATARI_COST_FORMAT_2

	#define ATARI_PROC2_PARANOID			0
	#define ATARI_PROC2_DEBUG_TRANSPARENT	0
	#define ATARI_PROC2_DEBUG_MASKING		0
	#define ATARI_PROC2_DEBUG_COLORS 		(ATARI_PROC2_DEBUG_MASKING || ATARI_PROC2_DEBUG_TRANSPARENT)

	static uint32 proc2DebugColor = 0;
	static const byte* _proc2PalettePtr = 0;
	#define ATARI_PROC2_CONVCOLOR(_c_) *((uint32*)(_proc2PalettePtr + (_c_)))

#if ATARI_PROC2_DEBUG_MASKING
	#define ATARI_PROC2_GETCOLOR(_c_) masking == 0 ? ATARI_PROC2_CONVCOLOR(_c_) : c2pfill16[masking]
#elif ATARI_PROC2_DEBUG_TRANSPARENT
	#define ATARI_PROC2_GETCOLOR(_c_) (_c_) == 0 ? ATARI_PROC2_CONVCOLOR((proc2DebugColor++ & 0xF) << 3) : ATARI_PROC2_CONVCOLOR(_c_)
#elif ATARI_PROC2_DEBUG_COLORS
	#define ATARI_PROC2_GETCOLOR(_c_) ATARI_PROC2_CONVCOLOR((proc2DebugColor++ & 0xF) << 3)
#else
	#define ATARI_PROC2_GETCOLOR(_c_) ATARI_PROC2_CONVCOLOR(_c_)
#endif

	#define ATARI_PROC2_INC(_var)				{ if (dir == 1) _var++; else _var--; }
	#define ATARI_PROC2_ADD(_var,_value)		{ if (dir == 1) _var += (_value); else _var -= (_value); }

	#define ATARI_PROC2_PARAMS_SET(_c, _mask)  	{ px = ((_c) & (_mask)); pm = (_mask); }
	#define ATARI_PROC2_PARAMS_ADD(_c, _mask)	{ px |= ((_c) & (_mask)); pm |= (_mask); }
	#define ATARI_PROC2_PARAMS_CLEAR() 			{ px = pm = 0; }
	#define ATARI_PROC2_PARAMS_FULL() 			((dir == 1 && (pm & 0x1)) || (dir == -1 && (pm & 0x80)))

	#define ATARI_PROC2_WRITETILE_INT2(_px,_pm,_dst) { \
		uint32 imask = ~_pm; \
		if (imask == 0) { *_dst = _px; } else \
		{ \
			uint32 masked_src = _px & _pm; \
			uint32 masked_dst = *_dst & imask; \
			*_dst = masked_dst | masked_src; \
		} \
	}

	#define ATARI_PROC2_WRITETILE_INT1(_px,_pm,_dst,_mask) { \
		if (masking) \
		{ \
			byte bgmask8; \
			if 		(masking == 1)	bgmask8 = *(_mask); \
			else if (masking == 2)	bgmask8 = *(_mask); \
			else if (masking == 3)	bgmask8 = *(_mask) | *((_mask) + v1.imgbufoffs); \
			if (/*((masking & 2) == 0) &&*/ (bgmask8 == 0)) { ATARI_PROC2_WRITETILE_INT2(_px,_pm,_dst); } \
			else { \
				uint32 srcmask32 = _pm & ~rptTable[bgmask8]; \
				ATARI_PROC2_WRITETILE_INT2(_px,srcmask32,_dst); \
			} \
		} else { \
			ATARI_PROC2_WRITETILE_INT2(_px,_pm,_dst); \
		} \
	}

	#define ATARI_PROC2_WRITETILE_INT1_NOSRCMASK(_px,_dst,_mask) { \
		if (masking) \
		{ \
			byte bgmask8; \
			if 		(masking == 1)	bgmask8 = *(_mask); \
			else if (masking == 2)	bgmask8 = *(_mask); \
			else if (masking == 3)	bgmask8 = *(_mask) | *((_mask) + v1.imgbufoffs); \
			if (/*((masking & 2) == 0) &&*/ (bgmask8 == 0)) { *_dst = _px; } \
			else { \
				uint32 srcmask32 = rptTable[bgmask8]; \
				uint32 masked_src = _px & ~srcmask32; \
				uint32 masked_dst = *_dst & srcmask32; \
				*_dst = masked_dst | masked_src; \
			} \
		} else { \
			*_dst = _px; \
		} \
	}

	#define ATARI_PROC2_WRITETILE_START(_xtile) { \
		uint32* tile_dstptr = &dst[_xtile]; \
		const byte* tile_maskptr; \
		if (masking) \
			tile_maskptr = &mask[_xtile]; \

	#define ATARI_PROC2_WRITETILE_WRITE() \
		ATARI_PROC2_WRITETILE_INT1(px, pm, tile_dstptr, tile_maskptr); \
		ATARI_PROC2_PARAMS_CLEAR(); \

	#define ATARI_PROC2_WRITETILE_CUSTOM_NOSRCMASK(_px) \
		ATARI_PROC2_WRITETILE_INT1_NOSRCMASK(_px, tile_dstptr, tile_maskptr);

	#define ATARI_PROC2_WRITETILE_NEXT() \
		ATARI_PROC2_INC(tile_dstptr); \
		if (masking) ATARI_PROC2_INC(tile_maskptr);

	#define ATARI_PROC2_WRITETILE_DONE() \
	}

	#define ATARI_PROC2_WRITETILE() { \
		uint16 xoffs = ((uint16)x)>>3; \
		ATARI_PROC2_WRITETILE_START(xoffs); \
		ATARI_PROC2_WRITETILE_WRITE(); \
		ATARI_PROC2_WRITETILE_DONE(); \
	}

	#define ATARI_PROC2_WRITESPAN(_c, _mask) { \
		ATARI_PROC2_PARAMS_ADD(_c, _mask) \
		if (ATARI_PROC2_PARAMS_FULL()) { \
			ATARI_PROC2_WRITETILE(); \
		} \
	}

	// xxxxxyyy:	x = color, y = length (if zero, then next byte is length)
	#define ATARI_PROC2_FETCH_SPAN() \
		int16 color = *src++; \
		int16 len = color & 7; \
		if (len == 0) len = *src++; \
		color &= 0xF8;

	#define ATARI_PROC2_SCALE_SPAN() { \
		int16 slen = len; \
		while (slen) \
		{ \
			if ((dir== 1 && *scalextab++ >= _scaleX) || (dir==-1 && *scalextab-- >= _scaleX)) \
				len--; \
			slen--; \
		} \
	}


	template <bool transp, char dir, byte masking, bool clipping>
	inline void CostumeRenderer::procAtari2Span(int16& x, int16& len, int16& color, uint32& pm, uint32& px, const byte*& src, uint32*& dst, const byte*& mask)
	{
		// skip transparent span
		if (transp)
		{
			#if !ATARI_PROC2_DEBUG_TRANSPARENT
			if (color == 0)
			{
				if (pm && ((dir == 1 && (((x + len) & ~7) != (x & ~7))) || (dir == -1 && (((x - len) & ~7) != (x & ~7))))) {
					ATARI_PROC2_WRITETILE();
				}
				return;
			}
			#endif
		}

		// draw opaque span
		uint32 c = ATARI_PROC2_GETCOLOR(color);
		/*if (len == 1)
		{
			uint32 xbitmask = proc2mask_SingleBit[x];
			ATARI_PROC2_WRITESPAN(c, xbitmask);
		}
		else
		*/

		{
			int16 bits = (dir == 1) ? (8 - (x & 7)) : ((x & 7) + 1);
			if (len <= bits)
			{
				if (!clipping || ((uint16)x)<_outwidth)
				{
					uint32 xbitmask = (dir == 1) ? (proc2mask_LeftBits[x] & proc2mask_RightBits[x + len - 1]) : (proc2mask_LeftBits[x - len + 1] & proc2mask_RightBits[x]);
					ATARI_PROC2_WRITESPAN(c, xbitmask);
				}
			}
			else
			{
				// left
				int16 xtile = x >> 3;
				ATARI_PROC2_WRITETILE_START(xtile);
				if (!clipping || ((uint16)xtile) < 40)
				{
					uint32 xbitmask = (dir == 1) ? proc2mask_LeftBits[x] : proc2mask_RightBits[x];
					ATARI_PROC2_PARAMS_ADD(c, xbitmask);
					ATARI_PROC2_WRITETILE_WRITE();
				}

				// middle
				int16 count = (len - bits) >> 3;
				while (count)
				{
					ATARI_PROC2_WRITETILE_NEXT();
					if (clipping)
						ATARI_PROC2_INC(xtile);
					if (!clipping || ((uint16)xtile) < 40)
						ATARI_PROC2_WRITETILE_CUSTOM_NOSRCMASK(c);
					count--;
				}

				// right
				int16 lastbit = (dir == 1) ? ((x + len) & 7) : ((x - len + 1) & 7);
				if (lastbit != 0)
				{
					if (clipping)
						ATARI_PROC2_INC(xtile);
					if (!clipping || ((uint16)xtile) < 40)
					{
						uint32 xbitmask = (dir == 1) ? proc2mask_RightBits[lastbit-1] : proc2mask_LeftBits[lastbit];
						ATARI_PROC2_PARAMS_SET(c, xbitmask);
						if (ATARI_PROC2_PARAMS_FULL())
						{
							ATARI_PROC2_WRITETILE_NEXT();
							ATARI_PROC2_WRITETILE_WRITE();
						}
					}
				}
				ATARI_PROC2_WRITETILE_DONE();
			}
		}
	}

	template <bool transp, char dir, byte masking, bool clipping>
	inline void CostumeRenderer::procAtari2Line(int16& x, int16& width, uint32& pm, uint32& px, const byte*& src, uint32*& dst, const byte*& mask)
	{
		// clip line

		while (width != 0)
		{
			// fetch span
			ATARI_PROC2_FETCH_SPAN();
			width -= len;

			// clip span

			// draw span
			procAtari2Span<transp, dir, masking, clipping>(x, len, color, pm, px, src, dst, mask);
			ATARI_PROC2_ADD(x,len);
		}

		if (pm) {
			ATARI_PROC2_WRITETILE();
		}
	}

	template <bool transp, char dir, byte masking, bool clipping>
	inline void CostumeRenderer::procAtari2Line_Scaled(int16& x, int16& width, const byte*& scalextab, uint32& pm, uint32& px, const byte*& src, uint32*& dst, const byte*& mask)
	{
		// clip line

		while (width != 0)
		{
			// fetch span
			ATARI_PROC2_FETCH_SPAN();
			width -= len;
			ATARI_PROC2_SCALE_SPAN();
		
			// clip span
			if (len == 0)
				continue;

			// draw span
			procAtari2Span<transp, dir, masking, clipping>(x, len, color, pm, px, src, dst, mask);
			ATARI_PROC2_ADD(x,len);
		}

		if (pm) {
			ATARI_PROC2_WRITETILE();
		}
	}

	template <char dir, byte masking, bool clipping>
	void CostumeRenderer::procAtari2(uint16 endy)
	{
		_proc2PalettePtr = (byte*)_palette;
		const byte *src = _srcptr;
		const int16 xstart = (int16) v1.x;

		v1.destptr += (_outoffs & ~7) >> 1;
		uint32 *dst = (uint32 *)v1.destptr;
		const byte *mask;
		if (masking == 1) 		mask = v1.mask_ptr + v1.imgbufoffs;		// zbuf
		else if (masking == 2)	mask = v1.mask_ptr;						// text
		else if (masking == 3)	mask = v1.mask_ptr;						// text + zbuf;

		byte linelen = *src++;
		int16 width = *src++;
		int16 skip = *src++;
		const byte* nextline = src + linelen;

		// clip top
		int16 y = v1.y;
		if (y < 0)
		{
			int ny = -y;
			dst += MUL40(ny);
			if (masking)
				mask += MUL40(ny);
			while (y < 0)
			{
				src += linelen;
				linelen = *src++;
			#if ATARI_PROC2_PARANOID
				if (linelen == 255)
					return;
			#endif
				width = *src++;
				skip = *src++;
				y++;
			}
			nextline = src + linelen;
		}

		uint32 px = 0;
		uint32 pm = 0;
		int16 height = endy - y;
		#if ATARI_PROC2_PARANOID
		if (height <= 0)
			return;
		#endif

		const byte* textlinemask;
		if (masking & 2)
			textlinemask = &_vm->_charset->_ymask[y];

		while(1)
		{
			// skip to start of data
			int16 x = xstart;
			if (skip & ~1) {
				ATARI_PROC2_ADD(x, skip>>1);
			}

			// draw line
			if (masking & 2)
			{
				if (*textlinemask++ == 0)
				{
					const byte* linemask;
					if (masking & 1)
						linemask = mask + v1.imgbufoffs;

					if (skip & 1)
						procAtari2Line<true,  dir, masking & 1, clipping>(x, width, pm, px, src, dst, linemask);
					else
						procAtari2Line<false, dir, masking & 1, clipping>(x, width, pm, px, src, dst, linemask);
				}
				else if (skip & 1)
					procAtari2Line<true,  dir, masking, clipping>(x, width, pm, px, src, dst, mask);
				else
					procAtari2Line<false, dir, masking, clipping>(x, width, pm, px, src, dst, mask);
			}
			else if (skip & 1)
				procAtari2Line<true,  dir, masking, clipping>(x, width, pm, px, src, dst, mask);
			else
				procAtari2Line<false, dir, masking, clipping>(x, width, pm, px, src, dst, mask);

			// prepare next line
			height--;
			if (height == 0)
				return;

			if (clipping)
				src = nextline;

			linelen = *src++;
		#if ATARI_PROC2_PARANOID
			if (linelen == 255)
				return;			
		#endif			
			width = *src++;
			skip = *src++;

			if (clipping)
				nextline = src + linelen;

			dst += 40;
			if (masking)
				mask += 40;
		}
	}

	template <char dir, byte masking, bool clipping>
	void CostumeRenderer::procAtari2_Scaled(uint16 endy)
	{
		_proc2PalettePtr = (byte*)_palette;
		const byte *src = _srcptr;
		const int16 xstart = (int16) v1.x;
		int16 y = v1.y;
		int16 x = xstart;

		v1.destptr += (_outoffs & ~7) >> 1;
		uint32 *dst = (uint32 *)v1.destptr;
		const byte *mask; // = v1.mask_ptr;
		if (masking == 1) 		mask = v1.mask_ptr + v1.imgbufoffs;		// zbuf
		else if (masking == 2)	mask = v1.mask_ptr;						// text
		else if (masking == 3)	mask = v1.mask_ptr;						// zbuf + text

		const byte* scalextabstart = &cost_scaleTable[_scaleIndexX];
		const byte* scalextab = scalextabstart;
		const byte* scaleytab = &cost_scaleTable[_scaleIndexY];

		byte linelen = *src++;
		int16 width = *src++;
		int16 skip = *src++;
		const byte* nextline = src + linelen;

		// clip top
		while (y < 0)
		{
			if (*scaleytab < _scaleY)
			{
				y++;
				dst += 40;
				if (masking)
					mask += 40;				
			}

			linelen = *nextline++;
			if (linelen == 255)
				return;
			width = *nextline++;
			skip = *nextline++;
			src = nextline;
			nextline += linelen;
			scaleytab++;
		}

		uint32 px = 0;
		uint32 pm = 0;
		int16 height = endy - y;
		#if ATARI_PROC2_PARANOID
		if (height <= 0)
			return;
		#endif
		
		const byte* textlinemask;
		if (masking & 2)
			textlinemask = &_vm->_charset->_ymask[y];	

		while(1)
		{
			// consider y scaling
			if (*scaleytab < _scaleY)
			{
				// skip to start of data
				if (skip & ~1) {
					int16 len = skip >> 1;
					ATARI_PROC2_SCALE_SPAN();
					ATARI_PROC2_ADD(x, len);
				}

				// draw line
				if (masking & 2)
				{
					if (*textlinemask++ == 0)
					{
						const byte* linemask;
						if (masking & 1)
							linemask = mask + v1.imgbufoffs;

						if (skip & 1)
							procAtari2Line_Scaled<true,  dir, masking & 1, clipping>(x, width, scalextab, pm, px, src, dst, linemask);
						else
							procAtari2Line_Scaled<false, dir, masking & 1, clipping>(x, width, scalextab, pm, px, src, dst, linemask);
					}
					else if (skip & 1)
						procAtari2Line_Scaled<true,  dir, masking, clipping>(x, width, scalextab, pm, px, src, dst, mask);
					else
						procAtari2Line_Scaled<false, dir, masking, clipping>(x, width, scalextab, pm, px, src, dst, mask);
				}
				else if (skip & 1)
					procAtari2Line_Scaled<true,  dir, masking, clipping>(x, width, scalextab, pm, px, src, dst, mask);
				else
					procAtari2Line_Scaled<false, dir, masking, clipping>(x, width, scalextab, pm, px, src, dst, mask);

				// prepare next line
				height--;
				if (height == 0)
					return;

				x = xstart;
				scalextab = scalextabstart;

				dst += 40;
				if (masking)
					mask += 40;
			}

			linelen = *nextline++;
			if (linelen == 255)
				return;
			width = *nextline++;
			skip = *nextline++;
			src = nextline;
			nextline += linelen;
			scaleytab++;
		}
	}

#endif //ENABLE_ATARI_FORMAT_2


	void LoadedCostume::loadCostume(int id)
	{
		_id = id;
		byte *ptr = _vm->getResourceAddress(rtCostume, id);

		if (_vm->_version == 6)
			ptr += 8;
		else
			ptr += 2;

		_baseptr = ptr;

		_numAnim = ptr[6];
		_format = ptr[7] & 0x7F;
		_mirror = (ptr[7] & 0x80) != 0;
		_palette = ptr + 8;
		switch (_format)
		{
		case 0x40: // new for atari - 16 color, costume palette lookup
			_numColors = 16;
			break;
		case 0x41: // new for atari - 32 color, costume palette lookup
			_numColors = 32;
			break;
		case 0x42: // new for atari - 16 color original, using static 16 color system palette
			_numColors = 16;
			break;
		case 0x43: // new for atari - 32 color original, using static 16 color system palette
			_numColors = 32;
			break;
			/*
		case 0x57: // Only used in V1 games
			_numColors = 0;
			break;
		case 0x58:
			_numColors = 16;
			break;
		case 0x59:
			_numColors = 32;
			break;
		case 0x60: // New since version 6
			_numColors = 16;
			break;
		case 0x61: // New since version 6
			_numColors = 32;
			break;
			*/
		default:
			error("Costume %d with format 0x%X is invalid", id, _format);
		}

		ptr += 8 + _numColors;
		_frameOffsets = ptr + 2;
		_dataOffsets = ptr + 34;
		_animCmds = _baseptr + READ_LE_UINT16(ptr);
	}

	byte CostumeRenderer::drawLimb(const CostumeData &cost, int limb)
	{
		int i;
		int code;
		const byte *frameptr;

		// If the specified limb is stopped or not existing, do nothing.
		if (cost.curpos[limb] == 0xFFFF || cost.stopped & (1 << limb))
			return 0;

		// Determine the position the limb is at
		i = cost.curpos[limb] & 0x7FFF;

		// Get the frame pointer for that limb
		frameptr = _loaded._baseptr + READ_LE_UINT16(_loaded._frameOffsets + limb * 2);

		// Determine the offset to the costume data for the limb at position i
		code = _loaded._animCmds[i] & 0x7F;

		// Code 0x7B indicates a limb for which there is nothing to draw
		if (code != 0x7B)
		{
			#ifdef GAME_SAMNMAX
			_srcptr = _loaded._baseptr + (READ_LE_UINT16(frameptr + code * 2) << 1);
			#else
			_srcptr = _loaded._baseptr + READ_LE_UINT16(frameptr + code * 2);
			#endif

			const CostumeInfo *costumeInfo;
			int xmoveCur, ymoveCur;

			costumeInfo = (const CostumeInfo *)_srcptr;
			_width = READ_LE_UINT16(&costumeInfo->width);
			_height = READ_LE_UINT16(&costumeInfo->height);
			xmoveCur = _xmove + (int16)READ_LE_UINT16(&costumeInfo->rel_x);
			ymoveCur = _ymove + (int16)READ_LE_UINT16(&costumeInfo->rel_y);
			_xmove += (int16)READ_LE_UINT16(&costumeInfo->move_x);
			_ymove -= (int16)READ_LE_UINT16(&costumeInfo->move_y);
			_srcptr += 12;

			// HACK: ignore large light post in beginning of Indy4
			//if ((_vm->_gameId == GID_INDY4) && (_vm->_currentRoom == 10) && (_height == 128))
			//	return 0;

			return mainRoutine(xmoveCur, ymoveCur);
		}

		return 0;
	}

	int ScummEngine::cost_frameToAnim(Actor *a, int frame)
	{
		return newDirToOldDir(a->getFacing()) + frame * 4;
	}

	void ScummEngine::cost_decodeData(Actor *a, int frame, uint usemask)
	{
		const byte *r;
		uint mask, j;
		int i;
		byte extra, cmd;
		int anim;
		LoadedCostume lc(this);

		lc.loadCostume(a->costume);

		anim = cost_frameToAnim(a, frame);

		if (anim > lc._numAnim)
		{
			return;
		}

		r = lc._baseptr + READ_LE_UINT16(lc._dataOffsets + anim * 2);

		if (r == lc._baseptr)
		{
			return;
		}

		mask = READ_LE_UINT16(r);
		r += 2;
		i = 0;
		do
		{
			if (mask & 0x8000)
			{
				j = READ_LE_UINT16(r);
				r += 2;
				if (usemask & 0x8000)
				{
					if (j == 0xFFFF)
					{
						a->cost.curpos[i] = 0xFFFF;
						a->cost.start[i] = 0;
						a->cost.frame[i] = frame;
					}
					else
					{
						extra = *r++;
						cmd = lc._animCmds[j];
						if (cmd == 0x7A)
						{
							a->cost.stopped &= ~(1 << i);
						}
						else if (cmd == 0x79)
						{
							a->cost.stopped |= (1 << i);
						}
						else
						{
							a->cost.curpos[i] = a->cost.start[i] = j;
							a->cost.end[i] = j + (extra & 0x7F);
							if (extra & 0x80)
								a->cost.curpos[i] |= 0x8000;
							a->cost.frame[i] = frame;
						}
					}
				}
				else
				{
					if (j != 0xFFFF)
						r++;
				}
			}
			i++;
			usemask <<= 1;
			mask <<= 1;
		} while (mask & 0xFFFF);
	}

	void CostumeRenderer::setPalette(byte *palette)
	{
		if ((_vm->_features & GF_NEW_OPCODES) || (_vm->VAR(_vm->VAR_CURRENT_LIGHTS) & LIGHTMODE_actor_color))
		{
			if ((_loaded._format & 0x42) == 0x42)
			{
				// use system palette directly
				uint32* dp32 = _palette;
				const uint32* sp = c2pfill16;
				*dp32++ = *sp++; *dp32++ = *sp++; *dp32++ = *sp++; *dp32++ = *sp++; *dp32++ = *sp++; *dp32++ = *sp++; *dp32++ = *sp++; *dp32++ = *sp++;
				*dp32++ = *sp++; *dp32++ = *sp++; *dp32++ = *sp++; *dp32++ = *sp++; *dp32++ = *sp++; *dp32++ = *sp++; *dp32++ = *sp++; *dp32++ = *sp++;
			}
			else
			{
				// use loaded/custom palette
				#define COST_MAKE_PAL_LPONLY()	{ c = *lp++; *dp32++ = c2pfill256[c]; dp32++; }
				#define COST_MAKE_PAL()			{ c = *palette++; if (c == 255) c = *lp; lp++; *dp32++ = c2pfill256[c]; dp32++; }

				uint32 c = 0;
				uint32* dp32 = _palette;
				const byte* lp = _loaded._palette;
				if (palette)
				{
					COST_MAKE_PAL(); COST_MAKE_PAL(); COST_MAKE_PAL(); COST_MAKE_PAL();
					COST_MAKE_PAL(); COST_MAKE_PAL(); COST_MAKE_PAL(); COST_MAKE_PAL();
					COST_MAKE_PAL(); COST_MAKE_PAL(); COST_MAKE_PAL(); COST_MAKE_PAL();
					COST_MAKE_PAL(); COST_MAKE_PAL(); COST_MAKE_PAL(); COST_MAKE_PAL();
					if (_loaded._numColors & ~15)
					{
						COST_MAKE_PAL(); COST_MAKE_PAL(); COST_MAKE_PAL(); COST_MAKE_PAL();
						COST_MAKE_PAL(); COST_MAKE_PAL(); COST_MAKE_PAL(); COST_MAKE_PAL();
						COST_MAKE_PAL(); COST_MAKE_PAL(); COST_MAKE_PAL(); COST_MAKE_PAL();
						COST_MAKE_PAL(); COST_MAKE_PAL(); COST_MAKE_PAL(); COST_MAKE_PAL();
					}
				}
				else
				{
					COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY();
					COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY();
					COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY();
					COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY();
					if (_loaded._numColors & ~15)
					{
						COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY();
						COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY();
						COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY();
						COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY(); COST_MAKE_PAL_LPONLY();
					}
				}
			}
		}
		else
		{
			uint32 c32 = c2pfill256[8];
			uint32* dp32 = _palette;
			*dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32;
			*dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32;
			*dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32;
			*dp32++ = 0; *dp32++ = 0; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32;
			if (_loaded._numColors & ~15)
			{
				*dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32;
				*dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32;
				*dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32;
				*dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32; *dp32++ = c32;					
			}
		}
	}

	void CostumeRenderer::setFacing(Actor *a)
	{
		_mirror = newDirToOldDir(a->getFacing()) != 0 || _loaded._mirror;
	}

	void CostumeRenderer::setCostume(int costume)
	{
		_loaded.loadCostume(costume);
	}

	byte LoadedCostume::increaseAnims(Actor *a)
	{
		int i;
		byte r = 0;

		for (i = 0; i != 16; i++)
		{
			if (a->cost.curpos[i] != 0xFFFF)
				r += increaseAnim(a, i);
		}
		return r;
	}

	byte LoadedCostume::increaseAnim(Actor *a, int slot)
	{
		int highflag;
		int i, end;
		byte code, nc;

		if (a->cost.curpos[slot] == 0xFFFF)
			return 0;

		highflag = a->cost.curpos[slot] & 0x8000;
		i = a->cost.curpos[slot] & 0x7FFF;
		end = a->cost.end[slot];
		code = _animCmds[i] & 0x7F;

		do
		{
			if (!highflag)
			{
				if (i++ >= end)
					i = a->cost.start[slot];
			}
			else
			{
				if (i != end)
					i++;
			}
			nc = _animCmds[i];

			if (nc == 0x7C)
			{
				a->cost.animCounter++;
				if (a->cost.start[slot] != end)
					continue;
			}
			else
			{
				if (_vm->_version == 6)
				{
					if (nc >= 0x71 && nc <= 0x78)
					{
						_vm->_sound->addSoundToQueue2(a->sound[nc - 0x71]);
						if (a->cost.start[slot] != end)
							continue;
					}
				}
				else
				{
					if (nc == 0x78)
					{
						a->cost.soundCounter++;
						if (a->cost.start[slot] != end)
							continue;
					}
				}
			}

			a->cost.curpos[slot] = i | highflag;
			return (_animCmds[i] & 0x7F) != code;
		} while (1);
	}

} // End of namespace Scumm
