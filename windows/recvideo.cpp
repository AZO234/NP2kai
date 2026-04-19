/*!
 * @file	revcideo.cpp
 * @brief	録画クラスの宣言およびインターフェイスの定義をします
 */

#include "compiler.h"

#if defined(SUPPORT_RECVIDEO)

#include "recvideo.h"
#include "pccore.h"
#include "iocore.h"
#include "scrndraw.h"
#include "dispsync.h"
#include "palettes.h"
#include "dosio.h"

#pragma comment(lib, "vfw32.lib")

#define	VIDEO_WIDTH		640
#define	VIDEO_HEIGHT	400
#define	VIDEO_FPS		30

RecodeVideo RecodeVideo::sm_instance;

/**
 * ヘッダー定義
 */
static const BITMAPINFOHEADER s_bmih =
{
	sizeof(BITMAPINFOHEADER),
	VIDEO_WIDTH, VIDEO_HEIGHT, 1, 24, BI_RGB,
	VIDEO_WIDTH * VIDEO_HEIGHT * 3, 0, 0, 0, 0
};

// ----

/**
 * コンストラクタ
 */
RecodeVideo::RecodeVideo()
	: m_bEnabled(false)
	, m_bDirty(false)
	, m_nStep(0)
	, m_pWork8(NULL)
	, m_pWork24(NULL)
	, m_pAvi(NULL)
	, m_pStm(NULL)
	, m_pStmTmp(NULL)
	, m_nFrame(0)
#if defined(AVI_SPLIT_SIZE)
	, m_nNumber(0)
	, m_dwSize(0)
#endif	// defined(AVI_SPLIT_SIZE)
{
	::AVIFileInit();

	ZeroMemory(&m_bmih, sizeof(m_bmih));
	ZeroMemory(&m_cv, sizeof(m_cv));

#if defined(AVI_SPLIT_SIZE)
	ZeroMemory(m_szPath, sizeof(m_szPath));
#endif	// defined(AVI_SPLIT_SIZE)
}

/**
 * デストラクタ
 */
RecodeVideo::~RecodeVideo()
{
	Close();
	::AVIFileExit();
}

/**
 * ファイル オープン
 * @param[in] lpFilename ファイル名
 * @retval true 成功
 * @retval false 失敗
 */
bool RecodeVideo::OpenFile(LPCTSTR lpFilename)
{
#if defined(AVI_SPLIT_SIZE)
	if (lpFilename)
	{
		m_nNumber = 0;
		::file_cpyname(m_szPath, lpFilename, NELEMENTS(m_szPath));
	}

	TCHAR szExt[16];
	::wsprintf(szExt, _T("_%04d.avi"), m_nNumber);

	TCHAR szPath[MAX_PATH];
	::file_cpyname(szPath, m_szPath, NELEMENTS(szPath));
	::file_cutext(szPath);
	::file_catname(szPath, szExt, NELEMENTS(szPath));

	lpFilename = szPath;
#endif	// defined(AVI_SPLIT_SIZE)

	if (::AVIFileOpen(&m_pAvi, lpFilename, OF_CREATE | OF_WRITE | OF_SHARE_DENY_NONE, NULL) != 0)
	{
		return false;
	}

	AVISTREAMINFO si =
	{
		streamtypeVIDEO,
		comptypeDIB,
		0, 0, 0, 0,
		1, VIDEO_FPS,
		0, (DWORD)-1, 0, 0, (DWORD)-1, 0,
		{0, 0, VIDEO_WIDTH, VIDEO_HEIGHT}, 0, 0, _T("Video #1")
	};
	si.fccHandler = m_cv.fccHandler;

	if (::AVIFileCreateStream(m_pAvi, &m_pStm, &si) != 0)
	{
		return false;
	}

	AVICOMPRESSOPTIONS opt;
	opt.fccType = streamtypeVIDEO;
	opt.fccHandler = m_cv.fccHandler;
	opt.dwKeyFrameEvery = m_cv.lKey;
	opt.dwQuality = m_cv.lQ;
	opt.dwBytesPerSecond = m_cv.lDataRate;
	opt.dwFlags = (m_cv.lDataRate > 0) ? AVICOMPRESSF_DATARATE : 0;
	opt.dwFlags |= (m_cv.lKey > 0) ? AVICOMPRESSF_KEYFRAMES : 0;
	opt.lpFormat = NULL;
	opt.cbFormat = 0;
	opt.lpParms = m_cv.lpState;
	opt.cbParms = m_cv.cbState;
	opt.dwInterleaveEvery = 0;
	if (::AVIMakeCompressedStream(&m_pStmTmp, m_pStm, &opt, NULL) != AVIERR_OK)
	{
		return false;
	}
	if (::AVIStreamSetFormat(m_pStmTmp, 0, &m_bmih, sizeof(m_bmih)) != 0)
	{
		return false;
	}

	m_bEnabled = true;

	m_bDirty = true;

	m_nFrame = 0;

#if defined(AVI_SPLIT_SIZE)
	m_nNumber++;
	m_dwSize = 0;
#endif	// defined(AVI_SPLIT_SIZE)

	return true;
}

/**
 * ファイル クローズ
 */
void RecodeVideo::CloseFile()
{
	m_bEnabled = false;

	if (m_pStmTmp)
	{
		::AVIStreamRelease(m_pStmTmp);
		m_pStmTmp = NULL;
	}
	if (m_pStm)
	{
		::AVIStreamRelease(m_pStm);
		m_pStm = NULL;
	}
	if (m_pAvi)
	{
		::AVIFileRelease(m_pAvi);
		m_pAvi = NULL;
	}
}

/**
 * 開く
 * @param[in] hWnd ウィンドウ ハンドル
 * @param[in] lpFilename ファイル名
 * @retval true 成功
 * @retval false 失敗
 */
bool RecodeVideo::Open(HWND hWnd, LPCTSTR lpFilename)
{
	ZeroMemory(&m_cv, sizeof(m_cv));
	m_cv.cbSize = sizeof(m_cv);
	m_cv.dwFlags = ICMF_COMPVARS_VALID;
	m_cv.fccHandler = comptypeDIB;
	m_cv.lQ = ICQUALITY_DEFAULT;

	m_bmih = s_bmih;
	if (!::ICCompressorChoose(hWnd, ICMF_CHOOSE_DATARATE | ICMF_CHOOSE_KEYFRAME, &m_bmih, NULL, &m_cv, NULL))
	{
		return false;
	}

	m_pWork8 = new UINT8 [SURFACE_WIDTH * SURFACE_HEIGHT];
	ZeroMemory(m_pWork8, SURFACE_WIDTH * SURFACE_HEIGHT);

	m_pWork24 = new UINT8 [VIDEO_WIDTH * VIDEO_HEIGHT * 3];
	ZeroMemory(m_pWork24, VIDEO_WIDTH * VIDEO_HEIGHT * 3);

	m_nStep = 0;

	return OpenFile(lpFilename);
}

/**
 * 閉じる
 */
void RecodeVideo::Close()
{
	CloseFile();

	::ICCompressorFree(&m_cv);
	ZeroMemory(&m_cv, sizeof(m_cv));

	if (m_pWork8)
	{
		delete[] m_pWork8;
		m_pWork8 = NULL;
	}
	if (m_pWork24)
	{
		delete[] m_pWork24;
		m_pWork24 = NULL;
	}
}

/**
 * フレームを書き込む
 */
void RecodeVideo::Write()
{
	if (!m_bEnabled)
	{
		return;
	}

	while (m_nStep >= 0)
	{
		UINT8* pBuffer = m_pWork24;
		UINT nBufferSize = 0;
		DWORD dwFlags = 0;
		if (m_bDirty)
		{
			nBufferSize = VIDEO_WIDTH * VIDEO_HEIGHT * 3;
			dwFlags = AVIIF_KEYFRAME;
		}
		LONG lSize = 0;
		if (::AVIStreamWrite(m_pStmTmp, m_nFrame, 1, pBuffer, nBufferSize, dwFlags, NULL, &lSize) != 0)
		{
			break;
		}
		m_bDirty = false;
		m_nStep -= 21052600 / 8;
		m_nFrame++;

#if defined(AVI_SPLIT_SIZE)
		m_dwSize += lSize;
		if (m_dwSize >= AVI_SPLIT_SIZE)
		{
			CloseFile();
			OpenFile(NULL);
		}
#endif	// defined(AVI_SPLIT_SIZE)
	}
	m_nStep += 106 * 440 * VIDEO_FPS;
}

// ----

static void screenmix1(UINT8* dest, const UINT8* src1, const UINT8* src2)
{
	for (int i = 0; i < (SURFACE_WIDTH * SURFACE_HEIGHT); i++)
	{
		dest[i] = src1[i] + src2[i] + NP2PAL_GRPH;
	}
}

static void screenmix2(UINT8* dest, const UINT8* src1, const UINT8* src2)
{
	for (int y = 0; y < (SURFACE_HEIGHT / 2); y++)
	{
		for (int x1 = 0; x1 < SURFACE_WIDTH; x1++)
		{
			dest[x1] = src1[x1] + src2[x1] + NP2PAL_GRPH;
		}
		dest += SURFACE_WIDTH;
		src1 += SURFACE_WIDTH;
		src2 += SURFACE_WIDTH;
		for (int x2 = 0; x2 < SURFACE_WIDTH; x2++)
		{
			dest[x2] = (src1[x2] >> 4) + NP2PAL_TEXT;
		}
		dest += SURFACE_WIDTH;
		src1 += SURFACE_WIDTH;
		src2 += SURFACE_WIDTH;
	}
}

static void screenmix3(UINT8* dest, const UINT8* src1, const UINT8* src2)
{
	for (int y = 0; y < (SURFACE_HEIGHT / 2); y++)
	{
		// dest == src1, dest == src2 の時があるので…
		for (int x = 0; x < SURFACE_WIDTH; x++)
		{
			UINT8 c = (src1[x + SURFACE_WIDTH]) >> 4;
			if (c == 0)
			{
				c = src2[x] + NP2PAL_SKIP;
			}
			dest[x + SURFACE_WIDTH] = c;
			dest[x] = src1[x] + src2[x] + NP2PAL_GRPH;
		}
		dest += SURFACE_WIDTH * 2;
		src1 += SURFACE_WIDTH * 2;
		src2 += SURFACE_WIDTH * 2;
	}
}

/**
 * フレームを更新
 */
void RecodeVideo::Update()
{
	if (!m_bEnabled)
	{
		return;
	}

	void (*fnMix)(UINT8 *dest, const UINT8* src1, const UINT8* src2) = NULL;
	if (!(gdc.mode1 & 0x10))
	{
		fnMix = screenmix1;
	}
	else if (!np2cfg.skipline)
	{
		fnMix = screenmix2;
	}
	else
	{
		fnMix = screenmix3;
	}

	ZeroMemory(m_pWork8, SURFACE_WIDTH * SURFACE_HEIGHT);
	UINT8* p = m_pWork8;
	UINT8* q = m_pWork8;
	if (gdc.mode1 & 0x80)
	{
		if (gdcs.textdisp & 0x80)
		{
			p = np2_tram;
		}
		if (gdcs.grphdisp & 0x80)
		{
			q = np2_vram[gdcs.disp];
		}
	}
	(*fnMix)(m_pWork8, p, q);

	const int nWidth = min(dsync.scrnxmax, VIDEO_WIDTH);
	const int nHeight = min(dsync.scrnymax, VIDEO_HEIGHT);
	p = m_pWork8;
	q = m_pWork24 + (VIDEO_WIDTH * nHeight * 3);
	for (int y = 0; y < nHeight; y++)
	{
		q -= VIDEO_WIDTH * 3;
		UINT8* r = q;
		for (int x = 0; x < nWidth; x++)
		{
			const RGB32* pPal = np2_pal32 + p[x];
			r[0] = pPal->p.b;
			r[1] = pPal->p.g;
			r[2] = pPal->p.r;
			r += 3;
		}
		p += SURFACE_WIDTH;
	}
	m_bDirty = true;
}

#endif	// defined(SUPPORT_RECVIDEO)
