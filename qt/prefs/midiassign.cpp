/* === preferences: where a MIDI port of the PC-98 goes === */

#include <compiler.h>
#include "midiassign.h"
#include "ui_midiassign.h"
#include "commng.h"
#include "cmmidi.h"

#include <common/milstr.h>

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>

enum { OUT_NONE, OUT_VERMOUTH, OUT_DEVICE };
enum { IN_NONE, IN_DEVICE };

/* The raw MIDI devices of ALSA and OSS */
static QStringList midiDevices()
{
	QStringList list;
	const QDir snd(QStringLiteral("/dev/snd"));
	for (const QString &name : snd.entryList({ QStringLiteral("midiC*") }, QDir::System, QDir::Name)) {
		list << snd.filePath(name);
	}
	const QDir dev(QStringLiteral("/dev"));
	for (const QString &name : dev.entryList({ QStringLiteral("midi*") }, QDir::System, QDir::Name)) {
		list << dev.filePath(name);
	}
	return list;
}

MidiAssign::MidiAssign(QWidget *parent)
	: QWidget(parent)
	, m_ui(new Ui::MidiAssign)
	, m_deviceFields(true)
{
	m_ui->setupUi(this);
#if !defined(VERMOUTH_LIB)
	m_ui->midiOut->removeItem(OUT_VERMOUTH);
#endif
	const QStringList devices = midiDevices();
	m_ui->midiOutDevice->addItems(devices);
	m_ui->midiInDevice->addItems(devices);
	m_ui->module->addItem(QString());
	for (int i = 0; i < MIDI_OTHER; i++) {
		m_ui->module->addItem(QString::fromLatin1(cmmidi_mdlname[i]));
	}

	connect(m_ui->midiOut, &QComboBox::currentIndexChanged, this, &MidiAssign::updateEnabled);
	connect(m_ui->midiIn, &QComboBox::currentIndexChanged, this, &MidiAssign::updateEnabled);
	connect(m_ui->defineEnabled, &QCheckBox::toggled, this, &MidiAssign::updateEnabled);
	connect(m_ui->defineBrowse, &QPushButton::clicked, this, [this]() {
		const QString current = m_ui->defineFile->text();
		const QString path = QFileDialog::getOpenFileName(this, tr("MIMPI Define File"),
		    current.isEmpty() ? QString() : QFileInfo(current).absolutePath(),
		    tr("Define files (*.def *.DEF);;All files (*)"));
		if (!path.isEmpty()) {
			m_ui->defineFile->setText(path);
		}
	});
}

MidiAssign::~MidiAssign() = default;

void MidiAssign::setDeviceFieldsVisible(bool visible)
{
	m_deviceFields = visible;
	m_ui->midiOutDevice->setVisible(visible);
	m_ui->midiInDevice->setVisible(visible);
	if (!visible) {
		const QString shared = tr("The device set for the MPU-PC98II on the MIDI page");
		m_ui->midiOut->setItemData(m_ui->midiOut->count() - 1, shared, Qt::ToolTipRole);
		m_ui->midiIn->setItemData(IN_DEVICE, shared, Qt::ToolTipRole);
	}
	updateEnabled();
}

void MidiAssign::updateEnabled()
{
	const bool out = m_ui->midiOut->currentIndex() != OUT_NONE;
	m_ui->midiOutDevice->setEnabled(m_ui->midiOut->currentIndex() == m_ui->midiOut->count() - 1);
	m_ui->midiInDevice->setEnabled(m_ui->midiIn->currentIndex() == IN_DEVICE);
	m_ui->module->setEnabled(out);
	m_ui->defineEnabled->setEnabled(out);
	m_ui->defineFile->setEnabled(out && m_ui->defineEnabled->isChecked());
	m_ui->defineBrowse->setEnabled(out && m_ui->defineEnabled->isChecked());
}

void MidiAssign::load(const COMCFG &cfg, const char *outDevice, const char *inDevice)
{
	int out = OUT_NONE;
#if defined(VERMOUTH_LIB)
	if (milstr_cmp(cfg.mout, cmmidi_vermouth) == 0) {
		out = OUT_VERMOUTH;
	}
#endif
	if (milstr_cmp(cfg.mout, cmmidi_midiout_device) == 0) {
		out = m_ui->midiOut->count() - 1;
	}
	m_ui->midiOut->setCurrentIndex(out);
	m_ui->midiIn->setCurrentIndex(milstr_cmp(cfg.min, cmmidi_midiin_device) == 0 ? IN_DEVICE : IN_NONE);
	if (outDevice) {
		m_ui->midiOutDevice->setCurrentText(QFile::decodeName(outDevice));
	}
	if (inDevice) {
		m_ui->midiInDevice->setCurrentText(QFile::decodeName(inDevice));
	}
	m_ui->module->setCurrentText(QString::fromUtf8(cfg.mdl));
	m_ui->defineEnabled->setChecked(cfg.def_en != 0);
	m_ui->defineFile->setText(QFile::decodeName(cfg.def));
	updateEnabled();
}

void MidiAssign::save(COMCFG &cfg, char *outDevice, char *inDevice) const
{
	const char *mout = "";
	if (m_ui->midiOut->currentIndex() == m_ui->midiOut->count() - 1) {
		mout = cmmidi_midiout_device;
	}
#if defined(VERMOUTH_LIB)
	else if (m_ui->midiOut->currentIndex() == OUT_VERMOUTH) {
		mout = cmmidi_vermouth;
	}
#endif
	milstr_ncpy(cfg.mout, mout, NELEMENTS(cfg.mout));
	milstr_ncpy(cfg.min, m_ui->midiIn->currentIndex() == IN_DEVICE ? cmmidi_midiin_device : "",
	            NELEMENTS(cfg.min));
	if (outDevice && m_deviceFields) {
		milstr_ncpy(outDevice, QFile::encodeName(m_ui->midiOutDevice->currentText().trimmed()).constData(), MAX_PATH);
	}
	if (inDevice && m_deviceFields) {
		milstr_ncpy(inDevice, QFile::encodeName(m_ui->midiInDevice->currentText().trimmed()).constData(), MAX_PATH);
	}
	milstr_ncpy(cfg.mdl, m_ui->module->currentText().trimmed().toUtf8().constData(), NELEMENTS(cfg.mdl));
	cfg.def_en = m_ui->defineEnabled->isChecked() ? 1 : 0;
	milstr_ncpy(cfg.def, QFile::encodeName(m_ui->defineFile->text().trimmed()).constData(), NELEMENTS(cfg.def));
}
