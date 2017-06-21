/**
 * @file	vsteffect.h
 * @brief	VST effect クラスの宣言およびインターフェイスの定義をします
 */

#pragma once

#include <map>
#include <pluginterfaces/vst2.x/aeffectx.h>

class IVstEditWnd;

/**
 * @brief VST effect クラス
 */
class CVstEffect
{
protected:
	AEffect* m_effect;		/*!< Effect */

public:
	CVstEffect();
	~CVstEffect();
	bool Load(LPCTSTR lpVst);
	void Unload();
	IVstEditWnd* Attach(IVstEditWnd* pWnd = NULL);

	void open();
	void close();
	void setProgram(VstInt32 program);
	void setSampleRate(float sampleRate);
	void setBlockSize(VstInt32 blockSize);
	void suspend();
	void resume();
	bool editGetRect(ERect** rect);
	bool editOpen(void *ptr);
	void editClose();
	void idle();
	VstIntPtr processEvents(const VstEvents* events);
	bool beginSetProgram();
	bool endSetProgram();
	VstIntPtr dispatcher(VstInt32 opcode, VstInt32 index = 0, VstIntPtr value = 0, void* ptr = NULL, float opt = 0.0f);
	void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames);

protected:
	static VstIntPtr cAudioMasterCallback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
	virtual VstIntPtr audioMasterCallback(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);

private:
#if _WIN32
	HMODULE m_hModule;			/*!< モジュール */
#else	// _WIN32
	void* m_hModule;			/*!< モジュール */
#endif
	char* m_lpDir;				/*!< ディレクトリ */
	IVstEditWnd* m_pWnd;		/*!< Window */

	static std::map<AEffect*, CVstEffect*> sm_effects;		/*!< エフェクト ハンドラー */
};

/**
 * Initialize this plugin instance
 */
inline void CVstEffect::open()
{
	dispatcher(effOpen);
}

/**
 * Deinitialize this plugin instance
 */
inline void CVstEffect::close()
{
	dispatcher(effClose);
}

/**
 * Changes the current program number
 * @param[in] program The number of program
 */
inline void CVstEffect::setProgram(VstInt32 program)
{
	dispatcher(effSetProgram, 0, program);
}

/**
 * Sets SampleRate
 * @param[in] sampleRate The rate of samples
 */
inline void CVstEffect::setSampleRate(float sampleRate)
{
	dispatcher(effSetSampleRate, 0, 0, NULL, sampleRate);
}

/**
 * Sets BlockSize
 * @param[in] blockSize The size of block
 */
inline void CVstEffect::setBlockSize(VstInt32 blockSize)
{
	dispatcher(effSetBlockSize, 0, blockSize);
}

/**
 * Switches audio processing off
 */
inline void CVstEffect::suspend()
{
	dispatcher(effMainsChanged, 0, 0);
}

/**
 * Switches audio processing on
 */
inline void CVstEffect::resume()
{
	dispatcher(effMainsChanged, 0, 1);
}

/**
 * Gets rect
 * @param[in] rect The ponter of ERect
 * @retval true If succeeded
 * @retval false If failed
 */
inline bool CVstEffect::editGetRect(ERect** rect)
{
	return (dispatcher(effEditGetRect, 0, 0, rect) != 0);
}

/**
 * Opens edit
 * @param[in] ptr The handle of window
 * @retval true If succeeded
 * @retval false If failed
 */
inline bool CVstEffect::editOpen(void *ptr)
{
	return (dispatcher(effEditOpen, 0, 0, ptr) != 0);
}

/**
 * Closes edit
 */
inline void CVstEffect::editClose()
{
	dispatcher(effEditClose);
}

/**
 * Idling edit
 */
inline void CVstEffect::idle()
{
	dispatcher(effEditIdle);
}

/**
 * Processes events
 * @param[in] events The pointer to VstEvents
 * @retval 0 wants no more
 */
inline VstIntPtr CVstEffect::processEvents(const VstEvents* events)
{
	return dispatcher(effProcessEvents, 0, 0, const_cast<VstEvents*>(events));
}

/**
 * Calls this before a new program is loaded
 * @retval true If succeeded
 * @retval true If failed
 */
inline bool CVstEffect::beginSetProgram()
{
	return (dispatcher(effBeginSetProgram) != 0);
}

/**
 * Calls this after the new program has been loaded
 * @retval true If succeeded
 * @retval true If failed
 */
inline bool CVstEffect::endSetProgram()
{
	return (dispatcher(effEndSetProgram) != 0);
}
