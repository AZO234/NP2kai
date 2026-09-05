; HOSTD9X VxD descriptor block.
; Keep the DDB in MASM so that the COFF symbol is exactly HOSTD9X_DDB
; (without the leading underscore added by the C compiler).

        TITLE   HOSTD9X
        .386P

        INCLUDE VMM.INC

        EXTRN   _HOSTD9X_Control@0:NEAR

VxD_LOCKED_DATA_SEG

        PUBLIC  HOSTD9X_DDB
HOSTD9X_DDB VxD_Desc_Block <,,UNDEFINED_DEVICE_ID,1,0,,"HOSTD9X ",FSD_Init_Order,OFFSET32 _HOSTD9X_Control@0>

VxD_LOCKED_DATA_ENDS

        END
