/* === preferences: DIP Switches page === */

#include <compiler.h>
#include "dipswpage.h"
#include "ui_dipswpage.h"

#include <pccore.h>

DipswPage::DipswPage(QWidget *parent)
	: PrefPage(parent)
	, m_ui(new Ui::DipswPage)
{
	m_ui->setupUi(this);
}

DipswPage::~DipswPage() = default;

void DipswPage::load()
{
	m_ui->DIPswtch->setValues(np2cfg.dipsw);
}

void DipswPage::save()
{
	m_ui->DIPswtch->values(np2cfg.dipsw);
}
