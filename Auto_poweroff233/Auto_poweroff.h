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

/**
 * @class AutoPoweroff
 * @brief 自动关机类，继承自QObject，用于实现定时播放音乐和自动关机功能
 */
class AutoPoweroff : public QObject
{
	Q_OBJECT  // Qt宏，用于支持信号槽机制

public:
    /**
     * @brief 构造函数
     * @param parent 父对象，默认为nullptr
     */
	explicit AutoPoweroff(QObject *parent = nullptr);

    /**
     * @brief 启动自动关机功能
     */
	void start();
    /**
     * @brief 获取配置的启动时间
     * @return 配置的启动时间
     */
	QTime configuredStartTime() const;
    /**
     * @brief 设置启动时间
     * @param time 要设置的启动时间
     */
	void setStartTime(const QTime &time);
    /**
     * @brief 取消关机计划
     */
	void cancelShutdown();
    /**
     * @brief 检查自动启动是否启用
     * @return 如果启用返回true，否则返回false
     */
	bool isAutoStartEnabled() const;
    /**
     * @brief 设置是否启用自动启动
     * @param enabled 是否启用自动启动
     */
	void setAutoStartEnabled(bool enabled);

signals:
    /**
     * @brief 状态变化信号
     * @param status 新的状态描述
     */
	void statusChanged(const QString &status);
    /**
     * @brief 播放开始信号
     */
	void playbackStarted();

private:
    /**
     * @brief 检查是否到达计划时间
     */
	void checkSchedule();
    /**
     * @brief 从配置加载启动时间
     */
	void loadStartTime();
    /**
     * @brief 保存启动时间到配置
     */
	void saveStartTime() const;
    /**
     * @brief 开始播放列表
     */
	void startPlaylist();
    /**
     * @brief 播放下一个音轨
     */
	void playNextTrack();
    /**
     * @brief 加载播放列表
     * @return 音轨列表
     */
	QStringList loadPlaylist();
    /**
     * @brief 安排关机
     */
	void scheduleShutdown();



    // 成员变量
	QTimer *timer;          // 定时器，用于定时检查
	QMediaPlayer *player;   // 媒体播放器
	QAudioOutput *audioOutput;  // 音频输出
	QStringList playlist;    // 播放列表
	int currentTrack;       // 当前播放的音轨索引
	bool startedToday;      // 今天是否已启动
	bool shutdownToday;     // 今天是否已关机
	int lastDay;
	bool scheduleInitialized;
	QTime startTime;
};

#endif
