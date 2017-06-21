/**
 * @file	viewreg.cpp
 * @brief	レジスタ表示クラスの動作の定義を行います
 */

#include "compiler.h"
#include "resource.h"
#include "np2.h"
#include "debugsub.h"
#include "viewer.h"
#include "viewreg.h"
#include "cpucore.h"

/**
 * コンストラクタ
 * @param[in] lpView ビューワ インスタンス
 */
CDebugUtyReg::CDebugUtyReg(CDebugUtyView* lpView)
	: CDebugUtyItem(lpView, IDM_VIEWMODEREG)
{
}

/**
 * デストラクタ
 */
CDebugUtyReg::~CDebugUtyReg()
{
}

/**
 * 初期化
 * @param[in] lpItem 基準となるアイテム
 */
void CDebugUtyReg::Initialize(const CDebugUtyItem* lpItem)
{
	m_lpView->SetVScroll(0, 4);
}

/**
 * 更新
 * @retval true 更新あり
 * @retval false 更新なし
 */
bool CDebugUtyReg::Update()
{
	return (m_buffer.empty());
}

/**
 * ロック
 * @retval true 成功
 * @retval false 失敗
 */
bool CDebugUtyReg::Lock()
{
#if defined(CPUCORE_IA32)
	m_buffer.resize(sizeof(i386core.s));
	CopyMemory(&m_buffer.at(0), &i386core.s, sizeof(i386core.s));
#elif defined(CPUCORE_V30)
	m_buffer.resize(sizeof(v30core.s));
	CopyMemory(&m_buffer.at(0), &v30core.s, sizeof(v30core.s));
#else
	m_buffer.resize(sizeof(i286core.s));
	CopyMemory(&m_buffer.at(0), &i286core.s, sizeof(i286core.s));
#endif
	return true;
}

/**
 * アンロック
 */
void CDebugUtyReg::Unlock()
{
	m_buffer.clear();
}

/**
 * ロック中?
 * @retval true ロック中である
 * @retval false ロック中でない
 */
bool CDebugUtyReg::IsLocked()
{
	return (!m_buffer.empty());
}

/**
 * 描画
 * @param[in] hDC デバイス コンテキスト
 * @param[in] rect 領域
 */
void CDebugUtyReg::OnPaint(HDC hDC, const RECT& rect)
{
	UINT nIndex = m_lpView->GetVScrollPos();

#if defined(CPUCORE_IA32)
	const I386STAT* r = &i386core.s;
	if (!m_buffer.empty())
	{
		r = reinterpret_cast<I386STAT*>(&m_buffer.at(0));
	}

	for (int y = 0; y < rect.bottom && nIndex < 4; y += 16, nIndex++)
	{
		TCHAR szTmp[128];
		switch (nIndex)
		{
			case 0:
				::wsprintf(szTmp, TEXT("EAX=%.8x EBX=%.8x ECX=%.8x EDX=%.8x"),
								r->cpu_regs.reg[CPU_EAX_INDEX].d,
								r->cpu_regs.reg[CPU_EBX_INDEX].d,
								r->cpu_regs.reg[CPU_ECX_INDEX].d,
								r->cpu_regs.reg[CPU_EDX_INDEX].d);
				break;

			case 1:
				::wsprintf(szTmp, TEXT("ESP=%.8x EBP=%.8x ESI=%.8x EDI=%.8x"),
								r->cpu_regs.reg[CPU_ESP_INDEX].d,
								r->cpu_regs.reg[CPU_EBP_INDEX].d,
								r->cpu_regs.reg[CPU_ESI_INDEX].d,
								r->cpu_regs.reg[CPU_EDI_INDEX].d);
				break;

			case 2:
				::wsprintf(szTmp, TEXT("CS=%.4x DS=%.4x ES=%.4x FS=%.4x GS=%.4x SS=%.4x"),
								r->cpu_regs.sreg[CPU_CS_INDEX],
								r->cpu_regs.sreg[CPU_DS_INDEX],
								r->cpu_regs.sreg[CPU_ES_INDEX],
								r->cpu_regs.sreg[CPU_FS_INDEX],
								r->cpu_regs.sreg[CPU_GS_INDEX],
								r->cpu_regs.sreg[CPU_SS_INDEX]);
				break;

			case 3:
				::wsprintf(szTmp, TEXT("EIP=%.8x   %s"),
								r->cpu_regs.eip.d,
								debugsub_flags(r->cpu_regs.eflags.d));
				break;
		}
		::TextOut(hDC, 0, y, szTmp, ::lstrlen(szTmp));
	}
#elif defined(CPUCORE_V30)
	const V30STAT* r = &v30core.s;
	if (!m_buffer.empty())
	{
		r = reinterpret_cast<V30STAT*>(&m_buffer.at(0));
	}

	for (int y = 0; (y < rect.bottom) && (nIndex < 4); y += 16, nIndex++)
	{
		TCHAR szTmp[128];
		switch (nIndex)
		{
			case 0:
				::wsprintf(szTmp, TEXT("AW=%.4x  BW=%.4x  CW=%.4x  DW=%.4x"), r->r.w.aw, r->r.w.bw, r->r.w.cw, r->r.w.dw);
				break;

			case 1:
				::wsprintf(szTmp, TEXT("SP=%.4x  BP=%.4x  IX=%.4x  IY=%.4x"), r->r.w.sp, r->r.w.bp, r->r.w.ix, r->r.w.iy);
				break;

			case 2:
				::wsprintf(szTmp, TEXT("PS=%.4x  DS0=%.4x  ES1=%.4x  SS=%.4x"), r->r.w.ps, r->r.w.ds0, r->r.w.ds1, r->r.w.ss);
				break;

			case 3:
				::wsprintf(szTmp, TEXT("PC=%.4x   %s"), r->r.w.pc, debugsub_flags(r->r.w.psw));
				break;
		}
		::TextOut(hDC, 0, y, szTmp, ::lstrlen(szTmp));
	}
#else
	const I286STAT* r = &i286core.s;
	if (!m_buffer.empty())
	{
		r = reinterpret_cast<I286STAT*>(&m_buffer.at(0));
	}

	for (int y = 0; y < rect.bottom && nIndex < 4; y += 16, nIndex++)
	{
		TCHAR szTmp[128];
		switch (nIndex)
		{
			case 0:
				::wsprintf(szTmp, TEXT("AX=%.4x  BX=%.4x  CX=%.4x  DX=%.4x"), r->r.w.ax, r->r.w.bx, r->r.w.cx, r->r.w.dx);
				break;

			case 1:
				::wsprintf(szTmp, TEXT("SP=%.4x  BP=%.4x  SI=%.4x  DI=%.4x"), r->r.w.sp, r->r.w.bp, r->r.w.si, r->r.w.di);
				break;

			case 2:
				::wsprintf(szTmp, TEXT("CS=%.4x  DS=%.4x  ES=%.4x  SS=%.4x"), r->r.w.cs, r->r.w.ds, r->r.w.es, r->r.w.ss);
				break;

			case 3:
				::wsprintf(szTmp, TEXT("IP=%.4x   %s"), r->r.w.ip, debugsub_flags(r->r.w.flag));
				break;
		}
		::TextOut(hDC, 0, y, szTmp, ::lstrlen(szTmp));
	}
#endif
}
