/**
 * @file	d_config.cpp
 * @brief	設定ダイアログ
 */

#include "compiler.h"
#include "resource.h"
#include "dialog.h"
#include "c_combodata.h"
#include "np2.h"
#include "soundmng.h"
#include "sysmng.h"
#include "misc/DlgProc.h"
#if defined(SUPPORT_ASIO)
#include "soundmng/sdasio.h"
#endif	// defined(SUPPORT_ASIO)
#include "soundmng/sddsound3.h"
#if defined(SUPPORT_WASAPI)
#include "soundmng\sdwasapi.h"
#endif	// defined(SUPPORT_WASAPI)
#include "pccore.h"
#include "common/strres.h"

/**
 * @brief 設定ダイアログ
 * @param[in] hwndParent 親ウィンドウ
 */
class CConfigureDlg : public CDlgProc
{
public:
	CConfigureDlg(HWND hwndParent);

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

private:
	CComboData m_baseClock;			//!< ベース クロック
	CComboData m_multiple;			//!< 倍率
	CComboData m_type;				//!< タイプ
	CComboData m_name;				//!< デバイス名
	CComboData m_rate;				//!< レート
	std::vector<LPCTSTR> m_dsound3;	//!< DSound3
	std::vector<LPCTSTR> m_wasapi;	//!< WASAPI
	std::vector<LPCTSTR> m_asio;	//!< ASIO
	void SetClock(UINT nMultiple = 0);
	void UpdateDeviceList();
};

//! コンボ ボックス アイテム
static const CComboData::Entry s_baseclock[] =
{
	{MAKEINTRESOURCE(IDS_2_0MHZ),	PCBASECLOCK20},
	{MAKEINTRESOURCE(IDS_2_5MHZ),	PCBASECLOCK25},
};

//! 倍率リスト
static const UINT32 s_mulval[] = {1, 2, 4, 5, 6, 8, 10, 12, 16, 20, 24, 30, 36, 40, 42};

//! クロック フォーマット
static const TCHAR str_clockfmt[] = _T("%2u.%.4u");

//! サンプリング レート
static const UINT32 s_nSamplingRate[] = {11025, 22050, 44100, 48000, 88200, 96000};

/**
 * コンストラクタ
 * @param[in] hwndParent 親ウィンドウ
 */
CConfigureDlg::CConfigureDlg(HWND hwndParent)
	: CDlgProc(IDD_CONFIG, hwndParent)
{
}

/**
 * このメソッドは WM_INITDIALOG のメッセージに応答して呼び出されます
 * @retval TRUE 最初のコントロールに入力フォーカスを設定
 * @retval FALSE 既に設定済
 */
BOOL CConfigureDlg::OnInitDialog()
{
	m_baseClock.SubclassDlgItem(IDC_BASECLOCK, this);
	m_baseClock.Add(s_baseclock, _countof(s_baseclock));
	const UINT32 nBaseClock = (np2cfg.baseclock == PCBASECLOCK20) ? PCBASECLOCK20 : PCBASECLOCK25;
	m_baseClock.SetCurItemData(nBaseClock);

	m_multiple.SubclassDlgItem(IDC_MULTIPLE, this);
	m_multiple.Add(s_mulval, _countof(s_mulval));
	SetDlgItemInt(IDC_MULTIPLE, np2cfg.multiple, FALSE);

	UINT nModel;
	if (!milstr_cmp(np2cfg.model, str_VM))
	{
		nModel = IDC_MODELVM;
	}
	else if (!milstr_cmp(np2cfg.model, str_EPSON))
	{
		nModel = IDC_MODELEPSON;
	}
	else
	{
		nModel = IDC_MODELVX;
	}
	CheckDlgButton(nModel, BST_CHECKED);

	// サウンド関係
	m_type.SubclassDlgItem(IDC_SOUND_DEVICE_TYPE, this);

	CSoundDeviceDSound3::EnumerateDevices(m_dsound3);
#if defined(SUPPORT_WASAPI)
	CSoundDeviceWasapi::EnumerateDevices(m_wasapi);
#endif	// defined(SUPPORT_WASAPI)
#if defined(SUPPORT_ASIO)
	CSoundDeviceAsio::EnumerateDevices(m_asio);
#endif	// defined(SUPPORT_ASIO)

	const CSoundMng::DeviceType nType = static_cast<CSoundMng::DeviceType>(np2oscfg.cSoundDeviceType);
	if (np2oscfg.szSoundDeviceName[0] != '\0')
	{
		std::vector<LPCTSTR>* pDevices = NULL;
		switch (nType)
		{
			case CSoundMng::kDSound3:
				pDevices = &m_dsound3;
				break;

			case CSoundMng::kWasapi:
				pDevices = &m_wasapi;
				break;

			case CSoundMng::kAsio:
				pDevices = &m_asio;
				break;
		}
		if (pDevices)
		{
			std::vector<LPCTSTR>::iterator it = pDevices->begin();
			while ((it != pDevices->end()) && (::lstrcmpi(np2oscfg.szSoundDeviceName, *it) != 0))
			{
				++it;
			}
			if (it == pDevices->end())
			{
				pDevices->push_back(np2oscfg.szSoundDeviceName);
			}
		}
	}
	m_type.Add(TEXT("Direct Sound"), CSoundMng::kDSound3);
	if ((nType == CSoundMng::kWasapi) || (!m_wasapi.empty()))
	{
		m_type.Add(TEXT("WASAPI"), CSoundMng::kWasapi);
	}
	if ((nType == CSoundMng::kAsio) || (!m_asio.empty()))
	{
		m_type.Add(TEXT("ASIO"), CSoundMng::kAsio);
	}
	if (!m_type.SetCurItemData(nType))
	{
		int nIndex = m_type.Add(TEXT("Unknown"), CSoundMng::kDefault);
		m_type.SetCurSel(nIndex);
	}

	m_name.SubclassDlgItem(IDC_SOUND_DEVICE_NAME, this);
	UpdateDeviceList();

	m_rate.SubclassDlgItem(IDC_SOUND_RATE, this);
	m_rate.Add(s_nSamplingRate, _countof(s_nSamplingRate));
	int nIndex = m_rate.FindItemData(np2cfg.samplingrate);
	if (nIndex == CB_ERR)
	{
		nIndex = m_rate.Add(np2cfg.samplingrate);
	}
	m_rate.SetCurSel(nIndex);

	SetDlgItemInt(IDC_SOUND_BUFFER, np2cfg.delayms, FALSE);

	CheckDlgButton(IDC_ALLOWRESIZE, (np2oscfg.thickframe) ? BST_CHECKED : BST_UNCHECKED);

#if !defined(_WIN64)
	if (mmxflag & MMXFLAG_NOTSUPPORT)
	{
		GetDlgItem(IDC_DISABLEMMX).EnableWindow(FALSE);
		CheckDlgButton(IDC_DISABLEMMX, BST_CHECKED);
	}
	else
	{
		CheckDlgButton(IDC_DISABLEMMX, (np2oscfg.disablemmx) ? BST_CHECKED : BST_UNCHECKED);
	}
#else	// !defined(_WIN64)
	GetDlgItem(IDC_DISABLEMMX).EnableWindow(FALSE);
#endif	// !defined(_WIN64)

	CheckDlgButton(IDC_COMFIRM, (np2oscfg.comfirm) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_RESUME, (np2oscfg.resume) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_SAVEWINDOWSIZE, (np2oscfg.svscrmul) ? BST_CHECKED : BST_UNCHECKED);
	SetClock();
	m_baseClock.SetFocus();

	return FALSE;
}

/**
 * リスト更新
 */
void CConfigureDlg::UpdateDeviceList()
{
	const CSoundMng::DeviceType nType = static_cast<CSoundMng::DeviceType>(m_type.GetCurItemData(np2oscfg.cSoundDeviceType));

	m_name.ResetContent();
	if (nType != CSoundMng::kAsio)
	{
		m_name.Add(TEXT("Default"), FALSE);
	}

	std::vector<LPCTSTR>* pDevices = NULL;
	switch (nType)
	{
		case CSoundMng::kDSound3:
			pDevices = &m_dsound3;
			break;

		case CSoundMng::kWasapi:
			pDevices = &m_wasapi;
			break;

		case CSoundMng::kAsio:
			pDevices = &m_asio;
			break;
	}
	if (pDevices)
	{
		for (std::vector<LPCTSTR>::const_iterator it = pDevices->begin(); it != pDevices->end(); ++it)
		{
			m_name.Add(*it, TRUE);
		}
	}

	int nIndex = m_name.FindStringExact(-1, np2oscfg.szSoundDeviceName);
	if (nIndex == CB_ERR)
	{
		nIndex = 0;
	}
	m_name.SetCurSel(nIndex);
}

/**
 * ユーザーが OK のボタン (IDOK ID がのボタン) をクリックすると呼び出されます
 */
void CConfigureDlg::OnOK()
{
	UINT nUpdated = 0;

	const UINT nBaseClock = m_baseClock.GetCurItemData(PCBASECLOCK20);
	if (np2cfg.baseclock != nBaseClock)
	{
		np2cfg.baseclock = nBaseClock;
		nUpdated |= SYS_UPDATECFG | SYS_UPDATECLOCK;
	}

	UINT nMultiple = GetDlgItemInt(IDC_MULTIPLE, NULL, FALSE);
	nMultiple = max(nMultiple, 1);
	nMultiple = min(nMultiple, 256);
	if (np2cfg.multiple != nMultiple)
	{
		np2cfg.multiple = nMultiple;
		nUpdated |= SYS_UPDATECFG | SYS_UPDATECLOCK;
	}

	LPCTSTR str;
	if (IsDlgButtonChecked(IDC_MODELVM) != BST_UNCHECKED)
	{
		str = str_VM;
	}
	else if (IsDlgButtonChecked(IDC_MODELEPSON) != BST_UNCHECKED)
	{
		str = str_EPSON;
	}
	else {
		str = str_VX;
	}
	if (milstr_cmp(np2cfg.model, str))
	{
		milstr_ncpy(np2cfg.model, str, NELEMENTS(np2cfg.model));
		nUpdated |= SYS_UPDATECFG;
	}

	const CSoundMng::DeviceType nOldType = static_cast<CSoundMng::DeviceType>(np2oscfg.cSoundDeviceType);
	const CSoundMng::DeviceType nType = static_cast<CSoundMng::DeviceType>(m_type.GetCurItemData(nOldType));
	TCHAR szName[MAX_PATH];
	ZeroMemory(szName, sizeof(szName));
	if (m_name.GetCurItemData(FALSE))
	{
		m_name.GetWindowText(szName, _countof(szName));
	}
	if ((nType != nOldType) || (::lstrcmpi(szName, np2oscfg.szSoundDeviceName) != 0))
	{
		np2oscfg.cSoundDeviceType = static_cast<UINT8>(nType);
		::lstrcpyn(np2oscfg.szSoundDeviceName, szName, _countof(np2oscfg.szSoundDeviceName));
		nUpdated |= SYS_UPDATEOSCFG | SYS_UPDATESNDDEV;
		soundrenewal = 1;
	}

	const UINT nSamplingRate = m_rate.GetCurItemData(np2cfg.samplingrate);
	if (np2cfg.samplingrate != nSamplingRate)
	{
		np2cfg.samplingrate = nSamplingRate;
		nUpdated |= SYS_UPDATECFG | SYS_UPDATERATE;
		soundrenewal = 1;
	}

	UINT nBuffer = GetDlgItemInt(IDC_SOUND_BUFFER, NULL, FALSE);
	nBuffer = max(nBuffer, 40);
	nBuffer = min(nBuffer, 1000);
	if (np2cfg.delayms != static_cast<UINT16>(nBuffer))
	{
		np2cfg.delayms = static_cast<UINT16>(nBuffer);
		nUpdated |= SYS_UPDATECFG | SYS_UPDATESBUF;
		soundrenewal = 1;
	}

	const UINT8 bAllowResize = (IsDlgButtonChecked(IDC_ALLOWRESIZE) != BST_UNCHECKED) ? 1 : 0;
	if (np2oscfg.thickframe != bAllowResize)
	{
		np2oscfg.thickframe = bAllowResize;
		nUpdated |= SYS_UPDATEOSCFG;
	}

#if !defined(_WIN64)
	if (!(mmxflag & MMXFLAG_NOTSUPPORT))
	{
		const UINT8 bDisableMMX = (IsDlgButtonChecked(IDC_DISABLEMMX) != BST_UNCHECKED) ? 1 : 0;
		if (np2oscfg.disablemmx != bDisableMMX)
		{
			np2oscfg.disablemmx = bDisableMMX;
			mmxflag &= ~MMXFLAG_DISABLE;
			mmxflag |= (bDisableMMX) ? MMXFLAG_DISABLE : 0;
			nUpdated |= SYS_UPDATEOSCFG;
		}
	}
#endif

	const UINT8 bConfirm = (IsDlgButtonChecked(IDC_COMFIRM) != BST_UNCHECKED) ? 1 : 0;
	if (np2oscfg.comfirm != bConfirm)
	{
		np2oscfg.comfirm = bConfirm;
		nUpdated |= SYS_UPDATEOSCFG;
	}

	const UINT8 bResume = (IsDlgButtonChecked(IDC_RESUME) != BST_UNCHECKED) ? 1 : 0;
	if (np2oscfg.resume != bResume)
	{
		np2oscfg.resume = bResume;
		nUpdated |= SYS_UPDATEOSCFG;
	}
	
	const UINT8 bSaveScrnMul = (IsDlgButtonChecked(IDC_SAVEWINDOWSIZE) != BST_UNCHECKED) ? 1 : 0;
	if (np2oscfg.svscrmul != bSaveScrnMul)
	{
		np2oscfg.svscrmul = bSaveScrnMul;
		nUpdated |= SYS_UPDATEOSCFG;
	}
	sysmng_update(nUpdated);

	CDlgProc::OnOK();
}

/**
 * ユーザーがメニューの項目を選択したときに、フレームワークによって呼び出されます
 * @param[in] wParam パラメタ
 * @param[in] lParam パラメタ
 * @retval TRUE アプリケーションがこのメッセージを処理した
 */
BOOL CConfigureDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
		case IDC_BASECLOCK:
			SetClock();
			return TRUE;

		case IDC_MULTIPLE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				const int nIndex = m_multiple.GetCurSel();
				if ((nIndex >= 0) && (nIndex < _countof(s_mulval)))
				{
					SetClock(s_mulval[nIndex]);
				}
			}
			else
			{
				SetClock(0);
			}
			return TRUE;

		case IDC_SOUND_DEVICE_TYPE:
			UpdateDeviceList();
			return TRUE;
	}
	return FALSE;
}

/**
 * クロックを設定する
 * @param[in] nMultiple 倍率
 */
void CConfigureDlg::SetClock(UINT nMultiple)
{
	const UINT nBaseClock = m_baseClock.GetCurItemData(PCBASECLOCK20);
	if (nMultiple == 0)
	{
		nMultiple = GetDlgItemInt(IDC_MULTIPLE, NULL, FALSE);
	}
	nMultiple = max(nMultiple, 1);
	nMultiple = min(nMultiple, 256);

	const UINT nClock = (nBaseClock / 100) * nMultiple;

	TCHAR szWork[32];
	wsprintf(szWork, str_clockfmt, nClock / 10000, nClock % 10000);
	SetDlgItemText(IDC_CLOCKMSG, szWork);
}

/**
 * 設定ダイアログ
 * @param[in] hwndParent 親ウィンドウ
 */
void dialog_configure(HWND hwndParent)
{
	CConfigureDlg dlg(hwndParent);
	dlg.DoModal();
}
