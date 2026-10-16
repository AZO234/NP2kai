/* === preferences: MIDI page ===
 * The MIDI tab of wx/dialog/prefframe.cpp, laid out and computed as the
 * Windows port's MPU-PC98II and S-MPU pages (windows/dialog/d_mpu98.cpp).
 */

#include <compiler.h>
#include "midipage.h"
#include "ui_midipage.h"
#include "np2.h"
#include "commng.h"

#include <pccore.h>
#include <generic/dipswbmp.h>

#include <QSignalBlocker>

/* Settings the boards come with: E0D0h, INT2 */
static const UINT8 kMpuDefault = 0x82;

/* The 16 I/O ports C0D0h-FCD0h, bits 4-7 of the option byte */
static void fillPorts(QComboBox *combo)
{
	for (int i = 0; i < 16; i++) {
		combo->addItem(QString::asprintf("%04Xh", 0xC0D0 + (i << 10)));
	}
}

MidiPage::MidiPage(QWidget *parent)
	: PrefPage(parent)
	, m_ui(new Ui::MidiPage)
	, m_mpu(0)
	, m_smpu(0)
{
	m_ui->setupUi(this);

	fillPorts(m_ui->MPU_PORT);
	auto mpuChanged = [this]() {
		setMpu(static_cast<UINT8>((m_ui->MPU_PORT->currentIndex() << 4) | m_ui->MPU_IRQ->currentIndex()));
	};
	connect(m_ui->MPU_PORT, &QComboBox::currentIndexChanged, this, mpuChanged);
	connect(m_ui->MPU_IRQ, &QComboBox::currentIndexChanged, this, mpuChanged);
	connect(m_ui->mpuDefaults, &QPushButton::clicked, this, [this]() { setMpu(kMpuDefault); });
	/* Switches 1-4 are the port, the jumpers further right the interrupt */
	connect(m_ui->mpuDipsw, &DipSwView::clicked, this, [this](const QPoint &pos) {
		const int x = pos.x() / 9;
		const int y = pos.y() / 9;
		if (y < 1 || y >= 3) {
			return;
		}
		if (x >= 2 && x < 6) {
			setMpu(static_cast<UINT8>(m_mpu ^ (0x80 >> (x - 2))));
		} else if (x >= 9 && x < 13) {
			setMpu(static_cast<UINT8>((m_mpu & ~0x03) | (12 - x)));
		}
	});
	connect(m_ui->USEMPU98, &QCheckBox::toggled, m_ui->mpuAssignGroup, &QWidget::setEnabled);

#if defined(SUPPORT_SMPU98)
	fillPorts(m_ui->SMPU_PORT);
	auto smpuChanged = [this]() {
		setSmpu(static_cast<UINT8>((m_ui->SMPU_PORT->currentIndex() << 4) | m_ui->SMPU_IRQ->currentIndex()));
	};
	connect(m_ui->SMPU_PORT, &QComboBox::currentIndexChanged, this, smpuChanged);
	connect(m_ui->SMPU_IRQ, &QComboBox::currentIndexChanged, this, smpuChanged);
	connect(m_ui->smpuDefaults, &QPushButton::clicked, this, [this]() { setSmpu(kMpuDefault); });
	/* The S-MPU numbers its port switches the other way round */
	connect(m_ui->smpuDipsw, &DipSwView::clicked, this, [this](const QPoint &pos) {
		const int x = pos.x() / 9;
		const int y = pos.y() / 9;
		if (y < 1 || y >= 3) {
			return;
		}
		if (x >= 2 && x < 6) {
			setSmpu(static_cast<UINT8>(m_smpu ^ (0x10 << (x - 2))));
		} else if (x >= 9 && x < 13) {
			setSmpu(static_cast<UINT8>((m_smpu & ~0x03) | (12 - x)));
		}
	});
	for (QWidget *w : { static_cast<QWidget *>(m_ui->SMPUMUTB), static_cast<QWidget *>(m_ui->smpuAGroup),
	                    static_cast<QWidget *>(m_ui->smpuBGroup) }) {
		connect(m_ui->USE_SMPU, &QCheckBox::toggled, w, &QWidget::setEnabled);
	}
#else
	m_ui->midiTabs->removeTab(m_ui->midiTabs->indexOf(m_ui->smpuTab));
#endif
}

MidiPage::~MidiPage() = default;

void MidiPage::setMpu(UINT8 value)
{
	m_mpu = value;
	{
		const QSignalBlocker b1(m_ui->MPU_PORT), b2(m_ui->MPU_IRQ);
		m_ui->MPU_PORT->setCurrentIndex(value >> 4);
		m_ui->MPU_IRQ->setCurrentIndex(value & 0x03);
	}
	m_ui->mpuDipsw->setBitmap(dipswbmp_getmpu(m_mpu));
}

void MidiPage::setSmpu(UINT8 value)
{
	m_smpu = value;
#if defined(SUPPORT_SMPU98)
	{
		const QSignalBlocker b1(m_ui->SMPU_PORT), b2(m_ui->SMPU_IRQ);
		m_ui->SMPU_PORT->setCurrentIndex(value >> 4);
		m_ui->SMPU_IRQ->setCurrentIndex(value & 0x03);
	}
	m_ui->smpuDipsw->setBitmap(dipswbmp_getsmpu(m_smpu));
#endif
}

void MidiPage::load()
{
	m_ui->USEMPU98->setChecked(np2cfg.mpuenable != 0);
	m_ui->mpuAssignGroup->setEnabled(np2cfg.mpuenable != 0);
	setMpu(np2cfg.mpuopt);
	m_ui->mpuAssign->load(np2oscfg.mpu, np2oscfg.MIDIDEV[0], np2oscfg.MIDIDEV[1]);
#if defined(SUPPORT_SMPU98)
	m_ui->USE_SMPU->setChecked(np2cfg.smpuenable != 0);
	m_ui->SMPUMUTB->setChecked(np2cfg.smpumuteB != 0);
	for (QWidget *w : { static_cast<QWidget *>(m_ui->SMPUMUTB), static_cast<QWidget *>(m_ui->smpuAGroup),
	                    static_cast<QWidget *>(m_ui->smpuBGroup) }) {
		w->setEnabled(np2cfg.smpuenable != 0);
	}
	setSmpu(np2cfg.smpuopt);
	m_ui->smpuAAssign->load(np2oscfg.smpuA, np2oscfg.MIDIDEVA[0], np2oscfg.MIDIDEVA[1]);
	m_ui->smpuBAssign->load(np2oscfg.smpuB, np2oscfg.MIDIDEVB[0], np2oscfg.MIDIDEVB[1]);
#endif
}

void MidiPage::save()
{
	np2cfg.mpuenable = m_ui->USEMPU98->isChecked() ? 1 : 0;
	np2cfg.mpuopt = m_mpu;
	/* commng only opens MIDI when the port says so */
	np2oscfg.mpu.port = COMPORT_MIDI;
	m_ui->mpuAssign->save(np2oscfg.mpu, np2oscfg.MIDIDEV[0], np2oscfg.MIDIDEV[1]);
#if defined(SUPPORT_SMPU98)
	np2cfg.smpuenable = m_ui->USE_SMPU->isChecked() ? 1 : 0;
	np2cfg.smpumuteB = m_ui->SMPUMUTB->isChecked() ? 1 : 0;
	np2cfg.smpuopt = m_smpu;
	np2oscfg.smpuA.port = COMPORT_MIDI;
	np2oscfg.smpuB.port = COMPORT_MIDI;
	m_ui->smpuAAssign->save(np2oscfg.smpuA, np2oscfg.MIDIDEVA[0], np2oscfg.MIDIDEVA[1]);
	m_ui->smpuBAssign->save(np2oscfg.smpuB, np2oscfg.MIDIDEVB[0], np2oscfg.MIDIDEVB[1]);
#endif
}
