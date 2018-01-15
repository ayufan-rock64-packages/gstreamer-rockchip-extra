#ifndef __RKISP1_LIB_H__
#define __RKISP1_LIB_H__

#include <stdbool.h>
#include <stdio.h>
#include <errno.h>

#include "ext/videodev2.h"
#include <linux/v4l2-subdev.h>

#include "rkisp1/regs.h"
#include "rkisp1/rk_aiq.h"
#include "rkisp1/rkisp1-config.h"

/* parameter (TODO: add interface to change config in runtime) */
#define DEBUG 0

#define STATS_SKIP  2

#define EXPOSURE_GAIN_DELAY 4
#define EXPOSURE_TIME_DELAY 4

#define CIT_MAX_MARGIN 10

/* sturct */
struct AiqResults {
    rk_aiq_ae_results aeResults;
    rk_aiq_awb_results awbResults;
    rk_aiq_af_results afResults;
    rk_aiq_misc_isp_results miscIspResults;
};

#endif
