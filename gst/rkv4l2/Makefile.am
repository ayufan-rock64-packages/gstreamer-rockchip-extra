plugin_LTLIBRARIES = libgstrkv4l2.la

libgstrkv4l2_la_SOURCES = v4l2/gstv4l2.c \
				v4l2/gstv4l2allocator.c \
				v4l2/gstv4l2colorbalance.c \
				v4l2/gstv4l2vidorient.c \
				v4l2/gstv4l2object.c \
				v4l2/gstv4l2bufferpool.c \
				v4l2/v4l2_calls.c \
				v4l2/v4l2-utils.c \
				rkispsrc.c \
				rkrgaconvert.c \
				rkcommon.c

libgstrkv4l2_la_CFLAGS =   $(GST_PLUGINS_BASE_CFLAGS) \
				 $(GST_BASE_CFLAGS) \
				 $(GST_CFLAGS) \
				 $(RKV4L2_CFLAGS) \
				 -I ./ext \
				 -I ./v4l2 \
				 -I ./

libgstrkv4l2_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstrkv4l2_la_LIBTOOLFLAGS = $(GST_PLUGIN_LIBTOOLFLAGS)
libgstrkv4l2_la_LIBADD =   $(GST_PLUGINS_BASE_LIBS) \
				 -lgstallocators-$(GST_API_VERSION) \
				 -lgstvideo-$(GST_API_VERSION) \
				 $(GST_BASE_LIBS) \
				 $(GST_LIBS) \
				 $(RKV4L2_LIBS)

noinst_HEADERS = \
	ext/types-compat.h \
	ext/v4l2-common.h \
	ext/v4l2-controls.h \
	ext/videodev2.h \
	v4l2/gstv4l2allocator.h \
	v4l2/gstv4l2bufferpool.h \
	v4l2/gstv4l2vidorient.h \
	v4l2/gstv4l2colorbalance.h \
	v4l2/gstv4l2object.h \
	v4l2/v4l2_calls.h \
	v4l2/v4l2-utils.h \
	rkispsrc.h \
	rkrgaconvert.h \
	rkcommon.h

