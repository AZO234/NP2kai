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
#include "commng\cmspooler.h"

#include <shlobj.h>

#ifdef __cplusplus
extern "C"
{
#endif
extern COMMNG cm_rs232c;
extern COMMNG cm_pc9861ch1;
extern COMMNG cm_pc9861ch2;
extern COMMNG cm_prt;
#ifdef __cplusplus
}
#endif

/**
 * @brief COM 設定
 */
class SerialOptComPage : public CPropPageProc
{
public:
	SerialOptComPage(UINT nCaption, COMMNG cm, COMCFG& cfg, bool isSerial = true, bool isParallel = false);
	virtual ~SerialOptComPage();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

private:
	const CComboData::Entry* m_portlist;	//!< 使用可能なポート設定リスト
	int m_portlistlen;				//!< 使用可能なポート設定リストのサイズ
	COMMNG m_cm;					//!< パラメタ
	COMCFG& m_cfg;					//!< コンフィグ
	UINT8 m_pentabfa;				//!< ペンタブアスペクト比固定
	CWndProc m_chkpentabfa;			//!< Pen tablet fixed aspect mode
	CWndProc m_chkfixedspeed;		//!< Fixed speed mode
	CWndProc m_chkDSRcheck;			//!< Hardware DSR check mode
	CComboData m_port;				//!< Port
	CComboData m_speed;				//!< Speed
	CComboData m_chars;				//!< Chars
	CComboData m_parity;			//!< Parity
	CComboData m_sbit;				//!< Stop bits
	CComboMidiDevice m_midiout;		//!< MIDI OUT
	CComboMidiModule m_module;		//!< MIDI Module
	CEditMimpiFile m_mimpifile;		//!< MIMPI
	CWndProc m_pipename;			//!< Pipe name
	CWndProc m_pipeserv;			//!< Pipe server
	CWndProc m_txtdirpath;			//!< Directory path
	CWndProc m_nudfiletimeout;		//!< File dump timeout
	CComboData m_cmbspname;			//!< Printer name
	CWndProc m_nudsptimeout;		//!< Spooler timeout
	CWndProc m_chkspcfg;			//!< Spooler show config
	CComboData m_cmbspemumode;		//!< Spooler emulation mode
	CWndProc m_nudspemu_dotsize;	//!< Spooler emulation dot size
	CWndProc m_chkspemu_rectdot;	//!< Spooler emulation rectangle dot mode
	CWndProc m_nudspemu_offsetx;	//!< Spooler emulation position offset X
	CWndProc m_nudspemu_offsety;	//!< Spooler emulation position offset y
	CWndProc m_nudspemu_scale;		//!< Spooler emulation scale
	void UpdateControls();
};

//! ポート
static const CComboData::Entry s_portALL[] =
{
	{MAKEINTRESOURCE(IDS_NONCONNECT),	COMPORT_NONE},
	{MAKEINTRESOURCE(IDS_COM1),			COMPORT_COM1},
	{MAKEINTRESOURCE(IDS_COM2),			COMPORT_COM2},
	{MAKEINTRESOURCE(IDS_COM3),			COMPORT_COM3},
	{MAKEINTRESOURCE(IDS_COM4),			COMPORT_COM4},
	{MAKEINTRESOURCE(IDS_MIDI),			COMPORT_MIDI},
#if defined(SUPPORT_WACOM_TABLET)
	{MAKEINTRESOURCE(IDS_TABLET),		COMPORT_TABLET},
#endif
#if defined(SUPPORT_NAMED_PIPE)
	{MAKEINTRESOURCE(IDS_PIPE),			COMPORT_PIPE},
#endif
	{MAKEINTRESOURCE(IDS_CFILE),		COMPORT_FILE},
	{MAKEINTRESOURCE(IDS_LPT1),			COMPORT_LPT1},
	{MAKEINTRESOURCE(IDS_LPT2),			COMPORT_LPT2},
	{MAKEINTRESOURCE(IDS_LPT3),			COMPORT_LPT3},
	{MAKEINTRESOURCE(IDS_LPT4),			COMPORT_LPT4},
	{MAKEINTRESOURCE(IDS_SPOOLER),		COMPORT_SPOOLER},
};
static const CComboData::Entry s_portCOM[] =
{
	{MAKEINTRESOURCE(IDS_NONCONNECT),	COMPORT_NONE},
	{MAKEINTRESOURCE(IDS_COM1),			COMPORT_COM1},
	{MAKEINTRESOURCE(IDS_COM2),			COMPORT_COM2},
	{MAKEINTRESOURCE(IDS_COM3),			COMPORT_COM3},
	{MAKEINTRESOURCE(IDS_COM4),			COMPORT_COM4},
	{MAKEINTRESOURCE(IDS_MIDI),			COMPORT_MIDI},
#if defined(SUPPORT_WACOM_TABLET)
	{MAKEINTRESOURCE(IDS_TABLET),		COMPORT_TABLET},
#endif
#if defined(SUPPORT_NAMED_PIPE)
	{MAKEINTRESOURCE(IDS_PIPE),			COMPORT_PIPE},
#endif
	{MAKEINTRESOURCE(IDS_CFILE),		COMPORT_FILE},
};
static const CComboData::Entry s_portLPT[] =
{
	{MAKEINTRESOURCE(IDS_NONCONNECT),	COMPORT_NONE},
	{MAKEINTRESOURCE(IDS_LPT1),			COMPORT_LPT1},
	{MAKEINTRESOURCE(IDS_LPT2),			COMPORT_LPT2},
	{MAKEINTRESOURCE(IDS_LPT3),			COMPORT_LPT3},
	{MAKEINTRESOURCE(IDS_LPT4),			COMPORT_LPT4},
#if defined(SUPPORT_NAMED_PIPE)
	{MAKEINTRESOURCE(IDS_PIPE),			COMPORT_PIPE},
#endif
	{MAKEINTRESOURCE(IDS_CFILE),		COMPORT_FILE},
	{MAKEINTRESOURCE(IDS_SPOOLER),		COMPORT_SPOOLER},
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
 * エミュレーションモードリスト
 */
static const CComboData::Entry s_emumode[] =
{
	{MAKEINTRESOURCE(IDC_COM1SPOOLEREMUMODE_RAW),		PRINT_EMU_MODE_RAW},
#ifdef SUPPORT_PRINT_PR201
	{MAKEINTRESOURCE(IDC_COM1SPOOLEREMUMODE_PR201),		PRINT_EMU_MODE_PR201},
#endif
#ifdef SUPPORT_PRINT_ESCP
	{MAKEINTRESOURCE(IDC_COM1SPOOLEREMUMODE_ESCP),		PRINT_EMU_MODE_ESCP},
#endif
};

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED) {
		SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM)TRUE, lpData);
	}
	return 0;
}

/**
 * コンストラクタ
 * @param[in] nCaption キャプション ID
 * @param[in] cm パラメータ
 * @param[in] cfg コンフィグ
 */
SerialOptComPage::SerialOptComPage(UINT nCaption, COMMNG cm, COMCFG& cfg, bool isSerial, bool isParallel)
	: CPropPageProc(IDD_SERIAL1, nCaption)
	, m_cm(cm)
	, m_cfg(cfg)
{
	if (isSerial && isParallel) {
		m_portlist = s_portALL;
		m_portlistlen = _countof(s_portALL);
	}
	else if (isParallel) {
		m_portlist = s_portLPT;
		m_portlistlen = _countof(s_portLPT);
	}
	else {
		m_portlist = s_portCOM;
		m_portlistlen = _countof(s_portCOM);
	}
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
	TCHAR numbuf[64];

	m_port.SubclassDlgItem(IDC_COM1PORT, this);
	m_port.Add(m_portlist, m_portlistlen);
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
	
#if defined(SUPPORT_WACOM_TABLET)
	m_pentabfa = np2oscfg.pentabfa;
#else
	m_pentabfa = 0;
#endif
	m_chkpentabfa.SubclassDlgItem(IDC_COM1PENTABFA, this);
	if(m_pentabfa)
		m_chkpentabfa.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	else
		m_chkpentabfa.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
	
	m_chkfixedspeed.SubclassDlgItem(IDC_COM1FSPEED, this);
	if(m_cm != cm_rs232c){
		m_chkfixedspeed.EnableWindow(FALSE);
		m_chkfixedspeed.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	}else{
		m_chkfixedspeed.SendMessage(BM_SETCHECK , m_cfg.fixedspeed ? BST_CHECKED : BST_UNCHECKED , 0);
	}
	
	m_chkDSRcheck.SubclassDlgItem(IDC_COM1DSRCHECK, this);
	if(m_cm != cm_rs232c){
		m_chkDSRcheck.EnableWindow(FALSE);
		m_chkDSRcheck.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	}else{
		m_chkDSRcheck.SendMessage(BM_SETCHECK , m_cfg.DSRcheck ? BST_CHECKED : BST_UNCHECKED , 0);
	}
	
#if defined(SUPPORT_NAMED_PIPE)
	m_pipename.SubclassDlgItem(IDC_COM1PIPENAME, this);
	m_pipename.SetWindowText(m_cfg.pipename);
	m_pipeserv.SubclassDlgItem(IDC_COM1PIPESERV, this);
	m_pipeserv.SetWindowText(m_cfg.pipeserv);
	{
		TCHAR pipecmd[MAX_PATH*3];
		_stprintf(pipecmd, _T("\\\\%s\\pipe\\%s"), m_cfg.pipeserv, m_cfg.pipename);
		SetDlgItemText(IDC_COM1STR32, pipecmd);
	}
#endif

	m_txtdirpath.SubclassDlgItem(IDC_COM1FILEPATH, this);
	m_txtdirpath.SetWindowText(m_cfg.dirpath); 
	m_nudfiletimeout.SubclassDlgItem(IDC_COM1FILETIMEOUT, this);
	_stprintf(numbuf, _T("%d"), m_cfg.fileTimeout);
	m_nudfiletimeout.SetWindowTextW(numbuf);

	m_cmbspname.SubclassDlgItem(IDC_COM1SPOOLERNAME, this);
	m_cmbspname.SetWindowText(m_cfg.spoolPrinterName);
	m_nudsptimeout.SubclassDlgItem(IDC_COM1SPOOLERTIMEOUT, this);
	_stprintf(numbuf, _T("%d"), m_cfg.spoolTimeout);
	m_chkspcfg.SubclassDlgItem(IDC_COM1SPOOLERCFG, this);
	CheckDlgButton(IDC_COM1SPOOLERCFG, (np2oscfg.prncfgpp) ? BST_CHECKED : BST_UNCHECKED);
	m_nudsptimeout.SetWindowTextW(numbuf);
	m_cmbspemumode.SetWindowText(m_cfg.spoolPrinterName);
	m_cmbspemumode.SubclassDlgItem(IDC_COM1SPOOLEREMUMODE, this);
	m_cmbspemumode.Add(s_emumode, _countof(s_emumode));
	m_cmbspemumode.SetCurItemData(m_cfg.spoolEmulation);
	m_nudspemu_dotsize.SubclassDlgItem(IDC_COM1SPOOLEREMU_DOTSIZE, this);
	_stprintf(numbuf, _T("%d"), m_cfg.spoolDotSize);
	m_nudspemu_dotsize.SetWindowTextW(numbuf);
	m_chkspemu_rectdot.SubclassDlgItem(IDC_COM1SPOOLEREMU_RECTDOT, this);
	CheckDlgButton(IDC_COM1SPOOLEREMU_RECTDOT, (m_cfg.spoolRectDot) ? BST_CHECKED : BST_UNCHECKED);
	m_nudspemu_offsetx.SubclassDlgItem(IDC_COM1SPOOLEREMU_OFFSETX, this);
	_stprintf(numbuf, _T("%d"), m_cfg.spoolOffsetXmm);
	m_nudspemu_offsetx.SetWindowTextW(numbuf);
	m_nudspemu_offsety.SubclassDlgItem(IDC_COM1SPOOLEREMU_OFFSETY, this);
	_stprintf(numbuf, _T("%d"), m_cfg.spoolOffsetYmm);
	m_nudspemu_offsety.SetWindowTextW(numbuf);
	m_nudspemu_scale.SubclassDlgItem(IDC_COM1SPOOLEREMU_SCALE, this);
	_stprintf(numbuf, _T("%d"), m_cfg.spoolScale);
	m_nudspemu_scale.SetWindowTextW(numbuf);
	if(m_cmbspname)
	{
		int indexsel = -1;
		int index = 0;
		DWORD needed = 0, returned = 0;

		m_cmbspname.Add(_T(""), index);
		index++;

		// まず必要サイズを取得
		EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, nullptr, 2, nullptr, 0, &needed, &returned);

		if (needed > 0) {
			BYTE* buffer = (BYTE*)malloc(needed);
			if (buffer) {
				if (EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, nullptr, 2, buffer, needed, &needed, &returned)) {
					PRINTER_INFO_2* pi2 = (PRINTER_INFO_2*)buffer;
					for (DWORD i = 0; i < returned; i++) {
						m_cmbspname.Add(pi2[i].pPrinterName, index);
						if (_tcscmp(pi2[i].pPrinterName, m_cfg.spoolPrinterName) == 0) {
							m_cmbspname.SetCurSel(index);
							indexsel = index;
						}
						index++;
					}
					if (indexsel == -1) {
						// 存在しないプリンタ名
						m_cmbspname.SetWindowText(m_cfg.spoolPrinterName);
					}
				}
				free(buffer);
			}
		}
	}

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
	
	const UINT8 cFSpeedEnable = (IsDlgButtonChecked(IDC_COM1FSPEED) != BST_UNCHECKED) ? 1 : 0;
	if (m_cfg.fixedspeed != cFSpeedEnable)
	{
		m_cfg.fixedspeed = cFSpeedEnable;
		nUpdated |= SYS_UPDATEOSCFG;
	}
	
	const UINT8 cDSRcheckEnable = (IsDlgButtonChecked(IDC_COM1DSRCHECK) != BST_UNCHECKED) ? 1 : 0;
	if (m_cfg.DSRcheck != cDSRcheckEnable)
	{
		m_cfg.DSRcheck = cDSRcheckEnable;
		nUpdated |= SYS_UPDATEOSCFG;
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
	
#if defined(SUPPORT_WACOM_TABLET)
	if (np2oscfg.pentabfa != m_pentabfa)
	{
		np2oscfg.pentabfa = m_pentabfa;
		nUpdated |= SYS_UPDATEOSCFG;
	}
#endif
	
#if defined(SUPPORT_NAMED_PIPE)
	TCHAR pipename[MAX_PATH];
	TCHAR pipeserv[MAX_PATH];
	GetDlgItemText(IDC_COM1PIPENAME, pipename, _countof(pipename));
	GetDlgItemText(IDC_COM1PIPESERV, pipeserv, _countof(pipeserv));
	if (milstr_cmp(m_cfg.pipename, pipename))
	{
		milstr_ncpy(m_cfg.pipename, pipename, _countof(m_cfg.pipename));
		nUpdated |= SYS_UPDATEOSCFG;
	}
	if (milstr_cmp(m_cfg.pipeserv, pipeserv))
	{
		milstr_ncpy(m_cfg.pipeserv, pipeserv, _countof(m_cfg.pipeserv));
		nUpdated |= SYS_UPDATEOSCFG;
	}
#endif

	TCHAR dirpath[MAX_PATH];
	GetDlgItemText(IDC_COM1FILEPATH, dirpath, _countof(dirpath));
	if (milstr_cmp(m_cfg.dirpath, dirpath))
	{
		milstr_ncpy(m_cfg.dirpath, dirpath, _countof(m_cfg.dirpath));
		nUpdated |= SYS_UPDATEOSCFG;
	}
	const UINT fileTimeout = GetDlgItemInt(IDC_COM1FILETIMEOUT, NULL, FALSE);
	if (m_cfg.fileTimeout != fileTimeout)
	{
		m_cfg.fileTimeout = fileTimeout;
		nUpdated |= SYS_UPDATEOSCFG;
	}
	const UINT8 spoolShowConfig = (IsDlgButtonChecked(IDC_COM1SPOOLERCFG) != BST_UNCHECKED) ? 1 : 0;
	if (np2oscfg.prncfgpp != spoolShowConfig)
	{
		np2oscfg.prncfgpp = spoolShowConfig;
		nUpdated |= SYS_UPDATEOSCFG;
	}

	TCHAR printername[MAX_PATH];
	GetDlgItemText(IDC_COM1SPOOLERNAME, printername, _countof(printername));
	if (milstr_cmp(m_cfg.spoolPrinterName, printername))
	{
		milstr_ncpy(m_cfg.spoolPrinterName, printername, _countof(m_cfg.spoolPrinterName));
		nUpdated |= SYS_UPDATEOSCFG;
	}
	const UINT spoolTimeout = GetDlgItemInt(IDC_COM1SPOOLERTIMEOUT, NULL, FALSE);
	if (m_cfg.spoolTimeout != spoolTimeout)
	{
		m_cfg.spoolTimeout = spoolTimeout;
		nUpdated |= SYS_UPDATEOSCFG;
	}
	const UINT32 spoolEmulation = m_cmbspemumode.GetCurItemData(PRINT_EMU_MODE_RAW);
	if (m_cfg.spoolEmulation != spoolEmulation)
	{
		m_cfg.spoolEmulation = spoolEmulation;
		nUpdated |= SYS_UPDATEOSCFG;
	}
	UINT spoolDotSize = GetDlgItemInt(IDC_COM1SPOOLEREMU_DOTSIZE, NULL, FALSE);
	if (m_cfg.spoolDotSize != spoolDotSize)
	{
		if (spoolDotSize < 50) spoolDotSize = 50;
		if (spoolDotSize > 200) spoolDotSize = 200;
		m_cfg.spoolDotSize = spoolDotSize;
		nUpdated |= SYS_UPDATEOSCFG;
	}
	const UINT8 spoolRectDot = (IsDlgButtonChecked(IDC_COM1SPOOLEREMU_RECTDOT) != BST_UNCHECKED) ? 1 : 0;
	if (m_cfg.spoolRectDot != spoolRectDot)
	{
		m_cfg.spoolRectDot = spoolRectDot;
		nUpdated |= SYS_UPDATEOSCFG;
	}
	UINT spoolOffsetX = GetDlgItemInt(IDC_COM1SPOOLEREMU_OFFSETX, NULL, TRUE);
	if (m_cfg.spoolOffsetXmm != spoolOffsetX)
	{
		m_cfg.spoolOffsetXmm = spoolOffsetX;
		nUpdated |= SYS_UPDATEOSCFG;
	}
	UINT spoolOffsetY = GetDlgItemInt(IDC_COM1SPOOLEREMU_OFFSETY, NULL, TRUE);
	if (m_cfg.spoolOffsetYmm != spoolOffsetY)
	{
		m_cfg.spoolOffsetYmm = spoolOffsetY;
		nUpdated |= SYS_UPDATEOSCFG;
	}
	UINT spoolScale = GetDlgItemInt(IDC_COM1SPOOLEREMU_SCALE, NULL, FALSE);
	if (m_cfg.spoolScale != spoolScale)
	{
		if (spoolDotSize < 1) spoolDotSize = 1;
		if (spoolDotSize > 200) spoolDotSize = 200;
		m_cfg.spoolScale = spoolScale;
		nUpdated |= SYS_UPDATEOSCFG;
	}
	if (nUpdated && cm_prt) {
		cm_prt->msg(cm_prt, COMMSG_REOPEN, (INTPTR)(&np2oscfg.lpt1));
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
			
		case IDC_COM1PENTABFA:
			m_pentabfa = (m_chkpentabfa.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
			return TRUE;
			
#if defined(SUPPORT_NAMED_PIPE)
		case IDC_COM1PIPENAME:
		case IDC_COM1PIPESERV:
		{
			TCHAR pipename[MAX_PATH];
			TCHAR pipeserv[MAX_PATH];
			TCHAR pipecmd[MAX_PATH*3];
			GetDlgItemText(IDC_COM1PIPENAME, pipename, _countof(pipename));
			GetDlgItemText(IDC_COM1PIPESERV, pipeserv, _countof(pipeserv));
			_stprintf(pipecmd, _T("\\\\%s\\pipe\\%s"), pipeserv, pipename);
			SetDlgItemText(IDC_COM1STR32, pipecmd);
			return TRUE;
		}
#endif

		case IDC_COM1FILEBROWSE:
		{
			OEMCHAR tmp[MAX_PATH];
			OEMCHAR name[MAX_PATH], dir[MAX_PATH];
			BROWSEINFO  binfo;
			LPITEMIDLIST idlist;
			TCHAR curdirpath[MAX_PATH];

			m_txtdirpath.GetWindowText(tmp, NELEMENTS(tmp));
			_tcscpy(curdirpath, tmp);

			binfo.hwndOwner = m_hWnd;
			binfo.pidlRoot = NULL;
			binfo.pszDisplayName = name;
			binfo.lpszTitle = OEMTEXT("");
			binfo.ulFlags = BIF_RETURNONLYFSDIRS;
			binfo.lpfn = BrowseCallbackProc;
			binfo.lParam = (LPARAM)(tmp);
			binfo.iImage = 0;

			if ((idlist = SHBrowseForFolder(&binfo)) != NULL) {
				SHGetPathFromIDList(idlist, dir);
				_tcscpy(tmp, dir);
				int tmppathlen = (int)_tcslen(tmp);
				if (tmp[tmppathlen - 1] == '\\') {
					tmp[tmppathlen - 1] = '\0';
				}
				if (_tcscmp(tmp, curdirpath) != 0) {
					m_txtdirpath.SetWindowText(tmp);
				}
				CoTaskMemFree(idlist);
			}
			return TRUE;
		}

		case IDC_COM1SPOOLEREMUMODE:
		{
			BOOL spemumode = m_cmbspemumode.GetCurItemData(PRINT_EMU_MODE_RAW) != PRINT_EMU_MODE_RAW;
			m_nudspemu_dotsize.EnableWindow(spemumode);
			m_chkspemu_rectdot.EnableWindow(spemumode);
			m_nudspemu_offsetx.EnableWindow(spemumode);
			m_nudspemu_offsety.EnableWindow(spemumode);
			m_nudspemu_scale.EnableWindow(spemumode);
			return TRUE;
		}
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
#if defined(SUPPORT_WACOM_TABLET)
	const bool bPentabShow = (nPort == COMPORT_TABLET);
#else
	const bool bPentabShow = false;
#endif
#if defined(SUPPORT_NAMED_PIPE)
	const bool bPipeShow = (nPort == COMPORT_PIPE);
#else
	const bool bPipeShow = false;
#endif
	const bool bParallelShow = ((nPort >= COMPORT_LPT1) && (nPort <= COMPORT_LPT4));
	const bool bFileShow = (nPort == COMPORT_FILE);
	const bool bSpoolerShow = (nPort == COMPORT_SPOOLER);

	// Physical serial port
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
	if(m_cm != cm_rs232c){
		m_chkfixedspeed.EnableWindow(FALSE);
	}else{
		m_chkfixedspeed.EnableWindow(bSerialShow ? TRUE : FALSE);
	}
	m_chkfixedspeed.ShowWindow(bSerialShow ? SW_SHOW : SW_HIDE);
	m_chkDSRcheck.ShowWindow(bSerialShow ? SW_SHOW : SW_HIDE);
	
	// Serial MIDI emulation
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
	
	// Serial pen tablet emulation
	m_chkpentabfa.EnableWindow(bPentabShow ? TRUE : FALSE);
	m_chkpentabfa.ShowWindow(bPentabShow ? SW_SHOW : SW_HIDE);
	
	// Named-pipe
	static const UINT pipe[] =
	{
		IDC_COM1PIPENAME, IDC_COM1PIPESERV,
		IDC_COM1STR30, IDC_COM1STR31, IDC_COM1STR32
	};
	for (UINT i = 0; i < _countof(pipe); i++)
	{
		CWndBase wnd = GetDlgItem(pipe[i]);
		wnd.EnableWindow(bPipeShow ? TRUE : FALSE);
		wnd.ShowWindow(bPipeShow ? SW_SHOW : SW_HIDE);
	}

	// Physical parallel port
    // nothing to do.

	// File dump
	static const UINT filedump[] =
	{
		IDC_COM1FILEPATH, IDC_COM1FILEBROWSE, IDC_COM1FILETIMEOUT,
		IDC_COM1FILESTR00, IDC_COM1FILESTR01, IDC_COM1FILESTR02,
	};
	for (UINT i = 0; i < _countof(filedump); i++)
	{
		CWndBase wnd = GetDlgItem(filedump[i]);
		wnd.EnableWindow(bFileShow ? TRUE : FALSE);
		wnd.ShowWindow(bFileShow ? SW_SHOW : SW_HIDE);
	}

	// Spooler
	static const UINT spooler[] =
	{
		IDC_COM1SPOOLERNAME, IDC_COM1SPOOLERTIMEOUT, IDC_COM1SPOOLERTIMEOUTSPIN, IDC_COM1SPOOLERCFG, IDC_COM1SPOOLEREMUMODE,
		IDC_COM1SPOOLEREMU_DOTSIZE, IDC_COM1SPOOLEREMU_RECTDOT,
		IDC_COM1SPOOLEREMU_OFFSETX, IDC_COM1SPOOLEREMU_OFFSETY, IDC_COM1SPOOLEREMU_SCALE,
		IDC_COM1SPOOLERSTR00, IDC_COM1SPOOLERSTR01, IDC_COM1SPOOLERSTR02, IDC_COM1SPOOLERSTR03, IDC_COM1SPOOLERSTR04,
		IDC_COM1SPOOLERSTR05, IDC_COM1SPOOLERSTR06, IDC_COM1SPOOLERSTR07,
	};
	for (UINT i = 0; i < _countof(spooler); i++)
	{
		CWndBase wnd = GetDlgItem(spooler[i]);
		wnd.EnableWindow(bSpoolerShow ? TRUE : FALSE);
		wnd.ShowWindow(bSpoolerShow ? SW_SHOW : SW_HIDE);
	}

	BOOL spemumode = m_cmbspemumode.GetCurItemData(PRINT_EMU_MODE_RAW) != PRINT_EMU_MODE_RAW;
	m_nudspemu_dotsize.EnableWindow(spemumode);
	m_chkspemu_rectdot.EnableWindow(spemumode);
	m_nudspemu_offsetx.EnableWindow(spemumode);
	m_nudspemu_offsety.EnableWindow(spemumode);
	m_nudspemu_scale.EnableWindow(spemumode);
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
	const bool allports = np2oscfg.allports;

	CPropSheetProc prop(IDS_SERIALOPTION, hwndParent);

	SerialOptComPage com1(IDS_SERIAL1, cm_rs232c, np2oscfg.com1, true || allports, false || allports);
	prop.AddPage(&com1);

	SerialOpt61Page pc9861;
	prop.AddPage(&pc9861);

	SerialOptComPage com2(IDS_PC9861B, cm_pc9861ch1, np2oscfg.com2, true || allports, false || allports);
	prop.AddPage(&com2);

	SerialOptComPage com3(IDS_PC9861C, cm_pc9861ch2, np2oscfg.com3, true || allports, false || allports);
	prop.AddPage(&com3);

	SerialOptComPage lpt1(IDS_PARALLEL, cm_prt, np2oscfg.lpt1, false || allports, true || allports);
	prop.AddPage(&lpt1);

	prop.m_psh.dwFlags |= PSH_NOAPPLYNOW | PSH_USEHICON | PSH_USECALLBACK;
	prop.m_psh.hIcon = LoadIcon(CWndProc::GetResourceHandle(), MAKEINTRESOURCE(IDI_ICON2));
	prop.m_psh.pfnCallback = np2class_propetysheet;
	prop.DoModal();

	::InvalidateRect(hwndParent, NULL, TRUE);
}
