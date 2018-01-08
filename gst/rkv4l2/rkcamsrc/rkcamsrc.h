/* GStreamer
 *
 * Copyright (C) 2001-2002 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *               2006 Edgard Lima <edgard.lima@gmail.com>
 *
 * gstv4l2src.h: BT8x8/V4L2 source element
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
 */

#ifndef __GST_RKCAMSRC_H__
#define __GST_RKCAMSRC_H__

#include <gstv4l2object.h>
#include <gstv4l2bufferpool.h>

#include "media-controller.h"
#include "rkisp1/thread.h"

enum rkisp1_isp_pad
{
  RKISP1_ISP_PAD_SINK,
  RKISP1_ISP_PAD_SINK_PARAMS,
  RKISP1_ISP_PAD_SOURCE_PATH,
  RKISP1_ISP_PAD_SOURCE_STATS,
  /* TODO: meta data pad ? */
  RKISP1_ISP_PAD_MAX
};

enum mipi_dphy_sy_pads
{
  MIPI_DPHY_SY_PAD_SINK = 0,
  MIPI_DPHY_SY_PAD_SOURCE,
  MIPI_DPHY_SY_PADS_NUM,
};

G_BEGIN_DECLS
#define GST_TYPE_RKCAMSRC \
  (gst_rkcamsrc_get_type())
#define GST_RKCAMSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RKCAMSRC,GstRKCamSrc))
#define GST_RKCAMSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RKCAMSRC,GstRKCamSrcClass))
#define GST_IS_RKCAMSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RKCAMSRC))
#define GST_IS_RKCAMSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RKCAMSRC))
typedef struct _GstRKCamSrc GstRKCamSrc;
typedef struct _GstRKCamSrcClass GstRKCamSrcClass;

#define ISP_MAX_SENSOR 32

/**
 * GstRKCamSrc:
 *
 * Opaque object.
 */
struct _GstRKCamSrc
{
  GstPushSrc pushsrc;

  struct RKISP1Thread *thread_3a;

  /* media controller */
  GstMediaController *controller;
  GstMediaEntity *main_path;
  GstMediaEntity *self_path;
  GstMediaEntity *isp_subdev;
  GstMediaEntity *isp_params_dev;
  GstMediaEntity *isp_stats_dev;
  GstMediaEntity *phy_subdev;
  GstMediaEntity *sensor_subdev;

  /* v4l2 stream */
  GstRKV4l2Object *capture_object;

  /* v4l2src part */
  guint64 offset;
  /* offset adjust after renegotiation */
  GstClockTime ctrl_time;
  /* Timestamp sanity check */
  GstClockTime last_timestamp;
  gboolean has_bad_timestamp;
};

struct _GstRKCamSrcClass
{
  GstPushSrcClass parent_class;

  GList *v4l2_class_devices;
};

GType gst_rkcamsrc_get_type (void);

G_END_DECLS
#endif /* __GST_RKCAMSRC_H__ */
