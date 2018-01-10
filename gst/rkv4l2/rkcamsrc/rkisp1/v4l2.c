/*
 * Copyright 2017 Rockchip Electronics Co., Ltd
 *     Author: Jacob Chen <jacob2.chen@rock-chips.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#include "v4l2.h"
#include "common.h"
#include "params.h"
#include "sensor.h"
#include "stats.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "ext/videodev2.h"

static int __check_cap(int params_fd, int stats_fd)
{
    struct v4l2_capability cap = { 0 };
    int ret = 0;

    ret = ioctl(params_fd, VIDIOC_QUERYCAP, &cap);
    if (ret)
        return ret;
    if (!(cap.capabilities & V4L2_CAP_META_OUTPUT)) {
        return -1;
    }

    ret = ioctl(stats_fd, VIDIOC_QUERYCAP, &cap);
    if (ret)
        return ret;
    if (!(cap.capabilities & V4L2_CAP_META_CAPTURE)) {
        return -1;
    }

    return 0;
}

static int __reqs_buffer(int params_fd, int stats_fd)
{
    struct v4l2_requestbuffers reqbuf;
    int ret = 0;

    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.count = RKISP1_MAX_BUF;
    reqbuf.type = V4L2_BUF_TYPE_META_OUTPUT;
    reqbuf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(params_fd, VIDIOC_REQBUFS, &reqbuf);
    if (ret != 0) {
        printf("RKISP1: failed to ioctl VIDIOC_REQBUFS for %d %s.\n", errno, strerror(errno));
        return -1;
    }

    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.count = RKISP1_MAX_BUF;
    reqbuf.type = V4L2_BUF_TYPE_META_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(stats_fd, VIDIOC_REQBUFS, &reqbuf);
    if (ret != 0) {
        printf("RKISP1: failed to ioctl VIDIOC_REQBUFS for %d %s.\n", errno, strerror(errno));
        return -1;
    }

    return ret;
}

static int __init_mmap(struct RKISP1Core* rkisp1_core)
{
    int n_buffers, ret = 0;

    for (n_buffers = 0; n_buffers < RKISP1_MAX_BUF; ++n_buffers) {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_META_OUTPUT;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        ret = ioctl(rkisp1_core->params_fd, VIDIOC_QUERYBUF, &buf);
        if (ret != 0) {
            printf("RKISP1: failed to ioctl VIDIOC_QUERYBUF for %d %s.\n", errno, strerror(errno));
            return -1;
        }

        rkisp1_core->params_buf[n_buffers].length = buf.length;
        rkisp1_core->params_buf[n_buffers].start = mmap(NULL /* start anywhere */,
            buf.length,
            PROT_READ | PROT_WRITE /* required */,
            MAP_SHARED /* recommended */,
            rkisp1_core->params_fd, buf.m.offset);

        if (rkisp1_core->params_buf[n_buffers].start == MAP_FAILED) {
            printf("RKISP1: failed to mmap. bufer.index %d.\n", buf.index);
            return -1;
        }
    }

    for (n_buffers = 0; n_buffers < RKISP1_MAX_BUF; ++n_buffers) {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_META_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        ret = ioctl(rkisp1_core->stats_fd, VIDIOC_QUERYBUF, &buf);
        if (ret != 0) {
            printf("RKISP1: failed to ioctl VIDIOC_QUERYBUF for %d %s.\n", errno, strerror(errno));
            return -1;
        }

        rkisp1_core->stats_buf[n_buffers].length = buf.length;
        rkisp1_core->stats_buf[n_buffers].start = mmap(NULL /* start anywhere */,
            buf.length,
            PROT_READ | PROT_WRITE /* required */,
            MAP_SHARED /* recommended */,
            rkisp1_core->stats_fd, buf.m.offset);

        if (rkisp1_core->stats_buf[n_buffers].start == MAP_FAILED) {
            printf("RKISP1: failed to mmap. bufer.index %d.\n", buf.index);
            return -1;
        }
    }

    return ret;
}

int rkisp1_3a_core_init(struct RKISP1Core* rkisp1_core, const char* params_node,
    const char* stats_node, const char* sensor_node, const char* xml_path)
{
    int ret = 0;

    rkisp1_core->sensor_fd = open(sensor_node, O_RDWR);
    if (rkisp1_core->sensor_fd < 0) {
        printf("RKISP1: Failed to open %s!\n", sensor_node);
        goto fail;
    }

    rkisp1_core->params_fd = open(params_node, O_RDWR);
    if (rkisp1_core->params_fd < 0) {
        printf("RKISP1: Failed to open %s!\n", params_node);
        goto close_sensor;
    }

    rkisp1_core->stats_fd = open(stats_node, O_RDWR);
    if (rkisp1_core->stats_fd < 0) {
        printf("RKISP1: Failed to open %s!\n", stats_node);
        goto close_params;
    }

    if (__check_cap(rkisp1_core->params_fd, rkisp1_core->stats_fd)) {
        printf("RKISP1: %s/%s is not rkisp1 params/stats!\n", params_node, stats_node);
        goto close_stats;
    }

    if (__reqs_buffer(rkisp1_core->params_fd, rkisp1_core->stats_fd)) {
        printf("RKISP1: failed to require buffers!\n");
        goto close_stats;
    }

    if (__init_mmap(rkisp1_core)) {
        printf("RKISP1: failed to init buffers!\n");
        goto close_stats;
    }

    rkisp1_core->mAiq = rk_aiq_init(xml_path);
    if (rkisp1_core->mAiq == NULL) {
        printf("RKISP1: failed to init aiq!\n");
        goto close_stats;
    }
    memset(&rkisp1_core->aiq_results, 0, sizeof(struct AiqResults));

    if (rkisp1_get_sensor_desc(rkisp1_core->sensor_fd, &rkisp1_core->sensor_desc)) {
        printf("RKISP1: failed to init sensor desc!\n");
        goto close_stats;
    }

    return ret;

close_stats:
    close(rkisp1_core->stats_fd);
close_params:
    close(rkisp1_core->params_fd);
close_sensor:
    close(rkisp1_core->sensor_fd);
fail:
    return -1;
}

void rkisp1_3a_core_deinit(struct RKISP1Core* rkisp1_core)
{
    int i;

    for (i = 0; i < RKISP1_MAX_BUF; ++i) {
        munmap(rkisp1_core->params_buf[i].start, rkisp1_core->params_buf[i].length);
        munmap(rkisp1_core->stats_buf[i].start, rkisp1_core->stats_buf[i].length);
    }

    close(rkisp1_core->params_fd);
    close(rkisp1_core->stats_fd);
    close(rkisp1_core->sensor_fd);

    rk_aiq_deinit(rkisp1_core->mAiq);
}

int rkisp1_3a_core_streamon(struct RKISP1Core* rkisp1_core)
{
    enum v4l2_buf_type type;
    int i, ret = 0;

    /* params should just use one buffer */
    for (i = 0; i < 1; ++i) {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_META_OUTPUT;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        ret = ioctl(rkisp1_core->params_fd, VIDIOC_QBUF, &buf);
        if (ret != 0)
            printf("RKISP1: failed to ioctl VIDIOC_QBUF for %d %s.\n",
                errno, strerror(errno));
    }

    type = V4L2_BUF_TYPE_META_OUTPUT;
    ret = ioctl(rkisp1_core->params_fd, VIDIOC_STREAMON, &type);
    if (ret != 0) {
        printf("RKISP1: failed to ioctl VIDIOC_STREAMON for %d %s.\n",
            errno, strerror(errno));
        return ret;
    }

    for (i = 0; i < RKISP1_MAX_BUF; ++i) {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_META_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        ret = ioctl(rkisp1_core->stats_fd, VIDIOC_QBUF, &buf);
        if (ret != 0)
            printf("RKISP1: failed to ioctl VIDIOC_QBUF for %d %s.\n",
                errno, strerror(errno));
    }

    type = V4L2_BUF_TYPE_META_CAPTURE;
    ret = ioctl(rkisp1_core->stats_fd, VIDIOC_STREAMON, &type);
    if (ret != 0) {
        printf("RKISP1: failed to ioctl VIDIOC_STREAMON for %d %s.\n",
            errno, strerror(errno));
        return ret;
    }

    return ret;
}

int rkisp1_3a_core_streamoff(struct RKISP1Core* rkisp1_core)
{
    enum v4l2_buf_type type;
    int ret = 0;

    type = V4L2_BUF_TYPE_META_OUTPUT;
    ret = ioctl(rkisp1_core->params_fd, VIDIOC_STREAMOFF, &type);
    if (ret != 0) {
        printf("RKISP1: failed to ioctl VIDIOC_STREAMON for %d %s.\n",
            errno, strerror(errno));
        return ret;
    }

    type = V4L2_BUF_TYPE_META_CAPTURE;
    ret = ioctl(rkisp1_core->stats_fd, VIDIOC_STREAMOFF, &type);
    if (ret != 0) {
        printf("RKISP1: failed to ioctl VIDIOC_STREAMON for %d %s.\n",
            errno, strerror(errno));
        return ret;
    }

    return ret;
}

int rkisp1_3a_core_process_stats(struct RKISP1Core* rkisp1_core)
{
    rk_aiq_statistics_input_params ispStatistics;
    struct rkisp1_stat_buffer* isp_stats;
    struct v4l2_buffer buf = { 0 };
    int ret = 0;

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_META_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    ret = ioctl(rkisp1_core->stats_fd, VIDIOC_DQBUF, &buf);
    if (ret != 0) {
        printf("RKISP1: failed to ioctl VIDIOC_DQBUF for %d %s.\n",
            errno, strerror(errno));
        return ret;
    }

    if (DEBUG)
        printf("\t stats buf.sequence: %d\n", buf.sequence);

    isp_stats = (struct rkisp1_stat_buffer*)rkisp1_core->stats_buf[buf.index].start;
    ispStatistics.ae_results = &rkisp1_core->aiq_results.aeResults;
    ispStatistics.awb_results = &rkisp1_core->aiq_results.awbResults;
    ispStatistics.af_results = &rkisp1_core->aiq_results.afResults;
    ispStatistics.misc_results = &rkisp1_core->aiq_results.miscIspResults;
    rkisp1_convert_stats(isp_stats, &ispStatistics);
    rk_aiq_stats_set(rkisp1_core->mAiq, &ispStatistics, &rkisp1_core->sensor_desc);

    ret = ioctl(rkisp1_core->stats_fd, VIDIOC_QBUF, &buf);
    if (ret != 0) {
        printf("RKISP1: failed to ioctl VIDIOC_QBUF for %d %s.\n",
            errno, strerror(errno));
        return ret;
    }

    return ret;
}

int rkisp1_3a_core_process_params(struct RKISP1Core* rkisp1_core)
{
    struct rkisp1_isp_params_cfg* isp_params;
    struct v4l2_buffer buf = { 0 };
    int ret = 0;

    /* apply sensor */
    if (rkisp1_apply_sensor_params(rkisp1_core->sensor_fd, &rkisp1_core->aiq_results.aeResults.sensor_exposure)) {
        printf("RKISP1: failed to apply sensor params for %d %s.\n",
            errno, strerror(errno));
        return ret;
    }

    /* apply isp_params */
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_META_OUTPUT;
    buf.memory = V4L2_MEMORY_MMAP;
    ret = ioctl(rkisp1_core->params_fd, VIDIOC_DQBUF, &buf);
    if (ret != 0) {
        printf("RKISP1: failed to ioctl VIDIOC_DQBUF for %d %s.\n",
            errno, strerror(errno));
        return ret;
    }

    if (DEBUG)
        printf("\t params buf.sequence: %d\n", buf.sequence);

    isp_params = (struct rkisp1_isp_params_cfg*)rkisp1_core->params_buf[buf.index].start;
    memset(isp_params, 0, sizeof(struct rkisp1_isp_params_cfg));
    rkisp1_convert_params(isp_params, &rkisp1_core->aiq_results);
    rkisp1_check_params(isp_params);

    ret = ioctl(rkisp1_core->params_fd, VIDIOC_QBUF, &buf);
    if (ret != 0) {
        printf("RKISP1: failed to ioctl VIDIOC_QBUF for %d %s.\n",
            errno, strerror(errno));
        return ret;
    }

    return ret;
}

void rkisp1_3a_core_run_ae(struct RKISP1Core* rkisp1_core)
{
    rk_aiq_ae_input_params aeInputParams;
    rk_aiq_ae_results results;
    int status = 0;

    memset(&aeInputParams, 0, sizeof(aeInputParams));
    memset(&results, 0, sizeof(results));

    aeInputParams.num_exposures = 1;
    aeInputParams.frame_use = rk_aiq_frame_use_preview;
    aeInputParams.flash_mode = rk_aiq_flash_mode_off;

    aeInputParams.manual_limits = NULL;
    aeInputParams.manual_exposure_time_us = NULL;
    aeInputParams.manual_analog_gain = NULL;
    aeInputParams.manual_iso = NULL;
    aeInputParams.ev_shift = 0.0f;

    aeInputParams.operation_mode = rk_aiq_ae_operation_mode_automatic;
    aeInputParams.metering_mode = rk_aiq_ae_metering_mode_evaluative;
    aeInputParams.priority_mode = rk_aiq_ae_priority_mode_normal;
    aeInputParams.flicker_reduction_mode = rk_aiq_ae_flicker_reduction_off;

    aeInputParams.window = NULL;

    aeInputParams.sensor_descriptor = &rkisp1_core->sensor_desc;

    status = rk_aiq_ae_run(rkisp1_core->mAiq, &aeInputParams, &results);
    if (status)
        printf("RKISP1: Error running AE %d", status);

    rkisp1_core->aiq_results.aeResults = results;
}

void rkisp1_3a_core_run_awb(struct RKISP1Core* rkisp1_core)
{
    rk_aiq_awb_input_params awbInputParams;
    rk_aiq_awb_results results;
    int status = 0;

    memset(&awbInputParams, 0, sizeof(awbInputParams));
    memset(&results, 0, sizeof(results));

    awbInputParams.frame_use = rk_aiq_frame_use_preview;
    awbInputParams.manual_cct_range = NULL;
    awbInputParams.window = NULL;

    status = rk_aiq_awb_run(rkisp1_core->mAiq, &awbInputParams, &results);
    if (status)
        printf("RKISP1: Error running AWB %d", status);

    rkisp1_core->aiq_results.awbResults = results;
}

void rkisp1_3a_core_run_misc(struct RKISP1Core* rkisp1_core)
{
    rk_aiq_misc_isp_input_params miscInputParams;
    rk_aiq_misc_isp_results results;
    int status = 0;

    memset(&miscInputParams, 0, sizeof(miscInputParams));
    memset(&results, 0, sizeof(results));

    status = rk_aiq_misc_run(rkisp1_core->mAiq, &miscInputParams, &results);
    if (status)
        printf("RKISP1: Error running MISC %d", status);

    rkisp1_core->aiq_results.miscIspResults = results;
}

/* TODO: add AF support */
void rkisp1_3a_core_run_af(struct RKISP1Core* rkisp1_core)
{
    rk_aiq_af_input_params afInputParams;
    rk_aiq_af_results results;
    int status = 0;

    memset(&afInputParams, 0, sizeof(afInputParams));
    memset(&results, 0, sizeof(results));

    status = rk_aiq_af_run(rkisp1_core->mAiq, &afInputParams, &results);
    if (status)
        printf("RKISP1: Error running AF %d", status);
}
