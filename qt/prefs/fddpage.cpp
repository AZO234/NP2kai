/* === preferences: FDD page === */

#include <compiler.h>
#include "fddpage.h"
#include "ui_fddpage.h"

#include <pccore.h>

FddPage::FddPage(QWidget *parent)
	: PrefPage(parent)
	, m_ui(new Ui::FddPage)
{
	m_ui->setupUi(this);
}

FddPage::~FddPage() = default;

void FddPage::load()
{
	QCheckBox *const drives[4] = { m_ui->FDDRIVE1, m_ui->FDDRIVE2, m_ui->FDDRIVE3, m_ui->FDDRIVE4 };
	for (int i = 0; i < 4; i++) {
		drives[i]->setChecked((np2cfg.fddequip >> i) & 1);
	}
	m_ui->USE144FD->setChecked(np2cfg.usefd144 != 0);
	m_ui->Seek_Snd->setChecked(np2cfg.MOTOR != 0);
}

void FddPage::save()
{
	QCheckBox *const drives[4] = { m_ui->FDDRIVE1, m_ui->FDDRIVE2, m_ui->FDDRIVE3, m_ui->FDDRIVE4 };
	UINT8 equip = np2cfg.fddequip & 0xf0;
	for (int i = 0; i < 4; i++) {
		if (drives[i]->isChecked()) {
			equip |= static_cast<UINT8>(1 << i);
		}
	}
	np2cfg.fddequip = equip;
	np2cfg.usefd144 = m_ui->USE144FD->isChecked() ? 1 : 0;
	np2cfg.MOTOR = m_ui->Seek_Snd->isChecked() ? 1 : 0;
}
