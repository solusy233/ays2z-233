#include "weather_background.h"

#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QRandomGenerator>

QString WeatherBackground::categoryForWeatherCode(int conditionCode)
{
    if (conditionCode >= 100 && conditionCode < 200)
        return QStringLiteral("sunny");
    if (conditionCode >= 300 && conditionCode < 400)
        return QStringLiteral("rainy");
    if (conditionCode >= 400 && conditionCode < 500)
        return QStringLiteral("snow");
    return QStringLiteral("sunny");
}

QPixmap WeatherBackground::randomBackgroundForWeatherCode(int conditionCode, const QString &applicationDirPath)
{
    const QString category = categoryForWeatherCode(conditionCode);
    const QString folderPath = QDir(applicationDirPath)
                                  .filePath(QStringLiteral("weather/picture/%1").arg(category));
    QDir imageDir(folderPath);
    if (!imageDir.exists()) {
        qWarning() << "天气背景目录不存在:" << folderPath;
        return QPixmap();
    }

    const QStringList imageFiles = imageDir.entryList(QStringList() << QStringLiteral("*.png")
                                                                     << QStringLiteral("*.jpg")
                                                                     << QStringLiteral("*.jpeg")
                                                                     << QStringLiteral("*.bmp"),
                                                     QDir::Files);
    if (imageFiles.isEmpty()) {
        qWarning() << "天气背景目录中没有可用图片:" << folderPath;
        return QPixmap();
    }

    const int selectedIndex = QRandomGenerator::global()->bounded(imageFiles.size());
    const QString backgroundPath = imageDir.filePath(imageFiles.at(selectedIndex));
    return QPixmap(backgroundPath);
}
