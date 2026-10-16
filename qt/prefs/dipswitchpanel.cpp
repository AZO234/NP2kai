/* === preferences: the DIP switches of the PC-98 itself === */

#include "dipswitchpanel.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

#include <cstring>

/* Logical pixels; the font decides the rest */
static const int kSwitchW = 18;
static const int kSwitchH = 40;
static const int kGap = 2;
static const int kBankGap = 24;

DipSwitchPanel::DipSwitchPanel(QWidget *parent)
	: QWidget(parent)
	, m_focus(0)
{
	std::memset(m_values, 0, sizeof(m_values));
	setFocusPolicy(Qt::StrongFocus);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void DipSwitchPanel::setValues(const unsigned char *values)
{
	std::memcpy(m_values, values, sizeof(m_values));
	update();
}

void DipSwitchPanel::values(unsigned char *values) const
{
	std::memcpy(values, m_values, sizeof(m_values));
}

/* Title line, ON label, the switches, then the numbers */
QRect DipSwitchPanel::switchRect(int index) const
{
	const int line = fontMetrics().height();
	const int bank = index / 8;
	const int bit = index % 8;
	const int bankW = 8 * kSwitchW + 7 * kGap;
	const int x = line * 2 + bank * (bankW + kBankGap) + bit * (kSwitchW + kGap);
	return QRect(x, line * 2, kSwitchW, kSwitchH);
}

QSize DipSwitchPanel::sizeHint() const
{
	const int line = fontMetrics().height();
	const QRect last = switchRect(23);
	return QSize(last.right() + line, last.bottom() + line * 2);
}

void DipSwitchPanel::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	const QPalette pal = palette();
	const int line = fontMetrics().height();

	for (int bank = 0; bank < 3; bank++) {
		const QRect first = switchRect(bank * 8);
		const QRect last = switchRect(bank * 8 + 7);
		const QRect body = QRect(first.topLeft(), last.bottomRight()).adjusted(-4, -4, 4, 4);
		p.setPen(Qt::NoPen);
		p.setBrush(QColor(0xc0, 0x30, 0x30));
		p.drawRoundedRect(body, 3, 3);
		p.setPen(pal.color(QPalette::WindowText));
		p.drawText(QRect(body.left(), 0, body.width(), line), Qt::AlignCenter, tr("SW%1").arg(bank + 1));
		p.drawText(QRect(0, first.top(), line * 2 - 4, line), Qt::AlignRight | Qt::AlignVCenter,
		           bank == 0 ? tr("ON") : QString());
		for (int bit = 0; bit < 8; bit++) {
			const int index = bank * 8 + bit;
			const QRect r = switchRect(index);
			/* A cleared bit is a switch turned ON (bios.c, gdc.c) */
			const bool on = !((m_values[bank] >> bit) & 1);
			p.setPen(Qt::NoPen);
			p.setBrush(QColor(0x30, 0x10, 0x10));
			p.drawRect(r.adjusted(2, 0, -2, 0));
			p.setBrush(QColor(0xf0, 0xf0, 0xf0));
			const int knob = r.height() / 2 - 2;
			p.drawRect(QRect(r.left() + 3, on ? r.top() + 2 : r.bottom() - 1 - knob, r.width() - 6, knob));
			if (hasFocus() && index == m_focus) {
				p.setPen(QPen(pal.color(QPalette::Highlight), 2));
				p.setBrush(Qt::NoBrush);
				p.drawRect(r.adjusted(-1, -1, 1, 1));
			}
			p.setPen(pal.color(QPalette::WindowText));
			p.drawText(QRect(r.left() - 4, body.bottom() + 2, r.width() + 8, line), Qt::AlignCenter,
			           QString::number(bit + 1));
		}
	}
}

void DipSwitchPanel::toggle(int index)
{
	m_values[index / 8] ^= static_cast<unsigned char>(1 << (index % 8));
	m_focus = index;
	update();
}

void DipSwitchPanel::mousePressEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton) {
		return;
	}
	for (int i = 0; i < 24; i++) {
		if (switchRect(i).adjusted(-1, -4, 1, 4).contains(event->position().toPoint())) {
			toggle(i);
			return;
		}
	}
}

void DipSwitchPanel::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
	case Qt::Key_Left:
		m_focus = (m_focus + 23) % 24;
		update();
		break;
	case Qt::Key_Right:
		m_focus = (m_focus + 1) % 24;
		update();
		break;
	case Qt::Key_Space:
		toggle(m_focus);
		break;
	default:
		QWidget::keyPressEvent(event);
	}
}
