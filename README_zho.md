<p align="center">
  <img src="images/EH3D_BANNER.svg" alt="EH3D-NODE-HMI-GUI banner" width="100%">
</p>

# 🖥️ EH3D-NODE-HMI-GUI

<p align="center"><a href="README.md">🇺🇸 English</a> | <a href="README_spa.md">🇪🇸 Español</a> | <a href="README_fra.md">🇫🇷 Français</a> | <a href="README_ita.md">🇮🇹 Italiano</a> | <a href="README_deu.md">🇩🇪 Deutsch</a> | 🇨🇳 <b>中文</b> | <a href="README_jpn.md">🇯🇵 日本語</a> | <a href="README_por.md">🇵🇹 Português</a></p>

### 一块带 ESP32-P4 的 10.1 英寸触摸屏，可独立控制数控机床、激光机或贴片机：与桌面程序相同的界面和流程，支持八种语言，语音提示和语音指令，硬件急停不经过屏幕。

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 or later">
  <img src="https://img.shields.io/badge/Stack-C%2B%2B17%20%2B%20ESP--IDF-00599C.svg" alt="C++17 and ESP-IDF">
  <img src="https://img.shields.io/badge/Languages-8-00E5FF.svg" alt="Languages">
  <img src="https://img.shields.io/badge/Version-0.0.1-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-scaffolding-FFB020.svg" alt="Maturity">
</p>

---

**诚实检查 - 今天实际能运行的内容：** 屏幕的规则用 C++17 编写，并已在计算机上测试：带安全默认值的设置文件、每次移动和每条主轴指令都必须通过的“闸门”（屏幕确认、硬件急停锁存、已就绪的连接）、八种语言的语音指令和提示、各界面的文本、竖屏和横屏布局、microSD 卡的文件，以及 EH3D-SERVER 的客户端（请求、机群的读取，以及一个作业从卡上的文件到结束的流程）。带 LVGL 界面的 ESP-IDF 固件是为 Guition JC8012P4A1C 模块编写的，但**尚未编译、烧录或在模块上运行**；它从未与真实服务器通信，机床本身的动作（点动、回原点）要等待与节点的连接，而该连接尚未编写。

---

> **安全状态（safety 与信息安全）：** 这不是安全系统。保护人员和机床的急停是一个硬件按钮，它自行切断驱动，接线独立于本屏幕；屏幕只读取并显示它，在它被按下或未确认期间拒绝移动任何东西。移动或主轴指令需要在屏幕上确认，语音指令始终需要确认，并且语音指令在操作员打开之前处于关闭状态。在整条链路于真实机床上试验之前，请勿用本屏幕运行机床。本项目是独立的：并非由 Bungard Elektronik 或 Guition 制造、认可或支持，产品名称仅用于说明其适用的机床和模块。

## 📋 速览

| | |
|---|---|
| 版本 | `0.0.1` |
| 成熟度 | 脚手架 |
| 测试 | 5 个主机测试程序（共 1018 项检查） |
| 技术栈 | C++17, ESP-IDF 5.5, LVGL 9, ESP32-P4 |
| 真实硬件 | 否：固件尚未编译或烧录，也未连接任何机床 |

**跳转到：** [概述](#-概述) · [架构](#-架构) · [快速开始](#-快速开始) · [测试](#-测试) · [结构](#-仓库结构) · [相关项目](#-相关项目)

## 🚦 项目状态

每一行是本仓库的一个部分，灯号表示它今天做到了哪一步。内容依据仓库能够展示的证据（测试、构建、运行），而不是计划。

🟢 已完成并验证 · 🟡 可用，有已知限制 · 🟠 部分完成或尚未验证 · 🔴 未完成或不可用

| 领域 | 状态 | 在这里的含义 |
|---|---|---|
| 软件 | 🟠 | ESP-IDF 工程（ESP32-P4、LVGL 9、独立分区中的设置文件）已编写；尚未编译或烧录。 |
| 测试 | 🟢 | 5 个主机测试程序通过（`ctest`）：共 1018 项检查，涵盖设置文件、闸门、语音指令和提示、键盘、录制的提示音、服务器客户端，以及八种语言的界面文本。 |
| 安全规则 | 🟡 | 每次移动和每条主轴指令都经过闸门：屏幕确认、硬件急停锁存（按钮松开后只能由屏幕上的确认解除）、连接未就绪时不得移动。有测试覆盖；从未配合真实急停按钮试过。 |
| 语音 | 🟡 | 语音指令和提示以八种语言识别和表述，并作为文本测试；未使用任何麦克风或扬声器。 |
| 界面 | 🟠 | 机床、作业（microSD）、机床和工位以及设置界面已编写，支持横屏和竖屏，外观与桌面程序一致；查看标签页仍是空卡片。目前只有布局示意图，没有模块上的截图。 |
| 真实硬件 | 🔴 | 尚无模块运行过此固件，与服务器的连接尚不存在，也未连接任何机床。 |
| 文档 | 🟢 | 八种语言的 README、设置参考和硬件说明。 |
| 服务器和存储卡 | 🟡 | microSD 卡的文件（列表、安全文件名、类型）、对 EH3D-SERVER 的请求、机群的读取以及作业的流程（以就绪状态发送、通过闸门启动、跟踪、应答换刀）已在计算机上用手写的服务器应答测试；套接字和存储卡驱动从未在模块上运行。 |

## 🎯 概述

* **闸门**：每次移动和每条主轴指令，无论来自触摸还是语音，都经过同一个地方，由它决定执行、要求确认或附带原因地拒绝；停止指令始终放行。
* **设置文件** `hmi.cfg`：每个调好的值（语言、亮度、服务器、安全开关、语音）都是独立分区中一个文件的一行，绝不是代码里的数字；超出范围的值会被报告，并保留安全的默认值。
* **语音**：语音提示（作业完成、换刀、错误、连接丢失、硬件急停）和八种语言的一份简短固定语音指令表；仅用于停止的指令在句子中也能识别，用于移动或转动的指令必须完整说出，并且始终需要确认。
* **界面**：机床（点动、回原点、主轴、开始、停止）、作业、查看、服务器的机床和工位以及设置，顺序和名称与桌面程序一致；停止按钮始终在最上方。
* **模块**：Guition JC8012P4A1C（ESP32-P4，32 MB 外部 RAM，10.1 英寸 800x1280 MIPI-DSI 触摸屏，用于 Wi-Fi 的 ESP32-C6，带扬声器功放的 ES8311 编解码器和 ES7210 麦克风输入）。本机的面板是新款。
* **竖放或横放：** 设置 `orientation`（`auto`、`landscape`、`portrait`）旋转面板的画面：横放（1280 x 800）时作业、平面视图和点动面板分三列排在位置栏上方，竖放（800 x 1280）时叠放；两种情况下触摸都落在被触摸的位置。
* **microSD 与服务器成为一条流程：** 卡上列出钻孔和铣切文件；屏幕把选中的文件作为就绪等待的作业发送给 EH3D-SERVER，操作员通过闸门启动，进度、换刀停顿和结束实时从机群返回，与桌面程序和节点一致。服务器对每个作业的应答会保存在卡上。

## 🧩 在 EH3D 中的角色

EH3D-NODE-HMI-GUI 是数控机床、激光机或贴片机操作员自己的屏幕（与 EH3D-NODE-BUNGARD-HMI 相同的硬件）：它取代机床旁的电脑，并且和 EH3D-DRILLPRO、EH3D-ROUTEPRO、EH3D-BUNGARD-GUI 一样，是 EH3D-SERVER 的又一个客户端。屏幕从 microSD 卡发送的作业由服务器规划，并且同时出现在桌面程序中；从桌面程序启动的作业会在这里显示进度；服务器通过机床的节点在机床上运行它。屏幕自身的直接动作（点动、回原点）将使用同一个节点，该连接尚未编写。

## 🏛️ 架构

<p align="center">
  <img src="images/architecture.svg" alt="EH3D-NODE-HMI-GUI 的架构：谁在使用、包含什么、依赖什么" width="100%">
</p>

本仓库各部分如何配合：谁与它通信、它包含什么、依赖什么。图由 `docs/architecture.json` 生成，并与代码核对。

## 🖼️ 屏幕截图

<p align="center">
  <img src="images/screenshots/screen-machine.png" alt="machine screen lying down" width="100%">
</p>

<p align="center"><em>横屏的机床界面（英语）：作业、平面视图和点动面板分三列，位置和停止按钮在底栏。这是根据固件文本绘制的布局示意图，不是模块上的截图。</em></p>

<p align="center">
  <img src="images/screenshots/screen-portrait.png" alt="machine screen standing up" width="40%">
</p>

<p align="center"><em>同一界面竖放（德语）：卡片叠放，位置栏留在底部。</em></p>

<p align="center">
  <img src="images/screenshots/screen-card.png" alt="files of the microSD card" width="100%">
</p>

<p align="center"><em>作业页（此处为意大利语）：microSD 卡上的钻孔和铣切文件、选中的文件，以及把它作为就绪等待的作业发送到服务器的按钮。</em></p>

<p align="center">
  <img src="images/screenshots/screen-fleet.png" alt="machines and stations" width="100%">
</p>

<p align="center"><em>机床和工位（此处为葡萄牙语）：服务器的所有机床按工位排列，显示状态和作业进度，与桌面程序所见一致。</em></p>

<p align="center">
  <img src="images/screenshots/screen-confirm.png" alt="confirmation of a move" width="100%">
</p>

<p align="center"><em>移动在执行任何动作之前都要求确认（此处为西班牙语）。</em></p>

<p align="center">
  <img src="images/screenshots/screen-stop.png" alt="hardware stop pressed" width="100%">
</p>

<p align="center"><em>硬件急停已按下：屏幕会提示，将会移动的按钮变暗，并在松开被确认之前拒绝移动任何东西（此处为日语）。</em></p>

## ✨ 功能

* **忘不掉的停止：** 停止按钮始终在最上方并且始终有效；硬件急停会被锁存，只有在按钮松开后在屏幕上确认才能解除。
* **移动前确认：** 移动或主轴指令会显示将要做什么，并在限时内等待一次触摸；新的请求或连接丢失会将其丢弃。
* **八种语言，语音也一样：** 英语、西班牙语、意大利语、德语、法语、欧洲葡萄牙语、中文和日语，覆盖界面、语音提示和语音指令。
* **默认即安全：** 每个安全开关都在设置中有对应项，并从保守位置开始：语音指令关闭、语音确认关闭、确认开启、要求硬件急停。
* **每项设置用合适的键盘：** 数字在了解该设置范围和小数位的数字键盘上输入（箭头每次移动一个单位，超出范围的值不被接受）；文本，如服务器地址或令牌，在带有当前语言按键（变音符、重音、波浪号、软音符）的完整键盘上输入，带单字母大写和符号层；开关和列表不需要键盘。
* **带符号的按钮：** 点动键盘、回零、主轴、暂停、开始、停止以及对问题的回答，都用符号和轴字母代替文字，颜色与桌面程序一致（绿色开始、蓝色暂停、红色停止）。
* **录制的提示：** 作业结束和请求刀具可以由录制的语音说出，语言和性别按设置（`sound_finish = voice:es:f`），来自存储卡的 `voices` 文件夹；录音与 EH3D-BUNGARD-GUI 相同。固件中尚未接上扬声器。
* **重启后继续跟踪作业：** 从操作员启动作业起，屏幕会把作业的标识（作业编号、机床、名称）保存在闪存中；断电或重启后继续跟踪同一个作业，服务器的下一次读数会说明它是否仍在运行、是否在换刀处等待或已结束，取消按钮可放弃跟踪。作业本身保存在服务器中。已在电脑上编写并测试；固件从未在模块上运行过。
* **机床类型：** 设置文件中的 `machine_kind`（`cnc`、`laser` 或 `pnp`）决定屏幕可以下达什么指令。只有数控机床才有主轴按钮，无论其他设置如何，闸门都会拒绝对激光机或贴片机的主轴指令：屏幕绝不会触发激光，也绝不会自行启动贴片头。

## 🔒 安全与防护

| 威胁 | 防御 |
|---|---|
| 触摸或语音指令意外移动机床 | 移动需要在屏幕上限时确认；语音指令始终需要确认；只有设置 `voice_confirm` 打开（默认关闭）时，说出的“是”才算确认 |
| 硬件急停被按下时屏幕开始移动 | 硬件急停是锁存的：按下，或松开但未在屏幕上确认时，所有移动和主轴指令都被拒绝，等待中的请求被丢弃；等待确认时按下急停会取消它 |
| 机床尚未就绪 | 连接未就绪、硬件急停输入未接线（`hw_stop_required`，默认开启）或主轴上限为 0 时，什么都不会移动 |
| 把说出的某个词当成指令 | 指令来自每种语言的一份简短固定列表，绝不是自由文本；只是包含指令的词（`stopwatch`）不是指令；语音指令在操作员打开前处于关闭状态 |
| 设置中的输入错误 | 超出范围的值或未知的键会被报告，并保留安全默认值；不会因失误而打开任何开关 |
| 固件或仓库中的机密 | 没有任何机密被编译进去；令牌是其分区中的设置，从不显示也不记录 |

**未设防的部分：** 屏幕不是安全装置，无法切断机床的驱动；这必须由硬件急停自行完成。目前还没有与服务器的连接，所以屏幕今天无法移动任何机床。语音识别在测试中以文本方式运行；真实的麦克风通路（回声消除、噪声、口音）尚未试过。拿到模块的人可以修改设置文件。

## 🔌 接口

| 接口 | 内容 | 位置 |
|---|---|---|
| 设置文件 | `config` 分区中的 `hmi.cfg`：`键 = 值` 行；见 `hmi.cfg.example` 和上表 | `core/hmi_config.hpp` |
| 硬件急停输入 | 一个 GPIO（menuconfig 中的 `HMI_HW_STOP_GPIO`，默认低电平有效，带上拉，因此断线视为按下） | `main/board_p4.cpp` |
| 触摸屏 | 通过 LVGL 9 驱动 800x1280 MIPI-DSI | `main/ui.cpp` |
| 声音 | 带扬声器功放的 ES8311 编解码器和 ES7210 麦克风输入 | `main/board_p4.cpp` |
| microSD | 卡的根目录 `/sdcard`：列出并读取钻孔、铣切、Gerber、DXF 和 ZIP 文件，写入报告；只接受简单文件名 | `core/sdfiles.hpp`, `main/board_p4.cpp` |
| EH3D-SERVER | 带访问令牌的 HTTP：每 `poll_s` 秒 `GET /api/v1/fleet`，`POST /api/v1/machines/:id/jobs`，`POST /api/v1/jobs/:id/start`，以及 `confirm` 和 `cancel` | `core/server_link.hpp`, `main/server_link.cpp` |

## ⚙️ 配置

所有调整项都是 `hmi.cfg` 中的一行（把 `hmi.cfg.example` 复制到 `config` 分区）；超出范围的值会在日志中报告并保留默认值。下面是默认值；所有安全默认值都是保守的。

| 设置 | 默认 | 范围 | 作用 |
|---|---|---|---|
| `language` | `en` | `en es it de fr pt zh ja` | 屏幕和语音的语言 |
| `confirm_moves` | on | on / off | 移动需要确认：仅在试验台上才关闭 |
| `confirm_timeout_ms` | 8000 | 1000 - 60000 | 确认等待多久 |
| `jog_max_mm` | 20 | 1 - 200 | 屏幕发送的最长单次点动 |
| `spindle_max_rpm` | 24000 | 0 - 65535 | 请求的最高主轴转速（0：永不启动主轴） |
| `hw_stop_required` | on | on / off | 仅当硬件急停输入已接线且松开时才控制机床 |
| `voice_orders` | off | on / off | 接受语音指令 |
| `voice_confirm` | off | on / off | 把说出的“是”当作移动的确认 |
| `token` | none | 8 - 64 characters | 服务器的访问令牌；只写，从不显示也不记录 |
| `orientation` | `auto` | `auto landscape portrait` | 竖放或横放；`auto` 跟随模块的安装方向 |
| `poll_s` | 3 | 1 - 60 | 屏幕多久向服务器请求一次机群 |

## 🚀 快速开始

主机测试需要 C++17 编译器和 CMake，固件需要 ESP-IDF 5.5.5（模块的 ESP32-P4 目标）。

```bash
build.bat            # the host tests (Windows)  -  ./build.sh on Linux
python -I tools/ci_validate.py
```

固件尚未编译：编译后，`idf.py set-target esp32p4 && idf.py build` 生成镜像，名为 `config` 的分区保存 `hmi.cfg`。

## 🧪 测试

主机测试（`build.bat` 或 `./build.sh`，然后 `ctest`）是两个程序，全部通过：`test_hmi`（638 项检查）涵盖设置文件、闸门（停止始终放行、硬件急停锁存、确认、超时与时钟回绕、语音规则、开关）、八种语言的语音指令和提示以及界面文本；`test_workflow`（115 项检查）涵盖方向与触摸、卡上的文件（类型、安全文件名、排序）、JSON 读取器、服务器的请求与应答，以及作业从卡到结束的流程。 另外两个程序覆盖键盘（`test_keyboard`，137 项检查：每项设置都有自己的键盘、数字键盘的范围与设置文件一致、小数位、符号、箭头、每种语言的完整键盘、大写、符号层和长度）以及录制的提示（`test_notices`，66 项检查：设置、每种语言和性别的文件）。

## 🏗️ 构建与发布

版本保存在 `eh3d.project.json` 中，每个版本在 [CHANGELOG.md](CHANGELOG.md) 中都有条目。`build.sh` / `build.bat` 编译并运行主机测试，`test.sh` / `test.bat` 运行仓库检查。固件在本仓库脚本之外、在可见窗口中编译，一次一个构建。

## 🩺 故障排除

| 现象 | 处理方法 |
|---|---|
| 什么都不动，屏幕提示硬件急停已按下或未确认 | 松开按钮，然后按屏幕上的确认按钮：已松开的急停不会自行解除。 |
| 屏幕提示机床未就绪 | 此版本还没有与机床的连接；在连接使其就绪之前，闸门拒绝一切移动。 |
| 屏幕提示硬件急停输入未接线 | 把急停按钮接到 menuconfig 中设置的引脚（`HMI_HW_STOP_GPIO`），或仅在试验台上关闭 `hw_stop_required`。 |
| 语音指令没有反应 | 语音指令默认关闭：设置 `voice_orders = on`。除非 `voice_confirm` 打开，移动或转动类指令需要在屏幕上触摸确认。 |

## 📂 仓库结构

<p align="center">
  <img src="images/repository-structure.svg" alt="EH3D-NODE-HMI-GUI 的仓库结构" width="100%">
</p>

<details>
<summary>目录树的文本版</summary>

```text
EH3D-NODE-HMI-GUI/
├── core/
│   ├── hmi_config.hpp lang.hpp                       设置文件及其安全默认值
│   ├── gate.hpp                                      闸门：确认、急停锁存
│   ├── machine.hpp                                   机床类型
│   ├── voice.hpp                                     八种语言的语音指令和提示
│   ├── orientation.hpp sdfiles.hpp                   竖屏和横屏布局、卡上的文件
│   ├── server_link.hpp workflow.hpp job_memory.hpp   服务器客户端和作业流程
│   └── ui_text.hpp keyboard.hpp notices.hpp          文本、键盘与提示
├── main/
│   ├── app_main.cpp                                  启动、设置、闸门循环
│   ├── ui.cpp ui.hpp                                 LVGL 界面和停止按钮
│   ├── board_p4.cpp board.hpp                        模块：显示、声音、急停输入
│   ├── server_link.cpp server_link.hpp               服务器客户端的套接字
│   └── idf_component.yml Kconfig.projbuild           组件列表和 menuconfig 引脚
├── tests/                                            设置、闸门、语音和界面文本的主机测试
├── docs/                                             硬件说明和两幅图的源文件
├── tools/                                            CI 检查、README 检查和绘图工具
├── CMakeLists.txt                                    ESP-IDF 工程
├── partitions.csv                                    带两个槽位的闪存布局
├── hmi.cfg.example                                   每个设置及其默认值和范围
├── sdkconfig.defaults                                ESP32-P4 模块的 ESP-IDF 设置
├── sdkconfig.board.p4                                开发板设置
├── dependencies.lock                                 组件的确切版本
├── images/                                           横幅图以及架构和仓库结构图片
├── .github/                                          CI 与发布工作流、Issue 和 PR 模板
├── build.sh build.bat                                在实时面板中编译并运行主机测试
├── eh3d.project.json                                 清单：版本、角色、成熟度和构建命令
├── CHANGELOG.md                                      每个版本的变更
├── SECURITY.md SUPPORT.md                            如何报告漏洞及在哪里提问
├── CONTRIBUTING.md CODE_OF_CONDUCT.md LICENSE        参与贡献、行为准则和许可证
└── bump_manifest_version.py                          清单的版本工具
```

</details>

## 🗺️ 路线图与已知限制

**目前已知的限制**

* 尚无任何内容在模块上运行：固件未编译、烧录或试用，所需的板级支持包可能需要为该模块做修改。
* 屏幕只能通过 EH3D-SERVER 的作业到达机床；它自己的动作（点动、回原点、主轴）先经过闸门，然后等待与节点的连接，而该连接尚未编写。服务器客户端和存储卡从未在模块上运行。
* 急停引脚和触摸控制器尚未确定：必须对照本机的原理图核实。
* 语音识别器和扬声器尚未进入固件；指令和提示以文本方式测试。
* 桌面程序的逻辑（格式、控制器、机群）没有复用：界面遵循桌面程序，但规则用 C++ 重写。

**下一步**

* 编译固件并启动模块：显示、触摸和声音。
* 编写与 EH3D-SERVER 和节点的连接，并补全界面的其他标签页。
* 加入语音识别器和扬声器，并配合回声消除试验。
* 接上真实的急停按钮，并在机床上试验锁存。

## 🌍 语言

README 和文本提供英语、西班牙语、法语、意大利语、德语、汉语、日语和欧洲葡萄牙语版本。

## 🔗 相关项目

* [EH3D-DRILLPRO](https://github.com/JuanenRac/EH3D-DRILLPRO): 界面所遵循的桌面程序
* [EH3D-ROUTEPRO](https://github.com/JuanenRac/EH3D-ROUTEPRO): 铣切程序；共享机床文件和协议库
* [EH3D-SERVER](https://github.com/JuanenRac/EH3D-SERVER): 家族的中心：运行所有客户端的作业
* [EH3D-NODE-BUNGARD](https://github.com/JuanenRac/EH3D-NODE-BUNGARD): 机床控制单元串口上的节点
* [EH3D-COMMON](https://github.com/JuanenRac/EH3D-COMMON): 家族的公共部分（格式、控制器、机群、主题）

## 📚 文档

* 设置：`hmi.cfg.example` 列出每个键及其默认值和范围。
* 硬件：模块的引脚以及厂商演示附带的说明，见 `docs/HARDWARE.md`。
* [更新日志](CHANGELOG.md)

## 🤝 参与贡献与社区

* 提交 pull request 之前请运行主机测试（`./build.sh` 或 `build.bat`）和 `python -I tools/ci_validate.py`：必须全部通过。参见 [CONTRIBUTING.md](CONTRIBUTING.md) 和[行为准则](CODE_OF_CONDUCT.md)。
* README 有八种语言：文本改动要在八个文件中同时进行，两幅图由 `tools/make_architecture.py` 和 `tools/make_structure.py` 根据 `docs/architecture.json` 和 `docs/structure.json` 绘制。
* 任何让屏幕能够移动机床的改动，都必须有覆盖它的闸门测试，以及在硬件上观察结果的记录。
* 请私下报告漏洞：[SECURITY.md](SECURITY.md)。问题、想法和报告：electrohobby3d@gmail.com（[SUPPORT.md](SUPPORT.md)）。

## 👤 作者

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 许可证

软件：GPL-3.0-or-later。见 [LICENSE](LICENSE)。套件的硬件设计使用 CERN OHL-S。
