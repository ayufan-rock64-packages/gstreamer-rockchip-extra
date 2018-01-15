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
#ifndef __RKISP1_V4L2_H__
#define __RKISP1_V4L2_H__

#define READY_STATUS 0
#define STREAMON_STATUS 1
#define STREAMOFF_STATUS 2

#include <stdbool.h>

#include "rkisp1-lib.h"

#define RKISP1_MAX_BUF 1

struct rkisp1_params;

struct RKISP1Buffer {
    void* start;
    unsigned long length;
};

struct RKISP1Core {
    /* fd */
    int isp_fd;
    int params_fd;
    int stats_fd;
    int sensor_fd;

    /* aiq */
    rk_aiq* mAiq;
    rk_aiq_exposure_sensor_descriptor sensor_desc;
    struct AiqResults aiq_results;
    struct RKISP1Buffer params_buf[RKISP1_MAX_BUF];
    struct RKISP1Buffer stats_buf[RKISP1_MAX_BUF];

    /* gain delay */
    int aGain[EXPOSURE_GAIN_DELAY];
    int dGain[EXPOSURE_GAIN_DELAY];
    int exposure[EXPOSURE_TIME_DELAY];

    /* other */
    int cur_frame_id;
    long long cur_time;
    int stats_skip;
};

int rkisp1_3a_core_init(struct RKISP1Core* rkisp1_core, struct rkisp1_params* params);
void rkisp1_3a_core_deinit(struct RKISP1Core* rkisp1_core);

int rkisp1_3a_core_process_stats(struct RKISP1Core* rkisp1_core);
int rkisp1_3a_core_process_params(struct RKISP1Core* rkisp1_core);

int rkisp1_3a_core_streamon(struct RKISP1Core* rkisp1_core);
int rkisp1_3a_core_streamoff(struct RKISP1Core* rkisp1_core);

void rkisp1_3a_core_run_ae(struct RKISP1Core* rkisp1_core);
void rkisp1_3a_core_run_awb(struct RKISP1Core* rkisp1_core);
void rkisp1_3a_core_run_misc(struct RKISP1Core* rkisp1_core);
void rkisp1_3a_core_run_af(struct RKISP1Core* rkisp1_core);

#endif