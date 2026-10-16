/* === preferences: Network page ===
 * As the Windows port's network options (windows/dialog/d_network.cpp); the
 * interrupt is stored as the IRQ number, which the wx tab got wrong.
 */

#include <compiler.h>
#include "networkpage.h"
#include "ui_networkpage.h"

#include <pccore.h>
#include <common/milstr.h>

#include <QDir>

#include <algorithm>

#if defined(SUPPORT_LGY98)
static const UINT16 kLgy98Io[] = { 0x00d0, 0x10d0, 0x20d0, 0x30d0, 0x40d0, 0x50d0, 0x60d0, 0x70d0 };
static const UINT8 kLgy98Irq[] = { 3, 5, 6, 12 };
#endif

NetworkPage::NetworkPage(QWidget *parent)
	: PrefPage(parent)
	, m_ui(new Ui::NetworkPage)
	, m_shownIrq(-1)
{
	m_ui->setupUi(this);
#if defined(SUPPORT_NET)
	const QDir net(QStringLiteral("/sys/class/net"));
	m_ui->NP2NETTAP->addItems(net.entryList({ QStringLiteral("tap*") }, QDir::Dirs | QDir::System, QDir::Name));
#else
	m_ui->tapGroup->hide();
#endif
#if !defined(SUPPORT_LGY98)
	m_ui->USELGY98->hide();
#endif
}

NetworkPage::~NetworkPage() = default;

void NetworkPage::load()
{
#if defined(SUPPORT_NET)
	m_ui->NP2NETTAP->setCurrentText(QString::fromUtf8(np2cfg.np2nettap));
	m_ui->NP2NETPMM->setChecked(np2cfg.np2netpmm != 0);
#endif
#if defined(SUPPORT_LGY98)
	m_ui->USELGY98->setChecked(np2cfg.uselgy98 != 0);
	setComboValue(m_ui->LGY98_IO, kLgy98Io, np2cfg.lgy98io, 1);
	setComboValue(m_ui->LGY98IRQ, kLgy98Irq, np2cfg.lgy98irq);
	m_shownIrq = m_ui->LGY98IRQ->currentIndex();
	QStringList mac;
	for (UINT8 b : np2cfg.lgy98mac) {
		mac << QStringLiteral("%1").arg(b, 2, 16, QLatin1Char('0')).toUpper();
	}
	m_ui->LGY98MAC->setText(mac.join(QLatin1Char(':')));
#endif
}

void NetworkPage::save()
{
#if defined(SUPPORT_NET)
	milstr_ncpy(np2cfg.np2nettap, m_ui->NP2NETTAP->currentText().trimmed().toUtf8().constData(),
	            NELEMENTS(np2cfg.np2nettap));
	np2cfg.np2netpmm = m_ui->NP2NETPMM->isChecked() ? 1 : 0;
#endif
#if defined(SUPPORT_LGY98)
	np2cfg.uselgy98 = m_ui->USELGY98->isChecked() ? 1 : 0;
	np2cfg.lgy98io = comboValue(m_ui->LGY98_IO, kLgy98Io);
	if (m_ui->LGY98IRQ->currentIndex() != m_shownIrq) {
		np2cfg.lgy98irq = comboValue(m_ui->LGY98IRQ, kLgy98Irq);
	}
	/* An incomplete address keeps the old one */
	const QStringList mac = m_ui->LGY98MAC->text().split(QLatin1Char(':'));
	bool ok = (mac.size() == 6);
	UINT8 bytes[6];
	for (int i = 0; ok && i < 6; i++) {
		bytes[i] = static_cast<UINT8>(mac[i].toUInt(&ok, 16));
		ok = ok && mac[i].size() == 2;
	}
	if (ok) {
		std::copy(bytes, bytes + 6, np2cfg.lgy98mac);
	}
#endif
}
