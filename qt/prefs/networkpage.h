/* === preferences: Network page === */

#ifndef NP2_QT_PREFS_NETWORKPAGE_H
#define NP2_QT_PREFS_NETWORKPAGE_H

#include "prefpage.h"

#include <memory>

namespace Ui { class NetworkPage; }

class NetworkPage : public PrefPage
{
public:
	explicit NetworkPage(QWidget *parent = nullptr);
	~NetworkPage() override;

	void load() override;
	void save() override;

private:
	std::unique_ptr<Ui::NetworkPage> m_ui;
	int m_shownIrq;   /* an IRQ not on the list is kept unless one is chosen */
};

#endif /* NP2_QT_PREFS_NETWORKPAGE_H */
