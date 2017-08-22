/**
 * @file	profile.c
 * @brief	Implementation of the profiler
 */

#include "compiler.h"
#include "strres.h"
#include "profile.h"
#include "dosio.h"
#include "textfile.h"
#if defined(SUPPORT_TEXTCNV)
#include "codecnv/textcnv.h"
#endif

/**
 * End of line style
 */
static const OEMCHAR s_eol[] =
{
#if defined(OSLINEBREAK_CR) || defined(OSLINEBREAK_CRLF)
	'\r',
#endif
#if defined(OSLINEBREAK_LF) || defined(OSLINEBREAK_CRLF)
	'\n',
#endif
};

/**
 * Trims space
 * @param[in] lpString The pointer to a string
 * @param[in, out] pcchString The size, in characters
 * @return The start of the string
 */
static OEMCHAR* TrimSpace(const OEMCHAR *lpString, UINT *pcchString)
{
	UINT cchString;

	cchString = *pcchString;
	while ((cchString > 0) && (lpString[0] == ' '))
	{
		lpString++;
		cchString--;
	}
	while ((cchString > 0) && (lpString[cchString - 1] == ' '))
	{
		cchString--;
	}
	*pcchString = cchString;
	return (OEMCHAR *)lpString;
}

/**
 * Parses line
 * @param[in] lpString The pointer to a string
 * @param[in,out] lpcchString The pointer to a length
 * @param[out] lppData The pointer to data
 * @param[out] lpcchData The pointer to data-length
 * @return The start of the string
 */
static OEMCHAR *ParseLine(const OEMCHAR *lpString, UINT *lpcchString, OEMCHAR **lppData, UINT *lpcchData)
{
	UINT cchString;
	UINT nIndex;
	const OEMCHAR *lpData = NULL;
	UINT cchData = 0;

	cchString = *lpcchString;
	lpString = TrimSpace(lpString, &cchString);

	if ((cchString >= 2) && (lpString[0] == '[') && (lpString[cchString - 1] == ']'))
	{
		lpString++;
		cchString -= 2;
	}
	else
	{
		for (nIndex = 0; nIndex < cchString; nIndex++)
		{
			if (lpString[nIndex] == '=')
			{
				break;
			}
		}
		if (nIndex >= cchString)
		{
			return NULL;
		}
		lpData = lpString + (nIndex + 1);
		cchData = cchString - (nIndex + 1);
		cchString = nIndex;
	}

	lpString = TrimSpace(lpString, &cchString);
	lpData = TrimSpace(lpData, &cchData);
	if ((cchData >= 2) && (lpData[0] == '\"') && (lpData[cchData - 1] == '\"'))
	{
		lpData++;
		cchData -= 2;
		lpData = TrimSpace(lpData, &cchData);
	}

	*lpcchString = cchString;
	if (lppData)
	{
		*lppData = (OEMCHAR*)lpData;
	}
	if (lpcchData)
	{
		*lpcchData = cchData;
	}
	return (OEMCHAR*)lpString;
}

/**
 * Retrieves a string from the specified section in an initialization file
 * @param[in] lpFileName The name of the initialization file
 * @param[in] lpParam An application-defined value to be passed to the callback function.
 * @param[in] lpFunc A pointer to an application-defined callback function
 * @retval SUCCESS If the function succeeds
 * @retval FAILIURE If the function fails
 */
BRESULT profile_enum(const OEMCHAR *lpFileName, void *lpParam, PROFILEENUMPROC lpFunc)
{
	TEXTFILEH fh;
	BRESULT r;
	OEMCHAR szAppName[256];
	OEMCHAR szBuffer[512];
	UINT cchBuffer;
	OEMCHAR *lpKeyName;
	OEMCHAR *lpString;
	UINT cchString;

	if (lpFunc == NULL)
	{
		return SUCCESS;
	}
	fh = textfile_open(lpFileName, 0x800);
	if (fh == NULL)
	{
		return SUCCESS;
	}

	r = SUCCESS;
	szAppName[0] = '\0';
	while (textfile_read(fh, szBuffer, NELEMENTS(szBuffer)) == SUCCESS)
	{
		cchBuffer = (UINT)OEMSTRLEN(szBuffer);
		lpKeyName = ParseLine(szBuffer, &cchBuffer, &lpString, &cchString);
		if (lpKeyName)
		{
			lpKeyName[cchBuffer] = '\0';
			if (lpString == NULL)
			{
				milstr_ncpy(szAppName, lpKeyName, NELEMENTS(szAppName));
			}
			else
			{
				lpString[cchString] = '\0';
				r = (*lpFunc)(lpParam, szAppName, lpKeyName, lpString);
				if (r != SUCCESS)
				{
					break;
				}
			}
		}
	}
	textfile_close(fh);
	return r;
}


/* profiler */

/**
 * @brief the structure of profiler's handle
 */
struct tagProfileHandle
{
	OEMCHAR *lpBuffer;			/*!< The pointer of buffer */
	UINT cchBuffer;				/*!< The size of buffer */
	UINT nSize;					/*!< The available */
	UINT8 szHeader[4];			/*!< The bom */
	UINT cbHeader;				/*!< The size of bom */
	UINT nFlags;				/*!< The flag */
	OEMCHAR szPath[MAX_PATH];	/*!< The file path */
};
typedef struct tagProfileHandle _PFILEH;	/*!< defines handle */

/**
 * @brief The result of search
 */
struct tagProfilePos
{
	UINT cchAppName;			/*!< The charactors of app */
	UINT cchKeyName;			/*!< The charactors of key */
	UINT nPos;					/*!< The position */
	UINT nSize;					/*!< The size of key */
	UINT apphit;				/*!< The returned flag */
	const OEMCHAR *lpString;	/*!< The pointer of the string */
	UINT cchString;				/*!< The charactors of the string */
};
typedef struct tagProfilePos	PFPOS;		/*!< defines the structure of position */

/*! default buffer size */
#define PFBUFSIZE	(1 << 8)

/**
 * Search
 * @param[in] hdl The handle
 * @param[out] pfp The pointer of returned
 * @param[in] lpAppName The name of app
 * @param[in] lpKeyName The name of key
 * @retval SUCCESS If succeeded
 * @retval FAILURE If failed
 */
static BRESULT SearchKey(PFILEH hdl, PFPOS *pfp, const OEMCHAR *lpAppName, const OEMCHAR *lpKeyName)
{
	PFPOS ret;
	const OEMCHAR *lpProfile;
	UINT cchProfile;
	UINT nIndex;
	UINT nSize;
	UINT cchLeft;
	const OEMCHAR *lpLeft;
	OEMCHAR *lpRight;
	UINT cchRight;

	if ((hdl == NULL) || (lpAppName == NULL) || (lpKeyName == NULL))
	{
		return FAILURE;
	}
	memset(&ret, 0, sizeof(ret));
	ret.cchAppName = (UINT)OEMSTRLEN(lpAppName);
	ret.cchKeyName = (UINT)OEMSTRLEN(lpKeyName);
	if ((ret.cchAppName == 0) || (ret.cchKeyName == 0))
	{
		return FAILURE;
	}

	lpProfile = hdl->lpBuffer;
	cchProfile = hdl->nSize;
	while (cchProfile > 0)
	{
		nIndex = 0;
		while ((nIndex < cchProfile) && (lpProfile[nIndex] != '\r') && (lpProfile[nIndex] != '\n'))
		{
			nIndex++;
		}
		lpLeft = lpProfile;
		cchLeft = nIndex;

		nSize = nIndex;
		if ((nSize < cchProfile) && (lpProfile[nSize] == '\r'))
		{
			nSize++;
		}
		if ((nSize < cchProfile) && (lpProfile[nSize] == '\n'))
		{
			nSize++;
		}

		lpLeft = ParseLine(lpLeft, &cchLeft, &lpRight, &cchRight);
		if (lpLeft)
		{
			if (lpRight == NULL)
			{
				if (ret.apphit)
				{
					break;
				}
				if ((cchLeft == ret.cchAppName) && (!milstr_memcmp(lpLeft, lpAppName)))
				{
					ret.apphit = 1;
				}
			}
			else if ((ret.apphit) && (cchLeft == ret.cchKeyName) && (!milstr_memcmp(lpLeft, lpKeyName)))
			{
				ret.nPos = (UINT)(lpProfile - hdl->lpBuffer);
				ret.nSize = nSize;

				ret.lpString = lpRight;
				ret.cchString = cchRight;
				break;
			}
		}

		lpProfile += nSize;
		cchProfile -= nSize;

		if (nIndex)
		{
			ret.nPos = (UINT)(lpProfile - hdl->lpBuffer);
			ret.nSize = 0;
		}
	}
	if (pfp)
	{
		*pfp = ret;
	}
	return SUCCESS;
}

/**
 * Replace
 * @param[in] hdl The handle
 * @param[in] nPos The position
 * @param[in] size1 The current size
 * @param[in] size2 The new size
 * @retval SUCCESS If succeeded
 * @retval FAILURE If failed
 */
static BRESULT replace(PFILEH hdl, UINT nPos, UINT size1, UINT size2)
{
	UINT	cnt;
	UINT	size;
	UINT	newsize;
	OEMCHAR	*p;
	OEMCHAR	*q;

	size1 += nPos;
	size2 += nPos;
	if (size1 > hdl->nSize)
	{
		return FAILURE;
	}
	cnt = hdl->nSize - size1;
	if (size1 < size2)
	{
		size = hdl->nSize + size2 - size1;
		if (size > hdl->cchBuffer)
		{
			newsize = (size & (~(PFBUFSIZE - 1))) + PFBUFSIZE;
			p = (OEMCHAR *)_MALLOC(newsize * sizeof(OEMCHAR), "profile");
			if (p == NULL)
			{
				return FAILURE;
			}
			if (hdl->lpBuffer)
			{
				CopyMemory(p, hdl->lpBuffer, hdl->cchBuffer * sizeof(OEMCHAR));
				_MFREE(hdl->lpBuffer);
			}
			hdl->lpBuffer = p;
			hdl->cchBuffer = newsize;
		}
		hdl->nSize = size;
		if (cnt)
		{
			p = hdl->lpBuffer + size1;
			q = hdl->lpBuffer + size2;
			do
			{
				--cnt;
				q[cnt] = p[cnt];
			} while (cnt);
		}
	}
	else if (size1 > size2)
	{
		hdl->nSize -= (size1 - size2);
		if (cnt)
		{
			p = hdl->lpBuffer + size1;
			q = hdl->lpBuffer + size2;
			do
			{
				*q++ = *p++;
			} while(--cnt);
		}
	}
	hdl->nFlags |= PFILEH_MODIFY;
	return SUCCESS;
}

/**
 * Regist file
 * @param[in] fh The handle of file
 * @return The handle
 */
static PFILEH registfile(FILEH fh)
{
	UINT nReadSize;
#if defined(SUPPORT_TEXTCNV)
	TCINF inf;
#endif
	UINT cbHeader;
	UINT nWidth;
	UINT8 szHeader[4];
	UINT nFileSize;
	UINT nNewSize;
	void *lpBuffer1;
#if defined(SUPPORT_TEXTCNV)
	void *lpBuffer2;
#endif
	PFILEH ret;

	nReadSize = file_read(fh, szHeader, sizeof(szHeader));
#if defined(SUPPORT_TEXTCNV)
	if (textcnv_getinfo(&inf, szHeader, nReadSize) == 0)
	{
		goto rf_err1;
	}
	if (!(inf.caps & TEXTCNV_READ))
	{
		goto rf_err1;
	}
	if ((inf.width != 1) && (inf.width != 2))
	{
		goto rf_err1;
	}
	cbHeader = inf.hdrsize;
	nWidth = inf.width;
#else
	cbHeader = 0;
	nWidth = 1;
	if ((nReadSize >= 3) && (szHeader[0] == 0xef) && (szHeader[1] == 0xbb) && (szHeader[2] == 0xbf))
	{
		// UTF-8
		cbHeader = 3;
	}
	else if ((nReadSize >= 2) && (szHeader[0] == 0xff) && (szHeader[1] == 0xfe))
	{
		// UCSLE
		cbHeader = 2;
		nWidth = 2;
#if defined(BYTESEX_BIG)
		goto rf_err1;
#endif
	}
	else if ((nReadSize >= 2) && (szHeader[0] == 0xfe) && (szHeader[1] == 0xff))
	{
		// UCS2BE
		cbHeader = 2;
		nWidth = 2;
#if defined(BYTESEX_LITTLE)
		goto rf_err1;
#endif
	}
	if (nWidth != sizeof(OEMCHAR))
	{
		goto rf_err1;
	}
#endif

	nFileSize = file_getsize(fh);
	if (nFileSize < cbHeader)
	{
		goto rf_err1;
	}
	if (file_seek(fh, (long)cbHeader, FSEEK_SET) != (long)cbHeader)
	{
		goto rf_err1;
	}
	nFileSize = (nFileSize - cbHeader) / nWidth;
	nNewSize = (nFileSize & (~(PFBUFSIZE - 1))) + PFBUFSIZE;
	lpBuffer1 = _MALLOC(nNewSize * nWidth, "profile");
	if (lpBuffer1 == NULL)
	{
		goto rf_err1;
	}
	nReadSize = file_read(fh, lpBuffer1, nNewSize * nWidth) / nWidth;
#if defined(SUPPORT_TEXTCNV)
	if (inf.xendian)
	{
		textcnv_swapendian16(lpBuffer1, nReadSize);
	}
	if (inf.tooem)
	{
		nFileSize = (inf.tooem)(NULL, 0, lpBuffer1, nReadSize);
		nNewSize = (nFileSize & (~(PFBUFSIZE - 1))) + PFBUFSIZE;
		lpBuffer2 = _MALLOC(nNewSize * sizeof(OEMCHAR), "profile tmp");
		if (lpBuffer2 == NULL)
		{
			goto rf_err2;
		}
		(inf.tooem)((OEMCHAR *)lpBuffer2, nFileSize, lpBuffer1, nReadSize);
		_MFREE(lpBuffer1);
		lpBuffer1 = lpBuffer2;
		nReadSize = nFileSize;
	}
#endif	// defined(SUPPORT_TEXTCNV)

	ret = (PFILEH)_MALLOC(sizeof(_PFILEH), "profile");
	if (ret == NULL)
	{
		goto rf_err2;
	}
	ZeroMemory(ret, sizeof(_PFILEH));
	ret->lpBuffer = (OEMCHAR *)lpBuffer1;
	ret->cchBuffer = nNewSize;
	ret->nSize = nReadSize;
	if (cbHeader)
	{
		CopyMemory(ret->szHeader, szHeader, cbHeader);
	}
	ret->cbHeader = cbHeader;
	return ret;

rf_err2:
	_MFREE(lpBuffer1);

rf_err1:
	return NULL;
}

/**
 * New file
 * @return The handle
 */
static PFILEH registnew(void)
{
	PFILEH ret;
	const UINT8 *lpHeader;
	UINT cbHeader;

	ret = (PFILEH)_MALLOC(sizeof(*ret), "profile");
	if (ret != NULL)
	{
		memset(ret, 0, sizeof(*ret));

#if defined(OSLANG_UTF8)
		lpHeader = str_utf8;
		cbHeader = sizeof(str_utf8);
#elif defined(OSLANG_UCS2) 
		lpHeader = (UINT8 *)str_ucs2;
		cbHeader = sizeof(str_ucs2);
#else
		lpHeader = NULL;
		cbHeader = 0;
#endif
		if (cbHeader)
		{
			memcpy(ret->szHeader, lpHeader, cbHeader);
		}
		ret->cbHeader = cbHeader;
	}
	return ret;
}

/**
 * Opens profiler
 * @param[in] lpFileName The name of the initialization file
 * @param[in] nFlags The flag of opening
 * @return The handle
 */
PFILEH profile_open(const OEMCHAR *lpFileName, UINT nFlags)
{
	PFILEH ret;
	FILEH fh;

	ret = NULL;
	if (lpFileName != NULL)
	{
		fh = file_open_rb(lpFileName);
		if (fh != FILEH_INVALID)
		{
			ret = registfile(fh);
			file_close(fh);
		}
		else if (nFlags & PFILEH_READONLY)
		{
		}
		else
		{
			ret = registnew();
		}
	}
	if (ret)
	{
		ret->nFlags = nFlags;
		file_cpyname(ret->szPath, lpFileName, NELEMENTS(ret->szPath));
	}
	return ret;
}

/**
 * Close
 * @param[in] hdl The handle of profiler
 */
void profile_close(PFILEH hdl)
{
	void *lpBuffer1;
	UINT cchBuffer1;
#if defined(SUPPORT_TEXTCNV)
	TCINF inf;
	void *lpBuffer2;
	UINT cchBuffer2;
#endif
	UINT cbHeader;
	UINT nWidth;
	FILEH fh;

	if (hdl == NULL)
	{
		return;
	}
	lpBuffer1 = hdl->lpBuffer;
	cchBuffer1 = hdl->nSize;
	if (hdl->nFlags & PFILEH_MODIFY)
	{
#if defined(SUPPORT_TEXTCNV)
		if (textcnv_getinfo(&inf, hdl->szHeader, hdl->cbHeader) == 0)
		{
			goto wf_err1;
		}
		if (!(inf.caps & TEXTCNV_WRITE))
		{
			goto wf_err1;
		}
		if ((inf.width != 1) && (inf.width != 2))
		{
			goto wf_err1;
		}
		if (inf.fromoem)
		{
			cchBuffer2 = (inf.fromoem)(NULL, 0, (const OEMCHAR *)lpBuffer1, cchBuffer1);
			lpBuffer2 = _MALLOC(cchBuffer2 * inf.width, "profile tmp");
			if (lpBuffer2 == NULL)
			{
				goto wf_err1;
			}
			(inf.fromoem)(lpBuffer2, cchBuffer2, (const OEMCHAR *)lpBuffer1, cchBuffer1);
			_MFREE(lpBuffer1);
			lpBuffer1 = lpBuffer2;
			cchBuffer1 = cchBuffer2;
		}
		if (inf.xendian)
		{
			textcnv_swapendian16(lpBuffer1, cchBuffer1);
		}
		cbHeader = inf.hdrsize;
		nWidth = inf.width;
#else	// defined(SUPPORT_TEXTCNV)
		cbHeader = hdl->cbHeader;
		nWidth = sizeof(OEMCHAR);
#endif	// defined(SUPPORT_TEXTCNV)
		fh = file_create(hdl->szPath);
		if (fh == FILEH_INVALID)
		{
			goto wf_err1;
		}
		if (cbHeader)
		{
			file_write(fh, hdl->szHeader, cbHeader);
		}
		file_write(fh, lpBuffer1, cchBuffer1 * nWidth);
		file_close(fh);
	}

wf_err1:
	if (lpBuffer1)
	{
		_MFREE(lpBuffer1);
	}
	_MFREE(hdl);
}

/**
 * Retrieves the names of all sections in an initialization file.
 * @param[out] lpBuffer A pointer to a buffer that receives the section names
 * @param[in] cchBuffer The size of the buffer pointed to by the lpBuffer parameter, in characters.
 * @param[in] hdl The handle of profiler
 * @return The return value specifies the number of characters copied to the specified buffer
 */
UINT profile_getsectionnames(OEMCHAR *lpBuffer, UINT cchBuffer, PFILEH hdl)
{
	UINT cchWritten = 0;
	const OEMCHAR* lpProfile;
	UINT cchProfile;
	UINT nIndex;
	OEMCHAR *lpKeyName;
	UINT cchKeyName;
	OEMCHAR *lpData;
	UINT cchRemain;

	if ((hdl == NULL) || (cchBuffer <= 1))
	{
		return 0;
	}
	lpProfile = hdl->lpBuffer;
	cchProfile = hdl->nSize;

	cchBuffer--;

	while (cchProfile > 0)
	{
		nIndex = 0;
		while ((nIndex < cchProfile) && (lpProfile[nIndex] != '\r') && (lpProfile[nIndex] != '\n'))
		{
			nIndex++;
		}

		cchKeyName = nIndex;
		lpKeyName = ParseLine(lpProfile, &cchKeyName, &lpData, NULL);
		if ((lpKeyName != NULL) && (lpData == NULL))
		{
			if (lpBuffer)
			{
				cchRemain = cchBuffer - cchWritten;
				if (cchRemain >= (cchKeyName + 1))
				{
					memcpy(lpBuffer + cchWritten, lpKeyName, cchKeyName * sizeof(*lpBuffer));
					cchWritten += cchKeyName;
					lpBuffer[cchWritten] = '\0';
					cchWritten++;
				}
			}
		}

		lpProfile += nIndex;
		cchProfile -= nIndex;

		if ((cchProfile >= 2) && (lpProfile[0] == '\r') && (lpProfile[1] == '\n'))
		{
			lpProfile++;
			cchProfile--;
		}
		if (cchProfile > 0)
		{
			lpProfile++;
			cchProfile--;
		}
	}

	if (lpBuffer)
	{
		lpBuffer[cchWritten] = '\0';
	}
	return cchWritten;
}

/**
 * Retrieves a string from the specified section in an initialization file
 * @param[in] lpAppName The name of the section containing the key name
 * @param[in] lpKeyName The name of the key whose associated string is to be retrieved
 * @param[in] lpDefault A default string
 * @param[out] lpReturnedString A pointer to the buffer that receives the retrieved string
 * @param[in] nSize The size of the buffer pointed to by the lpReturnedString parameter, in characters
 * @param[in] hdl The handle of profiler
 * @retval SUCCESS If the function succeeds
 * @retval FAILIURE If the function fails
 */
BRESULT profile_read(const OEMCHAR *lpAppName, const OEMCHAR *lpKeyName, const OEMCHAR *lpDefault, OEMCHAR *lpReturnedString, UINT nSize, PFILEH hdl)
{
	PFPOS pfp;

	if ((SearchKey(hdl, &pfp, lpAppName, lpKeyName) != SUCCESS) || (pfp.lpString == NULL))
	{
		if (lpDefault == NULL)
		{
			lpDefault = str_null;
		}
		milstr_ncpy(lpReturnedString, lpDefault, nSize);
		return FAILURE;
	}
	else
	{
		nSize = min(nSize, pfp.cchString + 1);
		milstr_ncpy(lpReturnedString, pfp.lpString, nSize);
		return SUCCESS;
	}
}

/**
 * Retrieves a string from the specified section in an initialization file
 * @param[in] lpAppName The name of the section containing the key name
 * @param[in] lpKeyName The name of the key whose associated string is to be retrieved
 * @param[in] nDefault A default value
 * @param[in] hdl The handle of profiler
 * @return The value
 */
int profile_readint(const OEMCHAR *lpAppName, const OEMCHAR *lpKeyName, int nDefault, PFILEH hdl)
{
	PFPOS pfp;
	UINT nSize;
	OEMCHAR szBuffer[32];

	if ((SearchKey(hdl, &pfp, lpAppName, lpKeyName) != SUCCESS) || (pfp.lpString == NULL))
	{
		return nDefault;
	}
	else
	{
		nSize = min(NELEMENTS(szBuffer), pfp.cchString + 1);
		milstr_ncpy(szBuffer, pfp.lpString, nSize);
		return (int)milstr_solveINT(szBuffer);
	}
}

/**
 * Copies a string into the specified section of an initialization file.
 * @param[in] lpAppName The name of the section to which the string will be copied
 * @param[in] lpKeyName The name of the key to be associated with a string
 * @param[in] lpString A null-terminated string to be written to the file
 * @param[in] hdl The handle of profiler
 * @retval SUCCESS If the function succeeds
 * @retval FAILIURE If the function fails
 */
BRESULT profile_write(const OEMCHAR *lpAppName, const OEMCHAR *lpKeyName, const OEMCHAR *lpString, PFILEH hdl)
{
	PFPOS pfp;
	UINT cchWrite;
	UINT cchString;
	OEMCHAR *lpBuffer;

	if ((hdl == NULL) || (hdl->nFlags & PFILEH_READONLY) || (lpString == NULL) || (SearchKey(hdl, &pfp, lpAppName, lpKeyName) != SUCCESS))
	{
		return FAILURE;
	}

	if (pfp.nPos != 0)
	{
		lpBuffer = hdl->lpBuffer + pfp.nPos;
		if ((lpBuffer[-1] != '\r') && (lpBuffer[-1] != '\n'))
		{
			if (replace(hdl, pfp.nPos, 0, NELEMENTS(s_eol)) != SUCCESS)
			{
				return FAILURE;
			}
			memcpy(lpBuffer, s_eol, sizeof(s_eol));
			pfp.nPos += NELEMENTS(s_eol);
		}
	}

	if (!pfp.apphit)
	{
		cchWrite = pfp.cchAppName + 2 + NELEMENTS(s_eol);
		if (replace(hdl, pfp.nPos, 0, cchWrite) != SUCCESS)
		{
			return FAILURE;
		}
		lpBuffer = hdl->lpBuffer + pfp.nPos;
		*lpBuffer++ = '[';
		memcpy(lpBuffer, lpAppName, pfp.cchAppName * sizeof(OEMCHAR));
		lpBuffer += pfp.cchAppName;
		*lpBuffer++ = ']';
		memcpy(lpBuffer, s_eol, sizeof(s_eol));
		pfp.nPos += cchWrite;
	}

	cchString = (UINT)OEMSTRLEN(lpString);
	cchWrite = pfp.cchKeyName + 1 + cchString + NELEMENTS(s_eol);
	if (replace(hdl, pfp.nPos, pfp.nSize, cchWrite) != SUCCESS)
	{
		return FAILURE;
	}
	lpBuffer = hdl->lpBuffer + pfp.nPos;
	memcpy(lpBuffer, lpKeyName, pfp.cchKeyName * sizeof(OEMCHAR));
	lpBuffer += pfp.cchKeyName;
	*lpBuffer++ = '=';
	memcpy(lpBuffer, lpString, cchString * sizeof(OEMCHAR));
	lpBuffer += cchString;
	memcpy(lpBuffer, s_eol, sizeof(s_eol));

	return SUCCESS;
}

/**
 * Copies a string into the specified section of an initialization file.
 * @param[in] lpAppName The name of the section to which the string will be copied
 * @param[in] lpKeyName The name of the key to be associated with a string
 * @param[in] nValue The value
 * @param[in] hdl The handle of profiler
 * @retval SUCCESS If the function succeeds
 * @retval FAILIURE If the function fails
 */
BRESULT profile_writeint(const OEMCHAR *lpAppName, const OEMCHAR *lpKeyName, int nValue, PFILEH hdl)
{
	OEMCHAR szBuffer[32];

	OEMSPRINTF(szBuffer, OEMTEXT("%d"), nValue);
	return profile_write(lpAppName, lpKeyName, szBuffer, hdl);

}



// ----

/**
 * Set bit
 * @param[in,out] ptr The pointer of the bit buffer
 * @param[in] nPos The posiiont
 * @param[in] set On/Off
 */
static void bitmapset(UINT8 *ptr, UINT nPos, BOOL set)
{
	UINT8 bit;

	ptr += (nPos >> 3);
	bit = 1 << (nPos & 7);
	if (set)
	{
		*ptr |= bit;
	}
	else
	{
		*ptr &= ~bit;
	}
}

/**
 * Get bit
 * @param[in] ptr The pointer of the bit buffer
 * @param[in] nPos The posiiont
 * @return The bit
 */
static BOOL bitmapget(const UINT8 *ptr, UINT nPos)
{
	return ((ptr[nPos >> 3] >> (nPos & 7)) & 1);
}

/**
 * Unserialize
 * @param[out] lpBin The pointer of the bit buffer
 * @param[in] cbBin The count of the bit
 * @param[in] lpString The string
 */
static void binset(UINT8 *lpBin, UINT cbBin, const OEMCHAR *lpString)
{
	UINT i;
	UINT8 val;
	BOOL set;
	OEMCHAR c;

	for (i = 0; i < cbBin; i++)
	{
		val = 0;
		set = FALSE;
		while (*lpString == ' ')
		{
			lpString++;
		}
		while (1)
		{
			c = *lpString;
			if ((c == '\0') || (c == ' '))
			{
				break;
			}
			else if ((c >= '0') && (c <= '9'))
			{
				val <<= 4;
				val += c - '0';
				set = TRUE;
			}
			else
			{
				c |= 0x20;
				if ((c >= 'a') && (c <= 'f'))
				{
					val <<= 4;
					val += c - 'a' + 10;
					set = TRUE;
				}
			}
			lpString++;
		}
		if (set == FALSE)
		{
			break;
		}
		lpBin[i] = val;
	}
}

/**
 * Serialize
 * @param[out] lpString The pointer of the string
 * @param[in] cchString The charactors of the string
 * @param[in] lpBin The pointer of the bit buffer
 * @param[in] cbBin The count of the bit
 */
static void binget(OEMCHAR *lpString, int cchString, const UINT8 *lpBin, UINT cbBin)
{
	UINT i;
	OEMCHAR tmp[8];

	if (cbBin)
	{
		OEMSPRINTF(tmp, OEMTEXT("%.2x"), lpBin[0]);
		milstr_ncpy(lpString, tmp, cchString);
	}
	for (i = 1; i < cbBin; i++)
	{
		OEMSPRINTF(tmp, OEMTEXT(" %.2x"), lpBin[i]);
		milstr_ncat(lpString, tmp, cchString);
	}
}

/**
 * Read
 * @param[in] lpPath The path
 * @param[in] lpApp The name of app
 * @param[in] lpTable The pointer of tables
 * @param[in] nCount The count of tables
 * @param[in] cb The callback
 */
void profile_iniread(const OEMCHAR *lpPath, const OEMCHAR *lpApp, const PFTBL *lpTable, UINT nCount, PFREAD cb)
{
	PFILEH pfh;
	const PFTBL *p;
	const PFTBL *pterm;
	OEMCHAR work[512];

	pfh = profile_open(lpPath, 0);
	if (pfh == NULL)
	{
		return;
	}
	p = lpTable;
	pterm = lpTable + nCount;
	while (p < pterm)
	{
		if (profile_read(lpApp, p->item, NULL, work, NELEMENTS(work), pfh) == SUCCESS)
		{
			switch(p->itemtype & PFTYPE_MASK)
			{
				case PFTYPE_STR:
					milstr_ncpy((OEMCHAR *)p->value, work, p->arg);
					break;

				case PFTYPE_BOOL:
					*((UINT8 *)p->value) = (!milstr_cmp(work, str_true)) ? 1 : 0;
					break;

				case PFTYPE_BITMAP:
					bitmapset((UINT8 *)p->value, p->arg, (!milstr_cmp(work, str_true)) ? TRUE : FALSE);
					break;

				case PFTYPE_BIN:
					binset((UINT8 *)p->value, p->arg, work);
					break;

				case PFTYPE_SINT8:
				case PFTYPE_UINT8:
					*(UINT8 *)p->value = (UINT32)milstr_solveINT(work);
					break;

				case PFTYPE_SINT16:
				case PFTYPE_UINT16:
					*(UINT16 *)p->value = (UINT32)milstr_solveINT(work);
					break;

				case PFTYPE_SINT32:
				case PFTYPE_UINT32:
					*(UINT32 *)p->value = (UINT32)milstr_solveINT(work);
					break;

				case PFTYPE_HEX8:
					*(UINT8 *)p->value = (UINT8)milstr_solveHEX(work);
					break;

				case PFTYPE_HEX16:
					*(UINT16 *)p->value = (UINT16)milstr_solveHEX(work);
					break;

				case PFTYPE_HEX32:
					*(UINT32 *)p->value = (UINT32)milstr_solveHEX(work);
					break;

				default:
					if (cb != NULL)
					{
						(*cb)(p, work);
					}
					break;
			}
		}
		p++;
	}
	profile_close(pfh);
}

/**
 * Write
 * @param[in] lpPath The path
 * @param[in] lpApp The name of app
 * @param[in] lpTable The pointer of tables
 * @param[in] nCount The count of tables
 * @param[in] cb The callback
 */
void profile_iniwrite(const OEMCHAR *lpPath, const OEMCHAR *lpApp, const PFTBL *lpTable, UINT nCount, PFWRITE cb)
{
	PFILEH pfh;
	const PFTBL *p;
	const PFTBL *pterm;
	const OEMCHAR *set;
	OEMCHAR work[512];

	pfh = profile_open(lpPath, 0);
	if (pfh == NULL)
	{
		return;
	}
	p = lpTable;
	pterm = lpTable + nCount;
	while (p < pterm)
	{
		if (!(p->itemtype & PFFLAG_RO))
		{
			work[0] = '\0';
			set = work;
			switch (p->itemtype & PFTYPE_MASK)
			{
				case PFTYPE_STR:
					set = (OEMCHAR *)p->value;
					break;

				case PFTYPE_BOOL:
					set = (*((UINT8 *)p->value)) ? str_true : str_false;
					break;

				case PFTYPE_BITMAP:
					set = (bitmapget((UINT8 *)p->value, p->arg)) ? str_true : str_false;
					break;

				case PFTYPE_BIN:
					binget(work, NELEMENTS(work), (UINT8 *)p->value, p->arg);
					break;

				case PFTYPE_SINT8:
					OEMSPRINTF(work, str_d, *((SINT8 *)p->value));
					break;

				case PFTYPE_SINT16:
					OEMSPRINTF(work, str_d, *((SINT16 *)p->value));
					break;

				case PFTYPE_SINT32:
					OEMSPRINTF(work, str_d, *((SINT32 *)p->value));
					break;

				case PFTYPE_UINT8:
					OEMSPRINTF(work, str_u, *((UINT8 *)p->value));
					break;

				case PFTYPE_UINT16:
					OEMSPRINTF(work, str_u, *((UINT16 *)p->value));
					break;

				case PFTYPE_UINT32:
					OEMSPRINTF(work, str_u, *((UINT32 *)p->value));
					break;

				case PFTYPE_HEX8:
					OEMSPRINTF(work, str_x, *((UINT8 *)p->value));
					break;

				case PFTYPE_HEX16:
					OEMSPRINTF(work, str_x, *((UINT16 *)p->value));
					break;

				case PFTYPE_HEX32:
					OEMSPRINTF(work, str_x, *((UINT32 *)p->value));
					break;

				default:
					if (cb != NULL)
					{
						set = (*cb)(p, work, NELEMENTS(work));
					}
					else
					{
						set = NULL;
					}
					break;
			}
			if (set)
			{
				profile_write(lpApp, p->item, set, pfh);
			}
		}
		p++;
	}
	profile_close(pfh);
}
