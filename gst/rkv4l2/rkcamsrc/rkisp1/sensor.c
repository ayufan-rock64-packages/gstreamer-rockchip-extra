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
#include "sensor.h"

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

static int __get_format(int fd, rk_aiq_exposure_sensor_descriptor* sensor_desc)
{
    struct v4l2_subdev_format fmt;
    int ret;

    memset(&fmt, 0, sizeof(fmt));
    fmt.pad = 0;
    fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;

    ret = ioctl(fd, VIDIOC_SUBDEV_G_FMT, &fmt);
    if (ret < 0)
        return -errno;

    sensor_desc->sensor_output_width = fmt.format.width;
    sensor_desc->sensor_output_height = fmt.format.height;

    return 0;
}

static int __get_exposure_range(int fd, rk_aiq_exposure_sensor_descriptor* sensor_desc)
{
    struct v4l2_queryctrl ctrl;
    int ret;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = V4L2_CID_EXPOSURE;

    ret = ioctl(fd, VIDIOC_QUERYCTRL, &ctrl);
    if (ret < 0)
        return -errno;

    sensor_desc->coarse_integration_time_min = ctrl.minimum;
    sensor_desc->coarse_integration_time_max_margin = CIT_MAX_MARGIN;

    return ret;
}

static int __get_blank(int fd, rk_aiq_exposure_sensor_descriptor* sensor_desc)
{
    struct v4l2_queryctrl ctrl;
    int ret, horzBlank, vertBlank;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = V4L2_CID_HBLANK;
    ret = ioctl(fd, VIDIOC_QUERYCTRL, &ctrl);
    if (ret < 0)
        return -errno;
    horzBlank = ctrl.minimum;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = V4L2_CID_VBLANK;
    ret = ioctl(fd, VIDIOC_QUERYCTRL, &ctrl);
    if (ret < 0)
        return -errno;
    vertBlank = ctrl.minimum;

    sensor_desc->pixel_periods_per_line = horzBlank + sensor_desc->sensor_output_width;
    sensor_desc->line_periods_per_field = vertBlank + sensor_desc->sensor_output_height;

    sensor_desc->line_periods_vertical_blanking = vertBlank;

    //INFO: fine integration is not supported by v4l2
    sensor_desc->fine_integration_time_min = 0;
    sensor_desc->fine_integration_time_max_margin = sensor_desc->pixel_periods_per_line;

    return ret;
}

static int __get_pixel(int fd, rk_aiq_exposure_sensor_descriptor* sensor_desc)
{
    struct v4l2_ext_controls controls;
    struct v4l2_ext_control ext_control;
    signed long pixel;
    int ret;

    memset(&controls, 0, sizeof(controls));
    memset(&ext_control, 0, sizeof(ext_control));

    ext_control.id = V4L2_CID_PIXEL_RATE;
    controls.ctrl_class = V4L2_CTRL_ID2CLASS(ext_control.id);
    controls.count = 1;
    controls.controls = &ext_control;
    ret = ioctl(fd, VIDIOC_G_EXT_CTRLS, &controls);
    if (ret < 0)
        return -errno;
    pixel = ext_control.value64;

    sensor_desc->pixel_clock_freq_mhz = (float)pixel / 1000000;

    return 0;
}

int rkisp1_get_sensor_desc(int fd, rk_aiq_exposure_sensor_descriptor* sensor_desc)
{
    memset(sensor_desc, 0, sizeof(rk_aiq_exposure_sensor_descriptor));

    if (__get_format(fd, sensor_desc))
        return -1;
    if (__get_blank(fd, sensor_desc))
        return -1;
    if (__get_pixel(fd, sensor_desc))
        return -1;
    if (__get_exposure_range(fd, sensor_desc))
        return -1;

    return 0;
}

int rkisp1_apply_sensor_params(int fd, rk_aiq_exposure_sensor_parameters* expParams)
{
    struct v4l2_control ctrl;
    int ret;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = V4L2_CID_ANALOGUE_GAIN;
    ctrl.value = expParams->analog_gain_code_global;
    ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        return -errno;

    if (expParams->digital_gain_global != 0) {
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.id = V4L2_CID_GAIN;
        ctrl.value = expParams->digital_gain_global;
        ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
        if (ret < 0)
            return -errno;
    }

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = V4L2_CID_EXPOSURE;
    ctrl.value = expParams->coarse_integration_time;
    ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0)
        return -errno;

    if (DEBUG) {
        printf("Sensor AEC: analog_gain_code_global: %d, digital_gain_global: %d, coarse_integration_time: %d\n",
            expParams->analog_gain_code_global, expParams->digital_gain_global, expParams->coarse_integration_time);
    }

    return 0;
}
