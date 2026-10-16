/* === preferences: Serial page === */

#ifndef NP2_QT_PREFS_SERIALPAGE_H
#define NP2_QT_PREFS_SERIALPAGE_H

#include "prefpage.h"

#include <compiler.h>

#include <memory>

namespace Ui { class SerialPage; }

class SerialPage : public PrefPage
{
	Q_OBJECT

public:
	explicit SerialPage(QWidget *parent = nullptr);
	~SerialPage() override;

	void load() override;
	void save() override;

private:
	std::unique_ptr<Ui::SerialPage> m_ui;

	/* PC-9861K switches and jumpers, as on the board */
	UINT8 m_sw[3];
	UINT8 m_jmp[6];

	void set9861(const UINT8 *sw, const UINT8 *jmp);
	UINT8 mode(int channel) const;
	void setMode(int channel, UINT8 value);
	void click9861(const QPoint &pos);
	void updateEnabled();
};

#endif /* NP2_QT_PREFS_SERIALPAGE_H */
