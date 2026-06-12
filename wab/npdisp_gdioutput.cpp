/**
 * @file	npdisp_gdioutput.c
 * @brief	Implementation of the Neko Project II Display Adapter GDI Output Functions
 */

#include	"compiler.h"

#if defined(SUPPORT_WAB_NPDISP)

#include	<map>
#include	<vector>

#include	"pccore.h"
#include	"cpucore.h"

#include	"npdispdef.h"
#include	"npdisp.h"
#include	"npdisp_mem.h"
#include	"npdisp_gdioutput.h"

#if 0
static void trace_fmt_ex2(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT2(s)	trace_fmt_ex2 s
#else
#define	TRACEOUT2(s)	(void)s
#endif	/* 1 */

bool npdisp_func_Output_POLYLINE(HDC tgtDC, NPDISP_WINDOWS_BMPHDC *bmphdc, NPDISP_PBITMAP_EXT *dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_POLYLINE %d", wCount));
	POINT* gdiPoints = (POINT*)malloc(wCount * sizeof(POINT));
	if (gdiPoints) {
		for (int i = 0; i < wCount; i++) {
			NPDISP_POINT pt;
			if (npdisp_readMemory(&pt, lpPointsAddr, sizeof(NPDISP_POINT))) {
				gdiPoints[i].x = pt.x;
				gdiPoints[i].y = pt.y;
			}
			else {
				break;
			}
			lpPointsAddr += sizeof(NPDISP_POINT);
		}
		Polyline(tgtDC, gdiPoints, wCount);
		free(gdiPoints);
		return true;
	}
	return false;
}
bool npdisp_func_Output_GetXYRange_POLYLINE(int* xBegin, int* width, int*lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_GetXYRange_POLYLINE %d", wCount));
	int penWidthOffset = curPenWidth;
	int minX = SHRT_MAX;
	int maxX = 0;
	int minY = SHRT_MAX;
	int maxY = 0;
	for (int i = 0; i < wCount; i++) {
		NPDISP_POINT pt;
		if (npdisp_readMemory(&pt, lpPointsAddr, sizeof(NPDISP_POINT))) {
			if (pt.x < minX) minX = pt.x;
			if (pt.x > maxX) maxX = pt.x;
			if (pt.y < minY) minY = pt.y;
			if (pt.y > maxY) maxY = pt.y;
		}
		else {
			break;
		}
		lpPointsAddr += sizeof(NPDISP_POINT);
	}
	if (minX <= maxX) {
		*xBegin = minX - penWidthOffset;
		*width = maxX - minX + 1 + penWidthOffset * 2;
	}
	else {
		*xBegin = 0;
		*width = 0;
	}
	if (minY <= maxY) {
		*lineBegin = minY - penWidthOffset;
		*numLines = maxY - minY + 1 + penWidthOffset * 2;
	}
	else {
		*lineBegin = 0;
		*numLines = 0;
	}
	return true;
}
bool npdisp_func_Output_SCANLINES(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_SCANLINES %d", wCount));
	NPDISP_POINT pt;
	if (npdisp_readMemory(&pt, lpPointsAddr, sizeof(NPDISP_POINT))) {
		int beginY = pt.y;
		lpPointsAddr += sizeof(NPDISP_POINT);
		for (int i = 1; i < wCount; i++) {
			if (npdisp_readMemory(&pt, lpPointsAddr, sizeof(NPDISP_POINT))) {
				if (curBrush) {
					RECT rect;
					rect.left = pt.x;
					rect.right = pt.y;
					rect.top = beginY;
					rect.bottom = rect.top + 1;
					FillRect(tgtDC, &rect, curBrush);
				}
				else {
					MoveToEx(tgtDC, pt.x, beginY, NULL);
					LineTo(tgtDC, pt.x, beginY);
				}
			}
			else {
				break;
			}
			lpPointsAddr += sizeof(NPDISP_POINT);
		}
		return true;
	}
	return false;
}
bool npdisp_func_Output_GetXYRange_SCANLINES(int* xBegin, int* width, int*lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_GetXYRange_SCANLINES %d", wCount));
	NPDISP_POINT pt;
	if (npdisp_readMemory(&pt, lpPointsAddr, sizeof(NPDISP_POINT))) {
		*lineBegin = pt.y;
		*numLines = 1;
		return true;
	}
	*lineBegin = 0;
	*numLines = 0;
	return false;
}
bool npdisp_func_Output_RECTANGLE(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr, NPDISP_POINT* pt)
{
	TRACEOUT2(("npdisp_func_Output_RECTANGLE %d", wCount));
	int penWidthOffset = 0;
	//if (curPen) {
	//	LOGPEN lp;
	//	GetObject(curPen, sizeof(LOGPEN), &lp);
	//	penWidthOffset = lp.lopnWidth.x / 2;
	//}
	Rectangle(tgtDC, pt[0].x - penWidthOffset, pt[0].y - penWidthOffset, pt[1].x + penWidthOffset, pt[1].y + penWidthOffset);
	return true;
}
bool npdisp_func_Output_GetXYRange_RECTANGLE(int* xBegin, int* width, int*lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr, NPDISP_POINT* pt)
{
	TRACEOUT2(("npdisp_func_Output_GetXYRange_RECTANGLE %d", wCount));
	int penWidthOffset = 0;
	//if (curPen) {
	//	LOGPEN lp;
	//	GetObject(curPen, sizeof(LOGPEN), &lp);
	//	penWidthOffset = lp.lopnWidth.x / 2;
	//}
	if (npdisp_readMemory(pt, lpPointsAddr, sizeof(NPDISP_POINT) * 2)) {
		if (pt[0].y <= pt[1].y) {
			*lineBegin = pt[0].y;
			*numLines = pt[1].y - pt[0].y;
		}
		else {
			*lineBegin = pt[1].y;
			*numLines = pt[0].y - pt[1].y;
		}
		if (pt[0].x <= pt[1].x) {
			*xBegin = pt[0].x;
			*width = pt[1].x - pt[0].x;
		}
		else {
			*xBegin = pt[1].x;
			*width = pt[0].x - pt[1].x;
		}
		*lineBegin -= penWidthOffset;
		*numLines += penWidthOffset * 2;
		*xBegin -= penWidthOffset;
		*width += penWidthOffset * 2;
		return true;
	}
	*lineBegin = 0;
	*numLines = 0;
	*xBegin = 0;
	*width = 0;
	return false;
}
bool npdisp_func_Output_WINDPOLYGON(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_WINDPOLYGON %d", wCount));
	POINT* gdiPoints = (POINT*)malloc(wCount * sizeof(POINT));
	if (gdiPoints) {
		for (int i = 0; i < wCount; i++) {
			NPDISP_POINT pt;
			if (npdisp_readMemory(&pt, lpPointsAddr, sizeof(NPDISP_POINT))) {
				gdiPoints[i].x = pt.x;
				gdiPoints[i].y = pt.y;
			}
			else {
				break;
			}
			lpPointsAddr += sizeof(NPDISP_POINT);
		}
		SetPolyFillMode(tgtDC, WINDING);
		Polygon(tgtDC, gdiPoints, wCount);
		free(gdiPoints);
		return true;
	}
	return false;
}
bool npdisp_func_Output_ALTPOLYGON(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_ALTPOLYGON %d", wCount));
	POINT* gdiPoints = (POINT*)malloc(wCount * sizeof(POINT));
	if (gdiPoints) {
		for (int i = 0; i < wCount; i++) {
			NPDISP_POINT pt;
			if (npdisp_readMemory(&pt, lpPointsAddr, sizeof(NPDISP_POINT))) {
				gdiPoints[i].x = pt.x;
				gdiPoints[i].y = pt.y;
			}
			else {
				break;
			}
			lpPointsAddr += sizeof(NPDISP_POINT);
		}
		SetPolyFillMode(tgtDC, ALTERNATE);
		Polygon(tgtDC, gdiPoints, wCount);
		free(gdiPoints);
		return true;
	}
	return false;
}
bool npdisp_func_Output_GetXYRange_POLYGON(int* xBegin, int* width, int*lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_GetXYRange_POLYGON %d", wCount));
	int penWidthOffset = curPenWidth;
	int minX = SHRT_MAX;
	int maxX = 0;
	int minY = SHRT_MAX;
	int maxY = 0;
	for (int i = 0; i < wCount; i++) {
		NPDISP_POINT pt;
		if (npdisp_readMemory(&pt, lpPointsAddr, sizeof(NPDISP_POINT))) {
			if (pt.x < minX) minX = pt.x;
			if (pt.x > maxX) maxX = pt.x;
			if (pt.y < minY) minY = pt.y;
			if (pt.y > maxY) maxY = pt.y;
		}
		else {
			break;
		}
		lpPointsAddr += sizeof(NPDISP_POINT);
	}
	if (minX <= maxX) {
		*xBegin = minX - penWidthOffset;
		*width = maxX - minX + 1 + penWidthOffset * 2;
	}
	else {
		*xBegin = 0;
		*width = 0;
	}
	if (minY <= maxY) {
		*lineBegin = minY - penWidthOffset;
		*numLines = maxY - minY + 1 + penWidthOffset * 2;
	}
	else {
		*lineBegin = 0;
		*numLines = 0;
	}
	return true;
}
bool npdisp_func_Output_ELLIPSE(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr, NPDISP_POINT* pt)
{
	TRACEOUT2(("npdisp_func_Output_ELLIPSE %d", wCount));
	int penWidthOffset = 0;
	Ellipse(tgtDC, pt[0].x - penWidthOffset, pt[0].y - penWidthOffset, pt[1].x + penWidthOffset, pt[1].y + penWidthOffset);
	return true;
}
bool npdisp_func_Output_GetXYRange_ELLIPSE(int* xBegin, int* width, int*lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr, NPDISP_POINT* pt)
{
	TRACEOUT2(("npdisp_func_Output_GetXYRange_ELLIPSE %d", wCount));
	int penWidthOffset = curPenWidth;
	if (npdisp_readMemory(pt, lpPointsAddr, sizeof(NPDISP_POINT) * 2)) {
		if (pt[0].y <= pt[1].y) {
			*lineBegin = pt[0].y;
			*numLines = pt[1].y - pt[0].y;
		}
		else {
			*lineBegin = pt[1].y;
			*numLines = pt[0].y - pt[1].y;
		}
		if (pt[0].x <= pt[1].x) {
			*xBegin = pt[0].x;
			*width = pt[1].x - pt[0].x;
		}
		else {
			*xBegin = pt[1].x;
			*width = pt[0].x - pt[1].x;
		}
		*lineBegin -= penWidthOffset;
		*numLines += penWidthOffset * 2;
		*xBegin -= penWidthOffset;
		*width += penWidthOffset * 2;
		return true;
	}
	*lineBegin = 0;
	*numLines = 0;
	*xBegin = 0;
	*width = 0;
	return false;
}
bool npdisp_func_Output_ARC(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_ARC %d", wCount));
	int penWidthOffset = 0;
	NPDISP_POINT pts[4];
	for (int i = 0; i < NELEMENTS(pts); i++) {
		if (!npdisp_readMemory(pts + i, lpPointsAddr, sizeof(NPDISP_POINT))) {
			return false;
		}
		lpPointsAddr += sizeof(NPDISP_POINT);
	}
	Arc(tgtDC, pts[0].x - penWidthOffset, pts[0].y - penWidthOffset, pts[1].x + penWidthOffset + 1, pts[1].y + penWidthOffset + 1, pts[2].x, pts[2].y, pts[3].x, pts[3].y);
	return true;
}
bool npdisp_func_Output_GetXYRange_ARC(int* xBegin, int* width, int*lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_ARC %d", wCount));
	int penWidthOffset = curPenWidth;
	NPDISP_POINT pt1, pt2;
	if (npdisp_readMemory(&pt1, lpPointsAddr, sizeof(NPDISP_POINT))) {
		lpPointsAddr += sizeof(NPDISP_POINT);
		if (npdisp_readMemory(&pt2, lpPointsAddr, sizeof(NPDISP_POINT))) {
			if (pt1.y <= pt2.y) {
				*lineBegin = pt1.y;
				*numLines = pt2.y - pt1.y + 1;
			}
			else {
				*lineBegin = pt2.y;
				*numLines = pt1.y - pt2.y + 1;
			}
			if (pt1.x <= pt2.x) {
				*xBegin = pt1.x;
				*width = pt2.x - pt1.x + 1;
			}
			else {
				*xBegin = pt2.x;
				*width = pt1.x - pt2.x + 1;
			}
			*lineBegin -= penWidthOffset;
			*numLines += penWidthOffset * 2;
			*xBegin -= penWidthOffset;
			*width += penWidthOffset * 2;
			return true;
		}
	}
	*lineBegin = 0;
	*numLines = 0;
	*xBegin = 0;
	*width = 0;
	return false;
}
bool npdisp_func_Output_PIE(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_PIE %d", wCount));
	int penWidthOffset = 0;
	NPDISP_POINT pts[4];
	for (int i = 0; i < NELEMENTS(pts); i++) {
		if (!npdisp_readMemory(pts + i, lpPointsAddr, sizeof(NPDISP_POINT))) {
			return false;
		}
		lpPointsAddr += sizeof(NPDISP_POINT);
	}
	Pie(tgtDC, pts[0].x - penWidthOffset, pts[0].y - penWidthOffset, pts[1].x + penWidthOffset + 1, pts[1].y + penWidthOffset + 1, pts[2].x, pts[2].y, pts[3].x, pts[3].y);
	return true;
}
bool npdisp_func_Output_GetXYRange_PIE(int* xBegin, int* width, int*lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_GetXYRange_PIE %d", wCount));
	int penWidthOffset = curPenWidth;
	NPDISP_POINT pt1, pt2;
	if (npdisp_readMemory(&pt1, lpPointsAddr, sizeof(NPDISP_POINT))) {
		lpPointsAddr += sizeof(NPDISP_POINT);
		if (npdisp_readMemory(&pt2, lpPointsAddr, sizeof(NPDISP_POINT))) {
			if (pt1.y <= pt2.y) {
				*lineBegin = pt1.y;
				*numLines = pt2.y - pt1.y + 1;
			}
			else {
				*lineBegin = pt2.y;
				*numLines = pt1.y - pt2.y + 1;
			}
			if (pt1.x <= pt2.x) {
				*xBegin = pt1.x;
				*width = pt2.x - pt1.x + 1;
			}
			else {
				*xBegin = pt2.x;
				*width = pt1.x - pt2.x + 1;
			}
			*lineBegin -= penWidthOffset;
			*numLines += penWidthOffset * 2;
			*xBegin -= penWidthOffset;
			*width += penWidthOffset * 2;
			return true;
		}
	}
	*lineBegin = 0;
	*numLines = 0;
	*xBegin = 0;
	*width = 0;
	return false;
}
bool npdisp_func_Output_CHORD(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_CHORD %d", wCount));
	int penWidthOffset = 0;
	NPDISP_POINT pts[4];
	for (int i = 0; i < NELEMENTS(pts); i++) {
		if (!npdisp_readMemory(pts + i, lpPointsAddr, sizeof(NPDISP_POINT))) {
			return false;
		}
		lpPointsAddr += sizeof(NPDISP_POINT);
	}
	Chord(tgtDC, pts[0].x - penWidthOffset, pts[0].y - penWidthOffset, pts[1].x + penWidthOffset + 1, pts[1].y + penWidthOffset + 1, pts[2].x, pts[2].y, pts[3].x, pts[3].y);
	return true;
}
bool npdisp_func_Output_GetXYRange_CHORD(int* xBegin, int* width, int*lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_GetXYRange_CHORD %d", wCount));
	int penWidthOffset = curPenWidth;
	NPDISP_POINT pt1, pt2;
	if (npdisp_readMemory(&pt1, lpPointsAddr, sizeof(NPDISP_POINT))) {
		lpPointsAddr += sizeof(NPDISP_POINT);
		if (npdisp_readMemory(&pt2, lpPointsAddr, sizeof(NPDISP_POINT))) {
			if (pt1.y <= pt2.y) {
				*lineBegin = pt1.y;
				*numLines = pt2.y - pt1.y + 1;
			}
			else {
				*lineBegin = pt2.y;
				*numLines = pt1.y - pt2.y + 1;
			}
			if (pt1.x <= pt2.x) {
				*xBegin = pt1.x;
				*width = pt2.x - pt1.x + 1;
			}
			else {
				*xBegin = pt2.x;
				*width = pt1.x - pt2.x + 1;
			}
			*lineBegin -= penWidthOffset;
			*numLines += penWidthOffset * 2;
			*xBegin -= penWidthOffset;
			*width += penWidthOffset * 2;
			return true;
		}
	}
	*lineBegin = 0;
	*numLines = 0;
	*xBegin = 0;
	*width = 0;
	return false;
}
bool npdisp_func_Output_ROUNDRECT(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr, NPDISP_POINT* pt)
{
	TRACEOUT2(("npdisp_func_Output_ROUNDRECT %d", wCount));
	int penWidthOffset = 0;
	RoundRect(tgtDC, pt[0].x - penWidthOffset, pt[0].y - penWidthOffset, pt[1].x + penWidthOffset, pt[1].y + penWidthOffset, pt[2].x - penWidthOffset*2, pt[2].y - penWidthOffset * 2);
	return true;
}
bool npdisp_func_Output_GetXYRange_ROUNDRECT(int* xBegin, int* width, int*lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr, NPDISP_POINT* pt)
{
	TRACEOUT2(("npdisp_func_Output_GetXYRange_ROUNDRECT %d", wCount));
	int penWidthOffset = curPenWidth;
	if (npdisp_readMemory(pt, lpPointsAddr, sizeof(NPDISP_POINT) * 3)) {
		if (pt[0].y <= pt[1].y) {
			*lineBegin = pt[0].y;
			*numLines = pt[1].y - pt[0].y;
		}
		else {
			*lineBegin = pt[1].y;
			*numLines = pt[0].y - pt[1].y;
		}
		if (pt[0].x <= pt[1].x) {
			*xBegin = pt[0].x;
			*width = pt[1].x - pt[0].x;
		}
		else {
			*xBegin = pt[1].x;
			*width = pt[0].x - pt[1].x;
		}
		*lineBegin -= penWidthOffset;
		*numLines += penWidthOffset * 2;
		*xBegin -= penWidthOffset;
		*width += penWidthOffset * 2;
		return true;
	}
	*lineBegin = 0;
	*numLines = 0;
	*xBegin = 0;
	*width = 0;
	return false;
}
bool npdisp_func_Output_POLYBEZIER(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_POLYBEZIER %d", wCount));
	POINT* gdiPoints = (POINT*)malloc(wCount * sizeof(POINT));
	if (gdiPoints) {
		for (int i = 0; i < wCount; i++) {
			NPDISP_POINT pt;
			if (npdisp_readMemory(&pt, lpPointsAddr, sizeof(NPDISP_POINT))) {
				gdiPoints[i].x = pt.x;
				gdiPoints[i].y = pt.y;
			}
			else {
				break;
			}
			lpPointsAddr += sizeof(NPDISP_POINT);
		}
		PolyBezier(tgtDC, gdiPoints, wCount);
		free(gdiPoints);
		return true;
	}
	return false;
}
bool npdisp_func_Output_GetXYRange_POLYBEZIER(int* xBegin, int* width, int*lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_GetXYRange_POLYBEZIER %d", wCount));
	int penWidthOffset = curPenWidth;
	int minX = SHRT_MAX;
	int maxX = 0;
	int minY = SHRT_MAX;
	int maxY = 0;
	for (int i = 0; i < wCount; i++) {
		NPDISP_POINT pt;
		if (npdisp_readMemory(&pt, lpPointsAddr, sizeof(NPDISP_POINT))) {
			if (pt.x < minX) minX = pt.x;
			if (pt.x > maxX) maxX = pt.x;
			if (pt.y < minY) minY = pt.y;
			if (pt.y > maxY) maxY = pt.y;
		}
		else {
			break;
		}
		lpPointsAddr += sizeof(NPDISP_POINT);
	}
	if (minX <= maxX) {
		*xBegin = minX - penWidthOffset;
		*width = maxX - minX + 1 + penWidthOffset * 2;
	}
	else {
		*xBegin = 0;
		*width = 0;
	}
	if (minY <= maxY) {
		*lineBegin = minY - penWidthOffset;
		*numLines = maxY - minY + 1 + penWidthOffset * 2;
	}
	else {
		*lineBegin = 0;
		*numLines = 0;
	}
	return true;
}
bool npdisp_func_Output_WINDPOLYPOLYGON(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_POLYPOLYGON | WIND %d", wCount));
	std::vector<POINT> pointList;
	std::vector<int> pointsList;
	for (int i = 0; i < wCount; i++) {
		UINT16 points = npdisp_readMemory16(lpPointsAddr);
		lpPointsAddr += sizeof(UINT16);
		for (int j = 0; j < points; j++) {
			NPDISP_POINT pt;
			if (npdisp_readMemory(&pt, lpPointsAddr, sizeof(NPDISP_POINT))) {
				POINT gdipt = { pt.x, pt.y };
				pointList.push_back(gdipt);
			}
			else {
				goto errorout;
			}
			lpPointsAddr += sizeof(NPDISP_POINT);
		}
		pointsList.push_back(points);
	}
	SetPolyFillMode(tgtDC, WINDING);
	PolyPolygon(tgtDC, &(pointList[0]), &(pointsList[0]), wCount);
errorout:
	return true;
}
bool npdisp_func_Output_ALTPOLYPOLYGON(HDC tgtDC, NPDISP_WINDOWS_BMPHDC* bmphdc, NPDISP_PBITMAP_EXT* dstPBmp, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_POLYPOLYGON | ALT %d", wCount));
	std::vector<int> pointsList;
	UINT32 polypolyPointCountAddr = npdisp_readMemory32(lpPointsAddr);
	for (int i = 0; i < wCount; i++) {
		const UINT32 points = npdisp_readMemory32(polypolyPointCountAddr);
		pointsList.push_back(points);
		polypolyPointCountAddr += sizeof(UINT32);
	}
	std::vector<POINT> pointList;
	UINT32 polypolyPointsAddr = npdisp_readMemory32(lpPointsAddr + 4);
	for (int i = 0; i < wCount; i++) {
		const int points = pointsList[i];
		for (int j = 0; j < points; j++) {
			NPDISP_POINT pt;
			if (npdisp_readMemory(&pt, polypolyPointsAddr, sizeof(NPDISP_POINT))) {
				POINT gdipt = { pt.x, pt.y };
				pointList.push_back(gdipt);
			}
			else {
				goto errorout;
			}
			polypolyPointsAddr += sizeof(NPDISP_POINT);
		}
	}
	SetPolyFillMode(tgtDC, ALTERNATE);
	PolyPolygon(tgtDC, &(pointList[0]), &(pointsList[0]), wCount);
errorout:
	return true;
}
bool npdisp_func_Output_GetXYRange_POLYPOLYGON(int* xBegin, int* width, int*lineBegin, int* numLines, int curPenWidth, HBRUSH curBrush, UINT16 wCount, UINT32 lpPointsAddr)
{
	TRACEOUT2(("npdisp_func_Output_GetXYRange_POLYPOLYGON %d", wCount));
	int penWidthOffset = curPenWidth;
	int minX = SHRT_MAX;
	int maxX = 0;
	int minY = SHRT_MAX;
	int maxY = 0;
	int pointsSum = 0;
	UINT32 polypolyPointCountAddr = npdisp_readMemory32(lpPointsAddr);
	for (int i = 0; i < wCount; i++) {
		UINT32 points = npdisp_readMemory32(polypolyPointCountAddr);
		polypolyPointCountAddr += sizeof(UINT32);
		pointsSum += points;
	}
	UINT32 polypolyPointsAddr = npdisp_readMemory32(lpPointsAddr + 4);
	for (int j = 0; j < pointsSum; j++) {
		NPDISP_POINT pt;
		if (npdisp_readMemory(&pt, lpPointsAddr, sizeof(NPDISP_POINT))) {
			if (pt.x < minX) minX = pt.x;
			if (pt.x > maxX) maxX = pt.x;
			if (pt.y < minY) minY = pt.y;
			if (pt.y > maxY) maxY = pt.y;
		}
		else {
			goto errorout;
		}
		lpPointsAddr += sizeof(NPDISP_POINT);
	}
errorout:
	if (minX <= maxX) {
		*xBegin = minX - penWidthOffset;
		*width = maxX - minX + 1 + penWidthOffset * 2;
	}
	else {
		*xBegin = 0;
		*width = 0;
	}
	if (minY <= maxY) {
		*lineBegin = minY - penWidthOffset;
		*numLines = maxY - minY + 1 + penWidthOffset * 2;
	}
	else {
		*lineBegin = 0;
		*numLines = 0;
	}
	return true;
}

#endif