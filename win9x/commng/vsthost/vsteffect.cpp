/**
 * @file	vsteffect.cpp
 * @brief	VST effect クラスの動作の定義を行います
 */

#include "compiler.h"
#include "vsteffect.h"
#include "vsteditwndbase.h"

#ifdef _WIN32
#include <shlwapi.h>
#include <atlbase.h>
#pragma comment(lib, "shlwapi.lib")
#else	// _WIN32
#include <dlfcn.h>
#endif	// _WIN32

/*! エフェクト ハンドラー */
std::map<AEffect*, CVstEffect*> CVstEffect::sm_effects;

/**
 * コンストラクタ
 */
CVstEffect::CVstEffect()
	: m_effect(NULL)
	, m_hModule(NULL)
	, m_lpDir(NULL)
	, m_pWnd(NULL)
{
}

/**
 * デストラクタ
 */
CVstEffect::~CVstEffect()
{
	Unload();
}

/**
 * ロードする
 * @param[in] lpVst プラグイン
 * @retval true 成功
 * @retval false 失敗
 */
bool CVstEffect::Load(LPCTSTR lpVst)
{
	Unload();

#ifdef _WIN32
	/* VSTi読み込み */
	HMODULE hModule = ::LoadLibrary(lpVst);
	if (hModule == NULL)
	{
		return false;
	}
	typedef AEffect* (*FnMain)(::audioMasterCallback audioMaster);
	FnMain fnMain = reinterpret_cast<FnMain>(::GetProcAddress(hModule, "VSTPluginMain"));
	if (fnMain == NULL)
	{
		fnMain = reinterpret_cast<FnMain>(::GetProcAddress(hModule, "main"));
	}
	if (fnMain == NULL)
	{
		::FreeLibrary(hModule);
		return false;
	}

	// 初期化
	AEffect* effect = (*fnMain)(cAudioMasterCallback);
	if (effect == NULL)
	{
		::FreeLibrary(hModule);
		return false;
	}
	if (effect->magic != kEffectMagic)
	{
		::FreeLibrary(hModule);
		return false;
	}

	TCHAR szDir[MAX_PATH];
	::lstrcpyn(szDir, lpVst, _countof(szDir));
	::PathRemoveFileSpec(szDir);

	USES_CONVERSION;
	m_lpDir = ::strdup(T2A(szDir));

#else	// _WIN32

	/* VSTi読み込み */
	void* hModule = ::dlopen(lpVst, 262);
	if (hModule == NULL)
	{
		return false;
	}
	typedef AEffect* (*FnMain)(::audioMasterCallback audioMaster);
	FnMain fnMain = reinterpret_cast<FnMain>(::dlsym(hModule, "VSTPluginMain"));
	if (fnMain == NULL)
	{
		fnMain = reinterpret_cast<FnMain>(::dlsym(hModule, "main"));
	}
	if (fnMain == NULL)
	{
		::dlclose(hModule);
		return false;
	}

	// 初期化
	AEffect* effect = (*fnMain)(cAudioMasterCallback);
	if (effect == NULL)
	{
		::dlclose(hModule);
		return false;
	}
	if (effect->magic != kEffectMagic)
	{
		::dlclose(hModule);
		return false;
	}

	m_lpDir = ::strdup(lpVst);
	char* pSlash = strrchr(m_lpDir, '/');
	if (pSlash)
	{
		*(pSlash + 1) = 0;
	}
	else
	{
		free(m_lpDir);
		m_lpDir = NULL;
	}
#endif

	printf("%d input(s), %d output(s)\n", effect->numInputs, effect->numOutputs);

	sm_effects[effect] = this;
	m_effect = effect;
	m_hModule = hModule;

	return true;
}

/**
 * アンロードする
 */
void CVstEffect::Unload()
{
	if (m_effect)
	{
		sm_effects.erase(m_effect);
		m_effect = NULL;
	}
	if (m_hModule)
	{
#ifdef _WIN32
		::FreeLibrary(m_hModule);
#else	// _WIN32
		::dlclose(m_hModule);
#endif	// _WIN32
		m_hModule = NULL;
	}
	if (m_lpDir)
	{
		free(m_lpDir);
		m_lpDir = NULL;
	}
}

/**
 * ウィンドウ アタッチ
 * @param[in] pWnd ハンドル
 * @return 以前のハンドル
 */
IVstEditWnd* CVstEffect::Attach(IVstEditWnd* pWnd)
{
	IVstEditWnd* pRet = m_pWnd;
	m_pWnd = pWnd;
	return pRet;
}

/**
 * ディスパッチ
 * @param[in] opcode The operation code
 * @param[in] index The index
 * @param[in] value The value
 * @param[in] ptr The pointer
 * @param[in] opt The option
 * @return The result
 */
VstIntPtr CVstEffect::dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
	if (m_effect)
	{
		return (*m_effect->dispatcher)(m_effect, opcode, index, value, ptr, opt);
	}
	return 0;
}

/**
 * プロセス
 * @param[in] inputs 入力
 * @param[in] outputs 出力
 * @param[in] sampleFrames サンプル数
 */
void CVstEffect::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames)
{
	if (m_effect)
	{
		(*m_effect->processReplacing)(m_effect, inputs, outputs, sampleFrames);
	}
}

/**
 * コールバック
 * @param[in] effect The instance of effect
 * @param[in] opcode The operation code
 * @param[in] index The index
 * @param[in] value The value
 * @param[in] ptr The pointer
 * @param[in] opt The option
 * @return The result
 */
VstIntPtr CVstEffect::cAudioMasterCallback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
	switch (opcode)
	{
		case audioMasterVersion:
			return 2400;

		default:
			break;
	}

	std::map<AEffect*, CVstEffect*>::iterator it = sm_effects.find(effect);
	if (it != sm_effects.end())
	{
		return it->second->audioMasterCallback(opcode, index, value, ptr, opt);
	}
	return 0;
}

/**
 * コールバック
 * @param[in] opcode The operation code
 * @param[in] index The index
 * @param[in] value The value
 * @param[in] ptr The pointer
 * @param[in] opt The option
 * @return The result
 */
VstIntPtr CVstEffect::audioMasterCallback(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
	VstIntPtr ret = 0;

	switch (opcode)
	{
		case audioMasterGetDirectory:
			ret = reinterpret_cast<VstIntPtr>(m_lpDir);
			break;

		case DECLARE_VST_DEPRECATED(audioMasterWantMidi):
			break;

		case audioMasterSizeWindow:
			if (m_pWnd)
			{
				ret = m_pWnd->OnResize(index, static_cast<VstInt32>(value));
			}
			break;

		case audioMasterUpdateDisplay:
			if (m_pWnd)
			{
				ret = m_pWnd->OnUpdateDisplay();
			}
			break;

		default:
			printf("callback: AudioMasterCallback %d %d\n", opcode, index);
			break;
	}

	return ret;
}
