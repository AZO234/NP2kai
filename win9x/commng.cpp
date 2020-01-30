/**
 * @file	commng.cpp
 * @brief	COM マネージャの動作の定義を行います
 */

#include "compiler.h"
#include "commng.h"
#include "np2.h"
#include "commng/cmmidi.h"
#include "commng/cmnull.h"
#include "commng/cmpara.h"
#include "commng/cmserial.h"
#if defined(SUPPORT_WACOM_TABLET)
#include "commng/cmwacom.h"
#endif
#if defined(SUPPORT_NAMED_PIPE)
#include "commng/cmpipe.h"
#endif
#include "generic/cmjasts.h"

/**
 * 初期化
 */
void commng_initialize(void)
{
	cmmidi_initailize();
#if defined(SUPPORT_WACOM_TABLET)
	cmwacom_initialize();
	cmwacom_setNCControl(!!np2oscfg.mouse_nc);
#endif
}
void commng_finalize(void)
{
#if defined(SUPPORT_WACOM_TABLET)
	cmwacom_finalize();
#endif
}

/**
 * 作成
 * @param[in] nDevice デバイス
 * @return ハンドル
 */
COMMNG commng_create(UINT nDevice)
{
	COMMNG ret = NULL;

	COMCFG* pComCfg = NULL;
	switch (nDevice)
	{
		case COMCREATE_SERIAL:
			pComCfg = &np2oscfg.com1;
			break;

		case COMCREATE_PC9861K1:
			pComCfg = &np2oscfg.com2;
			break;

		case COMCREATE_PC9861K2:
			pComCfg = &np2oscfg.com3;
			break;

		case COMCREATE_PRINTER:
			if (np2oscfg.jastsnd)
			{
				ret = cmjasts_create();
			}
			break;

		case COMCREATE_MPU98II:
			pComCfg = &np2oscfg.mpu;
			break;
			
#if defined(SUPPORT_SMPU98)
		case COMCREATE_SMPU98_A:
			pComCfg = &np2oscfg.smpuA;
			break;

		case COMCREATE_SMPU98_B:
			pComCfg = &np2oscfg.smpuB;
			break;
#endif

		default:
			break;
	}

	if (pComCfg)
	{
		if ((pComCfg->port >= COMPORT_COM1) && (pComCfg->port <= COMPORT_COM4))
		{
			ret = CComSerial::CreateInstance(pComCfg->port - COMPORT_COM1 + 1, pComCfg->param, pComCfg->speed, pComCfg->fixedspeed);
		}
		else if (pComCfg->port == COMPORT_MIDI)
		{
			ret = CComMidi::CreateInstance(pComCfg->mout, pComCfg->min, pComCfg->mdl);
			if (ret)
			{
				ret->msg(ret, COMMSG_MIMPIDEFFILE, (INTPTR)pComCfg->def);
				ret->msg(ret, COMMSG_MIMPIDEFEN, (INTPTR)pComCfg->def_en);
			}
		}
#if defined(SUPPORT_WACOM_TABLET)
		else if (pComCfg->port == COMPORT_TABLET)
		{
			ret = CComWacom::CreateInstance(g_hWndMain);
		}
#endif
#if defined(SUPPORT_NAMED_PIPE)
		else if (pComCfg->port == COMPORT_PIPE)
		{
			ret = CComPipe::CreateInstance(pComCfg->pipename, pComCfg->pipeserv);
		}
#endif
	}

	if (ret == NULL)
	{
		ret = new CComNull;
	}
	return ret;
}

/**
 * 破棄
 * @param[in] hdl ハンドル
 */
void commng_destroy(COMMNG hdl)
{
	if (hdl)
	{
		hdl->release(hdl);
	}
}
