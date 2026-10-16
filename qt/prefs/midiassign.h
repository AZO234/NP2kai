/* === preferences: where a MIDI port of the PC-98 goes ===
 * MIDI-OUT, MIDI-IN, the sound module and the MIMPI define file of one COMCFG,
 * as wx/cmmidi.cpp reads them: mout and min name the kind of connection, the
 * device files live apart in np2oscfg.MIDIDEV*.
 */

#ifndef NP2_QT_PREFS_MIDIASSIGN_H
#define NP2_QT_PREFS_MIDIASSIGN_H

#include <compiler.h>
#include "np2.h"

#include <QWidget>

#include <memory>

namespace Ui { class MidiAssign; }

class MidiAssign : public QWidget
{
	Q_OBJECT

public:
	explicit MidiAssign(QWidget *parent = nullptr);
	~MidiAssign() override;

	/* The serial ports have no device files of their own */
	void setDeviceFieldsVisible(bool visible);

	/* outDevice and inDevice may be null when the fields are hidden */
	void load(const COMCFG &cfg, const char *outDevice, const char *inDevice);
	void save(COMCFG &cfg, char *outDevice, char *inDevice) const;

private:
	std::unique_ptr<Ui::MidiAssign> m_ui;
	bool m_deviceFields;

	void updateEnabled();
};

#endif /* NP2_QT_PREFS_MIDIASSIGN_H */
