/* === preferences dialog for the Qt port === */

#include <compiler.h>
#include "prefdialog.h"
#include "ui_prefdialog.h"
#include "prefpage.h"
#include "systempage.h"
#include "displaypage.h"
#include "soundpage.h"
#include "inputpage.h"
#include "fddpage.h"
#include "hddpage.h"
#include "miscpage.h"
#include "midipage.h"
#include "serialpage.h"
#include "networkpage.h"
#include "hostdrvpage.h"
#include "dipswpage.h"
#include "calendarpage.h"
#include "np2.h"
#include "joymng.h"
#include "scrnmng.h"

#include <pccore.h>
#include <sysmng.h>
#include <font/font.h>
#include <io/iocore.h>
#include <timing.h>
#include <sound/sound.h>
#include <sound/beep.h>
#include <sound/tms3631.h>
#include <sound/fmboard.h>
#include <sound/opngen.h>
#include <sound/psggen.h>
#include <sound/rhythm.h>
#include <sound/adpcm.h>
#include <sound/pcm86.h>
#include <sound/oplgen.h>
#if defined(SUPPORT_FMGEN)
#include <sound/opna.h>
#endif
#include <vram/palettes.h>
#include <vram/scrndraw.h>
#include <vram/dispsync.h>
#if defined(SUPPORT_WAB)
#include <wab/wab.h>
#endif

#include <QComboBox>
#include <QGuiApplication>
#include <QMessageBox>
#include <QScreen>
#include <QScrollArea>
#include <QPushButton>

#include <cstring>

PrefDialog::PrefDialog(QWidget *parent)
	: QDialog(parent)
	, m_ui(new Ui::PrefDialog)
	, m_needsReset(false)
	, m_needsRestart(false)
{
	m_ui->setupUi(this);

	addPage(new SystemPage, tr("System"));
	addPage(new DisplayPage, tr("Display"));
	addPage(new SoundPage, tr("Sound"));
	addPage(new InputPage, tr("Input"));
	addPage(new FddPage, tr("FDD"));
#if defined(SUPPORT_IDEIO)
	addPage(new HddPage, tr("HDD"));
#endif
	addPage(new MidiPage, tr("MIDI"));
	addPage(new SerialPage, tr("Serial"));
#if defined(SUPPORT_NET) || defined(SUPPORT_LGY98)
	addPage(new NetworkPage, tr("Network"));
#endif
#if defined(SUPPORT_HOSTDRV)
	addPage(new HostdrvPage, tr("Host Drive"));
#endif
	addPage(new DipswPage, tr("DIP Switches"));
	addPage(new CalendarPage, tr("Calendar"));
	addPage(new MiscPage, tr("Misc"));

	/* Lists filled in code got their size while still empty when a layout
	 * asked early (setupUi() showing a page's first tab does), and kept it:
	 * the same ports came out wider on one tab than on the other. A combo
	 * box forgets its sizes on a style change. */
	for (PrefPage *page : std::as_const(m_pages)) {
		for (QComboBox *combo : page->findChildren<QComboBox *>()) {
			QEvent styleChange(QEvent::StyleChange);
			QCoreApplication::sendEvent(combo, &styleChange);
		}
	}

	/* The list as wide as its longest name */
	m_ui->pageList->setFixedWidth(m_ui->pageList->sizeHintForColumn(0)
	                              + 2 * m_ui->pageList->frameWidth() + 16);
	m_ui->pageList->setCurrentRow(0);

	/* Every page fits without scrolling where the screen allows it; on a
	 * small one the pages scroll instead of the dialog growing past it. */
	QSize full(0, 0);
	for (PrefPage *page : std::as_const(m_pages)) {
		full = full.expandedTo(page->sizeHint());
	}
	m_ui->pages->setMinimumSize(0, 0);
	const QSize chrome = sizeHint() - m_ui->pages->sizeHint();
	QSize wanted = chrome + full + QSize(4, 4);
	if (const QScreen *screen = parent ? parent->screen() : QGuiApplication::primaryScreen()) {
		wanted = wanted.boundedTo(screen->availableSize() * 9 / 10);
	}
	resize(wanted);

	QPushButton *defaults = m_ui->buttons->button(QDialogButtonBox::RestoreDefaults);
	defaults->setToolTip(tr("Puts the default settings on this page"));
	connect(defaults, &QPushButton::clicked, this, &PrefDialog::restoreDefaults);
}

PrefDialog::~PrefDialog() = default;

void PrefDialog::addPage(PrefPage *page, const QString &title)
{
	page->load();
	m_pages.append(page);
	auto *scroll = new QScrollArea;
	scroll->setWidget(page);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	m_ui->pages->addWidget(scroll);
	m_ui->pageList->addItem(title);
}

/* The defaults of the current page only, shown but not taken until OK */
void PrefDialog::restoreDefaults()
{
	const int index = m_ui->pages->currentIndex();
	PrefPage *page = (index >= 0 && index < m_pages.size()) ? m_pages[index] : nullptr;
	if (!page) {
		return;
	}
	const NP2CFG cfg = np2cfg;
	const NP2OSCFG oscfg = np2oscfg;
#if defined(SUPPORT_WAB)
	const NP2WABCFG wabcfg = np2wabcfg;
#endif
	char shotPath[sizeof(cycle_shot_path)];
	std::memcpy(shotPath, cycle_shot_path, sizeof(shotPath));
	const UINT32 shotInterval = cycle_shot_interval;

	pccore_setdefault();
	np2oscfg_setdefault();
	np2wabcfg_setdefault();
	page->load();

	np2cfg = cfg;
	np2oscfg = oscfg;
#if defined(SUPPORT_WAB)
	np2wabcfg = wabcfg;
#endif
	std::memcpy(cycle_shot_path, shotPath, sizeof(shotPath));
	cycle_shot_interval = shotInterval;
}

void PrefDialog::applyVolumes()
{
	const UINT master = np2cfg.vol_master;
	opngen_setvol(np2cfg.vol_fm * master / 100);
	psggen_setvol(np2cfg.vol_ssg * master / 100);
	rhythm_setvol(np2cfg.vol_rhythm * master / 100);
	adpcm_setvol(np2cfg.vol_adpcm * master / 100);
	pcm86gen_setvol(np2cfg.vol_pcm * master / 100);
	oplgen_setvol(np2cfg.vol_fm * master / 100);
#if defined(SUPPORT_FMGEN)
	opna_fmgen_setallvolumeFM_linear(np2cfg.vol_fm * master / 100);
	opna_fmgen_setallvolumePSG_linear(np2cfg.vol_ssg * master / 100);
	opna_fmgen_setallvolumeRhythmTotal_linear(np2cfg.vol_rhythm * master / 100);
	opna_fmgen_setallvolumeADPCM_linear(np2cfg.vol_adpcm * master / 100);
#endif
	/* The chips already running keep their own copy */
	for (UINT i = 0; i < NELEMENTS(g_opna); i++) {
		adpcm_update(&g_opna[i].adpcm);
		rhythm_update(&g_opna[i].rhythm);
	}
	pcm86gen_update();
	tms3631_setvol(np2cfg.vol14);
	beep_setvol(np2cfg.BEEP_VOL);
}

/* What a running machine picks up straight away; everything else in np2cfg is
 * read by pccore_reset(). */
static void copyLiveSettings(NP2CFG &to, const NP2CFG &from)
{
	to.vol_master = from.vol_master;
	to.vol_fm = from.vol_fm;
	to.vol_ssg = from.vol_ssg;
	to.vol_adpcm = from.vol_adpcm;
	to.vol_pcm = from.vol_pcm;
	to.vol_rhythm = from.vol_rhythm;
	to.vol_midi = from.vol_midi;
	to.davolume = from.davolume;
	std::memcpy(to.vol14, from.vol14, sizeof(to.vol14));
	to.BEEP_VOL = from.BEEP_VOL;
	to.MOTOR = from.MOTOR;
	to.DISPSYNC = from.DISPSYNC;
	to.RASTER = from.RASTER;
	to.realpal = from.realpal;
	to.LCD_MODE = from.LCD_MODE;
	to.skipline = from.skipline;
	to.skiplight = from.skiplight;
	to.uPD72020 = from.uPD72020;
	to.grcg = from.grcg;
	to.emuspeed = from.emuspeed;
	to.gdcwovrg = from.gdcwovrg;
	to.BTN_MODE = from.BTN_MODE;
	to.BTN_RAPID = from.BTN_RAPID;
	to.MOUSERAPID = from.MOUSERAPID;
	to.keyrepeat_enable = from.keyrepeat_enable;
	to.keyrepeat_delay = from.keyrepeat_delay;
	to.keyrepeat_interval = from.keyrepeat_interval;
	std::memcpy(to.fontfile, from.fontfile, sizeof(to.fontfile));
	to.calendar = from.calendar;
	to.cal_vofs = from.cal_vofs;
#if defined(SUPPORT_HOSTDRV)
	/* Read when the guest attaches the drive */
	to.hdrvenable = from.hdrvenable;
	to.hdrvacc = from.hdrvacc;
	std::memcpy(to.hdrvroot, from.hdrvroot, sizeof(to.hdrvroot));
#endif
}

/* The MIDI and serial connections are opened when the PC-98 is reset */
static bool connectionsChanged(const NP2OSCFG &a, const NP2OSCFG &b)
{
	bool changed = std::memcmp(&a.mpu, &b.mpu, sizeof(a.mpu)) != 0
	            || std::memcmp(a.com, b.com, sizeof(a.com)) != 0
	            || std::memcmp(a.MIDIDEV, b.MIDIDEV, sizeof(a.MIDIDEV)) != 0;
#if defined(SUPPORT_SMPU98)
	changed = changed
	       || std::memcmp(&a.smpuA, &b.smpuA, sizeof(a.smpuA)) != 0
	       || std::memcmp(&a.smpuB, &b.smpuB, sizeof(a.smpuB)) != 0
	       || std::memcmp(a.MIDIDEVA, b.MIDIDEVA, sizeof(a.MIDIDEVA)) != 0
	       || std::memcmp(a.MIDIDEVB, b.MIDIDEVB, sizeof(a.MIDIDEVB)) != 0;
#endif
	return changed;
}

void PrefDialog::accept()
{
	const NP2CFG before = np2cfg;
	const NP2OSCFG osBefore = np2oscfg;
	const char draw32Before = draw32bit;
#if defined(SUPPORT_WAB)
	const int multithreadBefore = np2wabcfg.multithread;
#endif

	for (PrefPage *page : std::as_const(m_pages)) {
		page->save();
	}

	/* Sound */
	applyVolumes();
	if (np2cfg.samplingrate != before.samplingrate || np2cfg.delayms != before.delayms
	    || np2cfg.MOTORVOL != before.MOTORVOL) {
		/* The stream and the seek sound are made again by the next reset */
		soundrenewal = 1;
	}

	/* Display */
	if (np2cfg.skipline != before.skipline || np2cfg.skiplight != before.skiplight) {
		pal_makeskiptable();
	}
	if (np2cfg.LCD_MODE != before.LCD_MODE) {
		pal_makelcdpal();
	}
	if (np2cfg.uPD72020 != before.uPD72020) {
		gdc_restorekacmode();
	}
	if (np2cfg.uPD72020 != before.uPD72020 || np2cfg.grcg != before.grcg) {
		gdcs.grphdisp |= GDCSCRN_ALLDRAW2;
	}
	if (np2cfg.gdcwovrg != before.gdcwovrg) {
		dispsync_updateoverrange();
	}
	if (draw32bit != draw32Before) {
		scrnmng_destroy();
		scrnmng_create(scrnmode);
	}
	if (std::strcmp(np2cfg.fontfile, before.fontfile) != 0) {
		/* An empty name builds the font from the host's */
		if (font_load(np2cfg.fontfile, np2cfg.fontfile[0] == '\0', np2cfg.fontface)) {
			gdcs.textdisp |= GDCSCRN_ALLDRAW2;
		} else {
			QMessageBox::warning(this, windowTitle(),
			    tr("%1 is not a font this emulator can read; the font is unchanged.")
			        .arg(QString::fromUtf8(np2cfg.fontfile)));
			std::memcpy(np2cfg.fontfile, before.fontfile, sizeof(np2cfg.fontfile));
		}
	}
	scrndraw_redraw();

	/* System */
	if (np2cfg.emuspeed != before.emuspeed) {
		timing_setspeed(np2cfg.emuspeed * 128 / 100);
	}

	/* Input */
	if (std::strcmp(np2oscfg.JOYDEV[0], osBefore.JOYDEV[0]) != 0) {
		joymng_deinitialize();
		joymng_initialize();
	}

	NP2CFG resetOnly = before;
	copyLiveSettings(resetOnly, np2cfg);
	m_needsReset = (std::memcmp(&resetOnly, &np2cfg, sizeof(NP2CFG)) != 0)
	            || connectionsChanged(osBefore, np2oscfg);
#if defined(SUPPORT_WAB)
	m_needsRestart = (np2wabcfg.multithread != multithreadBefore);
#endif

	sysmng_update(SYS_UPDATECFG | SYS_UPDATEOSCFG);
	QDialog::accept();
}
