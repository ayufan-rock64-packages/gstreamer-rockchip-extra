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
#ifndef __RKISP1_THREAD_H__
#define __RKISP1_THREAD_H__

#include <pthread.h>
#include <semaphore.h>

/* disable af/awb/ae */
#define AAA_DISABLE_MODE 0
/* disable af */
#define AF_DISABLE_MODE 1
/* enable af/awb/ae */
#define AAA_ENABLE_MODE 2

#define READY_STATUS 0
#define RUN_STATUS 1
#define STARTING_STATUS 2
#define STOPING_STATUS 3
#define EXITING_STATUS 4
#define EXITED_STATUS 5

struct media_entity;
struct AiqResults;
struct RKISP1Core;

struct rkisp1_params {
    const char* isp_node;
    const char* params_node;
    const char* stats_node;
    const char* sensor_node;
    const char* xml_path;
    int mode;

    unsigned short isp_input_width;
    unsigned short isp_input_height;
    unsigned short isp_output_width;
    unsigned short isp_output_height;
};

struct RKISP1Thread {
    pthread_t tid;
    pthread_mutex_t mutex;
    pthread_mutex_t result_mutex;

    int mode;
    volatile int status;

    struct RKISP1Core* rkisp1_core;
};

struct RKISP1Thread* RKISP1_3A_THREAD_CREATE(struct rkisp1_params* params);
void RKISP1_3A_THREAD_EXIT(struct RKISP1Thread* rkisp1_thread);

void RKISP1_3A_THREAD_START(struct RKISP1Thread* rkisp1_thread);
void RKISP1_3A_THREAD_STOP(struct RKISP1Thread* rkisp1_thread);

void RKISP1_GET_3A_RESULT(struct RKISP1Thread* rkisp1_thread, struct AiqResults* ret_result);

#endif
