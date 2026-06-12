/**
 * @file	npdisp.h
 * @brief	Interface of the Neko Project II Display Adapter
 */

#pragma once

#if defined(SUPPORT_WAB_NPDISP)

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct {
		UINT32	dataAddr;
		UINT32	cmdBuf;

		UINT16	version;

		UINT8	ioenabled;
		UINT8	enabled;
		UINT32	width;
		UINT32	height;
		UINT32	bpp;
		UINT32	dpiX;
		UINT32	dpiY;
		UINT32	usePalette;

		UINT32	updated;
		UINT32	paletteUpdated;
		UINT16	devType;

		SINT32	cursorX;
		SINT32	cursorY;
		SINT32	cursorWidth;
		SINT32	cursorHeight;
		SINT32  cursorHotSpotX;
		SINT32  cursorHotSpotY;
		UINT32  cursorStride;

		int longjmpnum;

		UINT8	active;

		UINT32  cursorBpp; // カーソルbpp (0の場合はモノクロ1bpp扱い)

		UINT8	isWin9x;

		int longjmpnum_nonfast;

		// プロトコルバージョン5以降
		UINT32 mm_vramPhysicalAddr;
		UINT8* mm_screenPtr;
		UINT32 mm_screenSize;
		UINT32 mm_bmpinfoAddr;
		UINT32 mm_beginAccessAddr;
		UINT32 mm_endAccessAddr;
		UINT32 mm_dcibufAddr;
		UINT32 mm_dciBeginAccessAddr;
		UINT32 mm_dciEndAccessAddr;
		UINT32 mm_dciDestroySurfaceAddr;
		UINT32 mm_vramLinearAddr;
		UINT32 mm_dciEnable;
	} NPDISP;

	extern NPDISP		npdisp;

	void npdisp_setDirty(int x1, int y1, int x2, int y2);
	void npdisp_setDirtyAll(void);
	void npdisp_resetDirty(void);

	int npdisp_drawGraphic(void);

	void npdisp_exec(void);

	void npdisp_reset(const NP2CFG* pConfig);
	void npdisp_bind(void);
	void npdisp_unbind(void);
	void npdisp_shutdown(void);

#ifdef __cplusplus
}
#endif



#endif