/* === new floppy disk image dialog for the Qt port ===
 * Asks for the label and type of a new D88 image, as the Windows port's
 * CNewFddDlg (windows/dialog/d_disk.cpp) does.
 */

#ifndef NP2_QT_NEWFDDIALOG_H
#define NP2_QT_NEWFDDIALOG_H

#include <QDialog>
#include <QByteArray>

#include <memory>

namespace Ui { class NewFdDialog; }

class NewFdDialog : public QDialog
{
	Q_OBJECT

public:
	explicit NewFdDialog(const QString &fileName, QWidget *parent = nullptr);
	~NewFdDialog() override;

	/* fd_type of the D88 header, for newdisk_fdd() */
	quint8 diskType() const;
	/* UTF-8, cut so that its Shift_JIS form fits the 16-byte header field */
	QByteArray diskLabel() const;

	void accept() override;

private:
	std::unique_ptr<Ui::NewFdDialog> m_ui;
};

#endif /* NP2_QT_NEWFDDIALOG_H */
