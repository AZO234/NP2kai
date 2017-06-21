/**
 * @file	sxsibios.h
 * @brief	Interface of SxSI BIOS
 */

#pragma once

enum {
	SXSIBIOS_SASI		= 0,
	SXSIBIOS_IDE		= 1,
	SXSIBIOS_SCSI		= 2
};

#ifdef __cplusplus
extern "C" {
#endif

REG8 sasibios_operate(void);

#if defined(SUPPORT_SCSI)
REG8 scsibios_operate(void);
#endif

#if defined(SUPPORT_IDEIO) || defined(SUPPORT_SASI)
void np2sysp_sasi(const void *arg1, long arg2);
#endif

#if defined(SUPPORT_SCSI)
void np2sysp_scsi(const void *arg1, long arg2);
void np2sysp_scsidev(const void *arg1, long arg2);
#endif

#ifdef __cplusplus
}
#endif
