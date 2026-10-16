/* === preferences: Calendar page === */

#ifndef NP2_QT_PREFS_CALENDARPAGE_H
#define NP2_QT_PREFS_CALENDARPAGE_H

#include "prefpage.h"

#include <memory>

namespace Ui { class CalendarPage; }

class CalendarPage : public PrefPage
{
public:
	explicit CalendarPage(QWidget *parent = nullptr);
	~CalendarPage() override;

	void load() override;
	void save() override;

private:
	std::unique_ptr<Ui::CalendarPage> m_ui;
	bool m_timeEdited;   /* the running virtual clock is only set when asked to */
};

#endif /* NP2_QT_PREFS_CALENDARPAGE_H */
