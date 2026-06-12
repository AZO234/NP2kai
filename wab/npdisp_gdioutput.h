/**
 * @file	npdisp_gdioutput.h
 * @brief	Interface of the Neko Project II Display Adapter GDI Output Functions
 */

#pragma once

#if defined(SUPPORT_WAB_NPDISP)

#ifdef __cplusplus
extern "C" {
#endif

bool npdisp_func_Output_POLYLINE(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_GetXYRange_POLYLINE(int* xBegin, int* width, int* lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_SCANLINES(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_GetXYRange_SCANLINES(int* xBegin, int* width, int* lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_RECTANGLE(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr, NPDISP_POINT* pt);
bool npdisp_func_Output_GetXYRange_RECTANGLE(int* xBegin, int* width, int* lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr, NPDISP_POINT* pt);
bool npdisp_func_Output_WINDPOLYGON(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_ALTPOLYGON(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_GetXYRange_POLYGON(int* xBegin, int* width, int* lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_ELLIPSE(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr, NPDISP_POINT* pt);
bool npdisp_func_Output_GetXYRange_ELLIPSE(int* xBegin, int* width, int* lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr, NPDISP_POINT* pt);
bool npdisp_func_Output_ARC(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_GetXYRange_ARC(int* xBegin, int* width, int* lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_PIE(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_GetXYRange_PIE(int* xBegin, int* width, int* lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_CHORD(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_GetXYRange_CHORD(int* xBegin, int* width, int* lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_ROUNDRECT(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr, NPDISP_POINT* pt);
bool npdisp_func_Output_GetXYRange_ROUNDRECT(int* xBegin, int* width, int* lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr, NPDISP_POINT* pt);
bool npdisp_func_Output_POLYBEZIER(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_GetXYRange_POLYBEZIER(int* xBegin, int* width, int* lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_WINDPOLYPOLYGON(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_ALTPOLYPOLYGON(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);
bool npdisp_func_Output_GetXYRange_POLYPOLYGON(int* xBegin, int* width, int* lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr);

#ifdef __cplusplus
}
#endif

#endif