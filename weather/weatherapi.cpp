#include "weatherapi.h"
#include "weather_schedule.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

WeatherApi::WeatherApi(QObject *parent)
    : QObject(parent)
    , networkManager(new QNetworkAccessManager(this))
    , dailyFetchTimer(new QTimer(this))
    , explicitStartTime()
    , lastFetchCycleDate()
    , fallbackWarned(false)
    , apiKey(qEnvironmentVariable("QWEATHER_API_KEY",
                                  "678996b6ca2142af8944f8ec259af633"))
{
    dailyFetchTimer->setInterval(60000);
    connect(dailyFetchTimer, &QTimer::timeout, this, &WeatherApi::checkDailyFetchSchedule);
}

void WeatherApi::startDailyFetchSchedule(const QTime &newStartTime)
{
    explicitStartTime = newStartTime;
    dailyFetchTimer->start();
}

QString WeatherApi::settingsFilePath() const
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("settings.json"));
}

/**
 * @brief 检查是否进入当天的天气抓取窗口
 *
 * 抓取窗口为"开始时间前 3 分钟 ~ 开始时间"，开始时间取自调用方指定值，未指定时
 * 每分钟重新读一次 settings.json 的 "startTime"（改时间无需重启）。窗口内每分钟
 * 都会判断一次，因此定时器抖动不会整天空过；窗口外（含启动时已过开始时间）
 * 不补抓，与 AutoPoweroff"错过不补播"保持一致。
 */
void WeatherApi::checkDailyFetchSchedule()
{
    QTime startTime = explicitStartTime;
    if (!startTime.isValid()) {
        startTime = WeatherSchedule::startTimeFromSettings(settingsFilePath());
        if (!startTime.isValid()) {
            startTime = WeatherSchedule::fallbackStartTime();
            if (!fallbackWarned) {
                fallbackWarned = true;
                qWarning() << "settings.json 中没有可用的 startTime，天气抓取时刻回退到"
                           << startTime.toString(QStringLiteral("HH:mm"));
            }
        }
    }

    const QDateTime scheduled = WeatherSchedule::fetchWindowScheduledAt(QDateTime::currentDateTime(), startTime);
    if (!scheduled.isValid())
        return;

    // 同一周期（同一次到点）只抓一次；到点那天就是周期日期，跨午夜的窗口不会重复抓取
    const QDate cycleDate = scheduled.date();
    if (lastFetchCycleDate == cycleDate)
        return;

    lastFetchCycleDate = cycleDate;
    qWarning() << "进入天气抓取窗口（到点:" << scheduled.toString(QStringLiteral("yyyy-MM-dd HH:mm"))
               << "，提前" << WeatherSchedule::kFetchLeadSeconds << "秒）";
    fetchAnyangWeather();
}

void WeatherApi::setApiKey(const QString &newApiKey)
{
    apiKey = newApiKey.trimmed();
}

void WeatherApi::fetchAnyangWeather()
{
    if (apiKey.isEmpty()) {
        emit requestFailed(QStringLiteral("未设置和风天气 API Key，请调用 setApiKey() 或设置 QWEATHER_API_KEY 环境变量"));
        return;
    }

    QUrl url(QStringLiteral("https://m776xb9kn6.re.qweatherapi.com/v7/weather/3d"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("location"), QStringLiteral("101180201"));
    query.addQueryItem(QStringLiteral("key"), apiKey);
    url.setQuery(query);

    QNetworkReply *reply = networkManager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray response = reply->readAll();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        qWarning() << "天气请求 HTTP 状态:" << statusCode
                   << "网络错误:" << reply->errorString();

        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(reply->errorString());
            reply->deleteLater();
            return;
        }

        if (statusCode < 200 || statusCode >= 300) {
            emit requestFailed(QStringLiteral("天气服务返回 HTTP %1").arg(statusCode));
            reply->deleteLater();
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            emit requestFailed(QStringLiteral("天气响应不是有效的 JSON"));
            reply->deleteLater();
            return;
        }

        const QJsonObject responseObject = document.object();
        const QString responseCode = responseObject.value(QStringLiteral("code")).toString();
        if (responseCode != QStringLiteral("200")) {
            emit requestFailed(QStringLiteral("和风天气请求失败，错误码：%1").arg(responseCode));
            reply->deleteLater();
            return;
        }

        const QJsonArray daily = responseObject.value(QStringLiteral("daily")).toArray();
        if (daily.size() < 2 || !daily.at(1).isObject()) {
            emit requestFailed(QStringLiteral("天气响应缺少明日预报数据"));
            reply->deleteLater();
            return;
        }

        const QJsonObject tomorrow = daily.at(1).toObject();

        QJsonObject tomorrowResponse;
        tomorrowResponse.insert(QStringLiteral("code"), responseObject.value(QStringLiteral("code")));
        tomorrowResponse.insert(QStringLiteral("updateTime"), responseObject.value(QStringLiteral("updateTime")));
        tomorrowResponse.insert(QStringLiteral("fxLink"), responseObject.value(QStringLiteral("fxLink")));
        tomorrowResponse.insert(QStringLiteral("daily"), QJsonArray { tomorrow });
        tomorrowResponse.insert(QStringLiteral("refer"), responseObject.value(QStringLiteral("refer")));

        const QString weatherDirectory = QDir(QCoreApplication::applicationDirPath())
                                             .filePath(QStringLiteral("weather"));
        if (!QDir().mkpath(weatherDirectory)) {
            emit requestFailed(QStringLiteral("无法创建天气 JSON 保存目录"));
            reply->deleteLater();
            return;
        }

        const QByteArray tomorrowResponseData = QJsonDocument(tomorrowResponse).toJson(QJsonDocument::Compact);
        QSaveFile jsonFile(QDir(weatherDirectory).filePath(QStringLiteral("weather.json")));
        if (!jsonFile.open(QIODevice::WriteOnly)
            || jsonFile.write(tomorrowResponseData) != tomorrowResponseData.size()
            || !jsonFile.commit()) {
            emit requestFailed(QStringLiteral("无法保存天气 JSON 文件：%1").arg(jsonFile.errorString()));
            reply->deleteLater();
            return;
        }

        qWarning() << "安阳市明日天气 JSON 已保存到:" << jsonFile.fileName();

        WeatherData weather;
        weather.temperature = tomorrow.value(QStringLiteral("tempMax")).toString().toDouble();
        weather.apparentTemperature = tomorrow.value(QStringLiteral("tempMin")).toString().toDouble();
        weather.weatherCode = tomorrow.value(QStringLiteral("iconDay")).toString().toInt();
        weather.windSpeed = tomorrow.value(QStringLiteral("windSpeedDay")).toString().toDouble();
        weather.time = tomorrow.value(QStringLiteral("fxDate")).toString();
        emit weatherReady(weather);

        reply->deleteLater();
    });
}