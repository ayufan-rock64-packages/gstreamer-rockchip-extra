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
rk_common_input_string_to_rect (char *input, GstVideoRectangle * rect)
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

      if (ret_name)
        snprintf (ret_name, 32, DEV_PATH "%s", ent->d_name);

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
rk_common_v4l2_set_rotation (GstV4l2Object * v4l2object, gint rotate)
{
  return gst_v4l2_set_attribute (v4l2object, V4L2_CID_ROTATE, rotate);
}

gboolean
rk_common_v4l2_set_vflip (GstV4l2Object * v4l2object, gboolean flip)
{
  return gst_v4l2_set_attribute (v4l2object, V4L2_CID_VFLIP, flip);
}

gboolean
rk_common_v4l2_set_hflip (GstV4l2Object * v4l2object, gboolean flip)
{
  return gst_v4l2_set_attribute (v4l2object, V4L2_CID_HFLIP, flip);
}

gboolean
rk_common_v4l2_set_selection (GstV4l2Object * v4l2object,
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


  /* isp */
  g_object_class_install_property (gobject_class, PROP_DISABLE_3A,
      g_param_spec_boolean ("disable-3A", "disable 3A",
          " ", FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DISABLE_AUTOCONF,
      g_param_spec_boolean ("disable-autoconf", "disable autoconf",
          " ", FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_SENSOR_NAME,
      g_param_spec_uint ("sensor-name", "active sensor name",
          " ", 0, 255, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_VIDEO_COMPOSE,
      g_param_spec_string ("video-compose", "video compose",
          " ", " ", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_VIDEO_CROP,
      g_param_spec_string ("video-crop", "video crop",
          " ", " ", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_SENSOR_CROP,
      g_param_spec_string ("sensor-crop", "sensor-crop",
          " ", " ", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

gboolean
rk_common_set_property_helper (GstV4l2Object * v4l2object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  char *string_val = NULL;

  /* common */
  switch (prop_id) {
    case PROP_INPUT_CROP:
      string_val = g_value_dup_string (value);
      rk_common_input_string_to_rect (string_val, &v4l2object->input_crop);
      g_free (string_val);
      break;
    case PROP_OUTPUT_CROP:
      string_val = g_value_dup_string (value);
      rk_common_input_string_to_rect (string_val, &v4l2object->output_crop);
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
    default:
      break;
  }

  /* isp */
  switch (prop_id) {
    case PROP_DISABLE_3A:
      v4l2object->disable_3A = g_value_get_boolean (value);
      break;
    case PROP_DISABLE_AUTOCONF:
      v4l2object->disable_autoconf = g_value_get_boolean (value);
      break;
    case PROP_SENSOR_NAME:
      v4l2object->sensor_name = g_value_dup_string (value);
      break;
    case PROP_VIDEO_COMPOSE:
      string_val = g_value_dup_string (value);
      rk_common_input_string_to_rect (string_val, &v4l2object->video_compose);
      g_free (string_val);
      break;
    case PROP_VIDEO_CROP:
      string_val = g_value_dup_string (value);
      rk_common_input_string_to_rect (string_val, &v4l2object->video_crop);
      g_free (string_val);
      break;
    case PROP_SENSOR_CROP:
      string_val = g_value_dup_string (value);
      rk_common_input_string_to_rect (string_val, &v4l2object->sensor_crop);
      g_free (string_val);
      break;
    default:
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
    default:
      break;
  }

  /* isp */
  switch (prop_id) {
    case PROP_DISABLE_3A:
      g_value_set_boolean (value, v4l2object->disable_3A);
      break;
    case PROP_DISABLE_AUTOCONF:
      g_value_set_boolean (value, v4l2object->disable_autoconf);
      break;
    case PROP_SENSOR_NAME:
      g_value_set_string (value, v4l2object->sensor_name);
      break;
    case PROP_VIDEO_COMPOSE:
      snprintf (out, 32, "%dx%dx%dx%d",
          v4l2object->video_compose.x, v4l2object->video_compose.y,
          v4l2object->video_compose.w, v4l2object->video_compose.h);
      g_value_set_string (value, out);
      break;
    case PROP_VIDEO_CROP:
      snprintf (out, 32, "%dx%dx%dx%d",
          v4l2object->video_crop.x, v4l2object->video_crop.y,
          v4l2object->video_crop.w, v4l2object->video_crop.h);
      g_value_set_string (value, out);
      break;
    case PROP_SENSOR_CROP:
      snprintf (out, 32, "%dx%dx%dx%d",
          v4l2object->sensor_crop.x, v4l2object->sensor_crop.y,
          v4l2object->sensor_crop.w, v4l2object->sensor_crop.h);
      g_value_set_string (value, out);
      break;
    default:
      break;
  }

  return TRUE;
}

void
rk_common_new (GstV4l2Object * v4l2object)
{
  /* common */
  memset (&v4l2object->input_crop, 0, sizeof (GstVideoRectangle));
  memset (&v4l2object->output_crop, 0, sizeof (GstVideoRectangle));

  /* rga */
  v4l2object->vflip = FALSE;
  v4l2object->hflip = FALSE;
  v4l2object->rotation = 0;

  /* isp */
  v4l2object->disable_3A = TRUE;
  v4l2object->disable_autoconf = FALSE;
  v4l2object->sensor_name = NULL;
  memset (&v4l2object->video_compose, 0, sizeof (GstVideoRectangle));
  memset (&v4l2object->video_crop, 0, sizeof (GstVideoRectangle));
  memset (&v4l2object->sensor_crop, 0, sizeof (GstVideoRectangle));
}

/*
 * media entity
 */

struct
{
  int index;
  struct media_device *device;
} g_media_data[16];

void
rk_common_media_init_global_data (void)
{
  char path[64];
  FILE *fp;
  int i = 0;

  for (i = 0; i < 16; i++) {
    g_media_data[i].index = -1;
  }

  i = 0;
  while (i < 16) {
    snprintf (path, 64, "/dev/media%d", i);

    fp = fopen (path, "r");
    if (!fp)
      break;
    fclose (fp);

    g_media_data[i].index = i;
    g_media_data[i].device = media_device_new (path);

    /* Enumerate entities, pads and links. */
    media_device_enumerate (g_media_data[i].device);

    i++;
  }
}

guint
rk_common_media_find_by_vnode (const char *vnode)
{
  int i, j;

  for (i = 0; i < 16; i++) {
    unsigned int nents;

    if (g_media_data[i].index == -1)
      break;

    nents = media_get_entities_count (g_media_data[i].device);
    for (j = 0; j < nents; ++j) {
      struct media_entity *entity =
          media_get_entity (g_media_data[i].device, j);
      const char *devname = media_entity_get_devname (entity);

      if (!strcmp (devname, vnode))
        return g_media_data[i].index;
    }
  }

  return -1;
}

struct media_entity *
rk_common_media_find_subdev_by_name (guint index, const char *subdev_name)
{
  return media_get_entity_by_name (g_media_data[index].device, subdev_name,
      strlen (subdev_name));
}

struct media_entity *
rk_common_media_get_last_enity (guint index)
{
  struct media_entity *entity;
  int nents;

  nents = media_get_entities_count (g_media_data[index].device);
  entity = media_get_entity (g_media_data[index].device, nents - 1);

  return entity;
}
