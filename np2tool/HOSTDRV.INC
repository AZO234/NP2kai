
%define	VERSION_ID		"20260718"


				struc	HDRVIF
.drive_no			resb	1
.dosver				resw	1
.sda_off			resw	1
.sda_seg			resw	1
.handoff_sig		resb	4
.cds_off			resw	1
.cds_seg			resw	1
				endstruc

%define	HDRV_HANDOFF_SIG	"H9X2"

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

