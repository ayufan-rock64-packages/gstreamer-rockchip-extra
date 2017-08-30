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

#ifndef __GST_ISPSRC_H__
#define __GST_ISPSRC_H__

#include <gstv4l2object.h>
#include <gstv4l2bufferpool.h>

GST_DEBUG_CATEGORY_EXTERN (ispsrc_debug);

G_BEGIN_DECLS
#define GST_TYPE_ISPSRC \
  (gst_ispsrc_get_type())
#define GST_ISPSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ISPSRC,GstISPSrc))
#define GST_ISPSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ISPSRC,GstISPSrcClass))
#define GST_IS_ISPSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ISPSRC))
#define GST_IS_ISPSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ISPSRC))

typedef struct _GstISPSrc GstISPSrc;
typedef struct _GstISPSrcClass GstISPSrcClass;

/**
 * GstISPSrc:
 *
 * Opaque object.
 */
struct _GstISPSrc
{
  GstPushSrc pushsrc;

  /*< private > */
  GstV4l2Object *v4l2object;

  guint64 offset;

  /* offset adjust after renegotiation */
  guint64 renegotiation_adjust;

  GstClockTime ctrl_time;

  gboolean pending_set_fmt;

  /* Timestamp sanity check */
  GstClockTime last_timestamp;
  gboolean has_bad_timestamp;
};

struct _GstISPSrcClass
{
  GstPushSrcClass parent_class;

  GList *v4l2_class_devices;
};

GType gst_ispsrc_get_type (void);

G_END_DECLS
#endif /* __GST_ISPSrc_H__ */
