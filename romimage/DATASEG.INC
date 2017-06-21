
; ITFは ROMの為、データセグメントは違う場所に定義する

DATASEG			equ		0a800h

int09off		equ		09d0h
int09seg		equ		09d2h
IMR0			equ		09d4h
IMR1			equ		09d5h
DIPSW_1			equ		09d6h
DIPSW_2			equ		09d7h
DIPSW_3			equ		09d8h
KEYSTAT			equ		09d9h
KEYBAK			equ		09dah
MACTYPE			equ		09dbh

LOCATION		equ		09e0h				; word
	LOCATION_X	equ		09e0h
	LOCATION_Y	equ		09e1h

I2ABUFFER		equ		09f0h				; db 11 dup(?)


BAK_UNDER16		equ		09fdh				; byte
BAK_OVER16		equ		09feh				; word

MEMSW_X			equ		0a00h

DATASEGEND		equ		0b00h
