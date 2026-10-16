/* === preferences: picture of a board's DIP switches and jumpers ===
 * Shows a bitmap from generic/dipswbmp.c, enlarged without smoothing so the
 * switches stay sharp, and reports clicks in the bitmap's own pixels, so a page
 * can flip the switch under the pointer as the Windows port does.
 */

#ifndef NP2_QT_PREFS_DIPSWVIEW_H
#define NP2_QT_PREFS_DIPSWVIEW_H

#include <QImage>
#include <QWidget>

class DipSwView : public QWidget
{
	Q_OBJECT

public:
	explicit DipSwView(QWidget *parent = nullptr);

	/* Takes a BMP file from dipswbmp_get*() and frees it */
	void setBitmap(unsigned char *bmp);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;

Q_SIGNALS:
	void clicked(const QPoint &pixel);

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;

private:
	QImage m_image;
};

#endif /* NP2_QT_PREFS_DIPSWVIEW_H */
