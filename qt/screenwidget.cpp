/* === emulated screen for the Qt port === */

#include <compiler.h>
#include "screenwidget.h"
#include "pointercapture.h"
#include "kbtrans.h"
#include "kbdmng.h"
#include "mousemng.h"
#include "scrnmng.h"
#include "np2.h"
#include <keystat.h>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTransform>

#include <cstring>

ScreenWidget::ScreenWidget(QWidget *parent)
	: QWidget(parent)
	, m_capture(new PointerCapture(this))
	, m_captureClick(false)
{
	setFocusPolicy(Qt::StrongFocus);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAttribute(Qt::WA_InputMethodEnabled, false);
	setMinimumSize(320, 200);
	connect(m_capture, &PointerCapture::capturedChanged, this, &ScreenWidget::mouseCaptureChanged);
}

QSize ScreenWidget::sizeHint() const
{
	return QSize(640, 400);
}

void ScreenWidget::updateFrame()
{
	int w, h, bpp;
	const QSize previous = m_image.size();
	scrnmng_lockbuf();
	const UINT8 *src = scrnmng_getpixbuf(&w, &h, &bpp);
	if (src && w > 0 && h > 0 && (bpp == 16 || bpp == 32)) {
		/* scrnmng keeps BGRA (bpp 32) or RGB565 (bpp 16) rows without
		 * padding; both are QImage formats as they are. */
		const QImage::Format fmt = (bpp == 32) ? QImage::Format_RGB32 : QImage::Format_RGB16;
		if (m_image.width() != w || m_image.height() != h || m_image.format() != fmt) {
			m_image = QImage(w, h, fmt);
		}
		const size_t row = static_cast<size_t>(w) * (bpp / 8);
		for (int y = 0; y < h; y++) {
			std::memcpy(m_image.scanLine(y), src + row * y, row);
		}
	}
	scrnmng_unlockbuf();
	update();
	if (m_image.size() != previous) {
		Q_EMIT frameSizeChanged(previous);
	}
}

static int rotationDegrees()
{
	switch (scrnmode & SCRNMODE_ROTATEMASK) {
	case SCRNMODE_ROTATELEFT:  return 270;
	case SCRNMODE_ROTATEDIR:   return 180;
	case SCRNMODE_ROTATERIGHT: return 90;
	default:                   return 0;
	}
}

QImage ScreenWidget::frame() const
{
	const int deg = rotationDegrees();
	if (m_image.isNull() || deg == 0) {
		return m_image;
	}
	return m_image.transformed(QTransform().rotate(deg));
}

QSize ScreenWidget::displaySizeFor(const QSize &frame)
{
	if (frame.isEmpty()) {
		return frame;
	}
	QSize s = (scrnmode & SCRNMODE_ROTATE) ? frame.transposed() : frame;
	if (np2oscfg.force43) {
		if (s.width() >= s.height()) {
			s.setHeight(qRound(s.width() * 3 / 4.0));
		} else {
			s.setWidth(qRound(s.height() * 3 / 4.0));
		}
	}
	return s;
}

/* Drawn in device pixels, so that pixel-perfect means it on HiDPI screens too.
 * Fitted with the aspect ratio kept, or at the largest whole multiple when
 * pixel-perfect; the rest stays black. */
void ScreenWidget::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	const qreal dpr = devicePixelRatioF();
	p.scale(1.0 / dpr, 1.0 / dpr);
	const QSizeF area = QSizeF(size()) * dpr;
	p.fillRect(QRectF(QPointF(0, 0), area), Qt::black);
	if (m_image.isNull()) {
		return;
	}

	const QSize base = displaySize();
	QSizeF fit;
	const int whole = static_cast<int>(qMin(area.width() / base.width(), area.height() / base.height()));
	if (np2oscfg.pixelperfect && whole >= 1) {
		fit = QSizeF(base) * whole;
	} else {
		fit = QSizeF(base).scaled(area, Qt::KeepAspectRatio);
	}
	const QRectF target(qRound((area.width() - fit.width()) / 2.0), qRound((area.height() - fit.height()) / 2.0),
	                    qRound(fit.width()), qRound(fit.height()));

	const int deg = rotationDegrees();
	const bool portrait = (deg == 90 || deg == 270);
	/* The target in the frame's own axes */
	const QSizeF drawn = portrait ? target.size().transposed() : target.size();

	QImage src = m_image;
	bool smooth = (np2oscfg.drawinterp == INTERP_BILINEAR || np2oscfg.drawinterp == INTERP_HYPER)
	              && !np2oscfg.pixelperfect;
	if (!smooth && np2oscfg.force43) {
		/* 4:3 needs a scale no whole multiple gives. Sharp bilinear: the
		 * whole part with square pixels, only the rest smoothed, instead of
		 * every fifth row drawn taller. */
		const int kx = qMax(1, static_cast<int>(drawn.width() / m_image.width()));
		const int ky = qMax(1, static_cast<int>(drawn.height() / m_image.height()));
		if (drawn != QSizeF(m_image.width() * kx, m_image.height() * ky)) {
			if (kx > 1 || ky > 1) {
				src = m_image.scaled(m_image.width() * kx, m_image.height() * ky,
				                     Qt::IgnoreAspectRatio, Qt::FastTransformation);
			}
			smooth = true;
		}
	}
	p.setRenderHint(QPainter::SmoothPixmapTransform, smooth);

	if (deg == 0) {
		p.drawImage(target, src);
		return;
	}
	p.translate(target.center());
	p.rotate(deg);
	p.drawImage(QRectF(QPointF(-drawn.width() / 2.0, -drawn.height() / 2.0), drawn), src);
}

/* Keys the frontend keeps for itself. Returns true when the key was used. */
bool ScreenWidget::handleHotkey(QKeyEvent *event, bool pressed)
{
	const int key = event->key();

	if (pressed && (event->modifiers() & Qt::AltModifier)
	    && (key == Qt::Key_Return || key == Qt::Key_Enter)) {
		Q_EMIT fullScreenToggleRequested();
		return true;
	}

	if (key == Qt::Key_F11) {
		/* F11KEY: 0 nothing, 1 menu bar in full screen, 2 full screen */
		if (pressed && !event->isAutoRepeat()) {
			if (np2oscfg.F11KEY == 1) {
				Q_EMIT menuBarToggleRequested();
			} else if (np2oscfg.F11KEY == 2) {
				Q_EMIT fullScreenToggleRequested();
			}
		}
		return true;
	}

	if (key == Qt::Key_F12) {
		/* F12KEY: 0 toggles mouse capture, otherwise a PC-98 key */
		if (event->isAutoRepeat()) {
			return true;
		}
		if (np2oscfg.F12KEY == 0) {
			if (pressed) {
				m_capture->toggle();
			}
		} else {
			const UINT8 kc = kbdmng_getf12key();
			if (kc != KEYBOARD_KC_NC) {
				if (pressed) {
					keystat_keydown(kc);
				} else {
					keystat_keyup(kc);
				}
			}
		}
		return true;
	}
	return false;
}

void ScreenWidget::keyPressEvent(QKeyEvent *event)
{
	if (!handleHotkey(event, true)) {
		wxkbd_keydown(0, static_cast<int>(event->nativeScanCode()));
	}
	event->accept();
}

void ScreenWidget::keyReleaseEvent(QKeyEvent *event)
{
	if (event->isAutoRepeat()) {
		event->accept();
		return;
	}
	if (!handleHotkey(event, false)) {
		wxkbd_keyup(0, static_cast<int>(event->nativeScanCode()));
	}
	event->accept();
}

/* The menu shortcuts work while the mouse is free; once it is captured, every
 * key goes to the PC-98. Full screen, F11 and F12 are handled in
 * handleHotkey(), so they work either way. */
bool ScreenWidget::event(QEvent *event)
{
	if (event->type() == QEvent::ShortcutOverride && m_capture->isCaptured()) {
		event->accept();
		return true;
	}
	return QWidget::event(event);
}

/* Tab belongs to the emulated machine, not to focus navigation */
bool ScreenWidget::focusNextPrevChild(bool)
{
	return false;
}

void ScreenWidget::mousePressEvent(QMouseEvent *event)
{
	switch (event->button()) {
	case Qt::LeftButton:
		/* The click that captures the mouse stays away from the PC-98, unlike
		 * in the wx port: a game would take it for a click on whatever its
		 * cursor rests on. Its release goes nowhere either. */
		if (!m_capture->isCaptured()) {
			m_capture->capture();
			m_captureClick = true;
		} else {
			mousemng_onleft(TRUE);
		}
		break;
	case Qt::RightButton:
		if (m_capture->isCaptured()) {
			mousemng_onright(TRUE);
		}
		break;
	case Qt::MiddleButton:
		m_capture->toggle();
		break;
	default:
		break;
	}
	event->accept();
}

void ScreenWidget::mouseReleaseEvent(QMouseEvent *event)
{
	switch (event->button()) {
	case Qt::LeftButton:
		if (m_captureClick) {
			m_captureClick = false;
		} else {
			mousemng_onleft(FALSE);
		}
		break;
	case Qt::RightButton:
		mousemng_onright(FALSE);
		break;
	default:
		break;
	}
	event->accept();
}

void ScreenWidget::mouseMoveEvent(QMouseEvent *event)
{
	m_capture->onMouseMove(event->position());
	event->accept();
}

void ScreenWidget::focusOutEvent(QFocusEvent *event)
{
	/* Nothing may stay pressed or grabbed behind another window */
	wxkbd_reset();
	m_capture->release();
	m_captureClick = false;
	QWidget::focusOutEvent(event);
}
