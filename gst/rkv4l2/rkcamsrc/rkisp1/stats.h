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
#ifndef __RKISP1_STATS_H__
#define __RKISP1_STATS_H__

#include "common.h"

static inline void rkisp1_stats_convertAWB(struct cifisp_awb_stat* awb_stats, rk_aiq_awb_measure_result* aiq_awb_stats)
{
    int i;

    for (i = 0; i < CIFISP_AWB_MAX_GRID; i++) {
        aiq_awb_stats->awb_meas[i].num_white_pixel = awb_stats->awb_mean[i].cnt;
        aiq_awb_stats->awb_meas[i].mean_y__g = awb_stats->awb_mean[i].mean_y_or_g;
        aiq_awb_stats->awb_meas[i].mean_cb__b = awb_stats->awb_mean[i].mean_cb_or_b;
        aiq_awb_stats->awb_meas[i].mean_cr__r = awb_stats->awb_mean[i].mean_cr_or_r;
    }

    if (DEBUG)
        printf("AwbStatDump: awb:mean:cnt(%d), awb:mean:y_or_g(%d), awb:mean:cb_or_b(%d), awb:mean:cr_or_r(%d)",
            awb_stats->awb_mean[0].cnt,
            awb_stats->awb_mean[0].mean_y_or_g,
            awb_stats->awb_mean[0].mean_cb_or_b,
            awb_stats->awb_mean[0].mean_cr_or_r);
}

static inline void rkisp1_stats_convertAE(struct cifisp_ae_stat* ae_stats, rk_aiq_aec_measure_result* aiq_ae_stats)
{
    int i;

    for (i = 0; i < CIFISP_AE_MEAN_MAX; i++) {
        aiq_ae_stats->exp_mean[i] = ae_stats->exp_mean[i];
    }

    if (DEBUG)
        printf("AecStatDump: exp_mean(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d), bls_val(%d,%d,%d,%d)",
            ae_stats->exp_mean[0], ae_stats->exp_mean[1], ae_stats->exp_mean[2], ae_stats->exp_mean[3], ae_stats->exp_mean[4], ae_stats->exp_mean[5], ae_stats->exp_mean[6], ae_stats->exp_mean[7],
            ae_stats->exp_mean[8], ae_stats->exp_mean[9], ae_stats->exp_mean[10], ae_stats->exp_mean[11], ae_stats->exp_mean[12], ae_stats->exp_mean[13], ae_stats->exp_mean[14], ae_stats->exp_mean[15],
            ae_stats->exp_mean[16], ae_stats->exp_mean[17], ae_stats->exp_mean[18], ae_stats->exp_mean[19], ae_stats->exp_mean[20], ae_stats->exp_mean[21], ae_stats->exp_mean[22], ae_stats->exp_mean[23], ae_stats->exp_mean[24],
            ae_stats->bls_val.meas_r, ae_stats->bls_val.meas_gr, ae_stats->bls_val.meas_gb, ae_stats->bls_val.meas_b);
}

static inline void rkisp1_stats_convertAF(struct cifisp_af_stat* af_stats, rk_aiq_af_meas_stat* aiq_af_stats)
{
    int i;

    for (i = 0; i < CIFISP_AFM_MAX_WINDOWS; i++) {
        aiq_af_stats->window[i].lum = af_stats->window[i].lum;
        aiq_af_stats->window[i].sum = af_stats->window[i].sum;
    }

    if (DEBUG)
        printf("AfStatDump: window[1] (%d, %d), window[2] (%d, %d), window[3] (%d, %d) ",
            af_stats->window[0].sum, af_stats->window[0].lum,
            af_stats->window[1].sum, af_stats->window[1].lum,
            af_stats->window[2].sum, af_stats->window[2].lum);
}

static inline void rkisp1_stats_convertHIST(struct cifisp_hist_stat* hist_stats, rk_aiq_aec_measure_result* aiq_hist_stats)
{
    int i;

    for (i = 0; i < CIFISP_HIST_BIN_N_MAX; i++) {
        aiq_hist_stats->hist_bin[i] = hist_stats->hist_bins[i];
    }

    if (DEBUG)
        printf("HistStatDump: hist_bins[%d-%d]: %d, %d, %d, %d, %d, %d, %d, %d",
            i * 8, i * 8 + 7, hist_stats->hist_bins[i * 8], hist_stats->hist_bins[i * 8 + 1], hist_stats->hist_bins[i * 8 + 2],
            hist_stats->hist_bins[i * 8 + 3], hist_stats->hist_bins[i * 8 + 4], hist_stats->hist_bins[i * 8 + 5],
            hist_stats->hist_bins[i * 8 + 6], hist_stats->hist_bins[i * 8 + 7]);
}

static inline int rkisp1_convert_stats(struct rkisp1_stat_buffer* isp_stats, rk_aiq_statistics_input_params* aiq_stats)
{
    rkisp1_stats_convertAWB(&isp_stats->params.awb, &aiq_stats->awb_stats);
    rkisp1_stats_convertAE(&isp_stats->params.ae, &aiq_stats->aec_stats);
    rkisp1_stats_convertAF(&isp_stats->params.af, &aiq_stats->af_stats);
    rkisp1_stats_convertHIST(&isp_stats->params.hist, &aiq_stats->aec_stats);

    return 0;
}

#endif