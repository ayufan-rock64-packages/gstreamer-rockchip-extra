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
#ifndef __GST_RGA_CONVERT_H__
#define __GST_RGA_CONVERT_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

#include <gstv4l2object.h>
#include <gstv4l2bufferpool.h>

GST_DEBUG_CATEGORY_EXTERN (v4l2transform_debug);

G_BEGIN_DECLS
#define GST_TYPE_RGACONVERT \
  (gst_rga_convert_get_type())
#define GST_RGA_CONVERT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RGACONVERT,GstRGAConvert))
#define GST_RGA_CONVERT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RGACONVERT,GstRGAConvertClass))
#define GST_IS_V4L2_TRANSFORM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RGACONVERT))
#define GST_IS_V4L2_TRANSFORM_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RGACONVERT))
#define GST_RGA_CONVERT_GET_CLASS(inst) \
  (G_TYPE_INSTANCE_GET_CLASS ((inst),GST_TYPE_RGACONVERT,GstRGAConvertClass))
typedef struct _GstRGAConvert GstRGAConvert;
typedef struct _GstRGAConvertClass GstRGAConvertClass;

struct _GstRGAConvert
{
  GstBaseTransform parent;

  /* < private > */
  GstRKV4l2Object *v4l2output;
  GstRKV4l2Object *v4l2capture;

  /* pads */
  GstCaps *probed_srccaps;
  GstCaps *probed_sinkcaps;

  /* Selected caps */
  GstCaps *incaps;
  GstCaps *outcaps;

  gchar default_device[32];
};

struct _GstRGAConvertClass
{
  GstBaseTransformClass parent_class;
};

GType gst_rga_convert_get_type (void);

gboolean gst_v4l2_is_transform (GstCaps * sink_caps, GstCaps * src_caps);
gboolean gst_rga_convert_register (GstPlugin * plugin,
    const gchar * basename,
    const gchar * device_path, GstCaps * sink_caps, GstCaps * src_caps);

G_END_DECLS
#endif /* __GST_RGA_CONVERT_H__ */
