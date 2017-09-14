#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rkcommon.h"

#include <gst/gst-i18n-plugin.h>

enum
{
  PROP_0,
  V4L2_STD_OBJECT_PROPS
};

gboolean
rk_common_set_rotation (GstV4l2Object * v4l2object, gint rotate)
{
  return gst_v4l2_set_attribute (v4l2object, V4L2_CID_ROTATE, rotate);
}

gboolean
rk_common_set_vflip (GstV4l2Object * v4l2object, gboolean flip)
{
  return gst_v4l2_set_attribute (v4l2object, V4L2_CID_VFLIP, flip);
}

gboolean
rk_common_set_hflip (GstV4l2Object * v4l2object, gboolean flip)
{
  return gst_v4l2_set_attribute (v4l2object, V4L2_CID_HFLIP, flip);
}

gboolean
rk_common_set_selection (GstV4l2Object * v4l2object, GstVideoRectangle * crop)
{
  struct v4l2_selection s = { 0 };

  s.type = v4l2object->type;
  s.target = v4l2object->type == V4L2_BUF_TYPE_VIDEO_OUTPUT ?
      V4L2_SEL_TGT_CROP : V4L2_SEL_TGT_COMPOSE;
  s.r.left = crop->x;
  s.r.top = crop->y;
  s.r.width = crop->w;
  s.r.height = crop->h;

  GST_DEBUG_OBJECT (v4l2object->element,
      "Desired cropping left %u, top %u, size %ux%u", s.r.left, s.r.top,
      s.r.width, s.r.height);

  if (v4l2_ioctl (v4l2object->video_fd, VIDIOC_S_SELECTION, &s) < 0) {
    GST_WARNING_OBJECT (v4l2object->element, "VIDIOC_S_SELECTION failed");
    return FALSE;
  }

  return TRUE;
}

void
rk_common_setup_attr_before_stream (GstV4l2Object * v4l2object)
{
  rk_common_set_rotation (v4l2object, v4l2object->rotation);
  rk_common_set_vflip (v4l2object, v4l2object->vflip);
  rk_common_set_hflip (v4l2object, v4l2object->hflip);
}

void
rk_common_v4l2device_find_by_name (const char *name, char *ret_name)
{
  DIR *dir;
  struct dirent *ent;

#define SYS_PATH		"/sys/class/video4linux/"
#define DEV_PATH		"/dev/"

  if ((dir = opendir (SYS_PATH)) != NULL) {
    while ((ent = readdir (dir)) != NULL) {
      FILE *fp;
      char path[64];
      char dev_name[64];

      snprintf (path, 64, SYS_PATH "%s/name", ent->d_name);
      fp = fopen (path, "r");
      if (!fp)
        continue;
      if (!fgets (dev_name, 32, fp))
        dev_name[0] = '\0';
      fclose (fp);

      if (!strstr (dev_name, name))
        continue;

      snprintf (ret_name, 32, DEV_PATH "%s", ent->d_name);

      break;
    }
    closedir (dir);
  }

  snprintf (ret_name, 32, "/dev/video0");
}

void
rk_common_install_rockchip_properties_helper (GObjectClass * gobject_class)
{
  /**
   * Rockchip defined
   */
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
  g_object_class_install_property (gobject_class, PROP_INPUT_CROP,
      g_param_spec_string ("input-crop", "input-crop",
          " ", " ", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OUTPUT_CROP,
      g_param_spec_string ("output-crop", "output-crop",
          " ", " ", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

gboolean
rk_common_set_property_helper (GstV4l2Object * v4l2object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  char *string_val = NULL;
  char delims[] = "x";
  char *result = NULL;

  switch (prop_id) {
      /* Rockchip defined */
    case PROP_OUTPUT_ROTATION:
      v4l2object->rotation = g_value_get_uint (value);
      break;
    case PROP_VFLIP:
      v4l2object->vflip = g_value_get_boolean (value);
      break;
    case PROP_HFLIP:
      v4l2object->hflip = g_value_get_boolean (value);
      break;
    case PROP_INPUT_CROP:
      string_val = g_value_dup_string (value);
      result = strtok (string_val, delims);
      v4l2object->input_crop.x = atoi (result);
      result = strtok (NULL, delims);
      v4l2object->input_crop.y = atoi (result);
      result = strtok (NULL, delims);
      v4l2object->input_crop.w = atoi (result);
      result = strtok (NULL, delims);
      v4l2object->input_crop.h = atoi (result);
      v4l2object->enable_selection = TRUE;
      g_free (string_val);
      break;
    case PROP_OUTPUT_CROP:
      string_val = g_value_dup_string (value);
      result = strtok (string_val, delims);
      v4l2object->output_crop.x = atoi (result);
      result = strtok (NULL, delims);
      v4l2object->output_crop.y = atoi (result);
      result = strtok (NULL, delims);
      v4l2object->output_crop.w = atoi (result);
      result = strtok (NULL, delims);
      v4l2object->output_crop.h = atoi (result);
      v4l2object->enable_selection = TRUE;
      g_free (string_val);
      break;
    default:
      return FALSE;
      break;
  }

  return TRUE;
}

gboolean
rk_common_get_property_helper (GstV4l2Object * v4l2object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  char out[32];

  /* common */
  switch (prop_id) {
    case PROP_OUTPUT_ROTATION:
      g_value_set_uint (value, v4l2object->rotation);
      break;
    default:
      break;
  }

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

  return TRUE;
}

void
rk_common_new (GstV4l2Object * v4l2object)
{

  /* common */
  v4l2object->rotation = 0;


  /* rga */
  v4l2object->vflip = FALSE;
  v4l2object->hflip = FALSE;
  v4l2object->input_crop.x = 0;
  v4l2object->input_crop.y = 0;
  v4l2object->input_crop.w = 0;
  v4l2object->input_crop.h = 0;
  v4l2object->output_crop.x = 0;
  v4l2object->output_crop.y = 0;
  v4l2object->output_crop.w = 0;
  v4l2object->output_crop.h = 0;
  v4l2object->enable_selection = FALSE;

  /* isp */


}
