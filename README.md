# Stadia 无线手柄接收器（ESP32-S3）

把 Google Stadia 蓝牙手柄连接到 ESP32-S3，再通过 USB 让 Windows 将它识别为 Xbox 360 / XInput 手柄。本项目包含震动转发、三档震动 Gamma 曲线固件、中文网页配置、额外按键映射和在线刷入页面。


## 快速开始

**如果你只是想使用接收器：**

1. **确认硬件：**需要带原生 USB OTG 的 ESP32-S3 开发板，以及支持数据传输的 USB 线。不同板子的 COM/UART 烧录口与原生 USB 口可能是两个不同接口。
2. **准备手柄：**Stadia 手柄必须已经切换到 Bluetooth（BLE）模式。Google 的蓝牙转换会永久关闭手柄原有的 Stadia Wi-Fi 功能；请在转换前确认自己接受这一变化。
3. **刷写固件：**用桌面版 Chrome 或 Edge 打开[在线刷入页](https://wulinlee.github.io/stadia-dongle-esp-2/)，选择 Reference、Gentle 或 Punch 曲线；默认 `firmware-merged.bin` 是 Reference 线性版。也可从 [最新 GitHub Release](https://github.com/WuLinLee/stadia-dongle-esp-2/releases/latest) 下载对应镜像。开发分支或 PR 测试才使用 [Actions 运行记录](https://github.com/WuLinLee/stadia-dongle-esp-2/actions/workflows/release.yml) 的临时 Artifact（通常保留 7 天）。
4. **切换 USB 接口：**刷写完成后，按开发板说明改接原生 USB OTG 接口，再连接电脑使用。
5. **配对和配置：**打开[USB 配置页](https://wulinlee.github.io/stadia-dongle-esp-2/config.html)，连接接收器、开始配对，并按页面提示操作手柄。
6. **验证输入：**Windows 可运行 `joy.cpl`，确认系统能看到 Xbox 360 兼容手柄，再测试按键和震动。

> **注意：**GitHub Actions 显示构建成功，只能说明自动测试和固件编译通过；不能替代开发板、手柄和游戏的实际测试。第一次刷写前请确认板型与 USB 接口，刷写过程中不要断开连接。

## 与上游网页刷写器 / 固件有什么区别？

对照上游 [danzig666/stadia-dongle-esp](https://github.com/danzig666/stadia-dongle-esp) 的 `main` 分支检查过源码和 GitHub Actions。**刷写页面的工作方式基本相同，但实际下载并写入开发板的固件不是同一个二进制文件**：两个仓库分别从各自源码构建。

| 项目 | 本仓库（WuLinLee） | 上游（danzig666） |
|---|---|---|
| 网页刷写技术 | ESP Web Tools 10 + `manifest.json` | 相同 |
| 芯片 / 主刷写偏移 | ESP32-S3，完整镜像写入偏移 `0x0` | 相同 |
| 刷写页 | 中文说明、下载完整 `.bin`、配置页和 Releases 入口 | 英文说明，主要提供 Install 按钮和配置页入口 |
| 核心用途 | Stadia BLE → USB Xbox 360/XInput、双手柄和配置功能，并包含本仓库的改动 | 相同的基础架构和大部分既有功能 |
| 震动转发 | 保持串行 BLE 写入、失败退避重试、最新目标优先；新增 Reference / Gentle / Punch 三档输出映射 | 上游自己的实现；不能把两边的行为假定为完全一致 |
| Assistant / Capture | 本仓库新增每键一个动作的按下/松开映射，可保持 XInput 按键 | 已有可配置额外按键动作，但无本仓库新增的 held XInput mapping |
| USB 配置与文档 | 中文 USB WebHID 配置页、协议合约测试和维护交接文档 | 原有 USB 配置 / WebHID 功能及上游文档 |

### 一个需要下一个维护者特别检查的固件打包差异

两边的 `partitions.csv` 都定义了位于 `0xf000` 的 `otadata` 分区，应用从 `0x20000` 开始。上游构建流程把 `build/ota_data_initial.bin` 一并合入 `firmware-merged.bin`，并另外生成、发布仅含应用程序的 `stadia-dongle-ota.bin`，供设备网页 OTA 更新使用。

**当前本仓库的 workflow 还没有做这两步**：合并完整镜像时只合入 bootloader、分区表和应用；Actions Artifact / Release 也只上传 `firmware-merged.bin`。本仓库的设备端网页 OTA 处理代码使用 ESP-IDF OTA API 写入下一个应用分区，因此应继续核实它所需的应用镜像获取和 OTA 更新路径。不要把完整的 `firmware-merged.bin` 当成 OTA 应用文件直接上传。

这并不自动证明在线 Install 刷写会失败：当前 manifest 明确刷写的是完整镜像（`offset: 0`），而不是 OTA 应用镜像。由于本仓库和上游分区表一致，**下一个 AI 应在修改打包流程前，先查看 `partitions.csv`、`main/web_server.c` OTA handler 和 workflow，补齐/验证 OTA 文件及 `otadata` 初始化，再用 CI 和实机冷启动、升级回滚路径验证**。当前差异需要明确跟踪，不要无依据地称两个构建产物完全等价。

另外，上游 workflow 显式拉取完整 Git 历史用于固件版本标记；本仓库 workflow 目前使用默认浅克隆。维护者也应核实配置页显示的固件版本是否可靠。

## 目录

- [与上游网页刷写器 / 固件有什么区别](#与上游网页刷写器--固件有什么区别)
- [功能与实现情况](#功能与实现情况)
- [开源参考与致谢](#参考了哪些开源项目复用了什么)
- [自动构建与发布](#自动编译是怎么工作的)
- [刷入固件](#刷入固件)
- [第一次配对](#第一次配对)
- [默认按键与限制](#默认按键)
- [常见问题](#常见问题)
- [项目文件](#项目文件在哪里)
- [维护者接手指南](#维护者--下一个-ai接手开发)
- [本地构建](#开发者本地构建可选)


> **普通使用者不需要在自己的电脑上编译。** 本项目使用 GitHub Actions 自动测试并构建 ESP32-S3 固件。每次成功的 `main` 分支构建都会自动创建一个长期保存的 GitHub Release，并上传完整刷写镜像；`codex/**` 开发分支和 Pull Request 主要用于测试，可从对应运行记录下载短期 Artifact。

## 我应该怎么做？

### 查看 GitHub 云端编译结果

1. 打开 [GitHub Actions：Build & Release](https://github.com/WuLinLee/stadia-dongle-esp-2/actions/workflows/release.yml)，选择与目标分支和提交 SHA 对应的运行；不要只看最上方的旧绿色记录。
2. 查看 `Test USB configuration protocol` 和 `Build with ESP-IDF` 是否都成功。失败或仍在运行的构建，不应当作为已验证固件刷入。
3. **普通使用者 / 最新正式构建：**打开 [最新 GitHub Release](https://github.com/WuLinLee/stadia-dongle-esp-2/releases/latest)，下载 `firmware-merged.bin`。该文件是从偏移 `0x0` 开始刷写的完整 ESP32-S3 镜像。
4. **验证 PR 或开发分支：**进入对应 Actions 运行的 **Artifacts** 下载 `firmware` 压缩包。包内有三种曲线镜像和兼容旧链接的 Reference `firmware-merged.bin`；确认它来自正确 head SHA。

Actions Artifact 通常只保留 7 天；Release 附件用于长期保存正式构建。**CI 通过不等于真机验收通过**；蓝牙配对、游戏震动和按键映射仍需在实际开发板与手柄上验证。

### 使用在线刷入网页

正式在线安装器会在代码合并到 `main` 并成功部署 GitHub Pages 后更新。相关入口：

- [项目在线安装器](https://wulinlee.github.io/stadia-dongle-esp-2/)
- [中文 USB 配置页](https://wulinlee.github.io/stadia-dongle-esp-2/config.html)
- [GitHub Releases 正式版本](https://github.com/WuLinLee/stadia-dongle-esp-2/releases/latest)
- [所有 GitHub Actions 编译任务](https://github.com/WuLinLee/stadia-dongle-esp-2/actions/workflows/release.yml)

开发分支的成功构建**不会直接更新正式在线安装器**。如果网页还没有部署，或网页内的版本并非当前开发版，请先查看 Actions 编译记录。

## 功能与实现情况

以下内容指本项目修复分支中的源码实现；是否已经成功编译、是否能在你的板子上正常运行，请以 GitHub Actions 和真机测试结果为准。

| 功能 | 实现内容 |
|---|---|
| Stadia 蓝牙连接 | 基于 ESP32-S3 BLE / NimBLE；维护独立手柄连接槽位，最多支持 2 个手柄 |
| Xbox / XInput USB | 通过 TinyUSB 自定义 Xbox 360 兼容 USB 接口输出按键、方向键、摇杆和扳机 |
| 双马达震动 | 读取 Xbox 输出报告里的大小马达强度，映射为 Stadia 手柄的两个马达 |
| 震动可靠性 | 每个手柄独立维护震动目标；串行处理 BLE 输出写入；写入失败时退避重试；快速变化时优先保留最新目标，降低停止震动指令丢失和旧震动积压的风险 |
| 震动强度换算 | Reference 默认使用 `n × 257`；另可刷入 Gentle（gamma 0.90）或 Punch（gamma 0.78），均保留零点和满量程 |
| Assistant / Capture 额外按键 | 每个按键独立配置一个动作；按下保持输出，松开释放 |
| 额外按键输出 | 除键盘和媒体动作外，还可输出 XInput A/B/X/Y、LB/RB、Start/Back、L3/R3、Guide、十字键和 LT/RT |
| 虚拟按键叠加 | 映射动作叠加到当前 XInput 报告；鼠标模式/断连时输出中立报告，物理松开后释放映射，仍按住的动作会在退出鼠标模式后恢复 |
| 中文 USB 配置页 | 使用 WebHID 通过 USB 查询状态、配对/删除手柄、查看输入和电量、保存动作配置、控制 Wi-Fi 热点及重启 |
| 在线刷入页 | 通过 ESP Web Tools 在桌面版 Chrome / Edge 中连接开发板并刷入完整镜像 |
| 配置保存 | 按键动作和相关配置保存于设备 NVS；修改一般不需要重新刷固件 |
| 自动构建 | GitHub Actions 运行 Node.js 合约测试，再使用 ESP-IDF 6.0 在云端构建 ESP32-S3 固件 |

### 震动曲线 / Gamma 固件

参考 [WuLinLee/Stadia](https://github.com/WuLinLee/Stadia) 的公开实现，增加编译期三档震动幅值映射。**本功能只改变两路马达强度公式；原有 BLE 写入队列、失败退避重试、多手柄隔离和停止震动处理保持不变。**

| 刷入选项 | 公式 | 说明 |
|---|---|---|
| **Reference · 线性（默认）** | `y = x × 257` | 原有线性映射；`firmware-merged.bin` 仍为此版本，兼容旧下载地址 |
| **Gentle · 温和增强** | `y = round(65535 × (x / 255)^0.90)` | 低/中强度略微增强 |
| **Punch · 较强增强** | `y = round(65535 × (x / 255)^0.78)` | 低/中强度增强更明显 |

三档都将输入 0 映射为 0、输入 255 映射为 65535。在线安装页有三个独立刷写入口；曲线在编译时确定，不能通过 USB 配置页实时切换，换档需重新刷写。Gentle/Punch 是实验曲线，不是已证实的 Google 原版算法，实际震感应在目标游戏和设备上 A/B 验证。

### 当前按键映射的边界

目前新增的映射功能是给 **Assistant 和 Capture 这两个额外按键**使用的，每个按键只能配置一个动作。动作会在物理按键按下时持续输出，松开时释放。它**不代表**所有 Stadia 原生按键都已经支持自由重映射；常规 A/B/X/Y、肩键、扳机和摇杆仍按当前固件的既有 Xbox 布局转换。

**鼠标模式快捷切换：**连续按住“十字键下 + Assistant”或“十字键下 + Capture”任一组合满 2 秒，即切换鼠标模式。组合条件（十字键下且 Assistant/Capture 至少一个按下）中断时会重置计时；组合键成立期间，Assistant 和 Capture 各自配置的独立映射会被抑制，避免意外输入。

映射到 XInput 的普通按钮会在物理按键保持按下期间持续按下，松开后释放；映射到 LT/RT 时会输出全按强度。Stadia 手柄没有独立的扳机马达，因此不能提供 Xbox Series 手柄那种扳机震动。

## 参考了哪些开源项目？复用了什么？

本项目优先参考现成的开源实现，并只为当前工程补上必要的兼容层。参考不等于整套复制另一个项目的固件。

### 1. [danzig666/stadia-dongle-esp](https://github.com/danzig666/stadia-dongle-esp) — 原始项目基础

本仓库从该项目 fork。当前工程沿用其 Stadia 手柄接入 ESP32-S3、通过 USB 暴露 Xbox 兼容手柄及配套配置的总体方向，再在本仓库维护和扩展代码。

### 2. [Scalee/stadia-dongle](https://github.com/Scalee/stadia-dongle) — 震动写入可靠性

重点参考了这个项目针对高负载震动写入问题的修复：[相关修复提交 d92ac6d](https://github.com/Scalee/stadia-dongle/commit/d92ac6d3e2b7bd07d716940a5e160aa1a215d655)。

参考的思路包括：

- BLE 写入失败时重试，而不是只记录错误就放弃。
- 不让过期的震动更新长期排队；优先处理当前最新的马达强度。
- 写入尚未完成时保护正在发送的数据，避免被后续震动更新覆盖。
- 减少不必要的重复写入，并注意 BLE 连接参数对写入节奏的影响。

当前 fork 按现有的**多手柄槽位结构**实现自己的异步写入和重试流程，而不是直接把上游单手柄代码原样覆盖进来。目标是降低游戏快速变化震动强度时丢失停止指令、震动拖尾或节奏被旧更新挤占的风险；仍需真机游戏测试验证最终效果。

### 3. [Joypad OS](https://github.com/joypad-ai/joypad-os) — Stadia BLE 报告和强度换算

参考了 Joypad OS 的 [Stadia BLE 驱动](https://github.com/joypad-ai/joypad-os/blob/main/src/bt/bthid/devices/vendors/google/stadia_bt.c)，尤其是 Stadia 双马达输出报告的处理方式。它使用 `n × 257` 把 8 位马达强度扩展为 16 位值；本项目也采用这种换算，避免只填 16 位强度的高字节而损失分辨率。

还参考了 Joypad OS 对通用按键映射/Profile 的设计思路，相关说明见 [Profiles 文档](https://github.com/joypad-ai/joypad-os/blob/main/docs/core/profiles.md)。但 Joypad OS 的 `bt2usb` 构建目标并没有启用 Profile 系统，而且其预编译固件按具体板型提供，因此**没有把它的整套固件当作当前通用 ESP32-S3 开发板的直接替换件**。本项目当前先实现 Assistant/Capture 额外按键到 XInput 按钮的映射，并保留当前 USB / WebHID 配置架构。

### 4. 构建与刷写工具

- [ESP-IDF](https://github.com/espressif/esp-idf)：ESP32-S3 固件构建系统。CI 中使用 ESP-IDF 6.0。
- [espressif/esp-idf-ci-action](https://github.com/espressif/esp-idf-ci-action)：让 GitHub Actions 云端环境运行 ESP-IDF 编译。
- [ESP Web Tools](https://github.com/esphome/esp-web-tools)：浏览器内的串口刷写界面组件。
- GitHub Actions 的 Artifact 和 Release 流程：Artifact 供短期开发测试；每次成功的 `main` 构建会自动发布固件到长期保存的 GitHub Release。

## 自动编译是怎么工作的？

工作流文件位于 [`.github/workflows/release.yml`](.github/workflows/release.yml)。它不是只写在说明里的计划，而是仓库中配置的 GitHub Actions 自动化工作流。

1. **触发：**推送到 `main` 或 `codex/**` 分支、创建 Pull Request、推送以 `v` 开头的标签，或手动运行工作流时触发。
2. **自动测试：**`npm test` 检查协议/页面合约和 Gamma 入口；GCC 另会逐一测试三档 C 映射公式。
3. **云端编译：**ESP-IDF 6.0 分别编译 Reference（profile 0）、Gentle（profile 1）和 Punch（profile 2）。
4. **生成镜像：**每档单独合并 bootloader、分区表和应用（偏移 `0x0`、`0x8000`、`0x20000`），输出 `firmware-reference-merged.bin`、`firmware-gentle-merged.bin`、`firmware-punch-merged.bin`。同时将 Reference 复制为兼容旧地址的 `firmware-merged.bin`。当前仍未将 `build/ota_data_initial.bin` 合入完整镜像，也未生成 app-only 的 `stadia-dongle-ota.bin`；详见[上游差异与待检查项](#与上游网页刷写器--固件有什么区别)。
5. **保存结果：**`firmware` Artifact 内有三档镜像及一个向后兼容的 Reference 别名；Artifact 通常保留 7 天。
6. **正式发布：**每次 `main` 成功构建自动创建 Latest Release，上传上述四个 `.bin` 文件。推送 `v*` 标签时沿用该标签发布。
7. **网页更新：**成功的 `main` 或 `v*` 标签构建会把四个完整镜像、三份曲线 manifest、刷写页和 USB 配置页一起部署。`codex/**` 和 PR 不会覆盖正式网页。

若编译失败，请打开 [Actions 页面](https://github.com/WuLinLee/stadia-dongle-esp-2/actions/workflows/release.yml)，进入失败任务查看具体步骤。重点区分 `Test USB configuration protocol`（自动测试）与 `Build with ESP-IDF`（固件编译）。不要把以前留存的固件当成失败任务产生的新版本。

**版本概念不要混淆：**

- **开发版 Artifact：**单次 Actions 运行生成的测试固件，适合开发调试。
- **GitHub Release：**每次成功的 `main` 构建会自动创建一个最新 Release；显式推送 `v*` 标签也会发布。Release 附件长期保存，适合手动下载。
- **在线安装器：**由 GitHub Pages 提供；只有成功的 `main` 或 `v*` 构建会触发部署。`codex/**` 开发分支和 PR 的成功构建不会更新正式网页。

## 刷入固件

正式在线安装器可用时：

1. 使用支持数据传输的 USB 线，将 ESP32-S3 连接到电脑的 **COM / UART 烧录接口**。
2. 在桌面版 Chrome 或 Edge 打开[在线安装器](https://wulinlee.github.io/stadia-dongle-esp-2/)。
3. 点击 **Install**，在弹出的设备列表中选择开发板对应的串口。
4. 等待刷入完成；过程中不要拔掉 USB。
5. 拔掉 USB，改接开发板的 **原生 USB / OTG 接口**，这是日常连接电脑使用的接口。
6. 在 Windows 按 `Win + R`，输入 `joy.cpl` 并回车，检查是否出现 Xbox 360 兼容手柄。

不同开发板接口标记可能不同，请以开发板说明书为准。若只有一个 USB 接口，请按开发板说明确认它是否支持 USB OTG 与刷写模式。

## 第一次配对

1. 使用 USB 连接接收器，打开[中文 USB 配置页](https://wulinlee.github.io/stadia-dongle-esp-2/config.html)。
2. 点击 **连接 USB**，在浏览器设备列表中选择接收器。
3. 点击 **开始配对**。
4. 按住手柄 **Stadia + Y**，直到指示灯闪烁黄色或橙色。
5. 等待页面显示手柄 **已就绪**。

“已连接”和“已就绪”不同；请等到“已就绪”后再测试按键和震动。刷写后若手柄无法自动重连，可以在配置页忘记手柄，然后重新配对。

## 默认按键

| Stadia 按键 | 默认输出 |
|---|---|
| A / B / X / Y | Xbox A / B / X / Y |
| Menu / Options | Start / Back |
| Stadia 按键 | Guide |
| LB / RB、LT / RT | 对应肩键和扳机 |
| 摇杆、方向键 | 对应摇杆和方向键 |
| Assistant / Capture | 默认分别映射为 F14 / F12，可在配置页各自选择一个动作 |

额外按键动作保存后通常不需要重新刷写。配置页通过 USB 与接收器通信，不需要登录帐号，不会把手柄输入上传到云端。

## 常见问题

**刷完后没有出现 Xbox 手柄**

- 确认已经从 COM/UART 烧录接口换到了原生 USB/OTG 接口。
- 按一次 RESET/RST，等待几秒。
- 更换一根确认支持数据传输的 USB 线。
- 在 Windows 运行 `joy.cpl` 检查，不要只依赖系统通知。

**手柄一直黄灯闪烁，或震动正常但没有输入**

在 USB 配置页忘记该手柄，关闭手柄，重新点击“开始配对”，再按住 Stadia + Y，直到页面显示“已就绪”。

**看不到 Wi-Fi 热点**

这是正常行为：Wi-Fi 默认关闭。先在 USB 配置页手动启动热点；它主要用于 OTA 或 USB 配置不可用时的恢复。

**改按键需要重新刷固件吗？**

不需要。通过 USB 配置页修改并保存即可。

## 文档导航：哪个文件该看？

当前 `main` 分支只有一份名为 `README.md` 的 README：就是本文件。其他几个常被一起看到的 Markdown 文件不是重复 README，而是按用途拆分的文档。**不要把它们合并成一份，也不要只改其中一份就假设其他文档已同步。**

| 文件 | 面向谁 | 应该放什么 |
|---|---|---|
| [`README.md`](README.md) | 普通使用者 + 接手者入口 | 项目概览、刷机/使用说明、重要风险提示、PR #4 历史摘要、其他文档入口；保持导航简明 |
| [`AI_HANDOFF.md`](AI_HANDOFF.md) | 下一个 AI / 维护者 | 架构索引、协议和 NVS 兼容约束、CI/Release/Pages 排查步骤、待核实事项；动态状态必须重新查证 |
| [`PRODUCT.md`](PRODUCT.md) | UI/产品开发者 | 产品目标、交互原则、可访问性与设计边界；不要放临时 CI 状态 |
| [`docs/usb-config-protocol.md`](docs/usb-config-protocol.md) | 固件与网页开发者 | WebHID 协议字段、命令、状态码和兼容性约束 |
| [`docs/hardware-test-checklist.md`](docs/hardware-test-checklist.md) | 真机测试者 | 可逐项执行的硬件回归清单，并记录哪些测试确实完成 |
| [`.github/workflows/release.yml`](.github/workflows/release.yml) | CI/发布维护者 | 实际自动测试、固件构建、Release 与 Pages 部署流程（这是工作流文件，不是说明文档） |

**PR #4 说明：**请继续阅读下方“维护者 / 下一个 AI：接手开发”里的交接记录。PR #4 已关闭且未合并；README 内容先被单独复制到 `main`，之后又逐文件审查并单独移植了有用的 `AI_HANDOFF.md` 与 `PRODUCT.md` 文档更新（见这两个文件的提交历史）。这些内容不是通过合并 PR #4 进入 main 的。不要重新合并旧分支；以当前 `main` 文件为准。

## 项目文件在哪里？

| 路径 | 作用 |
|---|---|
| `main/ble_central.c` | BLE 连接、手柄槽位与震动写入处理 |
| `main/xbox_dev.c` | Xbox 兼容 USB 接口与游戏发送的震动报告 |
| `main/bridge.c` | Stadia 输入到 XInput 报告转换、虚拟按钮叠加 |
| `main/button_actions.c` | Assistant/Capture 按下/松开状态和动作执行 |
| `main/config_store.c/.h` | 动作定义、默认值和配置保存 |
| `main/usb_config_protocol.c` | USB 配置协议固件端 |
| `main/webhid_transport.js` | 浏览器 WebHID 传输层及动作名称 |
| `main/index.html` | 中文 USB 配置界面 |
| `docs/index.html` / `docs/manifest.json` | 在线安装器与固件清单 |
| `.github/workflows/release.yml` | GitHub 云端自动测试、编译、Release 和 Pages 部署 |
| `main/rumble_map.c/.h` | Reference / Gentle / Punch 三档编译期震动强度曲线 |
| `tests/firmware_contract.test.mjs` | 协议命令/能力位、动作 ID 顺序、HID 描述符与固件/页面源码约定 |
| `tests/rumble_map_contract.test.mjs` | 曲线集成、manifest、发布产物与网页入口合约测试 |
| `tests/test_rumble_map.c` | GCC 主机端 Gamma 公式测试 |
| `tests/webhid_transport.test.mjs` | WebHID 报文解析、命令握手、待处理轮询、页面并发锁和断连/错误路径 |
| `tests/pages_contract.test.mjs` | Pages 文件准备、部署条件和配置页运行时依赖约定 |

## 维护者 / 下一个 AI：接手开发

> **PR #4 交接记录（请先读）**
>
> - [PR #4：docs: refresh release status, tests, and maintainer handoff](https://github.com/WuLinLee/stadia-dongle-esp-2/pull/4) 已**关闭且未合并**。不要把它描述成已合并的 PR，也不要为了“清理历史”重新打开或合并它。
> - PR #4 的分支是 [`codex/docs-current-release-and-tests`](https://github.com/WuLinLee/stadia-dongle-esp-2/tree/codex/docs-current-release-and-tests)，改动涉及 `README.md`、`AI_HANDOFF.md`、`PRODUCT.md`。为保留有用的用户说明，PR #4 的 README 内容后来被单独复制到 `main`；另外两个文件当时并未随之自动进入 main，之后已经过逐文件审查，将有用更新分别移植到当前 `main`（`AI_HANDOFF.md` 与 `PRODUCT.md`）。这不代表 PR #4 本身已合并。
> - 接手时先比较 `main` 与该分支的 `AI_HANDOFF.md`、`PRODUCT.md`，逐项判断是否仍准确、是否有价值，再决定是否单独移植；不要整支合并，也不要覆盖 main 上后来更新的内容。
> - PR #4 中记录的 Release 版本、Actions run、SHA 和 OTA 差异属于当时核对到的状态。尤其 OTA 打包问题应重新检查当前 workflow、分区表、OTA handler 和线上页面，不能只凭这段历史文字断定当前仍然存在或已经修复。
> - 用户已报告：**通过网页刷机后，设备能正常工作，按键与震动正常**。这是一条用户反馈，不是完整的冷启动、OTA、回滚或长期稳定性测试结果。不要因为已能正常使用就贸然改固件；若计划修改刷写/OTA 流程，先说明风险并准备可恢复方案。
>

如果要继续修改固件、WebHID 协议、按键映射或发布流程，请先读 [`AI_HANDOFF.md`](AI_HANDOFF.md) 和 [`PRODUCT.md`](PRODUCT.md)，再检查对应的源码与测试。本文面向使用者的说明不取代交接文档中更细的约束。

下一个 AI 应按这个顺序接手：

1. **先核实实时状态：**打开 [最新 Actions 运行](https://github.com/WuLinLee/stadia-dongle-esp-2/actions/workflows/release.yml)、[Releases](https://github.com/WuLinLee/stadia-dongle-esp-2/releases) 和 [main 分支](https://github.com/WuLinLee/stadia-dongle-esp-2/tree/main)，确认最新提交 SHA、build、release 和 Pages 三个 job；文档里的日期、PR 记录和旧 SHA 只是历史线索。
2. **先理解关键约束：**阅读 `.github/workflows/release.yml`、`partitions.csv`、`main/web_server.c`、`main/button_actions.c`、`main/bridge.c`、`main/config_store.h`、`main/usb_config_protocol.c`、`main/webhid_transport.js` 及相关 `tests/*.test.mjs`，不要只依据 README 推断实现细节。
3. **改动要有合约测试：**运行 `npm test`，同步检查固件与网页动作 ID/协议字段；动作枚举和 NVS 数据布局不可随意重排。固件变更还要让 ESP-IDF 6.0 的 CI 构建成功。
4. **检查正式发布与网页：**`main` 构建成功会自动创建 Latest Release，并上传 `firmware-merged.bin`；不要把 7 天的 Actions Artifact 当长期下载。还要确认 Pages 部署 job 成功，并实际检查安装器、`manifest.json`、固件 URL 和 `config.html`。
5. **保留未完成事项的事实边界：**CI 和部署成功都不等于真机验收。尤其检查当前完整镜像未合入 `build/ota_data_initial.bin`、Release 未生成 app-only `stadia-dongle-ota.bin` 的已知差异；修改前先追踪 `main/web_server.c` OTA handler 和网页 OTA 上传流程，不能把完整镜像当 OTA 应用文件上传。

截至最近一次核对，PR #1 与相关功能 PR 已经合并到 `main`。每次接手仍须以 [实时 Actions](https://github.com/WuLinLee/stadia-dongle-esp-2/actions/workflows/release.yml) 和线上验证为准；自动测试或固件编译通过不代表网页在线可用，更不代表真机验证已通过。

## 开发者验证（可选）

普通用户不需要本地构建。修改固件、WebHID 协议、配置页面或发布工作流后，建议先运行自动测试，再进行相应的 ESP-IDF 构建和真机验收。

### 自动测试

在仓库根目录运行：

```sh
npm test
```

该命令执行 `node --test tests/*.test.mjs`，目前包含三组测试：

- `firmware_contract.test.mjs`：检查固件与 JavaScript 的命令值、能力位和动作名称顺序，并检查 HID 描述符及相关页面约定。
- `webhid_transport.test.mjs`：用模拟 WebHID 设备检查报文解析、握手与待处理轮询、并发锁、协议错误和断连路径。
- `pages_contract.test.mjs`：检查独立 USB 配置页的发布准备、Pages 部署条件，以及配置页没有外部运行时依赖。

这些是源码/合约测试，不会连接真实 ESP32-S3，也不会验证 BLE 无线链路、实际游戏震动、完整刷写后的冷启动或设备端 OTA 回滚。硬件相关改动仍需按照[真机验收清单](docs/hardware-test-checklist.md)执行，并如实记录未执行的测试。

### 本地固件构建

若需在电脑上开发，安装 [ESP-IDF](https://github.com/espressif/esp-idf) 6.0 并准备 Node.js，然后运行：

```sh
idf.py set-target esp32s3
idf.py build
idf.py -p <串口> flash
```

GitHub 云端编译成功后也可以直接下载完整镜像；一般开发者无需为了编译固件在本机安装整套工具链。

更多文档：

- [USB 配置协议](docs/usb-config-protocol.md)
- [真机验收清单](docs/hardware-test-checklist.md)
- [产品与界面原则](PRODUCT.md)
- [GitHub Actions 工作流](.github/workflows/release.yml)

## 许可证

[GNU General Public License v3.0](LICENSE)
