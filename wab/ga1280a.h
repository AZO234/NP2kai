/**
 * @file	ga1280a.h
 * @brief	Interface of the I-O DATA GA-1280A
 */

#pragma once

#if defined(SUPPORT_WAB_GA1280A)

#define GA1280A_CONVENTIONAL_WINDOW_BASE	0x000c0000UL
#define GA1280A_CONVENTIONAL_WINDOW_END		0x000f0000UL
#define GA1280A_FLAT_APERTURE_BASE			0x00f00000UL
#define GA1280A_FLAT_APERTURE_END			0x00f10000UL

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct {
		UINT8	enabled;
		UINT8	active;
		UINT32	updated;
		UINT32	paletteUpdated;
		
		UINT32	width;
		UINT32	height;

		union {
			UINT8	b[0x20][4];
			UINT16	w[0x20][2];
		} reg;
	} GA1280A;

	extern GA1280A		ga1280a;

	void IOOUTCALL ga1280a_ow(UINT port, UINT16 dat);
	UINT16 IOINPCALL ga1280a_iw(UINT port);

	// PC-98êÍóp MMIOèàóùóp
	int MEMCALL ga1280a_memp_read8(UINT32 address, REG8* lpRetValue);
	int MEMCALL ga1280a_memp_read16(UINT32 address, REG16* lpRetValue);
	int MEMCALL ga1280a_memp_read32(UINT32 address, UINT32* lpRetValue);
	int MEMCALL ga1280a_memp_write8(UINT32 address, REG8 value);
	int MEMCALL ga1280a_memp_write16(UINT32 address, REG16 value);
	int MEMCALL ga1280a_memp_write32(UINT32 address, UINT32 value);

	int MEMCALL ga1280a_memp_translate_address(UINT32 address, UINT32* physical);
	int MEMCALL ga1280a_memp_range_may_hit(UINT32 address, UINT leng);
	int MEMCALL ga1280a_memp_try_read8(UINT32 address, REG8* value);
	int MEMCALL ga1280a_memp_try_read16(UINT32 address, REG16* value);
	int MEMCALL ga1280a_memp_try_read32(UINT32 address, UINT32* value);
	int MEMCALL ga1280a_memp_try_write8(UINT32 address, REG8 value);
	int MEMCALL ga1280a_memp_try_write16(UINT32 address, REG16 value);
	int MEMCALL ga1280a_memp_try_write32(UINT32 address, UINT32 value);

	int ga1280a_drawGraphic(void);

	void ga1280a_reset(const NP2CFG* pConfig);
	void ga1280a_bind(void);
	void ga1280a_unbind(void);
	void ga1280a_shutdown(void);

#ifdef __cplusplus
}
#endif



#endif