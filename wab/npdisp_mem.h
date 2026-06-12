/**
 * @file	npdisp_mem.h
 * @brief	Interface of the Neko Project II Display Adapter Memory Helper
 */

#pragma once

#if defined(SUPPORT_WAB_NPDISP)

#ifdef __cplusplus
extern "C" {
#endif
	void npdisp_memory_setFunctionId(int funcId);
	void npdisp_memory_clearallpreload();
	void npdisp_memory_clearpreload();
	void npdisp_memory_resetposition();
	int npdisp_memory_hasNewCacheData();
	int npdisp_memory_getTotalReadSize();
	int npdisp_memory_getTotalWriteSize();
	UINT32 npdisp_memory_getLastEIP();

	int npdisp_preloadAndReadMemoryWith32Offset(void* dst, UINT16 selector, UINT32 offset, int size);
	int npdisp_preloadAndReadMemory(void* dst, UINT32 lpAddr, int size);
	int npdisp_preloadMemoryWith32Offset(UINT16 selector, UINT32 offset, int size);
	int npdisp_preloadMemory(UINT32 lpAddr, int size);
	int npdisp_readMemoryWith32Offset(void* dst, UINT16 selector, UINT32 offset, int size);
	int npdisp_readMemory(void* dst, UINT32 lpAddr, int size);
	int npdisp_writeMemoryWith32Offset(void* src, UINT16 selector, UINT32 offset, int size);
	int npdisp_writeMemory(void* src, UINT32 lpAddr, int size);

	UINT8 npdisp_readMemory8With32Offset(UINT16 selector, UINT32 offset);
	UINT8 npdisp_readMemory8(UINT32 lpAddr);
	UINT16 npdisp_readMemory16(UINT32 lpAddr);
	UINT32 npdisp_readMemory32(UINT32 lpAddr);
	int npdisp_writeMemory8(UINT8 value, UINT32 lpAddr);
	int npdisp_writeMemory16(UINT16 value, UINT32 lpAddr);
	int npdisp_writeMemory32(UINT32 value, UINT32 lpAddr);
	void npdisp_writeReturnCode(NPDISP_REQUEST *lpReq, UINT32 dataAddr, UINT16 retCode);
	char* npdisp_readMemoryString(UINT32 lpAddr);
	char* npdisp_readMemoryStringWithCount(UINT32 lpAddr, int count);

	bool npdisp_isDisplayDevice(UINT32 lpAddr);
	UINT32 npdisp_readPBitmap(NPDISP_PBITMAP_EXT* bmp, UINT32 lpAddr, bool useSelected = true);
	UINT32 npdisp_writePBitmap(NPDISP_PBITMAP_EXT* bmp, UINT32 lpAddr);
	void npdisp_PreloadBitmapFromPBITMAP(NPDISP_PBITMAP_EXT* srcPBmp, int dcIdx, int beginLine = 0, int numLines = -1, int beginX = 0, int copyWidth = -1);
	int npdisp_MakeBitmapFromPBITMAP(NPDISP_PBITMAP_EXT* srcPBmp, NPDISP_WINDOWS_BMPHDC* bmpHDC, int dcIdx, int beginLine = 0, int numLines = -1, int beginX = 0, int copyWidth = -1, UINT16 *transTable = NULL);
	void npdisp_WriteBitmapToPBITMAP(NPDISP_PBITMAP_EXT* dstPBmp, NPDISP_WINDOWS_BMPHDC* bmpHDC, int beginLine = 0, int numLines = -1, int beginX = 0, int copyWidth = -1);
	void npdisp_ConvertToDDBMonoBitmap(NPDISP_WINDOWS_BMPHDC* bmpHDC);
	void npdisp_FreeBitmap(NPDISP_WINDOWS_BMPHDC* bmpHDC, bool force = false);
#ifdef __cplusplus
}
#endif

#endif