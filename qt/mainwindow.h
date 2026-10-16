/* === main window for the Qt port === */

#ifndef NP2_QT_MAINWINDOW_H
#define NP2_QT_MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include <compiler.h>

class QAction;
class QActionGroup;
class QMenu;
class QToolButton;
class KeyDispWindow;
class ScreenWidget;
class StatusIndicators;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();
	~MainWindow() override;

	ScreenWidget *screen() const { return m_screen; }

	/* Drive slots: 4 IDE (IA-32) or 2 SASI (i286); an IDE slot holds a hard
	 * disk or a CD-ROM drive, as the running machine has it */
	static int hdSlots();
	static bool isCdSlot(int slot);

	/* Pause and resume around anything modal, as the wx port does */
	void stopEmulation();
	void startEmulation();

public Q_SLOTS:
	void refreshScreen();
	void updateCaption(int flag);
	void updateMenuStatus();
	void toggleFullScreen();
	void toggleMenuBar();

protected:
	void closeEvent(QCloseEvent *event) override;
	void moveEvent(QMoveEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;

private:
	ScreenWidget  *m_screen;
	KeyDispWindow *m_keyDisp;
	QTimer         m_emuTimer;
	QTimer         m_cycleShotTimer;
	bool           m_running;

	QAction *m_actStartPause;
	QAction *m_actSpeed[6];
	QAction *m_actSlot[10];
	QAction *m_actLoadState;
	QAction *m_actFullScreen;
	QAction *m_actPixelPerfect;
	QAction *m_actForce43;
	QAction *m_actStatusBar;
	QAction *m_actRotate[4];
	QAction *m_actKeyDisp;
	QAction *m_actSize[3];
	QAction *m_actCycleShot;
	QAction *m_actTextHook;
	QMenu   *m_fdMenu[4];
	QMenu   *m_hdMenu[4];   /* the slots beyond hdSlots() stay null */
	QMenu   *m_cdMenu[4];   /* likewise */
	QAction *m_actFdEject[4];   /* named after the image in the drive */
	QAction *m_actHdEject[4];
	QAction *m_actCdEject[4];

	/* Status bar: left click mutes or goes back to x1, right click sets */
	QToolButton *m_volButton;
	QToolButton *m_speedButton;
	StatusIndicators *m_indicators;
	UINT8        m_lastVol;
	UINT32       m_lastSpeed;
	int          m_shownVol;     /* what the buttons show, -1 for nothing yet */
	UINT32       m_shownSpeed;

	void buildMenus();
	void buildStatusBar();
	void updateStatusBar();
	void onEmuTimer();
	void rememberGeometry();

	void startPause();
	void reset();
	void resetMachine();
	void openPreferences();
	void openFd(int drv);
	void ejectFd(int drv);
	void openHd(int drv);
	void ejectHd(int drv);
	void openCd(int drv);
	void ejectCd(int drv);
	/* save: write the configuration now; a slider saves once, when it closes */
	void setSpeed(UINT32 speed, bool save = true);
	void setVolume(UINT8 volume, bool save = true);
	void toggleMute();
	void toggleSpeed();
	void popupVolume();
	void popupSpeed();
	void updateLoadState();
	void setStateSlot(int slot);
	void saveState();
	void loadState();

	void sendCtrlAltDel();
	void pasteText();
	void setRotation(int index);
	void setScreenScale(int scale);
	void fitToFrame(const QSize &previous);
	void rescaleWindow(const QSize &before, const QSize &now);
	void rescaleWindow(double scale, const QSize &now);
	void setPixelPerfect(bool on);
	void setForce43(bool on);
	void setStatusBarShown(bool on);
	void showMouseCapture(bool captured);
	void updateSizeMenu();
	void showKeyDisp(bool show);

	void copyScreen();
	void savePng();
	void setCycleShot(bool on);
	void cycleShot();
	void setTextHook(bool on);
	void about();
};

#endif /* NP2_QT_MAINWINDOW_H */
