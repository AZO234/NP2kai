/* === preferences: Display page ===
 * The Display tab of wx/dialog/prefframe.cpp, laid out as the Windows port's
 * screen options (windows/dialog/d_screen.cpp, d_wab.cpp).
 */

#include <compiler.h>
#include "displaypage.h"
#include "ui_displaypage.h"
#include "np2.h"

#include <pccore.h>
#if defined(SUPPORT_CL_GD5430)
#include <wab/cirrus_vga_extern.h>
#endif

/* The scaling choices the screen widget tells apart */
static const UINT8 kInterps[] = { INTERP_NEAREST, INTERP_BILINEAR };

#if defined(SUPPORT_CL_GD5430)
static const UINT16 kClgdTypes[] = {
	CIRRUS_98ID_Be,
	CIRRUS_98ID_Xe,
	CIRRUS_98ID_Cb,
	CIRRUS_98ID_Cf,
	CIRRUS_98ID_Xe10,
	CIRRUS_98ID_Cb2,
	CIRRUS_98ID_Cx2,
	CIRRUS_98ID_PCI,
	CIRRUS_98ID_WAB,
	CIRRUS_98ID_WSN_A2F,
	CIRRUS_98ID_WSN,
	CIRRUS_98ID_GA98NBIC,
	CIRRUS_98ID_GA98NBII,
	CIRRUS_98ID_GA98NBIV,
	CIRRUS_98ID_96,
	CIRRUS_98ID_AUTO_XE_G1_PCI,
	CIRRUS_98ID_AUTO_XE_G2_PCI,
	CIRRUS_98ID_AUTO_XE_G4_PCI,
	CIRRUS_98ID_AUTO_XE_WA_PCI,
	CIRRUS_98ID_AUTO_XE_WS_PCI,
	CIRRUS_98ID_AUTO_XE_W4_PCI,
	CIRRUS_98ID_AUTO_XE10_WABS,
	CIRRUS_98ID_AUTO_XE10_WSN2,
	CIRRUS_98ID_AUTO_XE10_WSN4,
};
#endif

DisplayPage::DisplayPage(QWidget *parent)
	: PrefPage(parent)
	, m_ui(new Ui::DisplayPage)
{
	m_ui->setupUi(this);

	showSliderValue(m_ui->skplight, m_ui->skplightValue);
	showSliderValue(m_ui->tramwait, m_ui->tramwaitValue);
	showSliderValue(m_ui->vramwait, m_ui->vramwaitValue);
	showSliderValue(m_ui->grcgwait, m_ui->grcgwaitValue);
	showSliderValue(m_ui->RPal_tim, m_ui->RPal_timValue);

	connect(m_ui->LCD_MODE_en, &QCheckBox::toggled, m_ui->LCD_MODE_rev, &QWidget::setEnabled);
	connect(m_ui->skipline, &QCheckBox::toggled, m_ui->skipLightLabel, &QWidget::setEnabled);
	connect(m_ui->skipline, &QCheckBox::toggled, m_ui->skplight, &QWidget::setEnabled);
	connect(m_ui->skipline, &QCheckBox::toggled, m_ui->skplightValue, &QWidget::setEnabled);

#if !defined(SUPPORT_PEGC)
	m_ui->pegcplane->hide();
#endif
#if !defined(SUPPORT_WAB)
	m_ui->wabasw->hide();
#endif
#if !defined(SUPPORT_CL_GD5430)
	m_ui->USE_CLGD->hide();
#endif
#if !defined(SUPPORT_WAB) && !defined(SUPPORT_CL_GD5430)
	m_ui->displayTabs->removeTab(m_ui->displayTabs->indexOf(m_ui->accelTab));
#endif
}

DisplayPage::~DisplayPage() = default;

void DisplayPage::load()
{
	/* Video */
	m_ui->LCD_MODE_en->setChecked(np2cfg.LCD_MODE & 1);
	m_ui->LCD_MODE_rev->setChecked(np2cfg.LCD_MODE & 2);
	m_ui->LCD_MODE_rev->setEnabled(np2cfg.LCD_MODE & 1);
	m_ui->skipline->setChecked(np2cfg.skipline != 0);
	m_ui->skplight->setValue(np2cfg.skiplight);
	for (QWidget *w : { static_cast<QWidget *>(m_ui->skipLightLabel), static_cast<QWidget *>(m_ui->skplight),
	                    static_cast<QWidget *>(m_ui->skplightValue) }) {
		w->setEnabled(np2cfg.skipline != 0);
	}
	m_ui->DispSync->setChecked(np2cfg.DISPSYNC != 0);
	m_ui->Real_Pal->setChecked(np2cfg.RASTER != 0);
	/* Tiles draws sharp, hyper smooth */
	setComboValue(m_ui->INTERPOL, kInterps,
	              np2oscfg.drawinterp == INTERP_HYPER ? INTERP_BILINEAR : np2oscfg.drawinterp);
	m_ui->draw32bit->setChecked(draw32bit != 0);

	/* Chip */
	(np2cfg.uPD72020 ? m_ui->gdc_72020 : m_ui->gdc_7220)->setChecked(true);
	m_ui->GRCG_EGC->setCurrentIndex(np2cfg.grcg & 3);
#if defined(SUPPORT_PEGC)
	m_ui->pegcplane->setChecked(np2cfg.usepegcplane != 0);
#endif
	m_ui->color16b->setChecked(np2cfg.color16 != 0);
	m_ui->GDCWOVRG->setChecked(np2cfg.gdcwovrg != 0);

	/* Timing; the odd entries only say whether the even ones are set */
	m_ui->tramwait->setValue(np2cfg.wait[0]);
	m_ui->vramwait->setValue(np2cfg.wait[2]);
	m_ui->grcgwait->setValue(np2cfg.wait[4]);
	m_ui->RPal_tim->setValue(static_cast<int>(np2cfg.realpal) - 32);

	/* Accelerator */
#if defined(SUPPORT_WAB)
	m_ui->wabasw->setChecked(np2cfg.wabasw != 0);
#endif
#if defined(SUPPORT_CL_GD5430)
	m_ui->USE_CLGD->setChecked(np2cfg.usegd5430 != 0);
	setComboValue(m_ui->CLGDTYPE, kClgdTypes, np2cfg.gd5430type, 4);
	m_ui->CLGDFCUR->setChecked(np2cfg.gd5430fakecur != 0);
#endif
}

void DisplayPage::save()
{
	np2cfg.LCD_MODE = (m_ui->LCD_MODE_en->isChecked() ? 1 : 0) | (m_ui->LCD_MODE_rev->isChecked() ? 2 : 0);
	np2cfg.skipline = m_ui->skipline->isChecked() ? 1 : 0;
	np2cfg.skiplight = static_cast<UINT16>(m_ui->skplight->value());
	np2cfg.DISPSYNC = m_ui->DispSync->isChecked() ? 1 : 0;
	np2cfg.RASTER = m_ui->Real_Pal->isChecked() ? 1 : 0;
	/* Keep tiles or hyper when the choice still matches it */
	const UINT8 interp = comboValue(m_ui->INTERPOL, kInterps);
	const bool smooth = (np2oscfg.drawinterp == INTERP_BILINEAR || np2oscfg.drawinterp == INTERP_HYPER);
	if (smooth != (interp == INTERP_BILINEAR)) {
		np2oscfg.drawinterp = interp;
	}
	draw32bit = m_ui->draw32bit->isChecked() ? 1 : 0;

	np2cfg.uPD72020 = m_ui->gdc_72020->isChecked() ? 1 : 0;
	np2cfg.grcg = static_cast<UINT8>(m_ui->GRCG_EGC->currentIndex());
#if defined(SUPPORT_PEGC)
	np2cfg.usepegcplane = m_ui->pegcplane->isChecked() ? 1 : 0;
#endif
	np2cfg.color16 = m_ui->color16b->isChecked() ? 1 : 0;
	np2cfg.gdcwovrg = m_ui->GDCWOVRG->isChecked() ? 1 : 0;

	const int waits[3] = { m_ui->tramwait->value(), m_ui->vramwait->value(), m_ui->grcgwait->value() };
	for (int i = 0; i < 3; i++) {
		np2cfg.wait[i * 2] = static_cast<UINT8>(waits[i]);
		np2cfg.wait[i * 2 + 1] = waits[i] ? 1 : 0;
	}
	np2cfg.realpal = static_cast<UINT8>(m_ui->RPal_tim->value() + 32);

#if defined(SUPPORT_WAB)
	np2cfg.wabasw = m_ui->wabasw->isChecked() ? 1 : 0;
#endif
#if defined(SUPPORT_CL_GD5430)
	np2cfg.usegd5430 = m_ui->USE_CLGD->isChecked() ? 1 : 0;
	np2cfg.gd5430type = comboValue(m_ui->CLGDTYPE, kClgdTypes);
	np2cfg.gd5430fakecur = m_ui->CLGDFCUR->isChecked() ? 1 : 0;
#endif
}
