/* === mouse capture for the Qt port ===
 * Turns host pointer movement into relative PC-98 mouse movement while the
 * pointer is captured. Three ways of doing it, picked at capture time:
 *   Lock   - Wayland: the compositor holds the pointer in place and reports
 *            relative movement (waylandpointerlock.h).
 *   Warp   - X11: the pointer is moved back to the centre so it never reaches
 *            the edge of the window.
 *   Plain  - movement between consecutive positions, no warping. Used where
 *            neither works (a compositor without pointer constraints, a build
 *            without Wayland support, warping found not to work); the pointer
 *            then stops at the window edge.
 */

#ifndef NP2_QT_POINTERCAPTURE_H
#define NP2_QT_POINTERCAPTURE_H

#include <QObject>
#include <QPointF>

class QWidget;
class WaylandPointerLock;

class PointerCapture : public QObject
{
	Q_OBJECT

public:
	explicit PointerCapture(QWidget *target);

	bool isCaptured() const { return m_captured; }
	void capture();
	void release();
	void toggle();

	/* Feed every mouse move of the target widget, in widget coordinates */
	void onMouseMove(const QPointF &pos);

Q_SIGNALS:
	void capturedChanged(bool captured);

private:
	enum class Mode { Lock, Warp, Plain };

	QWidget *m_target;
	WaylandPointerLock *m_lock;
	Mode     m_mode;
	bool     m_captured;
	bool     m_grabbed;
	bool     m_warpWorks;

	bool     m_lastValid;
	QPointF  m_last;
	bool     m_warpPending;
	int      m_warpWait;
	QPointF  m_remainder;

	QPointF centre() const;
	void warpToCentre();
	void emitDelta(const QPointF &delta);
};

#endif /* NP2_QT_POINTERCAPTURE_H */
