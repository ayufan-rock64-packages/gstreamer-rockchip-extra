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

#include "rkisp1/rk_aiq.h"
#include "rkisp1/rkisp1-config.h"

#define RKISP1_MAX_BUF 1

struct RKISP1Buffer {
    void* start;
    unsigned long length;
};

struct AiqResults {
    rk_aiq_ae_results aeResults;
    rk_aiq_awb_results awbResults;
    rk_aiq_af_results afResults;
    rk_aiq_misc_isp_results miscIspResults;
};

struct RKISP1Core {
    int params_fd;
    int stats_fd;
    int sensor_fd;

    rk_aiq* mAiq;
    rk_aiq_exposure_sensor_descriptor sensor_desc;
    struct AiqResults aiq_results;
    struct RKISP1Buffer params_buf[RKISP1_MAX_BUF];
    struct RKISP1Buffer stats_buf[RKISP1_MAX_BUF];
};

int rkisp1_3a_core_init(struct RKISP1Core* rkisp1_core, const char* params_node,
    const char* stats_node, const char* sensor_node, const char* xml_path);
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