/* === preferences: Calendar page ===
 * As the Windows port's calendar dialog (windows/dialog/d_clnd.cpp). The core
 * keeps the time as BCD bytes: year, month << 4 | weekday, day, hour, minute,
 * second; the year covers 1980-2079.
 */

#include <compiler.h>
#include "calendarpage.h"
#include "ui_calendarpage.h"

#include <pccore.h>
#include <timemng.h>
#include <calendar.h>

#include <QDateTime>

static int fromBcd(UINT8 b)
{
	return (b >> 4) * 10 + (b & 0x0f);
}

static UINT8 toBcd(int v)
{
	return static_cast<UINT8>(((v / 10) << 4) | (v % 10));
}

static QDateTime fromCalendar(const UINT8 *bcd)
{
	int year = 1900 + fromBcd(bcd[0]);
	if (year < 1980) {
		year += 100;
	}
	return QDateTime(QDate(year, bcd[1] >> 4, fromBcd(bcd[2])),
	                 QTime(fromBcd(bcd[3]), fromBcd(bcd[4]), fromBcd(bcd[5])));
}

static void toCalendar(const QDateTime &dt, UINT8 *bcd)
{
	const QDate d = dt.date();
	const QTime t = dt.time();
	bcd[0] = toBcd(d.year() % 100);
	bcd[1] = static_cast<UINT8>((d.month() << 4) | (d.dayOfWeek() % 7));   /* Sunday is 0 */
	bcd[2] = toBcd(d.day());
	bcd[3] = toBcd(t.hour());
	bcd[4] = toBcd(t.minute());
	bcd[5] = toBcd(t.second());
}

CalendarPage::CalendarPage(QWidget *parent)
	: PrefPage(parent)
	, m_ui(new Ui::CalendarPage)
	, m_timeEdited(false)
{
	m_ui->setupUi(this);
	m_ui->virtualTime->setDateTimeRange(QDateTime(QDate(1980, 1, 1), QTime(0, 0)),
	                                    QDateTime(QDate(2079, 12, 31), QTime(23, 59, 59)));
	connect(m_ui->cal_vir, &QRadioButton::toggled, m_ui->virtualTime, &QWidget::setEnabled);
	connect(m_ui->cal_vir, &QRadioButton::toggled, m_ui->setNow, &QWidget::setEnabled);
	connect(m_ui->virtualTime, &QDateTimeEdit::dateTimeChanged, this, [this]() { m_timeEdited = true; });
	connect(m_ui->setNow, &QPushButton::clicked, this, [this]() {
		UINT8 bcd[6];
		calendar_getreal(bcd);
		m_ui->virtualTime->setDateTime(fromCalendar(bcd));
	});
}

CalendarPage::~CalendarPage() = default;

void CalendarPage::load()
{
	(np2cfg.calendar ? m_ui->cal_real : m_ui->cal_vir)->setChecked(true);
	m_ui->virtualTime->setEnabled(!np2cfg.calendar);
	m_ui->setNow->setEnabled(!np2cfg.calendar);
	UINT8 bcd[6];
	calendar_getvir(bcd);
	m_ui->virtualTime->setDateTime(fromCalendar(bcd));
	m_timeEdited = false;
}

void CalendarPage::save()
{
	const bool wasReal = np2cfg.calendar != 0;
	np2cfg.calendar = m_ui->cal_real->isChecked() ? 1 : 0;
	/* calendar_set() also remembers the distance to the host clock, which is
	 * why the mode is set first */
	if (!np2cfg.calendar && (m_timeEdited || wasReal)) {
		UINT8 bcd[6];
		toCalendar(m_ui->virtualTime->dateTime(), bcd);
		calendar_set(bcd);
	}
	m_timeEdited = false;
}
