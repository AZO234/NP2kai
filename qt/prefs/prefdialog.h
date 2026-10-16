/* === preferences dialog for the Qt port ===
 * The tabs of wx/dialog/prefframe.cpp, one PrefPage with its own .ui each.
 * The emulation is stopped while it is open. OK writes every page back, applies
 * what can change on a running machine and saves the configuration; the rest
 * waits for a reset, which the caller offers when needsReset() says so.
 */

#ifndef NP2_QT_PREFS_PREFDIALOG_H
#define NP2_QT_PREFS_PREFDIALOG_H

#include <QDialog>
#include <QList>

#include <memory>

namespace Ui { class PrefDialog; }
class PrefPage;

class PrefDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PrefDialog(QWidget *parent = nullptr);
	~PrefDialog() override;

	void accept() override;

	/* After OK: settings that only a reset of the PC-98 brings in changed */
	bool needsReset() const { return m_needsReset; }
	/* After OK: settings read only when the program starts changed */
	bool needsRestart() const { return m_needsRestart; }

	/* Pushes np2cfg's volumes into the sound generators */
	static void applyVolumes();

private:
	std::unique_ptr<Ui::PrefDialog> m_ui;
	QList<PrefPage *> m_pages;
	bool m_needsReset;
	bool m_needsRestart;

	void addPage(PrefPage *page, const QString &title);
	void restoreDefaults();
};

#endif /* NP2_QT_PREFS_PREFDIALOG_H */
