# AI 开发交接说明（Maintainer Handoff）

> 这份文档是给接手仓库的维护者或 AI 编程助手看的，不替代面向普通用户的 [README.md](README.md)。其中的 CI、PR 和 Pages 状态会变化；开始工作时务必打开文中的实时链接重新核对，不要把这份快照当成最新事实。
>
> 最近核对日期：2026-10-18

## 30 秒了解项目

本仓库是 [danzig666/stadia-dongle-esp](https://github.com/danzig666/stadia-dongle-esp) 的 fork。目标是运行在 ESP32-S3 上，把 Google Stadia BLE 手柄转换为 USB Xbox 360 / XInput 手柄，并保留震动、中文 USB 配置页、Assistant/Capture 额外按键动作和浏览器在线刷机流程。

**项目原则：**
- 优先复用成熟开源项目的协议和实现，不要重写 BLE HID、XInput 或 WebHID 底层，只做当前固件需要的适配。
- 普通用户无需本地安装 ESP-IDF；构建应优先通过 GitHub Actions 完成。
- 清楚区分“代码已实现”“CI 构建通过”“网页已部署”和“真机/游戏验证通过”，不要把它们混为一谈。
- 用户特别关心震动及时停止、额外按键映射、可用的刷机网页，以及充分的中文文档。

## 仓库与当前工作状态

- 仓库：<https://github.com/WuLinLee/stadia-dongle-esp-2>
- 主分支：`main`（PR #1 已于 2026-10-17 合并；合并提交 `f755823735f31b7625a358b5650860d992f3ba41`）。开发分支 `codex/rumble-write-reliability` 是历史工作分支，不要假设它仍是待合并状态。
- 历史 PR：[#1 — fix: improve Stadia rumble and add XInput button mapping](https://github.com/WuLinLee/stadia-dongle-esp-2/pull/1)
- CI 实时状态：<https://github.com/WuLinLee/stadia-dongle-esp-2/actions/workflows/release.yml>
- 工作流：[`.github/workflows/release.yml`](.github/workflows/release.yml)

交接开始时，重新打开 PR 和 Actions，确认 PR 是否仍然 open、head SHA 是什么、最新一次运行是否对应当前 head，以及每个 job 的结论。不要只看仓库中旧的绿色运行记录。

### 已知的 Pages/CI 注意事项

1. 用户报告过在线刷机网站返回 “There isn't a GitHub Pages site here” 的 404。因此，**不要宣称在线刷机页已经上线**，直到确认成功的 Pages deployment，并实际验证站点首页、清单、固件文件和配置页均可访问。
2. 开发分支 `codex/**` 的构建只运行测试、编译并上传 Actions Artifact；它不会发布 GitHub Pages。这是有意的。Pages 发布只应发生在 `main` 或 `v*` 标签上。
3. Pages 部署必须依赖成功的固件构建。工作流条件应保持为“`needs.build-firmware.result == 'success'` 且 ref 是 main 或版本标签”，不要恢复成无条件 `always()`，否则构建失败时也可能把缺少新固件的站点部署出去。
4. 上述 Pages 条件有对应的合约测试 `tests/pages_contract.test.mjs`。修改 workflow 条件时，同步更新测试，使测试约束正确的安全行为，而不是为了让测试通过而恢复有问题的逻辑。
5. 最近的 Pages 条件变更曾让 `npm test` 中旧断言失败：旧测试硬性要求 `always()`。这个失败属于测试预期落后于工作流变更，不是 ESP-IDF 编译错误。后续提交已经调整该断言；开始后续工作时要确认这之后的最新 CI 是否已通过。

### Pages 发布流程

- `docs/index.html`：在线刷机入口，使用 ESP Web Tools 的 `manifest.json`。
- `docs/manifest.json`：指向同目录的 `firmware-merged.bin`，烧录偏移为 `0`。
- Pages workflow 构建完成后，把刚生成的 `firmware` Artifact 下载到 `docs/`，并将 `main/index.html` 复制为 `docs/config.html`，将 `main/webhid_transport.js` 复制进 `docs/`。
- 最后部署整个 `docs/` 目录。

部署后请逐项实际核对：
- <https://wulinlee.github.io/stadia-dongle-esp-2/>
- <https://wulinlee.github.io/stadia-dongle-esp-2/manifest.json>
- <https://wulinlee.github.io/stadia-dongle-esp-2/firmware-merged.bin>
- <https://wulinlee.github.io/stadia-dongle-esp-2/config.html>

这些是预期 URL，不代表当前已经可用。若 Actions 的 deployment 成功但站点仍 404，核对仓库 Settings → Pages / Pages 域名配置和部署环境，而不要反复修改固件 manifest 来掩盖 Pages 未启用的问题。

## 功能与关键架构

### 固件与数据流

- Stadia 手柄通过 BLE 连接到 ESP32-S3。
- 手柄输入由固件解析并转换为 Xbox 兼容 USB 报告，通过原生 USB/OTG 接口给主机。
- 主机发来的 XInput 震动输出报告进入固件队列，再作为 Stadia 的 BLE 输出报告发送给对应手柄。
- Assistant/Capture 各配置一个动作；映射在物理按下时保持输出、松开时释放；XInput 虚拟按钮不覆盖真实输入。
- USB 配置页通过 WebHID 自定义协议访问设备，查询状态、管理配对和映射配置。映射持久化在 NVS 中。

### 主要文件索引

| 文件 | 责任与修改时的注意事项 |
|---|---|
| `main/ble_central.c` | BLE 连接、手柄连接槽位、震动输出 GATT 写入与重试。震动写入失败/过载时重点从这里排查。 |
| `main/xbox_dev.c` | USB Xbox 兼容接口及主机输出报告；解析游戏发来的马达强度。 |
| `main/main.c` | 固件任务、队列及不同 USB/BLE 子系统的协作。 |
| `main/bridge.c` | Stadia 输入到 Xbox/XInput 报告的转换；保存基础报告并叠加映射生成的虚拟按钮。 |
| `main/button_actions.c` | Assistant/Capture 物理按下/松开状态、映射输出与 5 秒鼠标模式组合键。 |
| `main/config_store.c`, `main/config_store.h` | 动作 ID/名称、默认配置和 NVS 持久化；注意 schema 与旧配置兼容。 |
| `main/usb_config_protocol.c`, `main/usb_config_protocol.h` | 固件端 USB 配置命令、状态与协议定义。 |
| `main/webhid_transport.js` | 浏览器 WebHID 传输、命令号、动作名称与能力位。 |
| `main/index.html` | 中文 USB 配置界面；配置动作选项须与固件端一致。 |
| `docs/index.html`, `docs/manifest.json` | 独立在线刷机页和刷机清单。 |
| `tests/firmware_contract.test.mjs` | 固件与 JS 协议/动作名称的一致性检查。 |
| `tests/pages_contract.test.mjs` | Pages 文件准备、链接和部署条件检查。 |
| `docs/hardware-test-checklist.md` | 实物 USB/WebHID/BLE/按键/震动回归清单。 |
| `.github/workflows/release.yml` | 云端自动测试、编译、Artifact、Release 和 Pages 部署。 |

## 必须保持的一致性与边界

### 动作 ID、按下/松开映射与 Web UI 是一份跨语言协议

新增或修改 Assistant/Capture 动作时，至少核对这些位置。配置页每个物理按键只使用一个动作。USB 配置协议仍保持 v1 的 9 字节 payload：偏移 0 是 Assistant 动作，偏移 2 是 Capture 动作；偏移 1、3、4–5 是旧版长按保留字段，新固件忽略它们，浏览器写入兼容值。动作枚举顺序不能重排。

1. 固件动作枚举与动作 ID：`main/config_store.h`。
2. 固件 ID ↔ 名称转换及默认值：`main/config_store.c`。
3. 浏览器动作名数组及 USB 协议：`main/webhid_transport.js`。
4. 下拉选项和中文标签：`main/index.html`。
5. 自动化合约测试：`tests/firmware_contract.test.mjs`。

已有动作枚举值可能已经写进 NVS。**不要随意给旧动作重新编号或重排枚举**；新增动作应以向后兼容方式追加。当前源码的 `CFG_SCHEMA` 为 7；NVS blob 原有结构布局保持不变，升级时保留旧短按动作作为新映射，并清除旧长按动作。若配置结构变化，必须重新审查 schema/migration。

### 虚拟 XInput 按键

新增的 XInput 动作是给 Assistant/Capture 额外按键用的，不等于全部 Stadia 原生按键均可自由重映射。普通按钮通过现有基础 Xbox 报告叠加虚拟按键状态，并在物理按住期间保持，松开时释放。不要用虚拟报告覆盖真实摇杆、物理按键或扳机状态。释放、USB reset 和断连路径必须清除虚拟按键，防止卡键。

当前行为边界：
- 键盘/媒体键与 XInput 虚拟按钮保持到物理按键松开，不再发送固定 20 ms 脉冲。
- 映射到 LT/RT 会发送全按强度，不是可配置的模拟压力。
- Stadia 手柄没有 Xbox Series 手柄那样独立的扳机震动马达。

### 震动

- Xbox 的 8 位强度使用 `n × 257` 扩展为 Stadia 所需 16 位强度（覆盖 0–65535）。
- 当前代码采用串行 BLE 输出写入、失败退避重试，以及优先发送每个手柄最新的震动目标，以避免旧更新长期排队或停止震动的零值被挤掉。
- 保留每个连接手柄的独立状态；项目架构最多处理两个手柄。
- 不要声称高负载震动问题已被彻底解决，直到真机和实际游戏回归完成。需要特别观察停止震动的零值、快速强弱变化、双手柄并发和 BLE 写入失败日志。

## 开源参考：优先适配，不要重新造轮子

- [danzig666/stadia-dongle-esp](https://github.com/danzig666/stadia-dongle-esp)：本仓库的基础 fork；保持现有 ESP32-S3 / BLE / USB 架构。
- [Scalee/stadia-dongle 的震动可靠性修复](https://github.com/Scalee/stadia-dongle/commit/d92ac6d3e2b7bd07d716940a5e160aa1a215d655)：参考失败重试、写入数据快照、合并过期震动目标、跳过重复写入等思路。不要假设其代码可不经适配直接替换当前多手柄结构。
- [Joypad OS Stadia BLE driver](https://github.com/joypad-ai/joypad-os/blob/main/src/bt/bthid/devices/vendors/google/stadia_bt.c)：参考 Stadia BLE 报告和 16 位马达强度换算。不要假设 Joypad OS 的预编译固件可直接用于所有 ESP32-S3 板型。
- [ESP-IDF](https://github.com/espressif/esp-idf)：当前 CI 使用 ESP-IDF 6.0。
- [espressif/esp-idf-ci-action](https://github.com/espressif/esp-idf-ci-action)：在 GitHub Actions 中构建。
- [ESP Web Tools](https://github.com/esphome/esp-web-tools)：浏览器在线刷机按钮及 manifest 工作流。

## 与上游固件的关键差异及待验证项

比较对象是 [danzig666/stadia-dongle-esp 的 main](https://github.com/danzig666/stadia-dongle-esp)，不是 Scalee 上游仓库中的任意历史提交。在线刷入页面沿用相同的 ESP Web Tools + manifest 机制，manifest 把完整 `firmware-merged.bin` 以偏移 `0x0` 写入；但两个仓库的固件由各自源码构建，二进制不应视为同一文件。

已确认的功能差异：
- 本仓库改进了 BLE 震动输出路径：串行写入、失败退避重试、保护在途 payload，并优先处理最新目标；震动 8 位强度按 `n * 257` 扩展为 16 位。效果仍要真机验证。
- 本仓库将 Assistant/Capture 改为每键单动作的按下/松开映射；NVS schema 7 迁移会清除旧长按动作。
- 本仓库主要面向中文用户，补充 USB WebHID 协议合约测试与维护交接文档。
- 原有双手柄、鼠标模式、Wi-Fi Web UI、USB 配置等能力不少是继承基础工程，不要把它们全都描述成本仓库新功能。

### 必须验证的打包差异

已对照两边当前 workflow 与 `partitions.csv`：
- 两边分区表都有 `otadata` 分区 `0xf000`，应用偏移是 `0x20000`。
- 上游在 `firmware-merged.bin` 中包含 `build/ota_data_initial.bin`，还会生成 `build/stadia-dongle-ota.bin`（从 `build/stadia-dongle.bin` 复制），并在 Artifact / Release 中一并发布。
- 本仓库当前 workflow 合并完整镜像时没有包含 `0xf000 build/ota_data_initial.bin`，也没有产出 / 发布 `stadia-dongle-ota.bin`。
- 本仓库 `main/web_server.c` OTA handler 调用 `esp_ota_get_next_update_partition()`、`esp_ota_begin()` 和 `esp_ota_write()`，所以应该单独验证网页 OTA 所需文件到底如何提供；不能把从偏移 0 开始的整合镜像当作应用 OTA 文件上传。

下一个 AI 的检查顺序：
1. 阅读 `partitions.csv`、`.github/workflows/release.yml`、`main/web_server.c` OTA handler 和设备端网页中 OTA 上传代码。
2. 确认 `ota_data_initial.bin` 的合入方式，并确认是否应像上游一样生成并发布 app-only `stadia-dongle-ota.bin`。
3. 给打包产物补合约检查，避免未来 workflow 不小心漏掉必需文件。
4. 先让 Actions 测试 / 构建通过，然后做冷启动、正常 USB 刷写、网页 OTA 更新以及 OTA 失败恢复的实机检查。
5. 检查当前 workflow 的浅克隆是否影响固件版本字符串；上游 workflow 用 `fetch-depth: 0` 取得完整 Git 历史。

以上是经源码比对确认的差异和待验证项目，不是已验证的故障结论。不要只根据 Pages deployment 成功就宣称固件 OTA / 双分区升级路径正常。

## 验证方法

### 自动测试

在本地安装 Node.js 后运行：

```sh
npm test
```

它使用 Node 内置 test runner（`node --test tests/*.test.mjs`），检查 USB 配置协议命令值、能力位、动作名称、HID 报告描述符、页面约定、Wi-Fi 默认行为和 Pages 发布逻辑。它们是合约/源码测试，并不模拟真实 BLE 无线链路，也不能取代板上验证。

### 云端固件构建

标准路径是 GitHub Actions，而不是要求用户安装本地 ESP-IDF：

1. 打开 [Actions](https://github.com/WuLinLee/stadia-dongle-esp-2/actions/workflows/release.yml)，确认运行对应正确分支和 head SHA。
2. 检查 `Test USB configuration protocol`。
3. 检查 `Build with ESP-IDF`。
4. 成功后检查 `firmware` Artifact；里面应有完整镜像 `firmware-merged.bin`，通常保留 7 天。

CI 使用 ESP-IDF 6.0、目标 `esp32s3`，执行 `idf.py build` 并用 esptool 将 bootloader、分区表和应用镜像合并。合并参数当前针对 4 MB flash：bootloader 偏移 `0x0`、partition table `0x8000`、app `0x20000`。更改分区表或应用偏移时，必须同步检查此步骤。

### 真机验收（不能省略）

参照 [真机验收清单](docs/hardware-test-checklist.md)。震动变更至少覆盖：
- XInput 测试程序可触发双马达震动。
- 强度快速变化后，停止报告能及时让两个马达停止，不出现震动拖尾或卡住。
- 《Silksong》冲刺后停止操作，震动能及时停止。
- 《A Hat in Time》帽子轮盘连续旋转时强弱变化和节奏可以辨认。
- 两个手柄连接时没有跨手柄发送错误。
- Assistant/Capture 每键只配置一个动作，输出在物理按住时保持、松开时释放；虚拟按钮不会卡住或覆盖真实输入。
- 保存配置后断电重启，NVS 中映射仍可用。
- USB 配置页断开/重连、配对、忘记手柄、Wi-Fi 手动开关和重启路径都符合 checklist。

将“未执行”的真机测试写成未验证项，禁止把源码检查或云端编译等同于真机通过。

## 建议下一个 AI 按此顺序继续

1. **先看实时状态：**打开 [PR #1](https://github.com/WuLinLee/stadia-dongle-esp-2/pull/1) 和 [最新 Actions](https://github.com/WuLinLee/stadia-dongle-esp-2/actions/workflows/release.yml)，不要相信旧运行或本文件里的快照。
2. **先让当前分支 CI 通过：**如失败，打开 job 日志定位首个失败断言/编译错误；先判断是实现 bug、测试过期还是环境问题。
3. **审查代码而非只看测试：**重点审阅 BLE 震动的重试生命周期、回调/缓冲区所有权、多手柄槽位索引，以及虚拟按钮叠加和释放逻辑。
4. **未经过用户确认，不要擅自合并 PR 或创建正式版本标签。**
5. 合并到 `main` 后，等 Pages deployment 成功，再访问上文的四个 URL 并确认 `manifest.json` 指向可下载的最新 `firmware-merged.bin`。如仍 404，检查 Pages 设置与部署记录。
6. 然后按硬件 checklist 做实物回归，并把结果（设备型号、日志、测试游戏和实际结果）更新到文档或 PR 描述中。

## 用户期望与交付标准

- 默认用中文说明开发状态和操作步骤。
- 尽量采用可验证的开源参考，不为同样功能重复造轮子。
- 保持 README 详细，并记下实现取舍、来源链接、测试方式和已知限制。
- 不要求普通用户在本地配置 ESP-IDF；优先提供 Actions 云端构建产物和可用的在线安装器。
- 每次交付都要区分代码、CI、部署、真机验证四种状态，并给出可以点击核查的链接。
