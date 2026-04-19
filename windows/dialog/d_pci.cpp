/**
 * @file	d_pci.cpp
 * @brief	PCI 設定ダイアログ
 */

#include "compiler.h"
#include "resource.h"
#include "strres.h"
#include "dialog.h"
#include "c_combodata.h"
#include "np2class.h"
#include "dosio.h"
#include "joymng.h"
#include "np2.h"
#include "sysmng.h"
#include "misc\PropProc.h"
#include "pccore.h"
#include "iocore.h"

#if defined(SUPPORT_PCI)

/**
 * @brief ウィンドウアクセラレータ基本設定ページ
 * @param[in] hwndParent 親ウィンドウ
 */
class CPCIPage : public CPropPageProc
{
public:
	CPCIPage();
	virtual ~CPCIPage();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	UINT8 m_enable;				//!< 有効
	UINT8 m_pcmctype;			//!< PCMC種類設定
	UINT8 m_bios32;			//!< BIOS32有効
	CWndProc m_chkenable;		//!< PCI ENABLE
	CComboData m_cmbpcmctype;	//!< PCMC TYPE
	CWndProc m_chkbios32;		//!< BIOS32 ENABLE
	void SetPCMCType(UINT16 cValue);
	UINT8 GetPCMCType() const;
};

/**
 * コンストラクタ
 */
CPCIPage::CPCIPage()
	: CPropPageProc(IDD_PCI)
{
}
/**
 * デストラクタ
 */
CPCIPage::~CPCIPage()
{
}

/**
 * 機種リスト
 */
static const CComboData::Entry s_type[] =
{
	{MAKEINTRESOURCE(IDS_PCI_PCMC_82434LX),		PCI_PCMC_82434LX},
	{MAKEINTRESOURCE(IDS_PCI_PCMC_82441FX),		PCI_PCMC_82441FX},
	{MAKEINTRESOURCE(IDS_PCI_PCMC_WILDCAT),		PCI_PCMC_WILDCAT},
};

/**
 * このメソッドは WM_INITDIALOG のメッセージに応答して呼び出されます
 * @retval TRUE 最初のコントロールに入力フォーカスを設定
 * @retval FALSE 既に設定済
 */
BOOL CPCIPage::OnInitDialog()
{
	m_enable = np2cfg.usepci;
	m_pcmctype = np2cfg.pci_pcmc;
	m_bios32 = np2cfg.pci_bios32;

	m_chkenable.SubclassDlgItem(IDC_PCIENABLE, this);
	if(m_enable)
		m_chkenable.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	else
		m_chkenable.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);
	
	m_cmbpcmctype.SubclassDlgItem(IDC_PCIPCMCTYPE, this);
	m_cmbpcmctype.Add(s_type, _countof(s_type));
	SetPCMCType(m_pcmctype);
	
	m_chkbios32.SubclassDlgItem(IDC_PCIBIOS32, this);
	if(m_bios32)
		m_chkbios32.SendMessage(BM_SETCHECK , BST_CHECKED , 0);
	else
		m_chkbios32.SendMessage(BM_SETCHECK , BST_UNCHECKED , 0);

	m_chkenable.SetFocus();

	return FALSE;
}

/**
 * ユーザーが OK のボタン (IDOK ID がのボタン) をクリックすると呼び出されます
 */
void CPCIPage::OnOK()
{
	UINT update = 0;
	
	if (np2cfg.usepci != m_enable
		|| np2cfg.pci_pcmc != m_pcmctype
		|| np2cfg.pci_bios32 != m_bios32)
	{
		np2cfg.usepci = m_enable;
		np2cfg.pci_pcmc = m_pcmctype;
		np2cfg.pci_bios32 = m_bios32;
		update |= SYS_UPDATECFG;
	}
	::sysmng_update(update);
}

/**
 * ユーザーがメニューの項目を選択したときに、フレームワークによって呼び出されます
 * @param[in] wParam パラメタ
 * @param[in] lParam パラメタ
 * @retval TRUE アプリケーションがこのメッセージを処理した
 */
BOOL CPCIPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
		case IDC_PCIENABLE:
			m_enable = (m_chkenable.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
			return TRUE;

		case IDC_PCIPCMCTYPE:
			m_pcmctype = GetPCMCType();
			return TRUE;
			
		case IDC_PCIBIOS32:
			m_bios32 = (m_chkbios32.SendMessage(BM_GETCHECK , 0 , 0) ? 1 : 0);
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
LRESULT CPCIPage::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return CDlgProc::WindowProc(nMsg, wParam, lParam);
}

/**
 * 機種を設定
 * @param[in] cValue 設定
 */
void CPCIPage::SetPCMCType(UINT16 cValue)
{
	m_cmbpcmctype.SetCurItemData(cValue);
}

/**
 * 機種を取得
 * @return I/O
 */
UINT8 CPCIPage::GetPCMCType() const
{
	return m_cmbpcmctype.GetCurItemData(PCI_PCMC_82434LX);
}


/**
 * コンフィグ ダイアログ
 * @param[in] hwndParent 親ウィンドウ
 */
void dialog_pciopt(HWND hwndParent)
{
	CPropSheetProc prop(IDS_PCIOPTION, hwndParent);
	
	CPCIPage pci;
	prop.AddPage(&pci);
	
	prop.m_psh.dwFlags |= PSH_NOAPPLYNOW | PSH_USEHICON | PSH_USECALLBACK;
	prop.m_psh.hIcon = LoadIcon(CWndProc::GetResourceHandle(), MAKEINTRESOURCE(IDI_ICON2));
	prop.m_psh.pfnCallback = np2class_propetysheet;
	prop.DoModal();

	InvalidateRect(hwndParent, NULL, TRUE);
}

#endif