/* === cycle screenshot settings for the Qt port ===
 * The file and interval the wx port keeps on its Misc tab; asked for when the
 * cycle screenshot is switched on.
 */

#ifndef NP2_QT_CYCLESHOTDIALOG_H
#define NP2_QT_CYCLESHOTDIALOG_H

#include <QDialog>

#include <memory>

namespace Ui { class CycleShotDialog; }

class CycleShotDialog : public QDialog
{
	Q_OBJECT

public:
	explicit CycleShotDialog(QWidget *parent = nullptr);
	~CycleShotDialog() override;

	void accept() override;

private:
	std::unique_ptr<Ui::CycleShotDialog> m_ui;

	void browse();
};

#endif /* NP2_QT_CYCLESHOTDIALOG_H */
