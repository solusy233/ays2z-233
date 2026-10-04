#include "weather_judge.h"

#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMutex>
#include <QMutexLocker>
#include <QPixmapCache>
#include <QRandomGenerator>
#include <QStringList>
#include <QStringView>

#include <array>

namespace {

// ---------------------------------------------------------------------------
// 表驱动数据：天气码区间 -> 分类 / 提示键
// ---------------------------------------------------------------------------

/// 天气分类，供分类表与提示键表共用
enum class WeatherCategory
{
    Sunny,
    Rainy,
    Snow,
    Fog
};

/// 天气码区间（左闭右开）与对应的分类、背景目录名
struct CategoryEntry
{
    int firstCode;
    int limitCode;
    WeatherCategory category;
    const char *folderName;
};

constexpr std::array<CategoryEntry, 4> kCategoryTable{{
    {100, 200, WeatherCategory::Sunny, "sunny"},
    {300, 400, WeatherCategory::Rainy, "rainy"},
    {400, 500, WeatherCategory::Snow, "snow"},
    {500, 600, WeatherCategory::Fog, "fog"},
}};

/// 未命中任何区间（含 0、200..299、600 及以上）时的兜底背景目录名，与重构前一致
constexpr const char *kFallbackFolderName = "sunny";

/// 提示音兜底分类目录：分类目录缺失或没有音频时使用 weather/tips_sound/others
constexpr const char *kFallbackTipSoundFolderName = "others";

/// 天气判定数据文件的候选相对路径：运行期由 WeatherApi 写入的文件优先，
/// 随程序分发的 weather/picture/weather.json 作为兜底
constexpr std::array<const char *, 2> kWeatherJsonPaths{{
    "weather/weather.json",
    "weather/picture/weather.json"
}};

/// 分类与 weather_tips 中的键；sunny 没有天气键，使用顶层 normor_tips 兜底
struct TipKeyEntry
{
    WeatherCategory category;
    const char *key;
};

constexpr std::array<TipKeyEntry, 3> kWeatherTipKeyTable{{
    {WeatherCategory::Rainy, "Rain"},
    {WeatherCategory::Snow, "Snow"},
    {WeatherCategory::Fog, "Fog"},
}};

/// 查找天气码命中的分类表项，未命中返回 nullptr
const CategoryEntry *findCategoryEntry(int conditionCode)
{
    for (const CategoryEntry &entry : kCategoryTable) {
        if (conditionCode >= entry.firstCode && conditionCode < entry.limitCode)
            return &entry;
    }
    return nullptr;
}

/// 查找分类对应的 weather_tips 键，无对应键时返回空视图
QLatin1StringView weatherTipKeyFor(WeatherCategory category)
{
    for (const TipKeyEntry &entry : kWeatherTipKeyTable) {
        if (entry.category == category)
            return QLatin1StringView(entry.key);
    }
    return QLatin1StringView();
}

/// 从"可判空、可按下标访问"的容器中随机取一个元素，容器为空时返回 fallback
template <typename Container>
auto randomElement(const Container &items, const typename Container::value_type &fallback)
    -> typename Container::value_type
{
    if (items.isEmpty())
        return fallback;
    return items.at(QRandomGenerator::global()->bounded(items.size()));
}

/// 把 JSON 里的天气码转成正整数：兼容 JSON number 与字符串两种写法，
/// 取不到或不是正数时返回 0
int positiveWeatherCode(const QJsonValue &value)
{
    if (!value.isDouble()) {
        bool textOk = false;
        const int textCode = value.toString().trimmed().toInt(&textOk);
        return textOk && textCode > 0 ? textCode : 0;
    }
    const int numberCode = value.toInt();
    return numberCode > 0 ? numberCode : 0;
}

/// 从 daily[0] 取天气码：condition.code 优先，其次 iconDay
int conditionCodeFromDaily(const QJsonObject &dailyDay)
{
    const QJsonValue condition = dailyDay.value(QStringLiteral("condition"));
    const int conditionCode = condition.isObject()
        ? positiveWeatherCode(condition.toObject().value(QStringLiteral("code")))
        : positiveWeatherCode(condition);
    if (conditionCode > 0)
        return conditionCode;
    return positiveWeatherCode(dailyDay.value(QStringLiteral("iconDay")));
}

// ---------------------------------------------------------------------------
// 磁盘 I/O 缓存：本文件这些函数会在窗口构造、每次刷新与每天关机时被重复调用，
// 这里缓存 JSON 文档、目录文件列表与解码后的 QPixmap，避免重复读盘/解码。
// ---------------------------------------------------------------------------

/// 一次 JSON 读取的结果；opened/parsed 分开记录，便于调用方区分警告文案
struct JsonLoadResult
{
    QJsonDocument document;
    QString parseError;
    bool opened = false;
    bool parsed = false;
};

/// JSON 缓存项：以（存在性、文件大小、最后修改时间）作为失效判据，
/// 并记录本次实际读盘的时刻，用于统计信息稳定期判断
struct CachedJson
{
    JsonLoadResult result;
    bool exists = false;
    qint64 size = -1;
    QDateTime lastModified;
    qint64 loadedAtMs = 0;
};

/// 统计信息稳定期（毫秒）。刚被写入的文件，其 (存在性, 大小, mtime) 可能与缓存时完全一致
/// （同一毫秒/同一秒内的等长改写、改写后用 QFile::setFileTime 还原 mtime 等），
/// 仅凭统计信息无法区分，因此稳定期内的缓存不予命中，强制重读一次。
constexpr qint64 kStatSettledMs = 1000;

/// 目录文件列表缓存项：以目录修改时间作为失效判据
struct CachedFileList
{
    QStringList fileNames;
    QDateTime lastModified;
};

/// 保护下面两个缓存容器；QPixmap/QPixmapCache 本身不受该互斥量保护，
/// 因此这里只保证缓存容器的线程安全，不承诺整套 QPixmap 逻辑可跨线程使用
QMutex g_cacheMutex;
QHash<QString, CachedJson> g_jsonCache;
QHash<QString, CachedFileList> g_fileListCache;

/// 读取并解析 JSON 文件；命中缓存的条件是：
///   * 文件存在，且 (存在性, 大小, mtime) 与缓存时完全一致；并且
///   * 距上次读盘已超过稳定期，或文件自身的 mtime 距现在已超过稳定期。
/// 后者保证"文件早已写入完成"的正常场景下一次读取后即可稳定命中缓存（连续调用不重复读盘）；
/// 刚被写入的文件两个条件都不满足，必然重读，从而覆盖统计信息无法区分的改写场景。
/// 文件不存在时不做命中，保持与重构前一样每次都尝试读盘。
JsonLoadResult loadJsonCached(const QString &filePath)
{
    const QFileInfo fileInfo(filePath);
    const bool exists = fileInfo.exists();
    const qint64 size = exists ? fileInfo.size() : -1;
    const QDateTime lastModified = fileInfo.lastModified();
    const qint64 lastModifiedMs = exists ? lastModified.toMSecsSinceEpoch() : -1;
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    const QMutexLocker locker(&g_cacheMutex);

    const auto cached = g_jsonCache.constFind(filePath);
    if (cached != g_jsonCache.constEnd()
        && cached->exists
        && cached->exists == exists
        && cached->size == size
        && cached->lastModified == lastModified
        && (nowMs - lastModifiedMs >= kStatSettledMs
            || nowMs - cached->loadedAtMs >= kStatSettledMs)) {
        return cached->result;
    }

    JsonLoadResult result;
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.opened = true;
        QJsonParseError parseError;
        result.document = QJsonDocument::fromJson(file.readAll(), &parseError);
        result.parseError = parseError.errorString();
        result.parsed = (parseError.error == QJsonParseError::NoError);
    }

    CachedJson entry;
    entry.result = result;
    entry.exists = exists;
    entry.size = size;
    entry.lastModified = lastModified;
    entry.loadedAtMs = nowMs;
    g_jsonCache.insert(filePath, entry);
    return result;
}

/// 列出目录下符合 filters 的文件（沿用 QDir 默认的大小写不敏感匹配）。
/// 列表按"目录路径 + 过滤器集合"缓存，以目录修改时间为失效判据；
/// 图片与提示音两类过滤器共用这份实现，但缓存键不同，不会互相串味。
QStringList fileNamesCached(const QDir &dir, const QString &folderPath, const QStringList &filters)
{
    const QDateTime lastModified = QFileInfo(folderPath).lastModified();
    const QString cacheKey = folderPath + QLatin1Char('\n') + filters.join(QLatin1Char(';'));

    const QMutexLocker locker(&g_cacheMutex);

    const auto cached = g_fileListCache.constFind(cacheKey);
    if (cached != g_fileListCache.constEnd() && cached->lastModified == lastModified)
        return cached->fileNames;

    const QStringList fileNames = dir.entryList(filters, QDir::Files);
    g_fileListCache.insert(cacheKey, CachedFileList{fileNames, lastModified});
    return fileNames;
}

/// 背景图片候选：*.png/*.jpg/*.jpeg/*.bmp
QStringList imageFileNamesCached(const QDir &dir, const QString &folderPath)
{
    static const QStringList kImageFilters{
        QStringLiteral("*.png"),
        QStringLiteral("*.jpg"),
        QStringLiteral("*.jpeg"),
        QStringLiteral("*.bmp"),
    };
    return fileNamesCached(dir, folderPath, kImageFilters);
}

/// 提示音候选：*.mp3/*.m4a/*.wav/*.aac/*.ogg
QStringList audioFileNamesCached(const QDir &dir, const QString &folderPath)
{
    static const QStringList kAudioFilters{
        QStringLiteral("*.mp3"),
        QStringLiteral("*.m4a"),
        QStringLiteral("*.wav"),
        QStringLiteral("*.aac"),
        QStringLiteral("*.ogg"),
    };
    return fileNamesCached(dir, folderPath, kAudioFilters);
}

/// 读取天气判定数据：依次尝试 weather/weather.json（运行期文件）、
/// weather/picture/weather.json（分发兜底），返回第一个能解析出非空 daily 数组的对象；
/// 都不可用时返回空对象。
QJsonObject loadWeatherDocument(const QString &applicationDirPath)
{
    const QDir baseDir(applicationDirPath);
    for (const char *relativePath : kWeatherJsonPaths) {
        const JsonLoadResult result =
            loadJsonCached(baseDir.filePath(QString::fromLatin1(relativePath)));
        if (!result.parsed || !result.document.isObject())
            continue;
        const QJsonObject object = result.document.object();
        const QJsonArray daily = object.value(QStringLiteral("daily")).toArray();
        if (daily.isEmpty() || !daily.first().isObject())
            continue;
        return object;
    }
    return QJsonObject();
}

/// 计算温度趋势键（Heating_up / Cooling_down），不满足条件时返回空字符串。
/// 数据来源与天气判定、天气快照保持同一份：weather/weather.json（运行期写入）优先、
/// weather/picture/weather.json 兜底，取其 daily[0].tempMax/tempMin 平均值，与
/// settings.json 的 averageTemperature 之差；仅当 settings.json 的 weatherUpdateTime
/// 有效、与今天相差不超过 2 天、且温差绝对值达到 6 时才给出趋势键。
QString temperatureTrendKey(const QDir &baseDir)
{
    const QJsonObject weather = loadWeatherDocument(baseDir.absolutePath());
    const QJsonArray daily = weather.value(QStringLiteral("daily")).toArray();
    if (daily.isEmpty() || !daily.first().isObject())
        return QString();

    const QJsonObject today = daily.first().toObject();
    bool maximumOk = false;
    bool minimumOk = false;
    const double maximum = today.value(QStringLiteral("tempMax")).toString().toDouble(&maximumOk);
    const double minimum = today.value(QStringLiteral("tempMin")).toString().toDouble(&minimumOk);
    if (!maximumOk || !minimumOk)
        return QString();
    const double todayAverage = (maximum + minimum) / 2.0;

    const QJsonObject settings =
        loadJsonCached(baseDir.filePath(QStringLiteral("settings.json"))).document.object();

    // weatherUpdateTime 形如 "2026-09-24T21:04+08:00"：Qt::ISODate 解析得到带 UTC 偏移的
    // QDateTime，其 date() 正是该偏移下的当地日期（即"数据更新当天"），不带偏移时按本地
    // 时间解析，date() 语义同样成立；因此这里直接取 date() 再做日期差。
    const QDateTime updateDateTime =
        QDateTime::fromString(settings.value(QStringLiteral("weatherUpdateTime")).toString(),
                              Qt::ISODate);
    if (!updateDateTime.isValid())
        return QString();
    const qint64 daysFromUpdateToToday = updateDateTime.date().daysTo(QDate::currentDate());
    if (daysFromUpdateToToday < -2 || daysFromUpdateToToday > 2)
        return QString();

    // averageTemperature 正常为 JSON number，同时兼容字符串形式的数字
    const QJsonValue previousAverageValue = settings.value(QStringLiteral("averageTemperature"));
    bool previousAverageOk = previousAverageValue.isDouble();
    double previousAverage = 0.0;
    if (previousAverageOk)
        previousAverage = previousAverageValue.toDouble();
    else
        previousAverage = previousAverageValue.toString().toDouble(&previousAverageOk);
    if (!previousAverageOk)
        return QString();

    const double temperatureDifference = todayAverage - previousAverage;
    if (temperatureDifference >= 6.0)
        return QStringLiteral("Heating_up");
    if (temperatureDifference <= -6.0)
        return QStringLiteral("Cooling_down");
    return QString();
}

} // namespace

QString WeatherJudge::categoryForWeatherCode(int conditionCode)
{
    const CategoryEntry *entry = findCategoryEntry(conditionCode);
    return QString::fromLatin1(entry ? entry->folderName : kFallbackFolderName);
}

QString WeatherJudge::randomTipForWeatherCode(int conditionCode, const QString &applicationDirPath)
{
    const QDir baseDir(applicationDirPath);

    // 1) 读取 weather/picture/weathertips.json（命中缓存则不重复读盘）
    const QString tipsPath = baseDir.filePath(QStringLiteral("weather/picture/weathertips.json"));
    const JsonLoadResult tipsResult = loadJsonCached(tipsPath);
    if (!tipsResult.opened) {
        qWarning() << "天气提示文件无法打开:" << tipsPath;
        return QString();
    }
    if (!tipsResult.parsed || !tipsResult.document.isObject()) {
        qWarning() << "天气提示文件解析失败:" << tipsResult.parseError;
        return QString();
    }

    const QJsonObject tips = tipsResult.document.object();
    const QJsonObject weatherTips = tips.value(QStringLiteral("weather_tips")).toObject();

    // 2) 温度趋势优先级最高；趋势键缺失或对应数组为空时继续向下兜底
    QJsonArray candidates;
    const QString trendKey = temperatureTrendKey(baseDir);
    if (!trendKey.isEmpty())
        candidates = weatherTips.value(trendKey).toArray();

    // 3) 天气键（Rain/Snow/Fog）次之，sunny 区间才回退到顶层 normor_tips；
    //    未命中任何天气码区间（例如 0、200..299）时保持为空，与重构前一致
    if (candidates.isEmpty()) {
        if (const CategoryEntry *category = findCategoryEntry(conditionCode)) {
            const QLatin1StringView weatherKey = weatherTipKeyFor(category->category);
            if (!weatherKey.isEmpty())
                candidates = weatherTips.value(weatherKey).toArray();
            else if (category->category == WeatherCategory::Sunny)
                candidates = tips.value(QStringLiteral("normor_tips")).toArray();
        }
    }

    if (candidates.isEmpty())
        return QString();

    return randomElement(candidates, QJsonValue()).toString();
}

QPixmap WeatherJudge::randomBackgroundForWeatherCode(int conditionCode, const QString &applicationDirPath)
{
    const QDir baseDir(applicationDirPath);
    const QString folderPath = baseDir.filePath(
        QStringLiteral("weather/picture/%1").arg(categoryForWeatherCode(conditionCode)));
    const QDir imageDir(folderPath);
    if (!imageDir.exists()) {
        qWarning() << "天气背景目录不存在:" << folderPath;
        return QPixmap();
    }

    const QStringList imageFiles = imageFileNamesCached(imageDir, folderPath);
    if (imageFiles.isEmpty()) {
        qWarning() << "天气背景目录中没有可用图片:" << folderPath;
        return QPixmap();
    }

    const QString backgroundPath = imageDir.filePath(randomElement(imageFiles, QString()));

    // 按路径命中已解码的图片，避免每次刷新都重新解码大图
    QPixmap background;
    if (QPixmapCache::find(backgroundPath, &background))
        return background;

    background.load(backgroundPath);
    if (!background.isNull())
        QPixmapCache::insert(backgroundPath, background);
    return background;
}

int WeatherJudge::conditionCodeFromWeatherFile(const QString &applicationDirPath)
{
    const QJsonObject weather = loadWeatherDocument(applicationDirPath);
    if (weather.isEmpty()) {
        qWarning() << "天气数据文件不可用，无法判定天气:"
                   << QDir(applicationDirPath).filePath(QString::fromLatin1(kWeatherJsonPaths.front()));
        return 0;
    }

    // daily[0] 已在 loadWeatherDocument 中校验
    const QJsonObject dailyDay =
        weather.value(QStringLiteral("daily")).toArray().first().toObject();
    return conditionCodeFromDaily(dailyDay);
}

QString WeatherJudge::randomTipSoundForWeatherCode(int conditionCode, const QString &applicationDirPath)
{
    const QDir baseDir(applicationDirPath);
    const QString soundRootPath = baseDir.filePath(QStringLiteral("weather/tips_sound"));

    // 先按天气码分类目录取，再回退 others（分类目录缺失或没有音频文件时）
    QStringList categories{categoryForWeatherCode(conditionCode)};
    if (!categories.contains(QString::fromLatin1(kFallbackTipSoundFolderName)))
        categories.append(QString::fromLatin1(kFallbackTipSoundFolderName));

    for (const QString &category : categories) {
        const QString folderPath = QDir(soundRootPath).filePath(category);
        const QDir soundDir(folderPath);
        if (!soundDir.exists())
            continue;

        const QStringList audioFiles = audioFileNamesCached(soundDir, folderPath);
        if (audioFiles.isEmpty())
            continue;

        return soundDir.absoluteFilePath(randomElement(audioFiles, QString()));
    }

    qWarning() << "天气提示音目录不存在或没有可用音频:" << soundRootPath;
    return QString();
}

bool WeatherJudge::recordWeatherSnapshot(const QString &applicationDirPath)
{
    const QJsonObject weather = loadWeatherDocument(applicationDirPath);
    if (weather.isEmpty())
        return false;

    const QJsonObject today = weather.value(QStringLiteral("daily")).toArray().first().toObject();
    bool maximumOk = false;
    bool minimumOk = false;
    const double maximum = today.value(QStringLiteral("tempMax")).toString().toDouble(&maximumOk);
    const double minimum = today.value(QStringLiteral("tempMin")).toString().toDouble(&minimumOk);
    const QString updateTime = weather.value(QStringLiteral("updateTime")).toString();
    if ((!maximumOk || !minimumOk) && updateTime.isEmpty())
        return false;

    const QString settingsPath = QDir(applicationDirPath).filePath(QStringLiteral("settings.json"));
    QJsonObject settings;
    QFile settingsFile(settingsPath);
    if (settingsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QJsonDocument settingsDocument = QJsonDocument::fromJson(settingsFile.readAll());
        if (settingsDocument.isObject())
            settings = settingsDocument.object();
        settingsFile.close();
    }

    if (maximumOk && minimumOk)
        settings.insert(QStringLiteral("averageTemperature"), (maximum + minimum) / 2.0);
    if (!updateTime.isEmpty())
        settings.insert(QStringLiteral("weatherUpdateTime"), updateTime);

    if (!settingsFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning() << "无法写入设置文件:" << settingsPath << settingsFile.errorString();
        return false;
    }
    return settingsFile.write(QJsonDocument(settings).toJson(QJsonDocument::Indented)) >= 0;
}
