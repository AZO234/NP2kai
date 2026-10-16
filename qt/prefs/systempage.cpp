/* === preferences: System page ===
 * As the System tab of wx/dialog/prefframe.cpp and the Windows port's
 * Configure dialog (windows/dialog/d_config.cpp).
 */

#include <compiler.h>
#include "systempage.h"
#include "ui_systempage.h"
#include "np2.h"

#include <pccore.h>
#include <common/strres.h>
#if defined(SUPPORT_WAB)
#include <wab/wab.h>
#endif

static const UINT32 kBaseClocks[] = { PCBASECLOCK20, PCBASECLOCK25 };
static const UINT32 kEmuSpeeds[] = { 50, 100, 200, 400, 800, 1600 };
#if defined(CPUCORE_IA32)
/* SetCpuTypeIndex() numbers, in the order of the CPUType list */
static const int kCpuTypes[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 255,
};
static const UINT8 kFpuTypes[] = { FPU_TYPE_SOFTFLOAT, FPU_TYPE_DOSBOX, FPU_TYPE_DOSBOX2 };
#endif

SystemPage::SystemPage(QWidget *parent)
	: PrefPage(parent)
	, m_ui(new Ui::SystemPage)
{
	m_ui->setupUi(this);

	m_ui->clk_mult->setMaximum(CPU_MULTIPLE_MAX);
#if defined(CPUCORE_IA32)
	m_ui->extmem->setMaximum(MEMORY_MAXSIZE);
#else
	/* The 80286 addresses 16 MB, the first one being conventional memory */
	m_ui->extmem->setMaximum(13);
	m_ui->cpuTypeLabel->hide();
	m_ui->CPUType->hide();
	m_ui->fpuTypeLabel->hide();
	m_ui->FPU_TYPE->hide();
	m_ui->DisableMMX->hide();
	m_ui->CONSTTSC->hide();
#endif
#if !defined(SUPPORT_FAST_MEMORYCHECK)
	m_ui->MemCheck->hide();
#endif
#if !defined(SUPPORT_WAB)
	m_ui->MULTITHREAD->hide();
#endif

	connect(m_ui->clk_base, &QComboBox::currentIndexChanged, this, &SystemPage::updateMHz);
	connect(m_ui->clk_mult, &QSpinBox::valueChanged, this, &SystemPage::updateMHz);
}

SystemPage::~SystemPage() = default;

void SystemPage::updateMHz()
{
	const double mhz = comboValue(m_ui->clk_base, kBaseClocks) * static_cast<double>(m_ui->clk_mult->value()) / 1000000.0;
	m_ui->cpuMHz->setText(tr("= %1 MHz").arg(mhz, 0, 'f', 4));
}

void SystemPage::load()
{
	if (milstr_cmp(np2cfg.model, str_VM) == 0) {
		m_ui->pc_model_VM->setChecked(true);
	} else if (milstr_cmp(np2cfg.model, str_EPSON) == 0
	           || milstr_cmp(np2cfg.model, OEMTEXT("286")) == 0) {   /* as the wx port wrote it */
		m_ui->pc_model_286->setChecked(true);
	} else {
		m_ui->pc_model_VX->setChecked(true);
	}
	/* Only the high byte masks I/O addresses (iocore.c) */
	m_ui->SYSIOMSK->setChecked((np2cfg.sysiomsk >> 8) != 0);

	setComboValue(m_ui->clk_base, kBaseClocks, np2cfg.baseclock, 1);
	m_ui->clk_mult->setValue(static_cast<int>(np2cfg.multiple));
	updateMHz();
#if defined(CPUCORE_IA32)
	setComboValue(m_ui->CPUType, kCpuTypes, static_cast<unsigned>(GetCpuTypeIndex()));
	setComboValue(m_ui->FPU_TYPE, kFpuTypes, np2cfg.fpu_type);
	m_ui->DisableMMX->setChecked(np2oscfg.disablemmx != 0);
	m_ui->CONSTTSC->setChecked(np2cfg.consttsc != 0);
#endif
	/* The nearest speed at or above the configured one */
	int speed = static_cast<int>(sizeof(kEmuSpeeds) / sizeof(kEmuSpeeds[0])) - 1;
	for (int i = 0; i <= speed; i++) {
		if (np2cfg.emuspeed <= kEmuSpeeds[i]) {
			speed = i;
			break;
		}
	}
	m_ui->EmuSpeed->setCurrentIndex(speed);

	m_ui->extmem->setValue(np2cfg.EXTMEM);
#if defined(SUPPORT_FAST_MEMORYCHECK)
	m_ui->MemCheck->setChecked(np2cfg.memcheckspeed > 1);
#endif
#if defined(SUPPORT_WAB)
	m_ui->MULTITHREAD->setChecked(np2wabcfg.multithread != 0);
#endif
}

void SystemPage::save()
{
	const OEMCHAR *model = str_VX;
	if (m_ui->pc_model_VM->isChecked()) {
		model = str_VM;
	} else if (m_ui->pc_model_286->isChecked()) {
		model = str_EPSON;
	}
	milstr_ncpy(np2cfg.model, model, NELEMENTS(np2cfg.model));
	if (m_ui->SYSIOMSK->isChecked()) {
		np2cfg.sysiomsk = 0xff00;
	} else {
		np2cfg.sysiomsk &= 0x00ff;
	}

	np2cfg.baseclock = comboValue(m_ui->clk_base, kBaseClocks);
	np2cfg.multiple = static_cast<UINT>(m_ui->clk_mult->value());
#if defined(CPUCORE_IA32)
	/* (custom) keeps whatever CPUID values the configuration file holds */
	const int cpuType = comboValue(m_ui->CPUType, kCpuTypes);
	if (cpuType != 0 && cpuType != GetCpuTypeIndex()) {
		SetCpuTypeIndex(static_cast<UINT>(cpuType));
	}
	np2cfg.fpu_type = comboValue(m_ui->FPU_TYPE, kFpuTypes);
	np2oscfg.disablemmx = m_ui->DisableMMX->isChecked() ? 1 : 0;
	np2cfg.consttsc = m_ui->CONSTTSC->isChecked() ? 1 : 0;
#endif
	np2cfg.emuspeed = comboValue(m_ui->EmuSpeed, kEmuSpeeds);

	np2cfg.EXTMEM = static_cast<decltype(np2cfg.EXTMEM)>(m_ui->extmem->value());
#if defined(SUPPORT_FAST_MEMORYCHECK)
	np2cfg.memcheckspeed = m_ui->MemCheck->isChecked() ? 8 : 1;
#endif
#if defined(SUPPORT_WAB)
	np2wabcfg.multithread = m_ui->MULTITHREAD->isChecked() ? 1 : 0;
#endif
}
