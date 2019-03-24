/**
 * @file	menu.h
 * @brief	メニューの宣言およびインターフェイスの定義をします
 */

#pragma once

#define MFCHECK(a) ((a) ? MF_CHECKED : MF_UNCHECKED)

bool menu_searchmenu(HMENU hMenu, UINT uID, HMENU *phmenuRet, int *pnPos);
int menu_addmenu(HMENU hMenu, int nPos, HMENU hmenuAdd, BOOL bSeparator);
int menu_addmenures(HMENU hMenu, int nPos, UINT uID, BOOL bSeparator);
void menu_addmenubar(HMENU popup, HMENU menubar);

void sysmenu_initialize(HMENU hMenu);
void sysmenu_update(HMENU hMenu);

void xmenu_initialize(HMENU hMenu);
void xmenu_update(HMENU hMenu);
