# libmov

ISO BMFF / MP4 **reserved-moov** recorder for embedded devices.

预分配固定长度文件、尾部预留 `moov`、录像中途可反复 `finalize()`，掉电后仍可被标准播放器打开。

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

## Commercial licensing / 商业授权

If you want to ship libmov in a product (IPC, DVR/NVR, firmware, cloud recording, etc.), request a commercial license.

若要将 libmov 用于产品（IPC、DVR/NVR、固件、云录像等），请申请商业授权。

**Contact / 联系：**

- Email: [yc731107935@gmail.com](mailto:yc731107935@gmail.com)
- Subject: `libmov commercial license`

Please include: company name, product type, expected volume, target CPU (e.g. MIPS / ARM), and whether you need source or binaries only.

请注明：公司名称、产品类型、预估出货量、目标 CPU（如 MIPS / ARM）、需要源码还是仅二进制。

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
```

Only `#include "mov.h"` is required. Link with `-lmov`.

---

## API sketch / 接口概要

```c
#include "mov.h"

mov_file_preallocate("rec.mp4", MOV_FILE_SIZE_64MIB);
fp = fopen("rec.mp4", "rb+");
c  = mov_container_create(mov_file_buffer(), fp, MOV_FLAG_RESERVED_MOOV, &media);
v  = mov_container_get_video_track(c);
a  = mov_container_get_audio_track(c);
mov_container_set_extra(c, v, avcc, avcc_len);
mov_container_write(c, v, nalu, n, pts_ms, dts_ms, MOV_AV_FLAG_KEYFRAME);
mov_container_finalize(c);   /* repeat during recording */
mov_container_close(c);
```

Accepted file sizes: **64 / 128 / 256 MiB**.  
Video: AVCC (length-prefixed NALUs). Audio: raw AAC (no ADTS).

详见 `api/mov.h` 中的完整注释与推荐调用顺序。

---

## Repository layout / 目录

```
api/mov.h                 # public header only
demo/reserved_moov_demo.c # sample
LICENSE                   # evaluation + commercial terms
README.md
Makefile                  # builds demo against prebuilt lib/, or full lib if src/ present locally
lib/                      # place licensed / evaluation .so here (not always in git)
```

`src/` exists only in private trees used by the copyright holder.

---

## Disclaimer / 声明

Third-party open-source components, if any, retain their original licenses and copyright notices inside the private source tree and in binary redistributions as required.

如实现中包含第三方开源组件，其原有许可证与版权声明在私有源码树及按要求再分发的二进制中予以保留。
