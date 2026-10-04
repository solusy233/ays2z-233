#一个普通的定时关机小exe文件
采用QT6搭建
https://github.com/solusy233/ays2z-233

#功能
1.定时关机
2.对exe关闭的监测（关闭后会播放音频）
3.播报天气
#

#使用
运行exe即可

## 启动与关闭行为
- **启动默认显示主窗口**（不最小化、不隐藏到托盘）：有些机器上托盘图标会被 Windows 折叠进 `︿` 溢出区甚至登记失败，藏起来就等于"没有界面"。
- 托盘图标仍会创建：悬停有提示、双击可在显示/隐藏窗口之间切换、右键菜单有"显示窗口 / 退出"。
- **关闭窗口（点 X）不会退出程序**，只把窗口最小化到任务栏，保证到点流程继续跑；要真正退出请打开设置界面点"退出"。
- 若任务栏上看不到托盘图标：点任务栏的 `︿` 溢出区，或到「设置 → 个性化 → 任务栏 → 通知区域 → 选择哪些图标显示在任务栏上」里打开本程序。

//本项目采用大量ai的代码（80%左右）
//我做了模块化架构,有一定可维护性(如果有人维护的话)
//练手项目不要喷我
//项目已用ai重构，下面是ai写的结构图

#项目结构

ays2z-233（Qt6 定时关机 + 天气播报）
构建产物目录（`build/`，Qt Creator / CMake 生成）不在下图内，已被 `.gitignore` 忽略。

```
untitled/
├── main.cpp                                  程序入口：高 DPI 策略、加载 HarmonyOS 字体、启动 MainWindow
├── CMakeLists.txt                            构建定义（源文件清单）+ POST_BUILD 部署规则
├── resources.qrc                             Qt 资源：主窗背景、设置窗背景、托盘图标、字体（编译进 exe）
├── settings.json                             配置文件模板（CMake 仅在目标缺失时复制，不覆盖运行期改动）
├── SetScale.ps1                              4K 屏辅助脚本：宽度 ≥3840 时把系统缩放设为 200%
├── .editorconfig / .gitignore                编辑器规则 / 忽略规则（build/、build_judge/、*.user）
├── CMakeLists.txt.user                       Qt Creator 本机工程设置
├── .vscode/                                  VS Code 配置：c_cpp_properties.json、launch.json、tasks.json
│
├── aboutwindows/                             界面层
│   ├── mainwindows/                          主窗口
│   │   ├── mainwindow.h / mainwindow.cpp     主窗体逻辑：黑底绘制、系统托盘、设置按钮、提示音阶段弹天气窗
│   │   ├── mainwindow.ui                     主窗体布局（背景图 :/images/background-dark.png）
│   │   ├── app_icon.ico / app_icon.rc        Windows 可执行文件图标
│   │   ├── anys2z.jpg                        托盘图标的源图（已被 assets/tray-icon.png 取代，保留备用）
│   │   ├── 05e55233-…-dcbfed013c5b.png        主窗背景图（qrc alias：background-dark / background-light）
│   │   └── 05e55233-…-dcbfed013c5.png         遗留背景图，qrc 与代码均未引用
│   └── settings/                             设置界面
│       ├── settingswindow.h / .cpp           密码校验、开始时间、开机自启、打开天气、退出
│       ├── settingswindow.ui                 设置窗体布局
│       └── settings 01.png                   设置窗背景（qrc alias：settings-background.png）
│
├── Auto_poweroff233/                         定时关机模块
│   ├── Auto_poweroff.h / Auto_poweroff.cpp   到点状态机：Idle → WeatherTip(10s) → Music → 关机(startTime+5min)
│   │                                         ↳ 读写 Auto_poweroff233/start_time.json，并同步 settings.json 的 startTime
│   └── music233/                             每天的音乐（按星期取文件）
│       ├── 001.mp3 … 007.mp3                 周一 … 周日
│       └── 歌单.txt                           曲目说明
│
├── booth/                                    exe 监测模块
│   ├── booth.h / booth.cpp                   选择并启动外部 exe，监测其关闭后播放音频
│   └── booth.m4a                             监测到 exe 关闭时播放的音频
│
├── weather/                                  天气模块
│   ├── weatherapi.h / weatherapi.cpp         和风天气接口：3 日预报（安阳 101180201）→ weather/weather.json
│   ├── weather_schedule.h / .cpp             抓取时刻计算：startTime 前 3 分钟的窗口 + settings.json 解析
│   ├── weather_judge.h / weather_judge.cpp   天气码判定、分类提示音随机、天气快照写回 settings.json、温度趋势提示
│   ├── weatherwindow.h / .cpp / .ui          天气展示窗口（到点播放提示音时弹出）
│   ├── weather.json                          随程序分发的天气数据（CMake 仅在目标缺失时复制）
│   ├── icons/                                和风天气图标 507 个 SVG（444 普通 + 63 fill）
│   ├── picture/                              背景图 + 兜底数据
│   │   ├── weather.json                      兜底天气数据（weather/weather.json 缺失时使用）
│   │   ├── weathertips.json                  提示语集合：normor / weather_tips / 温度趋势
│   │   ├── sunny/  sunny-1.png、sunny-2.png   晴
│   │   ├── rainy/  rainy-1.png、rainy-2.png   雨
│   │   ├── snow/   snow.png                  雪
│   │   ├── fog/    fog.png                   雾
│   │   └── others/                           空目录，预留兜底分类
│   └── tips_sound/                           到点播放的天气提示音（每类 01.m4a）
│       ├── sunny/ ├── rainy/ ├── snow/ ├── fog/ └── others/
│
├── Fout/                                     HarmonyOS Sans 字体 6 个字重（Regular 进 qrc，其余备用）
│   └── HarmonyOS_Sans_{Thin,Light,Regular,Medium,Bold,Black}.ttf
│
├── assets/                                   二进制资源
│   └── tray-icon.png                         系统托盘图标（qrc alias：tray-icon.png；PNG 为 Qt 内置格式，不依赖 qjpeg 插件）
│
└── cmake/
    └── CopySettingsIfMissing.cmake           部署脚本：目标不存在才复制（保住运行期配置）
```

## 运行期生成 / 更新的文件（不在仓库内，位于 exe 同级目录）

| 路径 | 谁写 | 说明 |
|---|---|---|
| `settings.json` | 设置界面、`AutoPoweroff`、`WeatherJudge` | 键：`password`、`startTime`、`averageTemperature`、`weatherUpdateTime`、`exePath`、`showWeatherSvg`、`weatherSvgDirectory` |
| `Auto_poweroff233/start_time.json` | `AutoPoweroff::saveStartTime()` | 开始时间的权威副本（设置界面改时间时写入）；缺失或非法时启动会回退读 `settings.json` 的 `startTime`，两处都取不到则当天不做任何到点动作 |
| `weather/weather.json` | `WeatherApi::fetchAnyangWeather()` | 每日抓取后覆盖（存明日预报），供到点判定天气 |
| `Auto_poweroff233/music233/`、`weather/{icons,picture,tips_sound}`、`Fout/`、`booth/booth.m4a` | CMake POST_BUILD | 构建时从仓库拷到 exe 目录 |

## 运行期数据流

```
设置界面改开始时间 ──► Auto_poweroff233/start_time.json ──┐
                          │                              │同步
                          ▼                              ▼
             AutoPoweroff 到点状态机            settings.json 的 startTime
                                                         │
                                                         ▼
                             WeatherApi：每天 startTime 前 3 分钟抓取
                                          ▼
                             weather/weather.json（明日预报）
                                          │
                    AutoPoweroff 到点 ──► WeatherJudge 判定 ──► weather/tips_sound/<分类>/01.m4a
                                          │
                                          ▼
                              settings.json（averageTemperature / weatherUpdateTime）
```

#天气提示语判定流程（weather/weather_judge.cpp）

`weather_judge.cpp` 的判定逻辑（原 `weather_background.cpp`：天气码 → 分类提示语 → 随机选一条）：

```mermaid
flowchart TD
    Start(["开始"]) --> LoadTips["加载并解析 **weathertips.json**"]
    LoadTips --> CheckTips{"解析成功且\n为JSON对象?"}
    CheckTips --> |"否"| WarnAndEnd1["qWarning() & 返回空字符串"]
    CheckTips --> |"是"| LoadWeather["加载并解析 **weather.json**"]
    LoadWeather --> CheckWeather{"解析成功且\n包含每日数据?"}
    CheckWeather --> |"否"| MatchCandidates["匹配候选提示语"]
    CheckWeather --> |"是"| CalcTodayAvg["计算**今日平均温度**\n(最高温 + 最低温) / 2"]
    CalcTodayAvg --> LoadSettings["加载并解析 **settings.json**"]
    LoadSettings --> CheckSettings{"解析成功且\n更新时间在±2天内?"}
    CheckSettings --> |"否"| MatchCandidates
    CheckSettings --> |"是"| CalcDiff["计算**温度差**\n(今日均温 - 历史均温)"]
    CalcDiff --> CheckTrend{"判断温度趋势"}
    CheckTrend --> |"温差 ≥ 6.0"| SetHeating["temperatureTrendKey = **Heating_up**"]
    CheckTrend --> |"温差 ≤ -6.0"| SetCooling["temperatureTrendKey = **Cooling_down**"]
    CheckTrend --> |"其他"| MatchCandidates
    SetHeating --> MatchCandidates
    SetCooling --> MatchCandidates
    
    MatchCandidates --> HasTrend{"存在温度趋势Key?"}
    HasTrend --> |"是"| GetTrendCandidates["从**weather_tips**获取\n对应趋势的候选列表"]
    HasTrend --> |"否"| CheckConditionCode{"条件码在100-199之间?"}
    
    GetTrendCandidates --> IsCandidatesEmpty{"候选列表为空?"}
    IsCandidatesEmpty --> |"否"| RandomSelect
    IsCandidatesEmpty --> |"是"| CheckConditionCode
    
    CheckConditionCode --> |"是"| GetNormalCandidates["获取 **normor_tips** (常规提示)"]
    CheckConditionCode --> |"否"| MapWeatherKey["根据条件码映射天气Key\n(300+:Rain, 400+:Snow, 500+:Fog)"]
    
    GetNormalCandidates --> CheckEmptyAgain{"候选列表仍为空?"}
    MapWeatherKey --> GetWeatherCandidates["从**weather_tips**获取\n对应天气的候选列表"]
    GetWeatherCandidates --> CheckEmptyAgain
    
    CheckEmptyAgain --> |"是"| ReturnEmpty["返回空字符串"]
    CheckEmptyAgain --> |"否"| RandomSelect["**随机选择**一条提示语"]
    
    RandomSelect --> ReturnTip["返回选中的提示语"]
    ReturnTip --> End((("END")))
    ReturnEmpty --> End
    WarnAndEnd1 --> End
```

