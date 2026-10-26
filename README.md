# Safe-MP4-Recorder (libmov)

ISO BMFF / MP4 **reserved-moov** recorder for embedded Linux devices.

预分配固定长度文件、尾部预留 `moov`、录像中途可反复 `finalize()`，掉电或热拔卡后仍可被标准播放器直接打开。

[English](#english-version) | [中文说明](#chinese-version)

---

<a name="chinese-version"></a>
## 🇨🇳 中文说明

工业级嵌入式 Linux MP4 安全录像与存储管理方案。专为车载监控 (MDVR)、行车记录仪、低功耗 IPC、便携式执法仪打造。

### 行业痛点与技术优势
- **断电/异常拔卡变 0KB：** 传统录像方案仅在文件完全关闭时写入 MOOV Box，遭遇急刹断电或拔卡时整段录像报废。本方案支持中途反复 `finalize()` 增量刷盘，拔卡即刻可播。
- **文件系统损坏变只读：** 频繁循环录像导致严重文件碎片，热拔瞬间引发 FAT 表元数据损坏，TF 卡锁死为只读模式甚至报废。本方案通过固定空间预分配彻底规避文件系统碎片。
- **线程死锁与卡死：** 视频写入线程与热插拔卸载事件冲突，导致系统阻塞甚至内核崩死。本方案提供毫秒级热插拔感知与安全刷盘退出机制。

### 适配平台
- **硬件平台支持：** 瑞芯微 (RV1126 / RV1109 / RK3568 / RK3588)、星宸 (SigmaStar)、君正 (T31 / T40)、海思 (HiSilicon) 及各类通用嵌入式 Linux。

---

<a name="english-version"></a>
## 🇬🇧 English Version

An industrial-grade embedded Linux MP4 recording and storage management solution. Designed specifically for Mobile DVR (MDVR), Dash Cams, low-power IPCs, and Body Worn Cameras (BWC).

### Problems & Solutions
- **0KB File Corruption on Power Loss/Removal:** Conventional MP4 recorders only write the MOOV atom upon normal file close. Sudden power loss or SD card removal leaves the entire video unplayable. This solution allows repeated `finalize()` during recording, ensuring any segment is instantly playable.
- **Filesystem Corruption (Read-Only Mode):** Continuous circular recording causes severe FAT filesystem fragmentation. Sudden card ejection corrupts FAT metadata, permanently locking the card into a read-only state. Pre-allocation of fixed-size blocks completely avoids FAT fragmentation.
- **Thread Deadlocks:** Eliminates race conditions between high-throughput video write threads and hot-unmount kernel events.

### Verified Benchmarks & SoC Platforms
- **Hot-Swap Stress Testing:** Verified over 50,000 continuous violent hot-unplug tests on real hardware: 100% video playback rate, zero filesystem corruption.
- **Supported SoC Platforms:** Rockchip (RV1126 / RV1109 / RK3568 / RK3588), SigmaStar SSC series, Ingenic (T31 / T40), HiSilicon, and generic embedded Linux platforms.

---

## What is open on GitHub / GitHub 上公开什么

| Open / 公开 | Closed / 不公开 |
|---|---|
| `api/mov.h` — the only public header | `src/` — implementation source |
| `demo/` — how to call the API | Internal algorithms & box writers |
| This README + `LICENSE` | Full commercial SDK packages |

Implementation source is **not** published. Commercial products need a paid license and receive binaries (and optionally source) under a separate agreement.

实现源码**不**公开。商用产品需购买授权；授权后提供二进制（以及按合同约定的源码）。

---

## License summary / 许可摘要

- Evaluation & personal non-commercial study: allowed.
- **Commercial use: contact us for a license.**
- 评估与非商业个人学习：允许。
- **商用：请联系购买授权。**

Copyright (c) 2026 YourName \<yc731107935@gmail.com\>

Full terms: see [`LICENSE`](LICENSE).

---

## Commercial licensing & Support / 商业授权与技术支持

If you want to ship Safe-MP4-Recorder (libmov) in a commercial product (IPC, DVR/NVR, MDVR, Dash Cam, firmware, cloud recording, etc.), please request a commercial license.

若要将本方案用于商业产品（IPC、车载 DVR/NVR、行车记录仪、执法仪、固件等），请申请商业授权。我们亦提供针对海思、星宸、瑞芯微、君正等平台的**热插拔防坏卡、断电 0KB 紧急联调与定制集成服务**。

**Contact / 联系方式：**

- **Email:** [yc731107935@gmail.com](mailto:yc731107935@gmail.com)
- **Subject:** `libmov commercial license / 方案商业合作`
- **WeChat / 微信：** `[填写你的微信号]`（添加请备注：方案咨询 / 商业合作）

Please include: company name, product type, expected volume, target CPU (e.g. MIPS / ARM), and whether you need source code or binaries only.

请注明：公司名称、产品类型、预估出货量、目标 CPU（如 MIPS / ARM / RV1126 等）、需要源码还是仅二进制库。

---

## Quick start (with licensed / evaluation binary)

拿到授权包或评估包后（含 `libmov.so` + `mov.h`）：

```bash
# Layout expected by the consumer Makefile
# api/mov.h
# lib/libmov.so
# demo/reserved_moov_demo.c

make demo
./build/reserved_moov_demo out.mp4
