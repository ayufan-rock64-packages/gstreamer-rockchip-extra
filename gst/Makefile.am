if USE_RKV4L2
V4l2_DIR=v4l2
else
V4l2_DIR=
endif

if USE_KMS
KMS_DIR=kms
else
KMS_DIR=
endif

if USE_RKXIMAGE
RKXIMAGE_DIR=rkximage
else
RKXIMAGE_DIR=
endif

SUBDIRS = $(V4l2_DIR) $(RKXIMAGE_DIR) $(KMS_DIR)

include $(top_srcdir)/common/parallel-subdirs.mak
