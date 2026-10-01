#ifndef BOOTH_H
#define BOOTH_H

#include <QObject>

class QAudioOutput;
class QMediaPlayer;
class QProcess;
class QTimer;
class QWidget;

class Booth : public QObject
{
	Q_OBJECT

public:
	explicit Booth(QObject *parent = nullptr);
	void selectAndStartExe(QWidget *dialogParent);

signals:
	void statusMessage(const QString &message);

private:
	void startSavedExe(QWidget *dialogParent);
	void checkWatchedProcess();
	void playBoothAudio();

	QProcess *m_process;
	QMediaPlayer *m_player;
	QAudioOutput *m_audioOutput;
	QTimer *m_watchTimer;
	QString m_watchedExePath;
	bool m_wasRunning;
};

#endif // BOOTH_H
