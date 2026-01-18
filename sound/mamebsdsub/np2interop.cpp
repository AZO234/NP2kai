/**
 * @file	np2interop.cpp
 * @brief	Implementation of np2 <-> mame opl 
 */

#ifdef USE_MAME_BSD

#include "compiler.h"
#include "pccore.h"
#include "cpucore.h"
#include "ymfm_opl.h"
#include "np2interop.h"

#include "np2ymfm.h"

 // 旧np21wとのステートセーブ互換を維持する　旧→新のみ互換
#include "np2compatible.h"

void* YMF262Init(int clock, int rate)
{
	opl3bsd* chipbsd = new opl3bsd();
	if (!chipbsd)
		return NULL;

	/* clear */
	ymfm::ymf262& chipcore = chipbsd->m_chip->GetChip();
	chipbsd->m_data.clock = clock;
	chipbsd->m_data.fmrate = chipcore.sample_rate(clock);
	chipbsd->m_data.playrate = rate;

	// reset
	chipcore.reset();

	return chipbsd;
}

void YMF262Shutdown(void* chipptr)
{
	if (!chipptr) return;

	opl3bsd* chipbsd = (opl3bsd*)chipptr;

	delete chipbsd;
}
void YMF262ResetChip(void* chipptr, int samplerate)
{
	if (!chipptr) return;

	opl3bsd* chipbsd = (opl3bsd*)chipptr;
	ymfm::ymf262& chipcore = chipbsd->m_chip->GetChip();

	chipcore.reset();

	chipbsd->m_data.playrate = samplerate;

	chipbsd->m_data.reg_addr = 0;
	chipbsd->m_data.reg_timer1 = 0;
	chipbsd->m_data.reg_timer2 = 0;
	chipbsd->m_data.reg_timerctrl = 0;
	chipbsd->m_data.timer_valid[0] = chipbsd->m_data.timer_valid[1] = false;
	chipbsd->m_data.timer_intr[0] = chipbsd->m_data.timer_intr[1] = false;
	chipbsd->m_data.timer_startclock[0] = chipbsd->m_data.timer_startclock[1] = 0;
}

int YMF262Write(void* chipptr, int a, int v)
{
	if (!chipptr) return 0;

	opl3bsd* chipbsd = (opl3bsd*)chipptr;
	ymfm::ymf262& chipcore = chipbsd->m_chip->GetChip();

	chipcore.write(a, v);

	// 仮実装タイマー
	if (a == 0) 
	{
		chipbsd->m_data.reg_addr = v;
	}
	else if (a == 1) 
	{
		switch (chipbsd->m_data.reg_addr) 
		{
		case 2:
			chipbsd->m_data.reg_timer1 = v;
			break;
		case 3:
			chipbsd->m_data.reg_timer2 = v;
			break;
		case 4:
			if (v & 0x80)
			{
				// Timer Reset
				chipbsd->m_data.timer_valid[0] = false;
				chipbsd->m_data.timer_valid[1] = false;
				chipbsd->m_data.timer_intr[0] = false;
				chipbsd->m_data.timer_intr[1] = false;
			}
			else 
			{
				if (!(chipbsd->m_data.reg_timerctrl & 0x01) && (v & 0x01))
				{
					// Timer1 start
					chipbsd->m_data.timer_startclock[0] = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
					chipbsd->m_data.timer_valid[0] = true;
					chipbsd->m_data.timer_intr[0] = false;
				}
				else if ((chipbsd->m_data.reg_timerctrl & 0x01) && !(v & 0x01))
				{
					// Timer1 stop
					chipbsd->m_data.timer_valid[0] = false;
					chipbsd->m_data.timer_intr[0] = false;
				}
				if (!(chipbsd->m_data.reg_timerctrl & 0x02) && (v & 0x02))
				{
					// Timer2 start
					chipbsd->m_data.timer_startclock[1] = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
					chipbsd->m_data.timer_valid[1] = true;
					chipbsd->m_data.timer_intr[1] = false;
				}
				else if ((chipbsd->m_data.reg_timerctrl & 0x02) && !(v & 0x02))
				{
					// Timer2 stop
					chipbsd->m_data.timer_valid[1] = false;
					chipbsd->m_data.timer_intr[1] = false;
				}
			}
			chipbsd->m_data.reg_timerctrl = v & ~0x80;
			break;
		default:
			break;
		}
	}

	return chipcore.read_status() >> 7;
}

unsigned char YMF262Read(void* chipptr, int a)
{
	UINT8 tmr = 0;

	if (!chipptr) return 0;

	opl3bsd* chipbsd = (opl3bsd*)chipptr;
	ymfm::ymf262& chipcore = chipbsd->m_chip->GetChip();

	// 仮実装タイマー ポートを呼んだときに判定する（割り込み発生などはしない）
	if (chipbsd->m_data.timer_valid[0] && !(chipbsd->m_data.reg_timerctrl & 0x40))
	{
		if (chipbsd->m_data.timer_intr[0])
		{
			// 再判定不要 割り込みも立てておく
			tmr |= 0xc0;
		}
		else if (CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK - chipbsd->m_data.timer_startclock[0] >= pccore.realclock / 1000 * (256 - chipbsd->m_data.reg_timer1) * 808 / 10000)
		{
			// 時間経過した　分解能は 80.8 usec
			chipbsd->m_data.timer_intr[0] = true;
			tmr |= 0xc0;
		}
	}
	if (chipbsd->m_data.timer_valid[1] && !(chipbsd->m_data.reg_timerctrl & 0x20))
	{
		if (chipbsd->m_data.timer_intr[1])
		{
			// 再判定不要 割り込みも立てておく
			tmr |= 0xa0;
		}
		else if (CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK - chipbsd->m_data.timer_startclock[1] >= pccore.realclock / 1000 * (256 - chipbsd->m_data.reg_timer1) * 3231 / 10000)
		{
			// 時間経過した　分解能は 323.1 usec
			chipbsd->m_data.timer_intr[1] = true;
			tmr |= 0xa0;
		}
	}

	return (chipcore.read(a) & 0x1f) | tmr;
}

int YMF262FlagSave(void* chipptr, void* dstbuf)
{
	if (!chipptr) return 0;

	opl3bsd* chipbsd = (opl3bsd*)chipptr;
	ymfm::ymf262& chipcore = chipbsd->m_chip->GetChip();

	// 保存
	std::vector<uint8_t> buffer;
	ymfm::ymfm_saved_state saver(buffer, true);
	chipcore.save_restore(saver);
	buffer.insert(buffer.end(), (uint8_t*)&chipbsd->m_data, (uint8_t*)(&chipbsd->m_data + 1));

	if (dstbuf != NULL) {
		memcpy(dstbuf, &(buffer[0]), buffer.size());
	}

	return buffer.size();
}
int YMF262FlagLoad(void* chipptr, void* srcbuf, int size)
{
	if (!chipptr) return 0;

	opl3bsd* chipbsd = (opl3bsd*)chipptr;
	ymfm::ymf262& chipcore = chipbsd->m_chip->GetChip();

	if (srcbuf == NULL) return 0;

	// バッファサイズを取得
	std::vector<uint8_t> dummybuffer;
	ymfm::ymfm_saved_state saver(dummybuffer, true);
	chipcore.save_restore(saver);
	const int fmbufsize = dummybuffer.size();
	dummybuffer.insert(dummybuffer.end(), (uint8_t*)&chipbsd->m_data, (uint8_t*)(&chipbsd->m_data + 1));
	
	// バッファサイズがあっていなくても通す
	if (size != dummybuffer.size()) {
		// reset
		chipcore.reset();

		// 旧版互換維持用　互換ロード不可なら0を返す
		const int ret = YMF262FlagLoad_NP2REV97(chipbsd, srcbuf, size);
		if (ret) return ret;

		return dummybuffer.size();
	}

	// データ読み取りバッファへ送る
	std::vector<uint8_t> buffer;
	buffer.insert(buffer.end(), (uint8_t*)srcbuf, (uint8_t*)srcbuf + fmbufsize);

	// 復元実行
	ymfm::ymfm_saved_state restorer(buffer, false);
	chipcore.save_restore(restorer);
	memcpy(&chipbsd->m_data, (uint8_t*)srcbuf + fmbufsize, sizeof(chipbsd->m_data));

	return dummybuffer.size();
}

#define OPL3_VOLUME_ADJUST	2
void YMF262UpdateOne(void* chipptr, INT16** buffers, int length)
{
	if (!chipptr) return;

	opl3bsd* chipbsd = (opl3bsd*)chipptr;
	ymfm::ymf262& chipcore = chipbsd->m_chip->GetChip();

	if (buffers == NULL) return;

	const double fmlengthf = chipbsd->m_data.fmcounter_rem + (double)length * chipbsd->m_data.fmrate / chipbsd->m_data.playrate;
	const int fmlength = (int)fmlengthf;
	chipbsd->m_data.fmcounter_rem = fmlengthf - fmlength;

	if (fmlength > 0) {
		if (fmlength > chipbsd->m_outputlen) {
			if (chipbsd->m_output) {
				delete[] (chipbsd->m_output);
			}
			chipbsd->m_outputlen = fmlength;
			chipbsd->m_output = new ymfm::ymfm_output[chipbsd->m_outputlen];
		}
		ymfm::ymfm_output* output = chipbsd->m_output;
		if (output == NULL) return;

		int32_t hasdata = 0;
		chipcore.generate(output, fmlength, &hasdata);

		if (hasdata) {
			for (int i = 0; i < length; i++) {
				int srcIndex = (int)((double)i * chipbsd->m_data.fmrate / chipbsd->m_data.playrate);
				buffers[0][i] = output[srcIndex].data[0] / OPL3_VOLUME_ADJUST;
				buffers[1][i] = output[srcIndex].data[1] / OPL3_VOLUME_ADJUST;
				buffers[2][i] = output[srcIndex].data[2] / OPL3_VOLUME_ADJUST;
				buffers[3][i] = output[srcIndex].data[3] / OPL3_VOLUME_ADJUST;
			}
			chipbsd->m_data.lastsample[0] = output[fmlength - 1].data[0];
			chipbsd->m_data.lastsample[1] = output[fmlength - 1].data[1];
			chipbsd->m_data.lastsample[2] = output[fmlength - 1].data[2];
			chipbsd->m_data.lastsample[3] = output[fmlength - 1].data[3];
		}
		else {
			memset(buffers[0], 0, sizeof(INT16) * length);
			memset(buffers[1], 0, sizeof(INT16) * length);
			memset(buffers[2], 0, sizeof(INT16) * length);
			memset(buffers[3], 0, sizeof(INT16) * length);
		}
	}
	else {
		for (int i = 0; i < length; i++) {
			buffers[0][i] = chipbsd->m_data.lastsample[0] / OPL3_VOLUME_ADJUST;
			buffers[1][i] = chipbsd->m_data.lastsample[1] / OPL3_VOLUME_ADJUST;
			buffers[2][i] = chipbsd->m_data.lastsample[2] / OPL3_VOLUME_ADJUST;
			buffers[3][i] = chipbsd->m_data.lastsample[3] / OPL3_VOLUME_ADJUST;
		}
	}
}

#endif 
