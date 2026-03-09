
SCRN_BUFSIZE	equ		0x4b000


				struc	vramop_t
.operate		resd	1
.tramwait		resd	1
.vramwait		resd	1
.grcgwait		resd	1
				endstruc

				struc	dsync_t
.text_vbp		resd	1
.textymax		resd	1
.grph_vbp		resd	1
.grphymax		resd	1

.scrnxpos		resd	1
.scrnxmax		resd	1
.scrnxextend	resd	1
.scrnymax		resd	1
.textvad		resd	1
.grphvad		resd	1
				endstruc

