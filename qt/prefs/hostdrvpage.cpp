/* === preferences: Host Drive page ===
 * As the Windows port's Hostdrv dialog (windows/dialog/d_hostdrv.cpp): reading
 * is always allowed, writing and deleting are chosen.
 */

#include <compiler.h>
#include "hostdrvpage.h"
#include "ui_hostdrvpage.h"

#include <pccore.h>
#include <common/milstr.h>
#include <generic/hostdrv.h>

#include <QFile>
#include <QFileDialog>

HostdrvPage::HostdrvPage(QWidget *parent)
	: PrefPage(parent)
	, m_ui(new Ui::HostdrvPage)
{
	m_ui->setupUi(this);
	connect(m_ui->rootBrowse, &QPushButton::clicked, this, [this]() {
		const QString dir = QFileDialog::getExistingDirectory(this, tr("Shared Folder"), m_ui->hdrvroot->text());
		if (!dir.isEmpty()) {
			m_ui->hdrvroot->setText(dir);
		}
	});
}

HostdrvPage::~HostdrvPage() = default;

void HostdrvPage::load()
{
#if defined(SUPPORT_HOSTDRV)
	m_ui->use_hdrv->setChecked(np2cfg.hdrvenable != 0);
	m_ui->hdrvroot->setText(QFile::decodeName(np2cfg.hdrvroot));
	m_ui->hdrv_acc_w->setChecked(np2cfg.hdrvacc & HDFMODE_WRITE);
	m_ui->hdrv_acc_d->setChecked(np2cfg.hdrvacc & HDFMODE_DELETE);
#endif
}

void HostdrvPage::save()
{
#if defined(SUPPORT_HOSTDRV)
	np2cfg.hdrvenable = m_ui->use_hdrv->isChecked() ? 1 : 0;
	milstr_ncpy(np2cfg.hdrvroot, QFile::encodeName(m_ui->hdrvroot->text().trimmed()).constData(),
	            NELEMENTS(np2cfg.hdrvroot));
	np2cfg.hdrvacc = static_cast<UINT8>(HDFMODE_READ | (m_ui->hdrv_acc_w->isChecked() ? HDFMODE_WRITE : 0)
	                                    | (m_ui->hdrv_acc_d->isChecked() ? HDFMODE_DELETE : 0));
#endif
}
