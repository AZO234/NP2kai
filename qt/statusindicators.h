/* === drive, network and keyboard indicators for the Qt port ===
 * The status bar icons of 86Box and the X port's tool window: one per drive,
 * dimmed when empty, with a green lamp while it reads and a red one while it
 * writes; clicking a drive opens its menu.
 */

#ifndef NP2_QT_STATUSINDICATORS_H
#define NP2_QT_STATUSINDICATORS_H

#include <QElapsedTimer>
#include <QHash>
#include <QIcon>
#include <QObject>
#include <QTimer>
#include <QVector>

class QHBoxLayout;
class QLabel;
class QMenu;
class QStatusBar;
class QToolButton;

class StatusIndicators : public QObject
{
	Q_OBJECT

public:
	/* Two groups for the window to place on the bar, as 86Box does: the
	 * drives and the network card, and the lock keys with the mouse */
	StatusIndicators(QStatusBar *bar, QWidget *window);
	QWidget *drivesWidget() const { return m_drives; }
	QWidget *inputWidget() const { return m_input; }

	/* The menus of the menu bar, one per drive: hd for hard disk or empty
	 * slots, cd for CD-ROM slots */
	void setMenus(QMenu *const fd[4], QMenu *const hd[4], QMenu *const cd[4]);
	void setMouseCaptured(bool captured);

private:
	enum Kind { Floppy, HardDisk, CdRom, Network, Mouse };
	enum Lamp { Empty, Loaded, Read, Write };

	struct Item {
		QToolButton *button;
		Kind         kind;
		int          drive;      /* FD or SxSI drive number */
		int          iconKey;    /* kind and lamp shown, -1 for none yet */
		QString      tip;
		qint64       lastRead;   /* ms of m_clock, -1 for never */
		qint64       lastWrite;
	};

	QStatusBar    *m_bar;
	QWidget       *m_window;
	QWidget       *m_drives;
	QWidget       *m_input;
	QVector<Item>  m_items;
	QLabel        *m_caps;
	QLabel        *m_kana;
	QMenu         *m_fdMenu[4];
	QMenu         *m_hdMenu[4];
	QMenu         *m_cdMenu[4];
	QTimer         m_timer;
	QElapsedTimer  m_clock;
	QHash<int, QIcon> m_icons;  /* by kind and lamp, for m_dpr */
	qreal          m_dpr;
	bool           m_mouseCaptured;

	void addItem(Kind kind, int drive, QHBoxLayout *layout);
	void tick();
	void refresh(Item &item, qint64 now, bool force);
	QIcon icon(Kind kind, Lamp lamp);
	QIcon baseIcon(Kind kind) const;
	QMenu *menuFor(const Item &item) const;
	void popup(int index);
};

/* base with a short text on a dark label in its lower right corner, drawn at
 * whatever scale the icon is shown; dimmed, it looks disabled whole; shrunk,
 * the label covers less of it */
QIcon badgeIcon(const QIcon &base, const QString &text, bool dimmed = false, bool shrink = true);

#endif /* NP2_QT_STATUSINDICATORS_H */
