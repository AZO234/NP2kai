/* === preferences: Input page ===
 * The Input tab of wx/dialog/prefframe.cpp, without drag and drop, which this
 * port does not read; the joypad device moved to the Joypad page under Sound,
 * next to the game port. The mouse speed is the Windows port's MOUSEMUL and
 * MOUSEDIV, offered as fractions of the host pointer's speed; RAWMOUSE leaves
 * the acceleration to the guest OS.
 */

#include <compiler.h>
#include "inputpage.h"
#include "ui_inputpage.h"
#include "np2.h"
#include "kbdmng.h"

#include <pccore.h>

#include <QGuiApplication>
#include <QSignalBlocker>

static const UINT8 kKeyboards[] = { KEY_KEY106, KEY_KEY101 };

/* Slider positions, slowest first; the last is full speed */
static const struct { SINT16 mul, div; } kMouseSpeeds[] = {
	{ 1, 8 }, { 1, 4 }, { 1, 3 }, { 1, 2 }, { 2, 3 }, { 1, 1 },
};
static const int kMouseFull = NELEMENTS(kMouseSpeeds) - 1;

InputPage::InputPage(QWidget *parent)
	: PrefPage(parent)
	, m_ui(new Ui::InputPage)
	, m_mousemul(1)
	, m_mousediv(1)
	, m_mouseMoved(false)
{
	m_ui->setupUi(this);
	/* Only the Wayland pointer lock gets both kinds of motion */
#if defined(NP2_QT_WAYLAND)
	m_ui->mouseAccel->setEnabled(QGuiApplication::platformName().startsWith(QLatin1String("wayland")));
#else
	m_ui->mouseAccel->setEnabled(false);
#endif
	connect(m_ui->Mouse_sp, &QSlider::valueChanged, this, [this](int index) {
		m_mouseMoved = true;
		showMouseSpeed(index);
	});
}

void InputPage::showMouseSpeed(int index)
{
	m_ui->mouseSpeedValue->setText(index == kMouseFull ? tr("Full")
	    : QStringLiteral("%1/%2").arg(kMouseSpeeds[index].mul).arg(kMouseSpeeds[index].div));
}

InputPage::~InputPage() = default;

void InputPage::load()
{
	setComboValue(m_ui->keyboard, kKeyboards, np2oscfg.KEYBOARD);
	m_ui->F11KEY->setCurrentIndex(np2oscfg.F11KEY < m_ui->F11KEY->count() ? np2oscfg.F11KEY : 0);
	m_ui->F12KEY->setCurrentIndex(np2oscfg.F12KEY < m_ui->F12KEY->count() ? np2oscfg.F12KEY : 0);
	m_ui->XSHIFT->setChecked(np2oscfg.xrollkey != 0);
	m_ui->keyrepeat_enable->setChecked(np2cfg.keyrepeat_enable != 0);
	m_ui->keyrepeat_delay->setValue(np2cfg.keyrepeat_delay);
	m_ui->keyrepeat_interval->setValue(np2cfg.keyrepeat_interval);

	m_ui->btnMODE->setChecked(np2cfg.BTN_MODE != 0);
	m_ui->btnRAPID->setChecked(np2cfg.BTN_RAPID != 0);
	m_ui->MS_RAPID->setChecked(np2cfg.MOUSERAPID != 0);

	m_mousemul = np2oscfg.mousemul;
	m_mousediv = np2oscfg.mousediv;
	if (m_mousemul <= 0 || m_mousediv <= 0) {
		/* Full speed, as the pointer capture reads it */
		m_mousemul = 1;
		m_mousediv = 1;
	}
	const double speed = double(m_mousemul) / m_mousediv;
	int index = kMouseFull;
	while (index > 0 && speed <= double(kMouseSpeeds[index - 1].mul) / kMouseSpeeds[index - 1].div) {
		index--;
	}
	{
		const QSignalBlocker block(m_ui->Mouse_sp);
		m_ui->Mouse_sp->setValue(index);
	}
	m_mouseMoved = false;
	m_ui->mouseAccel->setChecked(np2oscfg.rawmouse == 0);
	if (m_mousemul * kMouseSpeeds[index].div == m_mousediv * kMouseSpeeds[index].mul) {
		showMouseSpeed(index);
	} else {
		m_ui->mouseSpeedValue->setText(QStringLiteral("%1/%2").arg(m_mousemul).arg(m_mousediv));
	}
}

void InputPage::save()
{
	np2oscfg.KEYBOARD = comboValue(m_ui->keyboard, kKeyboards);
	np2oscfg.F11KEY = static_cast<UINT8>(m_ui->F11KEY->currentIndex());
	np2oscfg.F12KEY = static_cast<UINT8>(m_ui->F12KEY->currentIndex());
	np2oscfg.xrollkey = m_ui->XSHIFT->isChecked() ? 1 : 0;
	np2cfg.keyrepeat_enable = m_ui->keyrepeat_enable->isChecked() ? 1 : 0;
	np2cfg.keyrepeat_delay = static_cast<UINT16>(m_ui->keyrepeat_delay->value());
	np2cfg.keyrepeat_interval = static_cast<UINT16>(m_ui->keyrepeat_interval->value());

	np2cfg.BTN_MODE = m_ui->btnMODE->isChecked() ? 1 : 0;
	np2cfg.BTN_RAPID = m_ui->btnRAPID->isChecked() ? 1 : 0;
	np2cfg.MOUSERAPID = m_ui->MS_RAPID->isChecked() ? 1 : 0;

	np2oscfg.rawmouse = m_ui->mouseAccel->isChecked() ? 0 : 1;
	if (m_mouseMoved) {
		np2oscfg.mousemul = kMouseSpeeds[m_ui->Mouse_sp->value()].mul;
		np2oscfg.mousediv = kMouseSpeeds[m_ui->Mouse_sp->value()].div;
	} else {
		np2oscfg.mousemul = m_mousemul;
		np2oscfg.mousediv = m_mousediv;
	}
}
