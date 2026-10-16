/* === new hard disk image dialog for the Qt port === */

#include <compiler.h>
#include "newhddialog.h"
#include "ui_newhddialog.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>

namespace {

/* Size presets in MB, as in the Windows port */
const quint32 kSizes[] = { 20, 41, 65, 80, 127, 255, 511, 1023, 2047, 4095, 8191 };

/* Geometry presets */
const quint32 kCylinders[] = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65535 };
const quint32 kHeads[] = { 8, 15, 16 };
const quint32 kSectors[] = { 17, 63, 255 };
const quint32 kSectorSizes[] = { 256, 512 };

/* SASI drives (256-byte sectors) the PC-98 BIOS knows by geometry */
struct SasiGeometry {
	quint16 sectors;
	quint16 heads;
	quint16 cylinders;
	int     nominalMB;
};
const SasiGeometry kSasi[] = {
	{ 33, 4, 153,  5 },
	{ 33, 4, 310, 10 },
	{ 33, 6, 310, 15 },
	{ 33, 8, 310, 20 },
	{ 33, 6, 615, 30 },
	{ 33, 8, 615, 40 },
};

/* Shown limit for the large formats: what 28-bit LBA can address, as the
 * Windows port shows it */
const quint32 kLargeMaxMB = NHD_MAXSIZE28;

/* Size presets carry their size; SASI presets carry -(index + 1) */
int sasiIndex(const QVariant &data)
{
	const int v = data.toInt();
	return (v < 0) ? -v - 1 : -1;
}

void fillCombo(QComboBox *combo, const quint32 *values, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		combo->addItem(QString::number(values[i]), static_cast<uint>(values[i]));
	}
}

quint32 parseNumber(const QString &text, bool *ok)
{
	return text.trimmed().toUInt(ok);
}

} // namespace

bool NewHdDialog::hasGeometry(Format format)
{
	return format == Format::Nhd || format == Format::Hdi || format == Format::Vhd;
}

NewHdDialog::NewHdDialog(Format format, const QString &fileName, QWidget *parent)
	: QDialog(parent)
	, m_ui(new Ui::NewHdDialog)
	, m_format(format)
	, m_minSize(5)
	, m_maxSize(kLargeMaxMB)
	, m_allowSasi(format == Format::Nhd || format == Format::Hdi)
	, m_updating(false)
	, m_size(0)
	, m_c(0)
	, m_h(0)
	, m_s(0)
	, m_ss(512)
{
	m_ui->setupUi(this);

	static const char *const kNames[] = {
		QT_TR_NOOP("T98-Next hard disk image"),
		QT_TR_NOOP("Anex86 hard disk image"),
		QT_TR_NOOP("Virtual PC hard disk image"),
		QT_TR_NOOP("T98 hard disk image"),
		QT_TR_NOOP("Virtual98 hard disk image (SCSI)"),
		QT_TR_NOOP("RaSCSI hard disk image (SCSI)"),
	};
	m_ui->fileName->setText(QStringLiteral("%1\n%2")
	    .arg(QFileInfo(fileName).fileName(), tr(kNames[static_cast<int>(format)])));

	/* Limits of the core's newdisk_* functions (fdd/newdisk.c) */
	switch (format) {
	case Format::Thd: m_minSize = 5; m_maxSize = 256;  break;
	case Format::Hdd: m_minSize = 2; m_maxSize = 512;  break;
	case Format::Hdn: m_minSize = 2; m_maxSize = 6399; break;
	default: break;
	}
	m_ui->hddLimit->setText(tr("(%1–%2 MB)").arg(m_minSize).arg(m_maxSize));

	if (m_allowSasi) {
		for (size_t i = 0; i < NELEMENTS(kSasi); i++) {
			m_ui->hddSize->addItem(tr("(SASI) %1").arg(kSasi[i].nominalMB), -static_cast<int>(i) - 1);
		}
	}
	for (quint32 size : kSizes) {
		if (size >= m_minSize && size <= m_maxSize) {
			m_ui->hddSize->addItem(QString::number(size), static_cast<int>(size));
		}
	}
	m_ui->hddSize->setCurrentIndex(-1);
	m_ui->hddSize->setEditText(QString());

	fillCombo(m_ui->hddC, kCylinders, NELEMENTS(kCylinders));
	fillCombo(m_ui->hddH, kHeads, NELEMENTS(kHeads));
	fillCombo(m_ui->hddS, kSectors, NELEMENTS(kSectors));
	fillCombo(m_ui->hddSS, kSectorSizes, NELEMENTS(kSectorSizes));
	m_ui->hddSS->setCurrentText(QStringLiteral("512"));
	/* 256-byte sectors are for SASI drives only */
	m_ui->hddSS->setEnabled(m_allowSasi);
	for (QComboBox *combo : { m_ui->hddC, m_ui->hddH, m_ui->hddS }) {
		combo->setCurrentIndex(-1);
		combo->setEditText(QString());
	}

	m_ui->geometryGroup->setVisible(hasGeometry(format));
	m_ui->blankDisk->setVisible(hasGeometry(format));
	m_ui->vhdTypeGroup->setVisible(format == Format::Vhd);

	connect(m_ui->hddSize, &QComboBox::editTextChanged, this, &NewHdDialog::onSizeEdited);
	for (QComboBox *combo : { m_ui->hddC, m_ui->hddH, m_ui->hddS }) {
		connect(combo, &QComboBox::editTextChanged, this, &NewHdDialog::onGeometryEdited);
	}
	connect(m_ui->hddSS, &QComboBox::currentIndexChanged, this, &NewHdDialog::onGeometryEdited);

	updateOkButton();
	m_ui->hddSize->setFocus();
}

NewHdDialog::~NewHdDialog() = default;

bool NewHdDialog::isBlank() const
{
	return hasGeometry(m_format) && m_ui->blankDisk->isChecked();
}

bool NewHdDialog::isDynamic() const
{
	return m_format == Format::Vhd && m_ui->dynamicSize->isChecked();
}

/* Typed or picked size: a SASI preset brings its own geometry, a plain size
 * gets one worked out for it */
void NewHdDialog::onSizeEdited(const QString &text)
{
	if (m_updating) {
		return;
	}
	const int item = m_ui->hddSize->findText(text);
	const int sasi = (item >= 0) ? sasiIndex(m_ui->hddSize->itemData(item)) : -1;
	if (sasi >= 0) {
		m_c  = kSasi[sasi].cylinders;
		m_h  = kSasi[sasi].heads;
		m_s  = kSasi[sasi].sectors;
		m_ss = 256;
		m_size = kSasi[sasi].nominalMB;
		showGeometry();
	} else {
		bool ok;
		m_size = parseNumber(text, &ok);
		if (!ok) {
			m_size = 0;
		}
		if (hasGeometry(m_format)) {
			setGeometryFromSize();
		}
	}
	updateOkButton();
}

void NewHdDialog::onGeometryEdited()
{
	if (m_updating) {
		return;
	}
	bool okC, okH, okS, okSS;
	const quint32 c  = parseNumber(m_ui->hddC->currentText(), &okC);
	const quint32 h  = parseNumber(m_ui->hddH->currentText(), &okH);
	const quint32 s  = parseNumber(m_ui->hddS->currentText(), &okS);
	const quint32 ss = parseNumber(m_ui->hddSS->currentText(), &okSS);
	m_c  = okC ? c : 0;
	m_h  = (okH && h <= 0xffff) ? static_cast<quint16>(h) : 0;
	m_s  = (okS && s <= 0xffff) ? static_cast<quint16>(s) : 0;
	m_ss = okSS ? static_cast<quint16>(ss) : 0;
	showSizeFromGeometry();
	updateOkButton();
}

/* SetCHSfromSize of the Windows port (IDE branch) */
void NewHdDialog::setGeometryFromSize()
{
	if (m_size < 4351) {
		m_h = 8;
		m_s = 17;
	} else if (m_size < 32255) {
		m_h = 16;
		m_s = 63;
	} else {
		m_h = 16;
		m_s = 255;
	}
	m_ss = 512;
	m_c = static_cast<quint32>(static_cast<quint64>(m_size) * 1024 * 1024 / m_h / m_s / m_ss);
	showGeometry();
}

/* SetSizefromCHS of the Windows port: a SASI geometry shows as its preset */
void NewHdDialog::showSizeFromGeometry()
{
	m_size = static_cast<quint32>(static_cast<quint64>(m_c) * m_h * m_s * m_ss / 1024 / 1024);
	m_updating = true;
	int preset = -1;
	if (m_allowSasi && m_ss == 256) {
		for (size_t i = 0; i < NELEMENTS(kSasi); i++) {
			if (m_c == kSasi[i].cylinders && m_h == kSasi[i].heads && m_s == kSasi[i].sectors) {
				preset = static_cast<int>(i);
				m_size = kSasi[i].nominalMB;
			}
		}
	}
	if (preset >= 0) {
		m_ui->hddSize->setCurrentIndex(preset);
	} else {
		m_ui->hddSize->setCurrentIndex(-1);
		m_ui->hddSize->setEditText(m_size ? QString::number(m_size) : QString());
	}
	m_updating = false;
}

void NewHdDialog::showGeometry()
{
	m_updating = true;
	m_ui->hddC->setEditText(m_c ? QString::number(m_c) : QString());
	m_ui->hddH->setEditText(m_h ? QString::number(m_h) : QString());
	m_ui->hddS->setEditText(m_s ? QString::number(m_s) : QString());
	m_ui->hddSS->setCurrentText(QString::number(m_ss));
	m_updating = false;
}

bool NewHdDialog::isValid() const
{
	if (!hasGeometry(m_format)) {
		return m_size >= m_minSize && m_size <= m_maxSize;
	}
	/* The core wants at least 1 MB (fdd/newdisk.c) */
	const quint64 bytes = static_cast<quint64>(m_c) * m_h * m_s * m_ss;
	return m_c && m_h && m_s && m_ss
	    && bytes >= 1024 * 1024
	    && bytes / 1024 / 1024 <= m_maxSize;
}

void NewHdDialog::updateOkButton()
{
	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid());
}

void NewHdDialog::accept()
{
	if (!isValid()) {
		return;
	}
	/* Same question as the Windows port: BIOS CHS calls can't reach past
	 * these limits, only LBA can */
	if (hasGeometry(m_format) && (m_c > 65535 || m_h > 16 || m_s > 255)) {
		const auto answer = QMessageBox::question(this, windowTitle(),
		    tr("The geometry is incompatible with CHS access.\nCreate the image anyway?"));
		if (answer != QMessageBox::Yes) {
			return;
		}
	}
	QDialog::accept();
}
