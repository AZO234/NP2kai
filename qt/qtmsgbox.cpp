/* === message boxes for the Qt port ===
 * The core calls msgbox() from the emulator thread (disk errors, ia32 panics),
 * so the box is queued to the GUI thread and shown without blocking anyone.
 */

#include <compiler.h>

#include <QApplication>
#include <QMessageBox>
#include <QPointer>
#include <QString>

#include <atomic>
#include <cstdio>

/* A runaway core can call this in a loop; past this many boxes on screen the
 * messages only go to stderr. */
static const int kMaxOpenBoxes = 4;
static std::atomic<int> s_openBoxes{0};

extern "C" void np2qt_msgbox(const char *title, const char *msg)
{
	const QString t = QString::fromUtf8(title ? title : "");
	const QString m = QString::fromUtf8(msg ? msg : "");
	std::fprintf(stderr, "%s: %s\n", title ? title : "", msg ? msg : "");

	QCoreApplication *app = QCoreApplication::instance();
	if (!app || s_openBoxes.load() >= kMaxOpenBoxes) {
		return;
	}
	s_openBoxes++;
	QMetaObject::invokeMethod(app, [t, m]() {
		auto *box = new QMessageBox(QMessageBox::Information, t, m,
		                            QMessageBox::Ok, QApplication::activeWindow());
		box->setAttribute(Qt::WA_DeleteOnClose);
		QObject::connect(box, &QObject::destroyed, []() { s_openBoxes--; });
		box->open();
	}, Qt::QueuedConnection);
}
