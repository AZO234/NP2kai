/**
 * @file	np2ymfm.h
 * @brief	Interface of np2 ymfm classes
 */

#pragma once

#ifdef USE_MAME_BSD

#pragma pack(push, 1)

class chip_wrapper : public ymfm::ymfm_interface
{
public:
	chip_wrapper() :
		m_chip(*this)
	{
		m_chip.reset();
	}

	ymfm::ymf262& GetChip() {
		return m_chip;
	}

private:
	ymfm::ymf262 m_chip;
};

typedef struct {
	int clock;
	int fmrate;
	int playrate;
	double fmcounter_rem;
	INT16 lastsample[4];

	// 仮実装タイマー　正式にはNEVENTを使うべき？
	UINT8 reg_addr;
	UINT8 reg_timer1;
	UINT8 reg_timer2;
	UINT8 reg_timerctrl;
	bool timer_valid[2];
	bool timer_intr[2];
	UINT32 timer_startclock[2];

	UINT8 reserved[128];
} OPL3BSD_EX;

class opl3bsd {
public:
	opl3bsd() :
		m_chip(new chip_wrapper()),
		m_data(),
		m_output(nullptr),
		m_outputlen(0)
	{
		m_outputlen = 4096;
		m_output = new ymfm::ymfm_output[m_outputlen];
	}
	~opl3bsd()
	{
		if (m_output) {
			delete[] m_output;
		}
	}

	std::unique_ptr<chip_wrapper> m_chip;
	ymfm::ymfm_output* m_output;
	int m_outputlen;
	OPL3BSD_EX m_data;
};

#pragma pack(pop)

#endif