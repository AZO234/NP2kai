/* === preferences: one serial channel of the PC-98 ===
 * What the channel is connected to on the host, as wx/commng.cpp reads a
 * COMCFG: port COMPORT_COM1 + n opens the device file in mout of the same
 * channel, COMPORT_MIDI sends it to MIDI.
 */

#ifndef NP2_QT_PREFS_SERIALPORTBOX_H
#define NP2_QT_PREFS_SERIALPORTBOX_H

#include <compiler.h>
#include "np2.h"

#include <QWidget>

#include <memory>

namespace Ui { class SerialPortBox; }

class SerialPortBox : public QWidget
{
	Q_OBJECT

public:
	explicit SerialPortBox(QWidget *parent = nullptr);
	~SerialPortBox() override;

	/* channel: index into np2oscfg.com[] */
	void load(const COMCFG &cfg, int channel);
	void save(COMCFG &cfg, int channel) const;

private:
	std::unique_ptr<Ui::SerialPortBox> m_ui;

	/* What load() showed: a field left alone keeps its bits, even a value
	 * the lists do not offer */
	UINT8 m_param;
	UINT32 m_speed;
	int m_shown[4];

	void updateEnabled();
};

#endif /* NP2_QT_PREFS_SERIALPORTBOX_H */
