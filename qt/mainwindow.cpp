/* === main window for the Qt port ===
 * Mirrors Np2Frame (wx/np2frame.cpp): owns the emulation timer or thread,
 * the menus and the hooks the shared wx code calls (wx/np2ui.h).
 */

#include <compiler.h>
#include "mainwindow.h"
#include "screenwidget.h"
#include "pointercapture.h"
#include "newdisk/diskcreator.h"
#include "prefs/prefdialog.h"
#include "keydispwindow.h"
#include "cycleshotdialog.h"
#include "statusindicators.h"
#include "np2ui.h"
#include "np2mt.h"
#include "np2.h"
#include "scrnmng.h"
#include "soundmng.h"
#include <pccore.h>
#include <statsave.h>
#include <timing.h>
#include <codecnv/codecnv.h>
#include <font/font.h>
#include <keystat.h>
#include <sysmng.h>
#include <dosio.h>
#include <diskimage/fddfile.h>
#include <fdd/diskdrv.h>
#include <fdd/sxsi.h>
#include <io/iocore.h>
#include <wab/wab.h>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSaveFile>
#include <QScreen>
#include <QSlider>
#include <QStatusBar>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

#include <atomic>

static MainWindow *s_window = nullptr;

/* scrnmng_update() runs once per emulated frame on the emulator thread;
 * one queued repaint at a time is enough. */
static std::atomic<bool> s_redrawPending{false};

/* ---- hooks for the shared wx code (wx/np2ui.h) ---- */

extern "C" void np2frame_requestRedraw(void)
{
	if (s_window && !s_redrawPending.exchange(true)) {
		QMetaObject::invokeMethod(s_window, &MainWindow::refreshScreen, Qt::QueuedConnection);
	}
}

extern "C" void np2frame_updateCaption(UINT8 flag)
{
	if (s_window) {
		const int f = flag;
		QMetaObject::invokeMethod(s_window, [f]() {
			if (s_window) {
				s_window->updateCaption(f);
			}
		}, Qt::QueuedConnection);
	}
}

extern "C" void np2frame_setTitle(const char *title)
{
	if (s_window) {
		const QString t = QString::fromUtf8(title);
		QMetaObject::invokeMethod(s_window, [t]() {
			if (s_window) {
				s_window->setWindowTitle(t);
			}
		}, Qt::QueuedConnection);
	}
}

/* ---- window ---- */

MainWindow::MainWindow()
	: m_screen(new ScreenWidget(this))
	, m_keyDisp(nullptr)
	, m_running(true)
	, m_volButton(nullptr)
	, m_speedButton(nullptr)
	, m_indicators(nullptr)
	, m_lastVol(np2cfg.vol_master ? np2cfg.vol_master : 100)
	, m_lastSpeed(np2cfg.emuspeed != 100 ? np2cfg.emuspeed : 200)
	, m_shownVol(-1)
	, m_shownSpeed(0)
{
	s_window = this;
	setCentralWidget(m_screen);
	connect(&m_cycleShotTimer, &QTimer::timeout, this, &MainWindow::cycleShot);
	buildStatusBar();
	buildMenus();
	m_indicators->setMenus(m_fdMenu, m_hdMenu, m_cdMenu);
	updateLoadState();
	/* The core sets the title only when a drive changes; with the drives
	 * mounted at startup from the beginning */
	sysmng_updatecaption(SYS_UPDATECAPTION_ALL);

	connect(m_screen, &ScreenWidget::fullScreenToggleRequested, this, &MainWindow::toggleFullScreen);
	connect(m_screen, &ScreenWidget::menuBarToggleRequested, this, &MainWindow::toggleMenuBar);
	connect(m_screen, &ScreenWidget::frameSizeChanged, this, &MainWindow::fitToFrame);
	connect(m_screen, &ScreenWidget::mouseCaptureChanged, this, &MainWindow::showMouseCapture);
	connect(&m_emuTimer, &QTimer::timeout, this, &MainWindow::onEmuTimer);

	/* Always initialised: it also sets up the nevent mutex */
	np2_multithread_initialize();
	if (np2wabcfg.multithread) {
		/* The emulator has its own thread; the timer only refreshes the UI */
		np2_multithread_start();
		m_emuTimer.start(50);
	} else {
		m_emuTimer.start(1);
	}

	if (np2oscfg.cycle_shot_enabled) {
		setCycleShot(true);
	}
	if (np2oscfg.keydisp) {
		/* Once the main window is up, so it opens next to it */
		QTimer::singleShot(0, this, [this]() { showKeyDisp(true); });
	}
	m_screen->setFocus();
}

MainWindow::~MainWindow()
{
	np2_multithread_shutdown();
	s_window = nullptr;
}

void MainWindow::onEmuTimer()
{
	if (np2wabcfg.multithread) {
		updateMenuStatus();
	} else if (m_running) {
		np2_exec();
	}
}

void MainWindow::stopEmulation()
{
	soundmng_stop();
	np2_multithread_suspend();
}

void MainWindow::startEmulation()
{
	if (!m_running) {
		return;
	}
	np2_multithread_resume();
	soundmng_play();
}

void MainWindow::refreshScreen()
{
	s_redrawPending = false;
	m_screen->updateFrame();
}

void MainWindow::updateCaption(int flag)
{
	sysmng_updatecaption(static_cast<UINT8>(flag));
	updateMenuStatus();
}

/* ---- menus ---- */

/* The speeds of the wx menu and of the System page */
static const UINT32 kSpeeds[6] = { 50, 100, 200, 400, 800, 1600 };

static QString speedLabel(UINT32 speed)
{
	return QStringLiteral("x%1").arg(speed / 100.0);
}

/* Short enough for the corner of an icon */
static QString speedBadge(UINT32 speed)
{
	return (speed == 50) ? QStringLiteral("x\u00bd") : speedLabel(speed);
}

/* qtnp21kai.s00 to .s09 next to the configuration, as the X and SDL ports name
 * them; wx's "s0" is also the file the resume state goes to. */
static QString stateExt(int slot)
{
	return QStringLiteral("s%1").arg(slot, 2, 10, QLatin1Char('0'));
}

static QString stateFile(int slot)
{
	const QByteArray name = QByteArrayLiteral(NP2_WX_APPNAME ".") + stateExt(slot).toLatin1();
	return QFile::decodeName(file_getcd(name.constData()));
}

/* The shortcuts of the wx port, plus Ctrl+Q. The screen widget lets them
 * through only while the mouse is free, so a captured mouse leaves every key,
 * F5 and Ctrl+V included, to the PC-98. Alt+Enter, F11 and F12 always work. */
void MainWindow::buildMenus()
{
	/* Added to the window too: a hidden menu bar (full screen) would
	 * switch its actions' shortcuts off */
	auto shortcut = [this](QAction *action, const QKeySequence &key) {
		action->setShortcut(key);
		addAction(action);
	};

	QMenu *emu = menuBar()->addMenu(tr("&Emulate"));
	m_actStartPause = emu->addAction(tr("&Pause"), this, &MainWindow::startPause);
	shortcut(m_actStartPause, QKeySequence(Qt::Key_F5));
	emu->addAction(tr("&Reset"), this, &MainWindow::reset);
	QMenu *speed = emu->addMenu(tr("Spee&d"));
	auto *speedGroup = new QActionGroup(this);
	/* A speed set elsewhere may match none of them */
	speedGroup->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
	for (int i = 0; i < 6; i++) {
		const UINT32 value = kSpeeds[i];
		m_actSpeed[i] = speed->addAction(speedLabel(value), this, [this, value]() { setSpeed(value); });
		m_actSpeed[i]->setCheckable(true);
		speedGroup->addAction(m_actSpeed[i]);
	}
	emu->addSeparator();
	shortcut(emu->addAction(tr("&Save State"), this, &MainWindow::saveState), QKeySequence(Qt::Key_F8));
	m_actLoadState = emu->addAction(tr("&Load State"), this, &MainWindow::loadState);
	shortcut(m_actLoadState, QKeySequence(Qt::Key_F7));
	QMenu *slot = emu->addMenu(tr("State Sl&ot"));
	auto *slotGroup = new QActionGroup(this);
	for (int i = 0; i < 10; i++) {
		m_actSlot[i] = slot->addAction(tr("Slot &%1").arg(i), this, [this, i]() { setStateSlot(i); });
		m_actSlot[i]->setCheckable(true);
		slotGroup->addAction(m_actSlot[i]);
	}
	emu->addSeparator();
	shortcut(emu->addAction(tr("Pr&eferences..."), this, &MainWindow::openPreferences),
	         QKeySequence(Qt::CTRL | Qt::Key_P));
	emu->addSeparator();
	shortcut(emu->addAction(tr("E&xit"), this, &QWidget::close), QKeySequence::Quit);

	/* All drives in one menu, in the order of the status bar, whose icons
	 * open the same submenus: the floppy drives that are on, then each slot
	 * as the hard disk or the CD-ROM drive set in it. The digits stay the
	 * floppies' mnemonics. */
	QMenu *media = menuBar()->addMenu(tr("&Media"));
	for (int drv = 0; drv < 4; drv++) {
		m_fdMenu[drv] = media->addMenu(tr("FD &%1").arg(drv + 1));
		m_fdMenu[drv]->addAction(tr("&Open Image..."), this, [this, drv]() { openFd(drv); });
		m_actFdEject[drv] = m_fdMenu[drv]->addAction(tr("&Eject"), this, [this, drv]() { ejectFd(drv); });
	}
	media->addSeparator();
	for (int drv = 0; drv < 4; drv++) {
		m_hdMenu[drv] = nullptr;
		m_actHdEject[drv] = nullptr;
		m_cdMenu[drv] = nullptr;
		m_actCdEject[drv] = nullptr;
		if (drv >= hdSlots()) {
			continue;
		}
		m_hdMenu[drv] = media->addMenu(tr("HD %1").arg(drv + 1));
		m_hdMenu[drv]->addAction(tr("&Open Image..."), this, [this, drv]() { openHd(drv); });
		m_actHdEject[drv] = m_hdMenu[drv]->addAction(tr("&Eject", "hard disk"), this, [this, drv]() { ejectHd(drv); });
		m_cdMenu[drv] = media->addMenu(tr("&CD-ROM"));
		m_cdMenu[drv]->addAction(tr("&Open Image..."), this, [this, drv]() { openCd(drv); });
		m_actCdEject[drv] = m_cdMenu[drv]->addAction(tr("&Eject"), this, [this, drv]() { ejectCd(drv); });
	}
	/* A new image is made where it will be inserted */
	media->addSeparator();
	media->addAction(tr("&New Floppy Image..."), this, [this]() { DiskCreator::newFdImage(this); });
	media->addAction(tr("New &Hard Disk Image..."), this, [this]() { DiskCreator::newHdImage(this); });

	QMenu *view = menuBar()->addMenu(tr("&View"));
	m_actFullScreen = view->addAction(tr("&Full Screen"), this, &MainWindow::toggleFullScreen);
	m_actFullScreen->setCheckable(true);
	m_actFullScreen->setShortcuts({ QKeySequence(Qt::ALT | Qt::Key_Enter), QKeySequence(Qt::ALT | Qt::Key_Return) });
	addAction(m_actFullScreen);
	QMenu *size = view->addMenu(tr("&Window Size"));
	auto *sizeGroup = new QActionGroup(this);
	/* A window sized by hand is none of them */
	sizeGroup->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
	for (int scale = 1; scale <= 3; scale++) {
		m_actSize[scale - 1] = size->addAction(tr("&x%1").arg(scale), this, [this, scale]() { setScreenScale(scale); });
		m_actSize[scale - 1]->setCheckable(true);
		sizeGroup->addAction(m_actSize[scale - 1]);
	}
	connect(size, &QMenu::aboutToShow, this, &MainWindow::updateSizeMenu);
	view->addSeparator();
	m_actPixelPerfect = view->addAction(tr("&Pixel-perfect"), this, &MainWindow::setPixelPerfect);
	m_actPixelPerfect->setCheckable(true);
	m_actPixelPerfect->setStatusTip(tr("Enlarge the PC-98 pixels by whole multiples only, mostly seen in full screen"));
	m_actForce43 = view->addAction(tr("Force &4:3 Aspect Ratio"), this, &MainWindow::setForce43);
	m_actForce43->setCheckable(true);
	m_actForce43->setStatusTip(tr("Stretch the screen to 4:3, as a CRT showed 640x400"));
	/* In the order of the wx port's menu */
	QMenu *rotate = view->addMenu(tr("&Rotate"));
	auto *rotateGroup = new QActionGroup(this);
	const QString rotateLabels[4] = {
		tr("&Normal"), tr("90\u00b0 &Left"), tr("&180\u00b0"), tr("90\u00b0 &Right"),
	};
	for (int i = 0; i < 4; i++) {
		m_actRotate[i] = rotate->addAction(rotateLabels[i], this, [this, i]() { setRotation(i); });
		m_actRotate[i]->setCheckable(true);
		rotateGroup->addAction(m_actRotate[i]);
	}
	view->addSeparator();
	m_actStatusBar = view->addAction(tr("&Status Bar"), this, &MainWindow::setStatusBarShown);
	m_actStatusBar->setCheckable(true);

	QMenu *input = menuBar()->addMenu(tr("&Input"));
	QAction *paste = input->addAction(tr("&Paste Text"), this, &MainWindow::pasteText);
	paste->setStatusTip(tr("Type the text from the clipboard on the PC-98 keyboard"));
	shortcut(paste, QKeySequence(Qt::CTRL | Qt::Key_V));
	input->addAction(tr("Send &Ctrl+Alt+Del"), this, &MainWindow::sendCtrlAltDel)
	    ->setStatusTip(tr("Send hardware reset sequence"));

	QMenu *tools = menuBar()->addMenu(tr("&Tools"));
	QAction *copy = tools->addAction(tr("&Copy Screen"), this, &MainWindow::copyScreen);
	copy->setStatusTip(tr("Copy screen to clipboard"));
	shortcut(copy, QKeySequence(Qt::CTRL | Qt::Key_C));
	tools->addAction(tr("&Save Screenshot..."), this, &MainWindow::savePng)
	    ->setStatusTip(tr("Save screen as PNG image"));
	m_actCycleShot = tools->addAction(tr("Peri&odic Screenshot..."), this, [this](bool on) {
		if (on) {
			CycleShotDialog dialog(this);
			if (dialog.exec() != QDialog::Accepted) {
				on = false;
			}
		}
		setCycleShot(on);
	});
	m_actCycleShot->setCheckable(true);
	m_actCycleShot->setStatusTip(tr("Periodically save screenshots to a file"));
	tools->addSeparator();
	m_actTextHook = tools->addAction(tr("&Text ROM Hook"), this, &MainWindow::setTextHook);
	m_actTextHook->setCheckable(true);
	m_actTextHook->setStatusTip(tr("Write the text read from the font ROM to %1")
	    .arg(QStringLiteral(HF_FILENAME)));
	m_actKeyDisp = tools->addAction(tr("&Key Display"), this, &MainWindow::showKeyDisp);
	m_actKeyDisp->setCheckable(true);
	m_actKeyDisp->setStatusTip(tr("Show the notes the FM and MIDI channels play"));

	QMenu *help = menuBar()->addMenu(tr("&Help"));
	help->addAction(tr("&About %1...").arg(QStringLiteral(NP2_WX_TITLE)), this, &MainWindow::about);

	/* Menus take keyboard focus; give it back to the screen afterwards */
	for (QAction *a : menuBar()->actions()) {
		if (QMenu *m = a->menu()) {
			connect(m, &QMenu::aboutToHide, m_screen, [this]() { m_screen->setFocus(); });
		}
	}

	updateMenuStatus();
}

/* Menu position of each rotation, as in the wx port */
static const UINT8 kRotateModes[4] = {
	0,
	SCRNMODE_ROTATELEFT,
	SCRNMODE_ROTATEDIR,
	SCRNMODE_ROTATERIGHT,
};

static int rotationIndex()
{
	const UINT8 rot = scrnmode & SCRNMODE_ROTATEMASK;
	for (int i = 0; i < 4; i++) {
		if (kRotateModes[i] == rot) {
			return i;
		}
	}
	return 0;
}

/* The image's name for a menu: a long one shortened in the middle, so its
 * extension stays, and its & not taken for a mnemonic */
static QString menuFileName(const OEMCHAR *path)
{
	if (!path || !path[0]) {
		return QString();
	}
	QString name = QString::fromUtf8(file_getname(const_cast<OEMCHAR *>(path)));
	const int kMax = 32;
	if (name.size() > kMax) {
		const int tail = 12;
		name = name.left(kMax - tail - 1) + QChar(0x2026) + name.right(tail);
	}
	return name.replace(QLatin1Char('&'), QStringLiteral("&&"));
}

static QString withFileName(const QString &label, const OEMCHAR *path)
{
	const QString name = menuFileName(path);
	return name.isEmpty() ? label : label + QStringLiteral(": ") + name;
}

void MainWindow::updateMenuStatus()
{
	int cdCount = 0;
	for (int i = 0; i < hdSlots(); i++) {
		cdCount += isCdSlot(i) ? 1 : 0;
	}
	int cdNumber = 0;
	/* Eject names the image it takes out, as the status bar's menus are
	 * the same and have no title */
	for (int i = 0; i < 4; i++) {
		/* A drive is turned on in the preferences, as the status bar shows */
		m_fdMenu[i]->menuAction()->setVisible((np2cfg.fddequip & (1 << i)) != 0);
		m_fdMenu[i]->setTitle(withFileName(tr("FD &%1").arg(i + 1), np2cfg.fddfile[i]));
		const QString fd = menuFileName(np2cfg.fddfile[i]);
		m_actFdEject[i]->setText(fd.isEmpty() ? tr("&Eject") : tr("&Eject %1").arg(fd));
		const bool cd = (i < hdSlots()) && isCdSlot(i);
		if (m_hdMenu[i]) {
			const OEMCHAR *path = diskdrv_getsxsi(static_cast<REG8>(i));
			m_hdMenu[i]->menuAction()->setVisible(!cd);
			m_hdMenu[i]->setTitle(withFileName(tr("HD %1").arg(i + 1), path));
			const QString hd = menuFileName(path);
			m_actHdEject[i]->setText(hd.isEmpty() ? tr("&Eject", "hard disk") : tr("&Eject %1", "hard disk").arg(hd));
		}
		if (m_cdMenu[i]) {
			m_cdMenu[i]->menuAction()->setVisible(cd);
		}
		if (cd) {
			/* Numbered only when there is more than one */
			cdNumber++;
			const QString label = (cdCount == 1) ? tr("&CD-ROM") : tr("CD-ROM %1").arg(cdNumber);
			const OEMCHAR *path = sxsi_getfilename(static_cast<REG8>(i));
			m_cdMenu[i]->setTitle(withFileName(label, path));
			const QString disc = menuFileName(path);
			m_actCdEject[i]->setText(disc.isEmpty() ? tr("&Eject") : tr("&Eject %1").arg(disc));
		}
	}
	m_actFullScreen->setChecked(isFullScreen());
	m_actPixelPerfect->setChecked(np2oscfg.pixelperfect != 0);
	m_actForce43->setChecked(np2oscfg.force43 != 0);
	m_actStatusBar->setChecked(np2oscfg.statusbar != 0);
	m_actRotate[rotationIndex()]->setChecked(true);
	for (int i = 0; i < 6; i++) {
		m_actSpeed[i]->setChecked(np2cfg.emuspeed == kSpeeds[i]);
	}
	m_actSlot[qBound(0, np2_stateslotnow, 9)]->setChecked(true);
	m_actCycleShot->setChecked(m_cycleShotTimer.isActive());
	m_actTextHook->setChecked(hf_enable == 1);
	updateStatusBar();
}

/* ---- Emulate ---- */

void MainWindow::startPause()
{
	m_running = !m_running;
	if (m_running) {
		np2_multithread_resume();
		soundmng_play();
		m_actStartPause->setText(tr("&Pause"));
	} else {
		soundmng_stop();
		np2_multithread_suspend();
		m_actStartPause->setText(tr("&Start"));
	}
}

/* With the emulator thread stopped: it may be inside pccore_exec() */
void MainWindow::reset()
{
	stopEmulation();
	resetMachine();
	startEmulation();
}

/* For callers that have stopped the emulation already. The reset applies the
 * device types of the drive slots, which the menus follow. */
void MainWindow::resetMachine()
{
	pccore_reset();
	soundmng_reset();
	updateMenuStatus();
}

void MainWindow::openPreferences()
{
	stopEmulation();
	PrefDialog dialog(this);
	if (dialog.exec() == QDialog::Accepted) {
		if (dialog.needsReset()
		    && QMessageBox::question(this, dialog.windowTitle(),
		           tr("Some of the changes take effect when the PC-98 is reset. Reset it now?"))
		       == QMessageBox::Yes) {
			resetMachine();
		}
		if (dialog.needsRestart()) {
			QMessageBox::information(this, dialog.windowTitle(),
			    tr("The emulation thread setting takes effect the next time the emulator starts."));
		}
		m_screen->update();
	}
	startEmulation();
}

/* ---- drives ---- */

int MainWindow::hdSlots()
{
#if defined(SUPPORT_IDEIO)
	return 4;
#else
	return 2;
#endif
}

/* As the running machine has it: a device type changed in the preferences
 * takes effect when the PC-98 is reset, and the menus and the status bar
 * follow it then */
bool MainWindow::isCdSlot(int slot)
{
#if defined(SUPPORT_IDEIO)
	return sxsi_getdevtype(static_cast<REG8>(slot)) == SXSIDEV_CDROM;
#else
	(void)slot;
	return false;
#endif
}

static QString startDir(const char *folder, const OEMCHAR *current)
{
	if (current && current[0]) {
		return QFileInfo(QString::fromUtf8(current)).absolutePath();
	}
	return QString::fromUtf8(folder);
}

/* fdd/sxsi.c: set while a disc swap is pending */
extern "C" char cdchange_flag;

static void storePath(char *dst, const QString &path)
{
	milstr_ncpy(dst, QFile::encodeName(path).constData(), MAX_PATH);
}

void MainWindow::openFd(int drv)
{
	const QString path = QFileDialog::getOpenFileName(this,
	    tr("Open FD%1 Image").arg(drv + 1),
	    startDir(fddfolder, np2cfg.fddfile[drv]),
	    tr("FD images (*.d88 *.88d *.d98 *.98d *.fdi *.hdm *.hd4 *.xdf *.dup *.2hd *.nfd *.dcp *.dcu *.flp *.vfdd);;All files (*)"));
	if (path.isEmpty()) {
		return;
	}
	stopEmulation();
	np2cfg.fddequip |= static_cast<UINT8>(1 << drv);
	fdc.equip       |= static_cast<UINT8>(1 << drv);
	/* Mounted at once, as at startup, not after the usual delay */
	diskdrv_readyfdd(static_cast<REG8>(drv), QFile::encodeName(path).constData(), 0);
	storePath(np2cfg.fddfile[drv], path);
	storePath(fddfolder, QFileInfo(path).absolutePath());
	sysmng_update(SYS_UPDATEFDD | SYS_UPDATECFG);
	updateMenuStatus();
	startEmulation();
}

void MainWindow::ejectFd(int drv)
{
	stopEmulation();
	np2cfg.fddfile[drv][0] = '\0';
	fdd_eject(static_cast<REG8>(drv));
	sysmng_update(SYS_UPDATEFDD | SYS_UPDATECFG);
	updateMenuStatus();
	startEmulation();
}

void MainWindow::openHd(int drv)
{
	const QString path = QFileDialog::getOpenFileName(this,
	    tr("Open HD%1 Image").arg(drv + 1),
	    startDir(hddfolder, np2cfg.sasihdd[drv]),
	    tr("HD images (*.nhd *.hdi *.thd *.vhd *.hdd *.hdn *.sln);;All files (*)"));
	if (path.isEmpty()) {
		return;
	}
	stopEmulation();
	storePath(hddfolder, QFileInfo(path).absolutePath());
	storePath(np2cfg.sasihdd[drv], path);
#if defined(SUPPORT_IDEIO)
	np2cfg.idetype[drv] = SXSIDEV_HDD;
#endif
	sysmng_update(SYS_UPDATEHDD | SYS_UPDATECFG);
	updateMenuStatus();
	startEmulation();
}

void MainWindow::ejectHd(int drv)
{
	stopEmulation();
	np2cfg.sasihdd[drv][0] = '\0';
#if defined(SUPPORT_IDEIO)
	np2cfg.idetype[drv] = SXSIDEV_NC;
#endif
	sysmng_update(SYS_UPDATEHDD | SYS_UPDATECFG);
	updateMenuStatus();
	startEmulation();
}

void MainWindow::openCd(int drv)
{
	const OEMCHAR *current = nullptr;
#if defined(SUPPORT_IDEIO)
	current = np2cfg.idecd[drv & 0x03];
#endif
	const QString path = QFileDialog::getOpenFileName(this, tr("Mount CD Image"),
	    startDir(cdfolder, current),
	    tr("CD images (*.iso *.cue *.ccd *.mds *.nrg);;All files (*)"));
	if (path.isEmpty()) {
		return;
	}
	stopEmulation();
	storePath(cdfolder, QFileInfo(path).absolutePath());
	/* With a disc already in, the core ejects it, fails and inserts the new
	 * one a few seconds later; only a failure without that is a real one. */
	if (sxsi_devopen(static_cast<REG8>(drv), QFile::encodeName(path).constData()) != SUCCESS
	    && !cdchange_flag) {
		QMessageBox::warning(this, tr("CD-ROM"),
		    tr("Could not open the CD image:\n%1").arg(QDir::toNativeSeparators(path)));
	} else {
#if defined(SUPPORT_IDEIO)
		storePath(np2cfg.idecd[drv & 0x03], path);
#endif
	}
	sysmng_update(SYS_UPDATECFG);
	updateMenuStatus();
	startEmulation();
}

void MainWindow::ejectCd(int drv)
{
	stopEmulation();
	sxsi_devopen(static_cast<REG8>(drv), nullptr);
#if defined(SUPPORT_IDEIO)
	np2cfg.idecd[drv & 0x03][0] = '\0';
#endif
	sysmng_update(SYS_UPDATECFG);
	updateMenuStatus();
	startEmulation();
}

/* ---- display / input ---- */

void MainWindow::toggleFullScreen()
{
	/* Full screen hides the bars, as wx's ShowFullScreen does; F11 can bring
	 * the menu back (F11KEY = 1) */
	if (isFullScreen()) {
		showNormal();
		menuBar()->show();
		statusBar()->setVisible(np2oscfg.statusbar != 0);
	} else {
		menuBar()->hide();
		statusBar()->hide();
		showFullScreen();
	}
	m_actFullScreen->setChecked(isFullScreen());
	m_screen->setFocus();
}

void MainWindow::toggleMenuBar()
{
	/* Only in full screen, as in the X port; a window keeps its menu */
	if (isFullScreen()) {
		menuBar()->setVisible(!menuBar()->isVisible());
	}
}

void MainWindow::setRotation(int index)
{
	const UINT8 newmode = static_cast<UINT8>((scrnmode & ~SCRNMODE_ROTATEMASK) | kRotateModes[index]);
	const bool wasPortrait = (scrnmode & SCRNMODE_ROTATE) != 0;
	const bool portrait = (newmode & SCRNMODE_ROTATE) != 0;

	/* changescreen() rebuilds the screen buffer, which the emulator thread
	 * draws into, and restarts the sound behind our back */
	stopEmulation();
	changescreen(newmode);
	if (!m_running) {
		soundmng_stop();
	}
	startEmulation();

	/* Turn the window with the picture, keeping its scale */
	if (portrait != wasPortrait && !isFullScreen() && !isMaximized()) {
		const QSize extra = size() - m_screen->size();
		resize(m_screen->size().transposed() + extra);
	}
	m_screen->update();
	updateMenuStatus();
}

/* Window size for a whole multiple of the PC-98 screen (logical pixels, so
 * HiDPI scaling comes on top, as in the wx port) */
void MainWindow::setScreenScale(int scale)
{
	if (isFullScreen()) {
		toggleFullScreen();
	} else if (isMaximized()) {
		showNormal();
	}
	int w, h;
	scrnmng_getsize(&w, &h);
	const QSize extra = size() - m_screen->size();
	resize(ScreenWidget::displaySizeFor(QSize(w, h)) * scale + extra);
}

void MainWindow::sendCtrlAltDel()
{
	keystat_keydown(0x74);  /* CTRL */
	keystat_keydown(0x73);  /* GRPH */
	keystat_keydown(0x39);  /* DEL */
	keystat_keyup(0x39);
	keystat_keyup(0x73);
	keystat_keyup(0x74);
}

/* A new PC-98 resolution keeps the window's scale instead of being squeezed
 * into the old size; the first frame fits the window to its aspect ratio. Full
 * screen and a maximised window are left as they are. */
void MainWindow::fitToFrame(const QSize &previous)
{
	const QSize now = m_screen->displaySize();
	if (previous.isEmpty()) {
		/* The saved size may have another aspect ratio; keep its width */
		if (!now.isEmpty()) {
			rescaleWindow(m_screen->width() / double(now.width()), now);
		}
	} else {
		rescaleWindow(ScreenWidget::displaySizeFor(previous), now);
	}
}

/* The window shows a picture of size before; keeps that scale for one of size
 * now */
void MainWindow::rescaleWindow(const QSize &before, const QSize &now)
{
	if (!before.isEmpty()) {
		rescaleWindow(qMin(m_screen->width() / double(before.width()),
		                   m_screen->height() / double(before.height())), now);
	}
}

void MainWindow::rescaleWindow(double scale, const QSize &now)
{
	if (isFullScreen() || isMaximized() || now.isEmpty()) {
		return;
	}
	/* Not past the screen, bars and window frame included */
	const QSize extra = size() - m_screen->size();
	if (const QScreen *s = QWidget::screen()) {
		const QSize room = s->availableSize() - extra - (frameGeometry().size() - size());
		scale = qMin(scale, qMin(room.width() / double(now.width()), room.height() / double(now.height())));
	}
	const QSize target(qRound(now.width() * scale), qRound(now.height() * scale));
	if (target != m_screen->size() && !target.isEmpty()) {
		resize(target + extra);
	}
}

/* Marks the scale the window is at; the PC-98 screen is 640x400, or 640x480
 * with the window accelerator */
void MainWindow::updateSizeMenu()
{
	int w, h;
	scrnmng_getsize(&w, &h);
	const QSize base = ScreenWidget::displaySizeFor(QSize(w, h));
	const bool windowed = !isFullScreen() && !isMaximized();
	for (int scale = 1; scale <= 3; scale++) {
		m_actSize[scale - 1]->setChecked(windowed && m_screen->size() == base * scale);
	}
}

/* The picture keeps its size: the window grows or shrinks by the bar */
void MainWindow::setStatusBarShown(bool on)
{
	np2oscfg.statusbar = on ? 1 : 0;
	sysmng_update(SYS_UPDATEOSCFG);
	if (isFullScreen() || statusBar()->isVisible() == on) {
		return;     /* full screen shows no bars; leaving it applies this */
	}
	statusBar()->setVisible(on);
	if (!isMaximized()) {
		const int bar = statusBar()->sizeHint().height();
		resize(width(), height() + (on ? bar : -bar));
	}
}

void MainWindow::setPixelPerfect(bool on)
{
	np2oscfg.pixelperfect = on ? 1 : 0;
	sysmng_update(SYS_UPDATEOSCFG);
	m_screen->update();
}

/* The window keeps its scale: x2 of 640x400 becomes x2 of 640x480 */
void MainWindow::setForce43(bool on)
{
	const QSize before = m_screen->displaySize();
	np2oscfg.force43 = on ? 1 : 0;
	sysmng_update(SYS_UPDATEOSCFG);
	rescaleWindow(before, m_screen->displaySize());
	m_screen->update();
}

void MainWindow::showKeyDisp(bool show)
{
	if (show) {
		if (!m_keyDisp) {
			m_keyDisp = new KeyDispWindow(this);
			connect(m_keyDisp, &KeyDispWindow::closed, this, [this]() {
				m_actKeyDisp->setChecked(false);
				np2oscfg.keydisp = 0;
				sysmng_update(SYS_UPDATEOSCFG);
			});
		}
		m_keyDisp->show();
		m_keyDisp->raise();
	} else if (m_keyDisp) {
		m_keyDisp->close();
	}
	m_actKeyDisp->setChecked(show);
	np2oscfg.keydisp = show ? 1 : 0;
	sysmng_update(SYS_UPDATEOSCFG);
}

/* Typed in by the core's autokey, converted to Shift_JIS as the wx port does */
void MainWindow::pasteText()
{
	const QByteArray text = QApplication::clipboard()->text().toUtf8();
	if (text.isEmpty()) {
		return;
	}
	const UINT len = codecnv_utf8tosjis(nullptr, 0, text.constData(), static_cast<UINT>(-1));
	if (len == 0) {
		return;
	}
	QByteArray sjis(static_cast<int>(len) + 1, '\0');
	codecnv_utf8tosjis(sjis.data(), len + 1, text.constData(), static_cast<UINT>(-1));
	stopEmulation();
	autokey_start(sjis.constData());
	startEmulation();
}

/* ---- status bar ---- */

void MainWindow::buildStatusBar()
{
	/* As in 86Box: the drives on the left, the volume and the speed right
	 * after them, the mouse and the lock keys on the right. Messages, such as
	 * the menus' status tips, show in place of the left group for a moment. */
	statusBar()->setVisible(np2oscfg.statusbar != 0);
	m_indicators = new StatusIndicators(statusBar(), this);
	auto *left = new QWidget(this);
	auto *layout = new QHBoxLayout(left);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(m_indicators->drivesWidget());
	statusBar()->addWidget(left, 1);
	statusBar()->addPermanentWidget(m_indicators->inputWidget());
	auto makeButton = [left, layout]() {
		auto *button = new QToolButton(left);
		button->setAutoRaise(true);
		button->setFocusPolicy(Qt::NoFocus);
		button->setContextMenuPolicy(Qt::CustomContextMenu);
		layout->addWidget(button);
		return button;
	};
	m_volButton = makeButton();
	connect(m_volButton, &QToolButton::clicked, this, &MainWindow::toggleMute);
	connect(m_volButton, &QWidget::customContextMenuRequested, this, &MainWindow::popupVolume);
	m_speedButton = makeButton();
	connect(m_speedButton, &QToolButton::clicked, this, &MainWindow::toggleSpeed);
	connect(m_speedButton, &QWidget::customContextMenuRequested, this, &MainWindow::popupSpeed);
	layout->addStretch(1);
}

void MainWindow::showMouseCapture(bool captured)
{
	m_indicators->setMouseCaptured(captured);
}

void MainWindow::updateStatusBar()
{
	/* Called from the 50 ms UI timer too; the icon lookup is not free */
	if (!m_volButton || (m_shownVol == np2cfg.vol_master && m_shownSpeed == np2cfg.emuspeed)) {
		return;
	}
	m_shownVol = np2cfg.vol_master;
	m_shownSpeed = np2cfg.emuspeed;
	const bool muted = (np2cfg.vol_master == 0);
	m_volButton->setIcon(muted
	    ? QIcon::fromTheme(QIcon::ThemeIcon::AudioVolumeMuted, style()->standardIcon(QStyle::SP_MediaVolumeMuted))
	    : QIcon::fromTheme(QIcon::ThemeIcon::AudioVolumeHigh, style()->standardIcon(QStyle::SP_MediaVolume)));
	m_volButton->setToolTip(tr("Vol: %1").arg(np2cfg.vol_master));
	/* Fast forward with the speed on it, so it shows without the tooltip */
	m_speedButton->setIcon(badgeIcon(
	    QIcon::fromTheme(QIcon::ThemeIcon::MediaSeekForward, style()->standardIcon(QStyle::SP_MediaSeekForward)),
	    speedBadge(np2cfg.emuspeed)));
	m_speedButton->setToolTip(tr("Speed: %1").arg(speedLabel(np2cfg.emuspeed)));
}

void MainWindow::setVolume(UINT8 volume, bool save)
{
	np2cfg.vol_master = volume;
	PrefDialog::applyVolumes();
	if (save) {
		sysmng_update(SYS_UPDATECFG);
	}
	updateStatusBar();
}

/* pccore_reset() is what applies the speed otherwise */
void MainWindow::setSpeed(UINT32 speed, bool save)
{
	np2cfg.emuspeed = speed;
	timing_setspeed(speed * 128 / 100);
	if (save) {
		sysmng_update(SYS_UPDATECFG);
	}
	updateMenuStatus();
}

void MainWindow::toggleMute()
{
	if (np2cfg.vol_master > 0) {
		m_lastVol = np2cfg.vol_master;
		setVolume(0);
	} else {
		setVolume(m_lastVol ? m_lastVol : 100);
	}
}

void MainWindow::toggleSpeed()
{
	if (np2cfg.emuspeed != 100) {
		m_lastSpeed = np2cfg.emuspeed;
		setSpeed(100);
	} else {
		setSpeed(m_lastSpeed ? m_lastSpeed : 100);
	}
}

/* A vertical slider above the button, as wx's popup window. Its steps change
 * the setting at once; the configuration file is written when it closes. */
static QSlider *popupSlider(QWidget *button, QLabel **label)
{
	auto *menu = new QMenu(button);
	menu->setAttribute(Qt::WA_DeleteOnClose);
	QObject::connect(menu, &QMenu::aboutToHide, menu, []() { sysmng_update(SYS_UPDATECFG); });
	auto *box = new QWidget(menu);
	auto *layout = new QVBoxLayout(box);
	*label = new QLabel(box);
	(*label)->setAlignment(Qt::AlignCenter);
	auto *slider = new QSlider(Qt::Vertical, box);
	slider->setMinimumHeight(150);
	layout->addWidget(*label);
	layout->addWidget(slider, 0, Qt::AlignHCenter);
	auto *action = new QWidgetAction(menu);
	action->setDefaultWidget(box);
	menu->addAction(action);
	QTimer::singleShot(0, menu, [menu, button, slider]() {
		menu->popup(button->mapToGlobal(QPoint(0, -menu->sizeHint().height())));
		slider->setFocus();
	});
	return slider;
}

void MainWindow::popupVolume()
{
	QLabel *label;
	QSlider *slider = popupSlider(m_volButton, &label);
	slider->setRange(0, 100);
	slider->setValue(np2cfg.vol_master);
	label->setNum(np2cfg.vol_master);
	connect(slider, &QSlider::valueChanged, this, [this, label](int value) {
		label->setNum(value);
		setVolume(static_cast<UINT8>(value), false);
	});
}

void MainWindow::popupSpeed()
{
	QLabel *label;
	QSlider *slider = popupSlider(m_speedButton, &label);
	int index = 5;
	while (index > 0 && kSpeeds[index - 1] >= np2cfg.emuspeed) {
		index--;
	}
	slider->setRange(0, 5);
	slider->setPageStep(1);
	slider->setTickPosition(QSlider::TicksBothSides);
	slider->setValue(index);
	label->setText(speedLabel(np2cfg.emuspeed));
	connect(slider, &QSlider::valueChanged, this, [this, label](int value) {
		label->setText(speedLabel(kSpeeds[value]));
		setSpeed(kSpeeds[value], false);
	});
}

/* ---- states ---- */

/* Kept current for F7 too, not only for the menu */
void MainWindow::updateLoadState()
{
	m_actLoadState->setEnabled(QFile::exists(stateFile(np2_stateslotnow)));
}

void MainWindow::setStateSlot(int slot)
{
	np2_stateslotnow = slot;
	updateLoadState();
	updateCaption(SYS_UPDATECAPTION_ALL);
}

void MainWindow::saveState()
{
	const QByteArray ext = stateExt(np2_stateslotnow).toLatin1();
	stopEmulation();
	flagsave(ext.constData());
	const int ret = np2_statsave_flush();
	if (ret != STATFLAG_SUCCESS) {
		flagdelete(ext.constData());
	}
	startEmulation();
	if (ret != STATFLAG_SUCCESS) {
		QMessageBox::warning(this, tr("Save State"),
		    tr("Could not save the state to\n%1").arg(QDir::toNativeSeparators(stateFile(np2_stateslotnow))));
	}
	updateLoadState();
}

/* As the Windows port: a state saved with other disks in may still be wanted */
void MainWindow::loadState()
{
	const QString path = stateFile(np2_stateslotnow);
	const QByteArray name = QFile::encodeName(path);
	char info[1024] = "";
	stopEmulation();
	const int ret = statsave_check(name.constData(), info, sizeof(info));
	bool load = true;
	if (ret & ~STATFLAG_DISKCHG) {
		QMessageBox::warning(this, tr("Load State"),
		    tr("%1 is not a state this version can load.").arg(QDir::toNativeSeparators(path)));
		load = false;
	} else if (ret & STATFLAG_DISKCHG) {
		load = QMessageBox::question(this, tr("Load State"),
		    tr("The state was saved with other disks in:\n\n%1\nLoad it anyway?")
		        .arg(QString::fromUtf8(info))) == QMessageBox::Yes;
	}
	if (load) {
		statsave_load(name.constData());
		np2_statsave_flush();
		sysmng_workclockreset();
		sysmng_updatecaption(SYS_UPDATECAPTION_FDD);
	}
	startEmulation();
	updateMenuStatus();
}

/* ---- screenshots ---- */

/* The frame as on screen, the window accelerator's included; scrnsave, which
 * the wx port saves PNG files with, knows only the PC-98's own VRAM. */
void MainWindow::copyScreen()
{
	const QImage image = m_screen->frame();
	if (!image.isNull()) {
		QApplication::clipboard()->setImage(image);
	}
}

void MainWindow::savePng()
{
	const QImage image = m_screen->frame();
	if (image.isNull()) {
		return;
	}
	const QString dir = bmpfilefolder[0] ? QFile::decodeName(bmpfilefolder) : QDir::homePath();
	QString path = QFileDialog::getSaveFileName(this, tr("Save Screenshot"),
	    QDir(dir).filePath(QStringLiteral("screen.png")), tr("PNG images (*.png)"));
	if (path.isEmpty()) {
		return;
	}
	if (QFileInfo(path).suffix().isEmpty()) {
		path += QStringLiteral(".png");
	}
	storePath(bmpfilefolder, QFileInfo(path).absolutePath());
	sysmng_update(SYS_UPDATEOSCFG);
	if (!image.save(path, "PNG")) {
		QMessageBox::warning(this, tr("Save Screenshot"),
		    tr("Could not save\n%1").arg(QDir::toNativeSeparators(path)));
	}
}

void MainWindow::setCycleShot(bool on)
{
	if (on && cycle_shot_path[0] && cycle_shot_interval >= 1000) {
		m_cycleShotTimer.start(static_cast<int>(cycle_shot_interval));
	} else {
		m_cycleShotTimer.stop();
	}
	np2oscfg.cycle_shot_enabled = m_cycleShotTimer.isActive() ? 1 : 0;
	sysmng_update(SYS_UPDATEOSCFG);
	updateMenuStatus();
}

/* Replaced in one go, so whatever reads the file never sees half of it */
void MainWindow::cycleShot()
{
	const QImage image = m_screen->frame();
	if (image.isNull()) {
		return;
	}
	QSaveFile file(QFile::decodeName(cycle_shot_path));
	if (!file.open(QIODevice::WriteOnly) || !image.save(&file, "PNG") || !file.commit()) {
		setCycleShot(false);
		/* For a while: the message covers the drives */
		statusBar()->showMessage(tr("Periodic screenshot stopped: could not write %1")
		    .arg(QDir::toNativeSeparators(file.fileName())), 10000);
	}
}

void MainWindow::setTextHook(bool on)
{
	stopEmulation();
	hf_enable = on ? 1 : 0;
	if (on) {
		hook_fontrom_defenable();   /* clears hf_enable if the file fails */
	} else {
		hook_fontrom_defdisable();
	}
	startEmulation();
	if (on && hf_enable != 1) {
		QMessageBox::warning(this, tr("Text ROM Hook"),
		    tr("Could not create %1").arg(QDir::toNativeSeparators(QFile::decodeName(file_getcd(HF_FILENAME)))));
	}
	updateMenuStatus();
}

void MainWindow::about()
{
	extern const OEMCHAR np2version[];
	QMessageBox box(this);
	box.setWindowTitle(tr("About %1").arg(QStringLiteral(NP2_WX_TITLE)));
	box.setIconPixmap(windowIcon().pixmap(64));
	box.setTextFormat(Qt::RichText);
	box.setTextInteractionFlags(Qt::TextBrowserInteraction);
	box.setText(tr("<h3>%1</h3><p>%2 (Qt %3)</p><p>PC-9801 series emulator, the Qt frontend of NP2kai.</p>"
	               "<p>Neko Project II by Yui, ported to UNIX by NONAKA Kimihiro; "
	               "NP2kai by AZO234; NP21/W by SimK.</p>"
	               "<p><a href=\"https://github.com/AZO234/NP2kai\">github.com/AZO234/NP2kai</a></p>")
	            .arg(QStringLiteral(NP2_WX_TITLE), QString::fromUtf8(np2version), QString::fromLatin1(qVersion())));
	stopEmulation();
	box.exec();
	startEmulation();
}

/* ---- window state ---- */

void MainWindow::rememberGeometry()
{
	if (isMaximized() || isFullScreen() || !isVisible()) {
		return;
	}
	np2oscfg.winx      = x();
	np2oscfg.winy      = y();
	np2oscfg.winwidth  = static_cast<UINT>(m_screen->width());
	np2oscfg.winheight = static_cast<UINT>(m_screen->height());
}

void MainWindow::moveEvent(QMoveEvent *event)
{
	rememberGeometry();
	QMainWindow::moveEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
	rememberGeometry();
	QMainWindow::resizeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (m_keyDisp) {
		/* Qt would close it after us and save it as switched off */
		disconnect(m_keyDisp, nullptr, this, nullptr);
		m_keyDisp->close();
	}
	m_screen->pointerCapture()->release();
	m_emuTimer.stop();
	m_cycleShotTimer.stop();
	np2_multithread_stop();
	hook_fontrom_defdisable();
	np2_terminate();   /* saves the configuration */
	event->accept();
}
