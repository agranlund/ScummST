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
 * $Header: /cvsroot/scummvm/scummvm/common/scummsys.h,v 1.44 2004/01/06 12:45:28 fingolfin Exp $
 *
 */
#ifndef SCUMMSYS_H
#define SCUMMSYS_H

#ifndef _STDAFX_H
#error Included scummsys.h without including stdafx.h first!
#endif

#include <stdlib.h>
#include <stdio.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif

#if defined(__ATARI__)

	#define scumm_stricmp strcmp
	#define scumm_strnicmp strncmp

	#define CHECK_HEAP
	#define SCUMM_BIG_ENDIAN
	#define SCUMM_NEED_ALIGNMENT

	#define CDECL
	#define FORCEINLINE inline
	#define NORETURN __attribute__((__noreturn__))
	#define GCC_PACK __attribute__((packed))
	#define START_PACK_STRUCTS
	#define END_PACK_STRUCTS

	typedef unsigned char byte;
	typedef unsigned char uint8;
	typedef signed char int8;
	typedef unsigned short int uint16;
	typedef signed short int int16;

#ifdef __MSHORT__	
	typedef unsigned long int uintptr_t;
	typedef unsigned long int uint32;
	typedef signed long int int32;
#else
	typedef unsigned int uintptr_t;
	typedef unsigned int uint32;
	typedef signed int int32;
#endif

	typedef unsigned int uint;


	#ifdef NULL
	#undef NULL
	#define NULL 0
	#endif

	#define double float

	#define ALLOC_STRAM_ONLY	0
	#define ALLOC_TTRAM_ONLY	1
	#define ALLOC_STRAM_PREFER	2
	#define ALLOC_TTRAM_PREFER	3
	extern uintptr_t atari_alloc(uint32 size, uint16 mode);

	// TODO_ATARI: implement time()
	#define time(a) 0

	#ifdef RELEASEBUILD
	#ifdef assert
	#undef assert
	#define assert(x) 
	#endif
	#endif

	extern bool	YM2149_LOCK();
	extern void	YM2149_UNLOCK();

	#define 	YM2149_REGSELECT		((volatile byte*)0xFFFF8800)
	#define 	YM2149_REGDATA			((volatile byte*)0xFFFF8802)

	//#define 	YM2149_WR(r,d)			{ *YM2149_REGSELECT = (byte) (r); *YM2149_REGDATA = (byte) (d); }

	#define 	YM2149_WR(r,d) {			\
		uint16 v = (r << 8) | d; 			\
		byte* a = (byte*) 0xFFFF8800;		\
		__asm__ volatile					\
		(									\
			"movep.w	%0,0(%1)\n\t"		\
		: : "d"(v), "a"(a) : "memory", "cc"	\
		); }


	#define 	YM2149_RD(r,d)			{ *YM2149_REGSELECT = (byte) (r); d = *YM2149_REGSELECT; }

	#define 	YM2149_WR_MASKED(r,d,m)	{ byte b = 0; YM2149_RD(r, b); b &= ~(m); b |= ((d)&(m)); YM2149_WR(r, b); }

	#define		M68K_MOVEP(_addr,_off,_data)	{ __asm__ volatile ( "movep.l %1," #_off"(%0)\n\t" : : "a"(_addr), "d"(_data) : "memory" ); }
	#define		M68K_SWAP(_data)				{ __asm__ volatile ( "swap %0\n\t" : "=d"(_data) : "0"(_data) : ); }
	#define		M68K_ROLB(_off,_data)			{ __asm__ volatile ( "rol.b #"#_off", %0\n\t" : "=d"(_data) : "0"(_data) : ); }
	#define		M68K_ROLW(_off,_data)			{ __asm__ volatile ( "rol.w #"#_off", %0\n\t" : "=d"(_data) : "0"(_data) : ); }
	#define		M68K_ROLL(_off,_data)			{ __asm__ volatile ( "rol.l #"#_off", %0\n\t" : "=d"(_data) : "0"(_data) : ); }
	#define		M68K_RORB(_off,_data)			{ __asm__ volatile ( "ror.b #"#_off", %0\n\t" : "=d"(_data) : "0"(_data) : ); }
	#define		M68K_RORW(_off,_data)			{ __asm__ volatile ( "ror.w #"#_off", %0\n\t" : "=d"(_data) : "0"(_data) : ); }
	#define		M68K_RORL(_off,_data)			{ __asm__ volatile ( "ror.l #"#_off", %0\n\t" : "=d"(_data) : "0"(_data) : ); }


#elif defined(__WIN32__)

	#define scumm_stricmp strcasecmp
	#define scumm_strnicmp strncasecmp

	#define CHECK_HEAP
	#define SCUMM_LITTLE_ENDIAN
	#define SCUMM_NEED_ALIGNMENT

	#define CDECL 

	#ifndef FORCEINLINE
	#define FORCEINLINE inline
	#endif

	#ifndef _HEAPOK
	#define _HEAPOK 0
	#endif

	#define NORETURN __attribute__((__noreturn__))
	#define GCC_PACK __attribute__((packed))

	typedef unsigned char byte;
	typedef unsigned char uint8;
	typedef unsigned short uint16;
	typedef unsigned int uint32;
	typedef unsigned int uint;
	typedef signed char int8;
	typedef signed short int16;
	typedef signed int int32;

	#define START_PACK_STRUCTS pack (push, 1)
	#define END_PACK_STRUCTS	 pack(pop)

	#define M68K_MOVEP(_addr,_off,_data) \
	{ \
		byte* d = (byte*)(_addr); \
		d[(_off)+0] = (data >> 24) & 0xFF; \
		d[(_off)+2] = (data >> 16) & 0xFF; \
		d[(_off)+4] = (data >>  8) & 0xFF; \
		d[(_off)+6] = (data >>  0) & 0xFF; \
	}

	#define		M68K_SWAP(_off,_data) { _data = ((_data >> 16) | (_data << 16));			}

	#define		M68K_ROLB(_off,_data) { _data = ((data << _off) | (data >> (8 - off)));	}
	#define		M68K_ROLW(_off,_data) { _data = ((data << _off) | (data >> (16 - off))); }
	#define		M68K_ROLL(_off,_data) { _data = ((data << _off) | (data >> (32 - off))); }

	#define		M68K_RORB(_off,_data) { _data = ((data >> _off) | (data << (8 - off)));	}
	#define		M68K_RORW(_off,_data) { _data = ((data >> _off) | (data << (16 - off))); }
	#define		M68K_RORL(_off,_data) { _data = ((data >> _off) | (data << (32 - off))); }

#else
	#error No system type defined
#endif

FORCEINLINE uint32 SWAP_BYTES_32(uint32 a) {
	return ((a >> 24) & 0x000000FF) |
		   ((a >>  8) & 0x0000FF00) |
		   ((a <<  8) & 0x00FF0000) |
		   ((a << 24) & 0xFF000000);
}

FORCEINLINE uint16 SWAP_BYTES_16(uint16 a) {
	return ((a >> 8) & 0x00FF) + ((a << 8) & 0xFF00);
}



#if defined(SCUMM_LITTLE_ENDIAN)

	#define PROTO_MKID(a) ((uint32) \
			(((a) >> 24) & 0x000000FF) | \
			(((a) >>  8) & 0x0000FF00) | \
			(((a) <<  8) & 0x00FF0000) | \
			(((a) << 24) & 0xFF000000))
	#define PROTO_MKID_BE(a) ((uint32)(a))

	#if defined(INVERSE_MKID)
	#  define MKID(a) PROTO_MKID_BE(a)
	#  define MKID_BE(a) PROTO_MKID(a)
	#else
	#  define MKID(a) PROTO_MKID(a)
	#  define MKID_BE(a) PROTO_MKID_BE(a)
	#endif

	#define READ_UINT32(a) READ_LE_UINT32(a)

	#define FROM_LE_32(a) ((uint32)(a))
	#define FROM_LE_16(a) ((uint16)(a))

	#define TO_LE_32(a) ((uint32)(a))
	#define TO_LE_16(a) ((uint16)(a))

	#define TO_BE_32(a) SWAP_BYTES_32(a)
	#define TO_BE_16(a) SWAP_BYTES_16(a)

#elif defined(SCUMM_BIG_ENDIAN)

	#define MKID(a) ((uint32)(a))
	#define MKID_BE(a) ((uint32)(a))
	//#define MKID_BE(a) SWAP_BYTES_32(a)

	#define READ_UINT32(a) READ_BE_UINT32(a)

	#define FROM_LE_32(a) SWAP_BYTES_32(a)
	#define FROM_LE_16(a) SWAP_BYTES_16(a)

	#define TO_LE_32(a) SWAP_BYTES_32(a)
	#define TO_LE_16(a) SWAP_BYTES_16(a)

	#define TO_BE_32(a) ((uint32)(a))
	#define TO_BE_16(a) ((uint16)(a))

#else

	#error No endianness defined

#endif


#if defined(SCUMM_NEED_ALIGNMENT) || defined(SCUMM_BIG_ENDIAN)
	FORCEINLINE uint16 READ_LE_UINT16(const void *ptr) {
		const byte *b = (const byte *)ptr;
		return (b[1] << 8) + b[0];
	}
	FORCEINLINE uint32 READ_LE_UINT32(const void *ptr) {
		const byte *b = (const byte *)ptr;
		return (b[3] << 24) + (b[2] << 16) + (b[1] << 8) + (b[0]);
	}
	FORCEINLINE void WRITE_LE_UINT16(void *ptr, uint16 value) {
		byte *b = (byte *)ptr;
		b[0] = (byte)(value >> 0);
		b[1] = (byte)(value >> 8);
	}
	FORCEINLINE void WRITE_LE_UINT32(void *ptr, uint32 value) {
		byte *b = (byte *)ptr;
		b[0] = (byte)(value >>  0);
		b[1] = (byte)(value >>  8);
		b[2] = (byte)(value >> 16);
		b[3] = (byte)(value >> 24);
	}
#else
	FORCEINLINE uint16 READ_LE_UINT16(const void *ptr) {
		return *(const uint16 *)(ptr);
	}
	FORCEINLINE uint32 READ_LE_UINT32(const void *ptr) {
		return *(const uint32 *)(ptr);
	}
	FORCEINLINE void WRITE_LE_UINT16(void *ptr, uint16 value) {
		*(uint16 *)(ptr) = value;
	}
	FORCEINLINE void WRITE_LE_UINT32(void *ptr, uint32 value) {
		*(uint32 *)(ptr) = value;
	}
#endif


#if defined(SCUMM_NEED_ALIGNMENT) || defined(SCUMM_LITTLE_ENDIAN)
	FORCEINLINE uint16 READ_BE_UINT16(const void *ptr) {
		const byte *b = (const byte *)ptr;
		return (b[0] << 8) + b[1];
	}
	FORCEINLINE uint32 READ_BE_UINT32(const void *ptr) {
		const byte *b = (const byte*)ptr;
		return (b[0] << 24) + (b[1] << 16) + (b[2] << 8) + (b[3]);
	}
	FORCEINLINE void WRITE_BE_UINT16(void *ptr, uint16 value) {
		byte *b = (byte *)ptr;
		b[0] = (byte)(value >> 8);
		b[1] = (byte)(value >> 0);
	}
	FORCEINLINE void WRITE_BE_UINT32(void *ptr, uint32 value) {
		byte *b = (byte *)ptr;
		b[0] = (byte)(value >> 24);
		b[1] = (byte)(value >> 16);
		b[2] = (byte)(value >>  8);
		b[3] = (byte)(value >>  0);
	}

#else
	FORCEINLINE uint16 READ_BE_UINT16(const void *ptr) {
		return *(const uint16 *)(ptr);
	}
	FORCEINLINE uint32 READ_BE_UINT32(const void *ptr) {
		return *(const uint32 *)(ptr);
	}
	FORCEINLINE void WRITE_BE_UINT16(void *ptr, uint16 value) {
		*(uint16 *)(ptr) = value;
	}
	FORCEINLINE void WRITE_BE_UINT32(void *ptr, uint32 value) {
		*(uint32 *)(ptr) = value;
	}
#endif

#ifdef SCUMM_BIG_ENDIAN
	#define WRITE_UINT32	WRITE_BE_UINT32
#else
	#define WRITE_UINT32	WRITE_LE_UINT32
#endif

	typedef byte NewGuiColor;

#endif
