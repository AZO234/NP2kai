/**
 * @file	npdisp_rle.h
 * @brief	Interface of the Neko Project II Display Adapter RLE
 */

#if defined(SUPPORT_WAB_NPDISP)

#ifdef __cplusplus
extern "C" {
#endif
	int npdisp_DecompressRLEBMP(const BITMAPINFOHEADER* bih, const UINT8* rle_data, int rle_size, UINT8* out_pixels, UINT8* out_validpixels);
	int npdisp_RLEBMPReadAndCalcSize(const UINT32 lpDIBitsAddr, const UINT32 biCompression, UINT8* rle_data_buf, int rle_size_max);
#ifdef __cplusplus
}
#endif



#endif