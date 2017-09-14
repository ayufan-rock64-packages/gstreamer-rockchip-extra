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

#include "gstv4l2object.h"
#include "v4l2_calls.h"

gboolean rk_common_set_rotation (GstV4l2Object * v4l2object, gint rotate);
gboolean rk_common_set_vflip (GstV4l2Object * v4l2object, gboolean flip);
gboolean rk_common_set_hflip (GstV4l2Object * v4l2object, gboolean flip);
gboolean rk_common_set_selection (GstV4l2Object * v4l2object,
    GstVideoRectangle * crop);

void rk_common_v4l2device_find_by_name (const char *name, char *ret_name);
void rk_common_install_rockchip_properties_helper (GObjectClass *
    gobject_class);

gboolean rk_common_set_property_helper (GstV4l2Object * v4l2object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
gboolean rk_common_get_property_helper (GstV4l2Object * v4l2object,
    guint prop_id, GValue * value, GParamSpec * pspec);

void rk_common_new (GstV4l2Object * v4l2object);

#endif
