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
#ifndef __RKISP1_PARAMS_H__
#define __RKISP1_PARAMS_H__

#include <stdbool.h>

#include "rkisp1/rk_aiq.h"
#include "rkisp1/rkisp1-config.h"

struct AiqResults;

int rkisp1_check_params(struct rkisp1_isp_params_cfg *configs);
int rkisp1_convert_params(struct rkisp1_isp_params_cfg* isp_cfg, struct AiqResults* aiqResults);

#endif