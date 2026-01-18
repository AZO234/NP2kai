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
#if defined(CPUCORE_IA32)
#include "i386c/ia32/cpu.h"
#endif

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
	CComboData m_cputype;			//!< CPU種類
	CComboData m_type;				//!< タイプ
	CComboData m_name;				//!< デバイス名
	CComboData m_rate;				//!< レート
	CWndProc   m_chk21port;			//!< PC-9821ポートマップ
	std::vector<LPCTSTR> m_dsound3;	//!< DSound3
	std::vector<LPCTSTR> m_wasapi;	//!< WASAPI
	std::vector<LPCTSTR> m_asio;	//!< ASIO
	UINT16 m_21port;	//!< PC-9821 port
	void SetClock(UINT nMultiple = 0);
	void UpdateDeviceList();
	int GetCpuTypeIndex();
	int SetCpuTypeIndex(UINT index);
};

//! コンボ ボックス アイテム
static const CComboData::Entry s_baseclock[] =
{
	{MAKEINTRESOURCE(IDS_2_0MHZ),	PCBASECLOCK20},
	{MAKEINTRESOURCE(IDS_2_5MHZ),	PCBASECLOCK25},
};

//! CPU種類 コンボ ボックス アイテム
static const CComboData::Entry s_cputype[] =
{
	{MAKEINTRESOURCE(IDS_CPU_CUSTOM),	0},
	{MAKEINTRESOURCE(IDS_CPU_80386),	1},
	{MAKEINTRESOURCE(IDS_CPU_I486SX),	2},
	{MAKEINTRESOURCE(IDS_CPU_I486DX),	3},
	{MAKEINTRESOURCE(IDS_CPU_PENTIUM),	4},
	{MAKEINTRESOURCE(IDS_CPU_MMXPENTIUM),	5},
	{MAKEINTRESOURCE(IDS_CPU_PENTIUMPRO),	6},
	{MAKEINTRESOURCE(IDS_CPU_PENTIUMII),	7},
	{MAKEINTRESOURCE(IDS_CPU_PENTIUMIII),	8},
	{MAKEINTRESOURCE(IDS_CPU_PENTIUMM),		9},
	{MAKEINTRESOURCE(IDS_CPU_PENTIUM4),		10},
	{MAKEINTRESOURCE(IDS_CPU_CORE_2_DUO),	11},
	{MAKEINTRESOURCE(IDS_CPU_CORE_2_DUOW),	12},
	{MAKEINTRESOURCE(IDS_CPU_CORE_I),		13},
	{MAKEINTRESOURCE(IDS_CPU_AMD_K6_2),		15},
	{MAKEINTRESOURCE(IDS_CPU_AMD_K6_III),	16},
	{MAKEINTRESOURCE(IDS_CPU_AMD_K7_ATHLON),	17},
	{MAKEINTRESOURCE(IDS_CPU_AMD_K7_ATHLONXP),	18},
	{MAKEINTRESOURCE(IDS_CPU_AMD_K8_ATHLON64),	19},
	{MAKEINTRESOURCE(IDS_CPU_AMD_K8_ATHLON64X2),20},
	{MAKEINTRESOURCE(IDS_CPU_AMD_K10_PHENOM),	21},
	{MAKEINTRESOURCE(IDS_CPU_NEKOPRO),	255},
};
static const CComboData::Entry s_cputype_286[] =
{
	{MAKEINTRESOURCE(IDS_CPU_80286),	0},
};

//! 倍率リスト
static const UINT32 s_mulval[] = {1, 2, 4, 5, 6, 8, 10, 12, 16, 20, 24, 30, 32, 34, 36, 
	40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 100, 120, 140 };

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
	
	m_cputype.SubclassDlgItem(IDC_CPU_TYPE, this);
#if defined(CPUCORE_IA32)
	//const UINT32 cpufeaturelist[] = {0, CPU_FEATURES_I486SX, CPU_FEATURES_I486DX, CPU_FEATURES_PENTIUM, CPU_FEATURES_MMX_PENTIUM, CPU_FEATURES_PENTIUM_PRO, CPU_FEATURES_PENTIUM_II};
	//i = 0;
	//for(i=0;i<_countof(cpufeaturelist);i++){
	//	if((CPU_FEATURES_ALL & cpufeaturelist[i]) != cpufeaturelist[i]) break;
	//}
	//m_cputype.Add(s_cputype, i);
	m_cputype.Add(s_cputype, _countof(s_cputype));
	m_cputype.SetCurItemData(GetCpuTypeIndex());
#else
	m_cputype.Add(s_cputype_286, _countof(s_cputype_286));
	m_cputype.SetCurItemData(0);
#endif

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
	
	m_21port = np2cfg.sysiomsk;
	m_chk21port.SubclassDlgItem(IDC_MODEL21, this);
#if defined(SUPPORT_PC9821)
	if(m_21port == 0xff00)
		m_chk21port.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	else if((m_21port & ~0x0c00) == 0x0000)
		m_chk21port.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
	else
		m_chk21port.SendMessage(BM_SETCHECK , BST_INDETERMINATE , 0);
#else
	m_chk21port.EnableWindow(FALSE);
#endif

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
	CheckDlgButton(IDC_SAVEWINDOWSIZEPERRES, (np2oscfg.fsrescfg) ? BST_CHECKED : BST_UNCHECKED);
#if defined(SUPPORT_MULTITHREAD)
	CheckDlgButton(IDC_MULTITHREADMODE, (np2oscfg.multithread) ? BST_CHECKED : BST_UNCHECKED);
#else
	GetDlgItem(IDC_MULTITHREADMODE).EnableWindow(FALSE);
#endif
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
 * np2cfg CPUID -> CPU type index 相互変換
 */
int CConfigureDlg::GetCpuTypeIndex(){
#if defined(CPUCORE_IA32)
	if((CPU_FEATURES_ALL & CPU_FEATURES_80386) != CPU_FEATURES_80386) goto AMDCPUCheck;
	if(np2cfg.cpu_family == CPU_80386_FAMILY && 
	   np2cfg.cpu_model == CPU_80386_MODEL &&
	   np2cfg.cpu_stepping == CPU_80386_STEPPING &&
	   (np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_80386 &&
	   (np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_80386 &&
	   (np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_80386 &&
	   (np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_80386 &&
	   np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_80386){
		return 1;
	}
	if((CPU_FEATURES_ALL & CPU_FEATURES_I486SX) != CPU_FEATURES_I486SX) goto AMDCPUCheck;
	if(np2cfg.cpu_family == CPU_I486SX_FAMILY && 
	   np2cfg.cpu_model == CPU_I486SX_MODEL &&
	   np2cfg.cpu_stepping == CPU_I486SX_STEPPING &&
	   (np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_I486SX &&
	   (np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_I486SX &&
	   (np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_I486SX &&
	   (np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_I486SX &&
	   np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_I486SX){
		return 2;
	}
	if((CPU_FEATURES_ALL & CPU_FEATURES_I486DX) != CPU_FEATURES_I486DX) goto AMDCPUCheck;
	if(np2cfg.cpu_family == CPU_I486DX_FAMILY && 
	   np2cfg.cpu_model == CPU_I486DX_MODEL &&
	   np2cfg.cpu_stepping == CPU_I486DX_STEPPING &&
	   (np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_I486DX &&
	   (np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_I486DX &&
	   (np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_I486DX &&
	   (np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_I486DX &&
	   np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_I486DX){
		return 3;
	}
	if((CPU_FEATURES_ALL & CPU_FEATURES_PENTIUM) != CPU_FEATURES_PENTIUM) goto AMDCPUCheck;
	if(np2cfg.cpu_family == CPU_PENTIUM_FAMILY && 
	   np2cfg.cpu_model == CPU_PENTIUM_MODEL &&
	   np2cfg.cpu_stepping == CPU_PENTIUM_STEPPING &&
	   (np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_PENTIUM &&
	   (np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_PENTIUM &&
	   (np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_PENTIUM &&
	   (np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_PENTIUM &&
	   np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_PENTIUM){
		return 4;
	}
	if((CPU_FEATURES_ALL & CPU_FEATURES_MMX_PENTIUM) != CPU_FEATURES_MMX_PENTIUM) goto AMDCPUCheck;
	if(np2cfg.cpu_family == CPU_MMX_PENTIUM_FAMILY && 
	   np2cfg.cpu_model == CPU_MMX_PENTIUM_MODEL &&
	   np2cfg.cpu_stepping == CPU_MMX_PENTIUM_STEPPING &&
	   (np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_MMX_PENTIUM &&
	   (np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_MMX_PENTIUM &&
	   (np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_MMX_PENTIUM &&
	   (np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_MMX_PENTIUM &&
	   np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_MMX_PENTIUM){
		return 5;
	}
	if((CPU_FEATURES_ALL & CPU_FEATURES_PENTIUM_PRO) != CPU_FEATURES_PENTIUM_PRO) goto AMDCPUCheck;
	if(np2cfg.cpu_family == CPU_PENTIUM_PRO_FAMILY && 
	   np2cfg.cpu_model == CPU_PENTIUM_PRO_MODEL &&
	   np2cfg.cpu_stepping == CPU_PENTIUM_PRO_STEPPING &&
	   (np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_PENTIUM_PRO &&
	   (np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_PENTIUM_PRO &&
	   (np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_PENTIUM_PRO &&
	   (np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_PENTIUM_PRO &&
	   np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_PENTIUM_PRO){
		return 6;
	}
	if((CPU_FEATURES_ALL & CPU_FEATURES_PENTIUM_II) != CPU_FEATURES_PENTIUM_II) goto AMDCPUCheck;
	if(np2cfg.cpu_family == CPU_PENTIUM_II_FAMILY && 
	   np2cfg.cpu_model == CPU_PENTIUM_II_MODEL &&
	   np2cfg.cpu_stepping == CPU_PENTIUM_II_STEPPING &&
	   (np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_PENTIUM_II &&
	   (np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_PENTIUM_II &&
	   (np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_PENTIUM_II &&
	   (np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_PENTIUM_II &&
	   np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_PENTIUM_II){
		return 7;
	}
	if((CPU_FEATURES_ALL & CPU_FEATURES_PENTIUM_III) != CPU_FEATURES_PENTIUM_III) goto AMDCPUCheck;
	if(np2cfg.cpu_family == CPU_PENTIUM_III_FAMILY && 
	   np2cfg.cpu_model == CPU_PENTIUM_III_MODEL &&
	   np2cfg.cpu_stepping == CPU_PENTIUM_III_STEPPING &&
	   (np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_PENTIUM_III &&
	   (np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_PENTIUM_III &&
	   (np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_PENTIUM_III &&
	   (np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_PENTIUM_III &&
	   np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_PENTIUM_III){
		return 8;
	}
	if((CPU_FEATURES_ALL & CPU_FEATURES_PENTIUM_M) != CPU_FEATURES_PENTIUM_M) goto AMDCPUCheck;
	if(np2cfg.cpu_family == CPU_PENTIUM_M_FAMILY && 
	   np2cfg.cpu_model == CPU_PENTIUM_M_MODEL &&
	   np2cfg.cpu_stepping == CPU_PENTIUM_M_STEPPING &&
	   (np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_PENTIUM_M &&
	   (np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_PENTIUM_M &&
	   (np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_PENTIUM_M &&
	   (np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_PENTIUM_M &&
	   np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_PENTIUM_M){
		return 9;
	}
	if((CPU_FEATURES_ALL & CPU_FEATURES_PENTIUM_4) != CPU_FEATURES_PENTIUM_4) goto AMDCPUCheck;
	if(np2cfg.cpu_family == CPU_PENTIUM_4_FAMILY && 
	   np2cfg.cpu_model == CPU_PENTIUM_4_MODEL &&
	   np2cfg.cpu_stepping == CPU_PENTIUM_4_STEPPING &&
	   (np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_PENTIUM_4 &&
	   (np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_PENTIUM_4 &&
	   (np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_PENTIUM_4 &&
	   (np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_PENTIUM_4 &&
	   np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_PENTIUM_4){
		return 10;
	}
	if ((CPU_FEATURES_ALL & CPU_FEATURES_CORE_2_DUO) != CPU_FEATURES_CORE_2_DUO) goto AMDCPUCheck;
	if (np2cfg.cpu_family == CPU_CORE_2_DUO_FAMILY &&
		np2cfg.cpu_model == CPU_CORE_2_DUO_MODEL &&
		np2cfg.cpu_stepping == CPU_CORE_2_DUO_STEPPING &&
		(np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_CORE_2_DUO &&
		(np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_CORE_2_DUO &&
		(np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_CORE_2_DUO &&
		(np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_CORE_2_DUO &&
		np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_CORE_2_DUO)
	{
		return 11;
	}
	if ((CPU_FEATURES_ALL & CPU_FEATURES_CORE_2_DUOW) != CPU_FEATURES_CORE_2_DUOW) goto AMDCPUCheck;
	if (np2cfg.cpu_family == CPU_CORE_2_DUOW_FAMILY &&
		np2cfg.cpu_model == CPU_CORE_2_DUOW_MODEL &&
		np2cfg.cpu_stepping == CPU_CORE_2_DUOW_STEPPING &&
		(np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_CORE_2_DUOW &&
		(np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_CORE_2_DUOW &&
		(np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_CORE_2_DUOW &&
		(np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_CORE_2_DUOW &&
		np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_CORE_2_DUOW)
	{
		return 12;
	}
	if ((CPU_FEATURES_ALL & CPU_FEATURES_CORE_I) != CPU_FEATURES_CORE_I) goto AMDCPUCheck;
	if (np2cfg.cpu_family == CPU_CORE_I_FAMILY &&
		np2cfg.cpu_model == CPU_CORE_I_MODEL &&
		np2cfg.cpu_stepping == CPU_CORE_I_STEPPING &&
		(np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_CORE_I &&
		(np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_CORE_I &&
		(np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_CORE_I &&
		(np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_CORE_I &&
		np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_CORE_I)
	{
		return 13;
	}

AMDCPUCheck:
	if((CPU_FEATURES_ALL & CPU_FEATURES_AMD_K6_2) != CPU_FEATURES_AMD_K6_2 ||
		(CPU_FEATURES_EX_ALL & CPU_FEATURES_EX_AMD_K6_2) != CPU_FEATURES_EX_AMD_K6_2) goto NekoCPUCheck;
	if(np2cfg.cpu_family == CPU_AMD_K6_2_FAMILY && 
	   np2cfg.cpu_model == CPU_AMD_K6_2_MODEL &&
	   np2cfg.cpu_stepping == CPU_AMD_K6_2_STEPPING &&
	   (np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_AMD_K6_2 &&
	   (np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_AMD_K6_2 &&
	   (np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_AMD_K6_2 &&
		(np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_AMD_K6_2 &&
	   np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_AMD_K6_2){
		return 15;
	}
	if((CPU_FEATURES_ALL & CPU_FEATURES_AMD_K6_III) != CPU_FEATURES_AMD_K6_III ||
		(CPU_FEATURES_EX_ALL & CPU_FEATURES_EX_AMD_K6_III) != CPU_FEATURES_EX_AMD_K6_III) goto NekoCPUCheck;
	if(np2cfg.cpu_family == CPU_AMD_K6_III_FAMILY && 
	   np2cfg.cpu_model == CPU_AMD_K6_III_MODEL &&
	   np2cfg.cpu_stepping == CPU_AMD_K6_III_STEPPING &&
	   (np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_AMD_K6_III &&
	   (np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_AMD_K6_III &&
	   (np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_AMD_K6_III &&
		(np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_AMD_K6_III &&
	   np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_AMD_K6_III){
		return 16;
	}
	if((CPU_FEATURES_ALL & CPU_FEATURES_AMD_K7_ATHLON) != CPU_FEATURES_AMD_K7_ATHLON ||
		(CPU_FEATURES_EX_ALL & CPU_FEATURES_EX_AMD_K7_ATHLON) != CPU_FEATURES_EX_AMD_K7_ATHLON) goto NekoCPUCheck;
	if(np2cfg.cpu_family == CPU_AMD_K7_ATHLON_FAMILY && 
	   np2cfg.cpu_model == CPU_AMD_K7_ATHLON_MODEL &&
	   np2cfg.cpu_stepping == CPU_AMD_K7_ATHLON_STEPPING &&
	   (np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_AMD_K7_ATHLON &&
	   (np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_AMD_K7_ATHLON &&
	   (np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_AMD_K7_ATHLON &&
		(np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_AMD_K7_ATHLON &&
	   np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_AMD_K7_ATHLON){
		return 17;
	}
	if((CPU_FEATURES_ALL & CPU_FEATURES_AMD_K7_ATHLON_XP) != CPU_FEATURES_AMD_K7_ATHLON_XP ||
		(CPU_FEATURES_EX_ALL & CPU_FEATURES_EX_AMD_K7_ATHLON_XP) != CPU_FEATURES_EX_AMD_K7_ATHLON_XP) goto NekoCPUCheck;
	if(np2cfg.cpu_family == CPU_AMD_K7_ATHLON_XP_FAMILY && 
	   np2cfg.cpu_model == CPU_AMD_K7_ATHLON_XP_MODEL &&
	   np2cfg.cpu_stepping == CPU_AMD_K7_ATHLON_XP_STEPPING &&
	   (np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_AMD_K7_ATHLON_XP &&
	   (np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_AMD_K7_ATHLON_XP &&
	   (np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_AMD_K7_ATHLON_XP &&
		(np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_AMD_K7_ATHLON_XP &&
	   np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_AMD_K7_ATHLON_XP){
		return 18;
	}
	if ((CPU_FEATURES_ALL & CPU_FEATURES_AMD_K8_ATHLON_64) != CPU_FEATURES_AMD_K8_ATHLON_64 ||
		(CPU_FEATURES_EX_ALL & CPU_FEATURES_EX_AMD_K8_ATHLON_64) != CPU_FEATURES_EX_AMD_K8_ATHLON_64) goto NekoCPUCheck;
	if (np2cfg.cpu_family == CPU_AMD_K8_ATHLON_64_FAMILY &&
		np2cfg.cpu_model == CPU_AMD_K8_ATHLON_64_MODEL &&
		np2cfg.cpu_stepping == CPU_AMD_K8_ATHLON_64_STEPPING &&
		(np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_AMD_K8_ATHLON_64 &&
		(np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_AMD_K8_ATHLON_64 &&
		(np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_AMD_K8_ATHLON_64 &&
		(np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_AMD_K8_ATHLON_64 &&
		np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_AMD_K8_ATHLON_64)
	{
		return 19;
	}
	if ((CPU_FEATURES_ALL & CPU_FEATURES_AMD_K8_ATHLON_64X2) != CPU_FEATURES_AMD_K8_ATHLON_64X2 ||
		(CPU_FEATURES_EX_ALL & CPU_FEATURES_EX_AMD_K8_ATHLON_64X2) != CPU_FEATURES_EX_AMD_K8_ATHLON_64X2) goto NekoCPUCheck;
	if (np2cfg.cpu_family == CPU_AMD_K8_ATHLON_64X2_FAMILY &&
		np2cfg.cpu_model == CPU_AMD_K8_ATHLON_64X2_MODEL &&
		np2cfg.cpu_stepping == CPU_AMD_K8_ATHLON_64X2_STEPPING &&
		(np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_AMD_K8_ATHLON_64X2 &&
		(np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_AMD_K8_ATHLON_64X2 &&
		(np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_AMD_K8_ATHLON_64X2 &&
		(np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_AMD_K8_ATHLON_64X2 &&
		np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_AMD_K8_ATHLON_64X2)
	{
		return 20;
	}
	if ((CPU_FEATURES_ALL & CPU_FEATURES_AMD_K10_PHENOM) != CPU_FEATURES_AMD_K10_PHENOM ||
		(CPU_FEATURES_EX_ALL & CPU_FEATURES_EX_AMD_K10_PHENOM) != CPU_FEATURES_EX_AMD_K10_PHENOM) goto NekoCPUCheck;
	if (np2cfg.cpu_family == CPU_AMD_K10_PHENOM_FAMILY &&
		np2cfg.cpu_model == CPU_AMD_K10_PHENOM_MODEL &&
		np2cfg.cpu_stepping == CPU_AMD_K10_PHENOM_STEPPING &&
		(np2cfg.cpu_feature & CPU_FEATURES_ALL) == CPU_FEATURES_AMD_K10_PHENOM &&
		(np2cfg.cpu_feature_ecx & CPU_FEATURES_ECX_ALL) == CPU_FEATURES_ECX_AMD_K10_PHENOM &&
		(np2cfg.cpu_feature_ex & CPU_FEATURES_EX_ALL) == CPU_FEATURES_EX_AMD_K10_PHENOM &&
		(np2cfg.cpu_feature_ex_ecx & CPU_FEATURES_EX_ECX_ALL) == CPU_FEATURES_EX_ECX_AMD_K10_PHENOM &&
		np2cfg.cpu_eflags_mask == CPU_EFLAGS_MASK_AMD_K10_PHENOM)
	{
		return 21;
	}
	
NekoCPUCheck:
	if(np2cfg.cpu_family == 0 && 
	   np2cfg.cpu_model == 0 &&
	   np2cfg.cpu_stepping == 0 &&
	   np2cfg.cpu_feature == 0 &&
	   np2cfg.cpu_feature_ecx == 0 &&
	   np2cfg.cpu_feature_ex == 0){
		return 255;
	}
#endif
	return 0;
}
int CConfigureDlg::SetCpuTypeIndex(UINT index){
#if defined(CPUCORE_IA32)
	switch(index){
	case 1:
		np2cfg.cpu_family = CPU_80386_FAMILY;
		np2cfg.cpu_model = CPU_80386_MODEL;
		np2cfg.cpu_stepping = CPU_80386_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_80386;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_80386;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_80386;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_80386;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_80386;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_INTEL);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_80386);
		np2cfg.cpu_brandid = CPU_BRAND_ID_80386;
		break;
	case 2:
		np2cfg.cpu_family = CPU_I486SX_FAMILY;
		np2cfg.cpu_model = CPU_I486SX_MODEL;
		np2cfg.cpu_stepping = CPU_I486SX_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_I486SX;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_I486SX;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_I486SX;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_I486SX;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_I486SX;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_INTEL);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_I486SX);
		np2cfg.cpu_brandid = CPU_BRAND_ID_I486SX;
		break;
	case 3:
		np2cfg.cpu_family = CPU_I486DX_FAMILY;
		np2cfg.cpu_model = CPU_I486DX_MODEL;
		np2cfg.cpu_stepping = CPU_I486DX_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_I486DX;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_I486DX;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_I486DX;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_I486DX;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_I486DX;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_INTEL);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_I486DX);
		np2cfg.cpu_brandid = CPU_BRAND_ID_I486DX;
		break;
	case 4:
		np2cfg.cpu_family = CPU_PENTIUM_FAMILY;
		np2cfg.cpu_model = CPU_PENTIUM_MODEL;
		np2cfg.cpu_stepping = CPU_PENTIUM_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_PENTIUM;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_PENTIUM;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_PENTIUM;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_PENTIUM;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_PENTIUM;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_INTEL);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_PENTIUM);
		np2cfg.cpu_brandid = CPU_BRAND_ID_PENTIUM;
		break;
	case 5:
		np2cfg.cpu_family = CPU_MMX_PENTIUM_FAMILY;
		np2cfg.cpu_model = CPU_MMX_PENTIUM_MODEL;
		np2cfg.cpu_stepping = CPU_MMX_PENTIUM_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_MMX_PENTIUM;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_MMX_PENTIUM;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_MMX_PENTIUM;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_MMX_PENTIUM;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_MMX_PENTIUM;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_INTEL);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_MMX_PENTIUM);
		np2cfg.cpu_brandid = CPU_BRAND_ID_MMX_PENTIUM;
		break;
	case 6:
		np2cfg.cpu_family = CPU_PENTIUM_PRO_FAMILY;
		np2cfg.cpu_model = CPU_PENTIUM_PRO_MODEL;
		np2cfg.cpu_stepping = CPU_PENTIUM_PRO_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_PENTIUM_PRO;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_PENTIUM_PRO;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_PENTIUM_PRO;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_PENTIUM_PRO;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_PENTIUM_PRO;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_INTEL);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_PENTIUM_PRO);
		np2cfg.cpu_brandid = CPU_BRAND_ID_PENTIUM_PRO;
		break;
	case 7:
		np2cfg.cpu_family = CPU_PENTIUM_II_FAMILY;
		np2cfg.cpu_model = CPU_PENTIUM_II_MODEL;
		np2cfg.cpu_stepping = CPU_PENTIUM_II_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_PENTIUM_II;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_PENTIUM_II;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_PENTIUM_II;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_PENTIUM_II;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_PENTIUM_II;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_INTEL);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_PENTIUM_II);
		np2cfg.cpu_brandid = CPU_BRAND_ID_PENTIUM_II;
		break;
	case 8:
		np2cfg.cpu_family = CPU_PENTIUM_III_FAMILY;
		np2cfg.cpu_model = CPU_PENTIUM_III_MODEL;
		np2cfg.cpu_stepping = CPU_PENTIUM_III_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_PENTIUM_III;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_PENTIUM_III;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_PENTIUM_III;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_PENTIUM_III;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_PENTIUM_III;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_INTEL);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_PENTIUM_III);
		np2cfg.cpu_brandid = CPU_BRAND_ID_PENTIUM_III;
		break;
	case 9:
		np2cfg.cpu_family = CPU_PENTIUM_M_FAMILY;
		np2cfg.cpu_model = CPU_PENTIUM_M_MODEL;
		np2cfg.cpu_stepping = CPU_PENTIUM_M_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_PENTIUM_M;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_PENTIUM_M;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_PENTIUM_M;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_PENTIUM_M;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_PENTIUM_M;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_INTEL);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_PENTIUM_M);
		np2cfg.cpu_brandid = CPU_BRAND_ID_PENTIUM_M;
		break;
	case 10:
		np2cfg.cpu_family = CPU_PENTIUM_4_FAMILY;
		np2cfg.cpu_model = CPU_PENTIUM_4_MODEL;
		np2cfg.cpu_stepping = CPU_PENTIUM_4_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_PENTIUM_4;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_PENTIUM_4;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_PENTIUM_4;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_PENTIUM_4;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_PENTIUM_4;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_INTEL);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_PENTIUM_4);
		np2cfg.cpu_brandid = CPU_BRAND_ID_PENTIUM_4;
		break;
	case 11:
		np2cfg.cpu_family = CPU_CORE_2_DUO_FAMILY;
		np2cfg.cpu_model = CPU_CORE_2_DUO_MODEL;
		np2cfg.cpu_stepping = CPU_CORE_2_DUO_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_CORE_2_DUO;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_CORE_2_DUO;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_CORE_2_DUO;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_CORE_2_DUO;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_CORE_2_DUO;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_INTEL);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_CORE_2_DUO);
		np2cfg.cpu_brandid = CPU_BRAND_ID_CORE_2_DUO;
		break;
	case 12:
		np2cfg.cpu_family = CPU_CORE_2_DUOW_FAMILY;
		np2cfg.cpu_model = CPU_CORE_2_DUOW_MODEL;
		np2cfg.cpu_stepping = CPU_CORE_2_DUOW_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_CORE_2_DUOW;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_CORE_2_DUOW;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_CORE_2_DUOW;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_CORE_2_DUOW;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_CORE_2_DUOW;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_INTEL);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_CORE_2_DUOW);
		np2cfg.cpu_brandid = CPU_BRAND_ID_CORE_2_DUOW;
		break;
	case 13:
		np2cfg.cpu_family = CPU_CORE_I_FAMILY;
		np2cfg.cpu_model = CPU_CORE_I_MODEL;
		np2cfg.cpu_stepping = CPU_CORE_I_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_CORE_I;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_CORE_I;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_CORE_I;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_CORE_I;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_CORE_I;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_INTEL);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_CORE_I);
		np2cfg.cpu_brandid = CPU_BRAND_ID_CORE_I;
		break;
	case 15:
		np2cfg.cpu_family = CPU_AMD_K6_2_FAMILY;
		np2cfg.cpu_model = CPU_AMD_K6_2_MODEL;
		np2cfg.cpu_stepping = CPU_AMD_K6_2_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_AMD_K6_2;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_AMD_K6_2;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_AMD_K6_2;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_AMD_K6_2;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_AMD_K6_2;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_AMD);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_AMD_K6_2);
		np2cfg.cpu_brandid = CPU_BRAND_ID_AMD_K6_2;
		break;
	case 16:
		np2cfg.cpu_family = CPU_AMD_K6_III_FAMILY;
		np2cfg.cpu_model = CPU_AMD_K6_III_MODEL;
		np2cfg.cpu_stepping = CPU_AMD_K6_III_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_AMD_K6_III;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_AMD_K6_III;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_AMD_K6_III;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_AMD_K6_III;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_AMD_K6_III;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_AMD);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_AMD_K6_III);
		np2cfg.cpu_brandid = CPU_BRAND_ID_AMD_K6_III;
		break;
	case 17:
		np2cfg.cpu_family = CPU_AMD_K7_ATHLON_FAMILY;
		np2cfg.cpu_model = CPU_AMD_K7_ATHLON_MODEL;
		np2cfg.cpu_stepping = CPU_AMD_K7_ATHLON_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_AMD_K7_ATHLON;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_AMD_K7_ATHLON;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_AMD_K7_ATHLON;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_AMD_K7_ATHLON;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_AMD_K7_ATHLON;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_AMD);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_AMD_K7_ATHLON);
		np2cfg.cpu_brandid = CPU_BRAND_ID_AMD_K7_ATHLON;
		break;
	case 18:
		np2cfg.cpu_family = CPU_AMD_K7_ATHLON_XP_FAMILY;
		np2cfg.cpu_model = CPU_AMD_K7_ATHLON_XP_MODEL;
		np2cfg.cpu_stepping = CPU_AMD_K7_ATHLON_XP_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_AMD_K7_ATHLON_XP;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_AMD_K7_ATHLON_XP;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_AMD_K7_ATHLON_XP;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_AMD_K7_ATHLON_XP;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_AMD_K7_ATHLON_XP;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_AMD);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_AMD_K7_ATHLON_XP);
		np2cfg.cpu_brandid = CPU_BRAND_ID_AMD_K7_ATHLON_XP;
		break;
	case 19:
		np2cfg.cpu_family = CPU_AMD_K8_ATHLON_64_FAMILY;
		np2cfg.cpu_model = CPU_AMD_K8_ATHLON_64_MODEL;
		np2cfg.cpu_stepping = CPU_AMD_K8_ATHLON_64_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_AMD_K8_ATHLON_64;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_AMD_K8_ATHLON_64;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_AMD_K8_ATHLON_64;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_AMD_K8_ATHLON_64;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_AMD_K8_ATHLON_64;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_AMD);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_AMD_K8_ATHLON_64);
		np2cfg.cpu_brandid = CPU_BRAND_ID_AMD_K8_ATHLON_64;
		break;
	case 20:
		np2cfg.cpu_family = CPU_AMD_K8_ATHLON_64X2_FAMILY;
		np2cfg.cpu_model = CPU_AMD_K8_ATHLON_64X2_MODEL;
		np2cfg.cpu_stepping = CPU_AMD_K8_ATHLON_64X2_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_AMD_K8_ATHLON_64X2;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_AMD_K8_ATHLON_64X2;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_AMD_K8_ATHLON_64X2;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_AMD_K8_ATHLON_64X2;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_AMD_K8_ATHLON_64X2;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_AMD);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_AMD_K8_ATHLON_64X2);
		np2cfg.cpu_brandid = CPU_BRAND_ID_AMD_K8_ATHLON_64X2;
		break;
	case 21:
		np2cfg.cpu_family = CPU_AMD_K10_PHENOM_FAMILY;
		np2cfg.cpu_model = CPU_AMD_K10_PHENOM_MODEL;
		np2cfg.cpu_stepping = CPU_AMD_K10_PHENOM_STEPPING;
		np2cfg.cpu_feature = CPU_FEATURES_AMD_K10_PHENOM;
		np2cfg.cpu_feature_ecx = CPU_FEATURES_ECX_AMD_K10_PHENOM;
		np2cfg.cpu_feature_ex = CPU_FEATURES_EX_AMD_K10_PHENOM;
		np2cfg.cpu_feature_ex_ecx = CPU_FEATURES_EX_ECX_AMD_K10_PHENOM;
		np2cfg.cpu_eflags_mask = CPU_EFLAGS_MASK_AMD_K10_PHENOM;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_AMD);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_AMD_K10_PHENOM);
		np2cfg.cpu_brandid = CPU_BRAND_ID_AMD_K10_PHENOM;
		break;
	case 255: // 全機能使用可
		np2cfg.cpu_family = 0;
		np2cfg.cpu_model = 0;
		np2cfg.cpu_stepping = 0;
		np2cfg.cpu_feature = 0;
		np2cfg.cpu_feature_ecx = 0;
		np2cfg.cpu_feature_ex = 0;
		np2cfg.cpu_eflags_mask = 0;
		strcpy(np2cfg.cpu_vendor, CPU_VENDOR_NEKOPRO);
		strcpy(np2cfg.cpu_brandstring, CPU_BRAND_STRING_NEKOPRO);
		np2cfg.cpu_brandid = 0;
		break;
	default:
		return 0;
	}
//#ifdef UNICODE
//	MultiByteToWideChar(CP_ACP, 0, np2cfg.cpu_vendor, -1, np2cfg.cpu_vendor_o, sizeof(np2cfg.cpu_vendor_o));
//	MultiByteToWideChar(CP_ACP, 0, np2cfg.cpu_brandstring, -1, np2cfg.cpu_brandstring_o, sizeof(np2cfg.cpu_brandstring_o));
//#else
//	strcpy(np2cfg.cpu_vendor_o, np2cfg.cpu_vendor);
//	strcpy(np2cfg.cpu_brandstring_o, np2cfg.cpu_brandstring);
//#endif
	return SYS_UPDATECFG;
#else
	return 0;
#endif
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
	nMultiple = min(nMultiple, 2048);
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
	
#if defined(SUPPORT_PC9821)
	if(np2cfg.sysiomsk != m_21port){
		np2cfg.sysiomsk = m_21port;
		nUpdated |= SYS_UPDATECFG;
	}
#endif
#if defined(CPUCORE_IA32)
	UINT nCpuTypeIndex = m_cputype.GetCurItemData(GetCpuTypeIndex());
	if(GetCpuTypeIndex() != nCpuTypeIndex){
		nUpdated |= SetCpuTypeIndex(nCpuTypeIndex);
	}
#endif

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
	const UINT8 bSaveResConfig = (IsDlgButtonChecked(IDC_SAVEWINDOWSIZEPERRES) != BST_UNCHECKED) ? 1 : 0;
	if (np2oscfg.fsrescfg != bSaveResConfig)
	{
		np2oscfg.fsrescfg = bSaveResConfig;
		nUpdated |= SYS_UPDATEOSCFG;
	}
#if defined(SUPPORT_MULTITHREAD)
	const UINT8 bMultithreadMode = (IsDlgButtonChecked(IDC_MULTITHREADMODE) != BST_UNCHECKED) ? 1 : 0;
	if (np2oscfg.multithread != bMultithreadMode)
	{
		np2oscfg.multithread = bMultithreadMode;
		nUpdated |= SYS_UPDATEOSCFG;
	}
	sysmng_update(nUpdated);
#endif

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

		case IDC_MODEL21:
			m_21port = (m_chk21port.SendMessage(BM_GETCHECK , 0 , 0) ? 0xff00 : 0x0000);
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
	nMultiple = min(nMultiple, 2048);

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
