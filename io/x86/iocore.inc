

; ---- cgrom.h

				struc	cgwin_t
.low			resd	1
.high			resd	1
.writable		resb	1
				endstruc

	extern	_cgwindow


; ---- crtc.h

				struc	grcg_t
.counter		resd	1
.mode			resw	1
.modereg		resb	2
.tile_b			resw	1
.tile_r			resw	1
.tile_g			resw	1
.tile_e			resw	1
				endstruc

	extern	_grcg


; ---- dmac.h

				struc	dmach_t
.adrs			resd	1		; 00
.leng			resw	1		; 04
.adrsorg		resw	1		; 06
.lengorg		resw	1		; 08
.action			resw	1		; 0a
.outproc		resd	1		; 0c
.inproc			resd	1		; 10
.extproc		resd	1		; 14
.mode			resb	1		; 18
.sreq			resb	1		; 19
.ready			resb	1		; 1a
.mask			resb	1		; 1b
				endstruc		; 1c

				struc	dmac_t
.dmach			resb	(dmach_t_size * 4)
.lh				resd	1
.work			resb	1
.working		resb	1
.mask			resb	1
.stat			resb	1
				endstruc

DMAEXT_START		equ		0
DMAEXT_END			equ		1
DMAEXT_BREAK		equ		2
DMA_INITSIGNALONLY	equ		1

	extern	_dmac


; ---- egc.h

				struc	egc_t
.access			resw	1
.fgbg			resw	1
.ope			resw	1
.fg				resw	1
.mask			resw	1
.bg				resw	1
.sft			resw	1
.leng			resw	1
.lastvram		resd	2
.patreg			resd	2
.fgc			resd	2
.bgc			resd	2

.func			resd	1
.remain			resd	1
.stack			resd	1
.inptr			resd	1
.outptr			resd	1
.mask2			resw	1
.srcmask		resw	1
.srcbit			resb	1
.dstbit			resb	1
.sft8bitl		resb	1
.sft8bitr		resb	1

				resd	4
.buf			resb	(4096/8 + 4*4)
				resd	4
				endstruc

	extern	_egc


; ---- gdc.h

GDCCMD_MAX		equ		32

				struc	gdcd_t
.para			resb	256
.fifo			resw	GDCCMD_MAX
.cnt			resw	1
.ptr			resb	1
.rcv			resb	1
.snd			resb	1
.cmd			resb	1
.paracb			resb	1
.reserved		resb	1
				endstruc

				struc	gdc_t
.m				resb	gdcd_t_size
.s				resb	gdcd_t_size
.mode1			resb	1
.mode2			resb	1
.clock			resb	1
.crt15khz		resb	1
.m_drawing		resb	1
.s_drawing		resb	1
.vsync			resb	1
.vsyncint		resb	1
.display		resb	1
.bitac			resb	1
.reserved		resb	2
.analog			resd	1
.palnum			resd	1
.degpal			resb	4
.anapal			resd	16
.dispclock		resd	1
.vsyncclock		resd	1
.rasterclock	resd	1
.hsyncclock		resd	1
				endstruc

				struc	gdcs_t
.access			resb	1
.disp			resb	1
.textdisp		resb	1
.msw_accessable	resb	1
.grphdisp		resb	1
.palchange		resb	1
.mode2			resb	1
				endstruc

	extern	_gdc
	extern	_gdcs


; ---- pic.h

				struc	picitem_t
.icw			resb	4
.imr			resb	1
.isr			resb	1
.irr			resb	1
.ocw3			resb	1
.pry			resb	1
.writeicw		resb	1
				resb	2
				endstruc

				struc	pic_t
.master			resb	picitem_t_size
.slave			resb	picitem_t_size
				endstruc

	extern	_pic

