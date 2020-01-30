/**
 * @file	hostdrvs.c
 * @brief	Implementation of host-drive
 */

#include "compiler.h"
#include "hostdrvs.h"

#if defined(SUPPORT_HOSTDRV)

#if defined(OSLANG_EUC) || defined(OSLANG_UTF8) || defined(OSLANG_UCS2)
#include "oemtext.h"
#endif
#include "pccore.h"
#include "ini.h"

//#include <shlwapi.h>

/*! ルート情報 */
static const HDRVFILE s_hddroot = {{' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}, 0, 0, 0x10, {0}, {0}};

/*! 自分 */
static const char s_self[11] = {'.',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};

/*! 親 */
static const char s_parent[11] = {'.','.',' ',' ',' ',' ',' ',' ',' ',' ',' '};

/*! DOSで許可されるキャラクタ */
static const UINT8 s_cDosCharacters[] =
{
	0xfa, 0x23,		/* '&%$#"!  /.-,+*)( */
	0xff, 0x03,		/* 76543210 ?>=<;:98 */
	0xff, 0xff,		/* GFEDCBA@ ONMLKJIH */
	0xff, 0xef,		/* WVUTSRQP _^]\[ZYX */
	0x01, 0x00,		/* gfedcba` onmlkjih */
	0x00, 0x40		/* wvutsrqp ~}|{zyx  */
};

/**
 * パスを FCB に変換
 * @param[out] lpFcbname FCB
 * @param[in] cchFcbname FCB バッファ サイズ
 * @param[in] lpPath パス
 */
static void RealPath2FcbSub(char *lpFcbname, UINT cchFcbname, const char *lpPath)
{
	REG8 c;

	while (cchFcbname)
	{
		c = (UINT8)*lpPath++;
		if (c == 0)
		{
			break;
		}
#if defined(OSLANG_SJIS) || defined(OSLANG_EUC) || defined(OSLANG_UTF8) || defined(OSLANG_UCS2)
		if ((((c ^ 0x20) - 0xa1) & 0xff) < 0x3c)
		{
			if (lpPath[0] == '\0')
			{
				break;
			}
			if (cchFcbname < 2)
			{
				break;
			}
			lpFcbname[0] = c;
			lpFcbname[1] = *lpPath++;
			lpFcbname += 2;
			cchFcbname -= 2;
		}
		else if (((c - 0x20) & 0xff) < 0x60)
		{
			if (((c - 'a') & 0xff) < 26)
			{
				c -= 0x20;
			}
			if (s_cDosCharacters[(c >> 3) - (0x20 >> 3)] & (1 << (c & 7)))
			{
				*lpFcbname++ = c;
				cchFcbname--;
			}
		}
		else if (((c - 0xa0) & 0xff) < 0x40)
		{
			*lpFcbname++ = c;
			cchFcbname--;
		}
#else
		if (((c - 0x20) & 0xff) < 0x60)
		{
			if (((c - 'a') & 0xff) < 26)
			{
				c -= 0x20;
			}
			if (s_cDosCharacters[(c >> 3) - (0x20 >> 3)] & (1 << (c & 7)))
			{
				*lpFcbname++ = c;
				cchFcbname--;
			}
		}
		else if (c >= 0x80)
		{
			*lpFcbname++ = c;
			cchFcbname--;
		}
#endif
	}
}

/**
 * パスを FCB に変換
 * @param[out] lpFcbname FCB
 * @param[in] lpPath パス
 */
static void RealName2Fcb(char *lpFcbname, const OEMCHAR *lpPath)
{
	OEMCHAR	*ext;
#if defined(OSLANG_EUC) || defined(OSLANG_UTF8) || defined(OSLANG_UCS2)
	char sjis[MAX_PATH];
#endif
	OEMCHAR szFilename[MAX_PATH];

	FillMemory(lpFcbname, 11, ' ');

	ext = file_getext(lpPath);
#if defined(OSLANG_EUC) || defined(OSLANG_UTF8) || defined(OSLANG_UCS2)
	oemtext_oemtosjis(sjis, NELEMENTS(sjis), ext, (UINT)-1);
	RealPath2FcbSub(lpFcbname + 8, 3, sjis);
#else
	RealPath2FcbSub(lpFcbname + 8, 3, ext);
#endif

	file_cpyname(szFilename, lpPath, NELEMENTS(szFilename));
	file_cutext(szFilename);
#if defined(OSLANG_EUC) || defined(OSLANG_UTF8) || defined(OSLANG_UCS2)
	oemtext_oemtosjis(sjis, NELEMENTS(sjis), szFilename, (UINT)-1);
	RealPath2FcbSub(lpFcbname + 0, 8, sjis);
#else
	RealPath2FcbSub(lpFcbname + 0, 8, szFilename);
#endif
}

/**
 * FCB 名が一致するか?
 * @param[in] phdf ファイル情報
 * @param[in] lpMask マスク
 * @param[in] nAttr アトリビュート マスク
 * @retval TRUE 一致
 * @retval FALSE 不一致
 */
static BOOL IsMatchFcb(const HDRVFILE *phdf, const char *lpMask, UINT nAttr)
{
	UINT i;

	if ((phdf->attr & (~nAttr)) & 0x16)
	{
		return FALSE;
	}
	if (lpMask != NULL)
	{
		for (i = 0; i < 11; i++)
		{
			if ((phdf->fcbname[i] != lpMask[i]) && (lpMask[i] != '?'))
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

/**
 * FCB 名が一致するか
 * @param[in] vpItem アイテム
 * @param[in] vpArg ユーザ引数
 * @retval TRUE 一致
 * @retval FALSE 不一致
 */
static BOOL IsMatchName(void *vpItem, void *vpArg)
{
	return IsMatchFcb((HDRVFILE *)vpItem, (char *)vpArg, 0x16);
}

/**
 * ファイル一覧を取得
 * @param[in] phdp パス
 * @param[in] lpMask マスク
 * @param[in] nAttr アトリビュート
 * @return ファイル一覧
 */
LISTARRAY hostdrvs_getpathlist(const HDRVPATH *phdp, const char *lpMask, UINT nAttr)
{
	LISTARRAY ret;
	LISTARRAY lst;
	HDRVFILE file;
	HDRVLST hdd;
	FLISTH flh;
	FLINFO fli;

	ret = listarray_new(sizeof(_HDRVLST), 64);
	if (ret != NULL)
	{
		lst = listarray_new(sizeof(file), 64);

		if (phdp->file.attr & 0x10)
		{
			file = phdp->file;
			memcpy(file.fcbname, s_self, 11);
			listarray_append(lst, &file);
			if (IsMatchFcb(&file, lpMask, nAttr))
			{
				hdd = (HDRVLST)listarray_append(ret, NULL);
				if (hdd != NULL)
				{
					hdd->file = file;
					file_cpyname(hdd->szFilename, OEMTEXT("."), NELEMENTS(hdd->szFilename));
				}
			}

			file = phdp->file;
			memcpy(file.fcbname, s_parent, 11);
			listarray_append(lst, &file);
			if (IsMatchFcb(&file, lpMask, nAttr))
			{
				hdd = (HDRVLST)listarray_append(ret, NULL);
				if (hdd != NULL)
				{
					hdd->file = file;
					file_cpyname(hdd->szFilename, OEMTEXT(".."), NELEMENTS(hdd->szFilename));
				}
			}
		}

		flh = file_list1st(phdp->szPath, &fli);
		if (flh != FLISTH_INVALID)
		{
			do
			{
				RealName2Fcb(file.fcbname, fli.path);
				if ((file.fcbname[0] == ' ') || (listarray_enum(lst, IsMatchName, file.fcbname) != NULL))
				{
					continue;
				}

				file.caps = fli.caps;
				file.size = fli.size;
				file.attr = fli.attr;
				file.date = fli.date;
				file.time = fli.time;
				listarray_append(lst, &file);
				if (IsMatchFcb(&file, lpMask, nAttr))
				{
					hdd = (HDRVLST)listarray_append(ret, NULL);
					if (hdd != NULL)
					{
						hdd->file = file;
						file_cpyname(hdd->szFilename, fli.path, NELEMENTS(hdd->szFilename));
					}
				}
			} while (file_listnext(flh, &fli) == SUCCESS);
			file_listclose(flh);
		}
		if (listarray_getitems(ret) == 0)
		{
			listarray_destroy(ret);
			ret = NULL;
		}
		listarray_destroy(lst);
	}
	return ret;
}

/* ---- */

/**
 * DOS 名を FCB に変換
 * @param[out] lpFcbname FCB
 * @param[in] cchFcbname FCB バッファ サイズ
 * @param[in] lpDosPath DOS パス
 * @return 次の DOS パス
 */
static const char *DosPath2FcbSub(char *lpFcbname, UINT cchFcbname, const char *lpDosPath)
{
	char c;

	while (cchFcbname)
	{
		c = lpDosPath[0];
#if defined(_WIN32)
		if ((c == 0) || (c == '.') || (c == '\\'))
#else	/* _WIN32 */
		if ((c == 0) || (c == '.') || (c == '/'))
#endif	/* _WIN32 */
		{
			break;
		}
		if ((((c ^ 0x20) - 0xa1) & 0xff) < 0x3c)
		{
			if (lpDosPath[1] == '\0')
			{
				break;
			}
			if (cchFcbname < 2)
			{
				break;
			}
			lpDosPath++;
			lpFcbname[0] = c;
			lpFcbname[1] = *lpDosPath;
			lpFcbname += 2;
			cchFcbname -= 2;
		}
		else
		{
			*lpFcbname++ = c;
			cchFcbname--;
		}
		lpDosPath++;
	}
	return lpDosPath;
}

/**
 * DOS 名を FCB に変換
 * @param[out] lpFcbname FCB
 * @param[in] lpDosPath DOS パス
 * @return 次の DOS パス
 */
static const char *DosPath2Fcb(char *lpFcbname, const char *lpDosPath)
{
	FillMemory(lpFcbname, 11, ' ');
	lpDosPath = DosPath2FcbSub(lpFcbname, 8, lpDosPath);
	if (lpDosPath[0] == '.')
	{
		lpDosPath = DosPath2FcbSub(lpFcbname + 8, 3, lpDosPath + 1);
	}
	return lpDosPath;
}

/**
 * パス検索
 * @param[in,out] phdp HostDrv パス
 * @param[in] lpFcbname FCB 名
 * @retval SUCCESS 成功
 * @retval FAILURE 失敗
 */
static BRESULT FindSinglePath(HDRVPATH *phdp, const char *lpFcbname)
{
	BOOL r;
	FLISTH flh;
	FLINFO fli;
	char fcbname[11];

	r = FALSE;
	flh = file_list1st(phdp->szPath, &fli);
	if (flh != FLISTH_INVALID)
	{
		do
		{
			RealName2Fcb(fcbname, fli.path);
			if (memcmp(fcbname, lpFcbname, 11) == 0)
			{
				memcpy(phdp->file.fcbname, fcbname, 11);
				phdp->file.caps = fli.caps;
				phdp->file.size = fli.size;
				phdp->file.attr = fli.attr;
				phdp->file.date = fli.date;
				phdp->file.time = fli.time;
				file_setseparator(phdp->szPath, NELEMENTS(phdp->szPath));
				file_catname(phdp->szPath, fli.path, NELEMENTS(phdp->szPath));
				r = TRUE;
				break;
			}
		} while (file_listnext(flh, &fli) == SUCCESS);
		file_listclose(flh);
	}
	return (r) ? SUCCESS : FAILURE;
}

/**
 * ディレクトリを得る
 * @param[out] phdp HostDrv パス
 * @param[out] lpFcbname FCB 名
 * @param[in] lpDosPath DOS パス
 * @return DOS エラー コード
 */
BOOL PathIsRelative(char *path) {
	if(path[0] == '/') {
		return TRUE;
	}
	return FALSE;
}

#define _countof(array) (sizeof(array) / sizeof(array[0]))

UINT hostdrvs_getrealdir(HDRVPATH *phdp, char *lpFcbname, const char *lpDosPath)
{
	phdp->file = s_hddroot;
	if(PathIsRelative(np2cfg.hdrvroot)){
		TCHAR pathbuf[MAX_PATH+1];
		TCHAR *pathtmp;
		initgetfile(pathbuf, _countof(pathbuf));
#if defined(_WIN32)
		pathtmp = strrchr(pathbuf, '\\');
#else	/* _WIN32 */
		pathtmp = strrchr(pathbuf, '/');
#endif	/* _WIN32 */
		if(pathtmp){
			*(pathtmp+1) = 0;
		}else{
			pathbuf[0] = 0;
		}
		strcat(pathbuf, np2cfg.hdrvroot);
		file_cpyname(phdp->szPath, pathbuf, NELEMENTS(phdp->szPath));
	}else{
		file_cpyname(phdp->szPath, np2cfg.hdrvroot, NELEMENTS(phdp->szPath));
	}

	if (lpDosPath[0] == '\\')
	{
		lpDosPath++;
	}
	else if (lpDosPath[0] != '\0')
	{
		return ERR_PATHNOTFOUND;
	}
	while (TRUE /*CONSTCOND*/)
	{
		lpDosPath = DosPath2Fcb(lpFcbname, lpDosPath);
		if (lpDosPath[0] != '\\')
		{
			break;
		}
		if ((FindSinglePath(phdp, lpFcbname) != SUCCESS) || ((phdp->file.attr & 0x10) == 0))
		{
			return FAILURE;
		}
		lpDosPath++;
	}
	return (lpDosPath[0] == '\0') ? ERR_NOERROR : ERR_PATHNOTFOUND;
}

/**
 * パスを結合する
 * @param[in,out] phdp HostDrv パス
 * @param[in] lpFcbname FCB 名
 * @return DOS エラー コード
 */
UINT hostdrvs_appendname(HDRVPATH *phdp, const char *lpFcbname)
{
	char szDosName[16];
	char *p;
	UINT i;
#if defined(OSLANG_EUC) || defined(OSLANG_UTF8) || defined(OSLANG_UCS2)
	OEMCHAR oemname[64];
#endif

	if (lpFcbname[0] == ' ')
	{
		return ERR_PATHNOTFOUND;
	}
	else if (FindSinglePath(phdp, lpFcbname) == SUCCESS)
	{
		return ERR_NOERROR;
	}
	else
	{
		memset(&phdp->file, 0, sizeof(phdp->file));
		memcpy(phdp->file.fcbname, lpFcbname, 11);
		file_setseparator(phdp->szPath, NELEMENTS(phdp->szPath));

		p = szDosName;
		for (i = 0; (i < 8) && (lpFcbname[i] != ' '); i++)
		{
			*p++ = lpFcbname[i];
		}
		if (lpFcbname[8] != ' ')
		{
			*p++ = '.';
			for (i = 8; (i < 11) && (lpFcbname[i] != ' '); i++)
			{
				*p++ = lpFcbname[i];
			}
		}
		*p = '\0';
#if defined(OSLANG_EUC) || defined(OSLANG_UTF8) || defined(OSLANG_UCS2)
		oemtext_sjistooem(oemname, NELEMENTS(oemname), szDosName, (UINT)-1);
		file_catname(phdp->szPath, oemname, NELEMENTS(phdp->szPath));
#else
		file_catname(phdp->szPath, szDosName, NELEMENTS(phdp->szPath));
#endif
		return ERR_FILENOTFOUND;
	}
}

/**
 * パスを得る
 * @param[out] phdp HostDrv パス
 * @param[in] lpDosPath DOS パス
 * @return DOS エラー コード
 */
UINT hostdrvs_getrealpath(HDRVPATH *phdp, const char *lpDosPath)
{
	char fcbname[11];
	UINT nResult;
	
	if (lpDosPath[0] == '\0') // root check
	{
		return ERR_NOERROR;
	}
	nResult = hostdrvs_getrealdir(phdp, fcbname, lpDosPath);
	if (nResult == ERR_NOERROR)
	{
		nResult = hostdrvs_appendname(phdp, fcbname);
	}
	return nResult;
}

/* ---- */

/**
 * ファイルハンドルをクローズする
 * @param[in] vpItem アイテム
 * @param[in] vpArg ユーザ引数
 * @retval FALSE 継続
 */
static BOOL CloseFileHandle(void *vpItem, void *vpArg)
{
	INTPTR fh;

	fh = ((HDRVHANDLE)vpItem)->hdl;
	if (fh != (INTPTR)FILEH_INVALID)
	{
		((HDRVHANDLE)vpItem)->hdl = (INTPTR)FILEH_INVALID;
		file_close((FILEH)fh);
	}
	(void)vpArg;
	return FALSE;
}

/**
 * すべてクローズ
 * @param[in] fileArray ファイル リスト ハンドル
 */
void hostdrvs_fhdlallclose(LISTARRAY fileArray)
{
	listarray_enum(fileArray, CloseFileHandle, NULL);
}

/**
 * 空ハンドルを見つけるコールバック
 * @param[in] vpItem アイテム
 * @param[in] vpArg ユーザ引数
 * @retval TRUE 見つかった
 * @retval FALSE 見つからなかった
 */
static BOOL IsHandleInvalid(void *vpItem, void *vpArg)
{
	if (((HDRVHANDLE)vpItem)->hdl == (INTPTR)FILEH_INVALID)
	{
		return TRUE;
	}
	(void)vpArg;
	return FALSE;
}

/**
 * 新しいハンドルを得る
 * @param[in] fileArray ファイル リスト ハンドル
 * @return 新しいハンドル
 */
HDRVHANDLE hostdrvs_fhdlsea(LISTARRAY fileArray)
{
	HDRVHANDLE ret;

	if (fileArray == NULL)
	{
		TRACEOUT(("hostdrvs_fhdlsea hdl == NULL"));
	}
	ret = (HDRVHANDLE)listarray_enum(fileArray, IsHandleInvalid, NULL);
	if (ret == NULL)
	{
		ret = (HDRVHANDLE)listarray_append(fileArray, NULL);
		if (ret != NULL)
		{
			ret->hdl = (INTPTR)FILEH_INVALID;
		}
	}
	return ret;
}

#endif
