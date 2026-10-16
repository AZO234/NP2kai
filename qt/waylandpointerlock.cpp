/* === pointer locking under Wayland for the Qt port === */

#include <compiler.h>
#include "waylandpointerlock.h"
#include "np2.h"

#include <QGuiApplication>
#include <QWindow>
#include <QtGui/qpa/qplatformwindow_p.h>

#include <wayland-client.h>
#include "pointer-constraints-unstable-v1-client-protocol.h"
#include "relative-pointer-unstable-v1-client-protocol.h"

#include <cstring>

namespace {

/* Bound once per process on first use and kept until exit */
struct Globals {
	bool probed = false;
	zwp_pointer_constraints_v1       *constraints = nullptr;
	zwp_relative_pointer_manager_v1  *relative = nullptr;
};
Globals s_globals;

QNativeInterface::QWaylandApplication *waylandApp()
{
	return qGuiApp ? qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>() : nullptr;
}

void onGlobal(void *, wl_registry *registry, uint32_t name, const char *interface, uint32_t)
{
	if (std::strcmp(interface, zwp_pointer_constraints_v1_interface.name) == 0) {
		s_globals.constraints = static_cast<zwp_pointer_constraints_v1 *>(
		    wl_registry_bind(registry, name, &zwp_pointer_constraints_v1_interface, 1));
	} else if (std::strcmp(interface, zwp_relative_pointer_manager_v1_interface.name) == 0) {
		s_globals.relative = static_cast<zwp_relative_pointer_manager_v1 *>(
		    wl_registry_bind(registry, name, &zwp_relative_pointer_manager_v1_interface, 1));
	}
}

void onGlobalRemove(void *, wl_registry *, uint32_t)
{
}

const wl_registry_listener kRegistryListener = { onGlobal, onGlobalRemove };

/* Qt's event thread reads the same connection, so the registry gets a queue
 * of its own for the roundtrip. The managers then move to the default queue,
 * which Qt dispatches on the GUI thread; everything created from them later
 * (locks, relative pointers) lands there too. */
void probe()
{
	if (s_globals.probed) {
		return;
	}
	s_globals.probed = true;

	QNativeInterface::QWaylandApplication *app = waylandApp();
	wl_display *display = app ? app->display() : nullptr;
	if (!display) {
		return;
	}
	wl_event_queue *queue = wl_display_create_queue(display);
	auto *wrapped = static_cast<wl_display *>(wl_proxy_create_wrapper(display));
	wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(wrapped), queue);
	wl_registry *registry = wl_display_get_registry(wrapped);
	wl_proxy_wrapper_destroy(wrapped);

	wl_registry_add_listener(registry, &kRegistryListener, nullptr);
	wl_display_roundtrip_queue(display, queue);

	if (s_globals.constraints) {
		wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(s_globals.constraints), nullptr);
	}
	if (s_globals.relative) {
		wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(s_globals.relative), nullptr);
	}
	wl_registry_destroy(registry);
	wl_event_queue_destroy(queue);
}

} // namespace

WaylandPointerLock::WaylandPointerLock(QObject *parent)
	: QObject(parent)
	, m_locked(nullptr)
	, m_relative(nullptr)
	, m_generation(0)
{
}

WaylandPointerLock::~WaylandPointerLock()
{
	unlock();
}

bool WaylandPointerLock::isAvailable()
{
	if (!waylandApp()) {
		return false;
	}
	probe();
	return s_globals.constraints && s_globals.relative;
}

bool WaylandPointerLock::lock(QWindow *window)
{
	static const zwp_locked_pointer_v1_listener lockedListener = {
		&WaylandPointerLock::onLocked,
		&WaylandPointerLock::onUnlocked,
	};
	static const zwp_relative_pointer_v1_listener relativeListener = {
		&WaylandPointerLock::onRelativeMotion,
	};

	if (m_locked) {
		return true;
	}
	if (!window || !isAvailable()) {
		return false;
	}
	auto *native = window->nativeInterface<QNativeInterface::Private::QWaylandWindow>();
	wl_surface *surface = native ? native->surface() : nullptr;
	wl_pointer *pointer = waylandApp()->pointer();
	if (!surface || !pointer) {
		return false;
	}

	/* One-shot: once the compositor takes the lock away (another window got
	 * focus), it is gone for good and the capture ends with it. No region, so
	 * the whole surface counts; the click that captures is on it anyway. */
	m_generation++;
	m_locked = zwp_pointer_constraints_v1_lock_pointer(s_globals.constraints, surface, pointer,
	    nullptr, ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT);
	zwp_locked_pointer_v1_add_listener(m_locked, &lockedListener, this);
	m_relative = zwp_relative_pointer_manager_v1_get_relative_pointer(s_globals.relative, pointer);
	zwp_relative_pointer_v1_add_listener(m_relative, &relativeListener, this);

	/* Qt may recreate the surface (e.g. on hide/show); a lock on a dead
	 * surface would never come back. */
	m_surfaceGone = connect(native, &QNativeInterface::Private::QWaylandWindow::surfaceDestroyed,
	                        this, [this]() {
		if (m_locked) {
			unlock();
			Q_EMIT lost();
		}
	});
	return true;
}

void WaylandPointerLock::unlock()
{
	disconnect(m_surfaceGone);
	if (m_relative) {
		zwp_relative_pointer_v1_destroy(m_relative);
		m_relative = nullptr;
	}
	if (m_locked) {
		zwp_locked_pointer_v1_destroy(m_locked);
		m_locked = nullptr;
	}
}

void WaylandPointerLock::onLocked(void *, zwp_locked_pointer_v1 *)
{
}

void WaylandPointerLock::onUnlocked(void *data, zwp_locked_pointer_v1 *locked)
{
	auto *self = static_cast<WaylandPointerLock *>(data);
	if (self->m_locked != locked) {
		return;
	}
	/* Not from inside the handler: lost() ends up destroying this proxy. By
	 * then the lock may have been replaced by a new one, possibly at the same
	 * address, hence the generation. */
	const unsigned generation = self->m_generation;
	QMetaObject::invokeMethod(self, [self, generation]() {
		if (self->m_locked && self->m_generation == generation) {
			self->unlock();
			Q_EMIT self->lost();
		}
	}, Qt::QueuedConnection);
}

void WaylandPointerLock::onRelativeMotion(void *data, zwp_relative_pointer_v1 *,
                                          uint32_t, uint32_t,
                                          int32_t dx, int32_t dy,
                                          int32_t dxUnaccel, int32_t dyUnaccel)
{
	/* Unaccelerated by default: the guest OS, Windows 95 for one, has its
	 * own acceleration, and the two would add up. The accelerated values
	 * feel like the desktop's pointer, as under X11. */
	auto *self = static_cast<WaylandPointerLock *>(data);
	if (np2oscfg.rawmouse) {
		Q_EMIT self->moved(QPointF(wl_fixed_to_double(dxUnaccel), wl_fixed_to_double(dyUnaccel)));
	} else {
		Q_EMIT self->moved(QPointF(wl_fixed_to_double(dx), wl_fixed_to_double(dy)));
	}
}
