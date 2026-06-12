/**
 * @file	npdisp_palette.h
 * @brief	Interface of the Neko Project II Display Adapter Palette
 */

#pragma once

#if defined(SUPPORT_WAB_NPDISP)

// ●Windowsの色は以下のように扱う
// ・論理カラー(理想的なフルカラーRGB値)
// 　↓色近似
// ・物理カラー（デバイスが表現可能なのRGB値）
// CreateBurshの時はディザによって色近似を行う
// 
// ●置き換え可能256色パレットが有効な場合、以下のように扱う
// ・論理カラー or 論理パレット番号
// 論理カラーは色近似によって物理パレット番号へ変換できる
// 論理パレット番号はnpdisp_palette_transTblによって物理パレット番号へ変換できる
// 　↓
// ・物理パレット番号（内部的には常時0～255のグレースケールとする）色取得の際はnpdisp_palette_rgb256を引く必要がある
// 　↓npdisp_palette_rgb256[index]
// ・物理カラー（実際に画面に表示する色）

 // モノクロで白扱いする閾値
#define NPDISP_MONO_THRESHOLD	350
#define NPDISP_MONO_THRESHOLD_N	350

// 色表現　WinAPI RGBQUAD互換
typedef struct {
	UINT8 b;
	UINT8 g;
	UINT8 r;
	UINT8 reserved;
} NPDISP_RGB3;

extern NPDISP_RGB3 npdisp_palette_rgb2[2];
extern NPDISP_RGB3 npdisp_palette_rgb16[16];
extern NPDISP_RGB3 npdisp_palette_rgb256[256];
extern NPDISP_RGB3 npdisp_palette_gray256[256];

extern UINT16 npdisp_palette_transTbl[256];

#ifdef __cplusplus
extern "C" {
#endif
	void npdisp_palette_makeTable();

	int npdisp_FindNearest2(UINT8 r, UINT8 g, UINT8 b);
	int npdisp_FindNearest16(UINT8 r, UINT8 g, UINT8 b);
	int npdisp_FindNearest256(UINT8 r, UINT8 g, UINT8 b, bool fromSystemColor = false);
	int npdisp_FindNearest256Tbl(UINT8 r, UINT8 g, UINT8 b);

	UINT32 npdisp_FindNearestColor(UINT8 r, UINT8 g, UINT8 b);
	UINT32 npdisp_FindNearestColorUINT32(UINT32 color);
	UINT32 npdisp_FindNearestColorIndex(UINT8 r, UINT8 g, UINT8 b);
	UINT32 npdisp_FindNearestColorIndexUINT32(UINT32 color);

	UINT32 npdisp_ObjIdxToColor(int idx);

	/// <summary>
	/// COLORREFをホストGDI描画用に変換
	/// </summary>
	/// <param name="color"></param>
	/// <returns></returns>
	UINT32 npdisp_AdjustColorRefForGDI(UINT32 color, bool* preferDither = NULL);
	/// <summary>
	/// NPDISP_DRAWMODEの前景色と背景色をデバイス描画向けカラーへ変換（256色→カラーパレット番号、他→RGB）
	/// </summary>
	/// <param name="drawMode"></param>
	void npdisp_AdjustDrawModeColor(NPDISP_DRAWMODE *drawMode, bool use24buf = false);
	/// <summary>
	/// DrawModeの値に基づいてソース2値ビットマップの色を修正する
	/// </summary>
	/// <param name="bmpHdcSrc"></param>
	/// <param name="bmpHdcDst"></param>
	/// <param name="drawMode"></param>
	void npdisp_AdjustSrcMonoPaletteByDrawMode(NPDISP_WINDOWS_BMPHDC* bmpHdcSrc, NPDISP_WINDOWS_BMPHDC* bmpHdcDst, NPDISP_DRAWMODE* drawMode);

	void npdisp_palette_clearCache(int indexbegin, int indexEnd);
	void MakePaletteDitherBrushColor(UINT32 target, UINT32* actual1, UINT32* actual2, double* bestTValue);
	HBRUSH CreatePaletteDitherBrush(UINT32 actual1, UINT32 actual2, double bestTValue);
#ifdef __cplusplus
}
#endif

#endif