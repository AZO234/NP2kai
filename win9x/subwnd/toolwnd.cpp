#include	"compiler.h"
#include	"resource.h"
#include	"toolwnd.h"
#include	"strres.h"
#include	"np2.h"
#include	"winloc.h"
#include	"dosio.h"
#include	"soundmng.h"
#include	"sysmng.h"
#include	"menu.h"
#include	"ini.h"
#include "dialog\np2class.h"
#include "misc\DlgProc.h"
#include "misc\tstring.h"
#include	"pccore.h"
#include	"fdd/diskdrv.h"
#include	"dialog/winfiledlg.h"

extern WINLOCEX np2_winlocexallwin(HWND base);

enum
{
	IDC_BASE				= 3000
};

enum {
	TCTL_STATIC				= 0,
	TCTL_BUTTON				= 1,
	TCTL_DDLIST				= 2
};

typedef struct {
	OEMCHAR	main[MAX_PATH];
	OEMCHAR	font[64];
	SINT32	fontsize;
	UINT32	color1;
	UINT32	color2;
} TOOLSKIN;

typedef struct {
	UINT		tctl;
const OEMCHAR	*text;
	short		posx;
	short		posy;
	short		width;
	short		height;
	short		extend;
	short		padding;
} SUBITEM;

static	NP2TOOL		s_toolwndcfg;
static	TOOLSKIN	toolskin;
static	SUBITEM		subitem[IDC_MAXITEMS];
static	CToolWnd	toolwin;

#include "toolwnd.res"

typedef struct {
	UINT16	idc;
	UINT8	*counter;
} DISKACC;

static const UINT8 fddlist[FDDLIST_DRV] = {
					IDC_TOOLFDD1LIST, IDC_TOOLFDD2LIST};

static const DISKACC diskacc[3] = {
					{IDC_TOOLFDD1ACC,	&toolwin.m_fddaccess[0]},
					{IDC_TOOLFDD2ACC,	&toolwin.m_fddaccess[1]},
					{IDC_TOOLHDDACC,	&toolwin.m_hddaccess}};

static UINT fdlistlen = FDDLIST_DEFAULT; // FDファイル履歴数

/**
 * インスタンスを返す
 * @return インスタンス
 */
CToolWnd* CToolWnd::GetInstance()
{
	return &toolwin;
}

/**
 * コンストラクタ
 */
CToolWnd::CToolWnd()
{
}

/**
 * デストラクタ
 */
CToolWnd::~CToolWnd()
{
}

// ----

static HBITMAP skinload(const OEMCHAR *path)
{
	ZeroMemory(&toolskin, sizeof(toolskin));
	toolskin.fontsize = 12;
	milstr_ncpy(toolskin.font, str_deffont, NELEMENTS(toolskin.font));
	toolskin.color1 = 0x600000;
	toolskin.color2 = 0xff0000;
	if (path)
	{
		ini_read(path, s_skintitle, s_skinini1, _countof(s_skinini1));
	}
	if (toolskin.main[0])
	{
		ZeroMemory(subitem, sizeof(subitem));
		for (UINT i = 0; i < IDC_MAXITEMS; i++)
		{
			subitem[i].tctl = s_defsubitem[i].tctl;
			subitem[i].text = s_defsubitem[i].text;
		}
	}
	else
	{
		CopyMemory(subitem, s_defsubitem, sizeof(s_defsubitem));
	}
	if (path)
	{
		ini_read(path, s_skintitle, s_skinini2, _countof(s_skinini2));
	}

	HBITMAP ret = NULL;
	if (toolskin.main[0])
	{
		TCHAR fname[MAX_PATH];
		milstr_ncpy(fname, path, NELEMENTS(fname));
		file_cutname(fname);
		file_catname(fname, toolskin.main, NELEMENTS(fname));
		ret = static_cast<HBITMAP>(::LoadImage(NULL, fname, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE));
	}
	if (ret == NULL)
	{
#ifdef SUPPORT_PC9821
		HINSTANCE hInstance = CWndProc::FindResourceHandle(TEXT("NP21TOOL"), RT_BITMAP);
		ret = ::LoadBitmap(hInstance, TEXT("NP21TOOL"));
#else
		HINSTANCE hInstance = CWndProc::FindResourceHandle(TEXT("NP2TOOL"), RT_BITMAP);
		ret = ::LoadBitmap(hInstance, TEXT("NP2TOOL"));
#endif
	}
	return ret;
}


// ----

static void calctextsize(OEMCHAR *path, int leng, const OEMCHAR *p, int width) {

	HDC		hdc;
	SIZE	cur;
	OEMCHAR	work[MAX_PATH];
	int		l;
	SIZE	tail;
	int		pos;
	int		step;

	milstr_ncpy(path, p, leng);
	hdc = toolwin.m_hdcfont;
	GetTextExtentPoint32(hdc, p, OEMSTRLEN(p), &cur);
	if (cur.cx < width) {
		return;
	}

	file_cutname(path);
	file_cutseparator(path);
	file_cutname(path);
	file_cutseparator(path);
	l = OEMSTRLEN(path);
	work[0] = '\0';
	if (l) {
		milstr_ncpy(work, str_browse, NELEMENTS(work));
	}
	milstr_ncat(work, p + l, NELEMENTS(work));
	GetTextExtentPoint32(hdc, work, OEMSTRLEN(work), &tail);
	pos = 0;
	while(pos < l) {
#if defined(_UNICODE)
		step = 1;
#else
		step = (IsDBCSLeadByte((BYTE)p[pos]))?2:1;
#endif
		GetTextExtentPoint32(hdc, p, pos + step, &cur);
		if (cur.cx + tail.cx >= width) {
			break;
		}
		pos += step;
	}
	if (pos < leng) {
		path[pos] = '\0';
	}
	milstr_ncat(path, work, leng);
}

static void setlist(HWND hwnd, const TOOLFDD *fdd, UINT sel) {

	RECT		rc;
	int			width;
	OEMCHAR		basedir[MAX_PATH];
	UINT		i;
const OEMCHAR	*p;
	OEMCHAR		dir[MAX_PATH];
const OEMCHAR	*q;

	SendMessage(hwnd, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
	GetClientRect(hwnd, &rc);
	width = rc.right - rc.left - 6;			// border size?
	basedir[0] = '\0';
	if (sel < fdd->cnt) {
		milstr_ncpy(basedir, fdd->name[fdd->pos[sel]], NELEMENTS(basedir));
		file_cutname(basedir);
	}
	for (i=0; i<fdd->cnt; i++) {
		p = fdd->name[fdd->pos[i]];
		milstr_ncpy(dir, p, NELEMENTS(dir));
		file_cutname(dir);
		if (!file_cmpname(basedir, dir)) {
			q = file_getname(p);
		}
		else {
			calctextsize(dir, NELEMENTS(dir), p, width);
			q = dir;
		}
		SendMessage(hwnd, CB_INSERTSTRING, (WPARAM)i, (LPARAM)q);
		p += NELEMENTS(fdd->name[0]);
	}
	if (sel < fdd->cnt) {
		SendMessage(hwnd, CB_SETCURSEL, (WPARAM)sel, (LPARAM)0);
	}
}

static void sellist(UINT drv) {

	HWND	hwnd;
	TOOLFDD	*fdd;
	UINT	sel;

	if (drv >= FDDLIST_DRV) {
		return;
	}
	hwnd = toolwin.m_sub[fddlist[drv]];
	fdd = s_toolwndcfg.fdd + drv;
	sel = (UINT)SendMessage(hwnd, CB_GETCURSEL, 0, 0);
	if (sel < fdd->cnt) {
		diskdrv_setfdd(drv, fdd->name[fdd->pos[sel]], 0);
		fdd->insert = 1;
		setlist(hwnd, fdd, sel);
	}
}

static void remakefddlist(HWND hwnd, TOOLFDD *fdd) {

	OEMCHAR	*p;
	UINT	cnt;
	OEMCHAR	*q;
	OEMCHAR	*fname[FDDLIST_MAX];
	UINT	i;
	UINT	j;
	UINT	sel;

	p = fdd->name[0];
	for (cnt=0; cnt<fdlistlen; cnt++) {
		if (p[0] == '\0') {
			break;
		}
		q = file_getname(p);
		fname[cnt] = q;
		for (i=0; i<cnt; i++) {
			if (file_cmpname(q, fname[fdd->pos[i]]) < 0) {
				break;
			}
		}
		for (j=cnt; j>i; j--) {
			fdd->pos[j] = fdd->pos[j-1];
		}
		fdd->pos[i] = cnt;
		p += NELEMENTS(fdd->name[0]);
	}
	fdd->cnt = cnt;
	sel = (UINT)-1;
	if (fdd->insert) {
		for (i=0; i<cnt; i++) {
			if (fdd->pos[i] == 0) {
				sel = i;
				break;
			}
		}
	}
	setlist(hwnd, fdd, sel);
}

static void accdraw(HWND hWnd, UINT8 count) {

	HDC			hdc;
	PAINTSTRUCT	ps;
	RECT		rc;
	HBRUSH		hbrush;

	hdc = BeginPaint(hWnd, &ps);
	GetClientRect(hWnd, &rc);
	hbrush = (HBRUSH)SelectObject(hdc, toolwin.m_access[(count)?1:0]);
	PatBlt(hdc, rc.left, rc.top, rc.right, rc.bottom, PATCOPY);
	SelectObject(hdc, hbrush);
	EndPaint(hWnd, &ps);
}

static LRESULT CALLBACK twsub(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {

	UINT idc = GetWindowLong(hWnd, GWL_ID) - IDC_BASE;
	if (idc >= IDC_MAXITEMS) {
		return(0);
	}
#if 0
	if (msg == WM_KEYDOWN) {
		if ((short)wp == VK_TAB) {
			int dir = (GetKeyState(VK_SHIFT) >= 0)?1:-1;
			UINT newidc = idc;
			do {
				newidc += dir;
				if (newidc >= IDC_MAXITEMS) {
					newidc = (dir >= 0)?0:(IDC_MAXITEMS - 1);
				}
				if ((toolwin.m_sub[newidc] != NULL) &&
					(subitem[newidc].tctl != TCTL_STATIC)) {
					SetFocus(toolwin.m_sub[newidc]);
					break;
				}
			} while(idc != newidc);
		}
		else if ((short)wp == VK_RETURN) {
			if (subitem[idc].tctl == TCTL_BUTTON) {
				return(CallWindowProc(toolwin.m_subproc[idc],
										hWnd, WM_KEYDOWN, VK_SPACE, 0));
			}
		}
	}
	else
#endif
	 if (msg == WM_DROPFILES) {
	    int files = DragQueryFile((HDROP)wp, (UINT)-1, NULL, 0);
		if (files == 1)
		{
			OEMCHAR	fname[MAX_PATH];
			DragQueryFile((HDROP)wp, 0, fname, NELEMENTS(fname));
			if (idc == IDC_TOOLFDD1LIST) {
				diskdrv_setfdd(0, fname, 0);
				toolwin_setfdd(0, fname);
			}
			else if (idc == IDC_TOOLFDD2LIST) {
				diskdrv_setfdd(1, fname, 0);
				toolwin_setfdd(1, fname);
			}
		}
		DragFinish((HDROP)wp);
		return(FALSE);
	}
	else if (msg == WM_PAINT) {
		if (idc == IDC_TOOLHDDACC) {
			accdraw(hWnd, toolwin.m_hddaccess);
			return(FALSE);
		}
		else if (idc == IDC_TOOLFDD1ACC) {
			accdraw(hWnd, toolwin.m_fddaccess[0]);
			return(FALSE);
		}
		else if (idc == IDC_TOOLFDD2ACC) {
			accdraw(hWnd, toolwin.m_fddaccess[1]);
			return(FALSE);
		}
	}
	else if (msg == WM_SETFOCUS) {
//		SetWindowLongPtr(GetParent(hWnd), GTWLP_FOCUS, idc);
	}
	return CallWindowProc(toolwin.m_subproc[idc], hWnd, msg, wp, lp);
}

void CToolWnd::CreateSubItems()
{
	m_hfont = ::CreateFont(toolskin.fontsize, 0, 0, 0, 0, 0, 0, 0,
					SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, FIXED_PITCH, toolskin.font);
	HDC hdc = ::GetDC(NULL);
	m_hdcfont = ::CreateCompatibleDC(hdc);
	::ReleaseDC(NULL, hdc);
	::SelectObject(m_hdcfont, m_hfont);

	m_access[0] = ::CreateSolidBrush(0x000060);
	m_access[1] = ::CreateSolidBrush(0x0000ff);

	for (UINT i = 0; i < IDC_MAXITEMS; i++)
	{
		HWND sub = NULL;
		const TCHAR *cls = NULL;
		DWORD style = 0;

		const SUBITEM *p = &subitem[i];
		switch (p->tctl)
		{
			case TCTL_STATIC:
				cls = str_static;
				style = 0;
				break;

			case TCTL_BUTTON:
				if (p->extend == 0)
				{
					cls = str_button;
					style = BS_PUSHBUTTON;
				}
				else if (p->extend == 1)
				{
					cls = str_button;
					style = BS_OWNERDRAW;
				}
				break;

			case TCTL_DDLIST:
				cls = str_combobox;
				style = CBS_DROPDOWNLIST | WS_VSCROLL;
				break;
		}
		if ((cls) && (p->width > 0) && (p->height > 0))
		{
			sub = CreateWindow(cls, p->text, WS_CHILD | WS_VISIBLE | style,
							p->posx, p->posy, p->width, p->height,
							m_hWnd, (HMENU)(i + IDC_BASE), CWndProc::GetInstanceHandle(), NULL);
		}
		m_sub[i] = sub;
		m_subproc[i] = NULL;
		if (sub)
		{
			m_subproc[i] = (WNDPROC)GetWindowLongPtr(sub, GWLP_WNDPROC);
			SetWindowLongPtr(sub, GWLP_WNDPROC, (LONG_PTR)twsub);
			::SendMessage(sub, WM_SETFONT, (WPARAM)m_hfont, MAKELPARAM(TRUE, 0));
		}
	}
	for (UINT i = 0; i < FDDLIST_DRV; i++)
	{
		HWND sub = m_sub[fddlist[i]];
		if (sub)
		{
			DragAcceptFiles(sub, TRUE);
			remakefddlist(sub, s_toolwndcfg.fdd + i);
		}
	}
	for (UINT i = 0; i < IDC_MAXITEMS; i++)
	{
		if ((m_sub[i]) && (subitem[i].tctl != TCTL_STATIC))
		{
			// SetWindowLongPtr(hWnd, GTWLP_FOCUS, i);
			break;
		}
	}
}

void CToolWnd::DestroySubItems()
{
	for (UINT i = 0; i < IDC_MAXITEMS; i++)
	{
		HWND sub = m_sub[i];
		m_sub[i] = NULL;
		if (sub)
		{
			if(m_subproc[i]){
				SetWindowLongPtr(sub, GWLP_WNDPROC, (LONG_PTR)m_subproc[i]); // 一応戻しておいてあげましょう
			}
			m_subproc[i] = NULL;
			::SetParent(sub, NULL); // 子ウィンドウ解除
			::DestroyWindow(sub);
		}
	}

	for (UINT i = 0; i < 2; i++)
	{
		if (m_access[i])
		{
			::DeleteObject(m_access[i]);
			m_access[i] = NULL;
		}
	}
	if (m_hdcfont)
	{
		::DeleteObject(m_hdcfont);
		m_hdcfont = NULL;
	}
	if (m_hfont)
	{
		::DeleteObject(m_hfont);
		m_hfont = NULL;
	}
	if (m_hbmp)
	{
		::DeleteObject(m_hbmp);
		m_hbmp = NULL;
	}
}

void CToolWnd::OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(&ps);
	if (m_hbmp)
	{
		BITMAP bmp;
		::GetObject(m_hbmp, sizeof(bmp), &bmp);
		HDC hmdc = ::CreateCompatibleDC(hdc);
		::SelectObject(hmdc, m_hbmp);
		::BitBlt(hdc, 0, 0, bmp.bmWidth, bmp.bmHeight, hmdc, 0, 0, SRCCOPY);
		::DeleteDC(hmdc);
	}
	EndPaint(&ps);
}

void CToolWnd::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	nIDCtl -= IDC_BASE;
	if ((nIDCtl < 0) || (nIDCtl >= IDC_MAXITEMS))
	{
		return;
	}

	POINT pt;
	pt.x = 0;
	pt.y = 0;
	ClientToScreen(&pt);
	HWND sub = m_sub[nIDCtl];

	RECT rect;
	::GetWindowRect(sub, &rect);

	RECT btn;
	btn.left = 0;
	btn.top = 0;
	btn.right = rect.right - rect.left;
	btn.bottom = rect.bottom - rect.top;
	HDC hdc = lpDrawItemStruct->hDC;
	if (m_hbmp)
	{
		HDC hmdc = ::CreateCompatibleDC(hdc);
		::SelectObject(hmdc, m_hbmp);
		::BitBlt(hdc, 0, 0, btn.right, btn.bottom, hmdc, rect.left - pt.x, rect.top - pt.y, SRCCOPY);
		::DeleteDC(hmdc);
	}
	if (lpDrawItemStruct->itemState & ODS_FOCUS)
	{
		DrawFocusRect(hdc, &btn);
	}
}


// ----

static void setSkinMruMenu(HMENU hMenu)
{
	HMENU		hmenuSub;
const OEMCHAR	*pcszBase;
	UINT		uCount;
	OEMCHAR		*pszMru;
const OEMCHAR	*pcszMruList[SKINMRU_MAX];
	UINT		i;
	UINT		uID[SKINMRU_MAX];
	UINT		j;
	UINT		uFlag;

	for (i=0; i<SKINMRU_MAX; i++)
	{
		DeleteMenu(hMenu, IDM_TOOL_SKINMRU + i, MF_BYCOMMAND);
	}

	if (!menu_searchmenu(hMenu, IDM_TOOL_SKINDEF, &hmenuSub, NULL))
	{
		return;
	}

	pcszBase = s_toolwndcfg.skin;

	CheckMenuItem(hMenu, IDM_TOOL_SKINDEF, MFCHECK(pcszBase[0] == '\0'));

	for (uCount=0; uCount<SKINMRU_MAX; uCount++)
	{
		pszMru = s_toolwndcfg.skinmru[uCount];
		if (pszMru[0] == '\0')
		{
			break;
		}
		pszMru = file_getname(pszMru);
		for (i=0; i<uCount; i++)
		{
			if (file_cmpname(pszMru, pcszMruList[uID[i]]) < 0)
			{
				break;
			}
		}
		for (j=uCount; j>i; j--)
		{
			uID[j] = uID[j-1];
		}
		uID[i] = uCount;
		pcszMruList[uCount] = pszMru;
	}

	for (i=0; i<uCount; i++)
	{
		j = uID[i];
		uFlag = MFCHECK(!file_cmpname(pcszBase, s_toolwndcfg.skinmru[j]));
		const TCHAR *szPath = pcszMruList[j];
		AppendMenu(hmenuSub, MF_STRING + uFlag, IDM_TOOL_SKINMRU + j, pcszMruList[j]);
	}
}

void CToolWnd::ChangeSkin()
{
	const OEMCHAR* p = s_toolwndcfg.skin;
	if (p[0])
	{
		UINT i;
		for (i = 0; i < (SKINMRU_MAX - 1); i++)
		{
			if (!file_cmpname(p, s_toolwndcfg.skinmru[i]))
			{
				break;
			}
		}
		while (i > 0)
		{
			CopyMemory(s_toolwndcfg.skinmru[i], s_toolwndcfg.skinmru[i-1],
												sizeof(s_toolwndcfg.skinmru[0]));
			i--;
		}
		file_cpyname(s_toolwndcfg.skinmru[0], p, NELEMENTS(s_toolwndcfg.skinmru[0]));
	}
	setSkinMruMenu(np2class_gethmenu(m_hWnd));
	setSkinMruMenu(GetSystemMenu(FALSE));
	DrawMenuBar();
	sysmng_update(SYS_UPDATEOSCFG);

	WINLOCEX wlex = np2_winlocexallwin(g_hWndMain);
	winlocex_setholdwnd(wlex, m_hWnd);
	DestroySubItems();
	HBITMAP hbmp = skinload(s_toolwndcfg.skin);
	if (hbmp == NULL)
	{
		SendMessage(WM_CLOSE, 0, 0);
		return;
	}

	BITMAP bmp;
	GetObject(hbmp, sizeof(BITMAP), &bmp);
	m_hbmp = hbmp;
	winloc_setclientsize(m_hWnd, bmp.bmWidth, bmp.bmHeight);
	CreateSubItems();
	winlocex_move(wlex);
	winlocex_destroy(wlex);
}


// ----

void CToolWnd::OpenPopUp(LPARAM lParam)
{
	HMENU hMenu = CreatePopupMenu();
	if (!winui_en)
	{
		menu_addmenu(hMenu, 0, np2class_gethmenu(g_hWndMain), FALSE);
	}
	menu_addmenures(hMenu, -1, IDR_TOOLWIN, FALSE);
	menu_addmenures(hMenu, -1, IDR_CLOSE, TRUE);
	setSkinMruMenu(hMenu);
	xmenu_update(hMenu);
	POINT pt;
	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);
	ClientToScreen(&pt);
	TrackPopupMenu(hMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, m_hWnd, NULL);
	DestroyMenu(hMenu);
}

int CToolWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	np2class_wmcreate(m_hWnd);
	setSkinMruMenu(np2class_gethmenu(m_hWnd));

	HMENU hMenu = GetSystemMenu(FALSE);
	int nCount = menu_addmenures(hMenu, 0, IDR_TOOLWIN, FALSE);
	if (nCount)
	{
		InsertMenu(hMenu, nCount, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
		setSkinMruMenu(hMenu);
	}

	np2class_windowtype(m_hWnd, (s_toolwndcfg.type & 1) << 1);
	CreateSubItems();
	return 0;
}

LRESULT CToolWnd::WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	switch (nMsg)
	{
		case WM_CREATE:
			return OnCreate(reinterpret_cast<LPCREATESTRUCT>(lParam));
			break;

		case WM_SYSCOMMAND:
			switch (wParam)
			{
				case IDM_TOOL_SKINSEL:
				case IDM_TOOL_SKINDEF:
				case IDM_TOOL_SKINMRU + 0:
				case IDM_TOOL_SKINMRU + 1:
				case IDM_TOOL_SKINMRU + 2:
				case IDM_TOOL_SKINMRU + 3:
					return SendMessage(WM_COMMAND, wParam, lParam);

				default:
					return CSubWndBase::WindowProc(nMsg, wParam, lParam);
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_BASE + IDC_TOOLFDD1LIST:
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						sellist(0);
					}
					break;

				case IDC_BASE + IDC_TOOLFDD1BROWSE:
					if (!winui_en)
					{
						::SendMessage(g_hWndMain, WM_COMMAND, IDM_FDD1OPEN, 0);
					}
					break;

				case IDC_BASE + IDC_TOOLFDD1EJECT:
					diskdrv_setfdd(0, NULL, 0);
					toolwin_setfdd(0, NULL);
					break;

				case IDC_BASE + IDC_TOOLFDD2LIST:
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						sellist(1);
					}
					break;

				case IDC_BASE + IDC_TOOLFDD2BROWSE:
					if (!winui_en)
					{
						::SendMessage(g_hWndMain, WM_COMMAND, IDM_FDD2OPEN, 0);
					}
					break;

				case IDC_BASE + IDC_TOOLFDD2EJECT:
					diskdrv_setfdd(1, NULL, 0);
					toolwin_setfdd(1, NULL);
					break;

				case IDC_BASE + IDC_TOOLRESET:
					if (!winui_en)
					{
						::SendMessage(g_hWndMain, WM_COMMAND, IDM_RESET, 0);
						SetForegroundWindow(g_hWndMain);
					}
					break;

				case IDC_BASE + IDC_TOOLPOWER:
					if (!winui_en)
					{
						::SendMessage(g_hWndMain, WM_CLOSE, 0, 0L);
					}
					break;

				case IDM_TOOL_SKINSEL:
					{
						CSoundMng::GetInstance()->Disable(SNDPROC_TOOL);

						TCHAR szPath[MAX_PATH];
						TCHAR szName[MAX_PATH];
						std::tstring rExt(LoadTString(IDS_SKINEXT));
						std::tstring rFilter(LoadTString(IDS_SKINFILTER));
						std::tstring rTitle(LoadTString(IDS_SKINTITLE));

						CFileDlg dlg(TRUE, rExt.c_str(), s_toolwndcfg.skin, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, rFilter.c_str(), m_hWnd);
						dlg.m_ofn.lpstrTitle = rTitle.c_str();
						dlg.m_ofn.nFilterIndex = 1;
						OPENFILENAMEW ofnw;
						const BOOL r = WinFileDialogW(NULL, &ofnw, WINFILEDIALOGW_MODE_GET1, szPath, szName, rExt.c_str(), rTitle.c_str(), rFilter.c_str(), 1);

						CSoundMng::GetInstance()->Enable(SNDPROC_TOOL);

						if (r)
						{
							file_cpyname(s_toolwndcfg.skin, szPath, _countof(s_toolwndcfg.skin));
							ChangeSkin();
						}
					}
					break;

				case IDM_TOOL_SKINDEF:
					s_toolwndcfg.skin[0] = '\0';
					ChangeSkin();
					break;

				case IDM_TOOL_SKINMRU + 0:
				case IDM_TOOL_SKINMRU + 1:
				case IDM_TOOL_SKINMRU + 2:
				case IDM_TOOL_SKINMRU + 3:
					file_cpyname(s_toolwndcfg.skin, s_toolwndcfg.skinmru[LOWORD(wParam) - IDM_TOOL_SKINMRU], NELEMENTS(s_toolwndcfg.skin));
					ChangeSkin();
					break;

				case IDM_CLOSE:
					SendMessage(WM_CLOSE, 0, 0);
					break;

				default:
					if (!winui_en)
					{
						return ::SendMessage(g_hWndMain, nMsg, wParam, lParam);
					}
					break;
			}
			break;

#if 0
		case WM_KEYDOWN:						// TABを押した時に復帰
			if ((short)wParam == VK_TAB)
			{
				UINT idc = (UINT)GetWindowLongPtr(m_hWnd, GTWLP_FOCUS);
				if (idc < IDC_MAXITEMS)
				{
					::SetFocus(toolwin.sub[idc]);
				}
				return 0;
			}
			return ::SendMessage(g_hWndMain, nMsg, wParam, lParam);

		case WM_KEYUP:
			if ((short)wParam == VK_TAB)
			{
				return 0;
			}
			return ::SendMessage(g_hWndMain, nMsg, wParam, lParam);
#endif

		case WM_PAINT:
			OnPaint();
			break;

		case WM_DRAWITEM:
			OnDrawItem(static_cast<int>(wParam), reinterpret_cast<LPDRAWITEMSTRUCT>(lParam));
			break;

#if 0
		case WM_ENTERMENULOOP:
			CSoundMng::GetInstance()->Disable(SNDPROC_TOOL);
			break;

		case WM_EXITMENULOOP:
			CSoundMng::GetInstance()->Enable(SNDPROC_TOOL);
			break;

		case WM_ENTERSIZEMOVE:
			CSoundMng::GetInstance()->Disable(SNDPROC_TOOL);
			winlocex_destroy(toolwin.wlex);
			toolwin.wlex = np2_winlocexallwin(hWnd);
			break;

		case WM_MOVING:
			winlocex_moving(toolwin.wlex, (RECT *)lp);
			break;

		case WM_EXITSIZEMOVE:
			winlocex_destroy(toolwin.wlex);
			toolwin.wlex = NULL;
			CSoundMng::GetInstance()->Enable(SNDPROC_TOOL);
			break;
#endif

		case WM_MOVE:
			if (!(GetWindowLong(m_hWnd, GWL_STYLE) & (WS_MAXIMIZE | WS_MINIMIZE)))
			{
				RECT rc;
				GetWindowRect(&rc);
				s_toolwndcfg.posx = rc.left;
				s_toolwndcfg.posy = rc.top;
				sysmng_update(SYS_UPDATEOSCFG);
			}
			break;

		case WM_CLOSE:
			np2oscfg.toolwin = 0;
			sysmng_update(SYS_UPDATEOSCFG);
			DestroyWindow();
			break;

		case WM_DESTROY:
			np2class_wmdestroy(m_hWnd);
			DestroySubItems();
			break;

		case WM_LBUTTONDOWN:
			if (s_toolwndcfg.type & 1)
			{
				return SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, 0L);
			}
			break;

		case WM_RBUTTONDOWN:
			OpenPopUp(lParam);
			break;

		case WM_LBUTTONDBLCLK:
			{
				s_toolwndcfg.type ^= 1;
				WINLOCEX wlex = np2_winlocexallwin(g_hWndMain);
				winlocex_setholdwnd(wlex, m_hWnd);
				np2class_windowtype(m_hWnd, (s_toolwndcfg.type & 1) << 1);
				winlocex_move(wlex);
				winlocex_destroy(wlex);
				sysmng_update(SYS_UPDATEOSCFG);
			}
			break;

		default:
			return CSubWndBase::WindowProc(nMsg, wParam, lParam);
	}
	return 0;
}

/**
 * 作成
 */
void CToolWnd::Create()
{
	if (m_hWnd)
	{
		return;
	}

	m_hbmp = NULL;
	m_fddaccess[0] = 0;
	m_fddaccess[1] = 0;
	m_hddaccess = 0;
	m_hfont = NULL;
	m_hdcfont = NULL;
	m_access[0] = NULL;
	m_access[1] = NULL;
	ZeroMemory(m_sub, sizeof(m_sub));
	ZeroMemory(m_subproc, sizeof(m_subproc));

	m_hbmp = skinload(s_toolwndcfg.skin);
	if (m_hbmp == NULL)
	{
		return;
	}

	BITMAP bmp;
	::GetObject(m_hbmp, sizeof(BITMAP), &bmp);

	if (!CSubWndBase::Create(IDS_CAPTION_TOOL, WS_SYSMENU | WS_MINIMIZEBOX, s_toolwndcfg.posx, s_toolwndcfg.posy, bmp.bmWidth, bmp.bmHeight, NULL, NULL))
	{
		np2oscfg.toolwin = 0;
		sysmng_update(SYS_UPDATEOSCFG);
		return;
	}
	winloc_setclientsize(m_hWnd, bmp.bmWidth, bmp.bmHeight);
	UpdateWindow();
	ShowWindow(SW_SHOWNOACTIVATE);
	SetForegroundWindow(g_hWndMain);
}

void toolwin_setfdd(UINT8 drv, const OEMCHAR *name) {

	TOOLFDD	*fdd;
	OEMCHAR	*q;
	OEMCHAR	*p;
	UINT	i;
	HWND	sub;

	if (drv >= FDDLIST_DRV) {
		return;
	}
	fdd = s_toolwndcfg.fdd + drv;
	if ((name == NULL) || (name[0] == '\0')) {
		fdd->insert = 0;
	}
	else {
		fdd->insert = 1;
		q = fdd->name[0];
		for (i=0; i<(fdlistlen - 1); i++) {
			if (!file_cmpname(q, name)) {
				break;
			}
			q += NELEMENTS(fdd->name[0]);
		}
		p = q - NELEMENTS(fdd->name[0]);
		while(i > 0) {
			i--;
			CopyMemory(q, p, sizeof(fdd->name[0]));
			p -= NELEMENTS(fdd->name[0]);
			q -= NELEMENTS(fdd->name[0]);
		}
		file_cpyname(fdd->name[0], name, NELEMENTS(fdd->name[0]));
	}
	sysmng_update(SYS_UPDATEOSCFG);
	if (toolwin.m_hWnd != NULL) {
		sub = toolwin.m_sub[fddlist[drv]];
		if (sub) {
			remakefddlist(sub, fdd);
			SetForegroundWindow(g_hWndMain);
		}
	}
}

static void setdiskacc(UINT num, UINT8 count) {

const DISKACC	*acc;
	HWND		sub;

	if (toolwin.m_hWnd == NULL) {
		return;
	}
	if (num < NELEMENTS(diskacc)) {
		acc = diskacc + num;
		sub = NULL;
		if (*(acc->counter) == 0) {
			sub = toolwin.m_sub[acc->idc];
		}
		*(acc->counter) = count;
		if (sub) {
			InvalidateRect(sub, NULL, TRUE);
		}
	}
}

void toolwin_fddaccess(UINT8 drv)
{
	if (drv < 2)
	{
		setdiskacc(drv, 20);
	}
}

void toolwin_hddaccess(UINT8 drv)
{
	setdiskacc(2, 10);
}

void toolwin_draw(UINT8 frame) {

const DISKACC	*acc;
const DISKACC	*accterm;
	UINT8		counter;
	HWND		sub;

	if (toolwin.m_hWnd == NULL) {
		return;
	}
	if (!frame) {
		frame = 1;
	}
	acc = diskacc;
	accterm = acc + NELEMENTS(diskacc);
	while(acc < accterm) {
		counter = *acc->counter;
		if (counter) {
			if (counter <= frame) {
				*(acc->counter) = 0;
				sub = toolwin.m_sub[acc->idc];
				if (sub) {
					InvalidateRect(sub, NULL, TRUE);
				}
			}
			else {
				*(acc->counter) -= frame;
			}
		}
		acc++;
	}
}


// ----

//! タイトル
static const TCHAR s_toolwndapp[] = TEXT("NP2 tool");

/**
 * 設定
 */
static const PFTBL s_toolwndini[] =
{
	PFVAL("WindposX", PFTYPE_SINT32,	&s_toolwndcfg.posx),
	PFVAL("WindposY", PFTYPE_SINT32,	&s_toolwndcfg.posy),
	PFVAL("WindType", PFTYPE_BOOL,		&s_toolwndcfg.type),
	PFSTR("SkinFile", PFTYPE_STR,		s_toolwndcfg.skin),
	PFSTR("SkinMRU0", PFTYPE_STR,		s_toolwndcfg.skinmru[0]),
	PFSTR("SkinMRU1", PFTYPE_STR,		s_toolwndcfg.skinmru[1]),
	PFSTR("SkinMRU2", PFTYPE_STR,		s_toolwndcfg.skinmru[2]),
	PFSTR("SkinMRU3", PFTYPE_STR,		s_toolwndcfg.skinmru[3]),
	//PFSTR("FD1NAME0", PFTYPE_STR,		s_toolwndcfg.fdd[0].name[0]),
	//PFSTR("FD1NAME1", PFTYPE_STR,		s_toolwndcfg.fdd[0].name[1]),
	//PFSTR("FD1NAME2", PFTYPE_STR,		s_toolwndcfg.fdd[0].name[2]),
	//PFSTR("FD1NAME3", PFTYPE_STR,		s_toolwndcfg.fdd[0].name[3]),
	//PFSTR("FD1NAME4", PFTYPE_STR,		s_toolwndcfg.fdd[0].name[4]),
	//PFSTR("FD1NAME5", PFTYPE_STR,		s_toolwndcfg.fdd[0].name[5]),
	//PFSTR("FD1NAME6", PFTYPE_STR,		s_toolwndcfg.fdd[0].name[6]),
	//PFSTR("FD1NAME7", PFTYPE_STR,		s_toolwndcfg.fdd[0].name[7]),
	//PFSTR("FD2NAME0", PFTYPE_STR,		s_toolwndcfg.fdd[1].name[0]),
	//PFSTR("FD2NAME1", PFTYPE_STR,		s_toolwndcfg.fdd[1].name[1]),
	//PFSTR("FD2NAME2", PFTYPE_STR,		s_toolwndcfg.fdd[1].name[2]),
	//PFSTR("FD2NAME3", PFTYPE_STR,		s_toolwndcfg.fdd[1].name[3]),
	//PFSTR("FD2NAME4", PFTYPE_STR,		s_toolwndcfg.fdd[1].name[4]),
	//PFSTR("FD2NAME5", PFTYPE_STR,		s_toolwndcfg.fdd[1].name[5]),
	//PFSTR("FD2NAME6", PFTYPE_STR,		s_toolwndcfg.fdd[1].name[6]),
	//PFSTR("FD2NAME7", PFTYPE_STR,		s_toolwndcfg.fdd[1].name[7])
};

int gettoolwndini(PFTBL **ptoolwndini)
{
	PFTBL *toolwndini;
	int slistlen;

	fdlistlen = np2oscfg.toolwndhistory;
	if(fdlistlen == 0) fdlistlen = FDDLIST_DEFAULT;
	if(fdlistlen > FDDLIST_MAX) fdlistlen = FDDLIST_MAX;

	toolwndini = (PFTBL*)malloc(sizeof(s_toolwndini) + 2*fdlistlen*sizeof(PFTBL));
	
	slistlen = _countof(s_toolwndini);
	memcpy(toolwndini, s_toolwndini, sizeof(s_toolwndini));
	for(int j=0;j<2;j++){
		for(UINT i=0;i<fdlistlen;i++){
			OEMSPRINTF(toolwndini[slistlen + j*fdlistlen + i].item, OEMTEXT("FD%dNAME%X"), j+1, i);
			toolwndini[slistlen + j*fdlistlen + i].itemtype = PFTYPE_STR;
			toolwndini[slistlen + j*fdlistlen + i].value = s_toolwndcfg.fdd[j].name[i];
			toolwndini[slistlen + j*fdlistlen + i].arg = MAX_PATH;
		}
	}

	*ptoolwndini = toolwndini;

	return 2*fdlistlen + slistlen;
}

/**
 * 設定読み込み
 */
void toolwin_readini()
{
	PFTBL *toolwndini;
	int listlen;

	listlen = gettoolwndini(&toolwndini);

	ZeroMemory(&s_toolwndcfg, sizeof(s_toolwndcfg));
	s_toolwndcfg.posx = CW_USEDEFAULT;
	s_toolwndcfg.posy = CW_USEDEFAULT;
	s_toolwndcfg.type = 1;

	OEMCHAR szPath[MAX_PATH];
	initgetfile(szPath, _countof(szPath));
	ini_read(szPath, s_toolwndapp, toolwndini, listlen);

	free(toolwndini);
}

/**
 * 設定書き込み
 */
void toolwin_writeini()
{
	if(!np2oscfg.readonly){
		PFTBL *toolwndini;
		int listlen;

		listlen = gettoolwndini(&toolwndini);

		TCHAR szPath[MAX_PATH];
		initgetfile(szPath, _countof(szPath));
		ini_write(szPath, s_toolwndapp, toolwndini, listlen);

		free(toolwndini);
	}
}
