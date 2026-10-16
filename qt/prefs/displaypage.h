/* === preferences: Display page === */

#ifndef NP2_QT_PREFS_DISPLAYPAGE_H
#define NP2_QT_PREFS_DISPLAYPAGE_H

#include "prefpage.h"

#include <memory>

namespace Ui { class DisplayPage; }

class DisplayPage : public PrefPage
{
public:
	explicit DisplayPage(QWidget *parent = nullptr);
	~DisplayPage() override;

	void load() override;
	void save() override;

private:
	std::unique_ptr<Ui::DisplayPage> m_ui;
};

#endif /* NP2_QT_PREFS_DISPLAYPAGE_H */
