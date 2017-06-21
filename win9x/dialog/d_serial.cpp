/**
 * @file	d_serial.cpp
 * @brief	Serial configure dialog procedure
 */

#include "compiler.h"
#include "resource.h"
#include "dialog.h"
#include "c_combodata.h"
#include "c_dipsw.h"
#include "c_midi.h"
#include "np2class.h"
#include "commng\cmserial.h"
#include "dosio.h"
#include "np2.h"
#include "sysmng.h"
#include "misc\PropProc.h"
#include "pccore.h"
#include "iocore.h"
#include "cbus\pc9861k.h"
#include "common\strres.h"
#include "generic\dipswbmp.h"

#ifdef __cplusplus
extern "C"
{
#endif
extern COMMNG cm_rs232c;
extern COMMNG cm_pc9861ch1;
extern COMMNG cm_pc9861ch2;
#ifdef __cplusplus
}
#endif

/**
 * @brief COM 設定
 */
class SerialOptComPage : public CPropPageProc
{
public:
	SerialOptComPage(UINT nCaption, COMMNG cm, COMCFG& cfg);
	virtual ~SerialOptComPage();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

private:
	COMMNG m_cm;				//!< パラメタ
	COMCFG& m_cfg;				//!< コンフィグ
	CComboData m_port;			//!< Port
	CComboData m_speed;			//!< Speed
	CComboData m_chars;			//!< Chars
	CComboData m_parity;		//!< Parity
	CComboData m_sbit;			//!< Stop bits
	CComboMidiDevice m_midiout;	//!< MIDI OUT
	CComboMidiModule m_module;	//!< MIDI Module
	CEditMimpiFile m_mimpifile;	//!< MIMPI
	void UpdateControls();
};

//! ポート
static const CComboData::Entry s_port[] =
{
	{MAKEINTRESOURCE(IDS_NONCONNECT),	COMPORT_NONE},
	{MAKEINTRESOURCE(IDS_COM1),			COMPORT_COM1},
	{MAKEINTRESOURCE(IDS_COM2),			COMPORT_COM2},
	{MAKEINTRESOURCE(IDS_COM3),			COMPORT_COM3},
	{MAKEINTRESOURCE(IDS_COM4),			COMPORT_COM4},
	{MAKEINTRESOURCE(IDS_MIDI),			COMPORT_MIDI},
};

//! キャラクタ サイズ
static const CComboData::Value s_chars[] =
{
	{5,	0x00},
	{6,	0x04},
	{7,	0x08},
	{8,	0x0c},
};

//! パリティ
static const CComboData::Entry s_parity[] =
{
    {MAKEINTRESOURCE(IDS_PARITY_NONE),	0x00},
    {MAKEINTRESOURCE(IDS_PARITY_ODD),	0x20},
	{MAKEINTRESOURCE(IDS_PARITY_EVEN),	0x30},
};

//! ストップ ビット
static const CComboData::Entry s_sbit[] =
{
    {MAKEINTRESOURCE(IDS_1),			0x40},
    {MAKEINTRESOURCE(IDS_1HALF),		0x80},
	{MAKEINTRESOURCE(IDS_2),			0xc0},
};

/**
 * コンストラクタ
 * @param[in] nCaption キャプション ID
 * @param[in] cm パラメータ
 * @param[in] cfg コンフィグ
 */
SerialOptComPage::SerialOptComPage(UINT nCaption, COMMNG cm, COMCFG& cfg)
	: CPropPageProc(IDD_SERIAL1, nCaption)
	, m_cm(cm)
	, m_cfg(cfg)
{
}

/**
 * デストラクタ
 */
SerialOptComPage::~SerialOptComPage()
{
}

/**
 * このメソッドは WM_INITDIALOG のメッセージに応答して呼び出されます
 * @retval TRUE 最初のコントロールに入力フォーカスを設定
 * @retval FALSE 既に設定済
 */
BOOL SerialOptComPage::OnInitDialog()
{
	m_port.SubclassDlgItem(IDC_COM1PORT, this);
	m_port.Add(s_port, _countof(s_port));
	m_port.SetCurItemData(m_cfg.port);

	m_speed.SubclassDlgItem(IDC_COM1SPEED, this);
	m_speed.Add(cmserial_speed, _countof(cmserial_speed));
	m_speed.SetCurItemData(m_cfg.speed);

	m_chars.SubclassDlgItem(IDC_COM1CHARSIZE, this);
	m_chars.Add(s_chars, _countof(s_chars));
	m_chars.SetCurItemData(m_cfg.param & 0x0c);

	m_parity.SubclassDlgItem(IDC_COM1PARITY, this);
	m_parity.Add(s_parity, _countof(s_parity));
	const UINT8 cParity = m_cfg.param & 0x30;
	m_parity.SetCurItemData((cParity & 0x20) ? cParity : 0);

	m_sbit.SubclassDlgItem(IDC_COM1STOPBIT, this);
	m_sbit.Add(s_sbit, _countof(s_sbit));
	const UINT8 cSBit = m_cfg.param & 0xc0;
	m_sbit.SetCurItemData((cSBit) ? cSBit : 0x40);

	m_midiout.SubclassDlgItem(IDC_COM1MMAP, this);
	m_midiout.EnumerateMidiOut();
	m_midiout.SetCurString(m_cfg.mout);

	m_module.SubclassDlgItem(IDC_COM1MMDL, this);
	m_module.SetWindowText(m_cfg.mdl);
	CheckDlgButton(IDC_COM1DEFE, (m_cfg.def_en) ? BST_CHECKED : BST_UNCHECKED);

	m_mimpifile.SubclassDlgItem(IDC_COM1DEFF, this);
	m_mimpifile.SetWindowText(m_cfg.def);

	UpdateControls();

	m_port.SetFocus();
	return FALSE;
}

/**
 * ユーザーが OK のボタン (IDOK ID がのボタン) をクリックすると呼び出されます
 */
void SerialOptComPage::OnOK()
{
	UINT nUpdated = 0;

	const UINT8 cPort = static_cast<UINT8>(m_port.GetCurItemData(m_cfg.port));
	if (m_cfg.port != cPort)
	{
		m_cfg.port = cPort;
		nUpdated |= SYS_UPDATEOSCFG | SYS_UPDATESERIAL1;
	}

	const UINT32 nSpeed = m_speed.GetCurItemData(m_cfg.speed);
	if (m_cfg.speed != nSpeed)
	{
		m_cfg.speed = nSpeed;
		nUpdated |= SYS_UPDATEOSCFG | SYS_UPDATESERIAL1;
	}

	UINT8 cParam = 0;
	cParam |= m_chars.GetCurItemData(m_cfg.param & 0x0c);
	cParam |= m_parity.GetCurItemData(m_cfg.param & 0x30);
	cParam |= m_sbit.GetCurItemData(m_cfg.param & 0xc3);
	if (m_cfg.param != cParam)
	{
		m_cfg.param = cParam;
		nUpdated |= SYS_UPDATEOSCFG | SYS_UPDATESERIAL1;
	}

	TCHAR mmap[MAXPNAMELEN];
	GetDlgItemText(IDC_COM1MMAP, mmap, _countof(mmap));
	if (milstr_cmp(m_cfg.mout, mmap))
	{
		milstr_ncpy(m_cfg.mout, mmap, _countof(m_cfg.mout));
		nUpdated |= SYS_UPDATEOSCFG | SYS_UPDATESERIAL1;
	}

	TCHAR mmdl[64];
	GetDlgItemText(IDC_COM1MMDL, mmdl, _countof(mmdl));
	if (milstr_cmp(m_cfg.mdl, mmdl))
	{
		milstr_ncpy(m_cfg.mdl, mmdl, _countof(m_cfg.mdl));
		nUpdated |= SYS_UPDATEOSCFG | SYS_UPDATESERIAL1;
	}

	const UINT8 cDefEnable = (IsDlgButtonChecked(IDC_COM1DEFE) != BST_UNCHECKED) ? 1 : 0;
	if (m_cfg.def_en != cDefEnable)
	{
		m_cfg.def_en = cDefEnable;
		if (m_cm)
		{
			m_cm->msg(m_cm, COMMSG_MIMPIDEFEN, cDefEnable);
		}
		nUpdated |= SYS_UPDATEOSCFG;
	}

	TCHAR mdef[MAX_PATH];
	GetDlgItemText(IDC_COM1DEFF, mdef, _countof(mdef));
	if (milstr_cmp(m_cfg.def, mdef))
	{
		milstr_ncpy(m_cfg.def, mdef, _countof(m_cfg.def));
		if (m_cm)
		{
			m_cm->msg(m_cm, COMMSG_MIMPIDEFFILE, reinterpret_cast<INTPTR>(mdef));
		}
		nUpdated |= SYS_UPDATEOSCFG;
	}

	sysmng_update(nUpdated);
}

/**
 * ユーザーがメニューの項目を選択したときに、フレームワークによって呼び出されます
 * @param[in] wParam パラメタ
 * @param[in] lParam パラメタ
 * @retval TRUE アプリケーションがこのメッセージを処理した
 */
BOOL SerialOptComPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
		case IDC_COM1PORT:
			UpdateControls();
			return TRUE;

		case IDC_COM1DEFB:
			m_mimpifile.Browse();
			return TRUE;
	}
	return FALSE;
}

/**
 * コントロール更新
 */
void SerialOptComPage::UpdateControls()
{
	const UINT nPort = m_port.GetCurItemData(m_cfg.port);
	const bool bSerialShow = ((nPort >= COMPORT_COM1) && (nPort <= COMPORT_COM4));
	const bool bMidiShow = (nPort == COMPORT_MIDI);

	static const UINT serial[] =
	{
		IDC_COM1SPEED, IDC_COM1CHARSIZE, IDC_COM1PARITY, IDC_COM1STOPBIT,
		IDC_COM1STR00, IDC_COM1STR01, IDC_COM1STR02, IDC_COM1STR03,
		IDC_COM1STR04, IDC_COM1STR05, IDC_COM1STR06, IDC_COM1STR07
	};
	for (UINT i = 0; i < _countof(serial); i++)
	{
		CWndBase wnd = GetDlgItem(serial[i]);
		wnd.EnableWindow(bSerialShow ? TRUE : FALSE);
		wnd.ShowWindow(bSerialShow ? SW_SHOW : SW_HIDE);
	}

	static const UINT midi[] =
	{
		IDC_COM1MMAP, IDC_COM1MMDL, IDC_COM1DEFE, IDC_COM1DEFF, IDC_COM1DEFB,
		IDC_COM1STR10, IDC_COM1STR11, IDC_COM1STR12
	};
	for (UINT i = 0; i < _countof(midi); i++)
	{
		CWndBase wnd = GetDlgItem(midi[i]);
		wnd.EnableWindow(bMidiShow ? TRUE : FALSE);
		wnd.ShowWindow(bMidiShow ? SW_SHOW : SW_HIDE);
	}
}



// ----

/**
 * @brief 61 ページ
 */
class SerialOpt61Page : public CPropPageProc
{
public:
	SerialOpt61Page();
	virtual ~SerialOpt61Page();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	UINT8 m_sw[3];				//!< スイッチ
	UINT8 m_jmp[6];				//!< ジャンパ
	CComboData m_speed[2];		//!< Speed
	CComboData m_int[2];		//!< INT
	CComboData m_sync[2];		//!< Mode
	CStaticDipSw m_dipsw;		//!< DIPSW
	void Set(const UINT8* sw, const UINT8* jmp);
	UINT8 GetMode(UINT nIndex, UINT8 cMode);
	void SetMode(UINT nIndex, UINT8 cMode);
	void UpdateMode(UINT nIndex, UINT8& cMode);
	void OnDipSw();
};

enum
{
	PC9861S1_X		= 1,
	PC9861S2_X		= 10,
	PC9861S3_X		= 17,
	PC9861S_Y		= 1,

	PC9861J1_X		= 1,
	PC9861J2_X		= 9,
	PC9861J3_X		= 17,
	PC9861J4_X		= 1,
	PC9861J5_X		= 11,
	PC9861J6_X		= 19,
	PC9861J1_Y		= 4,
	PC9861J4_Y		= 7
};

//! INT1
static const CComboData::Value s_int1[] =
{
	{0,	0x00},
	{1,	0x02},
	{2,	0x01},
	{3,	0x03},
};

//! INT2
static const CComboData::Value s_int2[] =
{
	{0,	0x00},
	{4,	0x08},
	{5,	0x04},
	{6,	0x0c},
};

//! 同期方法
static const CComboData::Entry s_sync[] =
{
	{MAKEINTRESOURCE(IDS_SYNC),		0x03},
	{MAKEINTRESOURCE(IDS_ASYNC),	0x00},
	{MAKEINTRESOURCE(IDS_ASYNC16X),	0x01},
	{MAKEINTRESOURCE(IDS_ASYNC64X),	0x02},
};

/**
 * コンストラクタ
 */
SerialOpt61Page::SerialOpt61Page()
	: CPropPageProc(IDD_PC9861A)
{
	ZeroMemory(m_sw, sizeof(m_sw));
	ZeroMemory(m_jmp, sizeof(m_jmp));
}

/**
 * デストラクタ
 */
SerialOpt61Page::~SerialOpt61Page()
{
}

/**
 * このメソッドは WM_INITDIALOG のメッセージに応答して呼び出されます
 * @retval TRUE 最初のコントロールに入力フォーカスを設定
 * @retval FALSE 既に設定済
 */
BOOL SerialOpt61Page::OnInitDialog()
{
	CheckDlgButton(IDC_PC9861E, (np2cfg.pc9861enable) ? BST_CHECKED : BST_UNCHECKED);

	m_speed[0].SubclassDlgItem(IDC_CH1SPEED, this);
	m_speed[0].Add(pc9861k_speed, _countof(pc9861k_speed));
	m_speed[1].SubclassDlgItem(IDC_CH2SPEED, this);
	m_speed[1].Add(pc9861k_speed, _countof(pc9861k_speed));

	m_int[0].SubclassDlgItem(IDC_CH1INT, this);
	m_int[0].Add(s_int1, _countof(s_int1));
	m_int[1].SubclassDlgItem(IDC_CH2INT, this);
	m_int[1].Add(s_int2, _countof(s_int2));

	m_sync[0].SubclassDlgItem(IDC_CH1MODE, this);
	m_sync[0].Add(s_sync, _countof(s_sync));
	m_sync[1].SubclassDlgItem(IDC_CH2MODE, this);
	m_sync[1].Add(s_sync, _countof(s_sync));

	Set(np2cfg.pc9861sw, np2cfg.pc9861jmp);

	m_dipsw.SubclassDlgItem(IDC_PC9861DIP, this);

	return TRUE;
}

/**
 * ユーザーが OK のボタン (IDOK ID がのボタン) をクリックすると呼び出されます
 */
void SerialOpt61Page::OnOK()
{
	bool bUpdated = false;

	const UINT8 cEnabled = (IsDlgButtonChecked(IDC_PC9861E) != BST_UNCHECKED) ? 1 : 0;
	if (np2cfg.pc9861enable != cEnabled)
	{
		np2cfg.pc9861enable = cEnabled;
		bUpdated = true;
	}

	if (memcmp(np2cfg.pc9861sw, m_sw, 3))
	{
		CopyMemory(np2cfg.pc9861sw, m_sw, 3);
		bUpdated = true;
	}
	if (memcmp(np2cfg.pc9861jmp, m_jmp, 6))
	{
		CopyMemory(np2cfg.pc9861jmp, m_jmp, 6);
		bUpdated = true;
	}

	if (bUpdated)
	{
		::sysmng_update(SYS_UPDATECFG);
	}
}

/**
 * ユーザーがメニューの項目を選択したときに、フレームワークによって呼び出されます
 * @param[in] wParam パラメタ
 * @param[in] lParam パラメタ
 * @retval TRUE アプリケーションがこのメッセージを処理した
 */
BOOL SerialOpt61Page::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
		case IDC_CH1SPEED:
		case IDC_CH1MODE:
			UpdateMode(0, m_sw[0]);
			break;

		case IDC_CH2SPEED:
		case IDC_CH2MODE:
			UpdateMode(1, m_sw[2]);
			break;

		case IDC_CH1INT:
		case IDC_CH2INT:
			{
				UINT8 cMode = m_sw[1] & 0xf0;
				cMode |= m_int[0].GetCurItemData(m_sw[1] & 0x03);
				cMode |= m_int[1].GetCurItemData(m_sw[1] & 0x0c);
				if (m_sw[1] != cMode)
				{
					m_sw[1] = cMode;
					m_dipsw.Invalidate();
				}
			}
			break;

		case IDC_PC9861DIP:
			OnDipSw();
			break;
	}
	return FALSE;
}

/**
 * CWndProc オブジェクトの Windows プロシージャ (WindowProc) が用意されています
 * @param[in] nMsg 処理される Windows メッセージを指定します
 * @param[in] wParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @param[in] lParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
 * @return メッセージに依存する値を返します
 */
LRESULT SerialOpt61Page::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	switch (nMsg)
	{
		case WM_DRAWITEM:
			if (LOWORD(wParam) == IDC_PC9861DIP)
			{
				UINT8* pBitmap = dipswbmp_get9861(m_sw, m_jmp);
				m_dipsw.Draw((reinterpret_cast<LPDRAWITEMSTRUCT>(lParam))->hDC, pBitmap);
				_MFREE(pBitmap);
			}
			return FALSE;
	}
	return CDlgProc::WindowProc(nMsg, wParam, lParam);
}

/**
 * コントロール設定
 * @param[in] sw 設定値
 * @param[in] jmp 設定値
 */
void SerialOpt61Page::Set(const UINT8* sw, const UINT8* jmp)
{
	CopyMemory(m_sw, sw, sizeof(m_sw));
	CopyMemory(m_jmp, jmp, sizeof(m_jmp));

	SetMode(0, sw[0]);
	SetMode(1, sw[2]);
	m_int[0].SetCurItemData(sw[1] & 0x03);
	m_int[1].SetCurItemData(sw[1] & 0x0c);
}

/**
 * モードを得る
 * @param[in] nIndex ポート
 * @param[in] cMode デフォルト値
 * @return モード
 */
UINT8 SerialOpt61Page::GetMode(UINT nIndex, UINT8 cMode)
{
	const UINT8 cSync = m_sync[nIndex].GetCurItemData(cMode & 0x03);

	UINT nSpeed = m_speed[nIndex].GetCurSel();
	if (nSpeed > (_countof(pc9861k_speed) - 1))
	{
		nSpeed = _countof(pc9861k_speed) - 1;
	}
	if (cSync & 2)
	{
		nSpeed += 3;
	}
	else
	{
		if (nSpeed)
		{
			nSpeed--;
		}
	}
	return (((~nSpeed) & 0x0f) << 2) | cSync;
}

/**
 * モードを設定
 * @param[in] nIndex ポート
 * @param[in] cMode モード
 */
void SerialOpt61Page::SetMode(UINT nIndex, UINT8 cMode)
{
	UINT nSpeed = (((~cMode) >> 2) & 0x0f) + 1;
	if (cMode)
	{
		if (nSpeed > 4)
		{
			nSpeed -= 4;
		}
		else
		{
			nSpeed = 0;
		}
	}
	if (nSpeed > (_countof(pc9861k_speed) - 1))
	{
		nSpeed = _countof(pc9861k_speed) - 1;
	}
	m_speed[nIndex].SetCurSel(nSpeed);
	m_sync[nIndex].SetCurItemData(cMode & 0x03);
}

/**
 * 更新
 * @param[in] nIndex ポート
 * @param[out] cMode モード
 */
void SerialOpt61Page::UpdateMode(UINT nIndex, UINT8& cMode)
{
	const UINT8 cValue = GetMode(nIndex, cMode);
	if (cMode != cValue)
	{
		cMode = cValue;
		SetMode(nIndex, cMode);
		m_dipsw.Invalidate();
	}
}

/**
 * DIPSW をタップした
 */
void SerialOpt61Page::OnDipSw()
{
	RECT rect1;
	m_dipsw.GetWindowRect(&rect1);

	RECT rect2;
	m_dipsw.GetClientRect(&rect2);

	POINT p;
	::GetCursorPos(&p);
	p.x += rect2.left - rect1.left;
	p.y += rect2.top - rect1.top;
	p.x /= 9;
	p.y /= 9;

	UINT8 sw[3];
	UINT8 jmp[6];
	CopyMemory(sw, m_sw, sizeof(sw));
	CopyMemory(jmp, m_jmp, sizeof(jmp));

	if ((p.y >= 1) && (p.y < 3))					// 1段目
	{
		if ((p.x >= 1) && (p.x < 7))				// S1
		{
			sw[0] ^= (1 << (p.x - 1));
		}
		else if ((p.x >= 10) && (p.x < 14))			// S2
		{
			sw[1] ^= (1 << (p.x - 10));
		}
		else if ((p.x >= 17) && (p.x < 23))			// S3
		{
			sw[2] ^= (1 << (p.x - 17));
		}
	}
	else if ((p.y >= 4) && (p.y < 6))				// 2段目
	{
		if ((p.x >= 1) && (p.x < 7))				// J1
		{
			jmp[0] ^= (1 << (p.x - 1));
		}
		else if ((p.x >= 9) && (p.x < 15))			// J2
		{
			jmp[1] ^= (1 << (p.x - 9));
		}
		else if ((p.x >= 17) && (p.x < 19))			// J3
		{
			jmp[2] = (1 << (p.x - 17));
		}
	}
	else if ((p.y >= 7) && (p.y < 9))				// 3段目
	{
		if ((p.x >= 1) && (p.x < 9))				// J4
		{
			const UINT8 cBit = (1 << (p.x - 1));
			jmp[3] = (jmp[3] != cBit) ? cBit : 0;
		}
		else if ((p.x >= 11) && (p.x < 17))			// J5
		{
			jmp[4] ^= (1 << (p.x - 11));
		}
		else if ((p.x >= 19) && (p.x < 25))			// J6
		{
			jmp[5] ^= (1 << (p.x - 19));
		}
	}

	if ((memcmp(m_sw, sw, sizeof(sw)) != 0) || (memcmp(m_jmp, jmp, sizeof(jmp)) != 0))
	{
		Set(sw, jmp);
		m_dipsw.Invalidate();
	}
}



// ----

/**
 * シリアル設定
 * @param[in] hwndParent 親ウィンドウ
 */
void dialog_serial(HWND hwndParent)
{
	CPropSheetProc prop(IDS_SERIALOPTION, hwndParent);

	SerialOptComPage com1(IDS_SERIAL1, cm_rs232c, np2oscfg.com1);
	prop.AddPage(&com1);

	SerialOpt61Page pc9861;
	prop.AddPage(&pc9861);

	SerialOptComPage com2(IDS_PC9861B, cm_pc9861ch1, np2oscfg.com2);
	prop.AddPage(&com2);

	SerialOptComPage com3(IDS_PC9861C, cm_pc9861ch2, np2oscfg.com3);
	prop.AddPage(&com3);

	prop.m_psh.dwFlags |= PSH_NOAPPLYNOW | PSH_USEHICON | PSH_USECALLBACK;
	prop.m_psh.hIcon = LoadIcon(CWndProc::GetResourceHandle(), MAKEINTRESOURCE(IDI_ICON2));
	prop.m_psh.pfnCallback = np2class_propetysheet;
	prop.DoModal();

	::InvalidateRect(hwndParent, NULL, TRUE);
}
