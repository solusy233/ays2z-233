#ifndef AUTO_POWEROFF_H
#define AUTO_POWEROFF_H

#include <QAudioOutput>
#include <QDate>
#include <QDateTime>
#include <QMediaPlayer>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QTimer>

/**
 * @class AutoPoweroff
 * @brief 定时"播报天气提示音 + 播放音乐 + 自动关机"的调度器。
 *
 * 每天到达配置时间后按固定顺序执行：
 *   1) 到点后先按天气判定播放 weather/tips_sound 下的提示音，占用 kWeatherTipSeconds 秒；
 *   2) 提示音窗口结束后开始播放当天的音乐；
 *   3) 到达"开始时间 + kShutdownAfterSeconds"时停止播放并执行系统关机。
 * 三个阶段的截止时刻全部用绝对 QDateTime 计算，跨零点不会重复触发；
 * 每天最多执行一次，程序启动时若已过开始时间则当天不再补播。
 *
 * 仅供自动化验证使用（生产环境请勿设置）：环境变量 AUTO_POWEROFF_DRY_RUN=1 时
 * 只广播状态、不真正调用系统 shutdown 命令。
 */
class AutoPoweroff : public QObject
{
	Q_OBJECT

public:
	/// 到点后先播放天气提示音的秒数（不改变"开始时间 + 5 分钟关机"这一约定）
	static constexpr int kWeatherTipSeconds = 10;
	/// 从配置的开始时间起，多少秒后执行关机
	static constexpr int kShutdownAfterSeconds = 5 * 60;

	/**
	 * @brief 构造函数
	 * @param parent 父对象，默认为nullptr
	 */
	explicit AutoPoweroff(QObject *parent = nullptr);

	/**
	 * @brief 启动定时检查
	 */
	void start();
	/**
	 * @brief 获取配置的开始时间
	 * @return 配置的开始时间
	 */
	QTime configuredStartTime() const;
	/**
	 * @brief 设置开始时间；会作废当天正在进行的周期并按新时间重新判定
	 * @param time 要设置的开始时间
	 */
	void setStartTime(const QTime &time);
	/**
	 * @brief 取消当天剩余的自动流程（停止播放、不再执行关机）
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
	 * @brief 周期开始信号（到点即发出，界面据此展示天气窗口）
	 */
	void playbackStarted();

private:
	/// 当天周期的阶段
	enum class Stage
	{
		Idle,        ///< 今天还没到点，等待
		WeatherTip,  ///< 天气提示音窗口
		Music,       ///< 音乐播放阶段，直到关机截止时刻
		Finished     ///< 今天的周期已完成，或启动时已错过开始时间
	};

	/**
	 * @brief 定时检查：跨天复位 + 按阶段推进（每秒一次）
	 */
	void checkSchedule();
	/**
	 * @brief 到点：计算截止时刻、播放天气提示音
	 */
	void beginCycle();
	/**
	 * @brief 按天气判定选一个提示音并播放（取不到时只记录状态）
	 */
	void startWeatherTip();
	/**
	 * @brief 提示音窗口结束：开始播放当天的音乐
	 */
	void startMusic();
	/**
	 * @brief 当天的音乐文件路径（星期几 -> 00N.mp3）
	 */
	QString musicFilePath() const;
	/**
	 * @brief 播放歌单中的下一首
	 */
	void playNextTrack();
	/**
	 * @brief 媒体状态变化：音乐阶段播完一首自动续播
	 */
	void handleMediaStatus(QMediaPlayer::MediaStatus status);
	/**
	 * @brief 记录天气快照、停止播放并执行关机
	 */
	void shutdownNow();
	/**
	 * @brief 从配置加载开始时间
	 *
	 * 先读 Auto_poweroff233/start_time.json（设置界面改时间时写入的权威副本），
	 * 缺失或非法时回退到 settings.json 的 "startTime"（部署模板的默认开始时间）；
	 * 两处都取不到时保持无效，表现为当天不做任何到点动作。
	 */
	void loadStartTime();
	/**
	 * @brief 保存开始时间到配置（同时同步到 settings.json，供天气抓取读取）
	 */
	void saveStartTime() const;
	/**
	 * @brief 把开始时间写进 settings.json 的 "startTime"（保留其它键）
	 *
	 * 天气抓取时刻以 settings.json 的 startTime 为准，而开始时间的权威副本是
	 * Auto_poweroff233/start_time.json，两处必须一致：启动加载完成与每次改时间
	 * 之后都同步一次，避免天气按过期的开始时间抓取。
	 */
	void syncStartTimeToSettingsJson() const;

	// 成员变量
	QTimer *timer;              // 定时器，用于定时检查
	QMediaPlayer *player;       // 媒体播放器
	QAudioOutput *audioOutput;  // 音频输出
	QStringList playlist;       // 当前阶段的播放列表
	int currentTrack;           // 当前播放的音轨索引
	Stage stage;                // 当前阶段
	QDate cycleDate;            // 当前周期所属日期；无效值表示尚未初始化
	QDateTime tipDeadline;      // 天气提示音窗口结束时刻
	QDateTime shutdownDeadline; // 关机时刻
	QTime startTime;            // 每天的开始时间
	bool dryRun;                // 只广播状态、不真正关机（AUTO_POWEROFF_DRY_RUN=1）
};

#endif
