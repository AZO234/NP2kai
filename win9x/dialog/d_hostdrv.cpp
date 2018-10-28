/**
 * @file	d_hostdrv.cpp
 * @brief	HOSTDRV 設定ダイアログ
 */

#include "compiler.h"
#include "resource.h"
#include "dialog.h"
#include "c_combodata.h"
#include "np2.h"
#include "commng.h"
#include "sysmng.h"
#include "misc/DlgProc.h"
#include "pccore.h"
#include "common/strres.h"
#include "hostdrv.h"
#include "ini.h"

#include <shlobj.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

void hostdrv_readini();
void hostdrv_writeini();
void hostdrv_setcurrentpath(const TCHAR* newpath);

static TCHAR s_hostdrvdir[10][MAX_PATH] = {0};

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if(uMsg==BFFM_INITIALIZED){
        SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM)TRUE, lpData);
    }
    return 0;
}

/**
 * @brief HOSTDRV 設定ダイアログ
 * @param[in] hwndParent 親ウィンドウ
 */
class CHostdrvDlg : public CDlgProc
{
public:
	CHostdrvDlg(HWND hwndParent);

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	UINT8 m_hdrvenable;			//!< 有効
	TCHAR m_hdrvroot[MAX_PATH];	//!< 共有ディレクトリ
	UINT8 m_hdrvacc;			//!< アクセス権限
	CWndProc m_chkenabled;		//!< Enabled
	CComboData m_cmbdir;			//!< Shared Directory
	CWndProc m_chkread;			//!< Permission: Read
	CWndProc m_chkwrite;		//!< Permission: Write
	CWndProc m_chkdelete;		//!< Permission: Delete
};

/**
 * コンストラクタ
 * @param[in] hwndParent 親ウィンドウ
 */
CHostdrvDlg::CHostdrvDlg(HWND hwndParent)
	: CDlgProc(IDD_HOSTDRV, hwndParent)
{
}

/**
 * このメソッドは WM_INITDIALOG のメッセージに応答して呼び出されます
 * @retval TRUE 最初のコントロールに入力フォーカスを設定
 * @retval FALSE 既に設定済
 */
BOOL CHostdrvDlg::OnInitDialog()
{
	_tcscpy(m_hdrvroot, np2cfg.hdrvroot);
	hostdrv_setcurrentpath(m_hdrvroot);
	m_hdrvacc = np2cfg.hdrvacc;
	m_hdrvenable = np2cfg.hdrvenable;
	
	m_chkenabled.SubclassDlgItem(IDC_HOSTDRVENABLE, this);
	if(m_hdrvenable)
		m_chkenabled.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	else
		m_chkenabled.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
	
	m_cmbdir.SubclassDlgItem(IDC_HOSTDRVDIR, this);
	for(int i=0;i<_countof(s_hostdrvdir);i++){
		if(s_hostdrvdir[i][0]==0) break;
		m_cmbdir.Add(s_hostdrvdir[i], i);
	}
	m_cmbdir.SetWindowText(m_hdrvroot);
	
	m_chkread.SubclassDlgItem(IDC_HOSTDRVREAD, this);
	if(m_hdrvacc & HDFMODE_READ)
		m_chkread.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	else
		m_chkread.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
	
	m_chkwrite.SubclassDlgItem(IDC_HOSTDRVWRITE, this);
	if(m_hdrvacc & HDFMODE_WRITE)
		m_chkwrite.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	else
		m_chkwrite.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);

	m_chkdelete.SubclassDlgItem(IDC_HOSTDRVDELETE, this);
	if(m_hdrvacc & HDFMODE_DELETE)
		m_chkdelete.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	else
		m_chkdelete.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);

	m_cmbdir.SetFocus();

	return FALSE;
}

/**
 * ユーザーが OK のボタン (IDOK ID がのボタン) をクリックすると呼び出されます
 */
void CHostdrvDlg::OnOK()
{
	UINT update = 0;
	//UINT32 valtmp;
	//TCHAR numbuf[31];
	
	hostdrv_setcurrentpath(m_hdrvroot);
	if (m_hdrvenable!=np2cfg.hdrvenable || _tcscmp(np2cfg.hdrvroot, m_hdrvroot)!=0 || m_hdrvacc!=np2cfg.hdrvacc)
	{
		np2cfg.hdrvenable = m_hdrvenable;
		_tcscpy(np2cfg.hdrvroot, m_hdrvroot);
		np2cfg.hdrvacc = m_hdrvacc;
		update |= SYS_UPDATECFG;
	}

	sysmng_update(update);

	CDlgProc::OnOK();
}

/**
 * ユーザーがメニューの項目を選択したときに、フレームワークによって呼び出されます
 * @param[in] wParam パラメタ
 * @param[in] lParam パラメタ
 * @retval TRUE アプリケーションがこのメッセージを処理した
 */
BOOL CHostdrvDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	OEMCHAR hdrvroottmp[MAX_PATH];
	int hdrvpathlen;
	switch (LOWORD(wParam))
	{
		case IDC_HOSTDRVENABLE:
			m_hdrvenable = (UINT8)m_chkenabled.SendMessage(BM_GETCHECK , 0 , 0);
			return TRUE;

		case IDC_HOSTDRVDIR:
			hdrvroottmp[0] = 0;
			if (HIWORD(wParam) == CBN_EDITCHANGE){
				m_cmbdir.GetWindowText(hdrvroottmp, NELEMENTS(hdrvroottmp));
			}else if(HIWORD(wParam) == CBN_SELCHANGE) {
				int selindex = m_cmbdir.GetCurSel();
				if(selindex!=CB_ERR){
					_tcscpy(hdrvroottmp, s_hostdrvdir[selindex]);
				}else{
					break;
				}
			}
			if(hdrvroottmp[0]){
				hdrvpathlen = (int)_tcslen(hdrvroottmp);
				if(hdrvroottmp[hdrvpathlen-1]=='\\'){
					hdrvroottmp[hdrvpathlen-1] = '\0';
				}
				if(_tcscmp(hdrvroottmp, m_hdrvroot)!=0){
					_tcscpy(m_hdrvroot, hdrvroottmp);
					m_hdrvacc = (m_hdrvacc & ~(HDFMODE_WRITE|HDFMODE_DELETE));
					m_chkwrite.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
					m_chkdelete.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
					//hostdrv_setcurrentpath(m_hdrvroot);
					//m_cmbdir.ResetContent();
					//for(int i=0;i<_countof(s_hostdrvdir);i++){
					//	if(s_hostdrvdir[i][0]==0) break;
					//	m_cmbdir.Add(s_hostdrvdir[i], i);
					//}
					//m_cmbdir.SetWindowText(hdrvroottmp);
				}
				return TRUE;
				}
			break;

		case IDC_HOSTDRVBROWSE:
			{
				OEMCHAR name[MAX_PATH],dir[MAX_PATH];
				BROWSEINFO  binfo;
				LPITEMIDLIST idlist;
    
				m_cmbdir.GetWindowText(hdrvroottmp, NELEMENTS(hdrvroottmp));

				binfo.hwndOwner = g_hWndMain;
				binfo.pidlRoot = NULL;
				binfo.pszDisplayName = name;
				binfo.lpszTitle = OEMTEXT("");
				binfo.ulFlags = BIF_RETURNONLYFSDIRS; 
				binfo.lpfn = BrowseCallbackProc;              
				binfo.lParam = (LPARAM)(hdrvroottmp);               
				binfo.iImage = 0;
    
				if((idlist = SHBrowseForFolder(&binfo))!=NULL){
					SHGetPathFromIDList(idlist, dir);
					_tcscpy(hdrvroottmp, dir);
					hdrvpathlen = (int)_tcslen(hdrvroottmp);
					if(hdrvroottmp[hdrvpathlen-1]=='\\'){
						hdrvroottmp[hdrvpathlen-1] = '\0';
					}
					if(_tcscmp(hdrvroottmp, m_hdrvroot)!=0){
						_tcscpy(m_hdrvroot, hdrvroottmp);
						m_hdrvacc = (m_hdrvacc & ~(HDFMODE_WRITE|HDFMODE_DELETE));
						m_chkwrite.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
						m_chkdelete.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
						hostdrv_setcurrentpath(m_hdrvroot);
						m_cmbdir.ResetContent();
						for(int i=0;i<_countof(s_hostdrvdir);i++){
							if(s_hostdrvdir[i][0]==0) break;
							m_cmbdir.Add(s_hostdrvdir[i], i);
						}
						m_cmbdir.SetWindowText(hdrvroottmp);
					}
					CoTaskMemFree(idlist);
				}
			}
			return TRUE;

		case IDC_HOSTDRVREAD:
			m_hdrvacc = (m_hdrvacc & ~HDFMODE_READ);
			m_hdrvacc |= (m_chkread.SendMessage(BM_GETCHECK , 0 , 0) ? HDFMODE_READ : 0);
			return TRUE;

		case IDC_HOSTDRVWRITE:
			m_hdrvacc = (m_hdrvacc & ~HDFMODE_WRITE);
			m_hdrvacc |= (m_chkwrite.SendMessage(BM_GETCHECK , 0 , 0) ? HDFMODE_WRITE : 0);
			return TRUE;

		case IDC_HOSTDRVDELETE:
			m_hdrvacc = (m_hdrvacc & ~HDFMODE_DELETE);
			m_hdrvacc |= (m_chkdelete.SendMessage(BM_GETCHECK , 0 , 0) ? HDFMODE_DELETE : 0);
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
LRESULT CHostdrvDlg::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return CDlgProc::WindowProc(nMsg, wParam, lParam);
}

/**
 * コンフィグ ダイアログ
 * @param[in] hwndParent 親ウィンドウ
 */
void dialog_hostdrvopt(HWND hwndParent)
{
	CHostdrvDlg dlg(hwndParent);
	dlg.DoModal();
}


//! タイトル
static const TCHAR s_hostdrvapp[] = TEXT("NP2 hostdrv");

/**
 * 設定
 */
static const PFTBL s_hostdrvini[] =
{
	PFSTR("HOSTDRV0", PFTYPE_STR,		s_hostdrvdir[0]),
	PFSTR("HOSTDRV1", PFTYPE_STR,		s_hostdrvdir[1]),
	PFSTR("HOSTDRV2", PFTYPE_STR,		s_hostdrvdir[2]),
	PFSTR("HOSTDRV3", PFTYPE_STR,		s_hostdrvdir[3]),
	PFSTR("HOSTDRV4", PFTYPE_STR,		s_hostdrvdir[4]),
	PFSTR("HOSTDRV5", PFTYPE_STR,		s_hostdrvdir[5]),
	PFSTR("HOSTDRV6", PFTYPE_STR,		s_hostdrvdir[6]),
	PFSTR("HOSTDRV7", PFTYPE_STR,		s_hostdrvdir[7]),
	PFSTR("HOSTDRV8", PFTYPE_STR,		s_hostdrvdir[8]),
	PFSTR("HOSTDRV9", PFTYPE_STR,		s_hostdrvdir[9])
};

/**
 * 設定読み込み
 */
void hostdrv_readini()
{
	ZeroMemory(&s_hostdrvdir, sizeof(s_hostdrvdir));

	OEMCHAR szPath[MAX_PATH];
	initgetfile(szPath, _countof(szPath));
	ini_read(szPath, s_hostdrvapp, s_hostdrvini, _countof(s_hostdrvini));
}

/**
 * 設定書き込み
 */
void hostdrv_writeini()
{
	if(!np2oscfg.readonly){
		TCHAR szPath[MAX_PATH];
		initgetfile(szPath, _countof(szPath));
		ini_write(szPath, s_hostdrvapp, s_hostdrvini, _countof(s_hostdrvini));
	}
}

/**
 * 指定したパスを最上位に
 */
void hostdrv_setcurrentpath(const TCHAR* newpath)
{
	int i;
	if(!newpath[0]) 
		return;
	for(i=0;i<_countof(s_hostdrvini);i++){
		if(_tcsicmp(s_hostdrvdir[i], newpath)==0){
			i++;
			break;
		}
	}
	for(i=i-1;i>=1;i--){
		_tcscpy(s_hostdrvdir[i], s_hostdrvdir[i-1]);
	}
	_tcscpy(s_hostdrvdir[0], newpath);
}
