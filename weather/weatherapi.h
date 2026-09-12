#ifndef WEATHERAPI_H
#define WEATHERAPI_H

#include <QObject>
#include <QTime>
#include <QString>

class QNetworkAccessManager;
class QTimer;

struct WeatherData
{
    double temperature = 0.0;
    double apparentTemperature = 0.0;
    int relativeHumidity = 0;
    int weatherCode = 0;
    double windSpeed = 0.0;
    QString time;
};

class WeatherApi : public QObject
{
    Q_OBJECT

public:
    explicit WeatherApi(QObject *parent = nullptr);

    void setApiKey(const QString &apiKey);
    void fetchAnyangWeather();
    void startDailyFetchSchedule(const QTime &fetchTime = QTime(21, 0));

private slots:
    void checkDailyFetchSchedule();

signals:
    void weatherReady(const WeatherData &weather);
    void requestFailed(const QString &message);

private:
    QNetworkAccessManager *networkManager;
    QTimer *dailyFetchTimer;
    QTime fetchTime;
    QDate lastFetchDate;
    QString apiKey;
};

Q_DECLARE_METATYPE(WeatherData)

#endif // WEATHERAPI_H