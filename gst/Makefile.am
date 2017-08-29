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

SUBDIRS = $(RKXIMAGE_DIR) $(KMS_DIR)

include $(top_srcdir)/common/parallel-subdirs.mak
