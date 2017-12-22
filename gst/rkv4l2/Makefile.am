plugin_LTLIBRARIES = libgstrkv4l2.la

libgstrkv4l2_la_SOURCES =
	rkv4l2.c \
	common.c \
	v4l2/gstv4l2allocator.c \
	v4l2/gstv4l2object.c \
	v4l2/gstv4l2bufferpool.c \
	v4l2/v4l2_calls.c \
	v4l2/v4l2-utils.c \
	ext/mediactl.c \
	ext/v4l2subdev.c \
	rkcamsrc/rkcamsrc.c \
	rkcamsrc/rkcamsrc.c \
	rgaconvert/rgaconvert.c

libgstrkv4l2_la_CFLAGS = 
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_BASE_CFLAGS) \
	$(GST_CFLAGS) \
	$(RKV4L2_CFLAGS) \
	-I $(top_srcdir)/gst/rkv4l2/ext \
	-I $(top_srcdir)/gst/rkv4l2/v4l2
				 
libgstrkv4l2_la_LDFLAGS =
	$(GST_PLUGIN_LDFLAGS)
libgstrkv4l2_la_LIBTOOLFLAGS =
	$(GST_PLUGIN_LIBTOOLFLAGS)
libgstrkv4l2_la_LIBADD =
	$(GST_PLUGINS_BASE_LIBS) \
	-lgstallocators-$(GST_API_VERSION) \
	-lgstvideo-$(GST_API_VERSION) \
	$(GST_BASE_LIBS) \
	$(GST_LIBS) \
	$(RKV4L2_LIBS)
