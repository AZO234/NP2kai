/* === drive, network and keyboard indicators for the Qt port === */

#include <compiler.h>
#include "statusindicators.h"
#include "mainwindow.h"
#include "np2.h"
#include <pccore.h>
#include <keystat.h>
#include <sysmng.h>
#include <dosio.h>
#include <diskimage/fddfile.h>
#include <fdd/sxsi.h>

#include <QApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QIconEngine>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QStatusBar>
#include <QStyle>
#include <QStyleOption>
#include <QToolButton>
#include <QtMath>

/* How long a lamp stays lit after the last access, so a single sector shows */
static const qint64 kLampMs = 120;

StatusIndicators::StatusIndicators(QStatusBar *bar, QWidget *window)
	: QObject(window)
	, m_bar(bar)
	, m_window(window)
	, m_drives(new QWidget(window))
	, m_input(new QWidget(window))
	, m_fdMenu{}
	, m_hdMenu{}
	, m_cdMenu{}
	, m_dpr(0)
	, m_mouseCaptured(false)
{
	auto *drives = new QHBoxLayout(m_drives);
	drives->setContentsMargins(0, 0, 0, 0);
	drives->setSpacing(0);
	for (int drv = 0; drv < 4; drv++) {
		addItem(Floppy, drv, drives);
	}
	for (int drv = 0; drv < MainWindow::hdSlots(); drv++) {
		addItem(HardDisk, drv, drives);
	}
	addItem(Network, 0, drives);

	auto *input = new QHBoxLayout(m_input);
	input->setContentsMargins(0, 0, 0, 0);
	input->setSpacing(0);

	/* The lock keys of the PC-98 keyboard, lit as the PC-98 sets them */
	auto addLabel = [this, input](const QString &text, const QString &tip) {
		auto *label = new QLabel(text, m_input);
		label->setToolTip(tip);
		label->setEnabled(false);
		label->setContentsMargins(4, 0, 4, 0);
		input->addWidget(label);
		return label;
	};
	m_caps = addLabel(QStringLiteral("CAPS"), tr("Caps Lock"));
	m_kana = addLabel(QString::fromUtf8("カナ"), tr("Kana Lock"));
	addItem(Mouse, 0, input);

	m_clock.start();
	connect(&m_timer, &QTimer::timeout, this, &StatusIndicators::tick);
	m_timer.start(50);
	tick();
}

void StatusIndicators::setMenus(QMenu *const fd[4], QMenu *const hd[4], QMenu *const cd[4])
{
	for (int i = 0; i < 4; i++) {
		m_fdMenu[i] = fd[i];
		m_hdMenu[i] = hd[i];
		m_cdMenu[i] = cd[i];
	}
}

void StatusIndicators::setMouseCaptured(bool captured)
{
	m_mouseCaptured = captured;
	tick();
}

void StatusIndicators::addItem(Kind kind, int drive, QHBoxLayout *layout)
{
	auto *button = new QToolButton(layout->parentWidget());
	button->setAutoRaise(true);
	button->setFocusPolicy(Qt::NoFocus);
	button->setContextMenuPolicy(Qt::CustomContextMenu);
	layout->addWidget(button);
	Item item{ button, kind, drive, -1, QString(), -1, -1 };
	const int index = static_cast<int>(m_items.size());
	if (kind == Floppy || kind == HardDisk) {
		connect(item.button, &QToolButton::clicked, this, [this, index]() { popup(index); });
		connect(item.button, &QWidget::customContextMenuRequested, this, [this, index]() { popup(index); });
	}
	m_items.append(item);
}

void StatusIndicators::tick()
{
	SYSMNGACCESS access;
	sysmng_takeaccess(&access);
	const qint64 now = m_clock.elapsed();
	for (Item &item : m_items) {
		UINT32 read = 0;
		UINT32 write = 0;
		switch (item.kind) {
		case Floppy:
			read = access.fddread >> item.drive;
			write = access.fddwrite >> item.drive;
			break;
		case HardDisk:
		case CdRom:
			read = access.hddread >> item.drive;
			write = access.hddwrite >> item.drive;
			break;
		case Network:
			/* Received like read, sent like written */
			read = (access.net & SYS_ACCESS_NETRECV) ? 1 : 0;
			write = (access.net & SYS_ACCESS_NETSEND) ? 1 : 0;
			break;
		case Mouse:
			break;
		}
		if (read & 1) {
			item.lastRead = now;
		}
		if (write & 1) {
			item.lastWrite = now;
		}
	}
	if (!m_bar->isVisible()) {
		return;
	}

	/* Icons drawn for another scaling once the window moves screens */
	const qreal dpr = m_bar->devicePixelRatioF();
	const bool force = (dpr != m_dpr);
	if (force) {
		m_dpr = dpr;
		m_icons.clear();
	}
	for (Item &item : m_items) {
		refresh(item, now, force);
	}
	m_caps->setEnabled(keyctrl.capsref != NKEYREF_NC);
	m_kana->setEnabled(keyctrl.kanaref != NKEYREF_NC);
}

static QString nativePath(const OEMCHAR *path)
{
	return QDir::toNativeSeparators(QString::fromUtf8(path));
}

void StatusIndicators::refresh(Item &item, qint64 now, bool force)
{
	bool visible = true;
	bool loaded = false;
	QString tip;
	switch (item.kind) {
	case Floppy:
		visible = (np2cfg.fddequip & (1 << item.drive)) != 0;
		loaded = fdd_diskready(static_cast<REG8>(item.drive)) != 0;
		tip = loaded ? tr("FD%1: %2").arg(item.drive + 1).arg(nativePath(np2cfg.fddfile[item.drive]))
		             : tr("FD%1: no disk").arg(item.drive + 1);
		break;

	case HardDisk:
	case CdRom: {
		/* One position per slot; its icon follows the device set in it */
		const REG8 drv = static_cast<REG8>(item.drive);
		const SXSIDEV sxsi = sxsi_getptr(drv);
		item.kind = MainWindow::isCdSlot(item.drive) ? CdRom : HardDisk;
		loaded = sxsi && (sxsi->flag & SXSIFLAG_READY) && sxsi->devtype != SXSIDEV_NC;
		if (item.kind == CdRom) {
			tip = loaded ? tr("HD%1 (CD-ROM): %2").arg(item.drive + 1).arg(nativePath(sxsi_getfilename(drv)))
			             : tr("HD%1 (CD-ROM): no disc").arg(item.drive + 1);
		} else if (loaded) {
			tip = tr("HD%1: %2").arg(item.drive + 1).arg(nativePath(sxsi_getfilename(drv)));
		} else {
			tip = tr("HD%1: no disk").arg(item.drive + 1);
		}
		break;
	}

	case Network:
#if defined(SUPPORT_LGY98)
		visible = np2cfg.uselgy98 != 0;
#else
		visible = false;
#endif
		loaded = true;
		tip = tr("Network (LGY-98)");
		break;

	case Mouse: {
		/* F12 is left to the PC-98 when it is set to send one of its keys */
		const bool f12 = (np2oscfg.F12KEY == 0);
		if (!m_mouseCaptured) {
			tip = tr("Click the screen to capture the mouse");
		} else if (f12) {
			tip = tr("Press F12 or MMB to release the mouse");
		} else {
			tip = tr("Press MMB to release the mouse");
		}
		if (item.tip != tip) {
			item.tip = tip;
			item.button->setToolTip(tip);
		}
		/* The key that releases it on the icon, as the speed on its button;
		 * the middle button where F12 goes to the PC-98. Dimmed whole while
		 * the mouse is free. */
		const int key = (m_mouseCaptured ? 1 : 0) | (f12 ? 2 : 0);
		if (force || item.iconKey != key) {
			item.iconKey = key;
			const QString keys = f12 ? QStringLiteral("F12") : QStringLiteral("MMB");
			/* At its full size, the same design of the theme as elsewhere,
			 * and wider: the label is longer than the speed's and starts
			 * from the middle of the mouse as that one does */
			const int side = m_items.first().button->iconSize().height();
			QFont font = QApplication::font();
			font.setBold(true);
			font.setPixelSize(qMax(6, side / 2));
			const qreal label = QFontMetricsF(font).horizontalAdvance(keys) + side / 8.0;
			item.button->setIconSize(QSize(qMax(side, qCeil(side / 2.0 + label)), side));
			item.button->setIcon(badgeIcon(baseIcon(Mouse), keys, !m_mouseCaptured, false));
		}
		return;
	}
	}

	Lamp lamp = loaded ? Loaded : Empty;
	if (loaded && item.lastWrite >= 0 && now - item.lastWrite < kLampMs) {
		lamp = Write;
	} else if (loaded && item.lastRead >= 0 && now - item.lastRead < kLampMs) {
		lamp = Read;
	}

	/* isHidden(): the group itself gives way to status bar messages */
	if (item.button->isHidden() == visible) {
		item.button->setVisible(visible);
	}
	/* The kind is part of the key: a slot may turn from a hard disk into a
	 * CD-ROM drive with the same lamp */
	const int key = item.kind * 4 + lamp;
	if (force || item.iconKey != key) {
		item.iconKey = key;
		item.button->setIcon(icon(item.kind, lamp));
	}
	if (item.tip != tip) {
		item.tip = tip;
		item.button->setToolTip(tip);
	}
}

/* From the desktop's icon theme, with the style's own as the fallback */
QIcon StatusIndicators::baseIcon(Kind kind) const
{
	QStyle *style = m_window->style();
	switch (kind) {
	case Floppy:
		/* Qt names no floppy among its theme icons */
		return QIcon::fromTheme(QStringLiteral("media-floppy"), style->standardIcon(QStyle::SP_DriveFDIcon));
	case HardDisk:
		return QIcon::fromTheme(QIcon::ThemeIcon::DriveHarddisk, style->standardIcon(QStyle::SP_DriveHDIcon));
	case CdRom:
		return QIcon::fromTheme(QIcon::ThemeIcon::MediaOptical, style->standardIcon(QStyle::SP_DriveCDIcon));
	case Network:
		return QIcon::fromTheme(QIcon::ThemeIcon::NetworkWired, style->standardIcon(QStyle::SP_DriveNetIcon));
	case Mouse:
		return QIcon::fromTheme(QIcon::ThemeIcon::InputMouse, style->standardIcon(QStyle::SP_ComputerIcon));
	}
	return QIcon();
}

/* Drawn in device pixels, so the lamp stays sharp on HiDPI screens */
QIcon StatusIndicators::icon(Kind kind, Lamp lamp)
{
	const int key = kind * 4 + lamp;
	const auto cached = m_icons.constFind(key);
	if (cached != m_icons.constEnd()) {
		return cached.value();
	}

	const QSize size = m_items.first().button->iconSize();
	const QIcon base = baseIcon(kind);
	QPixmap pixmap = base.pixmap(size, m_dpr, (lamp == Empty) ? QIcon::Disabled : QIcon::Normal);
	if (lamp == Read || lamp == Write) {
		/* The theme may have no pixmap as large as asked for */
		const QSizeF drawn = pixmap.deviceIndependentSize();
		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing);
		const qreal d = drawn.width() * 0.45;
		const QRectF dot(drawn.width() - d, drawn.height() - d, d, d);
		painter.setPen(QPen(QColor(0, 0, 0, 170), 1.0));
		painter.setBrush((lamp == Read) ? QColor(0x2e, 0xcc, 0x40) : QColor(0xff, 0x41, 0x36));
		painter.drawEllipse(dot.adjusted(0.5, 0.5, -0.5, -0.5));
	}
	const QIcon result(pixmap);
	m_icons.insert(key, result);
	return result;
}

QMenu *StatusIndicators::menuFor(const Item &item) const
{
	switch (item.kind) {
	case Floppy:
		return m_fdMenu[item.drive];
	case HardDisk:
		return m_hdMenu[item.drive];
	case CdRom:
		return m_cdMenu[item.drive];
	default:
		return nullptr;
	}
}

/* Above the button, as the volume and speed popups */
void StatusIndicators::popup(int index)
{
	const Item &item = m_items.at(index);
	QMenu *menu = menuFor(item);
	if (!menu) {
		return;
	}
	menu->popup(item.button->mapToGlobal(QPoint(0, -menu->sizeHint().height())));
}

namespace {

class BadgeIconEngine : public QIconEngine
{
public:
	BadgeIconEngine(const QIcon &base, const QString &text, bool dimmed, bool shrink)
		: m_base(base), m_text(text), m_dimmed(dimmed), m_shrink(shrink) {}

	void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override
	{
		draw(painter, rect, mode, state);
	}

	/* Drawn at the size and scale asked for */
	QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
	{
		return scaledPixmap(size, mode, state, 1.0);
	}

	QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) override
	{
		QPixmap pixmap(size * scale);
		pixmap.setDevicePixelRatio(scale);
		pixmap.fill(Qt::transparent);
		QPainter painter(&pixmap);
		draw(&painter, QRect(QPoint(0, 0), size), mode, state);
		return pixmap;
	}

	QIconEngine *clone() const override
	{
		return new BadgeIconEngine(m_base, m_text, m_dimmed, m_shrink);
	}

private:
	QIcon   m_base;
	QString m_text;
	bool    m_dimmed;
	bool    m_shrink;

	/* Dimmed, the icon looks as the theme's disabled icons do, the drives'
	 * among them, and the label as the style dims a picture */
	void draw(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
	{
		const bool dimmed = m_dimmed || mode == QIcon::Disabled;
		/* Shrunk, the icon is a little smaller, up and left, so the label
		 * leaves it recognisable; a theme may draw it from another size's
		 * design then. In a wider rectangle it keeps to the left. */
		const int side = m_shrink ? rect.height() * 13 / 16 : rect.height();
		m_base.paint(painter, QRect(rect.topLeft(), QSize(side, side)), Qt::AlignCenter,
		             dimmed ? QIcon::Disabled : mode, state);
		if (!dimmed) {
			drawLabel(painter, rect);
			return;
		}
		const qreal scale = painter->device() ? painter->device()->devicePixelRatioF() : 1.0;
		QPixmap label(rect.size() * scale);
		label.setDevicePixelRatio(scale);
		label.fill(Qt::transparent);
		QPainter labelPainter(&label);
		labelPainter.setFont(painter->font());
		drawLabel(&labelPainter, QRect(QPoint(0, 0), rect.size()));
		labelPainter.end();
		QStyleOption option;
		option.palette = QApplication::palette();
		QPixmap grey = QApplication::style()->generatedIconPixmap(QIcon::Disabled, label, &option);
		grey.setDevicePixelRatio(scale);
		painter->drawPixmap(rect.topLeft(), grey);
	}

	void drawLabel(QPainter *painter, const QRect &rect)
	{
		painter->save();
		painter->setRenderHint(QPainter::Antialiasing);
		QFont font = painter->font();
		font.setBold(true);
		font.setPixelSize(qMax(6, rect.height() / 2));
		/* Smaller for a text that would not fit across the icon */
		const qreal room = rect.width() - rect.height() / 8.0;
		const qreal advance = QFontMetricsF(font).horizontalAdvance(m_text);
		if (advance > room) {
			font.setPixelSize(qMax(4, int(font.pixelSize() * room / advance)));
		}
		painter->setFont(font);
		const QFontMetricsF fm(font);
		const qreal h = fm.capHeight() + rect.height() / 8.0;
		const qreal w = qMin<qreal>(rect.width(), fm.horizontalAdvance(m_text) + rect.height() / 8.0);
		const QRectF label(rect.right() + 1 - w, rect.bottom() + 1 - h, w, h);
		painter->setPen(Qt::NoPen);
		painter->setBrush(QColor(0, 0, 0, 190));
		painter->drawRoundedRect(label, h * 0.25, h * 0.25);
		painter->setPen(Qt::white);
		painter->drawText(label, Qt::AlignCenter, m_text);
		painter->restore();
	}
};

} // namespace

QIcon badgeIcon(const QIcon &base, const QString &text, bool dimmed, bool shrink)
{
	return QIcon(new BadgeIconEngine(base, text, dimmed, shrink));
}
