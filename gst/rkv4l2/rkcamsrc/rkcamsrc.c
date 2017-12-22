#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>
#include <gst/video/videoorientation.h>
#include <gst/video/colorbalance.h>

#include "common.h"
#include "rkcamsrc.h"
#include "v4l2_calls.h"

#include "gst/gst-i18n-plugin.h"

GST_DEBUG_CATEGORY (rkcamsrc_debug);
#define GST_CAT_DEFAULT rkcamsrc_debug

#define DEFAULT_PROP_DEVICE   "/dev/video0"

enum
{
  PROP_0,
  V4L2_STD_OBJECT_PROPS,
  PROP_LAST
};

/* signals and args */
enum
{
  SIGNAL_PRE_SET_FORMAT,
  LAST_SIGNAL
};

static guint gst_v4l2_signals[LAST_SIGNAL] = { 0 };

static void gst_rkcamsrc_uri_handler_init (gpointer g_iface,
    gpointer iface_data);

#define gst_rkcamsrc_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstRKCamSrc, gst_rkcamsrc, GST_TYPE_PUSH_SRC,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
        gst_rkcamsrc_uri_handler_init));

static void gst_rkcamsrc_finalize (GstRKCamSrc * rkcamsrc);

/* element methods */
static GstStateChangeReturn gst_rkcamsrc_change_state (GstElement * element,
    GstStateChange transition);

/* basesrc methods */
static gboolean gst_rkcamsrc_start (GstBaseSrc * src);
static gboolean gst_rkcamsrc_unlock (GstBaseSrc * src);
static gboolean gst_rkcamsrc_unlock_stop (GstBaseSrc * src);
static gboolean gst_rkcamsrc_stop (GstBaseSrc * src);
static gboolean gst_rkcamsrc_set_caps (GstBaseSrc * src, GstCaps * caps);
static GstCaps *gst_rkcamsrc_get_caps (GstBaseSrc * src, GstCaps * filter);
static gboolean gst_rkcamsrc_query (GstBaseSrc * bsrc, GstQuery * query);
static gboolean gst_rkcamsrc_decide_allocation (GstBaseSrc * src,
    GstQuery * query);
static GstFlowReturn gst_rkcamsrc_create (GstPushSrc * src, GstBuffer ** out);
static GstCaps *gst_rkcamsrc_fixate (GstBaseSrc * basesrc, GstCaps * caps);
static gboolean gst_rkcamsrc_negotiate (GstBaseSrc * basesrc);

static void gst_rkcamsrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rkcamsrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void
gst_rkcamsrc_class_init (GstRKCamSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstBaseSrcClass *basesrc_class;
  GstPushSrcClass *pushsrc_class;

  gobject_class = G_OBJECT_CLASS (klass);
  element_class = GST_ELEMENT_CLASS (klass);
  basesrc_class = GST_BASE_SRC_CLASS (klass);
  pushsrc_class = GST_PUSH_SRC_CLASS (klass);

  gobject_class->finalize = (GObjectFinalizeFunc) gst_rkcamsrc_finalize;
  gobject_class->set_property = gst_rkcamsrc_set_property;
  gobject_class->get_property = gst_rkcamsrc_get_property;

  element_class->change_state = gst_rkcamsrc_change_state;

  gst_v4l2_object_install_properties_helper (gobject_class,
      DEFAULT_PROP_DEVICE);
  rk_common_install_rockchip_properties_helper (gobject_class);

  /**
   * GstRKCamSrc::prepare-format:
   * @rkcamsrc: the rkcamsrc instance
   * @fd: the file descriptor of the current device
   * @caps: the caps of the format being set
   *
   * This signal gets emitted before calling the v4l2 VIDIOC_S_FMT ioctl
   * (set format). This allows for any custom configuration of the device to
   * happen prior to the format being set.
   * This is mostly useful for UVC H264 encoding cameras which need the H264
   * Probe & Commit to happen prior to the normal Probe & Commit.
   *
   * Since: 0.10.32
   */
  gst_v4l2_signals[SIGNAL_PRE_SET_FORMAT] = g_signal_new ("prepare-format",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_INT, GST_TYPE_CAPS);

  gst_element_class_set_static_metadata (element_class,
      "ISP Source", "Source/Video", "Reads frames from ISP", " ");

  gst_element_class_add_pad_template
      (element_class,
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
          gst_v4l2_object_get_all_caps ()));

  basesrc_class->get_caps = GST_DEBUG_FUNCPTR (gst_rkcamsrc_get_caps);
  basesrc_class->set_caps = GST_DEBUG_FUNCPTR (gst_rkcamsrc_set_caps);
  basesrc_class->start = GST_DEBUG_FUNCPTR (gst_rkcamsrc_start);
  basesrc_class->unlock = GST_DEBUG_FUNCPTR (gst_rkcamsrc_unlock);
  basesrc_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_rkcamsrc_unlock_stop);
  basesrc_class->stop = GST_DEBUG_FUNCPTR (gst_rkcamsrc_stop);
  basesrc_class->query = GST_DEBUG_FUNCPTR (gst_rkcamsrc_query);
  basesrc_class->fixate = GST_DEBUG_FUNCPTR (gst_rkcamsrc_fixate);
  basesrc_class->negotiate = GST_DEBUG_FUNCPTR (gst_rkcamsrc_negotiate);
  basesrc_class->decide_allocation =
      GST_DEBUG_FUNCPTR (gst_rkcamsrc_decide_allocation);

  pushsrc_class->create = GST_DEBUG_FUNCPTR (gst_rkcamsrc_create);

  klass->v4l2_class_devices = NULL;

  GST_DEBUG_CATEGORY_INIT (rkcamsrc_debug, "rkcamsrc", 0,
      "ISP source element(Rockchip)");
}

static void
gst_rkcamsrc_init (GstRKCamSrc * rkcamsrc)
{
  rkcamsrc->v4l2object = gst_v4l2_object_new (GST_ELEMENT (rkcamsrc),
      V4L2_BUF_TYPE_VIDEO_CAPTURE, DEFAULT_PROP_DEVICE,
      gst_v4l2_get_input, gst_v4l2_set_input, NULL);

  gst_base_src_set_format (GST_BASE_SRC (rkcamsrc), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (rkcamsrc), TRUE);
}


static void
gst_rkcamsrc_finalize (GstRKCamSrc * rkcamsrc)
{
  gst_v4l2_object_destroy (rkcamsrc->v4l2object);

  G_OBJECT_CLASS (parent_class)->finalize ((GObject *) (rkcamsrc));
}


static void
gst_rkcamsrc_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstRKCamSrc *rkcamsrc = GST_RKCAMSRC (object);

  if (!gst_v4l2_object_set_property_helper (rkcamsrc->v4l2object,
          prop_id, value, pspec)) {
    if (!rk_common_set_property_helper (rkcamsrc->v4l2object,
            prop_id, value, pspec)) {
      switch (prop_id) {
        default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
          break;
      }
    }
  }
}

static void
gst_rkcamsrc_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstRKCamSrc *rkcamsrc = GST_RKCAMSRC (object);

  if (!gst_v4l2_object_get_property_helper (rkcamsrc->v4l2object,
          prop_id, value, pspec)) {
    if (!rk_common_get_property_helper (rkcamsrc->v4l2object,
            prop_id, value, pspec)) {
      switch (prop_id) {
        default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
          break;
      }
    }
  }
}

/* this function is a bit of a last resort */
static GstCaps *
gst_rkcamsrc_fixate (GstBaseSrc * basesrc, GstCaps * caps)
{
  GstStructure *structure;
  gint i;

  GST_DEBUG_OBJECT (basesrc, "fixating caps %" GST_PTR_FORMAT, caps);

  caps = gst_caps_make_writable (caps);

  for (i = 0; i < gst_caps_get_size (caps); ++i) {
    structure = gst_caps_get_structure (caps, i);

    /* We are fixating to a reasonable 320x200 resolution
       and the maximum framerate resolution for that size */
    if (gst_structure_has_field (structure, "width"))
      gst_structure_fixate_field_nearest_int (structure, "width", 320);

    if (gst_structure_has_field (structure, "height"))
      gst_structure_fixate_field_nearest_int (structure, "height", 200);

    if (gst_structure_has_field (structure, "framerate"))
      gst_structure_fixate_field_nearest_fraction (structure, "framerate",
          100, 1);

    if (gst_structure_has_field (structure, "format"))
      gst_structure_fixate_field (structure, "format");

    if (gst_structure_has_field (structure, "interlace-mode"))
      gst_structure_fixate_field (structure, "interlace-mode");
  }

  GST_DEBUG_OBJECT (basesrc, "fixated caps %" GST_PTR_FORMAT, caps);

  caps = GST_BASE_SRC_CLASS (parent_class)->fixate (basesrc, caps);

  return caps;
}


static gboolean
gst_rkcamsrc_negotiate (GstBaseSrc * basesrc)
{
  GstCaps *thiscaps;
  GstCaps *caps = NULL;
  GstCaps *peercaps = NULL;
  gboolean result = FALSE;

  /* first see what is possible on our source pad */
  thiscaps = gst_pad_query_caps (GST_BASE_SRC_PAD (basesrc), NULL);
  GST_DEBUG_OBJECT (basesrc, "caps of src: %" GST_PTR_FORMAT, thiscaps);

  /* nothing or anything is allowed, we're done */
  if (thiscaps == NULL || gst_caps_is_any (thiscaps))
    goto no_nego_needed;

  /* get the peer caps without a filter as we'll filter ourselves later on */
  peercaps = gst_pad_peer_query_caps (GST_BASE_SRC_PAD (basesrc), NULL);
  GST_DEBUG_OBJECT (basesrc, "caps of peer: %" GST_PTR_FORMAT, peercaps);
  if (peercaps && !gst_caps_is_any (peercaps)) {
    GstCaps *icaps = NULL;

    /* Prefer the first caps we are compatible with that the peer proposed */
    icaps = gst_caps_intersect_full (peercaps, thiscaps,
        GST_CAPS_INTERSECT_FIRST);

    GST_DEBUG_OBJECT (basesrc, "intersect: %" GST_PTR_FORMAT, icaps);
    if (icaps) {
      /* If there are multiple intersections pick the one with the smallest
       * resolution strictly bigger then the first peer caps */
      if (gst_caps_get_size (icaps) > 1) {
        GstStructure *s = gst_caps_get_structure (peercaps, 0);
        int best = 0;
        int twidth, theight;
        int width = G_MAXINT, height = G_MAXINT;

        if (gst_structure_get_int (s, "width", &twidth)
            && gst_structure_get_int (s, "height", &theight)) {
          int i;

          /* Walk the structure backwards to get the first entry of the
           * smallest resolution bigger (or equal to) the preferred resolution)
           */
          for (i = gst_caps_get_size (icaps) - 1; i >= 0; i--) {
            GstStructure *is = gst_caps_get_structure (icaps, i);
            int w, h;

            if (gst_structure_get_int (is, "width", &w)
                && gst_structure_get_int (is, "height", &h)) {
              if (w >= twidth && w <= width && h >= theight && h <= height) {
                width = w;
                height = h;
                best = i;
              }
            }
          }
        }

        caps = gst_caps_copy_nth (icaps, best);
        gst_caps_unref (icaps);
      } else {
        caps = icaps;
      }
    }
    gst_caps_unref (thiscaps);
  } else {
    /* no peer or peer have ANY caps, work with our own caps then */
    caps = thiscaps;
  }
  if (peercaps)
    gst_caps_unref (peercaps);
  if (caps) {
    caps = gst_caps_truncate (caps);

    /* now fixate */
    if (!gst_caps_is_empty (caps)) {
      caps = gst_rkcamsrc_fixate (basesrc, caps);
      GST_DEBUG_OBJECT (basesrc, "fixated to: %" GST_PTR_FORMAT, caps);

      if (gst_caps_is_any (caps)) {
        /* hmm, still anything, so element can do anything and
         * nego is not needed */
        result = TRUE;
      } else if (gst_caps_is_fixed (caps)) {
        /* yay, fixed caps, use those then */
        result = gst_base_src_set_caps (basesrc, caps);
      }
    }
    gst_caps_unref (caps);
  }
  return result;

no_nego_needed:
  {
    GST_DEBUG_OBJECT (basesrc, "no negotiation needed");
    if (thiscaps)
      gst_caps_unref (thiscaps);
    return TRUE;
  }
}

static GstCaps *
gst_rkcamsrc_get_caps (GstBaseSrc * src, GstCaps * filter)
{
  GstRKCamSrc *isps;
  GstRKV4l2Object *obj;

  isps = GST_RKCAMSRC (src);
  obj = isps->v4l2object;

  if (!GST_V4L2_IS_OPEN (obj)) {
    return gst_pad_get_pad_template_caps (GST_BASE_SRC_PAD (isps));
  }

  return gst_v4l2_object_get_caps (obj, filter);
}

static void
gst_rkcamsrc_set_crop_window (GstRKCamSrc * rkcamsrc)
{
  struct v4l2_rect rect;

  if (rkcamsrc->v4l2object->disable_autoconf)
    return;

  /* input crop */
  v4l2_subdev_get_selection (rkcamsrc->isp_subdev, &rect,
      RKISP1_ISP_PAD_SINK, V4L2_SEL_TGT_CROP_BOUNDS, V4L2_SUBDEV_FORMAT_ACTIVE);
  if (rkcamsrc->v4l2object->input_crop.w != 0) {
    if (rect.width <
        rkcamsrc->v4l2object->input_crop.w + rkcamsrc->v4l2object->input_crop.x
        || rect.height <
        rkcamsrc->v4l2object->input_crop.h + rkcamsrc->v4l2object->input_crop.y)
      printf ("ERROR: input crop is bigger than sensor image!\n");
    else
      gst_rect_to_v4l2_rect (&rkcamsrc->v4l2object->input_crop, &rect);

    v4l2_subdev_set_selection (rkcamsrc->isp_subdev, &rect, RKISP1_ISP_PAD_SINK,
        V4L2_SEL_TGT_CROP, V4L2_SUBDEV_FORMAT_ACTIVE);
  } else {
    v4l2_subdev_set_selection (rkcamsrc->isp_subdev, &rect, RKISP1_ISP_PAD_SINK,
        V4L2_SEL_TGT_CROP, V4L2_SUBDEV_FORMAT_ACTIVE);
  }

  /* output crop */
  v4l2_subdev_get_selection (rkcamsrc->isp_subdev, &rect,
      RKISP1_ISP_PAD_SOURCE_PATH, V4L2_SEL_TGT_CROP_BOUNDS,
      V4L2_SUBDEV_FORMAT_ACTIVE);
  v4l2_subdev_set_selection (rkcamsrc->isp_subdev, &rect,
      RKISP1_ISP_PAD_SOURCE_PATH, V4L2_SEL_TGT_CROP, V4L2_SUBDEV_FORMAT_ACTIVE);

  /* video_crop */
  v4l2_subdev_get_selection (rkcamsrc->isp_subdev, &rect,
      RKISP1_ISP_PAD_SINK, V4L2_SEL_TGT_CROP_BOUNDS, V4L2_SUBDEV_FORMAT_ACTIVE);
  rk_common_v4l2_set_selection (rkcamsrc->v4l2object, &rect, FALSE);

  /* video_compose */
  v4l2_subdev_get_selection (rkcamsrc->isp_subdev, &rect,
      RKISP1_ISP_PAD_SINK, V4L2_SEL_TGT_COMPOSE_BOUNDS,
      V4L2_SUBDEV_FORMAT_ACTIVE);
  rk_common_v4l2_set_selection (rkcamsrc->v4l2object, &rect, TRUE);
}

static void
gst_rkcamsrc_set_pad_format (GstRKCamSrc * rkcamsrc)
{
  struct v4l2_mbus_framefmt format;

  if (rkcamsrc->v4l2object->disable_autoconf)
    return;

  /* read format from sensor */
  v4l2_subdev_get_format (rkcamsrc->sensor_subdev, &format, 0,
      V4L2_SUBDEV_FORMAT_ACTIVE);

  /* propagate to dphy */
  v4l2_subdev_set_format (rkcamsrc->phy_subdev, &format, MIPI_DPHY_SY_PAD_SINK,
      V4L2_SUBDEV_FORMAT_ACTIVE);
  v4l2_subdev_get_format (rkcamsrc->phy_subdev, &format,
      MIPI_DPHY_SY_PAD_SOURCE, V4L2_SUBDEV_FORMAT_ACTIVE);

  /* propagate to rkisp */
  v4l2_subdev_set_format (rkcamsrc->isp_subdev, &format,
      RKISP1_ISP_PAD_SINK, V4L2_SUBDEV_FORMAT_ACTIVE);
  format.code = MEDIA_BUS_FMT_YUYV8_2X8;
  v4l2_subdev_get_format (rkcamsrc->isp_subdev, &format,
      RKISP1_ISP_PAD_SOURCE_PATH, V4L2_SUBDEV_FORMAT_ACTIVE);
}

static gboolean
gst_rkcamsrc_set_format (GstRKCamSrc * rkcamsrc, GstCaps * caps)
{
  GstV4l2Error error = GST_V4L2_ERROR_INIT;
  GstRKV4l2Object *obj;

  obj = rkcamsrc->v4l2object;

  g_signal_emit (rkcamsrc, gst_v4l2_signals[SIGNAL_PRE_SET_FORMAT], 0,
      rkcamsrc->v4l2object->video_fd, caps);

  if (!gst_v4l2_object_set_format (obj, caps, &error)) {
    gst_v4l2_error (rkcamsrc, &error);
    return FALSE;
  }

  gst_rkcamsrc_set_pad_format (rkcamsrc);
  gst_rkcamsrc_set_crop_window (rkcamsrc);

  return TRUE;
}

static gboolean
gst_rkcamsrc_set_caps (GstBaseSrc * src, GstCaps * caps)
{
  GstRKCamSrc *rkcamsrc;
  GstRKV4l2Object *obj;

  rkcamsrc = GST_RKCAMSRC (src);
  obj = rkcamsrc->v4l2object;

  /* make sure the caps changed before doing anything */
  if (gst_v4l2_object_caps_equal (obj, caps))
    return TRUE;

  if (GST_V4L2_IS_ACTIVE (obj)) {
    GstV4l2Error error = GST_V4L2_ERROR_INIT;
    /* Just check if the format is acceptable, once we know
     * no buffers should be outstanding we try S_FMT.
     *
     * Basesrc will do an allocation query that
     * should indirectly reclaim buffers, after that we can
     * set the format and then configure our pool */
    if (gst_v4l2_object_try_format (obj, caps, &error)) {
      rkcamsrc->renegotiation_adjust = rkcamsrc->offset + 1;
      rkcamsrc->pending_set_fmt = TRUE;
    } else {
      gst_v4l2_error (rkcamsrc, &error);
      return FALSE;
    }
  } else {
    /* make sure we stop capturing and dealloc buffers */
    if (!gst_v4l2_object_stop (obj))
      return FALSE;

    return gst_rkcamsrc_set_format (rkcamsrc, caps);
  }

  return TRUE;
}

static gboolean
gst_rkcamsrc_decide_allocation (GstBaseSrc * bsrc, GstQuery * query)
{
  GstRKCamSrc *src = GST_RKCAMSRC (bsrc);
  gboolean ret = TRUE;

  if (src->pending_set_fmt) {
    GstCaps *caps = gst_pad_get_current_caps (GST_BASE_SRC_PAD (bsrc));

    if (!gst_v4l2_object_stop (src->v4l2object))
      return FALSE;
    ret = gst_rkcamsrc_set_format (src, caps);
    gst_caps_unref (caps);
    src->pending_set_fmt = FALSE;
  } else if (gst_buffer_pool_is_active (src->v4l2object->pool)) {
    /* Trick basesrc into not deactivating the active pool. Renegotiating here
     * would otherwise turn off and on the camera. */
    GstAllocator *allocator;
    GstAllocationParams params;
    GstBufferPool *pool;

    gst_base_src_get_allocator (bsrc, &allocator, &params);
    pool = gst_base_src_get_buffer_pool (bsrc);

    if (gst_query_get_n_allocation_params (query))
      gst_query_set_nth_allocation_param (query, 0, allocator, &params);
    else
      gst_query_add_allocation_param (query, allocator, &params);

    if (gst_query_get_n_allocation_pools (query))
      gst_query_set_nth_allocation_pool (query, 0, pool,
          src->v4l2object->info.size, 1, 0);
    else
      gst_query_add_allocation_pool (query, pool, src->v4l2object->info.size, 1,
          0);

    if (pool)
      gst_object_unref (pool);
    if (allocator)
      gst_object_unref (allocator);

    return GST_BASE_SRC_CLASS (parent_class)->decide_allocation (bsrc, query);
  }

  if (ret) {
    ret = gst_v4l2_object_decide_allocation (src->v4l2object, query);
    if (ret)
      ret = GST_BASE_SRC_CLASS (parent_class)->decide_allocation (bsrc, query);
  }

  if (ret) {
    if (!gst_buffer_pool_set_active (src->v4l2object->pool, TRUE))
      goto activate_failed;
  }

  return ret;

activate_failed:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, SETTINGS,
        (_("Failed to allocate required memory.")),
        ("Buffer pool activation failed"));
    return FALSE;
  }
}

static gboolean
gst_rkcamsrc_query (GstBaseSrc * bsrc, GstQuery * query)
{
  GstRKCamSrc *src;
  GstRKV4l2Object *obj;
  gboolean res = FALSE;

  src = GST_RKCAMSRC (bsrc);
  obj = src->v4l2object;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_LATENCY:{
      GstClockTime min_latency, max_latency;
      guint32 fps_n, fps_d;
      guint num_buffers = 0;

      /* device must be open */
      if (!GST_V4L2_IS_OPEN (obj)) {
        GST_WARNING_OBJECT (src,
            "Can't give latency since device isn't open !");
        goto done;
      }

      fps_n = GST_V4L2_FPS_N (obj);
      fps_d = GST_V4L2_FPS_D (obj);

      /* we must have a framerate */
      if (fps_n <= 0 || fps_d <= 0) {
        GST_WARNING_OBJECT (src,
            "Can't give latency since framerate isn't fixated !");
        goto done;
      }

      /* min latency is the time to capture one frame */
      min_latency = gst_util_uint64_scale_int (GST_SECOND, fps_d, fps_n);

      /* max latency is total duration of the frame buffer */
      if (obj->pool != NULL)
        num_buffers = GST_V4L2_BUFFER_POOL_CAST (obj->pool)->max_latency;

      if (num_buffers == 0)
        max_latency = -1;
      else
        max_latency = num_buffers * min_latency;

      GST_DEBUG_OBJECT (bsrc,
          "report latency min %" GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
          GST_TIME_ARGS (min_latency), GST_TIME_ARGS (max_latency));

      /* we are always live, the min latency is 1 frame and the max latency is
       * the complete buffer of frames. */
      gst_query_set_latency (query, TRUE, min_latency, max_latency);

      res = TRUE;
      break;
    }
    default:
      res = GST_BASE_SRC_CLASS (parent_class)->query (bsrc, query);
      break;
  }

done:

  return res;
}

/* start and stop are not symmetric -- start will open the device, but not start
 * capture. it's setcaps that will start capture, which is called via basesrc's
 * negotiate method. stop will both stop capture and close the device.
 */
static gboolean
gst_rkcamsrc_start (GstBaseSrc * src)
{
  GstRKCamSrc *rkcamsrc = GST_RKCAMSRC (src);
  char vdev[32];

  rkcamsrc->offset = 0;
  rkcamsrc->renegotiation_adjust = 0;

  /* activate settings for first frame */
  rkcamsrc->ctrl_time = 0;
  gst_object_sync_values (GST_OBJECT (src), rkcamsrc->ctrl_time);

  rkcamsrc->has_bad_timestamp = FALSE;
  rkcamsrc->last_timestamp = 0;

  rk_common_v4l2device_find_by_name ("rkisp1_mainpath", vdev);
  if (strcmp (rkcamsrc->v4l2object->videodev, vdev)) {
    printf ("rkcamsrc: Using ISP self path......\n");
  } else {
    printf ("rkcamsrc: Using ISP main path......\n");
  }

  rkcamsrc->media_index =
      rk_common_media_find_by_vnode (rkcamsrc->v4l2object->videodev);
  if (rkcamsrc->media_index == -1)
    return FALSE;

  rkcamsrc->isp_subdev =
      rk_common_media_find_subdev_by_name (rkcamsrc->media_index,
      "rkisp1-isp-subdev");
  rkcamsrc->isp_params_subdev =
      rk_common_media_find_subdev_by_name (rkcamsrc->media_index,
      "rkisp1-input-params");
  rkcamsrc->isp_stats_subdev =
      rk_common_media_find_subdev_by_name (rkcamsrc->media_index,
      "rkisp1-statistics");
  rkcamsrc->phy_subdev =
      rk_common_media_find_subdev_by_name (rkcamsrc->media_index,
      "rockchip-sy-mipi-dphy");

  /* assume the last enity is sensor */
  rkcamsrc->sensor_subdev =
      rk_common_media_get_last_enity (rkcamsrc->media_index);

  return TRUE;
}

static gboolean
gst_rkcamsrc_unlock (GstBaseSrc * src)
{
  GstRKCamSrc *rkcamsrc = GST_RKCAMSRC (src);
  return gst_v4l2_object_unlock (rkcamsrc->v4l2object);
}

static gboolean
gst_rkcamsrc_unlock_stop (GstBaseSrc * src)
{
  GstRKCamSrc *rkcamsrc = GST_RKCAMSRC (src);

  rkcamsrc->last_timestamp = 0;

  return gst_v4l2_object_unlock_stop (rkcamsrc->v4l2object);
}

static gboolean
gst_rkcamsrc_stop (GstBaseSrc * src)
{
  GstRKCamSrc *rkcamsrc = GST_RKCAMSRC (src);
  GstRKV4l2Object *obj = rkcamsrc->v4l2object;

  if (GST_V4L2_IS_ACTIVE (obj)) {
    if (!gst_v4l2_object_stop (obj))
      return FALSE;
  }

  rkcamsrc->pending_set_fmt = FALSE;

  return TRUE;
}

static GstStateChangeReturn
gst_rkcamsrc_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstRKCamSrc *rkcamsrc = GST_RKCAMSRC (element);
  GstRKV4l2Object *obj = rkcamsrc->v4l2object;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      /* open the device */
      if (!gst_v4l2_object_open (obj))
        return GST_STATE_CHANGE_FAILURE;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
      /* close the device */
      if (!gst_v4l2_object_close (obj))
        return GST_STATE_CHANGE_FAILURE;

      break;
    default:
      break;
  }

  return ret;
}

static GstFlowReturn
gst_rkcamsrc_create (GstPushSrc * src, GstBuffer ** buf)
{
  GstRKCamSrc *rkcamsrc = GST_RKCAMSRC (src);
  GstRKV4l2Object *obj = rkcamsrc->v4l2object;
  GstRKV4l2BufferPool *pool = GST_V4L2_BUFFER_POOL_CAST (obj->pool);
  GstFlowReturn ret;
  GstClock *clock;
  GstClockTime abs_time, base_time, timestamp, duration;
  GstClockTime delay;
  GstMessage *qos_msg;

  do {
    ret = GST_BASE_SRC_CLASS (parent_class)->alloc (GST_BASE_SRC (src), 0,
        obj->info.size, buf);

    if (G_UNLIKELY (ret != GST_FLOW_OK))
      goto alloc_failed;

    ret = gst_v4l2_buffer_pool_process (pool, buf);

  } while (ret == GST_V4L2_FLOW_CORRUPTED_BUFFER);

  if (G_UNLIKELY (ret != GST_FLOW_OK))
    goto error;

  timestamp = GST_BUFFER_TIMESTAMP (*buf);
  duration = obj->duration;

  /* timestamps, LOCK to get clock and base time. */
  /* FIXME: element clock and base_time is rarely changing */
  GST_OBJECT_LOCK (rkcamsrc);
  if ((clock = GST_ELEMENT_CLOCK (rkcamsrc))) {
    /* we have a clock, get base time and ref clock */
    base_time = GST_ELEMENT (rkcamsrc)->base_time;
    gst_object_ref (clock);
  } else {
    /* no clock, can't set timestamps */
    base_time = GST_CLOCK_TIME_NONE;
  }
  GST_OBJECT_UNLOCK (rkcamsrc);

  /* sample pipeline clock */
  if (clock) {
    abs_time = gst_clock_get_time (clock);
    gst_object_unref (clock);
  } else {
    abs_time = GST_CLOCK_TIME_NONE;
  }

retry:
  if (!rkcamsrc->has_bad_timestamp && timestamp != GST_CLOCK_TIME_NONE) {
    struct timespec now;
    GstClockTime gstnow;

    /* v4l2 specs say to use the system time although many drivers switched to
     * the more desirable monotonic time. We first try to use the monotonic time
     * and see how that goes */
    clock_gettime (CLOCK_MONOTONIC, &now);
    gstnow = GST_TIMESPEC_TO_TIME (now);

    if (timestamp > gstnow || (gstnow - timestamp) > (10 * GST_SECOND)) {
      GTimeVal now;

      /* very large diff, fall back to system time */
      g_get_current_time (&now);
      gstnow = GST_TIMEVAL_TO_TIME (now);
    }

    /* Detect buggy drivers here, and stop using their timestamp. Failing any
     * of these condition would imply a very buggy driver:
     *   - Timestamp in the future
     *   - Timestamp is going backward compare to last seen timestamp
     *   - Timestamp is jumping forward for less then a frame duration
     *   - Delay is bigger then the actual timestamp
     * */
    if (timestamp > gstnow) {
      GST_WARNING_OBJECT (rkcamsrc,
          "Timestamp in the future detected, ignoring driver timestamps");
      rkcamsrc->has_bad_timestamp = TRUE;
      goto retry;
    }

    if (rkcamsrc->last_timestamp > timestamp) {
      GST_WARNING_OBJECT (rkcamsrc,
          "Timestamp going backward, ignoring driver timestamps");
      rkcamsrc->has_bad_timestamp = TRUE;
      goto retry;
    }

    delay = gstnow - timestamp;

    if (delay > timestamp) {
      GST_WARNING_OBJECT (rkcamsrc,
          "Timestamp does not correlate with any clock, ignoring driver timestamps");
      rkcamsrc->has_bad_timestamp = TRUE;
      goto retry;
    }

    /* Save last timestamp for sanity checks */
    rkcamsrc->last_timestamp = timestamp;

    GST_DEBUG_OBJECT (rkcamsrc, "ts: %" GST_TIME_FORMAT " now %" GST_TIME_FORMAT
        " delay %" GST_TIME_FORMAT, GST_TIME_ARGS (timestamp),
        GST_TIME_ARGS (gstnow), GST_TIME_ARGS (delay));
  } else {
    /* we assume 1 frame latency otherwise */
    if (GST_CLOCK_TIME_IS_VALID (duration))
      delay = duration;
    else
      delay = 0;
  }

  /* set buffer metadata */

  if (G_LIKELY (abs_time != GST_CLOCK_TIME_NONE)) {
    /* the time now is the time of the clock minus the base time */
    timestamp = abs_time - base_time;

    /* adjust for delay in the device */
    if (timestamp > delay)
      timestamp -= delay;
    else
      timestamp = 0;
  } else {
    timestamp = GST_CLOCK_TIME_NONE;
  }

  /* activate settings for next frame */
  if (GST_CLOCK_TIME_IS_VALID (duration)) {
    rkcamsrc->ctrl_time += duration;
  } else {
    /* this is not very good (as it should be the next timestamp),
     * still good enough for linear fades (as long as it is not -1)
     */
    rkcamsrc->ctrl_time = timestamp;
  }
  gst_object_sync_values (GST_OBJECT (src), rkcamsrc->ctrl_time);

  GST_INFO_OBJECT (src, "sync to %" GST_TIME_FORMAT " out ts %" GST_TIME_FORMAT,
      GST_TIME_ARGS (rkcamsrc->ctrl_time), GST_TIME_ARGS (timestamp));

  /* use generated offset values only if there are not already valid ones
   * set by the v4l2 device */
  if (!GST_BUFFER_OFFSET_IS_VALID (*buf)
      || !GST_BUFFER_OFFSET_END_IS_VALID (*buf)) {
    GST_BUFFER_OFFSET (*buf) = rkcamsrc->offset++;
    GST_BUFFER_OFFSET_END (*buf) = rkcamsrc->offset;
  } else {
    /* adjust raw v4l2 device sequence, will restart at null in case of renegotiation
     * (streamoff/streamon) */
    GST_BUFFER_OFFSET (*buf) += rkcamsrc->renegotiation_adjust;
    GST_BUFFER_OFFSET_END (*buf) += rkcamsrc->renegotiation_adjust;
    /* check for frame loss with given (from v4l2 device) buffer offset */
    if ((rkcamsrc->offset != 0)
        && (GST_BUFFER_OFFSET (*buf) != (rkcamsrc->offset + 1))) {
      guint64 lost_frame_count =
          GST_BUFFER_OFFSET (*buf) - rkcamsrc->offset - 1;
      GST_WARNING_OBJECT (rkcamsrc,
          "lost frames detected: count = %" G_GUINT64_FORMAT " - ts: %"
          GST_TIME_FORMAT, lost_frame_count, GST_TIME_ARGS (timestamp));

      qos_msg = gst_message_new_qos (GST_OBJECT_CAST (rkcamsrc), TRUE,
          GST_CLOCK_TIME_NONE, GST_CLOCK_TIME_NONE, timestamp,
          GST_CLOCK_TIME_IS_VALID (duration) ? lost_frame_count *
          duration : GST_CLOCK_TIME_NONE);
      gst_element_post_message (GST_ELEMENT_CAST (rkcamsrc), qos_msg);

    }
    rkcamsrc->offset = GST_BUFFER_OFFSET (*buf);
  }

  GST_BUFFER_TIMESTAMP (*buf) = timestamp;
  GST_BUFFER_DURATION (*buf) = duration;

  return ret;

  /* ERROR */
alloc_failed:
  {
    if (ret != GST_FLOW_FLUSHING)
      GST_ELEMENT_ERROR (src, RESOURCE, NO_SPACE_LEFT,
          ("Failed to allocate a buffer"), (NULL));
    return ret;
  }
error:
  {
    gst_buffer_replace (buf, NULL);
    if (ret == GST_V4L2_FLOW_LAST_BUFFER) {
      GST_ELEMENT_ERROR (src, RESOURCE, FAILED,
          ("Driver returned a buffer with no payload, this most likely "
              "indicate a bug in the driver."), (NULL));
      ret = GST_FLOW_ERROR;
    } else {
      GST_DEBUG_OBJECT (src, "error processing buffer %d (%s)", ret,
          gst_flow_get_name (ret));
    }
    return ret;
  }
}


/* GstURIHandler interface */
static GstURIType
gst_rkcamsrc_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_rkcamsrc_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "v4l2", NULL };

  return protocols;
}

static gchar *
gst_rkcamsrc_uri_get_uri (GstURIHandler * handler)
{
  GstRKCamSrc *rkcamsrc = GST_RKCAMSRC (handler);

  if (rkcamsrc->v4l2object->videodev != NULL) {
    return g_strdup_printf ("v4l2://%s", rkcamsrc->v4l2object->videodev);
  }

  return g_strdup ("v4l2://");
}

static gboolean
gst_rkcamsrc_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  GstRKCamSrc *rkcamsrc = GST_RKCAMSRC (handler);
  const gchar *device = DEFAULT_PROP_DEVICE;

  if (strcmp (uri, "v4l2://") != 0) {
    device = uri + 7;
  }
  g_object_set (rkcamsrc, "device", device, NULL);

  return TRUE;
}

static void
gst_rkcamsrc_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_rkcamsrc_uri_get_type;
  iface->get_protocols = gst_rkcamsrc_uri_get_protocols;
  iface->get_uri = gst_rkcamsrc_uri_get_uri;
  iface->set_uri = gst_rkcamsrc_uri_set_uri;
}
