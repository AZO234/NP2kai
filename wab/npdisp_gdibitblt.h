/**
 * @file	npdisp_gdioutput.h
 * @brief	Interface of the Neko Project II Display Adapter GDI BitBlt Functions
 */

#pragma once

#if defined(SUPPORT_WAB_NPDISP)

#ifdef __cplusplus
extern "C" {
#endif

UINT16 npdisp_func_StretchBlt_VRAMtoVRAM(int hasDstDev, int hasSrcDev, UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, SINT16 wDestXext, SINT16 wDestYext, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, SINT16 wSrcXext, SINT16 wSrcYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr, UINT32 lpClipAddr);
UINT16 npdisp_func_StretchBlt_MEMtoVRAM(int hasDstDev, int hasSrcDev, UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, SINT16 wDestXext, SINT16 wDestYext, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, SINT16 wSrcXext, SINT16 wSrcYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr, UINT32 lpClipAddr);
UINT16 npdisp_func_StretchBlt_VRAMtoMEM(int hasDstDev, int hasSrcDev, UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, SINT16 wDestXext, SINT16 wDestYext, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, SINT16 wSrcXext, SINT16 wSrcYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr, UINT32 lpClipAddr);
UINT16 npdisp_func_StretchBlt_MEMtoMEM(int hasDstDev, int hasSrcDev, UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, SINT16 wDestXext, SINT16 wDestYext, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, SINT16 wSrcXext, SINT16 wSrcYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr, UINT32 lpClipAddr);


UINT16 npdisp_func_BitBlt_VRAMtoVRAM(int hasDstDev, int hasSrcDev, UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, UINT16 wXext, UINT16 wYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr);
UINT16 npdisp_func_BitBlt_MEMtoVRAM(int hasDstDev, int hasSrcDev, UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, UINT16 wXext, UINT16 wYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr);
UINT16 npdisp_func_BitBlt_VRAMtoMEM(int hasDstDev, int hasSrcDev, UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, UINT16 wXext, UINT16 wYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr);
UINT16 npdisp_func_BitBlt_MEMtoMEM(int hasDstDev, int hasSrcDev, UINT32 lpDestDevAddr, SINT16 wDestX, SINT16 wDestY, UINT32 lpSrcDevAddr, SINT16 wSrcX, SINT16 wSrcY, UINT16 wXext, UINT16 wYext, UINT32 Rop3, UINT32 lpPBrushAddr, UINT32 lpDrawModeAddr);

#ifdef __cplusplus
}
#endif

#endif