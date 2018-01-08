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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"
#include "v4l2_calls.h"

#include <gst/gst-i18n-plugin.h>

#define SYS_PATH		"/sys/class/video4linux/"
#define DEV_PATH		"/dev/"

enum
{
  PROP_0,
  V4L2_STD_OBJECT_PROPS
};

/*
 * utils
 */
static void
__input_string_to_rect (char *input, GstVideoRectangle * rect)
{
  char delims[] = "x";
  char *result = NULL;

  result = strtok (input, delims);
  rect->x = atoi (result);
  result = strtok (NULL, delims);
  rect->y = atoi (result);
  result = strtok (NULL, delims);
  rect->w = atoi (result);
  result = strtok (NULL, delims);
  rect->h = atoi (result);
}

gboolean
rk_common_v4l2device_find_by_name (const char *name, char *ret_name)
{
  DIR *dir;
  struct dirent *ent;
  gboolean ret = FALSE;

  if ((dir = opendir (SYS_PATH)) != NULL) {
    while ((ent = readdir (dir)) != NULL) {
      FILE *fp;
      char path[512];
      char dev_name[512];

      snprintf (path, 512, SYS_PATH "%s/name", ent->d_name);
      fp = fopen (path, "r");
      if (!fp)
        continue;
      if (!fgets (dev_name, 32, fp))
        dev_name[0] = '\0';
      fclose (fp);

      if (!strstr (dev_name, name))
        continue;

      if (ret_name)
        snprintf (ret_name, 512, DEV_PATH "%s", ent->d_name);

      ret = TRUE;
      break;
    }
    closedir (dir);
  }

  return ret;
}

/*
 * v4l2 calls
 */

gboolean
rk_common_v4l2_set_rotation (GstRKV4l2Object * v4l2object, gint rotate)
{
  return gst_v4l2_set_attribute (v4l2object, V4L2_CID_ROTATE, rotate);
}

gboolean
rk_common_v4l2_set_vflip (GstRKV4l2Object * v4l2object, gboolean flip)
{
  return gst_v4l2_set_attribute (v4l2object, V4L2_CID_VFLIP, flip);
}

gboolean
rk_common_v4l2_set_hflip (GstRKV4l2Object * v4l2object, gboolean flip)
{
  return gst_v4l2_set_attribute (v4l2object, V4L2_CID_HFLIP, flip);
}

gboolean
rk_common_v4l2_set_selection (GstRKV4l2Object * v4l2object,
    struct v4l2_rect * rect, gboolean compose)
{
  struct v4l2_selection s = { 0 };

  s.type = v4l2object->type;
  s.target = compose ? V4L2_SEL_TGT_COMPOSE : V4L2_SEL_TGT_CROP;
  s.r = *rect;

  GST_DEBUG_OBJECT (v4l2object->element,
      "Desired cropping left %u, top %u, size %ux%u", s.r.left, s.r.top,
      s.r.width, s.r.height);

  if (v4l2_ioctl (v4l2object->video_fd, VIDIOC_S_SELECTION, &s) < 0) {
    GST_WARNING_OBJECT (v4l2object->element, "VIDIOC_S_SELECTION failed");
    return FALSE;
  }

  *rect = s.r;

  return TRUE;
}

/*
 * properties
 */

GType
gst_rk_3a_mode_get_type (void)
{
  static GType rk_3a_mode = 0;

  if (!rk_3a_mode) {
    static const GEnumValue modes_3a[] = {
      {RK_3A_DISABLE, "RK_3A_DISABLE", "0A"},
      {RK_3A_AEAWB, "RK_3A_AEAWB", "2A"},
      {RK_3A_AEAWBAF, "RK_3A_AEAWBAF", "3A"},
      {0, NULL, NULL}
    };
    rk_3a_mode = g_enum_register_static ("GstRk3AMode", modes_3a);
  }
  return rk_3a_mode;
}

void
rk_common_install_rockchip_properties_helper (GObjectClass * gobject_class)
{
  /* common */
  g_object_class_install_property (gobject_class, PROP_INPUT_CROP,
      g_param_spec_string ("input-crop", "input-crop",
          " ", " ", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OUTPUT_CROP,
      g_param_spec_string ("output-crop", "output-crop",
          " ", " ", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /* rga */
  g_object_class_install_property (gobject_class, PROP_OUTPUT_ROTATION,
      g_param_spec_uint ("rotation", "rotation",
          "Output rotation in 90-degree steps", 0, 360, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_HFLIP,
      g_param_spec_boolean ("hflip", "hflip",
          "horizontal flip", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_VFLIP,
      g_param_spec_boolean ("vflip", "vflip",
          "vertical flip", FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_VPU_STRIDE,
      g_param_spec_boolean ("vpu-stride", "vpu stride",
          "Use 4 alignment for input height, to handle VPU buffer correctly.",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /* isp */
  g_object_class_install_property (gobject_class, PROP_3A_MODE,
      g_param_spec_enum ("isp-mode", "ISP 3A mode", " ",
          GST_TYPE_RK_3A_MODE, RK_3A_DISABLE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DISABLE_AUTOCONF,
      g_param_spec_boolean ("disable-autoconf", "disable autoconf",
          " ", FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_XML_FILE,
      g_param_spec_string ("tuning-xml-path", "tuning xml file path",
          " ", " ", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

gboolean
rk_common_set_property_helper (GstRKV4l2Object * v4l2object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  char *string_val = NULL;

  /* common */
  switch (prop_id) {
    case PROP_INPUT_CROP:
      string_val = g_value_dup_string (value);
      __input_string_to_rect (string_val, &v4l2object->input_crop);
      g_free (string_val);
      break;
    case PROP_OUTPUT_CROP:
      string_val = g_value_dup_string (value);
      __input_string_to_rect (string_val, &v4l2object->output_crop);
      g_free (string_val);
      break;
    default:
      break;
  }

  /* rga */
  switch (prop_id) {
    case PROP_OUTPUT_ROTATION:
      v4l2object->rotation = g_value_get_uint (value);
      break;
    case PROP_VFLIP:
      v4l2object->vflip = g_value_get_boolean (value);
      break;
    case PROP_HFLIP:
      v4l2object->hflip = g_value_get_boolean (value);
      break;
    case PROP_VPU_STRIDE:
      v4l2object->vpu_stride = g_value_get_boolean (value);
      break;
    default:
      break;
  }

  /* isp */
  switch (prop_id) {
    case PROP_3A_MODE:
      v4l2object->isp_mode = g_value_get_enum (value);
      break;
    case PROP_DISABLE_AUTOCONF:
      v4l2object->disable_autoconf = g_value_get_boolean (value);
      break;
    case PROP_XML_FILE:
      v4l2object->xml_path = g_value_dup_string (value);
      break;
    default:
      break;
  }

  return TRUE;
}

gboolean
rk_common_get_property_helper (GstRKV4l2Object * v4l2object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  char out[32];

  /* rga */
  switch (prop_id) {
    case PROP_OUTPUT_ROTATION:
      g_value_set_uint (value, v4l2object->rotation);
      break;
    case PROP_VFLIP:
      g_value_set_boolean (value, v4l2object->vflip);
      break;
    case PROP_HFLIP:
      g_value_set_boolean (value, v4l2object->hflip);
      break;
    case PROP_VPU_STRIDE:
      g_value_set_boolean (value, v4l2object->vpu_stride);
      break;
    case PROP_OUTPUT_CROP:
      snprintf (out, 32, "%dx%dx%dx%d",
          v4l2object->output_crop.x, v4l2object->output_crop.y,
          v4l2object->output_crop.w, v4l2object->output_crop.h);
      g_value_set_string (value, out);
      break;
    case PROP_INPUT_CROP:
      snprintf (out, 32, "%dx%dx%dx%d",
          v4l2object->input_crop.x, v4l2object->input_crop.y,
          v4l2object->input_crop.w, v4l2object->input_crop.h);
      g_value_set_string (value, out);
      break;
    default:
      break;
  }

  /* isp */
  switch (prop_id) {
    case PROP_3A_MODE:
      g_value_set_enum (value, v4l2object->isp_mode);
      break;
    case PROP_DISABLE_AUTOCONF:
      g_value_set_boolean (value, v4l2object->disable_autoconf);
      break;
    default:
      break;
  }

  return TRUE;
}

void
rk_common_new (GstRKV4l2Object * v4l2object)
{
  /* common */
  memset (&v4l2object->input_crop, 0, sizeof (GstVideoRectangle));
  memset (&v4l2object->output_crop, 0, sizeof (GstVideoRectangle));

  /* rga */
  v4l2object->vflip = FALSE;
  v4l2object->hflip = FALSE;
  v4l2object->rotation = 0;
  v4l2object->vpu_stride = FALSE;

  /* isp */
  v4l2object->disable_autoconf = FALSE;
  v4l2object->xml_path = "/etc/cam_iq.xml";
}
