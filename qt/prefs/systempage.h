/* === preferences: System page === */

#ifndef NP2_QT_PREFS_SYSTEMPAGE_H
#define NP2_QT_PREFS_SYSTEMPAGE_H

#include "prefpage.h"

#include <memory>

namespace Ui { class SystemPage; }

class SystemPage : public PrefPage
{
	Q_OBJECT

public:
	explicit SystemPage(QWidget *parent = nullptr);
	~SystemPage() override;

	void load() override;
	void save() override;

private:
	std::unique_ptr<Ui::SystemPage> m_ui;

	void updateMHz();
};

#endif /* NP2_QT_PREFS_SYSTEMPAGE_H */
