# Safe-MP4-Recorder (嵌入式 Linux MP4 安全录像与热插拔容灾方案)

[English](#english-version) | [中文说明](#chinese-version)

---

<a name="chinese-version"></a>
## 🇨🇳 中文说明

工业级嵌入式 Linux MP4 安全录像与存储管理方案。专为车载监控 (MDVR)、行车记录仪、低功耗 IPC、便携式执法仪打造。

### 痛点与挑战
- **断电/异常拔卡变 0KB：** 传统录像方案仅在文件完全关闭时写入 MOOV Box，遭遇急刹断电或拔卡时整段录像报废。
- **文件系统损坏变只读：** 频繁循环录像导致严重文件碎片，热拔瞬间引发 FAT 表元数据损坏，TF 卡锁死为只读模式甚至报废。
- **线程死锁与卡死：** 视频写入线程与热插拔卸载事件冲突，导致系统阻塞甚至内核崩死。

### 核心设计机制
1. **MOOV 原子周期性重写：** 支持按时间/帧率增量同步与原子更新 MOOV Box，拔卡瞬间录像立即可播，0 坏文件。
2. **固定文件空间预分配：** 避免动态追加写入造成的碎片化擦写，保护存储介质 FAT/ext4 分区元数据。
3. **状态机安全感知：** 结合热插拔事件感知，平滑切换写缓存与安全刷盘。

### 开源与闭源范围说明
| 模块 / 内容 | 公开状态 | 交付形式 / 说明 |
| :--- | :--- | :--- |
| `api/` (公共接口头文件) | **公开** | 包含完整 API 定义与参数注释 |
| `demo/` (调用示例工程) | **公开** | 提供极简调用逻辑与测试 Demo |
| 本 `README.md` 与技术说明 | **公开** | 架构设计、测试指标与集成指南 |
| `src/` (核心算法与底层实现) | **商业闭源** | 内部状态机、MOOV 原子刷盘与存储管理核心实现 |
| 商业交付包 (Commercial SDK) | **商业授权** | 针对目标芯片平台的预编译静态库 (`.a`) 或完整商用源码 |

> **说明：** 核心实现源码未公开发布。商用落地需取得商业授权，提供对应芯片平台的二进制库或源码授权协议。

### 适配芯片平台
- **瑞芯微 (Rockchip):** RV1126 / RV1109 / RK3568 / RK3588 等
- **星宸 (SigmaStar):** SSC 系列 IPC 方案
- **君正 (Ingenic):** T31 / T40 系列
- **海思 (HiSilicon) 及其他通用嵌入式 Linux 平台**

### 商业授权与技术支持
- 提供商业授权许可（二进制静态库 `.a` / 完整商用 C 源码交付）
- 提供目标芯片平台的交叉编译、SDK 联调与热插拔容灾集成支持
- 承接断电丢帧、0KB 坏文件、TF 卡热插拔损毁等疑难杂症紧急联调
- **微信联系：** `[填写你的微信号]`（添加请备注：方案咨询 / 商业合作）
- **邮箱联系：** `[填写你的邮箱]`

---

<a name="english-version"></a>
## 🇬🇧 English Version

An industrial-grade embedded Linux MP4 recording and storage management solution. Designed specifically for Mobile DVR (MDVR), Dash Cams, low-power IPCs, and Body Worn Cameras (BWC).

### Problems & Challenges
- **0KB File Corruption on Power Loss/Removal:** Conventional MP4 recorders only write the MOOV atom upon normal file close. Sudden power loss or SD card removal leaves the entire video unplayable (0KB file).
- **Filesystem Corruption (Read-Only Mode):** Continuous circular recording causes severe FAT filesystem fragmentation. Sudden card ejection corrupts FAT metadata, permanently locking the card into a read-only state.
- **Thread Deadlocks:** Race conditions between high-throughput video write threads and hot-unmount kernel events often hang the system.

### Core Mechanisms
1. **Periodic MOOV Atom Atomic Updates:** Incrementally flushes and synchronizes MOOV data at configurable intervals/frame counts. Any recorded segment is instantly playable upon card removal.
2. **Fixed-Size Pre-allocation:** Pre-allocates continuous block storage to eliminate runtime metadata overhead and protect the partition table from frequent rewrites.
3. **Safe State-Machine Handling:** Coordinates with uevent/hot-plug notifications for smooth buffer switching and atomic sync.

### What is Open / Closed
| Component / Directory | Availability | Description |
| :--- | :--- | :--- |
| `api/` (Public Headers) | **Open** | Public C API definitions and function signatures |
| `demo/` (Reference Examples) | **Open** | Minimal integration demos and usage patterns |
| `README.md` & Documentation | **Open** | Architectural design, benchmarks, and integration guides |
| `src/` (Core Engine Implementation) | **Closed** | Atomic MOOV flush engine, pre-allocation & recovery routines |
| Commercial SDK Distribution | **Licensed** | Cross-compiled static libraries (`.a`) or full source code |

> **Notice:** The implementation source code is closed-source. Commercial deployment requires a license agreement, providing pre-compiled binaries or complete commercial source access.

### Supported SoC Platforms
- **Rockchip:** RV1126 / RV1109 / RK3568 / RK3588, etc.
- **SigmaStar:** SSC series IPC SoCs
- **Ingenic:** T31 / T40 series
- **HiSilicon & Generic Embedded Linux Platforms**

### Commercial Licensing & Support
- Commercial licenses available (pre-compiled `.a` libraries or full ANSI C source code)
- Cross-compilation support and on-target integration for major embedded platforms
- Emergency consultancy for video corruption, 0KB files, and SD card failure troubleshooting
- **WeChat:** `[Your WeChat ID]` (Please note: Solution Consultation / Commercial Licensing)
- **Email:** `[Your Email Address]`
