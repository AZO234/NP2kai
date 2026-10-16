/* === preferences: HDD page ===
 * IDE devices only: the images themselves are chosen in the HDD menu.
 */

#ifndef NP2_QT_PREFS_HDDPAGE_H
#define NP2_QT_PREFS_HDDPAGE_H

#include "prefpage.h"

#include <memory>

namespace Ui { class HddPage; }

class HddPage : public PrefPage
{
public:
	explicit HddPage(QWidget *parent = nullptr);
	~HddPage() override;

	void load() override;
	void save() override;

private:
	std::unique_ptr<Ui::HddPage> m_ui;
};

#endif /* NP2_QT_PREFS_HDDPAGE_H */
