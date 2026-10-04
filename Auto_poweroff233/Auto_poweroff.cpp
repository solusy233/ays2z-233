#include "Auto_poweroff.h"

#include "../weather/weather_judge.h"
#include "../weather/weather_schedule.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QProcess>
#include <QSettings>
#include <QUrl>

namespace {

/// 应用目录下的绝对路径
QString appPath(const QString &relativePath)
{
	return QDir(QCoreApplication::applicationDirPath()).filePath(relativePath);
}

} // namespace

AutoPoweroff::AutoPoweroff(QObject *parent)
	: QObject(parent)
	, timer(new QTimer(this))
	, player(new QMediaPlayer(this))
	, audioOutput(new QAudioOutput(this))
	, currentTrack(0)
	, stage(Stage::Idle)
	, cycleDate()
	, dryRun(qEnvironmentVariableIsSet("AUTO_POWEROFF_DRY_RUN"))
{
	loadStartTime();
	syncStartTimeToSettingsJson();
	timer->setInterval(1000);
	connect(timer, &QTimer::timeout, this, &AutoPoweroff::checkSchedule);
	connect(player, &QMediaPlayer::mediaStatusChanged, this, &AutoPoweroff::handleMediaStatus);
	connect(player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &errorString) {
		emit statusChanged(QStringLiteral("音频播放失败：%1").arg(errorString));
	});
	player->setAudioOutput(audioOutput);
	audioOutput->setVolume(1.0);
}

QTime AutoPoweroff::configuredStartTime() const
{
	return startTime;
}

void AutoPoweroff::start()
{
	timer->start();
	emit statusChanged(QStringLiteral("自动播放已开启：每天 %1 先播 %2 秒天气提示音，随后播放音乐，%3 分钟后关机")
	                       .arg(startTime.toString(QStringLiteral("HH:mm")))
	                       .arg(kWeatherTipSeconds)
	                       .arg(kShutdownAfterSeconds / 60));
}

void AutoPoweroff::setStartTime(const QTime &time)
{
	if (!time.isValid() || time == startTime)
		return;

	startTime = time;
	saveStartTime();

	// 改时间等价于作废当天正在进行的周期：停止播放，并按新时间重新判定今天
	player->stop();
	playlist.clear();
	currentTrack = 0;
	cycleDate = QDate::currentDate();
	stage = QTime::currentTime() >= startTime ? Stage::Finished : Stage::Idle;

	emit statusChanged(QStringLiteral("已设置每天 %1 开始：先播 %2 秒天气提示音，%3 分钟后关机")
	                       .arg(startTime.toString(QStringLiteral("HH:mm")))
	                       .arg(kWeatherTipSeconds)
	                       .arg(kShutdownAfterSeconds / 60));
}

void AutoPoweroff::loadStartTime()
{
	const QString startTimeFile = appPath(QStringLiteral("Auto_poweroff233/start_time.json"));
	QFile file(startTimeFile);
	if (file.open(QIODevice::ReadOnly)) {
		const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
		const QJsonValue value = document.object().value(QStringLiteral("startTime"));
		const QTime savedTime = QTime::fromString(value.toString(), QStringLiteral("HH:mm"));
		if (savedTime.isValid()) {
			startTime = savedTime;
			return;
		}
	}

	// start_time.json 缺失/非法时回退到 settings.json 的 startTime：
	// 部署模板里就写着默认开始时间，否则开始时间会停留在无效值，
	// 到点流程静默失效（不播提示音、不放音乐、不关机，设置界面显示 00:00）。
	const QTime fallback = WeatherSchedule::startTimeFromSettings(appPath(QStringLiteral("settings.json")));
	if (fallback.isValid())
		startTime = fallback;
}

void AutoPoweroff::saveStartTime() const
{
	const QString settingsDirectory = appPath(QStringLiteral("Auto_poweroff233"));
	QDir().mkpath(settingsDirectory);

	QFile file(settingsDirectory + QStringLiteral("/start_time.json"));
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return;

	const QJsonObject object{{QStringLiteral("startTime"), startTime.toString(QStringLiteral("HH:mm"))}};
	file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));

	syncStartTimeToSettingsJson();
}

void AutoPoweroff::syncStartTimeToSettingsJson() const
{
	if (!startTime.isValid())
		return;

	const QString settingsPath = appPath(QStringLiteral("settings.json"));
	QFile settingsFile(settingsPath);

	// 文件不存在就不新建：交给 SettingsWindow 用默认密码创建，避免写出只有 startTime 的残缺配置
	if (!settingsFile.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	const QJsonDocument document = QJsonDocument::fromJson(settingsFile.readAll());
	QJsonObject settings = document.isObject() ? document.object() : QJsonObject();
	settingsFile.close();

	const QString text = startTime.toString(QStringLiteral("HH:mm"));
	if (settings.value(QStringLiteral("startTime")).toString() == text)
		return; // 已经一致就不写，避免每次启动都刷新 mtime

	settings.insert(QStringLiteral("startTime"), text);
	if (!settingsFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		qWarning() << "无法同步 startTime 到设置文件:" << settingsPath << settingsFile.errorString();
		return;
	}
	settingsFile.write(QJsonDocument(settings).toJson(QJsonDocument::Indented));
}

void AutoPoweroff::cancelShutdown()
{
	const bool wasPlaying = (stage == Stage::WeatherTip || stage == Stage::Music);
	player->stop();
	playlist.clear();
	currentTrack = 0;

	// 当天不再自动执行；关机命令本身是 /t 0，这里用 /a 兜底取消系统里待执行的关机
	cycleDate = QDate::currentDate();
	stage = Stage::Finished;

	if (!dryRun)
		QProcess::execute(QStringLiteral("shutdown"), {QStringLiteral("/a")});

	emit statusChanged(wasPlaying ? QStringLiteral("已停止播放，并取消今天剩余的自动关机流程")
	                              : QStringLiteral("已取消今天剩余的自动关机流程"));
}

bool AutoPoweroff::isAutoStartEnabled() const
{
#ifdef Q_OS_WIN
	QSettings settings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
	                   QSettings::NativeFormat);
	const QString value = settings.value(QStringLiteral("untitled")).toString().trimmed();
	if (value.isEmpty())
		return false;

	QString executable = value;
	if (executable.startsWith(QLatin1Char('"')) && executable.endsWith(QLatin1Char('"')))
		executable = executable.mid(1, executable.size() - 2);
	return QFileInfo::exists(executable);
#else
	return false;
#endif
}

void AutoPoweroff::setAutoStartEnabled(bool enabled)
{
#ifdef Q_OS_WIN
	QSettings settings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
	                   QSettings::NativeFormat);
	if (enabled) {
		const QString executable = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
		settings.setValue(QStringLiteral("untitled"), QStringLiteral("\"%1\"").arg(executable));
		emit statusChanged(QStringLiteral("已开启开机自动启动"));
	} else {
		settings.remove(QStringLiteral("untitled"));
		emit statusChanged(QStringLiteral("已关闭开机自动启动"));
	}
	settings.sync();
#else
	Q_UNUSED(enabled)
#endif
}

void AutoPoweroff::checkSchedule()
{
	const QDateTime now = QDateTime::currentDateTime();
	const QDate today = now.date();

	// 跨天复位：只在空闲/已完成时复位，正在进行的周期跨零点时由绝对截止时刻继续驱动。
	// 新的一天里开始时间已过（程序刚启动、或刚过零点）则本日周期视为错过，不补播。
	if (today != cycleDate) {
		cycleDate = today;
		if (stage == Stage::Idle || stage == Stage::Finished)
			stage = now.time() >= startTime ? Stage::Finished : Stage::Idle;
	}

	switch (stage) {
	case Stage::Idle:
		if (now.time() >= startTime)
			beginCycle();
		break;
	case Stage::WeatherTip:
		if (now >= tipDeadline)
			startMusic();
		break;
	case Stage::Music:
		if (now >= shutdownDeadline)
			shutdownNow();
		break;
	case Stage::Finished:
		break;
	}
}

void AutoPoweroff::beginCycle()
{
	const QDateTime now = QDateTime::currentDateTime();
	const QDateTime scheduled(now.date(), startTime);
	shutdownDeadline = scheduled.addSecs(kShutdownAfterSeconds);
	// 提示音窗口从真正触发的时刻起算 kWeatherTipSeconds 秒，但不会把关机时刻往后拖
	tipDeadline = qMin(now.addSecs(kWeatherTipSeconds), shutdownDeadline);

	stage = Stage::WeatherTip;
	emit playbackStarted();
	emit statusChanged(QStringLiteral("到点：先播放天气提示音，%1 秒后开始播放音乐").arg(kWeatherTipSeconds));
	startWeatherTip();
}

void AutoPoweroff::startWeatherTip()
{
	const QString baseDir = QCoreApplication::applicationDirPath();
	const int conditionCode = WeatherJudge::conditionCodeFromWeatherFile(baseDir);
	if (conditionCode <= 0) {
		emit statusChanged(QStringLiteral("未取得可用天气数据，%1 秒后直接播放音乐").arg(kWeatherTipSeconds));
		return;
	}

	const QString tipSound = WeatherJudge::randomTipSoundForWeatherCode(conditionCode, baseDir);
	if (tipSound.isEmpty()) {
		emit statusChanged(QStringLiteral("未找到天气提示音（天气码 %1），%2 秒后直接播放音乐")
		                       .arg(conditionCode)
		                       .arg(kWeatherTipSeconds));
		return;
	}

	player->setSource(QUrl::fromLocalFile(tipSound));
	player->play();
	emit statusChanged(QStringLiteral("正在播放天气提示音（天气码 %1）：%2")
	                       .arg(conditionCode)
	                       .arg(QFileInfo(tipSound).fileName()));
}

void AutoPoweroff::startMusic()
{
	stage = Stage::Music;
	player->stop(); // 提示音还没播完的话在这里截断，保证按约定时刻开始放歌

	const QString trackPath = musicFilePath();
	if (!QFileInfo::exists(trackPath)) {
		emit statusChanged(QStringLiteral("找不到今天的音乐文件：%1").arg(trackPath));
		return; // 音乐缺失也照常在关机截止时刻关机
	}

	playlist = {trackPath};
	currentTrack = 0;
	emit statusChanged(QStringLiteral("开始播放星期 %1 的音乐：%2")
	                       .arg(QDate::currentDate().dayOfWeek())
	                       .arg(QFileInfo(trackPath).fileName()));
	playNextTrack();
}

QString AutoPoweroff::musicFilePath() const
{
	const int trackNumber = QDate::currentDate().dayOfWeek();
	const QString trackName = QStringLiteral("%1.mp3").arg(trackNumber, 3, 10, QLatin1Char('0'));
	return QDir(appPath(QStringLiteral("Auto_poweroff233/music233"))).absoluteFilePath(trackName);
}

void AutoPoweroff::playNextTrack()
{
	if (currentTrack >= playlist.size()) {
		emit statusChanged(QStringLiteral("歌单播放完成"));
		return;
	}

	const QString track = playlist.at(currentTrack++);
	player->setSource(QUrl::fromLocalFile(track));
	player->play();
	emit statusChanged(QStringLiteral("正在播放：%1").arg(QFileInfo(track).fileName()));
}

void AutoPoweroff::handleMediaStatus(QMediaPlayer::MediaStatus status)
{
	if (status != QMediaPlayer::EndOfMedia)
		return;

	// 提示音播完不立刻切音乐：音乐由 tipDeadline 驱动；
	// 音乐阶段则在当前曲目结束后继续歌单里的下一首
	if (stage == Stage::Music)
		playNextTrack();
}

void AutoPoweroff::shutdownNow()
{
	stage = Stage::Finished;
	player->stop();

	// 关机前记录天气快照（averageTemperature / weatherUpdateTime），供第二天的温度趋势提示比对
	WeatherJudge::recordWeatherSnapshot(QCoreApplication::applicationDirPath());

	if (dryRun) {
		emit statusChanged(QStringLiteral("已执行关机命令（dry-run，未真正关机）"));
		return;
	}

	// /s 表示关机，/t 0 表示立即执行
	QProcess::startDetached(QStringLiteral("shutdown"), {QStringLiteral("/s"), QStringLiteral("/t"), QStringLiteral("0")});
	emit statusChanged(QStringLiteral("已执行关机命令"));
}
