#ifndef AUTO_POWEROFF_H
#define AUTO_POWEROFF_H

#include <QAudioOutput>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QObject>
#include <QProcess>
#include <QSettings>
#include <QStringList>
#include <QTextStream>
#include <QTime>
#include <QTimer>
#include <QUrl>

class AutoPoweroff : public QObject
{
	Q_OBJECT

public:
	explicit AutoPoweroff(QObject *parent = nullptr);

	void start();
	QTime configuredStartTime() const;
	void setStartTime(const QTime &time);
	void cancelShutdown();
	bool isAutoStartEnabled() const;
	void setAutoStartEnabled(bool enabled);

signals:
	void statusChanged(const QString &status);
	void playbackStarted();

private:
	void checkSchedule();
	void loadStartTime();
	void saveStartTime() const;
	void updateSchoolDays();
	void startPlaylist();
	void playNextTrack();
	QStringList loadPlaylist();
	void scheduleShutdown();

	QTimer *timer;
	QMediaPlayer *player;
	QAudioOutput *audioOutput;
	QStringList playlist;
	int currentTrack;
	bool startedToday;
	bool shutdownToday;
	int lastDay;
	bool scheduleInitialized;
	QTime startTime;
};

#endif
