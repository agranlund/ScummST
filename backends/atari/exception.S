//-------------------------------------------------------------
//	Simple exception handling mechanism with callback
//
//	void InstallExceptionHandler(int vec, int(*handler)(int))
//	void RemoveExceptionHandler(int vec)
//
//	handler looks like:
//		int MyExceptionHandler(int vec)
//
//	return 0 to let previous handler act on it.
//	return 1 if no further handling should be done.
//
// This file is distributed under the GPL v2, or at your option any
// later version.  Read COPYING for details.
// (c)2021, Anders Granlund
//-------------------------------------------------------------

	.text
	.globl	_InstallExceptionHandler
	.globl	_RemoveExceptionHandler

	.bss
	.even
_vec_table_old:
	.ds.l	65
_vec_funcs:
	.ds.l	65

	.text
	.globl _vec_all
_vec_table_new:
	.word	0				// 0
	.word	0
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all		// 10
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all		// 20 
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all		// 30
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all		// 40
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all		// 50
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all		// 60
	bsr.s   _vec_all
	bsr.s   _vec_all
	bsr.s   _vec_all
	.word	0

_vec_all:
	move.l	d0, -(sp)					// save regs
	move.l	a0, -(sp)
	move.l	8(sp),d0					// d0 = old addr + 2
	lea		_vec_table_new+2(pc), a0
	sub.l	a0, d0
	lsl.l	#1, d0						// d0 = absolute vector address

	move.l	d0, -(sp)					// call exception handler
	lea		_vec_funcs, a0
	add.l	d0, a0
	cmp.l	#0, (a0)
	beq		_vec_all_do_old
	move.l	(a0),a0
	jsr		(a0)

	cmp.l	#0, d0						// returned false?
	beq		_vec_all_do_old				// yes -> let TOS handle it
	addq.l	#4, sp						// no -> restore stack and keep going
	move.l	(sp)+, a0
	move.l	(sp)+, d0
	addq.l	#4, sp
	rte

_vec_all_do_old:
	lea		_vec_table_old, a0			// old vector table
	move.l	(sp)+, d0
	add.l	d0, a0						// add offset
	move.l	(a0), 8(sp)					// set as return address
	move.l	(sp)+, a0					// restore regs
	move.l	(sp)+, d0
	rts									// jump to old vector


//-------------------------------------------------------------------
//
//	void InstallExceptionHandler(int nr, int(* func)())
//
//-------------------------------------------------------------------
_InstallExceptionHandler:
	movem.l	d0-d1/a0-a1,-(sp)
	move.l	16+4(sp),d0
	move.l	16+8(sp),d1
	lea		_vec_funcs,a0				// set handler func
	add.l	d0,a0
	move.l	d1,(a0)
	lea		0,a0						// save old vector
	lea		_vec_table_old,a1
	add.l	d0,a0
	add.l	d0,a1
	move.l	(a0),(a1)
	lea		_vec_table_new,a1			// replace vector
	lsr.l	#1,d0
	add.l	d0,a1
	move.l	a1,(a0)
	movem.l	(sp)+,d0-d1/a0-a1
	rts

//-------------------------------------------------------------------
//
// _RemoveExceptionHandler(int nr)
//
//-------------------------------------------------------------------
_RemoveExceptionHandler:
	movem.l	d0/a0-a1,-(sp)
	lea		0,a0
	lea		_vec_table_old,a1
	move.l	12+4(sp),d0
	add.l	d0,a0
	add.l	d0,a1
	move.l	(a1),(a0)
	movem.l	(sp)+,d0/a0-a1
	rts
