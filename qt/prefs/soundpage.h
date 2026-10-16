/* === preferences: Sound page === */

#ifndef NP2_QT_PREFS_SOUNDPAGE_H
#define NP2_QT_PREFS_SOUNDPAGE_H

#include "prefpage.h"

#include <compiler.h>

#include <memory>

namespace Ui { class SoundPage; }

class SoundPage : public PrefPage
{
	Q_OBJECT

public:
	explicit SoundPage(QWidget *parent = nullptr);
	~SoundPage() override;

	void load() override;
	void save() override;

	/* Room for every board tab, which the minimum size does not ask for */
	QSize sizeHint() const override;

private:
	std::unique_ptr<Ui::SoundPage> m_ui;

	/* The jumper boards are edited as one byte, as on the real board: the
	 * combo boxes and the picture change bits of it. */
	UINT8 m_snd26;
	UINT8 m_snd86;
	UINT8 m_spb;
	UINT8 m_spbvr;

	void set26(UINT8 value);
	void set86(UINT8 value);
	void setSpb(UINT8 value, UINT8 vr);
	void set118(UINT16 io, UINT8 id, UINT8 dma, UINT8 irqf, UINT8 irqp, UINT8 irqm, UINT8 rom);
	void setWss(UINT8 id, UINT8 dma, UINT8 irq);
	void setWaveStar(UINT16 io, UINT8 dma, UINT8 irq, UINT8 pnp, UINT8 pcm);
	void setSb16(UINT8 io, UINT8 dma, UINT8 irq);
	void update118Jumper();
	void click26(const QPoint &p);
	void click86(const QPoint &p);
	void clickSpb(const QPoint &p);
	void showBoardTab(int board);
	void fillJoypads();
};

#endif /* NP2_QT_PREFS_SOUNDPAGE_H */
