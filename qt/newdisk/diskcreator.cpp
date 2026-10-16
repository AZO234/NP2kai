/* === creating new disk images in the Qt port === */

#include <compiler.h>
#include "diskcreator.h"
#include "newfddialog.h"
#include "newhddialog.h"
#include "np2.h"
#include <pccore.h>
#include <sysmng.h>
#include <dosio.h>
#include <fdd/newdisk.h>

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProgressDialog>
#include <QThread>
#include <QTimer>

#include <functional>

/* The helpers sit in DiskCreator too, so that lupdate files their strings
 * under the context tr() looks them up in */
namespace DiskCreator {
namespace {

QString tr(const char *text)
{
	return QCoreApplication::translate("DiskCreator", text);
}

struct FileType {
	const char *filter;
	const char *ext;
};

QString filterString(const FileType *types, size_t count)
{
	QStringList list;
	for (size_t i = 0; i < count; i++) {
		list << tr(types[i].filter);
	}
	return list.join(QStringLiteral(";;"));
}

/* A name without a known extension gets the one of the chosen filter */
QString withExtension(const QString &path, const QString &selectedFilter,
                      const FileType *types, size_t count, const QStringList &known)
{
	if (known.contains(QFileInfo(path).suffix().toLower())) {
		return path;
	}
	for (size_t i = 0; i < count; i++) {
		if (selectedFilter == tr(types[i].filter)) {
			return path + QLatin1Char('.') + QLatin1String(types[i].ext);
		}
	}
	return path + QLatin1Char('.') + QLatin1String(types[0].ext);
}

QString startPath(const char *folder)
{
	const QString dir = folder[0] ? QString::fromUtf8(folder) : QDir::homePath();
	return QDir(dir).filePath(QStringLiteral("newdisk"));
}

/* Overwriting an image the emulator has open would pull it from under it */
bool isInUse(const QString &path)
{
	const QString target = QFileInfo(path).canonicalFilePath();
	if (target.isEmpty()) {
		return false;   /* doesn't exist yet */
	}
	auto same = [&target](const OEMCHAR *p) {
		return p && p[0] && QFileInfo(QString::fromUtf8(p)).canonicalFilePath() == target;
	};
	/* The arrays differ in size: two SASI drives on i286, four IDE slots on
	 * IA-32, each with a CD image of its own */
	for (const auto &file : np2cfg.fddfile) {
		if (same(file)) {
			return true;
		}
	}
	for (const auto &file : np2cfg.sasihdd) {
		if (same(file)) {
			return true;
		}
	}
#if defined(SUPPORT_IDEIO)
	for (const auto &file : np2cfg.idecd) {
		if (same(file)) {
			return true;
		}
	}
#endif
#if defined(SUPPORT_SCSI)
	for (const auto &file : np2cfg.scsihdd) {
		if (same(file)) {
			return true;
		}
	}
#endif
	return false;
}

bool checkNotInUse(QWidget *parent, const QString &path)
{
	if (isInUse(path)) {
		QMessageBox::warning(parent, tr("New Disk Image"),
		    tr("%1 is in use by the emulator. Eject it first.").arg(QFileInfo(path).fileName()));
		return false;
	}
	return true;
}

/* The newdisk_* functions report nothing; a file that was removed first and
 * exists afterwards is the only sign of success. */
bool prepareTarget(QWidget *parent, const QString &path)
{
	if (QFile::exists(path) && !QFile::remove(path)) {
		QMessageBox::warning(parent, tr("New Disk Image"),
		    tr("%1 cannot be replaced.").arg(QFileInfo(path).fileName()));
		return false;
	}
	return true;
}

void reportResult(QWidget *parent, const QString &path, bool cancelled)
{
	if (!cancelled && !QFile::exists(path)) {
		QMessageBox::warning(parent, tr("New Disk Image"),
		    tr("%1 could not be created.").arg(QFileInfo(path).fileName()));
	}
}

void rememberFolder(char *folder, const QString &path)
{
	milstr_ncpy(folder, QFile::encodeName(QFileInfo(path).absolutePath()).constData(), MAX_PATH);
	sysmng_update(SYS_UPDATECFG);
}

/* Runs job(progress, cancel) on a worker thread behind a progress dialog.
 * The core takes plain ints for both, as in the Windows port, which polls
 * them the same way; aligned int stores are not torn on the targets we
 * build for. Jobs without progress don't look at cancel either, so they get
 * a busy bar and no Cancel button. Returns true when cancelled. */
bool runWithProgress(QWidget *parent, const QString &label, bool hasProgress,
                     const std::function<void(int *, int *)> &job)
{
	int progress = 0;
	int cancel = 0;

	QProgressDialog dialog(label, tr("Cancel"), 0, hasProgress ? 100 : 0, parent);
	dialog.setWindowTitle(tr("New Disk Image"));
	dialog.setWindowModality(Qt::WindowModal);
	dialog.setMinimumDuration(0);
	dialog.setAutoClose(false);
	dialog.setAutoReset(false);
	if (!hasProgress) {
		dialog.setCancelButton(nullptr);
	}

	QThread *worker = QThread::create([&]() { job(&progress, &cancel); });
	QEventLoop loop;
	QObject::connect(worker, &QThread::finished, &loop, &QEventLoop::quit);
	/* Cancel hides the dialog; it stays up until the core has noticed the
	 * flag and deleted the partial file. A job that cannot stop (Escape still
	 * cancels the dialog) just runs on. */
	QObject::connect(&dialog, &QProgressDialog::canceled, &dialog, [&]() {
		if (!hasProgress) {
			dialog.show();
			return;
		}
		cancel = 1;
		dialog.setLabelText(tr("Cancelling..."));
		dialog.setCancelButton(nullptr);
		dialog.show();
	});

	QTimer poll;
	QObject::connect(&poll, &QTimer::timeout, &dialog, [&]() {
		if (hasProgress) {
			dialog.setValue(progress);
		}
	});

	worker->start();
	dialog.show();
	poll.start(100);
	loop.exec();
	poll.stop();
	worker->wait();
	delete worker;
	return cancel != 0;
}

} // namespace

void newFdImage(QWidget *parent)
{
	static const FileType kTypes[] = {
		{ QT_TRANSLATE_NOOP("DiskCreator", "D88 images (*.d88 *.d98 *.88d *.98d)"), "d88" },
		{ QT_TRANSLATE_NOOP("DiskCreator", "1.25 MB raw images (*.hdm)"), "hdm" },
		{ QT_TRANSLATE_NOOP("DiskCreator", "1.44 MB raw images (*.hd4)"), "hd4" },
	};
	static const QStringList kKnown = {
		QStringLiteral("d88"), QStringLiteral("d98"), QStringLiteral("88d"), QStringLiteral("98d"),
		QStringLiteral("hdm"), QStringLiteral("hd4"),
	};

	QString selected;
	QString path = QFileDialog::getSaveFileName(parent, tr("New Floppy Disk Image"),
	    startPath(fddfolder), filterString(kTypes, NELEMENTS(kTypes)), &selected);
	if (path.isEmpty()) {
		return;
	}
	path = withExtension(path, selected, kTypes, NELEMENTS(kTypes), kKnown);
	if (!checkNotInUse(parent, path)) {
		return;
	}
	const QString ext = QFileInfo(path).suffix().toLower();
	const QByteArray name = QFile::encodeName(path);

	if (ext == QLatin1String("hdm") || ext == QLatin1String("hd4")) {
		if (!prepareTarget(parent, path)) {
			return;
		}
		if (ext == QLatin1String("hdm")) {
			newdisk_123mb_fdd(name.constData());
		} else {
			newdisk_144mb_fdd(name.constData());
		}
	} else {
		NewFdDialog dialog(path, parent);
		if (dialog.exec() != QDialog::Accepted || !prepareTarget(parent, path)) {
			return;
		}
		newdisk_fdd(name.constData(), dialog.diskType(), dialog.diskLabel().constData());
	}
	reportResult(parent, path, false);
	rememberFolder(fddfolder, path);
}

void newHdImage(QWidget *parent)
{
	using Format = NewHdDialog::Format;
	static const FileType kTypes[] = {
		{ QT_TRANSLATE_NOOP("DiskCreator", "T98-Next images (*.nhd)"), "nhd" },
		{ QT_TRANSLATE_NOOP("DiskCreator", "Anex86 images (*.hdi)"), "hdi" },
		{ QT_TRANSLATE_NOOP("DiskCreator", "Virtual PC images (*.vhd)"), "vhd" },
		{ QT_TRANSLATE_NOOP("DiskCreator", "T98 images (*.thd)"), "thd" },
#if defined(SUPPORT_SCSI)
		{ QT_TRANSLATE_NOOP("DiskCreator", "Virtual98 SCSI images (*.hdd)"), "hdd" },
		{ QT_TRANSLATE_NOOP("DiskCreator", "RaSCSI images (*.hdn)"), "hdn" },
#endif
	};
	static const Format kFormats[] = {
		Format::Nhd, Format::Hdi, Format::Vhd, Format::Thd,
#if defined(SUPPORT_SCSI)
		Format::Hdd, Format::Hdn,
#endif
	};
	QStringList known;
	for (const FileType &t : kTypes) {
		known << QLatin1String(t.ext);
	}

	QString selected;
	QString path = QFileDialog::getSaveFileName(parent, tr("New Hard Disk Image"),
	    startPath(hddfolder), filterString(kTypes, NELEMENTS(kTypes)), &selected);
	if (path.isEmpty()) {
		return;
	}
	path = withExtension(path, selected, kTypes, NELEMENTS(kTypes), known);
	if (!checkNotInUse(parent, path)) {
		return;
	}
	const Format format = kFormats[known.indexOf(QFileInfo(path).suffix().toLower())];

	NewHdDialog dialog(format, path, parent);
	if (dialog.exec() != QDialog::Accepted || !prepareTarget(parent, path)) {
		return;
	}

	const QByteArray name = QFile::encodeName(path);
	const quint32 size = dialog.sizeMB();
	const quint32 c  = dialog.cylinders();
	const quint16 h  = dialog.heads();
	const quint16 s  = dialog.sectors();
	const quint16 ss = dialog.sectorSize();
	const int blank   = dialog.isBlank() ? 1 : 0;
	const int dynamic = dialog.isDynamic() ? 1 : 0;

	/* Only these three report progress and honour cancel; the others are a
	 * header (THD, HDD) or plain zeros (HDN) */
	const bool hasProgress = NewHdDialog::hasGeometry(format) && !dynamic;
	const bool cancelled = runWithProgress(parent,
	    tr("Creating %1...").arg(QFileInfo(path).fileName()), hasProgress,
	    [=](int *progress, int *cancel) {
		const char *fname = name.constData();
		switch (format) {
		case Format::Nhd: newdisk_nhd_ex_CHS(fname, c, h, s, ss, blank, progress, cancel); break;
		case Format::Hdi: newdisk_hdi_ex_CHS(fname, c, h, s, ss, blank, progress, cancel); break;
#if defined(SUPPORT_VPCVHD)
		case Format::Vhd: newdisk_vpcvhd_ex_CHS(fname, c, h, s, ss, dynamic, blank, progress, cancel); break;
#else
		case Format::Vhd: break;
#endif
		case Format::Thd: newdisk_thd(fname, size); break;
		case Format::Hdd: newdisk_vhd(fname, size); break;
		case Format::Hdn: newdisk_hdn(fname, size); break;
		}
	});
	reportResult(parent, path, cancelled);
	rememberFolder(hddfolder, path);
}

} // namespace DiskCreator
