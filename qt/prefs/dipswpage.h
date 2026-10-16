/* === preferences: DIP Switches page === */

#ifndef NP2_QT_PREFS_DIPSWPAGE_H
#define NP2_QT_PREFS_DIPSWPAGE_H

#include "prefpage.h"

#include <memory>

namespace Ui { class DipswPage; }

class DipswPage : public PrefPage
{
public:
	explicit DipswPage(QWidget *parent = nullptr);
	~DipswPage() override;

	void load() override;
	void save() override;

private:
	std::unique_ptr<Ui::DipswPage> m_ui;
};

#endif /* NP2_QT_PREFS_DIPSWPAGE_H */
