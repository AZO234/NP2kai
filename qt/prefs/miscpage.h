/* === preferences: Misc page === */

#ifndef NP2_QT_PREFS_MISCPAGE_H
#define NP2_QT_PREFS_MISCPAGE_H

#include "prefpage.h"

#include <memory>

namespace Ui { class MiscPage; }

class MiscPage : public PrefPage
{
	Q_OBJECT

public:
	explicit MiscPage(QWidget *parent = nullptr);
	~MiscPage() override;

	void load() override;
	void save() override;

private:
	std::unique_ptr<Ui::MiscPage> m_ui;

	void browseFont();
};

#endif /* NP2_QT_PREFS_MISCPAGE_H */
