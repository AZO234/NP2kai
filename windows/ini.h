/**
 *	@file	ini.h
 *	@brief	設定ファイル アクセスの宣言およびインターフェイスの定義をします
 */

#include "profile.h"

/**
 * 追加設定
 */
enum
{
	PFTYPE_ARGS16		= PFTYPE_USER,			/*!< 16ビット配列 */
	PFTYPE_BYTE3,								/*!< 3バイト */
	PFTYPE_KB,									/*!< キーボード設定 */
};

#ifdef __cplusplus
extern "C"
{
#endif

void ini_read(LPCTSTR lpPath, LPCTSTR lpTitle, const PFTBL* lpTable, UINT nCount);
void ini_write(LPCTSTR lpPath, LPCTSTR lpTitle, const PFTBL* lpTable, UINT nCount);

void initgetfile(LPTSTR lpPath, UINT cchPath);
void initload(void);
void initsave(void);

#ifdef __cplusplus
}
#endif
