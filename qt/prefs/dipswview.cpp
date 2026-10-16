/* === preferences: picture of a board's DIP switches and jumpers === */

#include <compiler.h>
#include "dipswview.h"

#include <common/bmpdata.h>

#include <QMouseEvent>
#include <QPainter>

/* The bitmaps draw a switch in 8 or 9 pixels; twice that is easy to hit */
static const int kZoom = 2;

DipSwView::DipSwView(QWidget *parent)
	: QWidget(parent)
{
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	setCursor(Qt::PointingHandCursor);
}

void DipSwView::setBitmap(unsigned char *bmp)
{
	if (!bmp) {
		m_image = QImage();
	} else {
		const auto *file = reinterpret_cast<const BMPFILE *>(bmp);
		const uint size = LOADINTELDWORD(file->bfSize);
		m_image = QImage::fromData(bmp, static_cast<int>(size), "BMP");
		_MFREE(bmp);
	}
	updateGeometry();
	update();
}

QSize DipSwView::sizeHint() const
{
	return m_image.isNull() ? QSize(0, 0) : m_image.size() * kZoom;
}

QSize DipSwView::minimumSizeHint() const
{
	return sizeHint();
}

void DipSwView::paintEvent(QPaintEvent *)
{
	if (m_image.isNull()) {
		return;
	}
	QPainter painter(this);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
	painter.drawImage(QRect(QPoint(0, 0), m_image.size() * kZoom), m_image);
}

void DipSwView::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && !m_image.isNull()) {
		const QPoint pos = event->position().toPoint();
		const QPoint p(pos.x() / kZoom, pos.y() / kZoom);
		if (m_image.rect().contains(p)) {
			Q_EMIT clicked(p);
		}
	}
}
