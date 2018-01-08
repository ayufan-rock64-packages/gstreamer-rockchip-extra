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
#ifndef __RK_COMMON_H__
#define __RK_COMMON_H__

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ext/videodev2.h"

#include <gst/gst.h>
#include <gst/gstatomicqueue.h>
#include <gst/base/gstpushsrc.h>
#include <gst/video/video.h>

// values
G_BEGIN_DECLS
#define GST_TYPE_RK_3A_MODE (gst_rk_3a_mode_get_type ())
    GType gst_rk_3a_mode_get_type (void);

typedef enum
{
  RK_3A_DISABLE = 0,
  RK_3A_AEAWB = 1,
  RK_3A_AEAWBAF = 2,
} GstRk3AMode;
G_END_DECLS

#define RK_V4L2_OBJECT_PROPS \
    PROP_VPU_STRIDE, \
    PROP_OUTPUT_ROTATION, \
    PROP_VFLIP, \
    PROP_HFLIP, \
    PROP_OUTPUT_CROP, \
    PROP_INPUT_CROP, \
    PROP_DISABLE_AUTOCONF, \
    PROP_3A_MODE, \
    PROP_XML_FILE

#define RK_V4L2_OBJECT \
  /* Rockchip Common */ \
  GstVideoRectangle input_crop; \
  GstVideoRectangle output_crop; \
  /* Rockchip RGA */ \
  guint32 rotation; \
  gboolean vflip; \
  gboolean hflip; \
  gboolean vpu_stride; \
  /* Rockchip ISP */ \
  gboolean disable_autoconf; \
  GstRk3AMode isp_mode;  \
  const gchar *xml_path;

struct _GstV4l2Object;

// utils
gboolean rk_common_v4l2device_find_by_name (const char *name, char *ret_name);

inline void
gst_rect_to_v4l2_rect (GstVideoRectangle * gst_rect, struct v4l2_rect *rect)
{
  rect->left = gst_rect->x;
  rect->top = gst_rect->y;
  rect->width = gst_rect->w;
  rect->height = gst_rect->h;
}

inline void
v4l2_rect_to_gst_rect (struct v4l2_rect *rect, GstVideoRectangle * gst_rect)
{
  gst_rect->x = rect->left;
  gst_rect->y = rect->top;
  gst_rect->w = rect->width;
  gst_rect->h = rect->height;
}

// v4l2
gboolean rk_common_v4l2_set_rotation (struct _GstV4l2Object *v4l2object,
    gint rotate);
gboolean rk_common_v4l2_set_vflip (struct _GstV4l2Object *v4l2object,
    gboolean flip);
gboolean rk_common_v4l2_set_hflip (struct _GstV4l2Object *v4l2object,
    gboolean flip);
gboolean rk_common_v4l2_set_selection (struct _GstV4l2Object *v4l2object,
    struct v4l2_rect *rect, gboolean compose);

// prop
void rk_common_install_rockchip_properties_helper (GObjectClass *
    gobject_class);
gboolean rk_common_set_property_helper (struct _GstV4l2Object *v4l2object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
gboolean rk_common_get_property_helper (struct _GstV4l2Object *v4l2object,
    guint prop_id, GValue * value, GParamSpec * pspec);
void rk_common_new (struct _GstV4l2Object *v4l2object);

#endif
