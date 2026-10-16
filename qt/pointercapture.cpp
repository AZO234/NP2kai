/* === mouse capture for the Qt port === */

#include <compiler.h>
#include "pointercapture.h"
#include "mousemng.h"
#include "scrnmng.h"
#include "np2.h"
#if defined(NP2_QT_WAYLAND)
#include "waylandpointerlock.h"
#endif

#include <QCursor>
#include <QGuiApplication>
#include <QWidget>

#include <cmath>

/* A warp shows up as a motion event right on the centre. This many events
 * without one and warping is taken not to work on this display. */
static const int kWarpWait = 8;

PointerCapture::PointerCapture(QWidget *target)
	: QObject(target)
	, m_target(target)
	, m_lock(nullptr)
	, m_mode(Mode::Plain)
	, m_captured(false)
	, m_grabbed(false)
	, m_warpWorks(true)
	, m_lastValid(false)
	, m_warpPending(false)
	, m_warpWait(0)
{
#if defined(NP2_QT_WAYLAND)
	if (WaylandPointerLock::isAvailable()) {
		m_lock = new WaylandPointerLock(this);
		connect(m_lock, &WaylandPointerLock::moved, this, &PointerCapture::emitDelta);
		connect(m_lock, &WaylandPointerLock::lost, this, &PointerCapture::release);
	}
#endif
}

QPointF PointerCapture::centre() const
{
	return QPointF(m_target->width() / 2.0, m_target->height() / 2.0);
}

void PointerCapture::capture()
{
	if (m_captured) {
		return;
	}
	/* Only X11 lets a client move the pointer; Wayland refuses by design
	 * and locks it instead. */
	if (QGuiApplication::platformName() == QLatin1String("xcb") && m_warpWorks) {
		m_mode = Mode::Warp;
	} else {
		m_mode = Mode::Plain;
	}

	m_captured = true;
	m_lastValid = false;   /* the first move sets the starting point */
	m_warpPending = false;
	m_remainder = QPointF();

	m_target->setMouseTracking(true);
	m_target->setCursor(Qt::BlankCursor);
	/* Wayland grants grabs to popups only (and complains about the rest);
	 * there the lock keeps the pointer on the window instead. */
	m_grabbed = !QGuiApplication::platformName().startsWith(QLatin1String("wayland"));
	if (m_grabbed) {
		m_target->grabMouse();
	}
#if defined(NP2_QT_WAYLAND)
	if (m_lock && m_lock->lock(m_target->window()->windowHandle())) {
		m_mode = Mode::Lock;
	}
#endif
	if (m_mode == Mode::Warp) {
		warpToCentre();
	}
	mousemng_hidecursor();
	Q_EMIT capturedChanged(true);
}

void PointerCapture::release()
{
	if (!m_captured) {
		return;
	}
	m_captured = false;
#if defined(NP2_QT_WAYLAND)
	if (m_lock) {
		m_lock->unlock();
	}
#endif
	if (m_grabbed) {
		m_target->releaseMouse();
		m_grabbed = false;
	}
	m_target->unsetCursor();
	m_target->setMouseTracking(false);
	mousemng_showcursor();
	Q_EMIT capturedChanged(false);
}

void PointerCapture::toggle()
{
	if (m_captured) {
		release();
	} else {
		capture();
	}
}

void PointerCapture::warpToCentre()
{
	QCursor::setPos(m_target->mapToGlobal(centre().toPoint()));
	m_warpPending = true;
	m_warpWait = kWarpWait;
}

void PointerCapture::onMouseMove(const QPointF &pos)
{
	/* Locked, the movement comes from the relative pointer alone */
	if (!m_captured || m_mode == Mode::Lock) {
		return;
	}
	const QPointF c = centre();

	/* Until the warp is seen, movement is measured from the last real
	 * position, so a warp still in flight or refused costs nothing. */
	if (m_warpPending) {
		if (std::abs(pos.x() - c.x()) < 1.0 && std::abs(pos.y() - c.y()) < 1.0) {
			m_warpPending = false;
			m_last = pos;
			m_lastValid = true;
			return;
		}
		if (--m_warpWait <= 0) {
			m_warpPending = false;
			m_warpWorks = false;
			m_mode = Mode::Plain;
		}
	}

	if (!m_lastValid) {
		m_last = pos;
		m_lastValid = true;
		return;
	}

	emitDelta(pos - m_last);
	m_last = pos;

	/* Back to the centre once outside the middle half of the widget */
	if (m_mode == Mode::Warp && !m_warpPending
	    && (std::abs(pos.x() - c.x()) > m_target->width() / 4.0
	        || std::abs(pos.y() - c.y()) > m_target->height() / 4.0)) {
		warpToCentre();
	}
}

/* Host movement as seen on a rotated screen, turned back into the
 * orientation of the PC-98 screen, so the mouse follows the hand. */
static QPointF unrotate(const QPointF &d)
{
	switch (scrnmode & SCRNMODE_ROTATEMASK) {
	case SCRNMODE_ROTATELEFT:  return QPointF(-d.y(), d.x());
	case SCRNMODE_ROTATEDIR:   return QPointF(-d.x(), -d.y());
	case SCRNMODE_ROTATERIGHT: return QPointF(d.y(), -d.x());
	default:                   return d;
	}
}

/* Widget coordinates are logical; the PC-98 mouse moves in device pixels, as
 * it did in the wx port, so HiDPI scaling doesn't slow it down. The speed set
 * on the Input page slows it from there. Fractions are carried over to the
 * next event, so a slow mouse still moves on small motions. */
void PointerCapture::emitDelta(const QPointF &delta)
{
	if (!m_captured) {
		return;
	}
	const double speed = (np2oscfg.mousemul > 0 && np2oscfg.mousediv > 0)
	                   ? double(np2oscfg.mousemul) / np2oscfg.mousediv : 1.0;
	const QPointF d = unrotate(delta) * (m_target->devicePixelRatioF() * speed) + m_remainder;
	const int dx = static_cast<int>(d.x());
	const int dy = static_cast<int>(d.y());
	m_remainder = QPointF(d.x() - dx, d.y() - dy);
	if (dx || dy) {
		mousemng_onmove(dx, dy);
	}
}
