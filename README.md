# Stadia 无线手柄接收器（ESP32-S3）

把 Google Stadia 蓝牙手柄连接到 ESP32-S3，再通过 USB 让 Windows 将它识别为 Xbox 360 / XInput 手柄。项目支持双马达震动，也提供网页配置和固件在线刷入功能。

> **最重要的一点：不需要在自己的电脑上安装 ESP-IDF，也不需要自己编译。**
> 本项目使用 **GitHub Actions（GitHub 提供的云端服务器）自动测试并编译固件**。你可以在 GitHub 上下载编译结果。

## 我应该怎么做？

### 方式一：查看 GitHub 云端编译结果（开发版）

每次把代码提交到本项目的开发分支，GitHub Actions 会自动运行测试和 ESP32-S3 编译。

1. 打开 [GitHub Actions 编译页面](https://github.com/WuLinLee/stadia-dongle-esp-2/actions/workflows/release.yml)。
2. 找到最新一条 **Build & Release** 任务，确认它对应分支 `codex/rumble-write-reliability` 或你要测试的提交。
3. 点进任务查看结果：
   - **绿色勾号（Success）**：自动测试和固件编译都通过了。
   - **红色叉号（Failure）**：编译或测试失败，先不要刷这个版本。
   - **黄色圆圈（In progress）**：GitHub 还在编译。
4. 编译成功后，在任务页面下方的 **Artifacts** 区域下载名为 **firmware** 的压缩包。
5. 解压后会得到 `firmware-merged.bin`。这是完整刷写镜像，不是用于 OTA 的单独应用镜像。

**注意：**开发版的 Artifact 通常只保留 7 天。分支上的编译成功不代表真机测试已通过；震动、蓝牙配对和额外按键仍需实际连接手柄验证。

### 方式二：使用在线刷入网页（适合普通用户）

正式网页只会在修改合并到 `main` 分支、并且 GitHub Pages 部署成功后更新。部署完成后，可打开：

- [项目在线安装器](https://wulinlee.github.io/stadia-dongle-esp-2/)
- [中文 USB 配置页](https://wulinlee.github.io/stadia-dongle-esp-2/config.html)
- [GitHub Releases 正式版本](https://github.com/WuLinLee/stadia-dongle-esp-2/releases/latest)

如果在线页面尚未部署，或它显示的版本不是你要测试的开发版，请不要假设它已经包含最新修改。请先查看上面的 Actions 编译记录。

## 如何刷入固件

正式在线安装器可用时，按以下步骤操作：

1. 使用支持数据传输的 USB 线，将 ESP32-S3 连接到电脑的 **COM / UART 烧录接口**。
2. 在桌面版 **Chrome 或 Edge** 打开[在线安装器](https://wulinlee.github.io/stadia-dongle-esp-2/)。
3. 点击 **Install**，在弹出的设备列表中选择开发板对应的串口。
4. 等待刷入完成；过程中不要拔掉 USB。
5. 拔掉 USB，改接开发板的 **原生 USB / OTG 接口**，这是日常连接电脑使用的接口。
6. 在 Windows 按 `Win + R`，输入 `joy.cpl` 并回车，检查是否出现 Xbox 360 兼容手柄。

不同开发板接口标记可能不同，请以板子说明书为准。如果电脑没有识别到手柄，先确认已从 COM/UART 接口换到了 USB/OTG 接口，再按一次 RESET/RST。

## 功能

- 最多连接 2 个 Stadia 手柄。
- USB 输出 Xbox 360 / XInput 按键、摇杆和扳机。
- 将游戏的双马达震动转发给 Stadia 手柄。
- Assistant / Capture 额外按键支持配置短按和长按动作。
- 额外按键动作可选择键盘、媒体键或 XInput 按钮（A/B/X/Y、LB/RB、Start/Back、L3/R3、Guide、十字键、LT/RT）。
- 中文网页可用于手柄配对、查看状态、修改动作和管理 Wi-Fi 热点。
- Wi-Fi 热点默认关闭；日常 USB 配置不需要改变电脑的 Wi-Fi。

Stadia 手柄没有 Xbox 扳机马达，因此无法提供 Xbox Series 手柄那种独立的扳机震动效果。

## 第一次使用

1. 通过 USB 配置页连接接收器。
2. 点击 **开始配对**。
3. 按住手柄的 **Stadia + Y**，直到指示灯开始闪烁黄色或橙色。
4. 等待配置页显示手柄 **已就绪**。

**“已连接”和“已就绪”不同。**请等到“已就绪”后再测试输入和震动。刷写固件后若手柄无法自动重连，可在配置页忘记该手柄，再重新配对。

## 默认按键

| Stadia 按键 | 默认输出 |
|---|---|
| A / B / X / Y | Xbox A / B / X / Y |
| Menu / Options | Start / Back |
| Stadia 按键 | Guide |
| LB / RB、LT / RT | 对应肩键和扳机 |
| 摇杆、方向键 | 对应摇杆和方向键 |
| Assistant / Capture | 默认映射为键盘动作，可在网页中修改 |

额外按键的映射保存在接收器中，正常情况下修改配置不需要重新刷写固件。

## 震动修复说明

本项目正在改进 BLE 震动写入的可靠性，包括写入失败后的重试、减少过期震动更新挤占，以及更准确地转换马达强度。这些改动主要针对快速改变震动强度或停止震动时，可能出现的震动节奏不准、停止指令丢失等情况。

是否彻底解决具体游戏中的问题，仍需使用真实 ESP32-S3 开发板和 Stadia 手柄测试。**GitHub 编译通过只代表代码通过自动检查和编译，不等于已经完成真机验证。**

## GitHub 云端编译失败怎么办？

1. 打开 [Actions 页面](https://github.com/WuLinLee/stadia-dongle-esp-2/actions/workflows/release.yml)。
2. 点击带红色叉号的任务。
3. 查看失败的步骤：`Test USB configuration protocol` 是自动测试；`Build with ESP-IDF` 是固件编译。
4. 将失败步骤底部的错误信息发给维护者。不要把失败任务生成的旧固件当成新版本。

如果完全看不到编译任务，请检查仓库的 **Actions** 是否已启用；代码提交到 `codex/**` 分支或创建 Pull Request 时，工作流会自动触发。

## 发布版本与在线网页如何更新？

本项目的自动工作流负责：

1. 在 GitHub 服务器运行配置协议测试。
2. 使用 ESP-IDF 6.0 为 ESP32-S3 编译固件。
3. 将完整刷写镜像作为 `firmware` Artifact 上传。
4. 正式发布 `v*` 标签时，把固件附加到 GitHub Release。
5. 当代码进入 `main` 且构建成功时，将新的固件和网页发布到 GitHub Pages。

开发分支的成功构建不会自动更新正式在线安装器；需要合并到 `main` 并成功部署网页。你可以在 [Actions](https://github.com/WuLinLee/stadia-dongle-esp-2/actions/workflows/release.yml) 查看进度，在 [Releases](https://github.com/WuLinLee/stadia-dongle-esp-2/releases) 下载正式发布的固件。

## 常见问题

**刷完后没有出现 Xbox 手柄**

- 确认已从 COM/UART 烧录接口换到原生 USB/OTG 接口。
- 按一次 RESET/RST，等待几秒。
- 更换确认支持数据传输的 USB 线。
- 在 Windows 运行 `joy.cpl` 检查，不要只依赖系统通知。

**手柄一直黄灯闪烁，或震动正常但没有输入**

在 USB 配置页忘记该手柄，关闭手柄，重新点击“开始配对”，再按住 Stadia + Y，直到页面显示“已就绪”。

**找不到 Wi-Fi 热点**

这是正常的：Wi-Fi 默认关闭。请先在 USB 配置页手动启动热点；它主要用于 OTA 或 USB 配置不可用时的恢复。

**要改按键是否需要重新刷固件？**

不需要。请通过 USB 配置页修改并保存。

## 开发者资料

普通用户不需要本地编译。代码修改提交到 GitHub 后，可用本页前面的 Actions 链接查看云端测试与编译结果。

- [USB 配置协议](docs/usb-config-protocol.md)
- [真机验收清单](docs/hardware-test-checklist.md)
- [产品与界面原则](PRODUCT.md)
- [GitHub Actions 工作流](.github/workflows/release.yml)

## 许可证

[GNU General Public License v3.0](LICENSE)
