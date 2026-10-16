/* === emulated screen for the Qt port ===
 * Shows the frame from scrnmng and feeds keyboard and mouse to the core.
 */

#ifndef NP2_QT_SCREENWIDGET_H
#define NP2_QT_SCREENWIDGET_H

#include <QImage>
#include <QWidget>

class PointerCapture;

class ScreenWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ScreenWidget(QWidget *parent = nullptr);

	/* Copy the current frame from scrnmng; GUI thread only */
	void updateFrame();

	PointerCapture *pointerCapture() const { return m_capture; }

	/* The frame as shown (rotation applied), for screenshots */
	QImage frame() const;
	/* The PC-98 screen as the emulator draws it, before rotation */
	QSize frameSize() const { return m_image.size(); }

	/* A PC-98 screen of that size as shown at x1: rotated, and stretched to
	 * 4:3 when that is on */
	static QSize displaySizeFor(const QSize &frame);
	QSize displaySize() const { return displaySizeFor(m_image.size()); }

	QSize sizeHint() const override;

Q_SIGNALS:
	void fullScreenToggleRequested();
	void menuBarToggleRequested();
	/* The PC-98 changed its resolution, e.g. 640x400 to 640x480; previous is
	 * invalid for the first frame */
	void frameSizeChanged(const QSize &previous);
	void mouseCaptureChanged(bool captured);

protected:
	bool event(QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void keyReleaseEvent(QKeyEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void focusOutEvent(QFocusEvent *event) override;
	bool focusNextPrevChild(bool next) override;

private:
	QImage          m_image;
	PointerCapture *m_capture;
	bool            m_captureClick;     /* the left button press only captured */

	bool handleHotkey(QKeyEvent *event, bool pressed);
};

#endif /* NP2_QT_SCREENWIDGET_H */
