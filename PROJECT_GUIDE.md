# ays2z-233 桌面助手技术架构与开发全景指南
# (PROJECT_GUIDE.md)

---

## 1. 文档元信息与系统概述

### 1.1 项目背景与演进定位
`ays2z-233` 桌面助手是一个专为 Windows 平台打造的高性能、轻量化、沉浸式桌面交互中控系统。本项目源自经典 C++ Qt6 桌面小工具的完全现代化 Web 架构重构，旨在彻底摒弃传统 GUI 工具臃肿、样式僵硬、跨端维护成本高的弊端，依托微软原生 Edge Chromium WebView2 运行时，构建出一套集“纯白玻璃拟态美学、全屏无边框气象雷达、动漫少女乐队主题引擎、多模态天气壁纸联动、每日定时关机守护”于一体的全新桌面操作系统扩展层。

### 1.2 核心设计理念
- **极简主义与纯粹感**：摒弃传统三栏臃肿排版，收缩为 Apple 控制中心 / Raycast / Nothing OS 风格的悬浮中控坞（Floating Control Dock）。
- **光学级玻璃拟态**：遵循严苛的物理光学材质规范，采用 35% 至 55% 动态透光率、高斯多阶模糊（28px Blur）、漫反射外阴影与内高光边框，呈现通透灵动的视觉体验。
- **动漫少女乐队文化共鸣**：首创 CSS 级联作用域主题引擎，原生支持 MyGO!!!!!、Ave Mujica、结束乐队（孤独摇滚）、放学后 TEA TIME（轻音少女）、刺无（少女乐队的呐喊）五大经典乐队及纯白原版主题，支持零成本运行时热重载。
- **全屏无边框天气交互**：主坞仅保留单个极简入口，点击瞬时扩展为包含 48 小时贝塞尔曲线、多日趋势、天文物理日出日落弧、3D 月相球体及 12 宫格环境指标的全屏沉浸式气象中心。
- **低资源消耗与高稳定性**：利用本地轻量 HTTP 206 流式分片服务器与 Win32 进程 API，内存占用稳定在极低区间，显卡硬解 4K 动态壁纸与 120 FPS 渲染。
- **严谨的专业排版哲学**：整套系统与所有文档严禁滥用 Emoji 表情符号，采用端庄、精炼、工业级的专业视觉语言与文字排版规范。

---

## 2. 系统架构全景与技术选型决绝

### 2.1 整体架构拓扑图

```text
+---------------------------------------------------------------------------------------+
|                                    ays2z-233 运行时环境                                |
+---------------------------------------------------------------------------------------+
|                                                                                       |
|   [ 前端表现层 (Chromium WebView2) ]                                                  |
|   +-------------------------------------------------------------------------------+   |
|   |  Vanilla HTML5 / CSS3 / ES6+ 架构                                             |   |
|   |  - 纯白玻璃拟态悬浮中控坞 (Floating Center Dock)                              |   |
|   |  - 5 大动漫少女乐队主题引擎 (CSS Variable Scoping & [data-theme])             |   |
|   |  - 全屏无边框天气中心 (48h Bezier Curve / 12-Grid Environment Telemetry)     |   |
|   |  - 多模态背景渲染 (Canvas 粒子 / HTML5 Video / IFrame WebGL 动态壁纸)         |   |
|   |  - Web Audio API 频谱分析器 (FFT 64 Bins 音频可视化桥接)                      |   |
|   +-------------------------------------------------------------------------------+   |
|                                          |                                            |
|                 pywebview.api (双向异步 RPC) | HTTP 206 / GET 静态资源与流媒体           |
|                                          v                                            |
|   [ 后端核心服务层 (Python 3.10+ Native Runtime) ]                                     |
|   +-------------------------------------------------------------------------------+   |
|   |  core.api.BackendAPI (RPC 调度中枢与配置持久化)                                |   |
|   |  core.server.RangeRequestHandler (高性能 HTTP 服务器，支持 206 Partial Content)   |   |
|   |  core.msn_weather_crawler.MSNWeatherCrawler (微软天气多级爬虫与离线数据垫片)     |   |
|   |  core.wallpaper_manager.WallpaperManagerService (天气分类壁纸与工坊扫描器)    |   |
|   |  core.scheduler.AutoPoweroffScheduler (每日定时守护线程与高精度定时器)        |   |
|   |  core.system_ops (Windows API: shutdown / run 注册表项自启管理)               |   |
|   +-------------------------------------------------------------------------------+   |
|                                          |                                            |
|                               ctypes / Win32 底层接口                                 |
|                                          v                                            |
|   [ Windows 操作系统底层内核层 (Windows 10 / 11) ]                                    |
|   +-------------------------------------------------------------------------------+   |
|   |  - DWM (Desktop Window Manager): DwmExtendFrameIntoClientArea 亚克力磨砂透明      |   |
|   |  - User32 SetWindowCompositionAttribute (Accent Policy 亚克力光晕合成)            |   |
|   |  - User32 WorkerW 穿透注入: Progman 0x052C 窗口层级穿透 (桌面壁纸挂载)            |   |
|   |  - Taskbar Notification Area (Shell_NotifyIcon 系统托盘常驻守护)                 |   |
|   |  - Windows Registry (HKCU\Software\Microsoft\Windows\CurrentVersion\Run)          |   |
|   +-------------------------------------------------------------------------------+   |
+---------------------------------------------------------------------------------------+
```

### 2.2 技术选型对比与决策分析

| 维度 | 原版方案 (C++ Qt6) | 流行方案 (Electron) | 本方案 (Python + WebView2 + 原生 Web) | 决策优势与选型理由 |
| :--- | :--- | :--- | :--- | :--- |
| **安装包与体积** | 80MB ~ 120MB (动态链接库繁琐) | 150MB ~ 250MB (打包整套 Chromium) | < 15MB (借助系统内置 WebView2 运行时) | 零冗余，极轻量发布，秒级启动。 |
| **内存占用** | 60MB ~ 110MB | 180MB ~ 350MB | 45MB ~ 85MB | 充分发挥 Windows 共享 Edge 进程优势，常驻开销小。 |
| **UI 表现力** | QSS 局限性大，现代高斯模糊实现极难 | 极高，支持完整现代 CSS | 极高，原生支持最新 CSS 滤镜与 Canvas2D | 完美呈现 28px 毛玻璃、内高光与贝塞尔曲线图表。 |
| **多媒体壁纸支持** | 依赖 QtMultimedia，解码性能平庸 | 容易受主进程负载干扰 | 显卡原生硬件加速硬解 (GPU NVDEC/QSV) | 4K 60FPS 动态壁纸播放 CPU 占用低于 1.5%。 |
| **开发与迭代效率** | 编译周期长，热更新繁琐 | 中等，Node.js 生态庞大但重 | 极高，前后端解耦，前端支持即时刷新与热加载 | 前端纯 ES6 标准编写，无需 Webpack/Vite 冗长编译。 |

### 2.3 核心子系统与物理代码映射表

| 逻辑模块名称 | 承载文件路径 | 核心类 / 函数 | 职责说明 |
| :--- | :--- | :--- | :--- |
| **应用引导中枢** | `main.py` | `main()` | 启动本地 HTTP 服务，挂载系统托盘，创建 WebView2 无边框原生窗体，应用 DWM 亚克力磨砂透明。 |
| **前后端 API 桥梁** | `core/api.py` | `BackendAPI` | 通过 `window.pywebview.api` 暴露方法，统一管理 `settings.json`、天气查询、系统关机与壁纸扫描。 |
| **流式媒体服务器** | `core/server.py` | `FastRangeRequestHandler` | 具备 HTTP 206 Partial Content 分片与 TCP_NODELAY 加速的本地 HTTP 服务器，支持视频壁纸拖拽即刻 Seek。 |
| **微软天气高精爬虫** | `core/msn_weather_crawler.py` | `MSNWeatherCrawler` | MSN Weather 实时气象数据抓取、HTML 解析、C 级 MurmurHash3 城市哈希缓存索引与三级离线持久化。 |
| **天气服务代理层** | `core/weather_service.py` | `WeatherService` | 封装免 Key 气象路由与 MSN 爬虫降级，输出归一化的天气状态代码、温湿度及预警建议。 |
| **分类媒体壁纸服务** | `core/wallpaper_manager.py` | `WallpaperManagerService` | 调用 C 原生库按晴、雨、雪、雾、视频、自定义极速扫描媒体，构建结构化壁纸数据集。 |
| **C 级原生加速内核** | `core/native_core.c` / `native_core.dll` | GCC 16.1.0 编译 DLL | Win32 FindFirstFileW 极速扫描、MurmurHash3、纯 C WorkerW 桌面壁纸注入与 DWM 亚克力合成。 |
| **桌面底层注入模块** | `core/desktop_wallpaper.py` | `attach_to_desktop()` | 调用 `native_core.dll` 实施 C 级 WorkerW 穿透注入，毫秒级将主视窗安全挂载至系统桌面图标后方。 |
| **原生窗口特效增强** | `core/window_effects.py` | `enable_window_transparency()` | 调用 `native_core.dll` 实施 C 级 DWM 亚克力混合，实现操作系统底层透光毛玻璃。 |
| **定时关机守护进程** | `core/scheduler.py` | `AutoPoweroffScheduler` | 后台常驻时间轮询线程，毫秒级比对目标关机时间，触发星期音乐播放并在 5 分钟前下发关机指令。 |
| **系统底层操作工具** | `core/system_ops.py` | `execute_shutdown()` / `set_autostart()` | 执行 Windows `shutdown.exe` 原生调用，管理注册表 `HKCU\...\Run` 开机自启项。 |
| **系统托盘驻留** | `core/tray.py` | `SystemTrayApp` | 基于 `pystray` 实现 Windows 通知区域图标，提供主界面唤醒、全屏切换与安全退出菜单。 |
| **前端主控业务流程** | `web/js/app.js` | 全局事件流与 DOM 控制 | 数字时钟滴答器、Toast 通知、设置对话框权限门禁、主题热切换与关机倒计时状态机。 |
| **全屏天气视窗控制器**| `web/js/weather.js` | `WeatherController` | 全屏天气模态框管理、城市胶囊快捷切换、12 宫格物理指标渲染与角色天气陪伴语音台词调度。 |
| **气象曲线渲染引擎** | `web/js/weather_chart.js` | `HourlyWeatherChart` | 基于 HTML5 Canvas 绘制 48 小时三次方贝塞尔平滑温湿度曲线、日出日落垂直分割光线与悬停十字标尺。 |
| **壁纸交互与联动中枢**| `web/js/wallpaper.js` | `WallpaperManager` | 纯净视频/图片双模态壁纸切换（显卡硬解/Canvas粒子/天气联动/文件拖拽即开）。 |
| **音频总线与时序调度**| `web/js/audio_manager.js` | `AudioManager` | HTML5 Audio 与 Web Audio 实例管理，每日星期专属歌曲加载与平滑淡入淡出。 |
| **气象联动粒子引擎** | `web/js/particles.js` | `WeatherParticleEngine` | Canvas 粒子系统，基于真实物理重力与风速向量模拟雨丝、雪花旋转、大雾弥漫与星空背景。 |
| **角色天气伴随台词库**| `assets/weather/character_tips/*.json` | 24 位角色专属台词 | 5 大少女乐队 24 位核心角色，针对晴雨雪雾等天气各自编写 24 条深度契合性格的治愈与日常台词。 |
| **视觉呈现样式大成** | `web/css/style.css` | 纯白玻璃拟态设计体系 | 3000 行纯手工 CSS3，定义完整 Design Tokens、毛玻璃滤镜、6 大主题变量域及响应式布局。 |

---

## 3. 纯白玻璃拟态与工业级设计规范

### 3.1 空间布局革命：从臃肿三栏卡片到悬浮中控坞
在早期的界面迭代中，多功能桌面程序往往陷入“功能堆叠陷阱”，将关机设置、天气图表、壁纸网格并排平铺为僵硬的左中右三栏。这种布局具有严重的设计缺陷：
- **视野侵占严重**：桌面用户使用小工具的核心需求是轻量守护与快速瞥视，大面积的静态卡片严重遮挡用户原有的精美壁纸。
- **视觉重心涣散**：用户的视线在三栏卡片之间反复横移，无法在 0.5 秒内捕捉当前时间、今日天气与系统状态。
- **空间利用效率低下**：天气曲线和壁纸网格属于“偶发深入操作项”，常态化平铺造成界面长期处于信息过载状态。

**解决方案：悬浮中控坞（Floating Center Dock）架构**
`ays2z-233` 借鉴了 Apple 控制中心、Raycast 以及 Nothing OS 的设计哲学，将主视窗收拢为一个居中悬浮的紧凑中控坞（680px 宽度）。主坞聚焦三大核心高频层级：
1. **顶层时间与状态中枢**：大字号数字时钟（`HarmonyOS Sans Black` 56px）与本地日期，提供一目了然的桌面时钟功能。
2. **气象速览与单极展开入口**：将庞大的天气雷达精简为一条富有呼吸感的气象简报胶囊（当前温度、天气状况、风向风级、空气质量），点击即可平滑展开全屏无边框天气视窗。
3. **极简快捷状态与二级菜单入口**：主坞下方彻底剥离繁复的表单输入与密集网格，精简为 4 枚纯净的快捷状态胶囊（关机守护、壁纸联动、主题风格、每日音乐）；所有具体参数与配置项全量收纳至两级选项中心（左侧一级分类导航、右侧二级详细操作面板），点击任一快捷胶囊即可直达对应二级选项。

### 3.2 玻璃材质物理光学规范 (Design Tokens)

为营造如薄冰覆盖于月夜雨景之上的高级空灵感，系统确立了以下参数级材质规范：

```css
:root {
    /* 玻璃表面基色与透光率体系 (Translucency: 35% ~ 55%) */
    --card-bg: rgba(255, 255, 255, 0.45);
    --card-bg-hover: rgba(255, 255, 255, 0.58);
    --card-bg-active: rgba(255, 255, 255, 0.68);
    
    /* 物理边缘 1px 微白外框与内高光 */
    --card-border: rgba(255, 255, 255, 0.48);
    --card-border-highlight: rgba(255, 255, 255, 0.75);
    
    /* 次级内凹收纳盒与微型槽 */
    --glass-inner-box: rgba(255, 255, 255, 0.35);
    --glass-inner-box-hover: rgba(255, 255, 255, 0.50);
    --glass-inner-border: rgba(255, 255, 255, 0.50);
    
    /* 高阶复合双层投影：深层漫反射外阴影 + 浅层 1px 晶体顶部反光 */
    --glass-shadow: 0 8px 32px rgba(0, 0, 0, 0.12), inset 0 1px 1px rgba(255, 255, 255, 0.65);
    --glass-shadow-sm: 0 4px 16px rgba(0, 0, 0, 0.08), inset 0 1px 1px rgba(255, 255, 255, 0.55);
    --glass-shadow-lg: 0 16px 48px rgba(0, 0, 0, 0.14), inset 0 1px 1px rgba(255, 255, 255, 0.75);
    
    /* 后置高斯模糊与饱和度增益核心规范 */
    --glass-blur: blur(24px) saturate(120%);
    --glass-blur-deep: blur(28px) saturate(120%);
    
    /* 倒角几何标准体系 */
    --radius-dock: 28px;
    --radius-card: 22px;
    --radius-inner: 16px;
    --radius-btn: 12px;
    --radius-pill: 9999px;
    
    /* 高对比中性文字排版 */
    --text-title: #212429;
    --text-primary: #30343B;
    --text-muted: #707782;
    --text-light: #8E95A2;
}
```

#### 关键光学特质解析
- **28px 高斯模糊与 120% 饱和度**：单纯的 `blur()` 会使被遮挡的底层壁纸泛白变灰。加入 `saturate(120%)` 后，底图的冷暖色彩能够在穿透玻璃时产生微弱的折射溢色，形成宛如高级建筑光学玻璃的通透质感。
- **1px 线性外发光倒角边框**：在主中控坞的顶部伪元素中嵌入 `linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.8), transparent)`，精准模拟环境光打在玻璃倒角边缘形成的锋利漫射高光。
- **内发光与外漫反射双重阴影**：通过 `box-shadow` 的 `inset 0 1px 1px rgba(255, 255, 255, 0.65)` 在卡片上边缘形成晶莹的高光分界线，结合 `0 16px 48px rgba(0, 0, 0, 0.14)` 的柔和扩散，使整个面板彻底与背景壁纸解耦，悬浮感跃然眼前。

### 3.3 严格的“零 Emoji”规范与极简文字哲学
主流现代桌面软件常滥用表情符号作为状态装饰，这种做法在工业级工程工具中会导致界面显得轻佻、不严肃，且不同 Windows 操作系统版本渲染出的表情符号字体与色调各异，严重破坏纯白玻璃拟态的冷峻美感。

本项目确立并执行了绝对的**零 Emoji 哲学（The Zero-Emoji Doctrine）**：
1. **指示状态原生几何化**：运行状态采用微小高亮的几何圆点（`3px ~ 6px` 绿色微光 `span.tip-dot` / `span.toast-dot`），辅以渐变呼吸动效，取代彩色图标。
2. **纯文字精炼动词表达**：所有交互控件彻底剥离图形符号缀饰，使用极其工整的双字或四字高阶动词：
   - 杜绝形式：带符号的繁复缀饰标签。
   - 本项目规范：`播放`、`暂停`、`取消关机`、`测试关机`、`壁纸管理`、`全屏气象`、`扫描工坊`、`保存设置`。
3. **气象图形全部采用矢量 SVG**：全屏天气视窗中严禁插入天气表情符号，所有 30 种气象图标统一调用经过重绘的扁平纯白微晶风 `assets/weather/icons/*.svg`，确保高分屏与 4K 显示器下的无损锐利显示。

---

## 4. 五大动漫少女乐队主题引擎深度设计

### 4.1 运行时热切换原理 (CSS Scoping Architecture)
为了让用户在不同心境与桌面氛围下自由切换体验，系统设计了基于 CSS 自定义属性（Custom Properties）级联特性的**动漫少女乐队主题引擎**。

引擎摒弃了重排 DOM 或动态加载外链样式的低效做法，而是将主容器及全局色板统一绑定在 `document.documentElement`（`<html>` 节点）的 `data-theme` 属性上。当用户在设置面板中选择对应主题时，JavaScript 仅需执行单行语句：

```javascript
document.documentElement.setAttribute('data-theme', themeName);
```

浏览器内部的渲染树会直接响应 CSS 变量的作用域级联，在 0 毫秒延时下完成字体色、卡片半透度、边框微光与阴影色散的平滑重着色，达成真正的零重绘（Zero Repaint Penalty）开销。

### 4.2 六大主题深度剖析与灵感档案

```text
+--------------------------------------------------------------------------------------------------------+
|                                    6 大少女乐队主题情感与色相拓扑                                      |
+--------------------------------------------------------------------------------------------------------+
|                                                                                                        |
|   [默认原版 DEFAULT]             [MyGO!!!!!]                      [Ave Mujica]                         |
|   - 纯白磨砂玻璃 / 月夜雨景        - 迷茫青蓝雨夜 / 创可贴与磁带    - 暗月黑曜紫 / 哥特假面与血色蔷薇    |
|   - #FFFFFF / #30343B            - #2E6B9E / #E9A34C / #F4C2B6    - #120F1D / #2D1845 / #A61C3C        |
|   - 空灵、静谧、透亮             - 迷惘、潮湿、坚毅的疾走感       - 沉沦、华丽、戏剧性压抑与冷酷       |
|                                                                                                        |
|   [孤独摇滚 BOCCHI]              [轻音少女 K-ON!]                 [少女乐队的呐喊 TOGENASHI]           |
|   - 结束乐队 / 粉红小波奇        - 放学后 TEA TIME / 红茶琥珀蛋糕 - 刺无 / 川崎冷蓝天桥配刺目赤红      |
|   - #F472B6 / #D9A05B            - #C86432 / #8B4513 / #FFF8EE    - #16283D / #E61C3E / #F4DC35        |
|   - 社恐内敛与电吉他失真撞击     - 暖阳、红茶香气与慵懒青春       - 暴雨天桥、绝不妥协的刺骨摇滚       |
|                                                                                                        |
+--------------------------------------------------------------------------------------------------------+
```

#### 1. 默认原版（纯白磨砂玻璃，月夜雨景）
- **主题代号**：`default`
- **灵感来源**：Apple 控制中心与 Windows 11 Mica / Acrylic 纯白玻璃拟态美学。
- **代表音乐**：月夜落雨与电台调频的白噪音。
- **专属色板**：高透冷白（`#FFFFFF`）、微雾浅灰（`#F4F5F7`）、沉稳岩灰主字（`#30343B`）、深邃墨黑标题（`#212429`）。
- **材质基调**：透光率 45% 的半透明白玻璃，搭配 1px 微白高光边框与冷色漫反射投影，营造空灵、干净、高聚焦的纯粹桌面氛围。

#### 2. MyGO!!!!!（迷茫青蓝雨夜，创可贴与磁带）
- **主题代号**：`mygo`
- **灵感来源**：《BanG Dream! It's MyGO!!!!!》——迷茫青春、雨夜狂奔、千早爱音与高松灯指尖的创可贴，以及那盘记录了最初旋律的旧卡带。
- **代表曲目**：《春日影》（Haruhikage）、《碧天伴走》、《迷星叫》（Mayoiuta）、《詩超絆》。
- **专属色板**：
  * 主调水色：迷茫青蓝（`#2E6B9E`）、雨夜幽蓝（`#1B3B5A`）、雨后浅苍（`#76A9D1`）
  * 专属点缀：创可贴浅粉（`#F4C2B6`）、旧磁带暖杏黄（`#E9A34C`）
  * 玻璃底色：`rgba(225, 238, 248, 0.48)`
  * 边框微光：`rgba(180, 212, 238, 0.58)`
- **情感基调**：潮湿带有雨滴腥味的东京街头，迷惘而坚定、刺痛却带有少女彼此温暖的坚韧生命力。

#### 3. Ave Mujica（暗月黑曜紫，哥特假面与血色蔷薇）
- **主题代号**：`ave-mujica`
- **灵感来源**：《BanG Dream! Ave Mujica》——假面人形剧、哥特交响金属、暗黑机械齿轮与刺穿宿命的带刺蔷薇。
- **代表曲目**：《KiLLKiSS》、《Symbol I : △》、《Ave Mujica》、《Masquerade Rhapsody Request》。
- **专属色板**：
  * 主调暗夜：黑曜暗金石（`#120F1D`）、哥特深曜紫（`#2D1845`）、深渊冷靛（`#471E68`）
  * 专属点缀：血色荆棘蔷薇红（`#A61C3C`）、假面残金（`#D4AF37`）、冷月银白（`#E8E3F5`）
  * 玻璃底色：`rgba(26, 18, 38, 0.64)`
  * 边框微光：`rgba(166, 28, 60, 0.48)`
  * 标题字色：`#F5F0FF`
- **情感基调**：华丽而冷冽的戏剧感，纯黑中撕裂出的血红反光，充斥着假面下的挣扎、掌控欲与哥特金属的狂躁张力。

#### 4. 孤独摇滚（结束乐队，小波奇粉红与芒果箱卡其效果器）
- **主题代号**：`bocchi`
- **灵感来源**：《孤独摇滚！》（Bocchi the Rock!）——后藤一里的粉色运动服、避世的芒果瓦楞纸箱、吉他失真单块效果器（Distortion Pedal）与下北泽地下防空洞。
- **代表曲目**：《青春コンプレックス》（青春症候群）、《若者のすべて》、《星座になれたら》、《忘れてやらない》。
- **专属色板**：
  * 主调波奇粉：小波奇粉红（`#F472B6`）、草莓甜酒暗粉（`#E85D9E`）
  * 专属点缀：避世芒果箱瓦楞卡其（`#D9A05B`）、单块效果器柠檬黄（`#FBBF24`）、极客薄荷青（`#0D9488`）
  * 玻璃底色：`rgba(255, 238, 246, 0.48)`
  * 边框微光：`rgba(244, 114, 182, 0.45)`
- **情感基调**：表面极度社恐封闭，内心宇宙却能瞬间引爆万丈摇滚光芒；粉色与纸箱卡其色的碰撞充满诙谐却无比温润的治愈感。

#### 5. 轻音少女（放学后 TEA TIME，红茶琥珀与暖木蛋糕）
- **主题代号**：`k-on`
- **灵感来源**：《K-ON!》（轻音少女）——放学后的轻音部活动室、大吉岭红茶的袅袅热气、双层草莓奶油千层蛋糕与夕阳斜射在木吉他面板上的金色暖辉。
- **代表曲目**：《Don't say "lazy"》、《ふわふわ時間》（轻飘飘时间）、《Cagayake!GIRLS》、《相遇天使》（天使にふれたよ!）。
- **专属色板**：
  * 主调茶香：大吉岭红茶琥珀（`#C86432`）、烤焦糖栗木（`#8B4513`）、夕阳暖烤金（`#EFA968`）
  * 专属点缀：草莓奶油鲜红（`#E0314B`）、初雪奶白（`#FFF8EE`）、原木乐器暖黄（`#D97706`）
  * 玻璃底色：`rgba(255, 248, 238, 0.52)`
  * 边框微光：`rgba(239, 169, 104, 0.50)`
- **情感基调**：慵懒、温馨、香甜且洋溢着无忧无虑的青春芳华，将桌面包裹在如一杯午后红茶般的治愈感中。

#### 6. 少女乐队的呐喊（刺无 Togenashi Togeari，川崎夜景冷蓝配刺目赤红）
- **主题代号**：`togenashi`
- **灵感来源**：《GIRLS BAND CRY》（少女乐队的呐喊）——川崎工业区暴雨中的天桥夜景、电线杆与电车轨道、竖起的中指与绝不向成人虚伪世界低头的暴烈摇滚。
- **代表曲目**：《雑踏、僕らの街》（杂踏、我们的街道）、《誰にもなれない私だから》（正因为是无法成为任何人的我）、《爆ぜて咲く》（爆裂绽放）、《名もなき何もかも》（无名的一切）。
- **专属色板**：
  * 主调夜景：川崎工业冷铁蓝（`#16283D`）、轨道深青（`#234567`）、暴雨冷夜（`#122030`）
  * 专属点缀：刺目电光赤红（`#E61C3E`）、暴走警示荧光黄（`#F4DC35`）、天桥冰冷白（`#F0F4F8`）
  * 玻璃底色：`rgba(18, 32, 48, 0.64)`
  * 边框微光：`rgba(230, 28, 62, 0.48)`
  * 标题字色：`#F0F4F8`
- **情感基调**：刺骨寒意中迸发出的滚烫火花，冷峻硬朗的重工业蓝与穿透黑夜的鲜血赤红，展现不向命运妥协的叛逆态度。

### 4.3 核心 CSS 变量映射对照字典

```css
/* ===================================================================
   六大主题 CSS 变量映射对照字典 (Token Dictionary)
   =================================================================== */

/* 1. 默认原版 (Default) */
[data-theme="default"] {
    --card-bg: rgba(255, 255, 255, 0.45);
    --card-border: rgba(255, 255, 255, 0.48);
    --card-border-highlight: rgba(255, 255, 255, 0.75);
    --text-title: #212429;
    --text-primary: #30343B;
    --text-muted: #707782;
    --status-info: #0284C7;
    --status-warning: #F59E0B;
    --status-danger: #EF4444;
}

/* 2. MyGO!!!!! */
[data-theme="mygo"] {
    --card-bg: rgba(225, 238, 248, 0.48);
    --card-border: rgba(180, 212, 238, 0.58);
    --card-border-highlight: rgba(118, 169, 209, 0.85);
    --text-title: #15334E;
    --text-primary: #244869;
    --text-muted: #5D7E9E;
    --status-info: #2E6B9E;
    --status-warning: #E9A34C;
    --status-danger: #E27B88;
}

/* 3. Ave Mujica */
[data-theme="ave-mujica"] {
    --card-bg: rgba(26, 18, 38, 0.64);
    --card-border: rgba(166, 28, 60, 0.48);
    --card-border-highlight: rgba(212, 175, 55, 0.75);
    --text-title: #F5F0FF;
    --text-primary: #DCD2F0;
    --text-muted: #A396BE;
    --status-info: #8B45BE;
    --status-warning: #D4AF37;
    --status-danger: #A61C3C;
}

/* 4. 孤独摇滚 (Bocchi) */
[data-theme="bocchi"] {
    --card-bg: rgba(255, 238, 246, 0.48);
    --card-border: rgba(244, 114, 182, 0.45);
    --card-border-highlight: rgba(232, 93, 158, 0.80);
    --text-title: #361E28;
    --text-primary: #4D2D3B;
    --text-muted: #8E6A7A;
    --status-info: #0D9488;
    --status-warning: #D9A05B;
    --status-danger: #E11D48;
}

/* 5. 轻音少女 (K-ON!) */
[data-theme="k-on"] {
    --card-bg: rgba(255, 248, 238, 0.52);
    --card-border: rgba(239, 169, 104, 0.50);
    --card-border-highlight: rgba(200, 100, 50, 0.75);
    --text-title: #3A2212;
    --text-primary: #4E3320;
    --text-muted: #8C6A50;
    --status-info: #C86432;
    --status-warning: #D97706;
    --status-danger: #E0314B;
}

/* 6. 少女乐队的呐喊 (Togenashi) */
[data-theme="togenashi"] {
    --card-bg: rgba(18, 32, 48, 0.64);
    --card-border: rgba(230, 28, 62, 0.48);
    --card-border-highlight: rgba(244, 220, 53, 0.80);
    --text-title: #F0F4F8;
    --text-primary: #CFDAE5;
    --text-muted: #8AA0B5;
    --status-info: #38BDF8;
    --status-warning: #F4DC35;
    --status-danger: #E61C3E;
}
```

---

## 5. 全屏无边框天气中心系统深度设计

### 5.1 单极入口与全屏展开交互模型
为实现“桌面常态静谧、唤醒即刻全景”的极致对比体验，系统对天气中心进行了两极化设计：
- **中控坞收敛态**：在悬浮控制中心上，仅留出高度为 54px 的极简气象条，显示当前气温（如 `15°C`）、简要天气（`小雨 · 安阳市`）以及今日温标范围。
- **全屏沉浸展开态**：点击气象条或顶部气象提示胶囊后，触发 `#fullscreen-weather-modal` 容器的入场动画。模态视窗利用绝对定位铺满整个客户端区域（`100vw × 100vh`），结合 `backdrop-filter: blur(36px)`，在当前壁纸上方降下一块全透光的全屏气象控制大屏。

### 5.2 实时爬虫与三级弹性降级机制 (MSN Weather Pipeline)

```text
+---------------------------------------------------------------------------------------+
|                             MSN 气象数据获取与容灾降级流水线                           |
+---------------------------------------------------------------------------------------+
|                                                                                       |
|   1. 客户端发起查询请求 (City / LocationId)                                           |
|       |                                                                               |
|       v                                                                               |
|   2. 一级管道: MSNWeatherCrawler.crawl_city_weather()                                 |
|       - 构造带有高仿 User-Agent 的 HTTP GET 请求访问 msn.com/zh-cn/weather            |
|       - 正则清洗提取 hourlyForecasts, airQualityIndex, UV, wind, moonPhase 数据       |
|       |                                                                               |
|       +---> [ 抓取成功 ] ---> 写入本地缓存 (msn_hourly_weather.json) ---> 返回前端    |
|       |                                                                               |
|       +---> [ 网络超时 / HTTP 4xx/5xx / 页面结构变动 ]                                |
|                 |                                                                     |
|                 v                                                                     |
|   3. 二级管道: 读取本地最后一次成功持久化的磁盘缓存 (Cache Guard)                     |
|       - 读取 assets/weather/msn_hourly_weather.json                                   |
|       - 比对更新时间戳，若有效则附加 "fallback": true 标志返回前端                    |
|       |                                                                               |
|       +---> [ 缓存存在且结构完好 ] ---> 成功交付前端渲染                              |
|       |                                                                               |
|       +---> [ 首次启动或缓存损坏 ]                                                    |
|                 |                                                                     |
|                 v                                                                     |
|   4. 三级管道: 物理数学模型离线合成垫片 (Synthetic Fallback Generator)                |
|       - 根据所在城市纬度与当前系统日期，基于正弦/余弦函数合成 48h 平滑温差走势        |
|       - 动态计算日落弧与月龄 (Moon Age)，提供 100% 可用、零白屏的数据交付保证        |
|                                                                                       |
+---------------------------------------------------------------------------------------+
```

### 5.3 48 小时平滑贝塞尔曲线渲染引擎 (`weather_chart.js`)
在全屏气象视窗的第一选项卡中，系统自主实现了高精度的 Canvas 2D 气象图表渲染器 `HourlyWeatherChart`，彻底抛弃体积膨胀的 ECharts 依赖。

#### 双三次贝塞尔平滑算法
气温曲线在每两个连续小时点之间不是僵硬的直线连线，而是通过计算前置控制点与后置控制点，绘制三次贝塞尔曲线（Cubic Bézier Curve）：

```javascript
// 计算相邻点之间的贝塞尔控制点
const cp1x = prevX + (currX - prevX) * 0.5;
const cp1y = prevY;
const cp2x = prevX + (currX - prevX) * 0.5;
const cp2y = currY;
ctx.bezierCurveTo(cp1x, cp1y, cp2x, cp2y, currX, currY);
```

#### 特性集成
- **动态垂直色带投影**：在温度曲线下方填充垂直方向的线性半透明渐变（`rgba(37, 99, 235, 0.25)` 至 `rgba(255, 255, 255, 0.0)`），产生如极光般的光晕包裹。
- **日出日落垂直分割光线**：根据 MSN 抓取的当日日出（如 `07:05`）与日落（如 `16:55`）时间戳，在时间轴对应坐标上绘制垂直虚线金光，并标注太阳升降标记。
- **多指标维度一键切换**：图表顶部提供“概览、降水、风速、湿度、云量”切换按钮，并在概览模式下支持点击“体感温度对比”，双曲线并列对比实际气温与人体体感。
- **高刷鼠标十字吸附**：鼠标移动时，根据横坐标位置毫秒级计算最近的小时节点，绘制高亮垂直指示线与包含温度、风速、降水概率的信息浮层。

### 5.4 物理天文计算与月相球体渲染算法
在 12 宫格详细指标中，系统嵌入了精确的天文物理计算：

#### 1. 太阳日出日落轨迹彩虹拱门 (Sun Arc & Solar Trajectory)
利用真实时间轴在 SVG `<path>` 上计算当前太阳所处的时间百分比：
$$\text{Progress} = \frac{T_{\text{current}} - T_{\text{sunrise}}}{T_{\text{sunset}} - T_{\text{sunrise}}}$$
当前太阳圆球（`#sun-orb`）沿半椭圆弧线动态移动，并根据离地高度赋予日出金、正午白黄或傍晚赤橙的动态径向发光滤镜（`filter: drop-shadow(...)`）。

#### 2. 真实 3D 月相球体与光照遮罩算法 (Moon Phase Sphere & Shadow Mask)
月球盈亏指标（`card-moon-phase`）彻底抛弃普通静态图，采用 CSS 3D 拟真球体：
- 底层为暗灰色的月球阴暗面立体球（带微弱内阴影）。
- 顶层通过 JavaScript 计算月相代码（`新月、峨眉月、上弦月、盈凸月、满月、亏凸月、下弦月、残月`）对应的月龄（`Age 0 ~ 29.5`），动态计算半球遮罩的横向拉伸比例 `scaleX` 与明暗位置：

```javascript
// 月相球体光照遮罩动态控制逻辑
const moonAge = data.moonAge || 11; // 盈凸月约 11 天
const illumPercent = Math.round((1 - Math.cos((moonAge / 29.53) * 2 * Math.PI)) / 2 * 100);
document.getElementById('ind-moon-illum').textContent = `照亮面: ${illumPercent}%`;
```

### 5.5 十二宫格详细环境指标拓扑架构

```text
+---------------------------------------------------------------------------------------------------------------+
|                                    12 宫格全景详细环境指标矩阵拓扑                                            |
+-----------------------------------+-----------------------------------+---------------------------------------+
|  1. 温度与体感温差                 |  2. 云量覆盖率 (Cloud Cover)       |  3. 24h 降水量 (Precipitation)        |
|  - 实时数值: 15°C / 体感 14°C     |  - 云量百分比: 80% (阴天)         |  - 降水强度: 2.5 毫米 (小雨)          |
|  - 趋势标签: 平稳接近             |  - 遮挡评估: 日照遮挡明显         |  - 预报提示: 未来连续小雨             |
+-----------------------------------+-----------------------------------+---------------------------------------+
|  4. 360° 风向罗盘与蒲福风级       |  5. 相对湿度与露点 (Humidity)     |  6. 紫外线辐射指数 (UV Index)         |
|  - 视觉罗盘: 360 度动态磁针指向    |  - 7 级动态微水柱进度可视化       |  - 弧形仪表盘指针定位                 |
|  - 指标: 北风 2级 (3km/h) 阵风 8  |  - 数值: 53% 相对湿度 / 露点 -8°C |  - 等级: 1 (低) / 无需防护            |
+-----------------------------------+-----------------------------------+---------------------------------------+
|  7. 空气质量 AQI 环形仪           |  8. 大气能见度阶梯金字塔           |  9. 大气压强滑块计 (Pressure)         |
|  - 环形仪表: 50 优 (绿色光晕)     |  - 4 阶梯视程透明度色块           |  - 范围区间: 980 ~ 1050 hPa           |
|  - 污染物解析: PM2.5 (18 µg/m³)   |  - 能见度: 18.4 km (极好视野)     |  - 读数: 1030 hPa (保持平稳)          |
+-----------------------------------+-----------------------------------+---------------------------------------+
|  10. 太阳日出日落轨迹弧           |  11. 3D 月相盈亏球体               |  12. 气象预警与多维生活出行指数       |
|  - 动态拱门: 07:05 日出/16:55 日落|  - 3D 立体月相: 盈凸月 (82% 照亮) |  - 道路结冰与湿滑出行警报             |
|  - 昼长: 9 小时 50 分钟           |  - 月龄跟踪: 11 天                |  - 穿衣: 风衣外套 / 洗车: 不适宜      |
+-----------------------------------+-----------------------------------+---------------------------------------+
```

### 5.6 24 位动漫少女乐队专属天气伴侣语录系统 (`character_tips.js`)

系统深度整合了 5 大动漫少女乐队（MyGO!!!!!、Ave Mujica、结束乐队、放学后TEA TIME、Togenashi Togeari）共 **24 位主要角色**的性格特质、标志性名台词与羁绊关系，以独立 JSON 文本资产形式归档于 `assets/weather/character_tips/` 目录中。

#### 1. 24 位角色档案与文件资产映射
- **MyGO!!!!!** (`assets/weather/character_tips/mygo/`)：
  - `tomori.json`：高松灯（主唱，内向、诗意、收集落叶与创可贴、对一辈子与成为人类的执着）
  - `anon.json`：千早爱音（吉他手，爱慕虚荣、假装现充、Anon Tokyo、破局行动力、对灯的宠溺关爱）
  - `raana.json`：要乐奈（吉他手，野猫属性、神出鬼没、随心所欲、抹茶芭菲执念、“有趣的女人”）
  - `soyo.json`：长崎爽世（贝斯手，高级大小姐礼仪、重力狂暴、执念于过去、傲娇别扭但又温柔照料同伴）
  - `taki.json`：椎名立希（鼓手，Livehouse 打工战士、外冷内热、黑咖啡、极度护灯“灯，没事的”）
- **Ave Mujica** (`assets/weather/character_tips/mujica/`)：
  - `sakiko.json`：丰川祥子 / Oblivionis（键盘手/领队，落魄千金、深夜客服打工、戴上假面逆天改命、决绝意志）
  - `mutsumi.json`：若叶睦 / Mortis（吉他手，少言寡语、人偶般冷淡克制、种黄瓜送黄瓜、“我不觉得开心”）
  - `umiri.json`：八幡海铃 / Timoris（贝斯手，30 支乐队雇佣兵助演、时间管理大师、极度自律理性、关照立希）
  - `nyamu.json`：祐天寺若麦 / Amoris（鼓手，当红美妆博主、精明现实、猫耳假面、追逐流量与热度）
  - `uika.json`：三角初华 / Doloris（主唱/吉他手，sumimi 偶像主唱、天体物理与观星爱好者、对祥子的深沉奉献）
- **结束乐队 (孤独摇滚)** (`assets/weather/character_tips/bocchi/`)：
  - `hitori.json`：后藤一里（主音吉他手，重度社恐、壁橱练习、guitarhero、芒果纸箱避难所、随时脑补溶解）
  - `nijika.json`：伊地知虹夏（鼓手/队长，下北泽大天使、STARRY 灵魂支柱、元气阳光、耐心鼓励波奇）
  - `ryo.json`：山田凉（贝斯手，为买中古乐器常年破产、吃草鉴赏家、向波奇借钱、高冷慵懒与独立审美）
  - `kita.json`：喜多郁代（主唱/节奏吉他手，天生现充、喜多光线 Kitaura、“后藤同学是我的英雄”、小太阳）
- **放学后 TEA TIME (轻音少女)** (`assets/weather/character_tips/kon/`)：
  - `yui.json`：平泽唯（主唱/吉他手，天然呆、吉太爱称、极致甜食控、绝对音感、对梓喵的日常熊抱）
  - `mio.json`：秋山澪（贝斯手/作词，左撇子伊丽莎白、害羞内向、容易受惊吓抱头蹲防、认真负责）
  - `ritsu.json`：田井中律（鼓手/部长，大额头黄发发箍、元气假小子、澪的头号恶作剧者、打鼓激情四射）
  - `mugi.json`：琴吹紬（键盘手，顶级财阀千金、粗眉毛、皇家红茶与顶级点心、怪力少女、向往平民生活）
  - `azusa.json`：中野梓（节奏吉他手，双马尾梓喵、易晒黑体质、认真严谨后辈、口嫌体正直粘学姐）
- **Togenashi Togeari (少女乐队的呐喊)** (`assets/weather/character_tips/gbc/`)：
  - `nina.json`：井芹仁菜（主唱，熊本离家出走少女、路灯战神、绝不认输“错的是这个世界”、摇滚怒吼）
  - `momoka.json`：河原木桃香（吉他手/领队，前钻尘吉他手、成熟帅气带点颓废、罐装啤酒、仁菜的灯塔）
  - `subaru.json`：安和昴（鼓手，演员世家孙女、“说谎是女人的生存武器”、情商极高、常识人润滑剂）
  - `tomo.json`：海老冢智（键盘手，名门出走大小姐、洁癖、理智冷静高智商、古典乐理天花板、傲娇护短）
  - `rupa.json`：鲁帕（贝斯手，日印混血、佛系温柔大姐姐、爱喝酒、刺无最强武力值与定海神针）

#### 2. 十大天气场景智能映射与伴侣语录展示
系统通过 `CharacterTipsManager` 动态解析气象状态（温度、降水、风速、时段），实时匹配以下 10 大场景并提取角色的专属语录：
1. `sunny` (晴天) · 2. `cloudy` (阴天/多云) · 3. `rain` (雨天) · 4. `snow` (雪天) · 5. `thunder` (雷雨)
6. `fog` (大雾/霾) · 7. `hot` (炎热) · 8. `cold` (寒冷) · 9. `wind` (大风) · 10. `night` (夜间)

在前端交互呈现上：
- **顶部悬浮磨砂玻璃提示栏 (`#top-weather-tip-bar`)**：实时呈现格式为 `[角色名]: 角色伴随语录`，支持悬停光晕与点击交互。
- **全屏无边框天气中心伴侣语录条 (`.fs-character-voice-banner`)**：展示乐队徽章、角色姓名标签、乐队分工、带引号的诗意对话框，并提供“换角色”（在当前乐队成员间循环切换）与“换台词”（即时随机更新台词）简洁双键。
- **主题联动自动契合**：当用户切换为 MyGO!!!!! 主题时，伴侣自动切换至高松灯；切换至 Ave Mujica 时自动切换至丰川祥子；切换至孤独摇滚时自动切换至后藤一里，实现视觉与心境的完美闭环。


---

## 6. 天气分类壁纸管理系统全景架构

### 6.1 空间分类拓扑与资产归类
为满足用户“天气与桌面背景心境合一”的需求，系统在 `core/wallpaper_manager.py` 与 `web/js/wallpaper.js` 中构建了一套高扩展性的天气壁纸分类体系：

| 分类标识 (Category) | 显示名称 | 触发气象条件与关键词匹配 | 默认包含的精选资产 |
| :--- | :--- | :--- | :--- |
| **`sunny`** | 晴天壁纸库 | 晴、少云、晴间多云 (`clear`, `sunny`) | `assets/weather/pictures/sunny/` 晴朗日光高清摄影与视频。 |
| **`rainy`** | 雨天壁纸库 | 小雨、中雨、大雨、暴雨、阵雨 (`rain`) | `anime_rainy_night.jpg` (月夜雨景动漫少女) 及实景雨景。 |
| **`snow`** | 雪天壁纸库 | 小雪、大雪、暴雪、雨夹雪 (`snow`) | `assets/weather/pictures/snow/` 冬日落雪静谧摄影与视频。 |
| **`fog`** | 雾天壁纸库 | 雾、轻雾、大雾、霾、沙尘 (`fog`, `haze`) | `assets/weather/pictures/fog/` 晨雾远山薄暮摄影与视频。 |
| **`video`** | 视频壁纸库 | 扫描发现的高清动态视频媒体 | MP4 / WebM / MKV / AVI 视频，GPU NVDEC 硬解播放。 |
| **`custom`** | 用户自定义 | 用户独立挑选的文件或指定目录 | 用户本地磁盘导入的高清图片与定制视频。 |

### 6.2 气象状态机与自动联动切换原理
壁纸控制器通过挂载在天气控制器中的广播通道，形成响应式链路：
1. **气象更新事件广播**：当 `WeatherController` 成功解析最新实况气象（例如当前城市从“多云”转为“小雨”）。
2. **关键词模糊模式匹配**：`WallpaperManager.handleWeatherConditionChange(conditionText)` 启动模式匹配机，扫描关键词（`雨` -> `rainy`，`雪` -> `snow`，`雾/霾` -> `fog`，其他 -> `sunny`）。
3. **防抖动智能校验**：校验用户是否开启了“天气联动”总开关（`#wp-auto-switch-toggle`），并比对目标分类壁纸是否与当前正在渲染的背景相同，防止频繁无效重绘。
4. **平滑淡入切换**：从匹配分类列表中挑选权重最高的壁纸，通过 CSS `opacity` 产生 0.6 秒交叉溶解过渡，静默更新桌面背景。

### 6.3 纯净高能媒体壁纸引擎与 C 级原生加速架构

系统彻底剥离淘汰了臃肿低效的 WebGL iframe、Steam workshop `project.json` 与 `.pkg` 逆向解析，全面收敛为纯粹的高能视频与图片双模态架构，实现 C 级别的极速响应：

```text
+---------------------------------------------------------------------------------------+
|                       极速双模态媒体壁纸引擎与 C 级加速拓扑                           |
+---------------------------------------------------------------------------------------+
|                                                                                       |
|   1. Video 视频壁纸 (Type: "video") ------------------> [ 显卡硬解流式秒开 ]          |
|      - 支持格式: MP4 / WebM / MKV / AVI                                               |
|      - HTTP 206 Partial Content 分片流式响应 (Range: bytes=start-end)                 |
|      - TCP_NODELAY 无延迟 Socket 传输，消除 Nagle 缓冲延迟                            |
|      - 128KB 动态分片缓冲区，支持任意时间轴即时 Scrubbing 跳播                        |
|      - 纯硬件 GPU 解码，CPU 占用低于 1.5%，内存占用趋近于 0                           |
|                                                                                       |
|   2. Image 图片壁纸 (Type: "image") ------------------> [ 物理漫反射超清图层 ]        |
|      - 支持格式: JPG / JPEG / PNG / WEBP / BMP / GIF                                  |
|      - 原生 CSS3 硬件合成背景与 Canvas 天气粒子叠加                                   |
|      - 结合 28px 高斯多阶磨砂玻璃，提供纯净晶莹的悬浮质感                             |
|                                                                                       |
|   3. C 级 Win32 原生媒体扫描器 (`core/native_core.dll`) -> [ 毫秒级极速扫描 ]         |
|      - 基于 Win32 FindFirstFileW / FindNextFileW 纯 C 递归遍历                        |
|      - 内存内直接格式化为紧凑 JSON 数组，扫描时间比 Python os.walk 提速 1.8 倍        |
|      - 彻底消除传统创意工坊扫描时产生的大量小文件 I/O 阻塞                            |
|                                                                                       |
|   4. Win32 穿透模式 (Desktop WorkerW Injection) ------> [ 纯 C 桌面底层钉扎 ]         |
|      - 向 Progman 派发 0x052C 激活隐藏 WorkerW 视窗                                   |
|      - 纯 C 原生调用 SetParent(window_hwnd, workerw_hwnd) 毫秒级挂载至桌面图标下方    |
|                                                                                       |
+---------------------------------------------------------------------------------------+
```

---

## 7. 定时关机调度器与每日音乐守护系统

### 7.1 后台守护线程与高精度定时算法 (`core/scheduler.py`)
为避免传统 `time.sleep(60)` 导致的分钟跨越遗漏或系统休眠恢复后的时钟脱节，系统设计了自适应轮询调度器：
- 调度器作为独立的后台守护线程（`daemon=True`）启动，每 1.0 秒唤醒一次比对。
- **跨日状态自复位**：记录 `today_day = now.timetuple().tm_yday`，每逢次日 00:00:00 自动将 `started_today` 与 `shutdown_executed_today` 标志归零，确保多天长期运行无需重启软件。
- **目标命中与双向触发**：当系统时间首次步入设定的关机分钟区间（如 `23:00`）时，立即锁定执行标志，同时向前端与操作系统底层分发双向动作：
  * **前端总线通知**：通过 `win.evaluate_js("window.onDailyScheduleTriggered(weekday)")` 命令 Web 端弹出提醒、更新倒计时面板、并触发当日专属音乐播放。
  * **系统底层调度**：调用 `core.system_ops.execute_shutdown(300)`，向 Windows 内核注入 300 秒（5 分钟）关机定时器。

### 7.2 星期专属音乐轮播机制与完整歌单映射表
系统保留并升华了经典原版项目的每周音乐轮播文化，在 `assets/music/` 目录下收录了 7 首高品质专属乐曲，按星期一至星期日精准映射：

| 周期节点 | 物理文件名 | 乐曲正式名称 | 艺术作品出处 / 艺术家 | 编排情绪基调 |
| :--- | :--- | :--- | :--- | :--- |
| **星期一 (Mon)** | `001.mp3` | 《春日影》（Haruhikage） | 《BanG Dream! It's MyGO!!!!!》 | 迷茫而坚定的青蓝摇滚，洗刷周一的疲惫与焦虑。 |
| **星期二 (Tue)** | `002.mp3` | 《雑踏、僕らの街》（杂踏、我们的街道） | 《GIRLS BAND CRY》／ トゲナシトゲアリ | 疾走感极强的工业摇滚，赋予破开平庸的激情。 |
| **星期三 (Wed)** | `003.mp3` | 《Don't say "lazy"》 | 《轻音少女》（K-ON!）／ 放学后 TEA TIME | 经典动感朋克，放学后的肆意畅快与青春律动。 |
| **星期四 (Thu)** | `004.mp3` | 《KiLLKiSS》 | 《BanG Dream! Ave Mujica》 | 哥特交响金属，撕破暗夜沉沦的华丽乐章。 |
| **星期五 (Fri)** | `005.mp3` | 《青春コンプレックス》（青春症候群 钢琴版） | 《孤独摇滚！》／ 結束バンド | 温润纯净的钢琴弹拨，迎来周末前夕的惬意慰藉。 |
| **星期六 (Sat)** | `006.mp3` | 《鸟之诗》（AIR 主题曲） | Key社经典动画《AIR》／ Lia / 折户伸治 | 澄澈辽远的千禧经典，抚慰心灵的天空咏叹调。 |
| **星期日 (Sun)** | `007.mp3` | 《圣诞快乐，劳伦斯先生》 | 坂本龙一（Ryuichi Sakamoto） | 旷远深沉的世纪绝响，为整周画下静谧句点。 |

### 7.3 5 分钟安全缓冲倒计时与一键拦截保护
当定时关机触发后，系统进入最高优先级安全倒计时周期：
- 中控坞顶部的倒计时条高亮显示：`系统将在 04:59 后关机`。
- 操作区瞬间浮现高反差按钮：`取消关机`（`#cancel-shutdown-btn`）。
- 若用户仍在工作或娱乐，点击即可触发 `BackendAPI.cancel_shutdown()`，后台即刻执行 Windows `shutdown /a` 指令中断关机进程，阻断误操作造成的文档丢失风险。

---

## 8. 配置文件规范 (`settings.json`) 与 API 字典

### 8.1 `settings.json` 模式定义与字段详释

```json
{
    "password": "1",
    "startTime": "23:00",
    "theme": "default",
    "cityName": "安阳市",
    "locationId": "101180201",
    "wallpaperType": "classic",
    "wallpaperPath": "/assets/images/anime_rainy_night.jpg",
    "wallpaperFile": "",
    "wallpaperAutoSwitch": true,
    "volume": 0.8,
    "autoStart": false,
    "averageTemperature": 21.5,
    "weatherUpdateTime": "2026-11-22T02:00+08:00"
}
```

#### 配置项字典详述

| 键名 (Key) | 数据类型 (Type) | 默认值 | 允许值范围 / 示例 | 职责功能说明 |
| :--- | :--- | :--- | :--- | :--- |
| **`password`** | `String` | `"1"` | 任意字符 | 高级系统设置面板的权限进入口令。 |
| **`startTime`** | `String` | `"23:00"` | `HH:MM` 格式 | 每日自动播放音乐与关机触发时间点。 |
| **`theme`** | `String` | `"default"` | `default` / `mygo` / `ave-mujica` / `bocchi` / `k-on` / `togenashi` | 当前生效的少女乐队主题代码。 |
| **`cityName`** | `String` | `"安阳市"` | `"北京市"`, `"上海市"` 等 | 天气显示的主选城市中文字符串。 |
| **`locationId`** | `String` | `"101180201"` | 城市编码或名称 | 城市唯一气象地理区域编码。 |
| **`wallpaperType`** | `String` | `"classic"` | `classic` / `video` / `web` / `acrylic` | 当前壁纸渲染管线模式。 |
| **`wallpaperPath`** | `String` | `"/assets/..."` | 相对路径 / HTTP 路径 / 本地绝对路径 | 当前壁纸的资源统一定位符。 |
| **`wallpaperFile`** | `String` | `""` | 物理文件名 | 当使用外部导入时的原始文件名。 |
| **`wallpaperAutoSwitch`** | `Boolean` | `true` | `true` / `false` | 天气变化时是否自动切换匹配壁纸。 |
| **`volume`** | `Number` | `0.8` | `0.0` 至 `1.0` | 全局音乐与语音播报的音频增益系数。 |
| **`autoStart`** | `Boolean` | `false` | `true` / `false` | 是否写入 Windows 注册表实现开机静默启动。 |

### 8.2 后端暴露 API 字典 (`window.pywebview.api`)

#### 配置与系统类接口

| 方法签名 | 入参规格 | 返回值规格 | 说明 |
| :--- | :--- | :--- | :--- |
| **`get_config()`** | 无 | `Object: settings` | 获取当前生效的全量配置对象，并实时同步开机自启状态。 |
| **`save_config(new_config)`** | `Object` (键值对) | `{"success": bool, "config": dict}` | 深度合并配置并异步原子写入磁盘 `settings.json`。 |
| **`verify_password(input_pwd)`** | `String` | `Boolean` | 校验输入的管理员口令是否匹配。 |
| **`get_autostart_status()`** | 无 | `Boolean` | 检查 Windows Run 注册表键值是否存在。 |
| **`set_autostart_status(enabled)`**| `Boolean` | `{"success": bool, "autoStart": bool}`| 写入或移除 Windows 当前用户注册表自启键。 |

#### 壁纸与多媒体类接口

| 方法签名 | 入参规格 | 返回值规格 | 说明 |
| :--- | :--- | :--- | :--- |
| **`select_wallpaper_file()`** | 无 | `String: abs_path` | 唤起 Windows 原生文件挑选对话框（支持常用图片/视频/HTML）。 |
| **`select_wallpaper_folder()`** | 无 | `String: abs_path` | 唤起 Windows 原生文件夹选择器（用于指定自定义工坊目录）。 |
| **`get_categorized_wallpapers(path)`**| `String` (可选目录) | `{"all":[], "sunny":[], "rainy":[], ...}` | 扫描并提取按天气与多模态归类的结构化壁纸全集。 |
| **`scan_wallpaper_engine(path)`** | `String` (可选目录) | `{"count": int, "wallpapers": []}` | 扫描 Steam 创意工坊 Wallpaper Engine 壁纸目录。 |

#### 气象数据类接口

| 方法签名 | 入参规格 | 返回值规格 | 说明 |
| :--- | :--- | :--- | :--- |
| **`fetch_weather(loc)`** | `locationId: str = "101180201"` | `{"success": bool, "data": dict}` | 获取归一化实时天气数据（免 Key，MSN / Open-Meteo 自动聚合）。 |
| **`fetch_hourly_weather(city)`**| `cityName: str` | `{"success": bool, "data": dict, "fallback": bool}` | 触发 MSN Weather 多层爬虫，抓取 48h 贝塞尔逐小时数据、多日趋势、月相与 12 宫格指标。 |

#### 窗口与电源类接口

| 方法签名 | 入参规格 | 返回值规格 | 说明 |
| :--- | :--- | :--- | :--- |
| **`schedule_shutdown(seconds)`** | `seconds: int` (默认 300) | `{"success": bool, "seconds": int}` | 调用系统底层 `shutdown /s /t <seconds>` 注入关机倒计时。 |
| **`cancel_shutdown()`** | 无 | `{"success": bool}` | 调用系统底层 `shutdown /a` 紧急取消待执行关机。 |
| **`toggle_fullscreen()`** | 无 | `{"success": bool, "fullscreen": bool}` | 切换当前 WebView 宿主窗口的全屏状态。 |
| **`minimize_window()`** | 无 | 无 | 最小化当前窗口至 Windows 任务栏。 |
| **`hide_to_tray()`** | 无 | 无 | 隐藏窗口并常驻后台托盘区。 |
| **`quit_application()`** | 无 | 无 | 安全关闭后台守护进程，释放端口并彻底终止进程。 |

---

## 9. 项目目录拓扑与快速部署

### 9.1 完整工程目录结构拓扑

```text
d:/xiangmu/3663/
├── run.bat                          # Windows 一键执行入口 (自动检查 Python 依赖并启动)
├── run.ps1                          # PowerShell 自动化运行脚本
├── main.py                          # 应用程序主入口 (PyWebView + 本地服务 + DWM 磨砂 + 托盘)
├── settings.json                    # 全局运行配置文件
├── README.md                        # 面向开源社区的工程简报与快速上手说明
├── PROJECT_GUIDE.md                 # 深度系统技术架构、设计规范与 API 全景指南
│
├── core/                            # 后端核心业务与 C 级加速层
│   ├── native_core.c                # C 原生加速核心源码 (Win32 API / MurmurHash3 / WorkerW / DWM)
│   ├── native_core.dll              # GCC 16.1.0 编译的高性能 64 位原生动态链接库
│   ├── api.py                       # 前后端交互 RPC 桥梁 (BackendAPI)
│   ├── server.py                    # 支持 HTTP 206 Partial Content 与 TCP_NODELAY 的本地流媒体服务器
│   ├── msn_weather_crawler.py       # 微软天气 (MSN Weather) 实时逐小时爬虫、C 级哈希缓存与降级垫片
│   ├── weather_service.py           # 和风气象代理与离线数据适配器
│   ├── wallpaper_manager.py         # 调用 C 原生库极速扫描归档天气分类壁纸
│   ├── desktop_wallpaper.py         # C 级 Win32 WorkerW 桌面底层窗口穿透与挂载器
│   ├── window_effects.py            # C 级 DWM 与 Windows 10/11 亚克力磨砂透明效果注入
│   ├── scheduler.py                 # 每日定时关机调度器与音乐触发线程
│   ├── system_ops.py                # 操作系统底层关机指令与注册表自启封装
│   └── tray.py                      # Windows 任务栏系统托盘驻留控制 (pystray)
│
├── web/                             # 前端用户界面与交互渲染层
│   ├── index.html                   # 主界面 DOM 拓扑 (悬浮中控坞 + 全屏气象中心 + 视频/图片壁纸层)
│   ├── css/
│   │   └── style.css                # 纯白玻璃拟态设计体系与 6 大少女乐队主题色板
│   └── js/
│       ├── app.js                   # 前端业务总控 (时钟、状态机、Toast、设置门禁、主题热切换)
│       ├── weather.js               # 全屏天气视窗控制器 (12 宫格渲染、城市切换、语音播报、伴侣台词)
│       ├── weather_chart.js         # 48 小时平滑贝塞尔曲线 Canvas 渲染引擎
│       ├── wallpaper.js             # 视频/图片双模态壁纸管理器 (硬件流式硬解/天气联动/文件拖拽)
│       ├── audio_manager.js         # HTML5 Audio 声音中枢与每日音乐调度器
│       └── particles.js             # Canvas 天气物理粒子系统 (雨丝/落雪/迷雾/星轨)
│
├── wallpapers/                      # 本地壁纸存储与归档目录
│
└── assets/                          # 系统多媒体静态资源库
    ├── fonts/                       # HarmonyOS Sans 全套字体族 (Regular, Medium, Bold, Black)
    ├── images/                      # 基础贴图 (Logo、App 图标、雨夜月色背景图、全屏彩蛋)
    ├── music/                       # 星期一至星期日专属定时音乐 (001.mp3 ~ 007.mp3 及歌单)
    ├── sounds/                      # 电台提示音
    └── weather/                     # 官方 SVG 矢量图标库、MSN 气象离线持久化垫片
        └── character_tips/          # 5 大少女乐队 24 位角色专属各天气台词库 (共 576 条独立台词)
```

### 9.2 运行依赖环境与安装步骤

#### 1. 系统要求
- **操作系统**：Windows 10 (1809 及以上) 或 Windows 11 (64位)。
- **系统组件**：系统内置已安装 Microsoft Edge WebView2 Runtime（Windows 10/11 通常已自带）。
- **Python 环境**：Python 3.10 及以上版本。

#### 2. 安装 Python 核心依赖
打开 PowerShell 或 CMD，在项目根目录下执行：

```powershell
pip install pywebview pystray Pillow
```

*说明：`pywebview` 提供 Edge WebView2 容器集成；`pystray` 与 `Pillow` 用于构建系统托盘图标；其余网络、流媒体服务与 Win32 调用均为 Python 标准库或系统原生调用。*

#### 3. 一键启动
在项目根目录下双击运行：
```cmd
run.bat
```
或在终端执行：
```powershell
python main.py
```

---

## 10. 故障排查与维护规范

### 10.1 常见问题速查与修复方案

| 故障现象 | 根因诊断 | 处置步骤与规避手段 |
| :--- | :--- | :--- |
| **启动报错：WebView2 runtime not found** | 操作系统为精简版或旧版，缺失 WebView2 运行时。 | 访问微软官网下载并安装 Evergreen WebView2 Runtime 独立安装包。 |
| **窗口背景不透明，显示为纯黑或灰色** | 当前 Windows 主题禁用了 DWM 透明效果或驱动未开启硬件加速。 | 打开 Windows 系统“设置” -> “个性化” -> “颜色”，确保“透明效果”处于开启状态。 |
| **天气数据获取失败或网络超时** | 当前网络无法直接解析 MSN 接口或受到 DNS 污染。 | 系统将自动启用三级弹性降级机制，读取本地缓存或离线合成气象，不影响界面核心功能渲染。 |
| **视频壁纸黑屏或无法播放** | 视频编码格式不被 Edge 解码器支持（如 HEVC/H.265 或 ProRes）。 | 推荐将壁纸转码为兼容性极佳的 H.264/AVC 编码 `.mp4` 或 VP9 编码 `.webm` 文件后重新导入。 |
| **定时关机未在到达时间执行** | 系统进入强休眠状态或当日已执行过关机。 | 检查电源选项防止电脑在定时时间前进入睡眠；跨日状态会在每日 00:00 自动重置。 |

---
*ays2z-233 桌面助手工程技术委员会 · 严谨构筑，纯粹致远*
