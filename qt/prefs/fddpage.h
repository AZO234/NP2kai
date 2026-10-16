/* === preferences: FDD page === */

#ifndef NP2_QT_PREFS_FDDPAGE_H
#define NP2_QT_PREFS_FDDPAGE_H

#include "prefpage.h"

#include <memory>

namespace Ui { class FddPage; }

class FddPage : public PrefPage
{
public:
	explicit FddPage(QWidget *parent = nullptr);
	~FddPage() override;

	void load() override;
	void save() override;

private:
	std::unique_ptr<Ui::FddPage> m_ui;
};

#endif /* NP2_QT_PREFS_FDDPAGE_H */
