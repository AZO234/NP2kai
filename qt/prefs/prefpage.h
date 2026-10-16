/* === preferences: one page of the dialog ===
 * A page shows part of np2cfg/np2oscfg. load() fills the controls from the
 * configuration and save() writes them back; nothing changes until save(). The
 * controls carry the objectName of the matching control in the wx dialog
 * (wx/dialog/prefframe.cpp), so a change there can be found here by name; a
 * name that is also a compiler macro takes the configuration key instead.
 */

#ifndef NP2_QT_PREFS_PREFPAGE_H
#define NP2_QT_PREFS_PREFPAGE_H

#include <QComboBox>
#include <QLabel>
#include <QSlider>
#include <QWidget>

#include <cstddef>

class PrefPage : public QWidget
{
public:
	using QWidget::QWidget;

	virtual void load() = 0;
	virtual void save() = 0;
};

/* Combo boxes list their items in the .ui; the values behind them live in an
 * array of the same order next to the page code. */
template <typename T, std::size_t N>
void setComboValue(QComboBox *combo, const T (&values)[N], unsigned value, int fallback = 0)
{
	for (std::size_t i = 0; i < N; i++) {
		if (static_cast<unsigned>(values[i]) == value) {
			combo->setCurrentIndex(static_cast<int>(i));
			return;
		}
	}
	combo->setCurrentIndex(fallback);
}

template <typename T, std::size_t N>
T comboValue(const QComboBox *combo, const T (&values)[N])
{
	const int i = combo->currentIndex();
	return (i >= 0 && static_cast<std::size_t>(i) < N) ? values[i] : values[0];
}

/* A slider and the label next to it showing its value */
inline void showSliderValue(QSlider *slider, QLabel *label, const QString &format = QStringLiteral("%1"))
{
	QObject::connect(slider, &QSlider::valueChanged, label, [label, format](int value) {
		label->setText(format.arg(value));
	});
	label->setText(format.arg(slider->value()));
}

#endif /* NP2_QT_PREFS_PREFPAGE_H */
