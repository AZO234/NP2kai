/**
 * @file	npdisp_statsave.h
 * @brief	Interface of the Neko Project II Display Adapter Statsave
 */

#pragma once

#if defined(SUPPORT_WAB_NPDISP)

#ifdef __cplusplus
extern "C" {
#endif

	int npdisp_sfsave(STFLAGH sfh, const SFENTRY* tbl);
	int npdisp_sfload(STFLAGH sfh, const SFENTRY* tbl);

#ifdef __cplusplus
}
#endif

#endif