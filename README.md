# 一个普通的定时关机小 exe 文件
采用 Windows + Qt 6.9.1 + MinGW 64-bit + CMake + C++17 开发

https://github.com/solusy233/ays2z-233

# 功能
1. 定时关机
2. 对 exe 关闭的监测（关闭后会播放音频）
3. 播报天气

# 使用
运行 exe 即可

// 本项目采用大量 AI 的代码（80%左右）
// 我做了模块化架构，有一定可维护性（如果有人维护的话）
// 练手项目不要喷我

# 项目结构

```text
ays2z-233/
├── CMakeLists.txt               # Qt/CMake 构建入口
├── CMakeLists.txt.user          # Qt Creator 自动生成的本地配置
├── README.md                    # 项目说明文档
├── main.cpp                     # 程序入口
├── resources.qrc                # Qt 资源文件
├── settings.json                # 运行时配置文件
├── cmake/
│   └── CopySettingsIfMissing.cmake
│
├── aboutwindows/                # 主界面与设置界面相关模块
│   ├── mainwindows/
│   │   ├── mainwindow.ui
│   │   ├── mainwindow.h
│   │   ├── mainwindow.cpp
│   │   └── app_icon.rc
│   └── settings/
│       ├── settingswindow.ui
│       ├── settingswindow.h
│       └── settingswindow.cpp
│
├── Auto_poweroff233/            # 定时关机模块
│   ├── Auto_poweroff.h
│   ├── Auto_poweroff.cpp
│   └── music233/
│       └── 歌单.txt
│
├── booth/                       # EXE 关闭监测模块
│   ├── booth.h
│   ├── booth.cpp
│   └── zhangtai.wav            # 关闭检测后的提示音
│
├── Easter-egg/                  # 彩蛋模块
│   ├── Easter-egg.h
│   └── Easter-egg.cpp
│
├── Fout/                        # 字体资源目录
│   └── (字体文件)
│
├── weather/                     # 天气模块
│   ├── weatherapi.h
│   ├── weatherapi.cpp
│   ├── weather_background.h
│   ├── weather_background.cpp
│   ├── weatherwindow.ui
│   ├── weatherwindow.h
│   ├── weatherwindow.cpp
│   ├── weather.json
│   ├── weathertips.json
│   ├── icons/
│   │   └── (天气图标资源)
│   ├── picture/
│   │   └── (天气背景图片)
│   └── tips_sound/
│       └── (天气提示音)
│
├── build/                       # CMake/Qt 构建输出目录
│   ├── CMakeCache.txt
│   ├── Makefile
│   ├── CMakeFiles/
│   └── ...
│
└── ...
```

# 模块说明
- `main.cpp`：程序入口，负责启动界面与核心逻辑。
- `aboutwindows`：主窗口与设置窗口的 UI 层。
- `Auto_poweroff233`：定时关机功能模块。
- `booth`：检测 exe 关闭状态，并在关闭后播放提示音。
- `weather`：获取天气信息并展示天气界面和提示。
- `Easter-egg`：彩蛋功能模块。
- `build`

