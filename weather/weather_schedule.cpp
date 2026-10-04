#include "weather_schedule.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace WeatherSchedule {

QTime fallbackStartTime()
{
    return QTime(21, 0);
}

QTime startTimeFromSettings(const QString &settingsPath)
{
    QFile file(settingsPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QTime();

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
        return QTime();

    const QString text = document.object().value(QStringLiteral("startTime")).toString().trimmed();
    QTime startTime = QTime::fromString(text, QStringLiteral("HH:mm"));
    if (!startTime.isValid())
        startTime = QTime::fromString(text, QStringLiteral("HH:mm:ss"));
    return startTime;
}

QDateTime fetchWindowScheduledAt(const QDateTime &now, const QTime &startTime)
{
    if (!now.isValid() || !startTime.isValid())
        return QDateTime();

    // 本次周期的到点时刻：今天的那一次已经过了就看明天那一次
    QDateTime scheduled(now.date(), startTime);
    if (scheduled < now)
        scheduled = scheduled.addDays(1);

    const QDateTime fetchAt = scheduled.addSecs(-kFetchLeadSeconds);
    if (now < fetchAt || now > scheduled)
        return QDateTime();

    return scheduled;
}

} // namespace WeatherSchedule
