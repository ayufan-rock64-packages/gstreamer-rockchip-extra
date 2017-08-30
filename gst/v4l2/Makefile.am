plugin_LTLIBRARIES = libgstrkv4l2.la

libgstrkv4l2_la_SOURCES = gstv4l2.c \
				gstv4l2allocator.c \
				gstv4l2colorbalance.c \
				gstv4l2deviceprovider.c \
				gstv4l2object.c \
				gstv4l2bufferpool.c \
				gstv4l2src.c \
				gstv4l2transform.c \
				gstv4l2vidorient.c \
				v4l2_calls.c \
				v4l2-utils.c

libgstrkv4l2_la_CFLAGS =   $(GST_PLUGINS_BASE_CFLAGS) \
				 $(GST_BASE_CFLAGS) \
				 $(GST_CFLAGS) \
				 $(RKV4L2_CFLAGS)

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
	gstv4l2allocator.h \
	gstv4l2bufferpool.h \
	gstv4l2colorbalance.h \
	gstv4l2deviceprovider.h \
	gstv4l2object.h \
	gstv4l2src.h \
	gstv4l2transform.h \
	gstv4l2vidorient.h \
	v4l2_calls.h \
	v4l2-utils.h

