/* === preferences: HDD page ===
 * As the Windows port's IDE dialog (windows/dialog/d_ide.cpp). The wx tab's
 * "Equipped" boxes repeated the device type and are gone.
 */

#include <compiler.h>
#include "hddpage.h"
#include "ui_hddpage.h"

#include <pccore.h>
#include <fdd/sxsi.h>

#if defined(SUPPORT_IDEIO)
static const UINT8 kIdeTypes[] = { SXSIDEV_NC, SXSIDEV_HDD, SXSIDEV_CDROM };
#endif

HddPage::HddPage(QWidget *parent)
	: PrefPage(parent)
	, m_ui(new Ui::HddPage)
{
	m_ui->setupUi(this);
	connect(m_ui->IDE_BIOS, &QCheckBox::toggled, m_ui->AIDEBIOS, &QWidget::setEnabled);
}

HddPage::~HddPage() = default;

void HddPage::load()
{
#if defined(SUPPORT_IDEIO)
	QComboBox *const types[4] = { m_ui->IDE1TYPE, m_ui->IDE2TYPE, m_ui->IDE3TYPE, m_ui->IDE4TYPE };
	for (int i = 0; i < 4; i++) {
		setComboValue(types[i], kIdeTypes, np2cfg.idetype[i]);
	}
	m_ui->IDERWAIT->setValue(static_cast<int>(np2cfg.iderwait));
	m_ui->IDEWWAIT->setValue(static_cast<int>(np2cfg.idewwait));
	m_ui->CD_ASYNC->setChecked(np2cfg.useasynccd != 0);
	m_ui->USECDECC->setChecked(np2cfg.usecdecc != 0);
	m_ui->CDTRAYOP->setChecked(np2cfg.allowcdtraycmd != 0);
	m_ui->IDE_BIOS->setChecked(np2cfg.idebios != 0);
	m_ui->AIDEBIOS->setChecked(np2cfg.autoidebios != 0);
	m_ui->AIDEBIOS->setEnabled(np2cfg.idebios != 0);
#endif
}

void HddPage::save()
{
#if defined(SUPPORT_IDEIO)
	QComboBox *const types[4] = { m_ui->IDE1TYPE, m_ui->IDE2TYPE, m_ui->IDE3TYPE, m_ui->IDE4TYPE };
	for (int i = 0; i < 4; i++) {
		np2cfg.idetype[i] = comboValue(types[i], kIdeTypes);
	}
	np2cfg.iderwait = static_cast<UINT32>(m_ui->IDERWAIT->value());
	np2cfg.idewwait = static_cast<UINT32>(m_ui->IDEWWAIT->value());
	np2cfg.useasynccd = m_ui->CD_ASYNC->isChecked() ? 1 : 0;
	np2cfg.usecdecc = m_ui->USECDECC->isChecked() ? 1 : 0;
	np2cfg.allowcdtraycmd = m_ui->CDTRAYOP->isChecked() ? 1 : 0;
	np2cfg.idebios = m_ui->IDE_BIOS->isChecked() ? 1 : 0;
	np2cfg.autoidebios = m_ui->AIDEBIOS->isChecked() ? 1 : 0;
#endif
}
