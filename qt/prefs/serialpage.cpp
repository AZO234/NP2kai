/* === preferences: Serial page ===
 * COM1 and the two channels of a PC-9861K board, plus the board itself, as
 * the Windows port's serial options (windows/dialog/d_serial.cpp).
 */

#include <compiler.h>
#include "serialpage.h"
#include "ui_serialpage.h"
#include "np2.h"

#include <pccore.h>
#include <cbus/pc9861k.h>
#include <generic/dipswbmp.h>

#include <QSignalBlocker>

#include <cstring>

/* Interrupt of each channel: bits 0-1 (CH.1) and 2-3 (CH.2) of switch 2 */
static const UINT8 kInt1[] = { 0x00, 0x02, 0x01, 0x03 };
static const UINT8 kInt2[] = { 0x00, 0x08, 0x04, 0x0c };
/* Bits 0-1 of switches 1 and 3 */
static const UINT8 kSync[] = { 0x03, 0x00, 0x01, 0x02 };

SerialPage::SerialPage(QWidget *parent)
	: PrefPage(parent)
	, m_ui(new Ui::SerialPage)
{
	std::memset(m_sw, 0, sizeof(m_sw));
	std::memset(m_jmp, 0, sizeof(m_jmp));
	m_ui->setupUi(this);

#if defined(SUPPORT_PC9861K)
	for (QComboBox *combo : { m_ui->ch1Speed, m_ui->ch2Speed }) {
		for (UINT32 speed : pc9861k_speed) {
			combo->addItem(tr("%1 bps").arg(speed));
		}
	}
	for (int i : { 0, 1, 2, 3 }) {
		m_ui->ch1Int->addItem(tr("INT%1").arg(i));
	}
	for (int i : { 0, 4, 5, 6 }) {
		m_ui->ch2Int->addItem(tr("INT%1").arg(i));
	}
	for (QComboBox *combo : { m_ui->ch1Mode, m_ui->ch2Mode }) {
		combo->addItems({ tr("Synchronous"), tr("Asynchronous"), tr("Asynchronous x16"),
		                  tr("Asynchronous x64") });
	}

	auto modeChanged = [this](int channel) {
		UINT8 sw[3];
		std::memcpy(sw, m_sw, sizeof(sw));
		sw[channel * 2] = mode(channel);
		set9861(sw, m_jmp);
	};
	for (QComboBox *combo : { m_ui->ch1Speed, m_ui->ch1Mode }) {
		connect(combo, &QComboBox::currentIndexChanged, this, [modeChanged]() { modeChanged(0); });
	}
	for (QComboBox *combo : { m_ui->ch2Speed, m_ui->ch2Mode }) {
		connect(combo, &QComboBox::currentIndexChanged, this, [modeChanged]() { modeChanged(1); });
	}
	for (QComboBox *combo : { m_ui->ch1Int, m_ui->ch2Int }) {
		connect(combo, &QComboBox::currentIndexChanged, this, [this]() {
			UINT8 sw[3];
			std::memcpy(sw, m_sw, sizeof(sw));
			sw[1] = static_cast<UINT8>((m_sw[1] & 0xf0) | comboValue(m_ui->ch1Int, kInt1)
			                           | comboValue(m_ui->ch2Int, kInt2));
			set9861(sw, m_jmp);
		});
	}
	connect(m_ui->pc9861Dipsw, &DipSwView::clicked, this, &SerialPage::click9861);
	connect(m_ui->pc9861_e, &QCheckBox::toggled, this, &SerialPage::updateEnabled);
#else
	for (QWidget *tab : { m_ui->pc9861Tab, static_cast<QWidget *>(m_ui->ch1Box),
	                      static_cast<QWidget *>(m_ui->ch2Box) }) {
		m_ui->serialTabs->removeTab(m_ui->serialTabs->indexOf(tab));
	}
#endif
}

SerialPage::~SerialPage() = default;

/* Without the board its two channels do not exist */
void SerialPage::updateEnabled()
{
	const bool board = m_ui->pc9861_e->isChecked();
	for (QWidget *w : m_ui->pc9861Tab->findChildren<QWidget *>()) {
		if (w != m_ui->pc9861_e) {
			w->setEnabled(board);
		}
	}
	m_ui->serialTabs->setTabEnabled(m_ui->serialTabs->indexOf(m_ui->ch1Box), board);
	m_ui->serialTabs->setTabEnabled(m_ui->serialTabs->indexOf(m_ui->ch2Box), board);
}

/* The mode byte holds the speed inverted in bits 2-5, counted differently in
 * synchronous mode (from 75 bps up) and asynchronous mode. */
UINT8 SerialPage::mode(int channel) const
{
	QComboBox *const speedCombo = channel ? m_ui->ch2Speed : m_ui->ch1Speed;
	QComboBox *const syncCombo = channel ? m_ui->ch2Mode : m_ui->ch1Mode;
	const UINT8 sync = comboValue(syncCombo, kSync);
	UINT speed = static_cast<UINT>(qMax(speedCombo->currentIndex(), 0));
	if (sync & 2) {
		speed += 3;
	} else if (speed) {
		speed--;
	}
	return static_cast<UINT8>((((~speed) & 0x0f) << 2) | sync);
}

void SerialPage::setMode(int channel, UINT8 value)
{
	QComboBox *const speedCombo = channel ? m_ui->ch2Speed : m_ui->ch1Speed;
	QComboBox *const syncCombo = channel ? m_ui->ch2Mode : m_ui->ch1Mode;
	UINT speed = (((~value) >> 2) & 0x0f) + 1;
	if (value) {
		speed = (speed > 4) ? speed - 4 : 0;
	}
	const UINT last = static_cast<UINT>(speedCombo->count() - 1);
	speedCombo->setCurrentIndex(static_cast<int>(qMin(speed, last)));
	setComboValue(syncCombo, kSync, value & 0x03);
}

void SerialPage::set9861(const UINT8 *sw, const UINT8 *jmp)
{
	std::memcpy(m_sw, sw, sizeof(m_sw));
	std::memcpy(m_jmp, jmp, sizeof(m_jmp));
#if defined(SUPPORT_PC9861K)
	{
		const QSignalBlocker b1(m_ui->ch1Speed), b2(m_ui->ch1Mode), b3(m_ui->ch1Int);
		const QSignalBlocker b4(m_ui->ch2Speed), b5(m_ui->ch2Mode), b6(m_ui->ch2Int);
		setMode(0, m_sw[0]);
		setMode(1, m_sw[2]);
		setComboValue(m_ui->ch1Int, kInt1, m_sw[1] & 0x03);
		setComboValue(m_ui->ch2Int, kInt2, m_sw[1] & 0x0c);
	}
	m_ui->pc9861Dipsw->setBitmap(dipswbmp_get9861(m_sw, m_jmp));
#endif
}

/* Three rows of 9-pixel cells: switches 1-3, jumpers 1-3, jumpers 4-6 */
void SerialPage::click9861(const QPoint &pos)
{
	const int x = pos.x() / 9;
	const int y = pos.y() / 9;
	UINT8 sw[3];
	UINT8 jmp[6];
	std::memcpy(sw, m_sw, sizeof(sw));
	std::memcpy(jmp, m_jmp, sizeof(jmp));

	if (y >= 1 && y < 3) {
		if (x >= 1 && x < 7) {
			sw[0] ^= static_cast<UINT8>(1 << (x - 1));
		} else if (x >= 10 && x < 14) {
			sw[1] ^= static_cast<UINT8>(1 << (x - 10));
		} else if (x >= 17 && x < 23) {
			sw[2] ^= static_cast<UINT8>(1 << (x - 17));
		}
	} else if (y >= 4 && y < 6) {
		if (x >= 1 && x < 7) {
			jmp[0] ^= static_cast<UINT8>(1 << (x - 1));
		} else if (x >= 9 && x < 15) {
			jmp[1] ^= static_cast<UINT8>(1 << (x - 9));
		} else if (x >= 17 && x < 19) {
			jmp[2] = static_cast<UINT8>(1 << (x - 17));
		}
	} else if (y >= 7 && y < 9) {
		if (x >= 1 && x < 9) {
			const UINT8 bit = static_cast<UINT8>(1 << (x - 1));
			jmp[3] = (jmp[3] != bit) ? bit : 0;
		} else if (x >= 11 && x < 17) {
			jmp[4] ^= static_cast<UINT8>(1 << (x - 11));
		} else if (x >= 19 && x < 25) {
			jmp[5] ^= static_cast<UINT8>(1 << (x - 19));
		}
	}
	set9861(sw, jmp);
}

void SerialPage::load()
{
	m_ui->com1Box->load(np2oscfg.com[0], 0);
#if defined(SUPPORT_PC9861K)
	m_ui->pc9861_e->setChecked(np2cfg.pc9861enable != 0);
	set9861(np2cfg.pc9861sw, np2cfg.pc9861jmp);
	updateEnabled();
	m_ui->ch1Box->load(np2oscfg.com[1], 1);
	m_ui->ch2Box->load(np2oscfg.com[2], 2);
#endif
}

void SerialPage::save()
{
	m_ui->com1Box->save(np2oscfg.com[0], 0);
#if defined(SUPPORT_PC9861K)
	np2cfg.pc9861enable = m_ui->pc9861_e->isChecked() ? 1 : 0;
	std::memcpy(np2cfg.pc9861sw, m_sw, sizeof(m_sw));
	std::memcpy(np2cfg.pc9861jmp, m_jmp, sizeof(m_jmp));
	m_ui->ch1Box->save(np2oscfg.com[1], 1);
	m_ui->ch2Box->save(np2oscfg.com[2], 2);
#endif
}
