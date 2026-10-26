/*****************************************************************************
 * @file    reserved_moov_demo.c
 * @brief   libmov 预留-moov 方案最小示例
 *          Minimal demo of the reserved-moov recording profile
 *
 * @details
 *          只依赖对外头文件 mov.h。
 *          Depends only on the public header mov.h.
 *          流程：预分配 → 打开 → create → write → 周期 finalize → close。
 *          Flow: preallocate, open, create, write, periodic finalize, close.
 *
 *          编译：make demo
 *          Build: make demo
 *          运行：./build/reserved_moov_demo [output.mp4]
 *          Run: ./build/reserved_moov_demo [output.mp4]
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "mov.h"

/*****************************************************************************
 * @brief   演示用文件尺寸，取库接受的 64 MiB
 *          Demo file size, the accepted 64 MiB value
 *****************************************************************************/
#define DEMO_FILE_SIZE      ((uint64_t)MOV_FILE_SIZE_64MIB)

/*****************************************************************************
 * @brief   演示帧率
 *          Demo frame rate
 *****************************************************************************/
#define DEMO_FPS            25

/*****************************************************************************
 * @brief   演示时长，秒
 *          Demo duration in seconds
 *****************************************************************************/
#define DEMO_DURATION_SEC   2

/*****************************************************************************
 * @brief   每隔多少帧刷一次 moov
 *          Flush moov every N frames
 *****************************************************************************/
#define DEMO_FLUSH_EVERY    25

/*****************************************************************************
 * @brief   填充一帧占位视频，不是合法 H.264
 *          Fill a placeholder video sample, not valid H.264
 *
 * @param   buf        输出缓冲
 *                     Output buffer
 * @param   len        缓冲长度
 *                     Buffer length
 * @param   frame_idx  帧序号
 *                     Frame index
 * @param   key        非 0 表示关键帧
 *                     Non-zero for a key frame
 *****************************************************************************/
static void demo_fill_video(uint8_t *buf, size_t len, int frame_idx, int key)
{
    size_t i;

    memset(buf, 0, len);
    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = (uint8_t)((len - 4) & 0xFF);
    buf[4] = key ? 0x65 : 0x41;
    for (i = 5; i < len; i++) {
        buf[i] = (uint8_t)((frame_idx + i) & 0xFF);
    }
}

/*****************************************************************************
 * @brief   填充一帧占位音频，不是合法 AAC
 *          Fill a placeholder audio sample, not valid AAC
 *
 * @param   buf        输出缓冲
 *                     Output buffer
 * @param   len        缓冲长度
 *                     Buffer length
 * @param   frame_idx  帧序号
 *                     Frame index
 *****************************************************************************/
static void demo_fill_audio(uint8_t *buf, size_t len, int frame_idx)
{
    size_t i;

    for (i = 0; i < len; i++) {
        buf[i] = (uint8_t)((frame_idx * 3 + i) & 0xFF);
    }
}

/*****************************************************************************
 * @brief   演示入口
 *          Demo entry
 *
 * @param   argc  参数个数
 *                Argument count
 * @param   argv  参数列表，argv[1] 可选输出路径
 *                Argument list, argv[1] is an optional output path
 * @return  0 成功，非 0 失败
 *          0 on success, non-zero on error
 *****************************************************************************/
int main(int argc, char **argv)
{
    const char *out_path = (argc > 1) ? argv[1] : "demo_reserved_moov.mp4";
    FILE *fp = NULL;
    mov_container_t *c = NULL;
    MovMediaInfo media;
    int vtrack;
    int atrack;
    int frame;
    int total;
    int ret = 1;
    uint8_t vbuf[512];
    uint8_t abuf[128];

    printf("libmov reserved-moov demo\n");
    printf("output : %s\n", out_path);
    printf("size   : %llu bytes\n", (unsigned long long)DEMO_FILE_SIZE);

    if (!mov_file_size_ok(DEMO_FILE_SIZE) ||
        mov_file_preallocate(out_path, DEMO_FILE_SIZE) != 0) {
        fprintf(stderr, "preallocate failed\n");
        return 1;
    }

    fp = fopen(out_path, "rb+");
    if (!fp) {
        fprintf(stderr, "reopen failed: %s\n", strerror(errno));
        return 1;
    }

    memset(&media, 0, sizeof(media));
    media.video_object = MOV_OBJECT_H264;
    media.video_width = 1280;
    media.video_height = 720;
    media.audio_object = MOV_OBJECT_AAC;
    media.audio_channel_count = 1;
    media.audio_bits_per_sample = 16;
    media.audio_sample_rate = 16000;

    c = mov_container_create(mov_file_buffer(), fp, MOV_FLAG_RESERVED_MOOV, &media);
    if (!c) {
        fprintf(stderr, "mov_container_create failed\n");
        goto done;
    }

    vtrack = mov_container_get_video_track(c);
    atrack = mov_container_get_audio_track(c);
    printf("tracks : video=%d audio=%d\n", vtrack, atrack);

    total = DEMO_FPS * DEMO_DURATION_SEC;
    for (frame = 0; frame < total; frame++) {
        int64_t pts_ms = (int64_t)frame * 1000 / DEMO_FPS;
        int key = (frame % DEMO_FPS == 0) ? MOV_AV_FLAG_KEYFRAME : 0;

        demo_fill_video(vbuf, sizeof(vbuf), frame, key);
        if (mov_container_write(c, vtrack, vbuf, sizeof(vbuf),
                                pts_ms, pts_ms, key) != 0) {
            fprintf(stderr, "write video frame %d failed\n", frame);
            goto done;
        }

        demo_fill_audio(abuf, sizeof(abuf), frame);
        if (mov_container_write(c, atrack, abuf, sizeof(abuf),
                                pts_ms, pts_ms, 0) != 0) {
            fprintf(stderr, "write audio frame %d failed\n", frame);
            goto done;
        }

        if ((frame + 1) % DEMO_FLUSH_EVERY == 0) {
            if (mov_container_finalize(c) != 0) {
                fprintf(stderr, "finalize at frame %d failed\n", frame);
                goto done;
            }
            printf("flushed moov at frame %d remain=%llu\n",
                   frame + 1,
                   (unsigned long long)mov_container_remain_size(c, 0));
        }
    }

    if (mov_container_finalize(c) != 0) {
        fprintf(stderr, "final finalize failed\n");
        goto done;
    }

    printf("done duration=%d s video_pts=%d s remain=%llu\n",
           DEMO_DURATION_SEC,
           mov_container_get_video_pts(c),
           (unsigned long long)mov_container_remain_size(c, 0));
    ret = 0;

done:
    if (c) {
        mov_container_close(c);
    }
    if (fp) {
        fclose(fp);
    }
    return ret;
}
