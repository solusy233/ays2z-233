#ifndef WEATHER_SCHEDULE_H
#define WEATHER_SCHEDULE_H

#include <QDateTime>
#include <QString>
#include <QTime>

/**
 * @file weather_schedule.h
 * @brief 天气抓取时刻的纯计算：每天在"开始时间"前 kFetchLeadSeconds 秒抓取一次天气。
 *
 * 抓取时刻以 settings.json 的 "startTime"（每天关机流程的开始时间）为准：
 * WeatherApi 每分钟据此判断是否进入抓取窗口。这里只做时间计算与配置解析，
 * 不涉及网络、定时器和文件写入，便于独立验证。
 */
namespace WeatherSchedule {

/// settings.json 的 startTime 之前多少秒抓取（需求：前 2~3 分钟，取 3 分钟）
constexpr int kFetchLeadSeconds = 180;

/**
 * @brief settings.json 里读不到可用 startTime 时的兜底"开始时间"
 * @return 21:00（与改造前的固定抓取时刻一致）
 */
QTime fallbackStartTime();

/**
 * @brief 从 settings.json 读出"开始时间"
 * @param settingsPath settings.json 的绝对路径
 * @return 按 "HH:mm"（其次 "HH:mm:ss"）解析成功的时间；
 *         文件打不开、不是 JSON 对象、键缺失或时间非法时返回无效 QTime
 */
QTime startTimeFromSettings(const QString &settingsPath);

/**
 * @brief 判断 now 是否落在抓取窗口内，并给出该周期的到点时刻
 *
 * 窗口 = [到点 - kFetchLeadSeconds, 到点]。到点指"开始时间"的那一次：
 * now 之前的那一次（今天已过则顺延到明天）。窗口用 QDateTime 计算，
 * 因此与 23:5X 这类会跨午夜的开始时间也成立。
 *
 * @param now 当前时刻
 * @param startTime 每天的开始时间
 * @return now 在窗口内时返回该周期的到点时刻；窗口外、startTime 或 now 无效时
 *         返回无效 QDateTime（调用方据此判断"已错过不补抓"）
 */
QDateTime fetchWindowScheduledAt(const QDateTime &now, const QTime &startTime);

} // namespace WeatherSchedule

#endif // WEATHER_SCHEDULE_H
