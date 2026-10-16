/* === preferences: Misc page ===
 * The Misc tab of wx/dialog/prefframe.cpp. Saving the state, which the core of
 * this port never reads, is gone; the cycle screenshot moves to the screenshot
 * menu.
 */

#include <compiler.h>
#include "miscpage.h"
#include "ui_miscpage.h"
#include "np2.h"

#include <pccore.h>

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>

MiscPage::MiscPage(QWidget *parent)
	: PrefPage(parent)
	, m_ui(new Ui::MiscPage)
{
	m_ui->setupUi(this);
	connect(m_ui->fontBrowse, &QPushButton::clicked, this, &MiscPage::browseFont);
#if !defined(SUPPORT_PCI)
	m_ui->USE_PCI->hide();
#endif
}

MiscPage::~MiscPage() = default;

void MiscPage::browseFont()
{
	const QString current = m_ui->fontfile->text();
	const QString path = QFileDialog::getOpenFileName(this, tr("Font"),
	    current.isEmpty() ? QString() : QFileInfo(current).absolutePath(),
	    tr("Fonts (*.bmp *.BMP font.rom FONT.ROM PC88.FNT SUBSYS_C.ROM FNT0808.X1 CGROM.DAT);;All files (*)"));
	if (!path.isEmpty()) {
		m_ui->fontfile->setText(QDir::toNativeSeparators(path));
	}
}

void MiscPage::load()
{
	m_ui->s_NOWAIT->setChecked(np2oscfg.NOWAIT != 0);
	m_ui->SkpFrame->setCurrentIndex(np2oscfg.DRAW_SKIP < m_ui->SkpFrame->count() ? np2oscfg.DRAW_SKIP : 0);
	m_ui->e_resume->setChecked(np2oscfg.resume != 0);
	m_ui->fontfile->setText(QFile::decodeName(np2cfg.fontfile));
#if defined(SUPPORT_PCI)
	m_ui->USE_PCI->setChecked(np2cfg.usepci != 0);
	m_ui->PCI_PCMC->setCurrentIndex(np2cfg.pci_pcmc < m_ui->PCI_PCMC->count() ? np2cfg.pci_pcmc : 0);
	m_ui->PCI_B32->setChecked(np2cfg.pci_bios32 != 0);
#endif
	m_ui->jast_snd->setChecked(np2oscfg.jastsnd != 0);
}

void MiscPage::save()
{
	np2oscfg.NOWAIT = m_ui->s_NOWAIT->isChecked() ? 1 : 0;
	np2oscfg.DRAW_SKIP = static_cast<UINT8>(m_ui->SkpFrame->currentIndex());
	np2oscfg.resume = m_ui->e_resume->isChecked() ? 1 : 0;
	milstr_ncpy(np2cfg.fontfile, QFile::encodeName(m_ui->fontfile->text().trimmed()).constData(),
	            NELEMENTS(np2cfg.fontfile));
#if defined(SUPPORT_PCI)
	np2cfg.usepci = m_ui->USE_PCI->isChecked() ? 1 : 0;
	np2cfg.pci_pcmc = static_cast<UINT8>(m_ui->PCI_PCMC->currentIndex());
	np2cfg.pci_bios32 = m_ui->PCI_B32->isChecked() ? 1 : 0;
#endif
	np2oscfg.jastsnd = m_ui->jast_snd->isChecked() ? 1 : 0;
}
