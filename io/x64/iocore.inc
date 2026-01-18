

; ---- cgrom.h

				struc	cgwin_t
.low			resd	1
.high			resd	1
.writable		resb	1
				endstruc

	extern	cgwindow


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

	extern	grcg


; ---- dmac.h

				struc	dmach_t		; 86:64
.adrs			resd	1			; 00:00
.leng			resw	1			; 04:04
.adrsorg		resw	1			; 06:06
.lengorg		resw	1			; 08:08
.action			resw	1			; 0a:0a
				resd	1			; --:0c
.outproc		resq	1			; 0c:10
.inproc			resq	1			; 10:18
.extproc		resq	1			; 14:20
.mode			resb	1			; 18:28
.sreq			resb	1			; 19:29
.ready			resb	1			; 1a:2a
.mask			resb	1			; 1b:2b
				resd	1			; --:2c
.size								; 1c:30
				endstruc

				struc	dmac_t
.dmach			resb	(dmach_t.size * 4)
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

	extern	dmac


; ---- egc.h

				struc	egc_t		; 86:64
.access			resw	1			; 00
.fgbg			resw	1			; 02
.ope			resw	1			; 04
.fg				resw	1			; 06
.mask			resw	1			; 08
.bg				resw	1			; 0a
.sft			resw	1			; 0c
.leng			resw	1			; 0e
.lastvram		resd	2			; 10
.patreg			resd	2			; 18
.fgc			resd	2			; 20
.bgc			resd	2			; 28

.func			resd	1			; 30
.remain			resd	1			; 34
.stack			resd	1			; 38
				resd	1
.inptr			resq	1			; 3c
.outptr			resq	1			; 40
.mask2			resw	1			; 44
.srcmask		resw	1			; 46
.srcbit			resb	1			; 48
.dstbit			resb	1			; 49
.sft8bitl		resb	1			; 4a
.sft8bitr		resb	1			; 4b

				resd	4
.buf			resb	(4096/8 + 4*4)
				resd	4
				endstruc

	extern	egc


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

	extern	gdc
	extern	gdcs


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

	extern	pic

