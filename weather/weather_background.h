#ifndef WEATHER_BACKGROUND_H
#define WEATHER_BACKGROUND_H

#include <QPixmap>
#include <QString>

class WeatherBackground
{
public:
    static QString categoryForWeatherCode(int conditionCode);
    static QPixmap randomBackgroundForWeatherCode(int conditionCode, const QString &applicationDirPath);
};

#endif // WEATHER_BACKGROUND_H
