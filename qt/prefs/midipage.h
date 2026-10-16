/* === preferences: MIDI page === */

#ifndef NP2_QT_PREFS_MIDIPAGE_H
#define NP2_QT_PREFS_MIDIPAGE_H

#include "prefpage.h"

#include <compiler.h>

#include <memory>

namespace Ui { class MidiPage; }

class MidiPage : public PrefPage
{
public:
	explicit MidiPage(QWidget *parent = nullptr);
	~MidiPage() override;

	void load() override;
	void save() override;

private:
	std::unique_ptr<Ui::MidiPage> m_ui;

	/* Port and interrupt of each board as one byte, as on its DIP switch */
	UINT8 m_mpu;
	UINT8 m_smpu;

	void setMpu(UINT8 value);
	void setSmpu(UINT8 value);
};

#endif /* NP2_QT_PREFS_MIDIPAGE_H */
