/**
 * @file	d_disk.cpp
 * @brief	disk dialog
 */

#include "compiler.h"
#include "resource.h"
#include "dialog.h"
#include "c_combodata.h"
#include "dosio.h"
#include "np2.h"
#include "sysmng.h"
#include "misc/DlgProc.h"
#include "subwnd/toolwnd.h"
#include "pccore.h"
#include "common/strres.h"
#include "fdd/diskdrv.h"
#include "diskimage/fddfile.h"
#include "fdd/newdisk.h"
#include "fdd/sxsi.h"
#include "np2class.h"
#include "winfiledlg.h"
#if defined(_WINDOWS)
#include	<process.h>
#endif
#ifdef SUPPORT_NVL_IMAGES
extern "C" BOOL nvl_check();
#endif
#include "dialog/winfiledlg.h"


// 進捗表示用（実装酷すぎ･･･）
static int mt_progressvalue = 0;
static int mt_progressmax = 100;

/**
 * FDD 選択ダイアログ
 * @param[in] hWnd 親ウィンドウ
 * @param[in] drv ドライブ
 */
void dialog_changefdd(HWND hWnd, REG8 drv)
{
	if (drv < 4)
	{
		char szPath[MAX_PATH];
		char szImage[MAX_PATH];
		char szName[MAX_PATH];

		strcpy(szPath, fdd_diskname(drv));
		if ((szPath == NULL) || (szPath[0] == '\0'))
		{
			strcpy(szPath, fddfolder);
		}

		std::tstring rExt(LoadTString(IDS_FDDEXT));
		std::tstring rFilter(LoadTString(IDS_FDDFILTER));
		std::tstring rTitle(LoadTString(IDS_FDDTITLE));

		OPENFILENAMEW ofnw;
		if (WinFileDialogW(hWnd, &ofnw, WINFILEDIALOGW_MODE_GET1, szPath, szName, "", rTitle.c_str(), rFilter.c_str(), 8))
		{
			LPCTSTR lpImage = szPath;
			BOOL bReadOnly = (ofnw.Flags & OFN_READONLY) ? TRUE : FALSE;

			file_cpyname(fddfolder, szImage, _countof(fddfolder));
			sysmng_update(SYS_UPDATEOSCFG);
			diskdrv_setfdd(drv, lpImage, bReadOnly);
			toolwin_setfdd(drv, lpImage);
		}
	}
}

/**
 * HDD 選択ダイアログ
 * @param[in] hWnd 親ウィンドウ
 * @param[in] drv ドライブ
 */
void dialog_changehdd(HWND hWnd, REG8 drv)
{
	const UINT num = drv & 0x0f;

	UINT nTitle = 0;
	UINT nExt = 0;
	UINT nFilter = 0;
	UINT nIndex = 0;

	if (!(drv & 0x20))			// SASI/IDE
	{
#if defined(SUPPORT_IDEIO)
		if (num < 4)
		{
			if(sxsi_getdevtype(drv)!=SXSIDEV_CDROM)
			{
				nTitle = IDS_SASITITLE;
				nExt = IDS_HDDEXT;
				nFilter = IDS_HDDFILTER;
				//nIndex = 6;
				nIndex = 0;
			}
			else
			{
				nTitle = IDS_ISOTITLE;
				nExt = IDS_ISOEXT;
				nFilter = IDS_ISOFILTER;
				//nIndex = 7; // 3
				nIndex = 0;
			}
		}
#else
		if (num < 2)
		{
#if defined(SUPPORT_SASI)
			nTitle = IDS_SASITITLE;
#else
			nTitle = IDS_HDDTITLE;
#endif
			nExt = IDS_HDDEXT;
			nFilter = IDS_HDDFILTER;
			//nIndex = 6;//4;
			nIndex = 0;
		}
#endif
	}
#if defined(SUPPORT_SCSI)
	else						// SCSI
	{
		if (num < 4)
		{
			nTitle = IDS_SCSITITLE;
			nExt = IDS_SCSIEXT;
			nFilter = IDS_SCSIFILTER;
			//nIndex = 3;	
			nIndex = 0;
		}
	}
#endif	// defined(SUPPORT_SCSI)
	if (nExt == 0)
	{
		return;
	}

	char szPath[MAX_PATH];
	char szImage[MAX_PATH];
	char szName[MAX_PATH];
#ifdef SUPPORT_IDEIO
	if(np2cfg.idetype[drv]!=SXSIDEV_CDROM)
	{
#endif
		strcpy(szPath, diskdrv_getsxsi(drv));
#ifdef SUPPORT_IDEIO
	}
	else
	{
		strcpy(szPath, np2cfg.idecd[drv]);
	}
#endif
	if ((szPath == NULL) || (szPath[0] == '\0') || _tcsnicmp(szPath, OEMTEXT("\\\\.\\"), 4)==0)
	{
		if(sxsi_getfilename(drv)) {
			strcpy(szPath, sxsi_getfilename(drv));
		}
		if ((szPath == NULL) || (szPath[0] == '\0') || _tcsnicmp(szPath, OEMTEXT("\\\\.\\"), 4)==0)
		{
			if(sxsi_getdevtype(drv)!=SXSIDEV_CDROM)
			{
				strcpy(szPath, hddfolder);
			}
			else
			{
				strcpy(szPath, cdfolder);
			}
		}
	}
	
#ifdef SUPPORT_NVL_IMAGES
	if(nFilter == IDS_HDDFILTER && nvl_check()){
		nFilter = IDS_HDDFILTER_NVL;
		nIndex = 0;
	}
#endif

	std::tstring rExt(LoadTString(nExt));
	std::tstring rFilter(LoadTString(nFilter));
	std::tstring rTitle(LoadTString(nTitle));
	
	if(nIndex==0){ // All supported files（後ろから2番目）を自動選択
		int seppos = 0;
		int seppostmp;
		int sepcount = 0;
		// 区切り文字の数を数える
		while((seppostmp = (int)rFilter.find('|', seppos)) != std::string::npos){
			if(seppostmp == std::string::npos) break;
			seppos = seppostmp + 1;
			sepcount++;
		}
		if(rFilter.back()!='|'){
			sepcount++; // 末尾が|でなければあるものとする
		}
		if((sepcount / 2) - 1 > 0){
			nIndex = (sepcount / 2) - 1; // 最後がAll filesなので一つ前を選択
		}
	}

	OPENFILENAMEW ofnw;
	if (WinFileDialogW(hWnd, &ofnw, WINFILEDIALOGW_MODE_GET2, szPath, szName, "", rTitle.c_str(), rFilter.c_str(), nIndex))
	{
		strcpy(szImage, szPath);
#ifdef SUPPORT_IDEIO
		if(np2cfg.idetype[drv]!=SXSIDEV_CDROM)
		{
			file_cpyname(hddfolder, szImage, _countof(hddfolder));
		}
		else
		{
#endif
			file_cpyname(cdfolder, szImage, _countof(cdfolder));
#ifdef SUPPORT_IDEIO
		}
#endif
		sysmng_update(SYS_UPDATEOSCFG);
		diskdrv_setsxsi(drv, szImage);
	}
}


// ---- newdisk

/** デフォルト名 */
static const TCHAR str_newdisk[] = TEXT("newdisk");

/** HDD サイズ */
#ifdef SUPPORT_LARGE_HDD
static const UINT32 s_hddsizetbl[] = {20, 41, 65, 80, 127, 255, 511, 1023, 2047, 4095, 8191};
#else
static const UINT32 s_hddsizetbl[] = {20, 41, 65, 80, 127, 255, 511, 1023, 2047};
#endif

/** HDD サイズ */
static const UINT32 s_hddCtbl[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};
static const UINT32 s_hddHtbl[] = { 8, 15, 16};
static const UINT32 s_hddStbl[] = {17, 63, 255};
static const UINT32 s_hddSStbl[] = {256, 512};

/** SASI HDD */
static const UINT16 s_sasires[6] = 
{
	IDC_NEWSASI5MB, IDC_NEWSASI10MB,
	IDC_NEWSASI15MB, IDC_NEWSASI20MB,
	IDC_NEWSASI30MB, IDC_NEWSASI40MB
};

/**
 * @brief 新しいHDD
 */
class CNewHddDlg : public CDlgProc
{
public:
	/**
	 * コンストラクタ
	 * @param[in] hwndParent 親ウィンドウ
	 * @param[in] nHddMinSize 最小サイズ
	 * @param[in] nHddMaxSize 最大サイズ
	 */
	CNewHddDlg(HWND hwndParent, UINT32 nHddMinSize, UINT32 nHddMaxSize)
		: CDlgProc(IDD_NEWHDDDISK, hwndParent)
		, m_nHddSize(0)
		, m_nHddMinSize(nHddMinSize)
		, m_nHddMaxSize(nHddMaxSize)
		, m_advanced(0)
		, m_usedynsize(0)
		, m_HddC(0)
		, m_HddH(0)
		, m_HddS(0)
		, m_HddSS(0)
		, m_dynsize(0)
		, m_blank(0)
	{
	}

	/**
	 * デストラクタ
	 */
	virtual ~CNewHddDlg()
	{
	}

	/**
	 * サイズを返す
	 * @return サイズ
	 */
	UINT32 GetSize() const
	{
		return m_nHddSize;
	}
	
	/**
	 * シリンダ数を返す
	 * @return サイズ
	 */
	UINT GetC() const
	{
		return m_HddC;
	}
	
	/**
	 * ヘッド数を返す
	 * @return サイズ
	 */
	UINT GetH() const
	{
		return m_HddH;
	}
	
	/**
	 * セクタ数を返す
	 * @return サイズ
	 */
	UINT GetS() const
	{
		return m_HddS;
	}
	
	/**
	 * セクタサイズを返す
	 * @return サイズ
	 */
	UINT GetSS() const
	{
		return m_HddSS;
	}
	
	/**
	 * 詳細設定モードならtrue
	 * @return 詳細設定モード
	 */
	bool IsAdvancedMode() const
	{
		return (m_advanced != 0);
	}
	
	/**
	 * 容量可変モードならtrue
	 * @return 詳細設定モード
	 */
	bool IsDynamicDisk() const
	{
		return (m_dynsize != 0);
	}
	
	/**
	 * 空ディスク作成ならtrue
	 * @return 詳細設定モード
	 */
	bool IsBlankDisk() const
	{
		return (m_blank != 0);
	}
	
	/**
	 * 拡張設定を許可
	 * @return 
	 */
	void EnableAdvancedOptions()
	{
		m_advanced = 1;
		GetDlgItem(IDC_HDDADVANCED).EnableWindow(TRUE);
	}
	
	/**
	 * 動的リサイズディスクを許可(VHD用)
	 * @return 
	 */
	void EnableDynamicSize()
	{
		m_usedynsize = 1;
		GetDlgItem(IDC_HDDADVANCED_FIXSIZE).EnableWindow(TRUE);
		GetDlgItem(IDC_HDDADVANCED_DYNSIZE).EnableWindow(TRUE);
	}
	
	/**
	 * ディスクサイズからCHSを自動決定して表示する
	 * @return 
	 */
	void SetCHSfromSize()
	{
		//UINT16 C,H,S,SS;
		//UINT32 hddsize; // disk size(MB
		
		//hddsize = GetDlgItemInt(IDC_HDDSIZE, NULL, FALSE);

		if(m_nHddSize < 4351){
			m_HddH = 8;
			m_HddS = 17;
			m_HddSS = 512;
		}else if(m_nHddSize < 32255){
			m_HddH = 16;
			m_HddS = 63;
			m_HddSS = 512;
		}else{
			m_HddH = 16;
			m_HddS = 255;
			m_HddSS = 512;
		}
		m_HddC = (UINT32)((FILELEN)m_nHddSize * 1024 * 1024 / m_HddH / m_HddS / m_HddSS);

		SetDlgItemInt(IDC_HDDADVANCED_C, m_HddC, FALSE);
		SetDlgItemInt(IDC_HDDADVANCED_H, m_HddH, FALSE);
		SetDlgItemInt(IDC_HDDADVANCED_S, m_HddS, FALSE);
		SetDlgItemInt(IDC_HDDADVANCED_SS, m_HddSS, FALSE);
	}
	
	/**
	 * CHSからディスクサイズに変換して表示する
	 * @return 
	 */
	void SetSizefromCHS()
	{
		//UINT16 C,H,S,SS;
		//UINT32 hddsize; // disk size(MB)
		
		//m_HddC = GetDlgItemInt(IDC_HDDADVANCED_C, NULL, FALSE);
		//m_HddH = GetDlgItemInt(IDC_HDDADVANCED_H, NULL, FALSE);
		//m_HddS = GetDlgItemInt(IDC_HDDADVANCED_S, NULL, FALSE);
		//m_HddSS = GetDlgItemInt(IDC_HDDADVANCED_SS, NULL, FALSE);

		m_nHddSize = (UINT32)((FILELEN)m_HddC * m_HddH * m_HddS * m_HddSS / 1024 / 1024);
		
		SetDlgItemInt(IDC_HDDSIZE, m_nHddSize, FALSE);
	}
	
	/**
	 * アイテムの領域を得る
	 * @param[in] nID ID
	 * @param[out] rect 領域
	 */
	void GetDlgItemRect(UINT nID, RECT& rect)
	{
		CWndBase wnd = GetDlgItem(nID);
		wnd.GetWindowRect(&rect);
		::MapWindowPoints(HWND_DESKTOP, m_hWnd, reinterpret_cast<LPPOINT>(&rect), 2);
	}

protected:
	/**
	 * このメソッドは WM_INITDIALOG のメッセージに応答して呼び出されます
	 * @retval TRUE 最初のコントロールに入力フォーカスを設定
	 * @retval FALSE 既に設定済
	 */
	virtual BOOL OnInitDialog()
	{
		int hddsizetblcount = 0;
		
		while(hddsizetblcount<_countof(s_hddsizetbl) && s_hddsizetbl[hddsizetblcount] <= m_nHddMaxSize){
			hddsizetblcount++;
		}

		m_hddsize.SubclassDlgItem(IDC_HDDSIZE, this);
		m_hddsize.Add(s_hddsizetbl, hddsizetblcount);
		
		m_cmbhddC.SubclassDlgItem(IDC_HDDADVANCED_C, this);
		m_cmbhddC.Add(s_hddCtbl, _countof(s_hddCtbl));

		m_cmbhddH.SubclassDlgItem(IDC_HDDADVANCED_H, this);
		m_cmbhddH.Add(s_hddHtbl, _countof(s_hddHtbl));

		m_cmbhddS.SubclassDlgItem(IDC_HDDADVANCED_S, this);
		m_cmbhddS.Add(s_hddStbl, _countof(s_hddStbl));

		m_cmbhddSS.SubclassDlgItem(IDC_HDDADVANCED_SS, this);
		m_cmbhddSS.Add(s_hddSStbl, _countof(s_hddSStbl));

		TCHAR work[32];
		::wsprintf(work, TEXT("(%d-%dMB)"), m_nHddMinSize, m_nHddMaxSize);
		SetDlgItemText(IDC_HDDLIMIT, work);
		
		m_rdbfixsize.SubclassDlgItem(IDC_HDDADVANCED_FIXSIZE, this);
		m_rdbdynsize.SubclassDlgItem(IDC_HDDADVANCED_DYNSIZE, this);
		m_rdbfixsize.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
		
		m_chkblank.SubclassDlgItem(IDC_HDDADVANCED_BLANK, this);
		m_chkblank.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);

		RECT rect;
		GetWindowRect(&rect);
		m_szNewDisk.cx = rect.right - rect.left;
		m_szNewDisk.cy = rect.bottom - rect.top;

		RECT rectMore;
		GetDlgItemRect(IDC_HDDADVANCED, rectMore);
		RECT rectInfo;
		GetDlgItemRect(IDC_HDDADVANCED_SS, rectInfo);
		const int nHeight = m_szNewDisk.cy - (rectInfo.bottom - rectMore.bottom);

		CWndBase wndParent = GetParent();
		wndParent.GetClientRect(&rect);

		POINT pt;
		pt.x = (rect.right - rect.left - m_szNewDisk.cx) / 2;
		pt.y = (rect.bottom - rect.top - m_szNewDisk.cy) / 2;
		wndParent.ClientToScreen(&pt);
		np2class_move(m_hWnd, pt.x, pt.y, m_szNewDisk.cx, nHeight);

		if(m_advanced) EnableAdvancedOptions();
		if(m_usedynsize) EnableDynamicSize();
		
		m_hddsize.SetFocus();
		
		return FALSE;
	}

	/**
	 * ユーザーが OK のボタン (IDOK ID がのボタン) をクリックすると呼び出されます
	 */
	virtual void OnOK()
	{
		UINT nSize = GetDlgItemInt(IDC_HDDSIZE, NULL, FALSE);
		nSize = max(nSize, m_nHddMinSize);
		nSize = min(nSize, m_nHddMaxSize);
		m_nHddSize = nSize;
		CDlgProc::OnOK();
	}
	
	/**
	 * ユーザーがメニューの項目を選択したときに、フレームワークによって呼び出されます
	 * @param[in] wParam パラメタ
	 * @param[in] lParam パラメタ
	 * @retval TRUE アプリケーションがこのメッセージを処理した
	 */
	BOOL OnCommand(WPARAM wParam, LPARAM lParam)
	{
		switch (LOWORD(wParam))
		{
			case IDC_HDDADVANCED:
				RECT rect;
				GetWindowRect(&rect);
				np2class_move(m_hWnd, rect.left, rect.top, m_szNewDisk.cx, m_szNewDisk.cy);
				GetDlgItem(IDC_HDDADVANCED).EnableWindow(FALSE);
				GetDlgItem(IDC_HDDADVANCED_C).EnableWindow(TRUE);
				GetDlgItem(IDC_HDDADVANCED_H).EnableWindow(TRUE);
				GetDlgItem(IDC_HDDADVANCED_S).EnableWindow(TRUE);
				GetDlgItem(IDC_HDDADVANCED_SS).EnableWindow(TRUE);
				GetDlgItem(IDC_HDDADVANCED_C).SetFocus();
				if(m_dynsize){
					GetDlgItem(IDC_HDDADVANCED_FIXSIZE).EnableWindow(TRUE);
					GetDlgItem(IDC_HDDADVANCED_DYNSIZE).EnableWindow(TRUE);
				}
				m_advanced = 1;
				return TRUE;
				
			case IDC_HDDSIZE:
				if (HIWORD(wParam) == CBN_EDITCHANGE){
					m_nHddSize = GetDlgItemInt(IDC_HDDSIZE, NULL, FALSE);
					SetCHSfromSize();
					return TRUE;
				}else if(HIWORD(wParam) == CBN_SELCHANGE) {
					int selindex = m_hddsize.GetCurSel();
					if(selindex!=CB_ERR){
						m_nHddSize = s_hddsizetbl[m_hddsize.GetCurSel()];
						SetCHSfromSize();
					}
					return TRUE;
				}
				break;
				
			case IDC_HDDADVANCED_C:
				if (HIWORD(wParam) == CBN_EDITCHANGE){
					m_HddC = GetDlgItemInt(IDC_HDDADVANCED_C, NULL, FALSE);
					SetSizefromCHS();
					return TRUE;
				}else if(HIWORD(wParam) == CBN_SELCHANGE) {
					int selindex = m_cmbhddC.GetCurSel();
					if(selindex!=CB_ERR){
						m_HddC = s_hddCtbl[m_cmbhddC.GetCurSel()];
						SetSizefromCHS();
					}
					return TRUE;
				}
				break;
				
			case IDC_HDDADVANCED_H:
				if (HIWORD(wParam) == CBN_EDITCHANGE){
					m_HddH = GetDlgItemInt(IDC_HDDADVANCED_H, NULL, FALSE);
					SetSizefromCHS();
					return TRUE;
				}else if(HIWORD(wParam) == CBN_SELCHANGE) {
					int selindex = m_cmbhddH.GetCurSel();
					if(selindex!=CB_ERR){
						m_HddH = s_hddHtbl[m_cmbhddH.GetCurSel()];
						SetSizefromCHS();
					}
					return TRUE;
				}
				break;
				
			case IDC_HDDADVANCED_S:
				if (HIWORD(wParam) == CBN_EDITCHANGE){
					m_HddS = GetDlgItemInt(IDC_HDDADVANCED_S, NULL, FALSE);
					SetSizefromCHS();
					return TRUE;
				}else if(HIWORD(wParam) == CBN_SELCHANGE) {
					int selindex = m_cmbhddS.GetCurSel();
					if(selindex!=CB_ERR){
						m_HddS = s_hddStbl[m_cmbhddS.GetCurSel()];
						SetSizefromCHS();
					}
					return TRUE;
				}
				break;
				
			case IDC_HDDADVANCED_SS:
				if (HIWORD(wParam) == CBN_EDITCHANGE){
					m_HddSS = GetDlgItemInt(IDC_HDDADVANCED_SS, NULL, FALSE);
					SetSizefromCHS();
					return TRUE;
				}else if(HIWORD(wParam) == CBN_SELCHANGE) {
					int selindex = m_cmbhddSS.GetCurSel();
					if(selindex!=CB_ERR){
						m_HddSS = s_hddSStbl[m_cmbhddSS.GetCurSel()];
						SetSizefromCHS();
					}
					return TRUE;
				}
				break;
				
			case IDC_HDDADVANCED_FIXSIZE:
			case IDC_HDDADVANCED_DYNSIZE:
				m_dynsize = (m_rdbdynsize.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
				return TRUE;
				
			case IDC_HDDADVANCED_BLANK:
				m_blank = (m_chkblank.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
				return TRUE;
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
	LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
	{
		return CDlgProc::WindowProc(nMsg, wParam, lParam);
	}
private:
	CComboData m_hddsize;			/*!< HDD サイズ コントロール */
	UINT32 m_nHddSize;				/*!< HDD サイズ */
	UINT32 m_nHddMinSize;			/*!< 最小サイズ */
	UINT32 m_nHddMaxSize;			/*!< 最大サイズ */
	
	SIZE m_szNewDisk;				//!< ウィンドウのサイズ
	CWndProc m_btnAdvanced;			/*!< 詳細設定ボタン */
	UINT8 m_advanced;				/*!< 詳細設定許可フラグ */
	UINT32 m_HddC;					/*!< Cylinder */
	UINT16 m_HddH;					/*!< Head */
	UINT16 m_HddS;					/*!< Sector */
	UINT16 m_HddSS;					/*!< Sector Size(Bytes) */
	CComboData m_cmbhddC;			/*!< Cylinder値 コントロール */
	CComboData m_cmbhddH;			/*!< Head値 コントロール */
	CComboData m_cmbhddS;			/*!< Sector値 コントロール */
	CComboData m_cmbhddSS;			/*!< Sector Size値 コントロール */
	UINT8 m_usedynsize;				/*!< 動的割り当て許可フラグ */
	UINT8 m_dynsize;				/*!< 動的割り当てディスク（VHDのみ） */
	CWndProc m_rdbfixsize;			//!< FIXED
	CWndProc m_rdbdynsize;			//!< DYNAMIC
	UINT8 m_blank;					/*!< 空ディスク作成フラグ */
	CWndProc m_chkblank;			//!< BLANK
};



/**
 * @brief 新しいHDD
 */
class CNewSasiDlg : public CDlgProc
{
public:
	/**
	 * コンストラクタ
	 * @param[in] hwndParent 親ウィンドウ
	 */
	CNewSasiDlg(HWND hwndParent)
		: CDlgProc(IDD_NEWSASI, hwndParent)
		, m_nType(0)
	{
	}

	/**
	 * HDD タイプを得る
	 * @return HDD タイプ
	 */
	UINT GetType() const
	{
		return m_nType;
	}

protected:
	/**
	 * このメソッドは WM_INITDIALOG のメッセージに応答して呼び出されます
	 * @retval TRUE 最初のコントロールに入力フォーカスを設定
	 * @retval FALSE 既に設定済
	 */
	virtual BOOL OnInitDialog()
	{
		GetDlgItem(IDC_NEWSASI5MB).SetFocus();
		return FALSE;
	}

	/**
	 * ユーザーが OK のボタン (IDOK ID がのボタン) をクリックすると呼び出されます
	 */
	virtual void OnOK()
	{
		for (UINT i = 0; i < 6; i++)
		{
			if (IsDlgButtonChecked(s_sasires[i]) != BST_UNCHECKED)
			{
				m_nType = (i > 3) ? (i + 1) : i;
				CDlgProc::OnOK();
				break;
			}
		}
	}

private:
	UINT m_nType;			/*!< HDD タイプ */
};

/**
 * @brief 新しいFDD
 */
class CNewFddDlg : public CDlgProc
{
public:
	/**
	 * コンストラクタ
	 * @param[in] hwndParent 親ウィンドウ
	 */
	CNewFddDlg(HWND hwndParent)
		: CDlgProc((np2cfg.usefd144) ? IDD_NEWDISK2 : IDD_NEWDISK, hwndParent)
		, m_nFdType(DISKTYPE_2HD << 4)
	{
	}

	/**
	 * タイプを得る
	 * @return タイプ
	 */
	UINT8 GetType() const
	{
		return m_nFdType;
	}

	/**
	 * ラベルを得る
	 * @return ラベル
	 */
	LPCTSTR GetLabel() const
	{
		return m_szDiskLabel;
	}

protected:
	/**
	 * このメソッドは WM_INITDIALOG のメッセージに応答して呼び出されます
	 * @retval TRUE 最初のコントロールに入力フォーカスを設定
	 * @retval FALSE 既に設定済
	 */
	virtual BOOL OnInitDialog()
	{
		UINT res;
		switch (m_nFdType)
		{
			case (DISKTYPE_2DD << 4):
				res = IDC_MAKE2DD;
				break;

			case (DISKTYPE_2HD << 4):
				res = IDC_MAKE2HD;
				break;

			default:
				res = IDC_MAKE144;
				break;
		}
		CheckDlgButton(res, BST_CHECKED);
		GetDlgItem(IDC_DISKLABEL).SetFocus();
		return FALSE;
	}

	/**
	 * ユーザーが OK のボタン (IDOK ID がのボタン) をクリックすると呼び出されます
	 */
	virtual void OnOK()
	{
		GetDlgItemText(IDC_DISKLABEL, m_szDiskLabel, _countof(m_szDiskLabel));
		if (milstr_kanji1st(m_szDiskLabel, _countof(m_szDiskLabel) - 1))
		{
			m_szDiskLabel[_countof(m_szDiskLabel) - 1] = '\0';
		}
		if (IsDlgButtonChecked(IDC_MAKE2DD) != BST_UNCHECKED)
		{
			m_nFdType = (DISKTYPE_2DD << 4);
		}
		else if (IsDlgButtonChecked(IDC_MAKE2HD) != BST_UNCHECKED)
		{
			m_nFdType = (DISKTYPE_2HD << 4);
		}
		else
		{
			m_nFdType = (DISKTYPE_2HD << 4) + 1;
		}
		CDlgProc::OnOK();
	}

private:
	UINT m_nFdType;					/*!< タイプ */
	TCHAR m_szDiskLabel[16 + 1];	/*!< ラベル */
};

/**
 * @brief HDD作成進捗
 */
class CNewHddDlgProg : public CDlgProc
{
public:
	/**
	 * コンストラクタ
	 * @param[in] hwndParent 親ウィンドウ
	 * @param[in] nProgMax 進捗最大値
	 * @param[in] nProgValue 進捗現在値
	 */
	CNewHddDlgProg(HWND hwndParent, UINT32 nProgMax, UINT32 nProgValue)
		: CDlgProc(IDD_NEWHDDPROC, hwndParent)
	{
		SetProgressMax(nProgMax);
		SetProgressMax(nProgValue);
	}

	/**
	 * デストラクタ
	 */
	virtual ~CNewHddDlgProg()
	{
	}

	/**
	 * プログレスバー最大値を設定
	 * @return サイズ
	 */
	void SetProgressMax(UINT32 value) const
	{
		::SendMessage(GetDlgItem(IDC_HDDCREATE_PROGRESS), PBM_SETRANGE, (WPARAM)0, MAKELPARAM(0, value));
	}
	
	/**
	 * プログレスバー現在値を設定
	 * @return サイズ
	 */
	void SetProgressValue(UINT32 value) const
	{
		::SendMessage(GetDlgItem(IDC_HDDCREATE_PROGRESS), PBM_SETPOS, (WPARAM)value, 0);
	}
	
protected:
	/**
	 * このメソッドは WM_INITDIALOG のメッセージに応答して呼び出されます
	 * @retval TRUE 最初のコントロールに入力フォーカスを設定
	 * @retval FALSE 既に設定済
	 */
	virtual BOOL OnInitDialog()
	{
		SetTimer(this->m_hWnd, 1, 500, NULL);
		return TRUE;
	}

	/**
	 * ユーザーがメニューの項目を選択したときに、フレームワークによって呼び出されます
	 * @param[in] wParam パラメタ
	 * @param[in] lParam パラメタ
	 * @retval TRUE アプリケーションがこのメッセージを処理した
	 */
	BOOL OnCommand(WPARAM wParam, LPARAM lParam)
	{
		return FALSE;
	}

	/**
	 * CWndProc オブジェクトの Windows プロシージャ (WindowProc) が用意されています
	 * @param[in] nMsg 処理される Windows メッセージを指定します
	 * @param[in] wParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
	 * @param[in] lParam メッセージの処理で使う付加情報を提供します。このパラメータの値はメッセージに依存します
	 * @return メッセージに依存する値を返します
	 */
	LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (nMsg) {
		case WM_DESTROY:
			KillTimer(this->m_hWnd, 1);
			break;
		case WM_TIMER:
			SetProgressValue(mt_progressvalue);
			SetProgressMax(mt_progressmax);
			if(mt_progressvalue >= mt_progressmax){
				// 処理終わり
				CDlgProc::OnOK();
			}
			return 0;
		}
		return CDlgProc::WindowProc(nMsg, wParam, lParam);
	}
private:
};

static HANDLE	newdisk_hThread = NULL; // ディスク作成用スレッド
static int _mt_cancel = 0;
static int _mt_dyndisk = 0;
static int _mt_blank = 0;
static TCHAR _mt_lpPath[MAX_PATH] = {0};
static UINT32 _mt_diskSize = 0;
static UINT32 _mt_diskC = 0;
static UINT16 _mt_diskH = 0;
static UINT16 _mt_diskS = 0;
static UINT16 _mt_diskSS = 0;

static unsigned int __stdcall newdisk_ThreadFunc(LPVOID vdParam)
{
	LPCTSTR lpPath = _mt_lpPath;
	LPCTSTR ext = file_getext(lpPath);
	if (!file_cmpname(ext, str_thd))
	{
		newdisk_thd(lpPath, _mt_diskSize);
	}
	else if (!file_cmpname(ext, str_nhd))
	{
		if(_mt_diskSize){
			// 全容量指定モード
			newdisk_nhd_ex(lpPath, _mt_diskSize, _mt_blank, &mt_progressvalue, &_mt_cancel);
		}else{
			// CHS指定モード
			newdisk_nhd_ex_CHS(lpPath, _mt_diskC, _mt_diskH, _mt_diskS, _mt_diskSS, _mt_blank, &mt_progressvalue, &_mt_cancel);
		}
	}
	else if (!file_cmpname(ext, str_hdi))
	{
		newdisk_hdi(lpPath, _mt_diskSize);
	}
#if defined(SUPPORT_SCSI)
	else if (!file_cmpname(ext, str_hdd))
	{
		newdisk_vhd(lpPath, _mt_diskSize);
	}
	else if (!file_cmpname(ext, str_hdn))
	{
		newdisk_hdn(lpPath, _mt_diskSize);
	}
#endif
#ifdef SUPPORT_VPCVHD
	else if (!file_cmpname(ext, str_vhd))
	{
		if(_mt_diskSize){
			// 全容量指定モード
			newdisk_vpcvhd_ex(lpPath, _mt_diskSize, _mt_dyndisk, _mt_blank, &mt_progressvalue, &_mt_cancel);
		}else{
			// CHS指定モード
			newdisk_vpcvhd_ex_CHS(lpPath, _mt_diskC, _mt_diskH, _mt_diskS, _mt_diskSS, _mt_dyndisk, _mt_blank, &mt_progressvalue, &_mt_cancel);
		}
	}
#endif
	mt_progressvalue = mt_progressmax;
	return 0;
}

/**
 * 新規ディスク作成 ダイアログ
 * @param[in] hWnd 親ウィンドウ
 */
void dialog_newdisk_ex(HWND hWnd, int mode)
{
	unsigned int dwID;
	char szPath[MAX_PATH];
	char szName[MAX_PATH];
	std::tstring rTitle;
	std::tstring rDefExt;
	std::tstring rFilter;
	if(mode == NEWDISKMODE_HD){
		file_cpyname(szPath, hddfolder, _countof(szPath));
		file_cutname(szPath);
		file_catname(szPath, str_newdisk, _countof(szPath));
		rTitle = std::tstring(LoadTString(IDS_NEWDISKTITLE));
		rDefExt = std::tstring(OEMTEXT("nhd"));
#if defined(SUPPORT_SCSI)
		rFilter = std::tstring(LoadTString(IDS_NEWDISKHDFILTER));
#else	// defined(SUPPORT_SCSI)
		rFilter = std::tstring(LoadTString(IDS_NEWDISKHDFILTER2));
#endif	// defined(SUPPORT_SCSI)
	}else if(mode == NEWDISKMODE_FD){
		file_cpyname(szPath, fddfolder, _countof(szPath));
		file_cutname(szPath);
		file_catname(szPath, str_newdisk, _countof(szPath));
		rTitle = std::tstring(LoadTString(IDS_NEWDISKTITLE));
		rDefExt = std::tstring(LoadTString(IDS_NEWDISKEXT));
		rFilter = std::tstring(LoadTString(IDS_NEWDISKFDFILTER));
	}else{
		file_cpyname(szPath, fddfolder, _countof(szPath));
		file_cutname(szPath);
		file_catname(szPath, str_newdisk, _countof(szPath));
		rTitle = std::tstring(LoadTString(IDS_NEWDISKTITLE));
		rDefExt = std::tstring(LoadTString(IDS_NEWDISKEXT));
#if defined(SUPPORT_SCSI)
		rFilter = std::tstring(LoadTString(IDS_NEWDISKFILTER));
#else	// defined(SUPPORT_SCSI)
		rFilter = std::tstring(LoadTString(IDS_NEWDISKFILTER2));
#endif	// defined(SUPPORT_SCSI)
	}

	OPENFILENAMEW ofnw;
	if (WinFileDialogW(hWnd, &ofnw, WINFILEDIALOGW_MODE_SET, szPath, szName, rDefExt.c_str(), rTitle.c_str(), rFilter.c_str(), 0))
	{
		return;
	}

	LPCTSTR lpPath = szPath;
	LPCTSTR ext = file_getext(lpPath);
	if (!file_cmpname(ext, str_thd))
	{
		CNewHddDlg dlg(hWnd, 5, 256);
		if (dlg.DoModal() == IDOK)
		{
			newdisk_thd(lpPath, dlg.GetSize());
		}
	}
	else if (!file_cmpname(ext, str_nhd))
	{
		CNewHddDlg dlg(hWnd, 5, np2oscfg.makelhdd ? NHD_MAXSIZE2 : NHD_MAXSIZE);
		dlg.EnableAdvancedOptions();
		if (dlg.DoModal() == IDOK)
		{
			if(dlg.IsAdvancedMode()){
				_mt_diskSize = 0;
				_mt_diskC = dlg.GetC();
				_mt_diskH = dlg.GetH();
				_mt_diskS = dlg.GetS();
				_mt_diskSS = dlg.GetSS();
			}else{
				_mt_diskSize = dlg.GetSize();
			}
			_mt_blank = dlg.IsBlankDisk();
			_mt_dyndisk = 0;
			_mt_cancel = 0;
			mt_progressvalue = 0;
			mt_progressmax = 100;
			_tcscpy(_mt_lpPath, lpPath);
			newdisk_hThread = (HANDLE)_beginthreadex(NULL , 0 , newdisk_ThreadFunc  , NULL , 0 , &dwID);
			CNewHddDlgProg dlg2(hWnd, mt_progressmax, mt_progressvalue);
			if (dlg2.DoModal() != IDOK)
			{
				_mt_cancel = 1;
				WaitForSingleObject(newdisk_ThreadFunc, INFINITE);
				CloseHandle(newdisk_ThreadFunc);
			}
			_mt_cancel = 1;
		}
	}
	else if (!file_cmpname(ext, str_hdi))
	{
		CNewSasiDlg dlg(hWnd);
		if (dlg.DoModal() == IDOK)
		{
			newdisk_hdi(lpPath, dlg.GetType());
		}
	}
#if defined(SUPPORT_SCSI)
	else if (!file_cmpname(ext, str_hdd))
	{
		CNewHddDlg dlg(hWnd, 2, 512);
		if (dlg.DoModal() == IDOK)
		{
			newdisk_vhd(lpPath, dlg.GetSize());
		}
	}
	else if (!file_cmpname(ext, str_hdn))
	{
		CNewHddDlg dlg(hWnd, 2, 6399);
		if (dlg.DoModal() == IDOK)
		{
			newdisk_hdn(lpPath, dlg.GetSize());
		}
	}
#endif
#ifdef SUPPORT_VPCVHD
	else if (!file_cmpname(ext, str_vhd))
	{
		CNewHddDlg dlg(hWnd, 5, np2oscfg.makelhdd ? NHD_MAXSIZE2 : NHD_MAXSIZE);
		dlg.EnableAdvancedOptions();
		dlg.EnableDynamicSize();
		if (dlg.DoModal() == IDOK)
		{
			if(dlg.IsAdvancedMode()){
				_mt_diskSize = 0;
				_mt_diskC = dlg.GetC();
				_mt_diskH = dlg.GetH();
				_mt_diskS = dlg.GetS();
				_mt_diskSS = dlg.GetSS();
			}else{
				_mt_diskSize = dlg.GetSize();
			}
			_mt_blank = dlg.IsBlankDisk();
			_mt_dyndisk = dlg.IsDynamicDisk();
			_mt_cancel = 0;
			mt_progressvalue = 0;
			mt_progressmax = 100;
			_tcscpy(_mt_lpPath, lpPath);
			newdisk_hThread = (HANDLE)_beginthreadex(NULL , 0 , newdisk_ThreadFunc  , NULL , 0 , &dwID);
			CNewHddDlgProg dlg2(hWnd, mt_progressmax, mt_progressvalue);
			if (dlg2.DoModal() != IDOK)
			{
				_mt_cancel = 1;
				WaitForSingleObject(newdisk_ThreadFunc, INFINITE);
				CloseHandle(newdisk_ThreadFunc);
			}
			_mt_cancel = 1;
		}
	}
#endif
	else if ((!file_cmpname(ext, str_d88)) ||
			(!file_cmpname(ext, str_d98)) ||
			(!file_cmpname(ext, str_88d)) ||
			(!file_cmpname(ext, str_98d)))
	{
		CNewFddDlg dlg(hWnd);
		if (dlg.DoModal()  == IDOK)
		{
			newdisk_fdd(lpPath, dlg.GetType(), dlg.GetLabel());
		}
	}
	else if (!file_cmpname(ext, str_hdm))
	{
		newdisk_123mb_fdd(lpPath);
	}
	else if (!file_cmpname(ext, str_hd4))
	{
		newdisk_144mb_fdd(lpPath);
	}
}

/**
 * 新規ディスク作成 ダイアログ
 * @param[in] hWnd 親ウィンドウ
 */
void dialog_newdisk(HWND hWnd)
{
	dialog_newdisk_ex(hWnd, NEWDISKMODE_ALL);
}

