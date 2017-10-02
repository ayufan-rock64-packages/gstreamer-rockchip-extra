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
#include "ext/mediactl.h"
#include "ext/v4l2subdev.h"

#include <gst/gst.h>
#include <gst/gstatomicqueue.h>
#include <gst/base/gstpushsrc.h>
#include <gst/video/video.h>

// values
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

#define RK_V4L2_OBJECT_PROPS \
    PROP_VPU_STRIDE, \
    PROP_OUTPUT_ROTATION, \
    PROP_VFLIP, \
    PROP_HFLIP, \
    PROP_OUTPUT_CROP, \
    PROP_INPUT_CROP, \
    PROP_DISABLE_AUTOCONF, \
    PROP_VIDEO_CROP, \
    PROP_VIDEO_COMPOSE, \
    PROP_SENSOR_CROP, \
    PROP_SENSOR_NAME, \
    PROP_DISABLE_3A

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
  gboolean disable_3A; \
  gchar *sensor_name; \
  GstVideoRectangle video_crop; \
  GstVideoRectangle video_compose; \
  GstVideoRectangle sensor_crop;

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

// media
void rk_common_media_init_global_data (void);
guint rk_common_media_find_by_vnode (const char *vnode);
struct media_entity *rk_common_media_find_subdev_by_name (guint index,
    const char *subdev_name);
struct media_entity *rk_common_media_get_last_enity (guint index);

#endif
