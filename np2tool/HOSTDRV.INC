
%define	VERSION_ID		"20040122"


				struc	HDRVIF
.drive_no			resb	1
.dosver				resw	1
.sda_off			resw	1
.sda_seg			resw	1
				endstruc

				struc	LOL
					resb	22
.cds				resd	1
					resb	7
.lastdrv			resb	1
				endstruc

				struc	CDS
.cur				resb	67
.flag				resw	1
					resb	10
.root				resw	1
.size
				endstruc

