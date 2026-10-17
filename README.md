# Stadia 无线手柄接收器（ESP32-S3）

把 Google Stadia 蓝牙手柄连接到 ESP32-S3，再通过 USB 让 Windows 将它识别为 Xbox 360 / XInput 手柄。本项目包含震动转发、中文网页配置、额外按键映射和在线刷入页面。

> **普通使用者不需要在自己的电脑上编译。** 本项目配置了 GitHub Actions：每次推送开发分支或提交 Pull Request 时，GitHub 云端服务器会运行自动测试并尝试编译 ESP32-S3 固件。编译产物可以从 Actions 页面下载。

## 我应该怎么做？

### 查看 GitHub 云端编译结果

1. 打开 [GitHub Actions：Build & Release](https://github.com/WuLinLee/stadia-dongle-esp-2/actions/workflows/release.yml)。
2. 找最新的任务，并确认它对应你要测试的分支或提交。
3. 查看结果：
   - **绿色勾号 / Success**：自动测试和固件编译通过。
   - **红色叉号 / Failure**：测试或编译失败，不要刷这个版本。
   - **黄色圆圈 / In progress**：GitHub 仍在执行任务。
4. 打开成功的任务，在页面的 **Artifacts** 区域下载名为 `firmware` 的压缩包。
5. 解压后会得到 `firmware-merged.bin`，这是 ESP32-S3 的完整刷写镜像。

开发版 Artifact 通常保留 7 天。**云端编译成功不等于已通过真机测试**；蓝牙配对、游戏震动和按键映射仍需实际设备验证。

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
| 震动强度换算 | 使用 `n × 257` 将 8 位强度扩展成 16 位强度，覆盖完整的 0–65535 范围 |
| Assistant / Capture 额外按键 | 可分别设置短按和长按动作及长按时间 |
| 额外按键输出 | 除键盘和媒体动作外，还可输出 XInput A/B/X/Y、LB/RB、Start/Back、L3/R3、Guide、十字键和 LT/RT |
| 虚拟按键叠加 | 映射动作叠加到当前 XInput 报告，而不是替换真实输入；离开或重置时清除虚拟按键，降低卡键风险 |
| 中文 USB 配置页 | 使用 WebHID 通过 USB 查询状态、配对/删除手柄、查看输入和电量、保存动作配置、控制 Wi-Fi 热点及重启 |
| 在线刷入页 | 通过 ESP Web Tools 在桌面版 Chrome / Edge 中连接开发板并刷入完整镜像 |
| 配置保存 | 按键动作和相关配置保存于设备 NVS；修改一般不需要重新刷固件 |
| 自动构建 | GitHub Actions 运行 Node.js 合约测试，再使用 ESP-IDF 6.0 在云端构建 ESP32-S3 固件 |

### 当前按键映射的边界

目前新增的 XInput 动作是给 **Assistant 和 Capture 这两个额外按键**使用的，支持短按、长按分别映射。它**不代表**所有 Stadia 原生按键都已经支持自由重映射；常规 A/B/X/Y、肩键、扳机和摇杆仍按当前固件的既有 Xbox 布局转换。

映射到 XInput 的普通按钮会发送约 20 ms 的虚拟按下脉冲；映射到 LT/RT 时会输出全按强度。Stadia 手柄没有独立的扳机马达，因此不能提供 Xbox Series 手柄那种扳机震动。

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
- GitHub Actions 的 Artifact 和 Release 流程：保存开发版构建产物，并在发布版本时提供下载。

## 自动编译是怎么工作的？

工作流文件位于 [`.github/workflows/release.yml`](.github/workflows/release.yml)。它不是只写在说明里的计划，而是仓库中配置的 GitHub Actions 自动化工作流。

1. **触发：**推送到 `main` 或 `codex/**` 分支、创建 Pull Request，或推送以 `v` 开头的版本标签时触发。
2. **自动测试：**执行 `npm test`，检查 USB 配置协议、固件与 JavaScript 动作编号的一致性及相关页面约定。
3. **云端编译：**GitHub 的 Ubuntu 服务器使用 ESP-IDF 6.0、目标 `esp32s3` 执行 `idf.py build`。
4. **生成完整镜像：**将 bootloader、分区表和应用镜像合并成 `firmware-merged.bin`。
5. **保存结果：**上传名为 `firmware` 的 Actions Artifact，供开发者下载；Artifact 通常保留 7 天。
6. **正式发布：**推送 `v*` 标签时，工作流会建立 GitHub Release 并附加固件文件。
7. **网页更新：**`main` 分支或版本标签的 Pages 部署流程，会准备刷入页、USB 配置页和固件，再尝试更新 GitHub Pages。

若编译失败，请打开 [Actions 页面](https://github.com/WuLinLee/stadia-dongle-esp-2/actions/workflows/release.yml)，进入失败任务查看具体步骤。重点区分 `Test USB configuration protocol`（自动测试）与 `Build with ESP-IDF`（固件编译）。不要把以前留存的固件当成失败任务产生的新版本。

**版本概念不要混淆：**

- **开发版 Artifact：**单次 Actions 运行生成的测试固件，适合开发调试。
- **GitHub Release：**由 `v*` 标签创建的发布包。
- **在线安装器：**由 GitHub Pages 提供；开发分支编译成功不会立即更新它。

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
| Assistant / Capture | 默认键盘动作，可在配置页分别修改短按和长按 |

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

## 项目文件在哪里？

| 路径 | 作用 |
|---|---|
| `main/ble_central.c` | BLE 连接、手柄槽位与震动写入处理 |
| `main/xbox_dev.c` | Xbox 兼容 USB 接口与游戏发送的震动报告 |
| `main/bridge.c` | Stadia 输入到 XInput 报告转换、虚拟按钮叠加 |
| `main/button_actions.c` | Assistant/Capture 短按、长按动作执行 |
| `main/config_store.c/.h` | 动作定义、默认值和配置保存 |
| `main/usb_config_protocol.c` | USB 配置协议固件端 |
| `main/webhid_transport.js` | 浏览器 WebHID 传输层及动作名称 |
| `main/index.html` | 中文 USB 配置界面 |
| `docs/index.html` / `docs/manifest.json` | 在线安装器与固件清单 |
| `.github/workflows/release.yml` | GitHub 云端自动测试、编译、Release 和 Pages 部署 |
| `tests/firmware_contract.test.mjs` | 自动合约测试 |

## 开发者：本地构建（可选）

普通用户不需要本地构建。若需在电脑上开发，安装 [ESP-IDF](https://github.com/espressif/esp-idf) 6.0 并准备 Node.js，然后运行：

```sh
npm test
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
