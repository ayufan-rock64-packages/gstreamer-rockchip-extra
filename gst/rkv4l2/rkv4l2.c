/* GStreamer
 *
 * Copyright (C) 2001-2002 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *               2006 Edgard Lima <edgard.lima@gmail.com>
 *
 * gstv4l2.c: plugin for v4l2 elements
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _GNU_SOURCE
# define _GNU_SOURCE            /* O_CLOEXEC */
#endif

#include "gst/gst-i18n-plugin.h"

#include <gst/gst.h>

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ext/videodev2.h"
#include "v4l2-utils.h"

#include "v4l2/gstv4l2object.h"
#include "rkcamsrc/rkcamsrc.h"
#include "rgaconvert/rgaconvert.h"

/* used in v4l2_calls.c and v4l2src_calls.c */
GST_DEBUG_CATEGORY (v4l2_debug);
#define GST_CAT_DEFAULT v4l2_debug

static gboolean
plugin_init (GstPlugin * plugin)
{
  const gchar *paths[] = { "/dev", "/dev/v4l2", NULL };
  const gchar *names[] = { "video", NULL };

  GST_DEBUG_CATEGORY_INIT (v4l2_debug, "rk_v4l2", 0,
      "V4L2 API calls(Rockchip)");

  /* Add some depedency, so the dynamic features get updated upon changes in
   * /dev/video* */
  gst_plugin_add_dependency (plugin,
      NULL, paths, names, GST_PLUGIN_DEPENDENCY_FLAG_FILE_NAME_IS_PREFIX);

  if (!gst_element_register (plugin, "rkcamsrc", GST_RANK_PRIMARY,
          GST_TYPE_RKCAMSRC) ||
      !gst_element_register (plugin, "rgaconvert", GST_RANK_NONE,
          GST_TYPE_RGACONVERT)
      )
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    rockchip_video4linux2,
    "elements for Video 4 Linux(Rockchip)",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
