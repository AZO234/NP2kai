/* === cycle screenshot settings for the Qt port === */

#include <compiler.h>
#include "cycleshotdialog.h"
#include "ui_cycleshotdialog.h"
#include "np2.h"

#include <dosio.h>
#include <sysmng.h>

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

/* A bare file name would land in whatever directory the program started in */
static QString shotPath()
{
	const QString path = QFile::decodeName(cycle_shot_path);
	if (path.isEmpty() || QFileInfo(path).isAbsolute()) {
		return path;
	}
	const QString dir = bmpfilefolder[0] ? QFile::decodeName(bmpfilefolder) : QDir::homePath();
	return QDir(dir).absoluteFilePath(path);
}

CycleShotDialog::CycleShotDialog(QWidget *parent)
	: QDialog(parent)
	, m_ui(new Ui::CycleShotDialog)
{
	m_ui->setupUi(this);
	m_ui->CyclePath->setText(QDir::toNativeSeparators(shotPath()));
	m_ui->CycleInt->setValue(static_cast<int>(qMax<UINT32>(cycle_shot_interval, 1000)));
	connect(m_ui->pathBrowse, &QPushButton::clicked, this, &CycleShotDialog::browse);
}

CycleShotDialog::~CycleShotDialog() = default;

void CycleShotDialog::browse()
{
	const QString path = QFileDialog::getSaveFileName(this, windowTitle(), m_ui->CyclePath->text(),
	    tr("PNG images (*.png)"), nullptr, QFileDialog::DontConfirmOverwrite);
	if (!path.isEmpty()) {
		m_ui->CyclePath->setText(QDir::toNativeSeparators(path));
	}
}

void CycleShotDialog::accept()
{
	const QString path = QDir::fromNativeSeparators(m_ui->CyclePath->text().trimmed());
	if (path.isEmpty() || !QFileInfo(path).absoluteDir().exists()) {
		QMessageBox::warning(this, windowTitle(), tr("Choose a file in an existing directory."));
		return;
	}
	milstr_ncpy(cycle_shot_path, QFile::encodeName(QFileInfo(path).absoluteFilePath()).constData(),
	            sizeof(cycle_shot_path));
	cycle_shot_interval = static_cast<UINT32>(m_ui->CycleInt->value());
	sysmng_update(SYS_UPDATEOSCFG);
	QDialog::accept();
}
