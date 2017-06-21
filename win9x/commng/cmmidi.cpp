/**
 * @file	cmmidi.cpp
 * @brief	MIDI クラスの動作の定義を行います
 */

#include "compiler.h"
#include "cmmidi.h"
#include "np2.h"

#include "cmmidiin32.h"
#include "cmmidiout32.h"
#if defined(MT32SOUND_DLL)
#include "cmmidioutmt32sound.h"
#endif	// defined(MT32SOUND_DLL)
#if defined(VERMOUTH_LIB)
#include "cmmidioutvermouth.h"
#endif	// defined(VERMOUTH_LIB)
#if defined(SUPPORT_VSTi)
#include "cmmidioutvst.h"
#endif	// defined(SUPPORT_VSTi)
#include "keydisp.h"

#define MIDIOUTS(a, b, c)	(((c) << 16) + (b << 8) + (a))
#define MIDIOUTS2(a)		(*(UINT16 *)(a))
#define MIDIOUTS3(a)		((*(UINT32 *)(a)) & 0xffffff)

const TCHAR cmmidi_midimapper[] = TEXT("MIDI MAPPER");
const TCHAR cmmidi_midivst[] = TEXT("VSTHost-Sound Canvas VA");

#if defined(VERMOUTH_LIB)
const TCHAR cmmidi_vermouth[] = TEXT("VERMOUTH");
#endif
#if defined(MT32SOUND_DLL)
const TCHAR cmmidi_mt32sound[] = TEXT("MT32Sound");
#endif

LPCTSTR cmmidi_mdlname[12] = {
		TEXT("MT-32"),	TEXT("CM-32L"),		TEXT("CM-64"),
		TEXT("CM-300"),	TEXT("CM-500(LA)"),	TEXT("CM-500(GS)"),
		TEXT("SC-55"),	TEXT("SC-88"),		TEXT("LA"),
		TEXT("GM"),		TEXT("GS"),			TEXT("XG")};

enum {		MIDI_MT32 = 0,	MIDI_CM32L,		MIDI_CM64,
			MIDI_CM300,		MIDI_CM500LA,	MIDI_CM500GS,
			MIDI_SC55,		MIDI_SC88,		MIDI_LA,
			MIDI_GM,		MIDI_GS,		MIDI_XG,	MIDI_OTHER};

static const UINT8 EXCV_MTRESET[] = {
			0xf0,0x41,0x10,0x16,0x12,0x7f,0x00,0x00,0x00,0x01,0xf7};
static const UINT8 EXCV_GMRESET[] = {
			0xf0,0x7e,0x7f,0x09,0x01,0xf7};
static const UINT8 EXCV_GM2RESET[] = {
			0xf0,0x7e,0x7f,0x09,0x03,0xf7};
static const UINT8 EXCV_GSRESET[] = {
			0xf0,0x41,0x10,0x42,0x12,0x40,0x00,0x7f,0x00,0x41,0xf7};
static const UINT8 EXCV_XGRESET[] = {
			0xf0,0x43,0x10,0x4c,0x00,0x00,0x7e,0x00,0xf7};

enum {
	MIDI_EXCLUSIVE		= 0xf0,
	MIDI_TIMECODE		= 0xf1,
	MIDI_SONGPOS		= 0xf2,
	MIDI_SONGSELECT		= 0xf3,
	MIDI_CABLESELECT	= 0xf5,
	MIDI_TUNEREQUEST	= 0xf6,
	MIDI_EOX			= 0xf7,
	MIDI_TIMING			= 0xf8,
	MIDI_START			= 0xfa,
	MIDI_CONTINUE		= 0xfb,
	MIDI_STOP			= 0xfc,
	MIDI_ACTIVESENSE	= 0xfe,
	MIDI_SYSTEMRESET	= 0xff
};


typedef CComMidi *CMMIDI;

static const UINT8 midictrltbl[] = { 0, 1, 5, 7, 10, 11, 64,
									65, 66, 67, 84, 91, 93,
									94,						// for SC-88
									71, 72, 73, 74};		// for XG

static	UINT8	midictrlindex[128];


/**
 * モジュール番号を得る
 * @param[in] lpModule モジュール名
 * @return モジュール番号
 */
UINT CComMidi::module2number(LPCTSTR lpModule)
{
	UINT i;

	for (i = 0; i < NELEMENTS(cmmidi_mdlname); i++)
	{
		if (!milstr_extendcmp(lpModule, cmmidi_mdlname[i]))
		{
			break;
		}
	}
	return i;
}

/**
 * オール ノート オフ
 */
void CComMidi::midiallnoteoff()
{
	for (UINT i = 0; i < 0x10; i++)
	{
		UINT8 msg[4];
		msg[0] = (UINT8)(0xb0 + i);
		msg[1] = 0x7b;
		msg[2] = 0x00;
		keydisp_midi(msg);
		if (m_pMidiOut)
		{
			m_pMidiOut->Short(MIDIOUTS3(msg));
		}
	}
}

/**
 * MIDI リセット
 */
void CComMidi::midireset()
{
	const UINT8* lpExcv;
	UINT cbExcv;

	switch (m_nModule)
	{
		case MIDI_GM:
			lpExcv = EXCV_GMRESET;
			cbExcv = sizeof(EXCV_GMRESET);
			break;

		case MIDI_CM300:
		case MIDI_CM500GS:
		case MIDI_SC55:
		case MIDI_SC88:
		case MIDI_GS:
			lpExcv = EXCV_GSRESET;
			cbExcv = sizeof(EXCV_GSRESET);
			break;

		case MIDI_XG:
			lpExcv = EXCV_XGRESET;
			cbExcv = sizeof(EXCV_XGRESET);
			break;

		case MIDI_MT32:
		case MIDI_CM32L:
		case MIDI_CM64:
		case MIDI_CM500LA:
		case MIDI_LA:
			lpExcv = EXCV_MTRESET;
			cbExcv = sizeof(EXCV_MTRESET);
			break;

		default:
			lpExcv = NULL;
			cbExcv = 0;
			break;
	}
	if ((lpExcv) && (m_pMidiOut))
	{
		m_pMidiOut->Long(lpExcv, cbExcv);
	}
	midiallnoteoff();
}

/**
 * パラメータ設定
 */
void CComMidi::midisetparam()
{
	if (m_pMidiOut == NULL)
	{
		return;
	}

	for (UINT i = 0; i < 16; i++)
	{
		const MIDICH* mch = &m_midich[i];
		if (mch->press != 0xff)
		{
			m_pMidiOut->Short(MIDIOUTS(0xa0 + i, mch->press, 0));
		}
		if (mch->bend != 0xffff)
		{
			m_pMidiOut->Short((mch->bend << 8) + 0xe0 + i);
		}
		for (UINT j = 0; j < NELEMENTS(midictrltbl); j++)
		{
			if (mch->ctrl[j+1] != 0xff)
			{
				m_pMidiOut->Short(MIDIOUTS(0xb0 + i, midictrltbl[j], mch->ctrl[j + 1]));
			}
		}
		if (mch->prog != 0xff)
		{
			m_pMidiOut->Short(MIDIOUTS(0xc0+i, mch->prog, 0));
		}
	}
}

/**
 * 初期化
 */
void cmmidi_initailize(void)
{
	ZeroMemory(midictrlindex, sizeof(midictrlindex));
	for (UINT i = 0; i < NELEMENTS(midictrltbl); i++)
	{
		midictrlindex[midictrltbl[i]] = (UINT8)(i + 1);
	}
	midictrlindex[32] = 1;
}

/**
 * インスタンス作成
 * @param[in] lpMidiOut MIDIOUT デバイス
 * @param[in] lpMidiIn MIDIIN デバイス
 * @param[in] lpModule モジュール
 * @return インスタンス
 */
CComMidi* CComMidi::CreateInstance(LPCTSTR lpMidiOut, LPCTSTR lpMidiIn, LPCTSTR lpModule)
{
	CComMidi* pMidi = new CComMidi;
	if (!pMidi->Initialize(lpMidiOut, lpMidiIn, lpModule))
	{
		delete pMidi;
		pMidi = NULL;
	}
	return pMidi;
}

/**
 * コンストラクタ
 */
CComMidi::CComMidi()
	: CComBase(COMCONNECT_MIDI)
	, m_pMidiIn(NULL)
	, m_pMidiOut(NULL)
	, m_nModule(MIDI_OTHER)
	, m_nMidiCtrl(MIDICTRL_READY)
	, m_nIndex(0)
	, m_nRecvSize(0)
	, m_cLastData(0)
	, m_bMimpiDef(false)
{
	ZeroMemory(&m_mimpiDef, sizeof(m_mimpiDef));
	FillMemory(m_midich, sizeof(m_midich), 0xff);
	ZeroMemory(m_sBuffer, sizeof(m_sBuffer));
}

/**
 * デストラクタ
 */
CComMidi::~CComMidi()
{
	midiallnoteoff();
	if (m_pMidiOut)
	{
		delete m_pMidiOut;
	}
	if (m_pMidiIn)
	{
		delete m_pMidiIn;
	}
}

/**
 * 初期化
 * @param[in] lpMidiOut MIDIOUT デバイス
 * @param[in] lpMidiIn MIDIIN デバイス
 * @param[in] lpModule モジュール
 * @retval true 成功
 * @retval false 失敗
 */
bool CComMidi::Initialize(LPCTSTR lpMidiOut, LPCTSTR lpMidiIn, LPCTSTR lpModule)
{
#if defined(VERMOUTH_LIB)
	if ((m_pMidiOut == NULL) && (!milstr_cmp(lpMidiOut, cmmidi_vermouth)))
	{
		m_pMidiOut = CComMidiOutVermouth::CreateInstance();
	}
#endif	// defined(VERMOUTH_LIB)
#if defined(MT32SOUND_DLL)
	if ((m_pMidiOut == NULL) && (!milstr_cmp(lpMidiOut, cmmidi_mt32sound)))
	{
		m_pMidiOut = CComMidiOutMT32Sound::CreateInstance();
	}
#endif	// defined(MT32SOUND_DLL)
#if defined(SUPPORT_VSTi)
	if ((m_pMidiOut == NULL) && (!milstr_cmp(lpMidiOut, cmmidi_midivst)))
	{
		m_pMidiOut = CComMidiOutVst::CreateInstance();
	}
#endif	// defined(SUPPORT_VSTi)
	if (m_pMidiOut == NULL)
	{
		m_pMidiOut = CComMidiOut32::CreateInstance(lpMidiOut);
	}

	m_pMidiIn = CComMidiIn32::CreateInstance(lpMidiIn);

	if ((!m_pMidiOut) && (!m_pMidiIn))
	{
		return false;
	}

	if (m_pMidiOut == NULL)
	{
		m_pMidiOut = new CComMidiOut;
	}

	m_nModule = module2number(lpModule);
	return true;
}

/**
 * 読み込み
 * @param[out] pData バッファ
 * @return サイズ
 */
UINT CComMidi::Read(UINT8* pData)
{
	if (m_pMidiIn)
	{
		return m_pMidiIn->Read(pData);
	}
	return 0;
}

/**
 * 書き込み
 * @param[out] cData データ
 * @return サイズ
 */
UINT CComMidi::Write(UINT8 cData)
{
	CMMIDI midi = this;

	switch (cData)
	{
		case MIDI_TIMING:
		case MIDI_START:
		case MIDI_CONTINUE:
		case MIDI_STOP:
		case MIDI_ACTIVESENSE:
		case MIDI_SYSTEMRESET:
			return 1;
	}
	if (m_nMidiCtrl == MIDICTRL_READY)
	{
		if (cData & 0x80)
		{
			m_nIndex = 0;
			switch (cData & 0xf0)
			{
				case 0xc0:
				case 0xd0:
					m_nMidiCtrl = MIDICTRL_2BYTES;
					break;

				case 0x80:
				case 0x90:
				case 0xa0:
				case 0xb0:
				case 0xe0:
					m_nMidiCtrl = MIDICTRL_3BYTES;
					m_cLastData = cData;
					break;

				default:
					switch (cData)
					{
						case MIDI_EXCLUSIVE:
							m_nMidiCtrl = MIDICTRL_EXCLUSIVE;
							break;

						case MIDI_TIMECODE:
							m_nMidiCtrl = MIDICTRL_TIMECODE;
							break;

						case MIDI_SONGPOS:
							m_nMidiCtrl = MIDICTRL_SYSTEM;
							m_nRecvSize = 3;
							break;

						case MIDI_SONGSELECT:
							m_nMidiCtrl = MIDICTRL_SYSTEM;
							m_nRecvSize = 2;
							break;

						case MIDI_CABLESELECT:
							m_nMidiCtrl = MIDICTRL_SYSTEM;
							m_nRecvSize = 1;
							break;

//						case MIDI_TUNEREQUEST:
//						case MIDI_EOX:
						default:
							return 1;
					}
					break;
			}
		}
		else						// Key-onのみな気がしたんだけど忘れた…
		{
			// running status
			m_sBuffer[0] = m_cLastData;
			m_nIndex = 1;
			m_nMidiCtrl = MIDICTRL_3BYTES;
		}
	}
	m_sBuffer[m_nIndex] = cData;
	m_nIndex++;

	switch (m_nMidiCtrl)
	{
		case MIDICTRL_2BYTES:
			if (m_nIndex >= 2)
			{
				m_sBuffer[1] &= 0x7f;
				MIDICH* mch = m_midich + (m_sBuffer[0] & 0xf);
				switch (m_sBuffer[0] & 0xf0)
				{
					case 0xa0:
						mch->press = m_sBuffer[1];
						break;

					case 0xc0:
						if (m_bMimpiDef)
						{
							const UINT type = m_mimpiDef.ch[m_sBuffer[0] & 0x0f];
							if (type < MIMPI_RHYTHM)
							{
								m_sBuffer[1] = m_mimpiDef.map[type][m_sBuffer[1]];
							}
						}
						mch->prog = m_sBuffer[1];
						break;
				}
				keydisp_midi(m_sBuffer);
				if (m_pMidiOut)
				{
					m_pMidiOut->Short(MIDIOUTS2(m_sBuffer));
				}
				m_nMidiCtrl = MIDICTRL_READY;
				return 2;
			}
			break;

		case MIDICTRL_3BYTES:
			if (m_nIndex >= 3)
			{
				*(UINT16 *)(m_sBuffer + 1) &= 0x7f7f;
				MIDICH* mch = m_midich + (m_sBuffer[0] & 0xf);
				switch (m_sBuffer[0] & 0xf0)
				{
					case 0xb0:
						if (m_sBuffer[1] == 123)
						{
							mch->press = 0;
							mch->bend = 0x4000;
							mch->ctrl[1+1] = 0;			// Modulation
							mch->ctrl[5+1] = 127;		// Explession
							mch->ctrl[6+1] = 0;			// Hold
							mch->ctrl[7+1] = 0;			// Portament
							mch->ctrl[8+1] = 0;			// Sostenute
							mch->ctrl[9+1] = 0;			// Soft
						}
						else
						{
							mch->ctrl[midictrlindex[m_sBuffer[1]]] = m_sBuffer[2];
						}
						break;

					case 0xe0:
						mch->bend = *(UINT16 *)(m_sBuffer + 1);
						break;
				}
				keydisp_midi(m_sBuffer);
				if (m_pMidiOut)
				{
					m_pMidiOut->Short(MIDIOUTS3(m_sBuffer));
				}
				m_nMidiCtrl = MIDICTRL_READY;
				return 3;
			}
			break;

		case MIDICTRL_EXCLUSIVE:
			if (cData == MIDI_EOX)
			{
				if (m_pMidiOut)
				{
					m_pMidiOut->Long(m_sBuffer, m_nIndex);
				}
				m_nMidiCtrl = MIDICTRL_READY;
				return m_nIndex;
			}
			else if (m_nIndex >= sizeof(m_sBuffer))		// おーばーふろー
			{
				m_nMidiCtrl = MIDICTRL_READY;
			}
			break;

		case MIDICTRL_TIMECODE:
			if (m_nIndex >= 2)
			{
				if ((cData == 0x7e) || (cData == 0x7f))
				{
					// exclusiveと同じでいい筈…
					m_nMidiCtrl = MIDICTRL_EXCLUSIVE;
				}
				else
				{
					m_nMidiCtrl = MIDICTRL_READY;
					return 2;
				}
			}
			break;

		case MIDICTRL_SYSTEM:
			if (m_nIndex >= m_nRecvSize)
			{
				m_nMidiCtrl = MIDICTRL_READY;
				return m_nRecvSize;
			}
			break;
	}
	return 0;
}

/**
 * ステータスを得る
 * @return ステータス
 */
UINT8 CComMidi::GetStat()
{
	return 0x00;
}

/**
 * メッセージ
 * @param[in] nMessage メッセージ
 * @param[in] nParam パラメタ
 * @return リザルト コード
 */
INTPTR CComMidi::Message(UINT nMessage, INTPTR nParam)
{
	switch (nMessage)
	{
		case COMMSG_MIDIRESET:
			midireset();
			return 1;

		case COMMSG_SETFLAG:
			{
				COMFLAG flag = reinterpret_cast<COMFLAG>(nParam);
				if ((flag) && (flag->size == sizeof(_COMFLAG) + sizeof(m_midich)) && (flag->sig == COMSIG_MIDI))
				{
					CopyMemory(m_midich, flag + 1, sizeof(m_midich));
					midisetparam();
					return 1;
				}
			}
			break;

		case COMMSG_GETFLAG:
			{
				COMFLAG flag = (COMFLAG)_MALLOC(sizeof(_COMFLAG) + sizeof(m_midich), "MIDI FLAG");
				if (flag)
				{
					flag->size = sizeof(_COMFLAG) + sizeof(m_midich);
					flag->sig = COMSIG_MIDI;
					flag->ver = 0;
					flag->param = 0;
					CopyMemory(flag + 1, m_midich, sizeof(m_midich));
					return reinterpret_cast<INTPTR>(flag);
				}
			}
			break;

		case COMMSG_MIMPIDEFFILE:
			::mimpidef_load(&m_mimpiDef, reinterpret_cast<LPCTSTR>(nParam));
			return 1;

		case COMMSG_MIMPIDEFEN:
			m_bMimpiDef = (nParam != 0);
			return 1;

		default:
			break;
	}
	return 0;
}
