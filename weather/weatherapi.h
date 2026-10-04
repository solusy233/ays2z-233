#ifndef WEATHERAPI_H
#define WEATHERAPI_H

#include <QDate>
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

    /**
     * @brief 启动每日天气抓取调度
     *
     * 每天在"开始时间"前 WeatherSchedule::kFetchLeadSeconds 秒（3 分钟）抓取一次
     * 明日预报，保证到点播放提示音时 weather/weather.json 是新鲜数据。
     *
     * @param startTime 显式指定开始时间；传入无效 QTime（默认）表示自动模式：
     *                  每次检查都重新读 <应用目录>/settings.json 的 "startTime"，
     *                  因此设置界面改时间后无需重启即可生效；
     *                  settings.json 里读不到可用值时回退 21:00。
     */
    void startDailyFetchSchedule(const QTime &startTime = QTime());

private slots:
    void checkDailyFetchSchedule();

signals:
    void weatherReady(const WeatherData &weather);
    void requestFailed(const QString &message);

private:
    /// settings.json 的绝对路径（应用目录下）
    QString settingsFilePath() const;

    QNetworkAccessManager *networkManager;
    QTimer *dailyFetchTimer;
    QTime explicitStartTime;   // 有效 = 调用方显式指定；无效 = 每次检查读 settings.json
    QDate lastFetchCycleDate;  // 已抓取过的周期日期（= 该周期 startTime 那天）
    bool fallbackWarned;       // 是否已提示过"settings.json 无有效 startTime"
    QString apiKey;
};

Q_DECLARE_METATYPE(WeatherData)

#endif // WEATHERAPI_H