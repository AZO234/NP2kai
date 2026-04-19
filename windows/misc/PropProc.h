/**
 * @file	PropProc.h
 * @brief	プロパティ シート クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <vector>
#include "DlgProc.h"
#include "tstring.h"

/**
 * @brief プロパティ シート ページ
 */
class CPropPageProc : public CDlgProc
{
public:
	PROPSHEETPAGE m_psp;			//!< プロパティ シート ページ構造体

public:
	CPropPageProc(UINT nIDTemplate, UINT nIDCaption = 0);
	CPropPageProc(LPCTSTR lpszTemplateName, UINT nIDCaption = 0);
	virtual ~CPropPageProc();
	void Construct(UINT nIDTemplate, UINT nIDCaption = 0);
	void Construct(LPCTSTR lpszTemplateName, UINT nIDCaption = 0);

protected:
	BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual BOOL OnApply();
	virtual void OnReset();
	virtual void OnOK();
	virtual void OnCancel();

private:
	LPTSTR m_lpCaption;				//!< キャプション
	static UINT CALLBACK PropPageCallback(HWND hWnd, UINT message, LPPROPSHEETPAGE pPropPage);
};

/**
 * @brief プロパティ シート
 */
class CPropSheetProc /* : public CWnd */
{
public:
	PROPSHEETHEADER m_psh;					//!< プロパティ シート ヘッダ構造体

public:
	CPropSheetProc();
	CPropSheetProc(UINT nIDCaption, HWND hwndParent = NULL, UINT iSelectPage = 0);
	CPropSheetProc(LPCTSTR pszCaption, HWND hwndParent = NULL, UINT iSelectPage = 0);
	INT_PTR DoModal();
	void AddPage(CPropPageProc* pPage);

protected:
	std::vector<CPropPageProc*> m_pages;	//!< The array of CPropPageProc pointers
	std::tstring m_strCaption;				//!< The caption

	void CommonConstruct(HWND hwndParent, UINT iSelectPage);
};
