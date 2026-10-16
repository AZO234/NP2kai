/* === preferences: Sound page ===
 * The Sound tab of wx/dialog/prefframe.cpp. The board pages follow the Windows
 * port's sound options (windows/dialog/d_sound.cpp): the same value lists, the
 * jumper pictures that flip a switch when clicked, the board defaults.
 */

#include <compiler.h>
#include "soundpage.h"
#include "ui_soundpage.h"
#include "np2.h"
#include "joymng.h"

#include <pccore.h>
#include <generic/dipswbmp.h>

#include <QSignalBlocker>
#include <QTabBar>

namespace {

struct Board {
	UINT8 id;
	const char *name;
};

/* The order of the Windows port's Device > Sound menu */
const Board kBoards[] = {
	{ SOUNDID_NONE,                QT_TRANSLATE_NOOP("SoundPage", "None") },
	{ SOUNDID_PC_9801_14,          QT_TRANSLATE_NOOP("SoundPage", "PC-9801-14") },
	{ SOUNDID_PC_9801_26K,         QT_TRANSLATE_NOOP("SoundPage", "PC-9801-26K") },
	{ SOUNDID_PC_9801_86,          QT_TRANSLATE_NOOP("SoundPage", "PC-9801-86") },
	{ SOUNDID_PC_9801_86_26K,      QT_TRANSLATE_NOOP("SoundPage", "PC-9801-26K + 86") },
	{ SOUNDID_PC_9801_86_ADPCM,    QT_TRANSLATE_NOOP("SoundPage", "PC-9801-86 + Chibi-oto") },
	{ SOUNDID_PC_9801_118,         QT_TRANSLATE_NOOP("SoundPage", "PC-9801-118") },
	{ SOUNDID_PC_9801_86_WSS,      QT_TRANSLATE_NOOP("SoundPage", "PC-9801-86 + Mate-X PCM (B460)") },
	{ SOUNDID_PC_9801_86_118,      QT_TRANSLATE_NOOP("SoundPage", "PC-9801-86 + PC-9801-118 (B460)") },
	{ SOUNDID_MATE_X_PCM,          QT_TRANSLATE_NOOP("SoundPage", "Mate-X PCM") },
	{ SOUNDID_SPEAKBOARD,          QT_TRANSLATE_NOOP("SoundPage", "Speak Board") },
	{ SOUNDID_86_SPEAKBOARD,       QT_TRANSLATE_NOOP("SoundPage", "PC-9801-86 + Speak Board") },
	{ SOUNDID_SPARKBOARD,          QT_TRANSLATE_NOOP("SoundPage", "Spark Board") },
#if defined(SUPPORT_SOUND_SB16)
	{ SOUNDID_SB16,                QT_TRANSLATE_NOOP("SoundPage", "Sound Blaster 16") },
	{ SOUNDID_PC_9801_86_SB16,     QT_TRANSLATE_NOOP("SoundPage", "PC-9801-86 + Sound Blaster 16") },
	{ SOUNDID_WSS_SB16,            QT_TRANSLATE_NOOP("SoundPage", "Mate-X PCM (B460) + Sound Blaster 16") },
	{ SOUNDID_PC_9801_86_WSS_SB16, QT_TRANSLATE_NOOP("SoundPage", "PC-9801-86 + Mate-X PCM (B460) + Sound Blaster 16") },
	{ SOUNDID_PC_9801_118_SB16,    QT_TRANSLATE_NOOP("SoundPage", "PC-9801-118 + Sound Blaster 16") },
	{ SOUNDID_PC_9801_86_118_SB16, QT_TRANSLATE_NOOP("SoundPage", "PC-9801-86 + PC-9801-118 (B460) + Sound Blaster 16") },
#endif
#if defined(SUPPORT_PX)
	{ SOUNDID_PX1,                 QT_TRANSLATE_NOOP("SoundPage", "PX1") },
	{ SOUNDID_PX2,                 QT_TRANSLATE_NOOP("SoundPage", "PX2") },
#endif
	{ SOUNDID_SOUNDORCHESTRA,      QT_TRANSLATE_NOOP("SoundPage", "Sound Orchestra") },
	{ SOUNDID_SOUNDORCHESTRAV,     QT_TRANSLATE_NOOP("SoundPage", "Sound Orchestra-V") },
	{ SOUNDID_MULTIMEDIAORCHESTRA, QT_TRANSLATE_NOOP("SoundPage", "Multimedia Orchestra") },
	{ SOUNDID_AMD98,               QT_TRANSLATE_NOOP("SoundPage", "AMD-98") },
	{ SOUNDID_WAVESTAR,            QT_TRANSLATE_NOOP("SoundPage", "Wave Star") },
};

const UINT32 kSampleRates[] = { 11025, 22050, 44100, 48000, 88200, 96000 };

/* PC-9801-26K and Speak Board, bits of snd26opt / spbopt */
const UINT8 kIo26[] = { 0x00, 0x10 };
const UINT8 kInt26[] = { 0x00, 0x80, 0xc0, 0x40 };
const UINT8 kRom26[] = { 0x00, 0x01, 0x02, 0x03, 0x04 };

/* PC-9801-86, bits of snd86opt */
const UINT8 kIo86[] = { 0x01, 0x00 };
const UINT8 kInt86[] = { 0x00, 0x04, 0x0c, 0x08 };
const UINT8 kId86[] = { 0xe0, 0xc0, 0xa0, 0x80, 0x60, 0x40, 0x20, 0x00 };

/* PC-9801-118 and Mate-X PCM */
const UINT16 kIo118[] = { 0x0088, 0x0188, 0x0288, 0x0388 };
const UINT8 kId118[] = { 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80 };
const UINT8 kDma118[] = { 0, 1, 3 };
const UINT8 kIrq118F[] = { 3, 10, 12, 13 };
const UINT8 kIrq118P[] = { 3, 5, 10, 12 };
const UINT8 kIrq118M[] = { 0xff, 10 };

/* Wave Star */
const UINT16 kIoWaveStar[] = { 0x0188, 0x0288 };
const UINT8 kDmaWaveStar[] = { 0, 3 };
const UINT8 kIrqWaveStar[] = { 12, 13 };

#if defined(SUPPORT_SOUND_SB16)
const UINT8 kIoSb16[] = { 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde };
const UINT8 kDmaSb16[] = { 0, 3 };
const UINT8 kIrqSb16[] = { 3, 5, 10, 12 };
#endif

/* Settings a board comes with, as the Windows port's Defaults buttons */
const UINT8 kDefault26 = 0xd1;
const UINT8 kDefault86 = 0x7f;
const UINT8 kDefaultSpb = 0xd1;

} // namespace

SoundPage::SoundPage(QWidget *parent)
	: PrefPage(parent)
	, m_ui(new Ui::SoundPage)
	, m_snd26(0)
	, m_snd86(0)
	, m_spb(0)
	, m_spbvr(0)
{
	m_ui->setupUi(this);

	for (const Board &b : kBoards) {
		m_ui->SNDboard->addItem(tr(b.name), b.id);
	}
	connect(m_ui->SNDboard, &QComboBox::currentIndexChanged, this, [this]() {
		showBoardTab(m_ui->SNDboard->currentData().toInt());
	});

	/* Mixer */
	const std::pair<QSlider *, QLabel *> sliders[] = {
		{ m_ui->vol_master, m_ui->vol_masterValue }, { m_ui->vol_fm, m_ui->vol_fmValue },
		{ m_ui->vol_ssg, m_ui->vol_ssgValue },       { m_ui->vol_adpcm, m_ui->vol_adpcmValue },
		{ m_ui->vol_pcm, m_ui->vol_pcmValue },       { m_ui->vol_rhythm, m_ui->vol_rhythmValue },
		{ m_ui->davolume, m_ui->davolumeValue },     { m_ui->vol_midi, m_ui->vol_midiValue },
		{ m_ui->MOTORVOL, m_ui->MOTORVOLValue },
		{ m_ui->vol14_0, m_ui->vol14_0Value },       { m_ui->vol14_1, m_ui->vol14_1Value },
		{ m_ui->vol14_2, m_ui->vol14_2Value },       { m_ui->vol14_3, m_ui->vol14_3Value },
		{ m_ui->vol14_4, m_ui->vol14_4Value },       { m_ui->vol14_5, m_ui->vol14_5Value },
		{ m_ui->SPBVRLEVEL, m_ui->SPBVRLEVELValue },
	};
	for (const auto &s : sliders) {
		showSliderValue(s.first, s.second);
	}
	m_ui->MOTORVOL->setToolTip(tr("Floppy disk seek and display relay sounds"));

	/* PC-9801-26K */
	auto jumper26 = [this](QComboBox *combo, auto &values, UINT8 mask) {
		connect(combo, &QComboBox::currentIndexChanged, this, [this, combo, &values, mask]() {
			set26(static_cast<UINT8>((m_snd26 & ~mask) | comboValue(combo, values)));
		});
	};
	jumper26(m_ui->SND26IO, kIo26, 0x10);
	jumper26(m_ui->SND26INT, kInt26, 0xc0);
	jumper26(m_ui->SND26ROM, kRom26, 0x07);
	connect(m_ui->snd26Jumper, &DipSwView::clicked, this, &SoundPage::click26);
	connect(m_ui->snd26TabDefaults, &QPushButton::clicked, this, [this]() { set26(kDefault26); });

	/* PC-9801-86 */
	auto dip86 = [this](QComboBox *combo, auto &values, UINT8 mask) {
		connect(combo, &QComboBox::currentIndexChanged, this, [this, combo, &values, mask]() {
			set86(static_cast<UINT8>((m_snd86 & ~mask) | comboValue(combo, values)));
		});
	};
	dip86(m_ui->SND86IO, kIo86, 0x01);
	dip86(m_ui->SND86INTA, kInt86, 0x0c);
	dip86(m_ui->SND86ID, kId86, 0xe0);
	connect(m_ui->SND86INT, &QCheckBox::toggled, this, [this](bool on) {
		set86(static_cast<UINT8>((m_snd86 & ~0x10) | (on ? 0x10 : 0)));
	});
	connect(m_ui->SND86ROM, &QCheckBox::toggled, this, [this](bool on) {
		set86(static_cast<UINT8>((m_snd86 & ~0x02) | (on ? 0x02 : 0)));
	});
	connect(m_ui->snd86Dipsw, &DipSwView::clicked, this, &SoundPage::click86);
	connect(m_ui->snd86TabDefaults, &QPushButton::clicked, this, [this]() { set86(kDefault86); });

	/* PC-9801-118: the picture is drawn from the combo boxes */
	for (QComboBox *combo : { m_ui->SND118IO, m_ui->SND118DMA, m_ui->SND118INTF,
	                          m_ui->SND118INTP, m_ui->SND118INTM }) {
		connect(combo, &QComboBox::currentIndexChanged, this, &SoundPage::update118Jumper);
	}
	connect(m_ui->SND118ROM, &QCheckBox::toggled, this, &SoundPage::update118Jumper);
	connect(m_ui->snd118TabDefaults, &QPushButton::clicked, this, [this]() {
		set118(0x0188, 0x80, 3, 12, 12, 0xff, 0);
	});

	connect(m_ui->sndWssTabDefaults, &QPushButton::clicked, this, [this]() { setWss(0x70, 1, 3); });
	connect(m_ui->sndWaveStarTabDefaults, &QPushButton::clicked, this, [this]() {
		setWaveStar(0x0188, 3, 12, 1, 1);
	});
#if defined(SUPPORT_SOUND_SB16)
	connect(m_ui->sndSb16TabDefaults, &QPushButton::clicked, this, [this]() { setSb16(0xd2, 3, 5); });
#else
	m_ui->soundTabs->removeTab(m_ui->soundTabs->indexOf(m_ui->sndSb16Tab));
#endif

	/* Speak Board */
	auto jumperSpb = [this](QComboBox *combo, auto &values, UINT8 mask) {
		connect(combo, &QComboBox::currentIndexChanged, this, [this, combo, &values, mask]() {
			setSpb(static_cast<UINT8>((m_spb & ~mask) | comboValue(combo, values)), m_spbvr);
		});
	};
	jumperSpb(m_ui->SPBIO, kIo26, 0x10);
	jumperSpb(m_ui->SPBINT, kInt26, 0xc0);
	jumperSpb(m_ui->SPBROM, kRom26, 0x07);
	for (QCheckBox *vr : { m_ui->SPBVRL, m_ui->SPBVRR }) {
		connect(vr, &QCheckBox::toggled, this, [this]() {
			setSpb(m_spb, static_cast<UINT8>((m_ui->SPBVRL->isChecked() ? 1 : 0)
			                                 | (m_ui->SPBVRR->isChecked() ? 2 : 0)));
		});
	}
	connect(m_ui->sndSpbJumper, &DipSwView::clicked, this, &SoundPage::clickSpb);
	connect(m_ui->sndSpbTabDefaults, &QPushButton::clicked, this, [this]() {
		setSpb(kDefaultSpb, 0);
		m_ui->SPBVRLEVEL->setValue(24);
		m_ui->SPBREVERSE->setChecked(false);
	});

	/* Joypad */
#if defined(SUPPORT_GAMEPORT)
	connect(m_ui->PAD1_GAMEPORT, &QCheckBox::toggled, m_ui->PAD1_ANALOG, &QWidget::setEnabled);
#else
	m_ui->PAD1_GAMEPORT->hide();
	m_ui->PAD1_ANALOG->hide();
#endif
#if !defined(SUPPORT_FMGEN)
	m_ui->soundTabs->removeTab(m_ui->soundTabs->indexOf(m_ui->sndFmgenTab));
#endif

	/* Wide enough for every board where there is room; scroll buttons on a
	 * narrow screen */
	m_ui->soundTabs->tabBar()->setExpanding(false);
}

SoundPage::~SoundPage() = default;

QSize SoundPage::sizeHint() const
{
	const int tabs = m_ui->soundTabs->tabBar()->sizeHint().width();
	const QMargins m = layout()->contentsMargins();
	return PrefPage::sizeHint().expandedTo(QSize(tabs + m.left() + m.right() + 8, 0));
}

/* ---- jumper boards ---- */

void SoundPage::set26(UINT8 value)
{
	m_snd26 = value;
	{
		const QSignalBlocker b1(m_ui->SND26IO), b2(m_ui->SND26INT), b3(m_ui->SND26ROM);
		setComboValue(m_ui->SND26IO, kIo26, value & 0x10);
		setComboValue(m_ui->SND26INT, kInt26, value & 0xc0);
		/* Every setting with bit 2 leaves the ROM out */
		setComboValue(m_ui->SND26ROM, kRom26, (value & 0x04) ? 0x04 : (value & 0x07));
	}
	m_ui->snd26Jumper->setBitmap(dipswbmp_getsnd26(m_snd26));
}

void SoundPage::set86(UINT8 value)
{
	m_snd86 = value;
	{
		const QSignalBlocker b1(m_ui->SND86IO), b2(m_ui->SND86INTA), b3(m_ui->SND86ID);
		const QSignalBlocker b4(m_ui->SND86INT), b5(m_ui->SND86ROM);
		setComboValue(m_ui->SND86IO, kIo86, value & 0x01);
		setComboValue(m_ui->SND86INTA, kInt86, value & 0x0c);
		setComboValue(m_ui->SND86ID, kId86, value & 0xe0);
		m_ui->SND86INT->setChecked(value & 0x10);
		m_ui->SND86ROM->setChecked(value & 0x02);
	}
	m_ui->SND86INTA->setEnabled(value & 0x10);
	m_ui->snd86Dipsw->setBitmap(dipswbmp_getsnd86(m_snd86));
}

void SoundPage::setSpb(UINT8 value, UINT8 vr)
{
	m_spb = value;
	m_spbvr = vr;
	{
		const QSignalBlocker b1(m_ui->SPBIO), b2(m_ui->SPBINT), b3(m_ui->SPBROM);
		const QSignalBlocker b4(m_ui->SPBVRL), b5(m_ui->SPBVRR);
		setComboValue(m_ui->SPBIO, kIo26, value & 0x10);
		setComboValue(m_ui->SPBINT, kInt26, value & 0xc0);
		setComboValue(m_ui->SPBROM, kRom26, (value & 0x04) ? 0x04 : (value & 0x07));
		m_ui->SPBVRL->setChecked(vr & 1);
		m_ui->SPBVRR->setChecked(vr & 2);
	}
	m_ui->sndSpbJumper->setBitmap(dipswbmp_getsndspb(m_spb, m_spbvr));
}

void SoundPage::set118(UINT16 io, UINT8 id, UINT8 dma, UINT8 irqf, UINT8 irqp, UINT8 irqm, UINT8 rom)
{
	{
		const QSignalBlocker b1(m_ui->SND118IO), b2(m_ui->SND118DMA), b3(m_ui->SND118INTF);
		const QSignalBlocker b4(m_ui->SND118INTP), b5(m_ui->SND118INTM), b6(m_ui->SND118ROM);
		setComboValue(m_ui->SND118IO, kIo118, io, 1);
		setComboValue(m_ui->SND118ID, kId118, id, 8);
		setComboValue(m_ui->SND118DMA, kDma118, dma, 2);
		setComboValue(m_ui->SND118INTF, kIrq118F, irqf, 2);
		setComboValue(m_ui->SND118INTP, kIrq118P, irqp, 3);
		setComboValue(m_ui->SND118INTM, kIrq118M, irqm);
		m_ui->SND118ROM->setChecked(rom != 0);
	}
	update118Jumper();
}

/* Only the Windows port has this picture among its resources */
void SoundPage::update118Jumper()
{
	UINT8 *bmp = dipswbmp_getsnd118(
	    comboValue(m_ui->SND118IO, kIo118), comboValue(m_ui->SND118DMA, kDma118),
	    comboValue(m_ui->SND118INTF, kIrq118F), comboValue(m_ui->SND118INTP, kIrq118P),
	    comboValue(m_ui->SND118INTM, kIrq118M), m_ui->SND118ROM->isChecked() ? 1 : 0);
	m_ui->snd118JumperLabel->setVisible(bmp != nullptr);
	m_ui->snd118Jumper->setBitmap(bmp);
}

void SoundPage::setWss(UINT8 id, UINT8 dma, UINT8 irq)
{
	setComboValue(m_ui->SNDWSSID, kId118, id, 7);
	setComboValue(m_ui->SNDWSSDMA, kDma118, dma, 1);
	setComboValue(m_ui->SNDWSSINT, kIrq118P, irq);
}

void SoundPage::setWaveStar(UINT16 io, UINT8 dma, UINT8 irq, UINT8 pnp, UINT8 pcm)
{
	setComboValue(m_ui->SNDWSIO, kIoWaveStar, io);
	setComboValue(m_ui->SNDWSDMA, kDmaWaveStar, dma, 1);
	setComboValue(m_ui->SNDWSINT, kIrqWaveStar, irq);
	m_ui->SNDWSPNP->setChecked(pnp != 0);
	m_ui->SNDWSPCM->setChecked(pcm != 0);
}

void SoundPage::setSb16(UINT8 io, UINT8 dma, UINT8 irq)
{
#if defined(SUPPORT_SOUND_SB16)
	setComboValue(m_ui->SNDSB16IO, kIoSb16, io);
	setComboValue(m_ui->SNDSB16DMA, kDmaSb16, dma, 1);
	setComboValue(m_ui->SNDSB16INT, kIrqSb16, irq, 1);
#else
	(void)io;
	(void)dma;
	(void)irq;
#endif
}

/* The click handlers map the pictures of generic/dipswbmp.c, as the Windows
 * port does: 9-pixel cells for the 26K and the Speak Board, 8 for the 86. */
void SoundPage::click26(const QPoint &pos)
{
	const int x = pos.x() / 9;
	const int y = pos.y() / 9;
	if (y < 1 || y >= 3) {
		return;
	}
	UINT value = m_snd26;
	if (x >= 2 && x < 7) {
		value = (value & ~7u) | static_cast<UINT>(x - 2);
	} else if (x >= 9 && x < 12) {
		const UINT bit = 0x40u << (2 - y);
		switch (x) {
		case 9:  value |= bit;  break;
		case 10: value ^= bit;  break;
		case 11: value &= ~bit; break;
		}
	} else if (x >= 15 && x < 17) {
		value = (value & ~0x10u) | static_cast<UINT>((x - 15) << 4);
	}
	set26(static_cast<UINT8>(value));
}

void SoundPage::click86(const QPoint &pos)
{
	const int x = pos.x() / 8 - 2;
	const int y = pos.y() / 8;
	if (x < 0 || x >= 8 || y < 1 || y >= 3) {
		return;
	}
	set86(static_cast<UINT8>(m_snd86 ^ (1 << x)));
}

void SoundPage::clickSpb(const QPoint &pos)
{
	const int x = pos.x() / 9;
	const int y = pos.y() / 9;
	if (y < 1 || y >= 3) {
		return;
	}
	UINT value = m_spb;
	UINT vr = m_spbvr;
	if (x >= 2 && x < 5) {
		const UINT bit = 0x40u << (2 - y);
		switch (x) {
		case 2: value |= bit;  break;
		case 3: value ^= bit;  break;
		case 4: value &= ~bit; break;
		}
	} else if (x == 7) {
		value ^= 0x20;
	} else if (x >= 10 && x < 12) {
		value = (value & ~0x10u) | static_cast<UINT>((x - 10) << 4);
	} else if (x >= 14 && x < 19) {
		value = (value & ~7u) | static_cast<UINT>(x - 14);
	} else if (x >= 21 && x < 24) {
		vr ^= static_cast<UINT>(3 - y);
	}
	setSpb(static_cast<UINT8>(value), static_cast<UINT8>(vr));
}

/* Choosing a board brings up the page with its settings */
void SoundPage::showBoardTab(int board)
{
	QWidget *tab = nullptr;
	switch (board) {
	case SOUNDID_PC_9801_14:
		tab = m_ui->snd14Tab;
		break;
	case SOUNDID_PC_9801_26K:
		tab = m_ui->snd26Tab;
		break;
	case SOUNDID_PC_9801_86:
	case SOUNDID_PC_9801_86_26K:
	case SOUNDID_PC_9801_86_ADPCM:
	case SOUNDID_PC_9801_86_WSS:
	case SOUNDID_PC_9801_86_118:
		tab = m_ui->snd86Tab;
		break;
	case SOUNDID_PC_9801_118:
		tab = m_ui->snd118Tab;
		break;
	case SOUNDID_MATE_X_PCM:
		tab = m_ui->sndWssTab;
		break;
	case SOUNDID_SPEAKBOARD:
	case SOUNDID_86_SPEAKBOARD:
		tab = m_ui->sndSpbTab;
		break;
	case SOUNDID_WAVESTAR:
		tab = m_ui->sndWaveStarTab;
		break;
#if defined(SUPPORT_SOUND_SB16)
	case SOUNDID_SB16:
	case SOUNDID_PC_9801_86_SB16:
	case SOUNDID_WSS_SB16:
	case SOUNDID_PC_9801_86_WSS_SB16:
	case SOUNDID_PC_9801_118_SB16:
	case SOUNDID_PC_9801_86_118_SB16:
		tab = m_ui->sndSb16Tab;
		break;
#endif
	}
	if (tab) {
		m_ui->soundTabs->setCurrentWidget(tab);
	}
}

/* The first joypad found, or one of those SDL sees by name; a configured device that is not plugged in
 * stays on the list, so saving does not forget it. */
void SoundPage::fillJoypads()
{
	m_ui->joypad->clear();
	m_ui->joypad->addItem(tr("First one found"), QString());
	joymng_devinfo_t **list = joymng_get_devinfo_list();
	for (int i = 0; list && list[i]; i++) {
		const QString name = QString::fromUtf8(list[i]->devname);
		m_ui->joypad->addItem(name, name);
	}
	const QString current = QString::fromUtf8(np2oscfg.JOYDEV[0]);
	int index = m_ui->joypad->findData(current);
	if (index < 0) {
		m_ui->joypad->addItem(tr("%1 (not connected)").arg(current), current);
		index = m_ui->joypad->count() - 1;
	}
	m_ui->joypad->setCurrentIndex(index);
}

/* ---- load / save ---- */

void SoundPage::load()
{
	int board = m_ui->SNDboard->findData(np2cfg.SOUND_SW);
	if (board < 0) {
		m_ui->SNDboard->addItem(tr("Other (%1h)").arg(np2cfg.SOUND_SW, 2, 16, QLatin1Char('0')), np2cfg.SOUND_SW);
		board = m_ui->SNDboard->count() - 1;
	}
	m_ui->SNDboard->setCurrentIndex(board);
	setComboValue(m_ui->SampleHz, kSampleRates, np2cfg.samplingrate, 2);
	m_ui->Latencys->setValue(np2cfg.delayms);

	m_ui->vol_master->setValue(np2cfg.vol_master);
	m_ui->vol_fm->setValue(np2cfg.vol_fm);
	m_ui->vol_ssg->setValue(np2cfg.vol_ssg);
	m_ui->vol_adpcm->setValue(np2cfg.vol_adpcm);
	m_ui->vol_pcm->setValue(np2cfg.vol_pcm);
	m_ui->vol_rhythm->setValue(np2cfg.vol_rhythm);
	m_ui->davolume->setValue(np2cfg.davolume);
	m_ui->vol_midi->setValue(np2cfg.vol_midi);
	m_ui->MOTORVOL->setValue(np2cfg.MOTORVOL);
	QRadioButton *const beep[4] = { m_ui->beep_off, m_ui->beep_low, m_ui->beep_mid, m_ui->beep_high };
	beep[np2cfg.BEEP_VOL & 3]->setChecked(true);

	QSlider *const vol14[6] = { m_ui->vol14_0, m_ui->vol14_1, m_ui->vol14_2,
	                            m_ui->vol14_3, m_ui->vol14_4, m_ui->vol14_5 };
	for (int i = 0; i < 6; i++) {
		vol14[i]->setValue(np2cfg.vol14[i]);
	}

	set26(np2cfg.snd26opt);
	set86(np2cfg.snd86opt);
	set118(np2cfg.snd118io, np2cfg.snd118id, np2cfg.snd118dma, np2cfg.snd118irqf,
	       np2cfg.snd118irqp, np2cfg.snd118irqm, np2cfg.snd118rom);
	setWss(np2cfg.sndwssid, np2cfg.sndwssdma, np2cfg.sndwssirq);
	setWaveStar(np2cfg.sndwsio, np2cfg.sndwsdma, np2cfg.sndwsirq, np2cfg.sndwspnp, np2cfg.sndwspcm);
#if defined(SUPPORT_SOUND_SB16)
	setSb16(np2cfg.sndsb16io, np2cfg.sndsb16dma, np2cfg.sndsb16irq);
#endif
	setSpb(np2cfg.spbopt, np2cfg.spb_vrc);
	m_ui->SPBVRLEVEL->setValue(np2cfg.spb_vrl);
	m_ui->SPBREVERSE->setChecked(np2cfg.spb_x != 0);

	fillJoypads();
#if defined(SUPPORT_GAMEPORT)
	m_ui->PAD1_GAMEPORT->setChecked(np2cfg.gameport & 1);
	m_ui->PAD1_ANALOG->setChecked(np2cfg.analogjoy & 1);
	m_ui->PAD1_ANALOG->setEnabled(np2cfg.gameport & 1);
#endif
#if defined(SUPPORT_FMGEN)
	m_ui->USEFMGEN->setChecked(np2cfg.usefmgen != 0);
#endif
}

void SoundPage::save()
{
	np2cfg.SOUND_SW = static_cast<UINT8>(m_ui->SNDboard->currentData().toInt());
	np2cfg.samplingrate = comboValue(m_ui->SampleHz, kSampleRates);
	np2cfg.delayms = static_cast<UINT16>(m_ui->Latencys->value());

	np2cfg.vol_master = static_cast<UINT8>(m_ui->vol_master->value());
	np2cfg.vol_fm = static_cast<UINT8>(m_ui->vol_fm->value());
	np2cfg.vol_ssg = static_cast<UINT8>(m_ui->vol_ssg->value());
	np2cfg.vol_adpcm = static_cast<UINT8>(m_ui->vol_adpcm->value());
	np2cfg.vol_pcm = static_cast<UINT8>(m_ui->vol_pcm->value());
	np2cfg.vol_rhythm = static_cast<UINT8>(m_ui->vol_rhythm->value());
	np2cfg.davolume = static_cast<UINT8>(m_ui->davolume->value());
	np2cfg.vol_midi = static_cast<UINT8>(m_ui->vol_midi->value());
	np2cfg.MOTORVOL = static_cast<UINT8>(m_ui->MOTORVOL->value());
	QRadioButton *const beep[4] = { m_ui->beep_off, m_ui->beep_low, m_ui->beep_mid, m_ui->beep_high };
	for (UINT8 i = 0; i < 4; i++) {
		if (beep[i]->isChecked()) {
			np2cfg.BEEP_VOL = i;
		}
	}

	QSlider *const vol14[6] = { m_ui->vol14_0, m_ui->vol14_1, m_ui->vol14_2,
	                            m_ui->vol14_3, m_ui->vol14_4, m_ui->vol14_5 };
	for (int i = 0; i < 6; i++) {
		np2cfg.vol14[i] = static_cast<UINT8>(vol14[i]->value());
	}

	np2cfg.snd26opt = m_snd26;
	np2cfg.snd86opt = m_snd86;
	np2cfg.snd118io = comboValue(m_ui->SND118IO, kIo118);
	np2cfg.snd118id = comboValue(m_ui->SND118ID, kId118);
	np2cfg.snd118dma = comboValue(m_ui->SND118DMA, kDma118);
	np2cfg.snd118irqf = comboValue(m_ui->SND118INTF, kIrq118F);
	np2cfg.snd118irqp = comboValue(m_ui->SND118INTP, kIrq118P);
	np2cfg.snd118irqm = comboValue(m_ui->SND118INTM, kIrq118M);
	np2cfg.snd118rom = m_ui->SND118ROM->isChecked() ? 1 : 0;
	np2cfg.sndwssid = comboValue(m_ui->SNDWSSID, kId118);
	np2cfg.sndwssdma = comboValue(m_ui->SNDWSSDMA, kDma118);
	np2cfg.sndwssirq = comboValue(m_ui->SNDWSSINT, kIrq118P);
	np2cfg.sndwsio = comboValue(m_ui->SNDWSIO, kIoWaveStar);
	np2cfg.sndwsdma = comboValue(m_ui->SNDWSDMA, kDmaWaveStar);
	np2cfg.sndwsirq = comboValue(m_ui->SNDWSINT, kIrqWaveStar);
	np2cfg.sndwspnp = m_ui->SNDWSPNP->isChecked() ? 1 : 0;
	np2cfg.sndwspcm = m_ui->SNDWSPCM->isChecked() ? 1 : 0;
#if defined(SUPPORT_SOUND_SB16)
	np2cfg.sndsb16io = comboValue(m_ui->SNDSB16IO, kIoSb16);
	np2cfg.sndsb16dma = comboValue(m_ui->SNDSB16DMA, kDmaSb16);
	np2cfg.sndsb16irq = comboValue(m_ui->SNDSB16INT, kIrqSb16);
#endif
	np2cfg.spbopt = m_spb;
	np2cfg.spb_vrc = m_spbvr;
	np2cfg.spb_vrl = static_cast<UINT8>(m_ui->SPBVRLEVEL->value());
	np2cfg.spb_x = m_ui->SPBREVERSE->isChecked() ? 1 : 0;

	milstr_ncpy(np2oscfg.JOYDEV[0], m_ui->joypad->currentData().toString().toUtf8().constData(),
	            NELEMENTS(np2oscfg.JOYDEV[0]));
#if defined(SUPPORT_GAMEPORT)
	np2cfg.gameport = m_ui->PAD1_GAMEPORT->isChecked() ? 1 : 0;
	np2cfg.analogjoy = m_ui->PAD1_ANALOG->isChecked() ? 1 : 0;
#endif
#if defined(SUPPORT_FMGEN)
	np2cfg.usefmgen = m_ui->USEFMGEN->isChecked() ? 1 : 0;
#endif
}
