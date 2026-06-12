/**
 * @file	npdispdef.h
 * @brief	Definition of the Neko Project II Display Adapter
 */

#pragma once

#if defined(SUPPORT_WAB_NPDISP)

#define NPDISP_DEVTYPE_DIBENG	0x5250
#define NPDISP_DEVTYPE			NPDISP_DEVTYPE_DIBENG
#define NPDISP_DEVTYPE_DDB		0x2222
#define NPDISP_EXEC_MAGIC	0x3132504e

#define NPDISP_RETCODE_NONE		0
#define NPDISP_RETCODE_SUCCESS	1
#define NPDISP_RETCODE_FAILED	2

// 関数番号　特に意味は無いがSDK記載の序数と合わせておく
#define NPDISP_FUNCORDER_NP2INITIALIZE			0 // 初期化用
#define NPDISP_FUNCORDER_Enable					5
#define NPDISP_FUNCORDER_Disable				4
#define NPDISP_FUNCORDER_RealizeObject			10
#define NPDISP_FUNCORDER_BitBlt					1
#define NPDISP_FUNCORDER_BitmapBits				30
#define NPDISP_FUNCORDER_ColorInfo				2
#define NPDISP_FUNCORDER_Control				3
#define NPDISP_FUNCORDER_CreateDIBitmap			20
#define NPDISP_FUNCORDER_DeviceBitmap			16
#define NPDISP_FUNCORDER_DeviceBitmapBits		19
#define NPDISP_FUNCORDER_DeviceMode				13
#define NPDISP_FUNCORDER_EnumDFonts				6
#define NPDISP_FUNCORDER_EnumObj				7
#define NPDISP_FUNCORDER_ExtDeviceMode			90
#define NPDISP_FUNCORDER_ExtTextOut				14
#define NPDISP_FUNCORDER_GetCharWidth			15
#define NPDISP_FUNCORDER_GetDriverResourceID	450
#define NPDISP_FUNCORDER_GetPalette				23
#define NPDISP_FUNCORDER_GetPalTrans			25
#define NPDISP_FUNCORDER_Output					8
#define NPDISP_FUNCORDER_Pixel					9
#define NPDISP_FUNCORDER_ScanLR					12
#define NPDISP_FUNCORDER_SelectBitmap			29
#define NPDISP_FUNCORDER_SetAttribute			18
#define NPDISP_FUNCORDER_SetDIBitsToDevice		21
#define NPDISP_FUNCORDER_SetPalette				22
#define NPDISP_FUNCORDER_SetPalTrans			24
#define NPDISP_FUNCORDER_StrBlt					11
#define NPDISP_FUNCORDER_StretchBlt				27
#define NPDISP_FUNCORDER_StretchDIBits			28
#define NPDISP_FUNCORDER_UpdateColors			26
#define NPDISP_FUNCORDER_CheckCursor			104
#define NPDISP_FUNCORDER_FastBorder				17
#define NPDISP_FUNCORDER_Inquire				101
#define NPDISP_FUNCORDER_MoveCursor				103
#define NPDISP_FUNCORDER_SaveScreenBitmap		92
#define NPDISP_FUNCORDER_SetCursor				102
#define NPDISP_FUNCORDER_UserRepaintDisable		500 // DDK HELPにないがこれがないとプログラム終了時に例外 
#define NPDISP_FUNCORDER_DCI_BEGINACCESS		0xfe00
#define NPDISP_FUNCORDER_DCI_ENDACCESS			0xfe01
#define NPDISP_FUNCORDER_DCI_DESTROYSURFACE		0xfe02
#define NPDISP_FUNCORDER_INT2Fh					0xff2f // 序数がないので0xff2fとしておく
#define NPDISP_FUNCORDER_MEMORYMAP				0xfffc // 序数がないので0xfffcとしておく
#define NPDISP_FUNCORDER_WEP					0xffff // 序数がないので0xffffとしておく
// 以降 Win9x用
#define NPDISP_FUNCORDER_ReEnable				31
#define NPDISP_FUNCORDER_ValidateMode			700
#define NPDISP_FUNCORDER_SelectBitmap			29
#define NPDISP_FUNCORDER_BitmapBits				30

#define NPDISP_DT_RASDISPLAY	1

#define NPDISP_CC_CIRCLES		1
#define NPDISP_CC_PIE			2
#define NPDISP_CC_CHORD			4
#define NPDISP_CC_ELLIPSES		8
#define NPDISP_CC_WIDE			16
#define NPDISP_CC_STYLED		32
#define NPDISP_CC_WIDESTYLED	64
#define NPDISP_CC_INTERIORS		128
#define NPDISP_CC_ROUNDRECT		256
#define NPDISP_CC_POLYBEZIER	512

#define NPDISP_LC_POLYLINE		2
#define NPDISP_LC_MARKER		4
#define NPDISP_LC_POLYMARKER	8
#define NPDISP_LC_WIDE			16
#define NPDISP_LC_STYLED		32
#define NPDISP_LC_WIDESTYLED	64
#define NPDISP_LC_INTERIORS		128

#define NPDISP_PC_POLYGON		1
#define NPDISP_PC_RECTANGLE		2
#define NPDISP_PC_WINDPOLYGON	4
#define NPDISP_PC_SCANLINE		8
#define NPDISP_PC_WIDE			16
#define NPDISP_PC_STYLED		32
#define NPDISP_PC_WIDESTYLED	64
#define NPDISP_PC_INTERIORS		128
#define NPDISP_PC_POLYPOLYGON	256
#define NPDISP_PC_PATHS			512

#define NPDISP_CP_RECTANGLE		1

#define NPDISP_TC_OP_CHARACTER	0x0001
#define NPDISP_TC_OP_STROKE		0x0002
#define NPDISP_TC_CP_STROKE		0x0004
#define NPDISP_TC_CR_90			0x0008
#define NPDISP_TC_CR_ANY		0x0010
#define NPDISP_TC_SF_X_YINDEP	0x0020
#define NPDISP_TC_SA_DOUBLE		0x0040
#define NPDISP_TC_SA_INTEGER	0x0080
#define NPDISP_TC_SA_CONTIN		0x0100
#define NPDISP_TC_EA_DOUBLE		0x0200
#define NPDISP_TC_IA_ABLE		0x0400
#define NPDISP_TC_UA_ABLE		0x0800
#define NPDISP_TC_SO_ABLE		0x1000
#define NPDISP_TC_RA_ABLE		0x2000
#define NPDISP_TC_VA_ABLE		0x4000

#define NPDISP_RC_BITBLT		0x0001
#define NPDISP_RC_BANDING		0x0002
#define NPDISP_RC_SCALING		0x0004
#define NPDISP_RC_BITMAP64		0x0008
#define NPDISP_RC_GDI20_OUTPUT	0x0010
#define NPDISP_RC_GDI20_STATE	0x0020
#define NPDISP_RC_SAVEBITMAP	0x0040
#define NPDISP_RC_DI_BITMAP		0x0080
#define NPDISP_RC_PALETTE		0x0100
#define NPDISP_RC_DIBTODEV		0x0200
#define NPDISP_RC_BIGFONT		0x0400
#define NPDISP_RC_STRETCHBLT	0x0800
#define NPDISP_RC_FLOODFILL		0x1000
#define NPDISP_RC_STRETCHDIB	0x2000
#define NPDISP_RC_OP_DX_OUTPUT	0x4000
#define NPDISP_RC_DEVBITS		0x8000

#define NPDISP_C1_TRANSPARENT	0x0001
#define NPDISP_C1_DIBENGINE 	0x0010
#define NPDISP_C1_REINIT_ABLE	0x0080
#define NPDISP_C1_COLORCURSOR	0x0800
#define NPDISP_C1_SLOW_CARD		0x2000

#define NPDISP_PEN_STYLE_SOLID			0
#define NPDISP_PEN_STYLE_DASHED			1
#define NPDISP_PEN_STYLE_DOTTED			2
#define NPDISP_PEN_STYLE_DOTDASHED		3
#define NPDISP_PEN_STYLE_DASHDOTDOT 	4
#define NPDISP_PEN_STYLE_NOLINE			5
#define NPDISP_PEN_STYLE_INSIDEFRAME	6

#define NPDISP_BRUSH_STYLE_SOLID		0
#define NPDISP_BRUSH_STYLE_HOLLOW		1
#define NPDISP_BRUSH_STYLE_HATCHED		2
#define NPDISP_BRUSH_STYLE_PATTERN		3

#define NPDISP_BRUSH_HATCH_HORIZONTAL	0
#define NPDISP_BRUSH_HATCH_VERTICAL		1
#define NPDISP_BRUSH_HATCH_FDIAGONAL	2
#define NPDISP_BRUSH_HATCH_BDIAGONAL	3
#define NPDISP_BRUSH_HATCH_CROSS		4
#define NPDISP_BRUSH_HATCH_DIAGCROSS	5

#define NPDISP_DBB_SET 1
#define NPDISP_DBB_GET 2
#define NPDISP_DBB_COPY 4
#define NPDISP_DBB_SETWITHFILLER 8

#define NPDISP_QDI_SETDIBITS                1
#define NPDISP_QDI_GETDIBITS                2
#define NPDISP_QDI_DIBTOSCREEN              4
#define NPDISP_QDI_STRETCHDIB               8

#define NPDISP_VALMODE_YES			0
#define NPDISP_VALMODE_NO_WRONGDRV	1
#define NPDISP_VALMODE_NO_NOMEM		2
#define NPDISP_VALMODE_NO_NODAC		3
#define NPDISP_VALMODE_NO_UNKNOWN	4

#define NPDISP_CONTROL_SETCOLORTABLE        4
#define NPDISP_CONTROL_GETCOLORTABLE        5
#define NPDISP_CONTROL_QUERYESCSUPPORT		8
#define NPDISP_CONTROL_QUERYDIBSUPPORT		3073
#define NPDISP_CONTROL_DCICOMMAND			3075

#define NPDISP_CONTROL_OPENGL_CMD			4352
#define NPDISP_CONTROL_OPENGL_GETINFO		4353
#define NPDISP_CONTROL_WNDOBJ_SETUP			4354

#define NPDISP_CONTROL_NP2DCIENABLE			0x7222
#define NPDISP_CONTROL_NP2DCIDISABLE		0x7223

#define NPDISP_CONTROL_DCI_DCICREATEPRIMARYSURFACE		1
#define NPDISP_CONTROL_DCI_DCICREATEOFFSCREENSURFACE	2
#define NPDISP_CONTROL_DCI_DCICREATEOVERLAYSURFACE		3
#define NPDISP_CONTROL_DCI_DCIENUMSURFACE				4
#define NPDISP_CONTROL_DCI_DCIESCAPE					5

#define NPDISP_CONTROL_DCI_DDCREATEDRIVEROBJECT	10
#define NPDISP_CONTROL_DCI_DDGET32BITDRIVERNAME	11
#define NPDISP_CONTROL_DCI_DDNEWCALLBACKFNS		12

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)

	typedef struct _tagNPDISP_REQUEST
	{
		UINT16 version;
		UINT16 funcOrder;
		UINT16 returnCode;
		UINT16 reserved;
		union
		{
			struct
			{
				UINT16 dpiX;
				UINT16 dpiY;
				UINT16 width;
				UINT16 height;
				UINT16 bpp;
				UINT16 isWin9x;
				UINT32 bmpinfoAddr;
				UINT32 beginAccessAddr;
				UINT32 endAccessAddr;
				UINT32 dcibufAddr;
				UINT32 dciBeginAccessAddr;
				UINT32 dciEndAccessAddr;
				UINT32 dciDestroySurfaceAddr;
				UINT32 vramLinearAddr;
				UINT32 vramPhysicalAddr;
			} init;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpDevInfoAddr;
				UINT16 wStyle;
				UINT32 lpDestDevTypeAddr;
				UINT32 lpOutputFileAddr;
				UINT32 lpDataAddr;
			} enable;
			struct
			{
				UINT32 lpDestDevAddr;
			} disable;
			struct
			{
				UINT32 lpRetValueAddr;
				SINT16 iResId;
				UINT32 lpResTypeAddr;
			} GetDriverResourceID;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpDestDevAddr;
				UINT32 dwColorin;
				UINT32 lpPColorAddr;
			} ColorInfo;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpDestDevAddr;
				UINT16 wStyle;
				UINT32 lpInObjAddr;
				UINT32 lpOutObjAddr;
				UINT32 lpTextXFormAddr;
			} RealizeObject;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpDestDevAddr;
				UINT16 wFunction;
				UINT32 lpInDataAddr;
				UINT32 lpOutDataAddr;
			} Control;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpDestDevAddr;
				SINT16 wDestX;
				SINT16 wDestY;
				UINT32 lpSrcDevAddr;
				SINT16 wSrcX;
				SINT16 wSrcY;
				UINT16 wXext;
				UINT16 wYext;
				UINT32 Rop3;
				UINT32 lpPBrushAddr;
				UINT32 lpDrawModeAddr;
			} BitBlt;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpBitmapAddr;
				UINT16 fGet;
				UINT16 iStart;
				UINT16 cScans;
				UINT32 lpDIBitsAddr;
				UINT32 lpBitmapInfoAddr;
				UINT32 lpDrawModeAddr;
				UINT32 lpTranslateAddr;
			} DeviceBitmapBits;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpDestDevAddr;
				SINT16 X;
				SINT16 Y;
				UINT16 iScan;
				UINT16 cScans;
				UINT32 lpClipRectAddr;
				UINT32 lpDrawModeAddr;
				UINT32 lpDIBitsAddr;
				UINT32 lpBitmapInfoAddr;
				UINT32 lpTranslateAddr;
			} SetDIBitsToDevice;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpRect;
				UINT16 wCommand;
			} SaveScreenBitmap;
			struct
			{
				UINT16 wAbsX;
				UINT16 wAbsY;
			} MoveCursor;
			struct
			{
				UINT32 lpCursorShapeAddr;
			} SetCursor;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpDestDevAddr;
				SINT16 wDestXOrg;
				SINT16 wDestYOrg;
				UINT32 lpClipRectAddr;
				UINT32 lpStringAddr;
				SINT16 wCount;
				UINT32 lpFontInfoAddr;
				UINT32 lpDrawModeAddr;
				UINT32 lpTextXFormAddr;
				UINT32 lpCharWidthsAddr;
				UINT32 lpOpaqueRectAddr;
				UINT16 wOptions;
			} extTextOut;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpDestDevAddr;
				SINT16 wDestXOrg;
				SINT16 wDestYOrg;
				UINT32 lpClipRectAddr;
				UINT32 lpStringAddr;
				SINT16 wCount;
				UINT32 lpFontInfoAddr;
				UINT32 lpDrawModeAddr;
				UINT32 lpTextXFormAddr;
			} strBlt;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpDestDevAddr;
				UINT16 wStyle;
				UINT16 wCount;
				UINT32 lpPointsAddr;
				UINT32 lpPPenAddr;
				UINT32 lpPBrushAddr;
				UINT32 lpDrawModeAddr;
				UINT32 lpClipRectAddr;
			} output;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpRectAddr;
				UINT16 wHorizBorderThick;
				UINT16 wVertBorderThick;
				UINT32 dwRasterOp;
				UINT32 lpDestDevAddr;
				UINT32 lpPBrushAddr;
				UINT32 lpDrawModeAddr;
				UINT32 lpClipRectAddr;
			} fastBorder;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpDestDevAddr;
				UINT16 X;
				UINT16 Y;
				UINT32 dwPhysColor;
				UINT32 lpDrawModeAddr;
			} pixel;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpDestDevAddr;
				UINT16 X;
				UINT16 Y;
				UINT32 dwPhysColor;
				UINT16 Style;
			} scanLR;
			struct
			{
				UINT32 lpRetValueAddr; // 0=Complete, 1=hasData
				UINT32 lpDestDevAddr;
				UINT16 wStyle; // 1=pen, 2=brush
				UINT16 enumIdx; // 返すオブジェクトの要素番号
				UINT32 lpLogObjAddr; // オブジェクトの内容書き込み先
			} enumObj;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpDestDevAddr;
				SINT16 wDestX;
				SINT16 wDestY;
				SINT16 wDestXext;
				SINT16 wDestYext;
				UINT32 lpSrcDevAddr;
				SINT16 wSrcX;
				SINT16 wSrcY;
				SINT16 wSrcXext;
				SINT16 wSrcYext;
				UINT32 Rop3;
				UINT32 lpPBrushAddr;
				UINT32 lpDrawModeAddr;
				UINT32 lpClipAddr;
			} stretchBlt;
			struct
			{
				UINT16 nStartIndex;
				UINT16 nNumEntries;
				UINT32 lpPaletteAddr;
			} getPalette;
			struct
			{
				UINT16 nStartIndex;
				UINT16 nNumEntries;
				UINT32 lpPaletteAddr;
			} setPalette;
			struct
			{
				UINT32 lpIndexesAddr;
			} getPalTrans;
			struct
			{
				UINT32 lpIndexesAddr;
			} setPalTrans;
			struct
			{
				SINT16 wStartX;
				SINT16 wStartY;
				UINT16 wExtX;
				UINT16 wExtY;
				UINT32 lpTranslateAddr;
			} updateColors;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpDestDevAddr;
				UINT32 lpBufferAddr;
				UINT16 wFirstChar;
				UINT16 wLastChar;
				UINT32 lpFontInfoAddr;
				UINT32 lpDrawModeAddr;
				UINT32 lpFontTransAddr;
			} getCharWidth;
			struct
			{
				UINT16 ax;
			} INT2Fh;
			struct
			{
				UINT32 bSystemExit;
			} WEP;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpPDeviceAddr;
				UINT32 lpGDIInfoAddr;
			} reEnable;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpValModeAddr;
			} validateMode;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpDeviceAddr;
				UINT32 lpPrevBitmapAddr;
				UINT32 lpBitmapAddr;
				UINT32 fFlags;
			} selectBitmap;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpDeviceAddr;
				UINT32 fFlags;
				UINT32 dwCount;
				UINT32 lpBitsAddr;
			} bitmapBits;
			struct
			{
				UINT32 lpRetValueAddr;
				UINT32 lpPDevice;
				UINT16 fGet;
				SINT16 DestX;
				SINT16 DestY;
				SINT16 DestXE;
				SINT16 DestYE;
				UINT16 SrcX;
				UINT16 SrcY;
				UINT16 SrcXE;
				UINT16 SrcYE;
				UINT32 lpBitsAddr;
				UINT32 lpBitmapInfoAddr;
				UINT32 lpTranslateAddr;
				UINT32 dwROP;
				UINT32 lpPBrushAddr;
				UINT32 lpDrawModeAddr;
				UINT32 lpClipRecAddr;
			} stretchDIBits;
			struct
			{
				UINT32 physicalAddr;
				UINT32 linearAddr;
				UINT16 farSelector;
				UINT32 farOffset;
			} MEMORYMAP;
			struct
			{
				UINT32 lpDeviceAddr;
				UINT32 lpRectAddr;
			} DCI_BeginAccess;
			struct
			{
				UINT32 lpDeviceAddr;
			} DCI_EndAccess;
			struct
			{
				UINT32 lpDeviceAddr;
			} DCI_DestroySurface;
			struct
			{
				UINT16 arguments[20];
			} others;
		} parameters;
	} NPDISP_REQUEST;

#pragma pack(pop)

#pragma pack(push, 2)
	typedef struct {
		SINT16  x;
		SINT16  y;
	} NPDISP_POINT;
	typedef struct {
		SINT16 left;
		SINT16 top;
		SINT16 right;
		SINT16 bottom;
	} NPDISP_RECT;
	typedef struct {
		SINT16 dpVersion;
		SINT16 dpTechnology;
		SINT16 dpHorzSize;
		SINT16 dpVertSize;
		SINT16 dpHorzRes;
		SINT16 dpVertRes;
		SINT16 dpBitsPixel;
		SINT16 dpPlanes;
		SINT16 dpNumBrushes;
		SINT16 dpNumPens;
		SINT16 futureuse;
		SINT16 dpNumFonts;
		SINT16 dpNumColors;
		UINT16 dpDEVICEsize;
		UINT16 dpCurves;
		UINT16 dpLines;
		UINT16 dpPolygonals;
		UINT16 dpText;
		UINT16 dpClip;
		UINT16 dpRaster;
		SINT16 dpAspectX;
		SINT16 dpAspectY;
		SINT16 dpAspectXY;
		SINT16 dpStyleLen;
		NPDISP_POINT dpMLoWin;
		NPDISP_POINT dpMLoVpt;
		NPDISP_POINT dpMHiWin;
		NPDISP_POINT dpMHiVpt;
		NPDISP_POINT dpELoWin;
		NPDISP_POINT dpELoVpt;
		NPDISP_POINT dpEHiWin;
		NPDISP_POINT dpEHiVpt;
		NPDISP_POINT dpTwpWin;
		NPDISP_POINT dpTwpVpt;
		SINT16 dpLogPixelsX;
		SINT16 dpLogPixelsY;
		SINT16 dpDCManage;
		SINT16 dpCaps1;
		SINT32 dpSpotSizeX;
		SINT32 dpSpotSizeY;
		SINT16 dpPalColors;
		SINT16 dpPalReserved;
		SINT16 dpPalResolution;
	} NPDISP_GDIINFO;
	typedef struct {
		char dmDeviceName[32];
		UINT16 dmSpecVersion;
		UINT16 dmDriverVersion;
		UINT16 dmSize;
		UINT16 dmDriverExtra;
		UINT32 dmFields;
		SINT16 dmOrientation;
		SINT16 dmPaperSize;
		SINT16 dmPaperLength;
		SINT16 dmPaperWidth;
		SINT16 dmScale;
		SINT16 dmCopies;
		SINT16 dmDefaultSource;
		SINT16 dmPrintQuality;
		SINT16 dmColor;
		SINT16 dmDuplex;
		SINT16 dmYResolution;
		SINT16 dmTTOption;
	} NPDISP_DEVMODE;
	typedef struct {
		SINT16 txfHeight;
		SINT16 txfWidth;
		SINT16 txfEscapement;
		SINT16 txfOrientation;
		SINT16 txfWeight;
		char txfItalic;
		char txfUnderline;
		char txfStrikeOut;
		char txfOutPrecision;
		char txfClipPrecision;
		SINT16 txfAccelerator;
		SINT16 txfOverhang;
	} NPDISP_TEXTXFORM;
	typedef struct {
		SINT16 opnStyle;
		NPDISP_POINT lopnWidth;
		SINT32 lopnColor;
	} NPDISP_LPEN;
	typedef struct {
		SINT16 lbStyle;
		SINT32 lbColor;
		SINT16 lbHatch;
		SINT32 lbBkColor;
	} NPDISP_LBRUSH;
	typedef struct {
		SINT16 lfHeight;
		SINT16 lfWidth;
		SINT16 lfEscapement;
		SINT16 lfOrientation;
		SINT16 lfWeight;
		UINT8 lfItalic;
		UINT8 lfUnderline;
		UINT8 lfStrikeOut;
		UINT8 lfCharSet;
		UINT8 lfOutPrecision;
		UINT8 lfClipPrecision;
		UINT8 lfQuality;
		UINT8 lfPitchAndFamily;
		char lfFaceName[32];
	} NPDISP_LFONT;

	typedef struct {
		SINT16 bmType;
		SINT16 bmWidth;
		SINT16 bmHeight;
		SINT16 bmWidthBytes;
		UINT8 bmPlanes;
		UINT8 bmBitsPixel;
		UINT32 bmBitsAddr;
		SINT32 bmWidthPlanes;
		UINT32 bmlpPDeviceAddr;
		UINT16 bmSegmentIndex;
		UINT16 bmScanSegment;
		UINT16 bmFillBytes;
		UINT32 reserved;
	} NPDISP_PBITMAP;

	typedef struct {
		SINT16 bmType;
		SINT16 bmWidth;
		SINT16 bmHeight;
		SINT16 bmWidthBytes;
		UINT8 bmPlanes;
		UINT8 bmBitsPixel;
		UINT32 bmBitsAddr;
		SINT32 bmWidthPlanes;
		UINT32 bmlpPDeviceAddr;
		UINT16 bmSegmentIndex;
		UINT16 bmScanSegment;
		UINT16 bmFillBytes;    
		UINT16 reserved1;
		UINT16 reserved2;
		UINT32 ddbmpKey; // np2側のキー 
	} NPDISP_PBITMAP_EXT;

	typedef struct {
		UINT16 deType;
		UINT16 deWidth;
		UINT16 deHeight;
		UINT16 deWidthBytes;
		UINT8 dePlanes;
		UINT8 deBitsPixel;
		UINT32 deReserved1;
		SINT32 deDeltaScan;
		UINT32 delpPDeviceAddr;
		UINT32 deBitsOffset;
		UINT16 deBitsSelector;
		UINT16 deFlags;
		UINT16 deVersion;
		UINT32 deBitmapInfoAddr;
		UINT32 deBeginAccessFuncAddr;
		UINT32 deEndAccessFuncAddr;
		UINT32 deDriverReserved;
	} NPDISP_DIBENGINE;

	typedef struct {
		union {
			NPDISP_PBITMAP bmp;
			NPDISP_DIBENGINE dibe;
		};
	} NPDISP_PDEVICE;

	typedef struct {
		NPDISP_LPEN lpen; // NPDISP_PENの先頭はLPENとする
		int key; // np2側のキー 
	} NPDISP_PEN;
	typedef struct {
		NPDISP_LBRUSH lbrush; // NPDISP_BRUSHの先頭はLBRUSHとする
		int key; // np2側のキー 
	} NPDISP_BRUSH;
	typedef struct {
		NPDISP_LFONT lfont; // NPDISP_FONTの先頭はNPDISP_LFONTとする
		int key; // np2側のキー 
	} NPDISP_FONT;

	typedef struct {
		UINT16 Rop2;
		UINT16 bkMode;
		UINT32 bkColor;    
		UINT32 TextColor;  
		UINT16 TBreakExtra;
		UINT16 BreakExtra; 
		UINT16 BreakErr;   
		UINT16 BreakRem;   
		UINT16 BreakCount; 
		UINT16 CharExtra;  
		UINT32 LbkColor;
		UINT32 LTextColor;
	} NPDISP_DRAWMODE;

	typedef struct {
		UINT16 csHotX;
		UINT16 csHotY;
		UINT16 csWidth;
		UINT16 csHeight;
		UINT16 csWidthBytes;
		UINT16 csColor;
	} NPDISP_CURSORSHAPE;

	typedef struct {
		UINT16 diHdrSize;
		UINT16 diInfoFlags;
		UINT32 diDevNodeHandle;
		UINT8 diDriverName[16];
		UINT16 diXRes;
		UINT16 diYRes;
		UINT16 diDPI;
		UINT8 diPlanes;
		UINT8 diBpp;
		UINT16 diRefreshRateMax;
		UINT16 diRefreshRateMin;
		UINT16 diLowHorz;
		UINT16 diHighHorz;
		UINT16 diLowVert;
		UINT16 diHighVert;
		UINT32 diMonitorDevNodeHandle;
		UINT8 diHorzSyncPolarity;
		UINT8 diVertSyncPolarity;
	} NPDISP_DISPLAYINFO;

	typedef struct {
		UINT16 dvmSize;
		UINT16 dvmBpp;
		SINT16 dvmXRes;
		SINT16 dvmYRes;
	} NPDISP_DISPVALMODE;

	typedef struct
	{
		long version;
		long driverVersion;
		char dllName[262];
	} NPDISP_OPENGL_INFO;


	// ***** DirectDraw Support

	typedef struct {
		UINT32 dwCommand;
		UINT32 dwParam1;
		UINT32 dwParam2;
		UINT32 dwVersion;
		UINT32 dwReserved;
	} NPDISP_DCICMD;

	typedef struct {
		NPDISP_DCICMD  cmd;
		UINT32 dwCompression;
		UINT32 dwMask[3];
		UINT32 dwWidth;
		UINT32 dwHeight;
		UINT32 dwDCICaps;
		UINT32 dwBitCount;
		UINT32 lpSurfaceAddr;
	} NPDISP_DCICREATEINPUT;

	typedef struct {
		UINT32 dwSize;
		UINT32 dwDCICaps;
		UINT32 dwCompression;
		UINT32 dwMask[3];

		UINT32 dwWidth;
		UINT32 dwHeight;
		SINT32 lStride;

		UINT32 dwBitCount;
		UINT32 dwOffSurface;
		UINT16 wSelSurface;
		UINT16 wReserved;

		UINT32 dwReserved1;
		UINT32 dwReserved2;
		UINT32 dwReserved3;

		UINT32 BeginAccessAddr;
		UINT32 EndAccessAddr;
		UINT32 DestroySurfaceAddr;
	} NPDISP_DCISURFACEINFO;

	typedef struct {
		NPDISP_DCICMD cmd;
		RECT rSrc;
		RECT rDst;
		UINT32 EnumCallbackAddr;
		UINT32 lpContextAddr;
	} NPDISP_DCIENUMINPUT;

	typedef struct {
		NPDISP_DCISURFACEINFO  dciInfo;
		UINT32 DrawAddr;
		UINT32 SetClipListAddr;
		UINT32 SetDestinationAddr;
	} NPDISP_DCIOFFSCREEN;

	typedef struct {
		NPDISP_DCISURFACEINFO  dciInfo;
		UINT32   dwChromakeyValue;
		UINT32   dwChromakeyMask;
	} NPDISP_DCIOVERLAY;

	typedef struct {
		UINT32 dwSize;
		UINT32 dwFlags;
		UINT32 dwFourCC;
		union
		{
			UINT32 dwRGBBitCount;
			UINT32 dwYUVBitCount;
			UINT32 dwZBufferBitDepth;
			UINT32 dwAlphaBitDepth;
		};
		union
		{
			UINT32 dwRBitMask;
			UINT32 dwYBitMask;
		};
		union
		{
			UINT32 dwGBitMask;
			UINT32 dwUBitMask;
		};
		union
		{
			UINT32 dwBBitMask;
			UINT32 dwVBitMask;
		};
		union
		{
			UINT32 dwRGBAlphaBitMask;
			UINT32 dwYUVAlphaBitMask;
			UINT32 dwRGBZBitMask;
			UINT32 dwYUVZBitMask;
		};
	} NPDISP_DDPIXELFORMAT;


	typedef struct
	{
		UINT32 fpPrimary;
		UINT32 dwFlags;
		UINT32 dwDisplayWidth;
		UINT32 dwDisplayHeight;
		SINT32 lDisplayPitch;
		NPDISP_DDPIXELFORMAT ddpfDisplay;
		UINT32 dwOffscreenAlign;
		UINT32 dwOverlayAlign;
		UINT32 dwTextureAlign;
		UINT32 dwZBufferAlign;
		UINT32 dwAlphaAlign;
		UINT32 dwNumHeaps;
		UINT32 pvmList;
	} NPDISP_VIDMEMINFO;

	typedef struct {
		UINT32 dwCaps;
	} NPDISP_DDSCAPS;

#define NPDISP_DD_ROP_SPACE	(256 / 32)

	typedef struct
	{
		UINT32 dwWidth;
		UINT32 dwHeight;
		SINT32 lPitch;
		UINT32 dwBPP;
		UINT16 wFlags;
		UINT16 wRefreshRate;
		UINT32 dwRBitMask;
		UINT32 dwGBitMask;
		UINT32 dwBBitMask;
		UINT32 dwAlphaBitMask;
	} NPDISP_DDHALMODEINFO;


	typedef struct
	{
		UINT32 dwSize;
		UINT32 dwCaps;
		UINT32 dwCaps2;
		UINT32 dwCKeyCaps;
		UINT32 dwFXCaps;
		UINT32 dwFXAlphaCaps;
		UINT32 dwPalCaps;
		UINT32 dwSVCaps;
		UINT32 dwAlphaBltConstBitDepths;
		UINT32 dwAlphaBltPixelBitDepths;
		UINT32 dwAlphaBltSurfaceBitDepths;
		UINT32 dwAlphaOverlayConstBitDepths;
		UINT32 dwAlphaOverlayPixelBitDepths;
		UINT32 dwAlphaOverlaySurfaceBitDepths;
		UINT32 dwZBufferBitDepths;
		UINT32 dwVidMemTotal;
		UINT32 dwVidMemFree;
		UINT32 dwMaxVisibleOverlays;
		UINT32 dwCurrVisibleOverlays;
		UINT32 dwNumFourCCCodes;
		UINT32 dwAlignBoundarySrc;
		UINT32 dwAlignSizeSrc;
		UINT32 dwAlignBoundaryDest;
		UINT32 dwAlignSizeDest;
		UINT32 dwAlignStrideAlign;
		UINT32 dwRops[NPDISP_DD_ROP_SPACE];
		NPDISP_DDSCAPS ddsCaps;
		UINT32 dwMinOverlayStretch;
		UINT32 dwMaxOverlayStretch;
		UINT32 dwMinLiveVideoStretch;
		UINT32 dwMaxLiveVideoStretch;
		UINT32 dwMinHwCodecStretch;
		UINT32 dwMaxHwCodecStretch;
		UINT32 dwReserved1;
		UINT32 dwReserved2;
		UINT32 dwReserved3;
		UINT32 dwSVBCaps;
		UINT32 dwSVBCKeyCaps;
		UINT32 dwSVBFXCaps;
		UINT32 dwSVBRops[NPDISP_DD_ROP_SPACE];
		UINT32 dwVSBCaps;
		UINT32 dwVSBCKeyCaps;
		UINT32 dwVSBFXCaps;
		UINT32 dwVSBRops[NPDISP_DD_ROP_SPACE];
		UINT32 dwSSBCaps;
		UINT32 dwSSBCKeyCaps;
		UINT32 dwSSBFXCaps;
		UINT32 dwSSBRops[NPDISP_DD_ROP_SPACE];
		UINT32 dwMaxVideoPorts;
		UINT32 dwCurrVideoPorts;
		UINT32 dwSVBCaps2;
	} NPDISP_DDCORECAPS;

	typedef struct
	{
		UINT32 dwSize;
		UINT32 lpDDCallbacksAddr;
		UINT32 lpDDSurfaceCallbacksAddr;
		UINT32 lpDDPaletteCallbacksAddr;
		NPDISP_VIDMEMINFO vmiData;
		NPDISP_DDCORECAPS ddCaps;
		UINT32 dwMonitorFrequency;
		UINT32 GetDriverInfoAddr;
		UINT32 dwModeIndex;
		UINT32 lpdwFourCC;
		UINT32 dwNumModes;
		UINT32 lpModeInfo;
		UINT32 dwFlags;
		UINT32 lpPDevice;
		UINT32 hInstance;
		UINT32 lpD3DGlobalDriverData;
		UINT32 lpD3DHALCallbacks;
		UINT32 lpDDExeBufCallbacksAddr;
	} NPDISP_DDHALINFO;

#pragma pack(pop)

#pragma pack(push, 1)
	typedef struct {
		UINT16 width;
		UINT32 offset;
	} NPDISP_FONTCHARINFO3;
	typedef struct {
		SINT16 dfType;
		SINT16 dfPoints;
		SINT16 dfVertRes;
		SINT16 dfHorizRes;
		SINT16 dfAscent;
		SINT16 dfInternalLeading;
		SINT16 dfExternalLeading;
		SINT8 dfItalic;
		SINT8 dfUnderline;
		SINT8 dfStrikeOut;
		SINT16 dfWeight;
		SINT8 dfCharSet;
		SINT16 dfPixWidth;
		SINT16 dfPixHeight;
		SINT8 dfPitchAndFamily;
		SINT16 dfAvgWidth;
		SINT16 dfMaxWidth;
		UINT8 dfFirstChar;
		UINT8 dfLastChar;
		UINT8 dfDefaultChar;
		UINT8 dfBreakChar;

		SINT16 dfWidthBytes;
		SINT32 dfDevice;
		SINT32 dfFace;
		UINT32 dfBitsPointer;
		UINT32 dfBitsOffset;
		SINT8 dfReserved;
		/* The following fields present only for Windows 3.x fonts */
		SINT32 dfFlags;
		SINT16 dfAspace;
		SINT16 dfBspace;
		SINT16 dfCspace;
		UINT32 dfColorPointer;
		SINT32 dfReserved1[4];
	} NPDISP_FONTINFO;


	// np2側で控えておく情報

	typedef struct {
		BITMAPINFOHEADER bmiHeader;
		RGBQUAD          bmiColors[2];
	} BITMAPINFO_1BPP;
	typedef struct {
		BITMAPINFOHEADER bmiHeader;
		RGBQUAD          bmiColors[16];
	} BITMAPINFO_4BPP;
	typedef struct {
		BITMAPINFOHEADER bmiHeader;
		RGBQUAD          bmiColors[256];
	} BITMAPINFO_8BPP;
	typedef struct {
		BITMAPINFOHEADER bmiHeader;
		RGBQUAD          bmiColors[3];
	} BITMAPINFO_16BPP;
	typedef struct {
		BITMAPINFOHEADER bmiHeader;
	} BITMAPINFO_24BPP;
	typedef struct {
		BITMAPINFOHEADER bmiHeader;
		RGBQUAD          bmiColors[3];
	} BITMAPINFO_32BPP;

	typedef struct {
		BITMAPINFOHEADER biHeader;
		RGBQUAD pal[256];
		char bmBits[4 * 8 * 8]; // Win3.1は8x8px上限
	} NPDISP_HOSTPATTERNBITMAP;

	typedef struct {
		NPDISP_LBRUSH lbrush;
		NPDISP_HOSTPATTERNBITMAP pattern;
		UINT8 actualColorNum; // 実際の色の数 0=無効（計算が必要）, 1=1色, 2=2色
		UINT32 actualColor; // 実際の色
		UINT32 actualColor2; // ディザの場合の第2色目
		double actualColor2Ratio; // ディザの場合の混合比
		HBRUSH brs; // Windows向け
		UINT32 refCount; // 参照数
	} NPDISP_HOSTBRUSH;

	typedef struct {
		NPDISP_LPEN lpen;
		UINT8 actualColorNum; // 実際の色の数 0=無効（計算が必要）, 1=1色
		UINT32 actualColor; // 実際の色
		HPEN pen; // Windows向け
		UINT32 refCount; // 参照数
	} NPDISP_HOSTPEN;


	// Windows向けコード群

	typedef struct {
		HDC hdc;
		void* pBits;
		HBITMAP hBmp;
		HGDIOBJ hOldBmp;
		UINT32 stride;
		BITMAPINFO* lpbi;

		HBITMAP hBmpDDB;

		UINT8 isDevMemBmp;
	} NPDISP_WINDOWS_BMPHDC;

	typedef struct {
		NPDISP_WINDOWS_BMPHDC bmphdc;
	} NPDISP_HOSTBITMAP;

	typedef struct {
		BITMAPINFO_8BPP bi;
		HDC hdc;
		void* pBits;
		HBITMAP hBmp;
		HGDIOBJ hOldBmp;
		HGDIOBJ hOldPen;
		HGDIOBJ hOldBrush;
		UINT32 stride;
		HFONT hFont;
		HGDIOBJ hOldhFont;

		HDC hdcShadow;
		void* pBitsShadow;
		HBITMAP hBmpShadow;
		HGDIOBJ hOldBmpShadow;
		RECT rectShadow;

		HDC hdcBltBuf;
		void* pBitsBltBuf;
		HBITMAP hBmpBltBuf;
		HGDIOBJ hOldBmpBltBuf;

		//HDC hdc16BltBuf;
		//HBITMAP hBmp16BltBuf;
		//HGDIOBJ hOldBmp16BltBuf;

		HDC hdcCursor;
		HBITMAP hBmpCursor;
		HBITMAP hOldBmpCursor;
		void* pBitsCursor;
		HDC hdcCursorMask;
		HBITMAP hBmpCursorMask;
		HBITMAP hOldBmpCursorMask;
		void* pBitsCursorMask;

		HDC hdcCache[3];

		UINT32 pensIdx;
		std::map<UINT32, NPDISP_HOSTPEN> pens;
		UINT32 brushesIdx;
		std::map<UINT32, NPDISP_HOSTBRUSH> brushes;
		UINT32 bitmapsIdx;
		std::map<UINT32, NPDISP_HOSTBITMAP> bitmaps;

		RECT dirtyRect;
		RECT lastCursorRect;
		bool cursorUpdated;

		RECT dciDirtyRect;

		NPDISP_DRAWMODE lastScreenDrawMode;
	} NPDISP_WINDOWS;

	typedef struct {
		UINT32 funcId;

		std::vector<UINT8> npdisp_memread_buf; // リクエストされてから読み込み完了しているデータを表す
		UINT32 npdisp_memwrite_bufwpos; // リクエストされてから書き込み完了している位置を表す

		UINT32 npdisp_memread_curpos; // リクエストされてからのデータ読み取りバイト数
		UINT32 npdisp_memread_preloadcount; // データプリロードバイト数
		UINT32 npdisp_memwrite_curpos; // リクエストされてからのデータ書き込みバイト数

		UINT32 last_npdisp_memread_bufsize;
		UINT32 last_npdisp_memwrite_bufwpos;
	} NPDISP_MEMCACHE;
#pragma pack(pop)


#ifdef __cplusplus
}
#endif



#endif