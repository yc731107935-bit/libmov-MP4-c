#ifndef MOV_H
#define MOV_H

/*****************************************************************************
 * @file    mov.h
 * @brief   libmov 唯一对外头文件，预分配录像方案
 *          The only public header of libmov, reserved-moov recorder
 *
 * @details
 *          文件必须先按固定长度预分配。媒体写入前部 mdat，索引盒 moov
 *          落在文件尾预留区。录像中途可多次调用 finalize()，把已写入
 *          样本刷进 moov。掉电后，最后一次成功 finalize 之前的内容
 *          仍可被播放器打开。
 *          Pre-allocate a fixed-size file first. Samples go into mdat;
 *          moov lives in a reserved tail. Call finalize() during recording
 *          to rewrite that moov. After a crash, media written before the
 *          last successful finalize stays playable.
 *
 *          布局（低地址 → 高地址）：
 *            [ftyp] [free 8B] [mdat …… 向后增长] [moov 预留区] [free 填充]
 *          Layout (low → high):
 *            [ftyp] [free 8B] [mdat …… grows forward] [reserved moov] [free pad]
 *
 *          仅接受三种文件长度：64 / 128 / 256 MiB。
 *          Accepted sizes: 64 / 128 / 256 MiB only.
 *
 *          视频样本必须是 AVCC（4 字节大端长度 + NALU），禁止 Annex-B。
 *          音频样本必须是裸 AAC，禁止 ADTS。
 *          Video samples must be AVCC (4-byte big-endian length + NALU).
 *          Audio samples must be raw AAC, not ADTS.
 *
 *          集成：#include "mov.h"，链接 -lmov。
 *          Integration: #include "mov.h", link -lmov.
 *
 * @code
 *          MovMediaInfo media;
 *          FILE *fp;
 *          mov_container_t *c;
 *          int v, a;
 *
 *          memset(&media, 0, sizeof(media));
 *          media.video_object = MOV_OBJECT_H264;
 *          media.video_width  = 1280;
 *          media.video_height = 720;
 *          media.audio_object = MOV_OBJECT_AAC;
 *          media.audio_channel_count   = 1;
 *          media.audio_bits_per_sample = 16;
 *          media.audio_sample_rate     = 16000;
 *
 *          if (mov_file_preallocate("rec.mp4", MOV_FILE_SIZE_64MIB) != 0)
 *              return -1;
 *          fp = fopen("rec.mp4", "rb+");
 *          c = mov_container_create(mov_file_buffer(), fp,
 *                                   MOV_FLAG_RESERVED_MOOV, &media);
 *          v = mov_container_get_video_track(c);
 *          a = mov_container_get_audio_track(c);
 *          mov_container_set_extra(c, v, avcc, avcc_bytes);
 *          mov_container_set_extra(c, a, asc,  asc_bytes);
 *          if (mov_container_remain_size(c, nalu_bytes) == 0)
 *              break;
 *          mov_container_write(c, v, nalu, nalu_bytes, pts_ms, dts_ms,
 *                              is_idr ? MOV_AV_FLAG_KEYFRAME : 0);
 *          mov_container_write(c, a, aac, aac_bytes, pts_ms, dts_ms, 0);
 *          mov_container_finalize(c);
 *          mov_container_close(c);
 *          fclose(fp);
 * @endcode
 *****************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * @brief   预分配尺寸，64 MiB，索引约 62500 帧
 *          Pre-allocated size, 64 MiB, about 62500 sample slots
 *****************************************************************************/
#define MOV_FILE_SIZE_64MIB     0x04000000u

/*****************************************************************************
 * @brief   预分配尺寸，128 MiB，索引约 125000 帧
 *          Pre-allocated size, 128 MiB, about 125000 sample slots
 *****************************************************************************/
#define MOV_FILE_SIZE_128MIB    0x08000000u

/*****************************************************************************
 * @brief   预分配尺寸，256 MiB，索引约 250000 帧
 *          Pre-allocated size, 256 MiB, about 250000 sample slots
 *****************************************************************************/
#define MOV_FILE_SIZE_256MIB    0x10000000u

/*****************************************************************************
 * @brief   H.264 / AVC 对象类型
 *          H.264 / AVC object type
 *****************************************************************************/
#define MOV_OBJECT_H264         0x21
#define MOV_OBJECT_AVC          MOV_OBJECT_H264

/*****************************************************************************
 * @brief   AAC 对象类型
 *          AAC object type
 *****************************************************************************/
#define MOV_OBJECT_AAC          0x40

/*****************************************************************************
 * @brief   启用预留-moov 方案，create() 必须带上此标志
 *          Enable the reserved-moov profile, required by create()
 *****************************************************************************/
#define MOV_FLAG_RESERVED_MOOV  0x00000004u

/*****************************************************************************
 * @brief   视频关键帧标志，音频写 0
 *          Video key / IDR flag, use 0 for audio
 *****************************************************************************/
#define MOV_AV_FLAG_KEYFRAME    0x0001u
#define MOV_AV_FLAG_KEYFREAME   MOV_AV_FLAG_KEYFRAME

/*****************************************************************************
 * @brief   随机访问存储回调，库不打开文件、不 mmap
 *          Random-access store callbacks, the library does not open or mmap
 *
 * @details
 *          read / write 必须一次完成 bytes 字节，否则返回负值。
 *          read / write must transfer exactly bytes, or return negative.
 *          seek(offset >= 0) 从文件头定位，seek(offset < 0) 从文件尾定位。
 *          create() 用 seek(-1) 加 tell() 取得文件长度。
 *          seek(offset >= 0) from start, seek(offset < 0) from end.
 *          create() uses seek(-1) plus tell() to learn the file length.
 *          tell() 返回当前偏移，失败为负。
 *          tell() returns the current offset, or a negative error.
 *          非 FILE 存储（例如 mmap）自行实现这四个函数，param 传上下文。
 *          For non-FILE stores such as mmap, implement these four functions
 *          and pass the context as param.
 *****************************************************************************/
struct mov_buffer_t
{
    /*************************************************************************
     * @brief   读取
     *          Read
     * @param   param  用户上下文
     *                 User context
     * @param   data   读出缓冲
     *                 Destination buffer
     * @param   bytes  请求字节数
     *                 Requested size
     * @return  0 成功，<0 失败
     *          0 on success, negative on error
     *************************************************************************/
    int (*read)(void *param, void *data, uint64_t bytes);

    /*************************************************************************
     * @brief   写入
     *          Write
     * @param   param  用户上下文
     *                 User context
     * @param   data   待写数据
     *                 Source buffer
     * @param   bytes  写入字节数
     *                 Size to write
     * @return  0 成功，<0 失败
     *          0 on success, negative on error
     *************************************************************************/
    int (*write)(void *param, const void *data, uint64_t bytes);

    /*************************************************************************
     * @brief   定位
     *          Seek
     * @param   param   用户上下文
     *                  User context
     * @param   offset  >=0 绝对偏移，<0 从文件尾起算
     *                  >=0 absolute, <0 from end
     * @return  0 成功，<0 失败
     *          0 on success, negative on error
     *************************************************************************/
    int (*seek)(void *param, int64_t offset);

    /*************************************************************************
     * @brief   当前偏移
     *          Current offset
     * @param   param  用户上下文
     *                 User context
     * @return  >=0 当前偏移，<0 失败
     *          Current offset, or negative on error
     *************************************************************************/
    int64_t (*tell)(void *param);
};

/*****************************************************************************
 * @brief   库内置 FILE 适配器，create() 的 param 传 FILE *
 *          Built-in FILE adapter, pass FILE * as create() param
 *
 * @details
 *          FILE 必须已按合法尺寸预分配，并以 "rb+" 打开。
 *          The FILE must already be pre-allocated and opened with "rb+".
 *
 * @return  I/O 回调表
 *          I/O callback table
 *****************************************************************************/
const struct mov_buffer_t *mov_file_buffer(void);

/*****************************************************************************
 * @brief   判断尺寸是否为本方案接受的预分配长度
 *          Check whether size is one of the three accepted lengths
 *
 * @param   size  文件字节数
 *                File size in bytes
 * @return  1 合法，0 非法
 *          1 if accepted, 0 if rejected
 *****************************************************************************/
int mov_file_size_ok(uint64_t size);

/*****************************************************************************
 * @brief   在路径上预分配固定长度文件，已存在则覆盖
 *          Pre-allocate a fixed-size file at path, overwrite if present
 *
 * @param   path  输出路径
 *                Output path
 * @param   size  必须是 64 / 128 / 256 MiB 之一
 *                Must be one of 64 / 128 / 256 MiB
 * @return  0 成功，-1 失败
 *          0 on success, -1 on error
 *****************************************************************************/
int mov_file_preallocate(const char *path, uint64_t size);

/*****************************************************************************
 * @brief   音视频参数，create() 按此各建一条轨
 *          Audio and video parameters, create() adds one track of each
 *
 * @details
 *          解码器配置不在这里填，create() 之后用 set_extra() 写入对应轨。
 *          Decoder config is not set here. After create(), call set_extra().
 *****************************************************************************/
typedef struct media_info_t MovMediaInfo;
struct media_info_t
{
    /*************************************************************************
     * @brief   视频对象类型，填 MOV_OBJECT_H264
     *          Video object type, set MOV_OBJECT_H264
     *************************************************************************/
    int video_object;

    /*************************************************************************
     * @brief   视频宽度，像素
     *          Video width in pixels
     *************************************************************************/
    int video_width;

    /*************************************************************************
     * @brief   视频高度，像素
     *          Video height in pixels
     *************************************************************************/
    int video_height;

    /*************************************************************************
     * @brief   音频对象类型，填 MOV_OBJECT_AAC
     *          Audio object type, set MOV_OBJECT_AAC
     *************************************************************************/
    int audio_object;

    /*************************************************************************
     * @brief   声道数
     *          Channel count
     *************************************************************************/
    int audio_channel_count;

    /*************************************************************************
     * @brief   位深
     *          Bits per sample
     *************************************************************************/
    int audio_bits_per_sample;

    /*************************************************************************
     * @brief   采样率，赫兹
     *          Sample rate in Hz
     *************************************************************************/
    int audio_sample_rate;
};

typedef struct mov_container_t mov_container_t;

/*****************************************************************************
 * @brief   在已预分配的存储上创建容器
 *          Create a container on an already pre-allocated store
 *
 * @details
 *          调用前长度必须是 64 / 128 / 256 MiB，可用 mov_file_size_ok 检查。
 *          flags 必须含 MOV_FLAG_RESERVED_MOOV。
 *          buffer、param、media_info 均不可为空。
 *          成功后写入 ftyp / mdat 头，建轨，并在尾部写一份空 moov。
 *          Size must be 64 / 128 / 256 MiB before calling.
 *          flags must include MOV_FLAG_RESERVED_MOOV.
 *          buffer, param and media_info must be non-NULL.
 *          On success writes ftyp / mdat headers, creates tracks, and writes
 *          an empty moov in the reserved tail.
 *
 * @param   buffer      I/O 回调表
 *                      I/O callback table
 * @param   param       用户上下文，FILE 适配器时传 FILE *
 *                      User context, FILE * when using mov_file_buffer()
 * @param   flags       必须含 MOV_FLAG_RESERVED_MOOV
 *                      Must include MOV_FLAG_RESERVED_MOOV
 * @param   media_info  音视频参数
 *                      Audio and video parameters
 * @return  句柄，参数或尺寸非法时为 NULL
 *          Handle, or NULL if arguments or size are invalid
 *****************************************************************************/
mov_container_t *mov_container_create(const struct mov_buffer_t *buffer,
                                      void *param,
                                      int flags,
                                      MovMediaInfo *media_info);

/*****************************************************************************
 * @brief   释放容器，不写 moov，停录前请先 finalize()
 *          Destroy the handle, does not write moov, call finalize() first
 *
 * @param   container  容器句柄
 *                     Container handle
 *****************************************************************************/
void mov_container_close(mov_container_t *container);

/*****************************************************************************
 * @brief   给已有轨道写入 extra，进入 moov 的 avcC / esds
 *          Write extra data onto an existing track, avcC / esds in moov
 *
 * @details
 *          create() 建轨时 extra 为空。拿到 SPS/PPS 或 AudioSpecificConfig
 *          后调用本函数，下次 finalize() 会把它写进 moov。
 *          create() leaves extra empty. Call this after SPS/PPS or
 *          AudioSpecificConfig is ready. The next finalize() writes it.
 *
 * @param   container        容器句柄
 *                           Container handle
 * @param   track            get_video_track() 或 get_audio_track() 的返回值
 *                           Return value of get_video_track() or get_audio_track()
 * @param   extra_data       视频为 AVCDecoderConfigurationRecord，音频为 AudioSpecificConfig
 *                           Video: AVCDecoderConfigurationRecord, audio: AudioSpecificConfig
 * @param   extra_data_size  字节数
 *                           Size in bytes
 * @return  0 成功，-1 失败
 *          0 on success, -1 on error
 *****************************************************************************/
int mov_container_set_extra(mov_container_t *container,
                            int track,
                            const void *extra_data,
                            size_t extra_data_size);

/*****************************************************************************
 * @brief   向指定轨追加一帧，负载写到当前 mdat 指针
 *          Append one sample, payload is written at the current mdat cursor
 *
 * @details
 *          写入前用 remain_size(container, bytes) 确认 mdat 尚未顶到 moov。
 *          Check remain_size(container, bytes) before writing.
 *
 * @param   container  容器句柄
 *                     Container handle
 * @param   track      轨号
 *                     Track id
 * @param   data       AVCC 视频或裸 AAC
 *                     AVCC video or raw AAC
 * @param   bytes      负载长度
 *                     Payload size
 * @param   pts        显示时间戳，毫秒
 *                     Presentation time in milliseconds
 * @param   dts        解码时间戳，毫秒
 *                     Decode time in milliseconds
 * @param   flags      视频关键帧填 MOV_AV_FLAG_KEYFRAME，否则填 0
 *                     MOV_AV_FLAG_KEYFRAME on video key frames, otherwise 0
 * @return  0 成功，非 0 失败
 *          0 on success, non-zero on error
 *****************************************************************************/
int mov_container_write(mov_container_t *container,
                        int track,
                        const void *data,
                        size_t bytes,
                        int64_t pts,
                        int64_t dts,
                        int flags);

/*****************************************************************************
 * @brief   把当前已写入样本刷进尾部 moov，可中途反复调用
 *          Rewrite the reserved moov with samples written so far, repeatable
 *
 * @details
 *          成功后文件即可播放，之后仍可继续 write()。
 *          After success the file is playable, write() may continue.
 *
 * @param   container  容器句柄
 *                     Container handle
 * @return  0 成功，非 0 失败
 *          0 on success, non-zero on error
 *****************************************************************************/
int mov_container_finalize(mov_container_t *container);

/*****************************************************************************
 * @brief   再写入 add_size 字节后，mdat 距 moov 还剩多少
 *          Bytes left in mdat after writing add_size more bytes
 *
 * @param   container  容器句柄
 *                     Container handle
 * @param   add_size   即将写入的样本长度，只查询时填 0
 *                     Upcoming sample size, pass 0 to query only
 * @return  剩余字节，已满或参数无效时为 0
 *          Remaining bytes, 0 if full or arguments are invalid
 *****************************************************************************/
uint64_t mov_container_remain_size(mov_container_t *container,
                                   uint32_t add_size);

/*****************************************************************************
 * @brief   查询 create() 建立的音频轨号
 *          Audio track id created by create()
 *
 * @param   container  容器句柄
 *                     Container handle
 * @return  音频轨号
 *          Audio track id
 *****************************************************************************/
int mov_container_get_audio_track(mov_container_t *container);

/*****************************************************************************
 * @brief   查询 create() 建立的视频轨号
 *          Video track id created by create()
 *
 * @param   container  容器句柄
 *                     Container handle
 * @return  视频轨号
 *          Video track id
 *****************************************************************************/
int mov_container_get_video_track(mov_container_t *container);

/*****************************************************************************
 * @brief   已写入视频时长，单位整秒
 *          Elapsed video duration in whole seconds
 *
 * @param   container  容器句柄
 *                     Container handle
 * @return  秒数
 *          Duration in seconds
 *****************************************************************************/
int mov_container_get_video_pts(mov_container_t *container);

#ifdef __cplusplus
}
#endif

#endif /* MOV_H */
