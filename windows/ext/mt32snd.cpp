#include "compiler.h"

#if defined(MT32SOUND_DLL)

#include "mt32snd.h"

MT32Sound MT32Sound::sm_instance;

// DLL名
static const TCHAR s_szMT32SoundDll[] = TEXT("mt32sound.dll");

static const char fn_mt32soundopen[] = "MT32Sound_Open";
static const char fn_mt32soundclose[] = "MT32Sound_Close";
static const char fn_mt32soundwrite[] = "MT32Sound_Write";
static const char fn_mt32soundmix[] = "MT32Sound_Mix";

/**
 * コンストラクタ
 */
MT32Sound::MT32Sound()
	: m_hModule(NULL)
	, m_bOpened(false)
	, m_nRate(0)
	, m_fnOpen(NULL)
	, m_fnClose(NULL)
	, m_fnWrite(NULL)
	, m_fnMix(NULL)
{
}

/**
 * デストラクタ
 */
MT32Sound::~MT32Sound()
{
	Deinitialize();
}

/**
 * 初期化
 * @retval true 成功
 * @retval false 失敗
 */
bool MT32Sound::Initialize()
{
	Deinitialize();

	m_hModule = ::LoadLibrary(s_szMT32SoundDll);
	if (m_hModule == NULL)
	{
		return false;
	}

	//! ロード関数リスト
	static const ProcItem s_dllProc[] =
	{
		{fn_mt32soundopen,	offsetof(MT32Sound, m_fnOpen)},
		{fn_mt32soundclose,	offsetof(MT32Sound, m_fnClose)},
		{fn_mt32soundwrite,	offsetof(MT32Sound, m_fnWrite)},
		{fn_mt32soundmix,	offsetof(MT32Sound, m_fnMix)}
	};

	for (size_t i = 0; i < _countof(s_dllProc); i++)
	{
		FARPROC proc = ::GetProcAddress(m_hModule, s_dllProc[i].lpSymbol);
		if (proc == NULL)
		{
			Deinitialize();
			return false;
		}
		*(reinterpret_cast<FARPROC*>(reinterpret_cast<INT_PTR>(this) + s_dllProc[i].nOffset)) = proc;
	}
	return true;
}

/**
 * 解放
 */
void MT32Sound::Deinitialize()
{
	if (m_hModule)
	{
		::FreeLibrary(m_hModule);
	}
	m_hModule = NULL;
	m_bOpened = false;
	m_fnOpen = NULL;
	m_fnClose = NULL;
	m_fnWrite = NULL;
	m_fnMix = NULL;
}

/**
 * オープン
 * @retval true 成功
 * @retval false 失敗
 */
bool MT32Sound::Open()
{
	if ((m_fnOpen != NULL) && (!m_bOpened) && (m_nRate))
	{
		(*m_fnOpen)(m_nRate, 0, 0, 0, 0, 0);
		m_bOpened = true;
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * クローズ
 */
void MT32Sound::Close()
{
	if (m_bOpened)
	{
		if (m_fnClose)
		{
			(*m_fnClose)();
		}
		m_bOpened = false;
	}
}

/**
 * ショート メッセージ
 * @param[in] msg メッセージ
 */
void MT32Sound::ShortMsg(UINT32 msg)
{
	FnWrite fnWrite = m_fnWrite;
	if (fnWrite == NULL)
	{
		return;
	}

	switch ((msg >> 4) & (0xf0 >> 4))
	{
		case 0xc0 >> 4:
		case 0xd0 >> 4:
			(*fnWrite)(static_cast<UINT8>(msg >> 0));
			(*fnWrite)(static_cast<UINT8>(msg >> 8));
			break;

		case 0x80 >> 4:
		case 0x90 >> 4:
		case 0xa0 >> 4:
		case 0xb0 >> 4:
		case 0xe0 >> 4:
			(*fnWrite)(static_cast<UINT8>(msg >> 0));
			(*fnWrite)(static_cast<UINT8>(msg >> 8));
			(*fnWrite)(static_cast<UINT8>(msg >> 16));
			break;
	}
}

/**
 * ロング メッセージ
 * @param[in] lpBuffer バッファ
 * @param[in] cchBuffer バッファ長
 */
void MT32Sound::LongMsg(const UINT8* lpBuffer, UINT cchBuffer)
{
	if (lpBuffer == NULL)
	{
		return;
	}

	FnWrite fnWrite = m_fnWrite;
	if (fnWrite == NULL)
	{
		return;
	}

	for (UINT i = 0; i < cchBuffer; i++)
	{
		(*fnWrite)(lpBuffer[i]);
	}
}

/**
 * ミックス
 * @param[in,out] lpBuffer バッファ
 * @param[in] cchBuffer サンプル数
 * @return 出力サンプル数
 */
UINT MT32Sound::Mix(SINT32* lpBuffer, UINT cchBuffer)
{

	UINT ret = 0;
	while (cchBuffer)
	{
		SINT16 sSamples[512 * 2];
		const UINT nLength = min(cchBuffer, 512);
		(*m_fnMix)(sSamples, nLength);
		for (UINT i = 0; i < nLength; i++)
		{
			lpBuffer[i * 2 + 0] += sSamples[i * 2 + 0] * 2;
			lpBuffer[i * 2 + 1] += sSamples[i * 2 + 1] * 2;
		}
		lpBuffer += nLength * 2;
		cchBuffer -= nLength;
		ret += nLength;
	}
	return ret;
}
#endif
