/* === preferences: the DIP switches of the PC-98 itself ===
 * Three banks of eight, drawn as on the machine: ON at the top, switch 1 on
 * the left. A click or Space flips the switch under the pointer or the focus.
 */

#ifndef NP2_QT_PREFS_DIPSWITCHPANEL_H
#define NP2_QT_PREFS_DIPSWITCHPANEL_H

#include <QWidget>

class DipSwitchPanel : public QWidget
{
	Q_OBJECT

public:
	explicit DipSwitchPanel(QWidget *parent = nullptr);

	/* 3 bytes as in np2cfg.dipsw: bit 0 is switch 1, a cleared bit means ON */
	void setValues(const unsigned char *values);
	void values(unsigned char *values) const;

	QSize sizeHint() const override;

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;

private:
	unsigned char m_values[3];
	int m_focus;   /* 0-23 */

	QRect switchRect(int index) const;
	void toggle(int index);
};

#endif /* NP2_QT_PREFS_DIPSWITCHPANEL_H */
