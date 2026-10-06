#include "booth.h"

#include <QAudioOutput>
#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>
#include <QUrl>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#endif

namespace {
QString settingsJsonPath()
{
	return QCoreApplication::applicationDirPath() + QStringLiteral("/settings.json");
}

QString readExecutablePathFromJson()
{
	QFile file(settingsJsonPath());
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return QString();

	const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
	return document.isObject()
		? document.object().value(QStringLiteral("exePath")).toString()
		: QString();
}

bool writeExecutablePathToJson(const QString &path)
{
	QFile file(settingsJsonPath());
	QJsonObject object;
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		const QJsonDocument currentDocument = QJsonDocument::fromJson(file.readAll());
		if (currentDocument.isObject())
			object = currentDocument.object();
		file.close();
	}

	object.insert(QStringLiteral("exePath"), path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
		return false;

	const QByteArray data = QJsonDocument(object).toJson(QJsonDocument::Indented);
	return file.write(data) == data.size();
}

bool isProcessRunning(const QString &executablePath)
{
	if (executablePath.isEmpty())
		return false;

#ifdef Q_OS_WIN
	const QString expectedPath = QDir::cleanPath(QFileInfo(executablePath).absoluteFilePath());
	const QString expectedName = QFileInfo(expectedPath).fileName();
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) {
		qWarning() << "Cannot enumerate Windows processes:" << GetLastError();
		return false;
	}

	PROCESSENTRY32W processEntry{};
	processEntry.dwSize = sizeof(processEntry);
	bool nameMatchWithoutPathAccess = false;
	if (Process32FirstW(snapshot, &processEntry)) {
		do {
			const QString processName = QString::fromWCharArray(processEntry.szExeFile);
			if (processName.compare(expectedName, Qt::CaseInsensitive) != 0)
				continue;

			HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
										 FALSE,
									 processEntry.th32ProcessID);
			if (!process) {
				nameMatchWithoutPathAccess = true;
				continue;
			}

			wchar_t processPath[32768]{};
			DWORD pathLength = static_cast<DWORD>(sizeof(processPath) / sizeof(processPath[0]));
			const BOOL pathAvailable = QueryFullProcessImageNameW(process, 0, processPath, &pathLength);
			CloseHandle(process);
			if (pathAvailable) {
				const QString actualPath = QDir::cleanPath(
					QString::fromWCharArray(processPath, static_cast<int>(pathLength)));
				if (actualPath.compare(expectedPath, Qt::CaseInsensitive) == 0) {
					CloseHandle(snapshot);
					return true;
				}
			} else {
				nameMatchWithoutPathAccess = true;
			}
		} while (Process32NextW(snapshot, &processEntry));
	}

	CloseHandle(snapshot);
	return nameMatchWithoutPathAccess;
#else
	Q_UNUSED(executablePath);
	return false;
#endif
}
}

Booth::Booth(QObject *parent)
	: QObject(parent)
	, m_process(new QProcess(this))
	, m_player(new QMediaPlayer(this))
	, m_audioOutput(new QAudioOutput(this))
	, m_watchTimer(new QTimer(this))
	, m_watchedExePath(readExecutablePathFromJson())
	, m_wasRunning(isProcessRunning(m_watchedExePath))
{
	m_audioOutput->setVolume(1.0);
	m_player->setAudioOutput(m_audioOutput);

	if (m_wasRunning)
		qInfo() << "Configured exe is already running:" << m_watchedExePath;
	connect(m_watchTimer, &QTimer::timeout, this, &Booth::checkWatchedProcess);
	m_watchTimer->start(500);
	connect(m_process, &QProcess::started, this, [this]() {
		if (!m_wasRunning) {
			m_wasRunning = true;
			qInfo() << "Monitored exe started:" << m_watchedExePath;
			emit statusMessage(QStringLiteral("已检测到程序启动：%1").arg(m_watchedExePath));
		}
	});
	connect(m_player, &QMediaPlayer::errorOccurred, this,
			[this](QMediaPlayer::Error error, const QString &errorString) {
				qWarning() << "Booth audio playback failed:" << error << errorString;
				emit statusMessage(QStringLiteral("Qt 播放失败，尝试使用系统播放器：%1").arg(errorString));
				if (!QDesktopServices::openUrl(m_player->source()))
					emit statusMessage(QStringLiteral("系统播放器也无法打开 zhantai.wav。"));
			});
	connect(m_player, &QMediaPlayer::playbackStateChanged, this,
			[this](QMediaPlayer::PlaybackState state) {
				qInfo() << "Booth audio playback state:" << state;
			});
}

void Booth::selectAndStartExe(QWidget *dialogParent)
{
	if (m_process->state() != QProcess::NotRunning) {
		QMessageBox::information(dialogParent,
								 QStringLiteral("程序运行中"),
								 QStringLiteral("所选程序仍在运行，请关闭后再启动。"));
		return;
	}

	const QFileInfo savedFileInfo(readExecutablePathFromJson());
	const QString initialDirectory = savedFileInfo.exists()
		? savedFileInfo.absolutePath()
		: QStringLiteral("C:/");
	const QString filePath = QFileDialog::getOpenFileName(
		dialogParent,
		QStringLiteral("选择要启动的程序"),
		initialDirectory,
		QStringLiteral("可执行文件 (*.exe)"));

	if (filePath.isEmpty())
		return;

	const QFileInfo fileInfo(filePath);
	if (!fileInfo.isFile() || fileInfo.suffix().compare(QStringLiteral("exe"), Qt::CaseInsensitive) != 0) {
		QMessageBox::warning(dialogParent,
							 QStringLiteral("文件无效"),
							 QStringLiteral("请选择有效的 .exe 文件。"));
		return;
	}

	if (!writeExecutablePathToJson(filePath)) {
		QMessageBox::warning(dialogParent,
							 QStringLiteral("保存失败"),
							 QStringLiteral("无法将程序路径写入 settings.json。"));
		return;
	}

	startSavedExe(dialogParent);
}

void Booth::startSavedExe(QWidget *dialogParent)
{
	const QString filePath = readExecutablePathFromJson();
	const QFileInfo fileInfo(filePath);
	if (filePath.isEmpty() || !fileInfo.isFile()) {
		QMessageBox::warning(dialogParent,
							 QStringLiteral("程序路径无效"),
							 QStringLiteral("settings.json 中保存的 exe 路径不存在。"));
		return;
	}

	m_watchedExePath = QDir::cleanPath(fileInfo.absoluteFilePath());
	m_wasRunning = isProcessRunning(m_watchedExePath);
	if (m_wasRunning) {
		qInfo() << "Configured exe is already running:" << m_watchedExePath;
		emit statusMessage(QStringLiteral("程序已经运行，正在监测关闭：%1").arg(m_watchedExePath));
		return;
	}

	m_process->setWorkingDirectory(fileInfo.absolutePath());
	m_process->setProgram(filePath);
	m_process->start();

	if (!m_process->waitForStarted(3000)) {
		QMessageBox::warning(dialogParent,
							 QStringLiteral("启动失败"),
							 m_process->errorString());
		return;
	}

	emit statusMessage(QStringLiteral("已启动：%1").arg(filePath));
}

void Booth::checkWatchedProcess()
{
	if (m_watchedExePath.isEmpty())
		return;

	const bool isRunning = isProcessRunning(m_watchedExePath);
	if (isRunning == m_wasRunning)
		return;

	m_wasRunning = isRunning;
	if (isRunning) {
		qInfo() << "Monitored exe started:" << m_watchedExePath;
		emit statusMessage(QStringLiteral("已检测到程序启动：%1").arg(m_watchedExePath));
		return;
	}

	qInfo() << "Monitored exe closed:" << m_watchedExePath;
	emit statusMessage(QStringLiteral("所选程序已关闭，正在播放 zhantai.wav。"));
	playBoothAudio();
}

void Booth::playBoothAudio()
{
	const QString audioPath = QCoreApplication::applicationDirPath()
		+ QStringLiteral("/booth/zhantai.wav");
	const QFileInfo audioFileInfo(audioPath);
	if (!audioFileInfo.isFile() || !audioFileInfo.isReadable()) {
		emit statusMessage(QStringLiteral("找不到音频文件：%1").arg(audioPath));
		qWarning() << "Booth audio file is missing or unreadable:" << audioPath;
		return;
	}

	qInfo() << "Playing booth audio:" << audioPath;
	m_player->setSource(QUrl::fromLocalFile(audioPath));
	m_player->play();
}
