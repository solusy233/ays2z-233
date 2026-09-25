#include "Auto_poweroff.h"

#include <algorithm>
#include <QJsonDocument>
#include <QJsonObject>

AutoPoweroff::AutoPoweroff(QObject *parent)
	: QObject(parent)
	, timer(new QTimer(this))
	, player(new QMediaPlayer(this))
	, audioOutput(new QAudioOutput(this))
	, currentTrack(0)
	, startedToday(false)
	, shutdownToday(false)
	, lastDay(-1)
	, scheduleInitialized(false)
	, startTime(22, 0)
{
	loadStartTime();
	timer->setInterval(1000);
	connect(timer, &QTimer::timeout, this, &AutoPoweroff::checkSchedule);
	connect(player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
		if (status == QMediaPlayer::EndOfMedia)
			playNextTrack();
	});
	connect(player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &errorString) {
		emit statusChanged(QStringLiteral("MP3 播放失败：%1").arg(errorString));
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
	emit statusChanged(QStringLiteral("自动播放已开启：每天 %1 播放，5 分钟后关机")
	                      .arg(startTime.toString(QStringLiteral("HH:mm"))));
}

void AutoPoweroff::setStartTime(const QTime &time)
{
	if (!time.isValid() || time == startTime)
		return;

	startTime = time;
	saveStartTime();
	const QTime currentTime = QTime::currentTime();
	startedToday = currentTime >= startTime;
	shutdownToday = startedToday;
	emit statusChanged(QStringLiteral("已设置每天 %1 播放，5 分钟后关机")
	                      .arg(startTime.toString(QStringLiteral("HH:mm"))));
}

void AutoPoweroff::loadStartTime()
{
	const QString settingsFile = QCoreApplication::applicationDirPath()
		+ QStringLiteral("/Auto_poweroff233/start_time.json");
	QFile file(settingsFile);
	if (!file.open(QIODevice::ReadOnly))
		return;

	const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
	const QJsonValue value = document.object().value(QStringLiteral("startTime"));
	const QTime savedTime = QTime::fromString(value.toString(), QStringLiteral("HH:mm"));
	if (savedTime.isValid())
		startTime = savedTime;
}

void AutoPoweroff::saveStartTime() const
{
	const QString settingsDirectory = QCoreApplication::applicationDirPath()
		+ QStringLiteral("/Auto_poweroff233");
	QDir().mkpath(settingsDirectory);

	QFile file(settingsDirectory + QStringLiteral("/start_time.json"));
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return;

	const QJsonObject object{{QStringLiteral("startTime"), startTime.toString(QStringLiteral("HH:mm"))}};
	file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
}

void AutoPoweroff::cancelShutdown()
{
	QProcess::execute(QStringLiteral("shutdown"), {QStringLiteral("/a")});
	emit statusChanged(QStringLiteral("已取消待执行的关机命令"));
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
	const QTime time = now.time();
	const int day = now.date().dayOfYear();
	if (!scheduleInitialized) {
		scheduleInitialized = true;
		lastDay = day;
		startedToday = time >= startTime;
		shutdownToday = startedToday;
		return;
	}
	if (day != lastDay) {
		lastDay = day;
		startedToday = false;
		shutdownToday = false;
	}

	const QTime shutdownTime = startTime.addSecs(5 * 60);
	if (!startedToday && time >= startTime) {
		startedToday = true;
		startPlaylist();
	}
	if (startedToday && !shutdownToday && time >= shutdownTime) {
		shutdownToday = true;
		scheduleShutdown();
	}
}

void AutoPoweroff::startPlaylist()
{
	const int trackNumber = QDate::currentDate().dayOfWeek();
	const QString musicDirectory = QCoreApplication::applicationDirPath()
		+ QStringLiteral("/Auto_poweroff233/music233");
	const QString trackName = QStringLiteral("%1.mp3").arg(trackNumber, 3, 10, QLatin1Char('0'));
	const QString trackPath = QDir(musicDirectory).absoluteFilePath(trackName);
	if (!QFileInfo::exists(trackPath)) {
		emit statusChanged(QStringLiteral("找不到今天的音乐文件：%1").arg(trackPath));
		return;
	}

	playlist = {trackPath};
	currentTrack = 0;
	emit playbackStarted();
	emit statusChanged(QStringLiteral("开始播放星期 %1 的音乐：%2").arg(trackNumber).arg(trackName));
	playNextTrack();
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

QStringList AutoPoweroff::loadPlaylist()
{
	const QString playlistFile = QCoreApplication::applicationDirPath()
		+ QStringLiteral("/Auto_poweroff233/歌单.txt");
	QFile file(playlistFile);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return {};

	QStringList tracks;
	QTextStream stream(&file);
	while (!stream.atEnd()) {
		const QString line = stream.readLine().trimmed();
		if (line.isEmpty() || line.startsWith('#'))
			continue;

		const QFileInfo listedFile(line);
		const QString path = listedFile.isAbsolute()
			? listedFile.absoluteFilePath()
			: QDir(QFileInfo(playlistFile).absolutePath()).absoluteFilePath(line);
		if (QFileInfo::exists(path))
			tracks.append(QFileInfo(path).absoluteFilePath());
		else
			emit statusChanged(QStringLiteral("找不到文件：%1").arg(path));
	}
	return tracks;
}

void AutoPoweroff::scheduleShutdown()
{
	player->stop();
	QProcess::startDetached(QStringLiteral("shutdown"), {QStringLiteral("/s"), QStringLiteral("/t"), QStringLiteral("0")});
	emit statusChanged(QStringLiteral("已执行关机命令"));
}
