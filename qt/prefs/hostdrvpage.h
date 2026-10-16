/* === preferences: Host Drive page === */

#ifndef NP2_QT_PREFS_HOSTDRVPAGE_H
#define NP2_QT_PREFS_HOSTDRVPAGE_H

#include "prefpage.h"

#include <memory>

namespace Ui { class HostdrvPage; }

class HostdrvPage : public PrefPage
{
	Q_OBJECT

public:
	explicit HostdrvPage(QWidget *parent = nullptr);
	~HostdrvPage() override;

	void load() override;
	void save() override;

private:
	std::unique_ptr<Ui::HostdrvPage> m_ui;
};

#endif /* NP2_QT_PREFS_HOSTDRVPAGE_H */
