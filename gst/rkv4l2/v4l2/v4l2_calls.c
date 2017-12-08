/* GStreamer
 *
 * Copyright (C) 2002 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *               2006 Edgard Lima <edgard.lima@gmail.com>
 *
 * v4l2_calls.c - generic V4L2 calls handling
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
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "v4l2_calls.h"

#include "rkcamsrc.h"

#include "gst/gst-i18n-plugin.h"

GST_DEBUG_CATEGORY_EXTERN (v4l2_debug);
#define GST_CAT_DEFAULT v4l2_debug

/******************************************************
 * gst_v4l2_get_capabilities():
 *   get the device's capturing capabilities
 * return value: TRUE on success, FALSE on error
 ******************************************************/
gboolean
gst_v4l2_get_capabilities (GstRKV4l2Object * v4l2object)
{
  GstElement *e;

  e = v4l2object->element;

  GST_DEBUG_OBJECT (e, "getting capabilities");

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (v4l2_ioctl (v4l2object->video_fd, VIDIOC_QUERYCAP, &v4l2object->vcap) < 0)
    goto cap_failed;

  if (v4l2object->vcap.capabilities & V4L2_CAP_DEVICE_CAPS)
    v4l2object->device_caps = v4l2object->vcap.device_caps;
  else
    v4l2object->device_caps = v4l2object->vcap.capabilities;

  GST_LOG_OBJECT (e, "driver:      '%s'", v4l2object->vcap.driver);
  GST_LOG_OBJECT (e, "card:        '%s'", v4l2object->vcap.card);
  GST_LOG_OBJECT (e, "bus_info:    '%s'", v4l2object->vcap.bus_info);
  GST_LOG_OBJECT (e, "version:     %08x", v4l2object->vcap.version);
  GST_LOG_OBJECT (e, "capabilites: %08x", v4l2object->device_caps);

  return TRUE;

  /* ERRORS */
cap_failed:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, SETTINGS,
        (_("Error getting capabilities for device '%s': "
                "It isn't a v4l2 driver. Check if it is a v4l1 driver."),
            v4l2object->videodev), GST_ERROR_SYSTEM);
    return FALSE;
  }
}

/******************************************************
 * The video4linux command line tool v4l2-ctrl
 * normalises the names of the controls received from
 * the kernel like:
 *
 *     "Exposure (absolute)" -> "exposure_absolute"
 *
 * We follow their lead here.  @name is modified
 * in-place.
 ******************************************************/
static void
gst_v4l2_normalise_control_name (gchar * name)
{
  int i, j;
  for (i = 0, j = 0; name[j]; ++j) {
    if (g_ascii_isalnum (name[j])) {
      if (i > 0 && !g_ascii_isalnum (name[j - 1]))
        name[i++] = '_';
      name[i++] = g_ascii_tolower (name[j]);
    }
  }
  name[i++] = '\0';
}

/******************************************************
 * gst_v4l2_empty_lists() and gst_v4l2_fill_lists():
 *   fill/empty the lists of enumerations
 * return value: TRUE on success, FALSE on error
 ******************************************************/
static gboolean
gst_v4l2_fill_lists (GstRKV4l2Object * v4l2object)
{
  gint n, next;
  struct v4l2_queryctrl control = { 0, };

  GstElement *e;

  e = v4l2object->element;

  GST_DEBUG_OBJECT (e, "getting enumerations");
  GST_V4L2_CHECK_OPEN (v4l2object);

  /* and lastly, controls+menus (if appropriate) */
  next = V4L2_CTRL_FLAG_NEXT_CTRL;
  n = 0;
  control.id = next;

  while (TRUE) {
    if (!next)
      n++;

  retry:
    /* when we reached the last official CID, continue with private CIDs */
    if (n == V4L2_CID_LASTP1) {
      GST_DEBUG_OBJECT (e, "checking private CIDs");
      n = V4L2_CID_PRIVATE_BASE;
    }
    GST_DEBUG_OBJECT (e, "checking control %08x", n);

    control.id = n | next;
    if (v4l2_ioctl (v4l2object->video_fd, VIDIOC_QUERYCTRL, &control) < 0) {
      if (next) {
        if (n > 0) {
          GST_DEBUG_OBJECT (e, "controls finished");
          break;
        } else {
          GST_DEBUG_OBJECT (e, "V4L2_CTRL_FLAG_NEXT_CTRL not supported.");
          next = 0;
          n = V4L2_CID_BASE;
          goto retry;
        }
      }
      if (errno == EINVAL || errno == ENOTTY || errno == EIO || errno == ENOENT) {
        if (n < V4L2_CID_PRIVATE_BASE) {
          GST_DEBUG_OBJECT (e, "skipping control %08x", n);
          /* continue so that we also check private controls */
          n = V4L2_CID_PRIVATE_BASE - 1;
          continue;
        } else {
          GST_DEBUG_OBJECT (e, "controls finished");
          break;
        }
      } else {
        GST_WARNING_OBJECT (e, "Failed querying control %d on device '%s'. "
            "(%d - %s)", n, v4l2object->videodev, errno, strerror (errno));
        continue;
      }
    }
    /* bogus driver might mess with id in unexpected ways (e.g. set to 0), so
     * make sure to simply try all if V4L2_CTRL_FLAG_NEXT_CTRL not supported */
    if (next)
      n = control.id;
    if (control.flags & V4L2_CTRL_FLAG_DISABLED) {
      GST_DEBUG_OBJECT (e, "skipping disabled control");
      continue;
    }

    if (control.type == V4L2_CTRL_TYPE_CTRL_CLASS) {
      GST_DEBUG_OBJECT (e, "starting control class '%s'", control.name);
      continue;
    }
  }

  GST_DEBUG_OBJECT (e, "done");
  return TRUE;
}


static void
gst_v4l2_empty_lists (GstRKV4l2Object * v4l2object)
{
  GST_DEBUG_OBJECT (v4l2object->element, "deleting enumerations");

  g_datalist_clear (&v4l2object->controls);
}

static void
gst_v4l2_adjust_buf_type (GstRKV4l2Object * v4l2object)
{
  /* when calling gst_v4l2_object_new the user decides the initial type
   * so adjust it if multi-planar is supported
   * the driver should make it exclusive. So the driver should
   * not support both MPLANE and non-PLANE.
   * Because even when using MPLANE it still possibles to use it
   * in a contiguous manner. In this case the first v4l2 plane
   * contains all the gst planes.
   */
  switch (v4l2object->type) {
    case V4L2_BUF_TYPE_VIDEO_OUTPUT:
      if (v4l2object->device_caps &
          (V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_VIDEO_M2M_MPLANE)) {
        GST_DEBUG ("adjust type to multi-planar output");
        v4l2object->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
      }
      break;
    case V4L2_BUF_TYPE_VIDEO_CAPTURE:
      if (v4l2object->device_caps &
          (V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_M2M_MPLANE)) {
        GST_DEBUG ("adjust type to multi-planar capture");
        v4l2object->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
      }
      break;
    default:
      break;
  }
}

/******************************************************
 * gst_v4l2_open():
 *   open the video device (v4l2object->videodev)
 * return value: TRUE on success, FALSE on error
 ******************************************************/
gboolean
gst_v4l2_open (GstRKV4l2Object * v4l2object)
{
  struct stat st;
  int libv4l2_fd;

  GST_DEBUG_OBJECT (v4l2object->element, "Trying to open device %s",
      v4l2object->videodev);

  GST_V4L2_CHECK_NOT_OPEN (v4l2object);
  GST_V4L2_CHECK_NOT_ACTIVE (v4l2object);

  /* be sure we have a device */
  if (!v4l2object->videodev)
    v4l2object->videodev = g_strdup ("/dev/video");

  /* check if it is a device */
  if (stat (v4l2object->videodev, &st) == -1)
    goto stat_failed;

  if (!S_ISCHR (st.st_mode))
    goto no_device;

  /* open the device */
  v4l2object->video_fd =
      open (v4l2object->videodev, O_RDWR /* | O_NONBLOCK */ );

  if (!GST_V4L2_IS_OPEN (v4l2object))
    goto not_open;

  libv4l2_fd = v4l2_fd_open (v4l2object->video_fd,
      V4L2_ENABLE_ENUM_FMT_EMULATION);
  /* Note the v4l2_xxx functions are designed so that if they get passed an
     unknown fd, the will behave exactly as their regular xxx counterparts, so
     if v4l2_fd_open fails, we continue as normal (missing the libv4l2 custom
     cam format to normal formats conversion). Chances are big we will still
     fail then though, as normally v4l2_fd_open only fails if the device is not
     a v4l2 device. */
  if (libv4l2_fd != -1)
    v4l2object->video_fd = libv4l2_fd;

  /* get capabilities, error will be posted */
  if (!gst_v4l2_get_capabilities (v4l2object))
    goto error;

  /* do we need to be a capture device? */
  if (GST_IS_RKCAMSRC (v4l2object->element) &&
      !(v4l2object->device_caps & (V4L2_CAP_VIDEO_CAPTURE |
              V4L2_CAP_VIDEO_CAPTURE_MPLANE)))
    goto not_capture;

  gst_v4l2_adjust_buf_type (v4l2object);

  /* create enumerations, posts errors. */
  if (!gst_v4l2_fill_lists (v4l2object))
    goto error;

  GST_INFO_OBJECT (v4l2object->element,
      "Opened device '%s' (%s) successfully",
      v4l2object->vcap.card, v4l2object->videodev);

  if (v4l2object->extra_controls)
    gst_v4l2_set_controls (v4l2object, v4l2object->extra_controls);

  /* UVC devices are never interlaced, and doing VIDIOC_TRY_FMT on them
   * causes expensive and slow USB IO, so don't probe them for interlaced
   */
  if (!strcmp ((char *) v4l2object->vcap.driver, "uvcusb")) {
    v4l2object->never_interlaced = TRUE;
  }

  return TRUE;

  /* ERRORS */
stat_failed:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, NOT_FOUND,
        (_("Cannot identify device '%s'."), v4l2object->videodev),
        GST_ERROR_SYSTEM);
    goto error;
  }
no_device:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, NOT_FOUND,
        (_("This isn't a device '%s'."), v4l2object->videodev),
        GST_ERROR_SYSTEM);
    goto error;
  }
not_open:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, OPEN_READ_WRITE,
        (_("Could not open device '%s' for reading and writing."),
            v4l2object->videodev), GST_ERROR_SYSTEM);
    goto error;
  }
not_capture:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, NOT_FOUND,
        (_("Device '%s' is not a capture device."),
            v4l2object->videodev),
        ("Capabilities: 0x%x", v4l2object->device_caps));
    goto error;
  }
error:
  {
    if (GST_V4L2_IS_OPEN (v4l2object)) {
      /* close device */
      v4l2_close (v4l2object->video_fd);
      v4l2object->video_fd = -1;
    }
    /* empty lists */
    gst_v4l2_empty_lists (v4l2object);

    return FALSE;
  }
}

gboolean
gst_v4l2_dup (GstRKV4l2Object * v4l2object, GstRKV4l2Object * other)
{
  GST_DEBUG_OBJECT (v4l2object->element, "Trying to dup device %s",
      other->videodev);

  GST_V4L2_CHECK_OPEN (other);
  GST_V4L2_CHECK_NOT_OPEN (v4l2object);
  GST_V4L2_CHECK_NOT_ACTIVE (other);
  GST_V4L2_CHECK_NOT_ACTIVE (v4l2object);

  v4l2object->vcap = other->vcap;
  v4l2object->device_caps = other->device_caps;
  gst_v4l2_adjust_buf_type (v4l2object);

  v4l2object->video_fd = v4l2_dup (other->video_fd);
  if (!GST_V4L2_IS_OPEN (v4l2object))
    goto not_open;

  g_free (v4l2object->videodev);
  v4l2object->videodev = g_strdup (other->videodev);

  GST_INFO_OBJECT (v4l2object->element,
      "Cloned device '%s' (%s) successfully",
      v4l2object->vcap.card, v4l2object->videodev);

  v4l2object->never_interlaced = other->never_interlaced;

  return TRUE;

not_open:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, OPEN_READ_WRITE,
        (_("Could not dup device '%s' for reading and writing."),
            v4l2object->videodev), GST_ERROR_SYSTEM);

    return FALSE;
  }
}


/******************************************************
 * gst_v4l2_close():
 *   close the video device (v4l2object->video_fd)
 * return value: TRUE on success, FALSE on error
 ******************************************************/
gboolean
gst_v4l2_close (GstRKV4l2Object * v4l2object)
{
  GST_DEBUG_OBJECT (v4l2object->element, "Trying to close %s",
      v4l2object->videodev);

  GST_V4L2_CHECK_OPEN (v4l2object);
  GST_V4L2_CHECK_NOT_ACTIVE (v4l2object);

  /* close device */
  v4l2_close (v4l2object->video_fd);
  v4l2object->video_fd = -1;

  /* empty lists */
  gst_v4l2_empty_lists (v4l2object);

  return TRUE;
}

/******************************************************
 * gst_v4l2_get_attribute():
 *   try to get the value of one specific attribute
 * return value: TRUE on success, FALSE on error
 ******************************************************/
gboolean
gst_v4l2_get_attribute (GstRKV4l2Object * v4l2object,
    int attribute_num, int *value)
{
  struct v4l2_control control = { 0, };

  GST_DEBUG_OBJECT (v4l2object->element, "getting value of attribute %d",
      attribute_num);

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  control.id = attribute_num;

  if (v4l2_ioctl (v4l2object->video_fd, VIDIOC_G_CTRL, &control) < 0)
    goto ctrl_failed;

  *value = control.value;

  return TRUE;

  /* ERRORS */
ctrl_failed:
  {
    GST_WARNING_OBJECT (v4l2object,
        _("Failed to get value for control %d on device '%s'."),
        attribute_num, v4l2object->videodev);
    return FALSE;
  }
}


/******************************************************
 * gst_v4l2_set_attribute():
 *   try to set the value of one specific attribute
 * return value: TRUE on success, FALSE on error
 ******************************************************/
gboolean
gst_v4l2_set_attribute (GstRKV4l2Object * v4l2object,
    int attribute_num, const int value)
{
  struct v4l2_control control = { 0, };

  GST_DEBUG_OBJECT (v4l2object->element, "setting value of attribute %d to %d",
      attribute_num, value);

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  control.id = attribute_num;
  control.value = value;
  if (v4l2_ioctl (v4l2object->video_fd, VIDIOC_S_CTRL, &control) < 0)
    goto ctrl_failed;

  return TRUE;

  /* ERRORS */
ctrl_failed:
  {
    GST_WARNING_OBJECT (v4l2object,
        _("Failed to set value %d for control %d on device '%s'."),
        value, attribute_num, v4l2object->videodev);
    return FALSE;
  }
}

static gboolean
set_control (GQuark field_id, const GValue * value, gpointer user_data)
{
  GstRKV4l2Object *v4l2object = user_data;
  GQuark normalised_field_id;
  gpointer *d;

  /* 32 bytes is the maximum size for a control name according to v4l2 */
  gchar name[32];

  /* Backwards compatibility: in the past GStreamer would normalise strings in
     a subtly different way to v4l2-ctl.  e.g. the kernel's "Focus (absolute)"
     would become "focus__absolute_" whereas now it becomes "focus_absolute".
     Please remove the following in GStreamer 1.5 for 1.6 */
  strncpy (name, g_quark_to_string (field_id), sizeof (name));
  name[31] = '\0';
  gst_v4l2_normalise_control_name (name);
  normalised_field_id = g_quark_from_string (name);
  if (normalised_field_id != field_id)
    g_warning ("In GStreamer 1.4 the way V4L2 control names were normalised "
        "changed.  Instead of setting \"%s\" please use \"%s\".  The former is "
        "deprecated and will be removed in a future version of GStreamer",
        g_quark_to_string (field_id), name);
  field_id = normalised_field_id;

  d = g_datalist_id_get_data (&v4l2object->controls, field_id);
  if (!d) {
    GST_WARNING_OBJECT (v4l2object,
        "Control '%s' does not exist or has an unsupported type.",
        g_quark_to_string (field_id));
    return TRUE;
  }
  if (!G_VALUE_HOLDS (value, G_TYPE_INT)) {
    GST_WARNING_OBJECT (v4l2object,
        "'int' value expected for control '%s'.", g_quark_to_string (field_id));
    return TRUE;
  }
  gst_v4l2_set_attribute (v4l2object, GPOINTER_TO_INT (d),
      g_value_get_int (value));
  return TRUE;
}

gboolean
gst_v4l2_set_controls (GstRKV4l2Object * v4l2object, GstStructure * controls)
{
  return gst_structure_foreach (controls, set_control, v4l2object);
}

gboolean
gst_v4l2_get_input (GstRKV4l2Object * v4l2object, gint * input)
{
  gint n;

  GST_DEBUG_OBJECT (v4l2object->element, "trying to get input");

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (v4l2_ioctl (v4l2object->video_fd, VIDIOC_G_INPUT, &n) < 0)
    goto input_failed;

  *input = n;

  GST_DEBUG_OBJECT (v4l2object->element, "input: %d", n);

  return TRUE;

  /* ERRORS */
input_failed:
  if (v4l2object->device_caps & V4L2_CAP_TUNER) {
    /* only give a warning message if driver actually claims to have tuner
     * support
     */
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        (_("Failed to get current input on device '%s'. May be it is a radio device"), v4l2object->videodev), GST_ERROR_SYSTEM);
  }
  return FALSE;
}

gboolean
gst_v4l2_set_input (GstRKV4l2Object * v4l2object, gint input)
{
  GST_DEBUG_OBJECT (v4l2object->element, "trying to set input to %d", input);

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (v4l2_ioctl (v4l2object->video_fd, VIDIOC_S_INPUT, &input) < 0)
    goto input_failed;

  return TRUE;

  /* ERRORS */
input_failed:
  if (v4l2object->device_caps & V4L2_CAP_TUNER) {
    /* only give a warning message if driver actually claims to have tuner
     * support
     */
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        (_("Failed to set input %d on device %s."),
            input, v4l2object->videodev), GST_ERROR_SYSTEM);
  }
  return FALSE;
}

gboolean
gst_v4l2_get_output (GstRKV4l2Object * v4l2object, gint * output)
{
  gint n;

  GST_DEBUG_OBJECT (v4l2object->element, "trying to get output");

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (v4l2_ioctl (v4l2object->video_fd, VIDIOC_G_OUTPUT, &n) < 0)
    goto output_failed;

  *output = n;

  GST_DEBUG_OBJECT (v4l2object->element, "output: %d", n);

  return TRUE;

  /* ERRORS */
output_failed:
  if (v4l2object->device_caps & V4L2_CAP_TUNER) {
    /* only give a warning message if driver actually claims to have tuner
     * support
     */
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        (_("Failed to get current output on device '%s'. May be it is a radio device"), v4l2object->videodev), GST_ERROR_SYSTEM);
  }
  return FALSE;
}

gboolean
gst_v4l2_set_output (GstRKV4l2Object * v4l2object, gint output)
{
  GST_DEBUG_OBJECT (v4l2object->element, "trying to set output to %d", output);

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (v4l2_ioctl (v4l2object->video_fd, VIDIOC_S_OUTPUT, &output) < 0)
    goto output_failed;

  return TRUE;

  /* ERRORS */
output_failed:
  if (v4l2object->device_caps & V4L2_CAP_TUNER) {
    /* only give a warning message if driver actually claims to have tuner
     * support
     */
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        (_("Failed to set output %d on device %s."),
            output, v4l2object->videodev), GST_ERROR_SYSTEM);
  }
  return FALSE;
}
