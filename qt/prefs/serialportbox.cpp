/* === preferences: one serial channel of the PC-98 === */

#include <compiler.h>
#include "serialportbox.h"
#include "ui_serialportbox.h"
#include "commng.h"
#include "cmserial.h"
#include "cmmidi.h"
#include "prefpage.h"

#include <common/milstr.h>

#include <QDir>
#include <QFile>

enum { CONNECT_NONE, CONNECT_SERIAL, CONNECT_MIDI };

/* Bits of the 8251 mode byte kept in COMCFG.param */
static const UINT8 kParities[] = { 0x00, 0x10, 0x30 };
static const UINT8 kStopBits[] = { 0x40, 0x80, 0xc0 };

/* The serial ports of the host, USB adapters included */
static QStringList serialDevices()
{
	QStringList list;
	const QDir dev(QStringLiteral("/dev"));
	for (const QString &name : dev.entryList({ QStringLiteral("ttyS*"), QStringLiteral("ttyUSB*"),
	                                           QStringLiteral("ttyACM*") }, QDir::System, QDir::Name)) {
		list << dev.filePath(name);
	}
	return list;
}

SerialPortBox::SerialPortBox(QWidget *parent)
	: QWidget(parent)
	, m_ui(new Ui::SerialPortBox)
	, m_param(0)
	, m_speed(0)
	, m_shown{ -1, -1, -1, -1 }
{
	m_ui->setupUi(this);
	m_ui->device->addItems(serialDevices());
	for (UINT32 speed : cmserial_speed) {
		m_ui->speed->addItem(tr("%1 bps").arg(speed), speed);
	}
	m_ui->midi->setDeviceFieldsVisible(false);
	connect(m_ui->direct, &QCheckBox::toggled, this, &SerialPortBox::updateEnabled);
}

SerialPortBox::~SerialPortBox() = default;

void SerialPortBox::updateEnabled()
{
	const bool fixed = !m_ui->direct->isChecked();
	for (QWidget *w : { static_cast<QWidget *>(m_ui->speedLabel), static_cast<QWidget *>(m_ui->speed),
	                    static_cast<QWidget *>(m_ui->dataBitsLabel), static_cast<QWidget *>(m_ui->dataBits),
	                    static_cast<QWidget *>(m_ui->parityLabel), static_cast<QWidget *>(m_ui->parity),
	                    static_cast<QWidget *>(m_ui->stopBitsLabel), static_cast<QWidget *>(m_ui->stopBits) }) {
		w->setEnabled(fixed);
	}
}

void SerialPortBox::load(const COMCFG &cfg, int channel)
{
	int connect = CONNECT_NONE;
	if (cfg.port == COMPORT_MIDI) {
		connect = CONNECT_MIDI;
	} else if (cfg.port >= COMPORT_COM1 && cfg.port <= COMPORT_COM4) {
		connect = CONNECT_SERIAL;
	}
	m_ui->connectTo->setCurrentIndex(connect);
	m_ui->connectPages->setCurrentIndex(connect);
	(void)channel;

	/* mout holds the device file of a serial connection, the kind of MIDI
	 * output of a MIDI one */
	m_ui->device->setCurrentText(connect == CONNECT_SERIAL ? QFile::decodeName(cfg.mout) : QString());
	m_ui->direct->setChecked(cfg.direct != 0);
	int speed = m_ui->speed->findData(cfg.speed);
	if (speed < 0) {
		speed = m_ui->speed->findData(19200);
	}
	m_ui->speed->setCurrentIndex(speed);
	m_ui->dataBits->setCurrentIndex((cfg.param >> 2) & 3);
	setComboValue(m_ui->parity, kParities, cfg.param & 0x30);
	setComboValue(m_ui->stopBits, kStopBits, cfg.param & 0xc0);
	m_param = cfg.param;
	m_speed = cfg.speed;
	m_shown[0] = m_ui->speed->currentIndex();
	m_shown[1] = m_ui->dataBits->currentIndex();
	m_shown[2] = m_ui->parity->currentIndex();
	m_shown[3] = m_ui->stopBits->currentIndex();
	COMCFG midi = cfg;
	if (connect != CONNECT_MIDI) {
		midi.mout[0] = '\0';
		midi.min[0] = '\0';
	}
	m_ui->midi->load(midi, nullptr, nullptr);
	updateEnabled();
}

void SerialPortBox::save(COMCFG &cfg, int channel) const
{
	switch (m_ui->connectTo->currentIndex()) {
	case CONNECT_SERIAL:
		/* cmserial opens com[port - COM1].mout: the channel's own */
		cfg.port = static_cast<UINT8>(COMPORT_COM1 + channel);
		milstr_ncpy(cfg.mout, QFile::encodeName(m_ui->device->currentText().trimmed()).constData(),
		            NELEMENTS(cfg.mout));
		cfg.min[0] = '\0';
		break;
	case CONNECT_MIDI:
		cfg.port = COMPORT_MIDI;
		m_ui->midi->save(cfg, nullptr, nullptr);
		break;
	default:
		cfg.port = COMPORT_NONE;
		break;
	}
	cfg.direct = m_ui->direct->isChecked() ? 1 : 0;
	cfg.speed = (m_ui->speed->currentIndex() != m_shown[0]) ? m_ui->speed->currentData().toUInt() : m_speed;
	UINT8 param = m_param;
	if (m_ui->dataBits->currentIndex() != m_shown[1]) {
		param = static_cast<UINT8>((param & ~0x0c) | (m_ui->dataBits->currentIndex() << 2));
	}
	if (m_ui->parity->currentIndex() != m_shown[2]) {
		param = static_cast<UINT8>((param & ~0x30) | comboValue(m_ui->parity, kParities));
	}
	if (m_ui->stopBits->currentIndex() != m_shown[3]) {
		param = static_cast<UINT8>((param & ~0xc0) | comboValue(m_ui->stopBits, kStopBits));
	}
	cfg.param = param;
}
