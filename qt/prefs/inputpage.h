/* === preferences: Input page === */

#ifndef NP2_QT_PREFS_INPUTPAGE_H
#define NP2_QT_PREFS_INPUTPAGE_H

#include "prefpage.h"

#include <compiler.h>

#include <memory>

namespace Ui { class InputPage; }

class InputPage : public PrefPage
{
	Q_OBJECT

public:
	explicit InputPage(QWidget *parent = nullptr);
	~InputPage() override;

	void load() override;
	void save() override;

private:
	std::unique_ptr<Ui::InputPage> m_ui;

	/* MOUSEMUL/MOUSEDIV as read; a fraction the slider doesn't offer stays
	 * unless the slider is moved */
	SINT16 m_mousemul;
	SINT16 m_mousediv;
	bool   m_mouseMoved;

	void showMouseSpeed(int index);
};

#endif /* NP2_QT_PREFS_INPUTPAGE_H */
