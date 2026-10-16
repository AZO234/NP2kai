/* === new floppy disk image dialog for the Qt port === */

#include <compiler.h>
#include "newfddialog.h"
#include "ui_newfddialog.h"
#include <pccore.h>
#include <codecnv/codecnv.h>
#include <diskimage/fddfile.h>

#include <QFileInfo>

/* Kept for the session, as in the other ports */
static UINT8 s_lastType = DISKTYPE_2HD << 4;

/* D88 fd_name holds 16 bytes plus the terminator */
static const UINT kLabelBytes = 16;

NewFdDialog::NewFdDialog(const QString &fileName, QWidget *parent)
	: QDialog(parent)
	, m_ui(new Ui::NewFdDialog)
{
	m_ui->setupUi(this);
	m_ui->fileName->setText(QFileInfo(fileName).fileName());

	/* As the Windows port: 1.44 MB only when the machine can read it */
	m_ui->make144->setVisible(np2cfg.usefd144 != 0);
	if (s_lastType == (DISKTYPE_2DD << 4)) {
		m_ui->make2DD->setChecked(true);
	} else if (s_lastType == ((DISKTYPE_2HD << 4) + 1) && np2cfg.usefd144) {
		m_ui->make144->setChecked(true);
	} else {
		m_ui->make2HD->setChecked(true);
	}
	m_ui->diskLabel->setFocus();
}

NewFdDialog::~NewFdDialog() = default;

quint8 NewFdDialog::diskType() const
{
	if (m_ui->make2DD->isChecked()) {
		return DISKTYPE_2DD << 4;
	}
	if (m_ui->make144->isChecked()) {
		return (DISKTYPE_2HD << 4) + 1;
	}
	return DISKTYPE_2HD << 4;
}

/* The core converts the label to Shift_JIS into a fixed field and would cut
 * a double-byte character in half; whole characters are dropped instead. */
QByteArray NewFdDialog::diskLabel() const
{
	const QString text = m_ui->diskLabel->text();
	QByteArray result;
	for (int i = 0; i < text.size(); ) {
		const int len = (text.at(i).isHighSurrogate() && i + 1 < text.size()) ? 2 : 1;
		const QByteArray next = result + text.mid(i, len).toUtf8();
		if (codecnv_utf8tosjis(nullptr, 0, next.constData(), static_cast<UINT>(next.size())) > kLabelBytes) {
			break;
		}
		result = next;
		i += len;
	}
	return result;
}

void NewFdDialog::accept()
{
	s_lastType = diskType();
	QDialog::accept();
}
