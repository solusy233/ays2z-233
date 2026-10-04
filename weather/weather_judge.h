#ifndef WEATHER_JUDGE_H
#define WEATHER_JUDGE_H

#include <QPixmap>
#include <QString>

/**
 * 天气判定工具类（原 weather_background，随文件一并改名）。
 *
 * 纯静态工具类，无实例状态；所有资源都相对 applicationDirPath 定位到
 * weather/ 目录下，数据来源与重构前完全一致。职责分三类：
 *   1) 天气码 -> 背景分类目录名（sunny / rainy / snow / fog）；
 *   2) 分类 -> 背景图、文字提示、提示音；
 *   3) 从 weather/weather.json 读出判定用的天气码，并把天气快照写回 settings.json。
 */
class WeatherJudge
{
public:
    /**
     * 把和风天气 conditionCode 归类为背景资源目录名。
     * 100..199 -> "sunny"，300..399 -> "rainy"，400..499 -> "snow"，
     * 500..599 -> "fog"；未命中任何区间（含 0、200..299、600 及以上）一律回退为 "sunny"。
     */
    static QString categoryForWeatherCode(int conditionCode);
    /**
     * 在 weather/picture/<分类> 目录下随机取一张图片作为背景。
     * 匹配 png、jpg、jpeg、bmp 后缀；目录不存在或没有可用图片时返回空 QPixmap 并输出 qWarning。
     * 目录列表按目录修改时间缓存；小于 QPixmapCache 容量上限的图片可按绝对路径命中已解码的缓存，
     * 超过上限的大图（例如 fog）仍会重新解码。
     */
    static QPixmap randomBackgroundForWeatherCode(int conditionCode, const QString &applicationDirPath);
    /**
     * 从 weather/picture/weathertips.json 随机取一条提示语。
     * 取值优先级：温度趋势键（Heating_up / Cooling_down）> 天气键（Rain / Snow / Fog）
     * > 顶层 normor_tips（仅 sunny 区间 100..199 兜底）；都没有时返回空 QString。
     * 提示文件与天气/设置数据均带失效判定缓存，重复调用不会重复读盘。
     */
    static QString randomTipForWeatherCode(int conditionCode, const QString &applicationDirPath);

    /**
     * 读出"天气判定"用的天气码。
     * 依次尝试 weather/weather.json（运行期由 WeatherApi 写入）与
     * weather/picture/weather.json（随程序分发的兜底数据）；
     * 每个文件依次取 daily[0].condition.code（兼容字符串与数字）、daily[0].iconDay。
     * 全部取不到时返回 0，表示无法判定天气（0 不是有效的和风天气码）。
     */
    static int conditionCodeFromWeatherFile(const QString &applicationDirPath);

    /**
     * 在 weather/tips_sound/<分类> 目录下随机取一个提示音。
     * 匹配 mp3、m4a、wav、aac、ogg 后缀；该分类目录不存在或没有可用音频时
     * 回退 weather/tips_sound/others/；仍然取不到时返回空 QString 并输出 qWarning。
     * 目录列表同样按目录修改时间缓存。
     */
    static QString randomTipSoundForWeatherCode(int conditionCode, const QString &applicationDirPath);

    /**
     * 把当前天气数据快照写回 settings.json：averageTemperature 取
     * daily[0] 的 (tempMax + tempMin) / 2，weatherUpdateTime 取 updateTime；
     * 其它键保持不变，供第二天的温度趋势提示比对。
     * 天气数据不可用（文件缺失/解析失败/字段缺失）时不写入，返回 false。
     */
    static bool recordWeatherSnapshot(const QString &applicationDirPath);
};

#endif // WEATHER_JUDGE_H
