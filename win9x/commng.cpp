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
#include "generic/cmjasts.h"

/**
 * 初期化
 */
void commng_initialize(void)
{
	cmmidi_initailize();
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

		default:
			break;
	}

	if (pComCfg)
	{
		if ((pComCfg->port >= COMPORT_COM1) && (pComCfg->port <= COMPORT_COM4))
		{
			ret = CComSerial::CreateInstance(pComCfg->port - COMPORT_COM1 + 1, pComCfg->param, pComCfg->speed);
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
