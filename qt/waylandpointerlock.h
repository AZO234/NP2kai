/* === pointer locking under Wayland for the Qt port ===
 * Wayland doesn't let a client move the pointer, so capturing the mouse the X11
 * way (warping back to the centre) can't work there. Instead the pointer is
 * locked in place with zwp_pointer_constraints_v1 and the movement is read from
 * zwp_relative_pointer_v1, which keeps reporting while the pointer stands still.
 *
 * Qt doesn't wrap either protocol, so they are used through libwayland on Qt's
 * own connection: wl_display and wl_pointer from the public
 * QNativeInterface::QWaylandApplication, wl_surface from the private
 * QNativeInterface::Private::QWaylandWindow.
 */

#ifndef NP2_QT_WAYLANDPOINTERLOCK_H
#define NP2_QT_WAYLANDPOINTERLOCK_H

#include <QObject>
#include <QPointF>

#include <cstdint>

class QWindow;
struct zwp_locked_pointer_v1;
struct zwp_relative_pointer_v1;

class WaylandPointerLock : public QObject
{
	Q_OBJECT

public:
	explicit WaylandPointerLock(QObject *parent = nullptr);
	~WaylandPointerLock() override;

	/* True when running on Wayland and the compositor offers both protocols */
	static bool isAvailable();

	/* Lock the pointer on the window's surface. The lock takes effect once the
	 * pointer is over the surface; movement is reported from now on. */
	bool lock(QWindow *window);
	void unlock();
	bool isLocked() const { return m_locked != nullptr; }

Q_SIGNALS:
	/* Relative movement in surface (logical) coordinates */
	void moved(const QPointF &delta);
	/* The compositor ended the lock (focus moved elsewhere); not sent after
	 * unlock() */
	void lost();

private:
	zwp_locked_pointer_v1   *m_locked;
	zwp_relative_pointer_v1 *m_relative;
	unsigned                 m_generation;   /* bumped by every lock() */
	QMetaObject::Connection  m_surfaceGone;

	static void onLocked(void *data, zwp_locked_pointer_v1 *locked);
	static void onUnlocked(void *data, zwp_locked_pointer_v1 *locked);
	static void onRelativeMotion(void *data, zwp_relative_pointer_v1 *relative,
	                             uint32_t utimeHi, uint32_t utimeLo,
	                             int32_t dx, int32_t dy,
	                             int32_t dxUnaccel, int32_t dyUnaccel);
};

#endif /* NP2_QT_WAYLANDPOINTERLOCK_H */
