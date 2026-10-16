/* === new hard disk image dialog for the Qt port ===
 * Size and geometry of a new image, after the Windows port's CNewHddDlg
 * (windows/dialog/d_disk.cpp): a size picks a geometry, an edited geometry
 * gives the size back. Which fields apply depends on the image format.
 */

#ifndef NP2_QT_NEWHDDIALOG_H
#define NP2_QT_NEWHDDIALOG_H

#include <QDialog>

#include <memory>

namespace Ui { class NewHdDialog; }

class NewHdDialog : public QDialog
{
	Q_OBJECT

public:
	enum class Format {
		Nhd,    /* T98-Next */
		Hdi,    /* Anex86 */
		Vhd,    /* Virtual PC */
		Thd,    /* T98 */
		Hdd,    /* Virtual98 */
		Hdn,    /* RaSCSI */
	};

	NewHdDialog(Format format, const QString &fileName, QWidget *parent = nullptr);
	~NewHdDialog() override;

	/* Whether the format takes a geometry (NHD, HDI, VHD) or a size only */
	static bool hasGeometry(Format format);

	quint32 sizeMB() const { return m_size; }
	quint32 cylinders() const { return m_c; }
	quint16 heads() const { return m_h; }
	quint16 sectors() const { return m_s; }
	quint16 sectorSize() const { return m_ss; }
	bool isBlank() const;
	bool isDynamic() const;

	void accept() override;

private:
	std::unique_ptr<Ui::NewHdDialog> m_ui;
	Format  m_format;
	quint32 m_minSize;
	quint32 m_maxSize;
	bool    m_allowSasi;
	bool    m_updating;

	quint32 m_size;
	quint32 m_c;
	quint16 m_h;
	quint16 m_s;
	quint16 m_ss;

	void onSizeEdited(const QString &text);
	void onGeometryEdited();
	void setGeometryFromSize();
	void showSizeFromGeometry();
	void showGeometry();
	void updateOkButton();
	bool isValid() const;
};

#endif /* NP2_QT_NEWHDDIALOG_H */
