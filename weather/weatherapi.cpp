#include "weatherapi.h"

#include <QCoreApplication>
#include <QDate>
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
    , fetchTime(21, 0)
    , lastFetchDate()
    , apiKey(qEnvironmentVariable("QWEATHER_API_KEY",
                                  "678996b6ca2142af8944f8ec259af633"))
{
    dailyFetchTimer->setInterval(60000);
    connect(dailyFetchTimer, &QTimer::timeout, this, &WeatherApi::checkDailyFetchSchedule);
}

void WeatherApi::startDailyFetchSchedule(const QTime &newFetchTime)
{
    fetchTime = newFetchTime.isValid() ? newFetchTime : QTime(21, 0);
    dailyFetchTimer->start();
}

/**
 * @brief 检查每日天气数据获取的定时任务
 * 该函数用于检查当前时间是否到达预设的天气数据获取时间
 * 如果到达时间且当天尚未获取数据，则触发数据获取
 */
void WeatherApi::checkDailyFetchSchedule()
{
    // 获取当前日期和时间
    const QDate today = QDate::currentDate();
    const QTime now = QTime::currentTime();
    // 如果已经获取过今天的数据，则直接返回
    if (lastFetchDate == today)
        return;

    // 检查当前时间是否在获取时间前后一秒内
    if (now >= fetchTime && now < fetchTime.addSecs(60)) {
        // 更新最后获取日期为今天
        lastFetchDate = today;
        // 获取安阳天气数据
        fetchAnyangWeather();
    }
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