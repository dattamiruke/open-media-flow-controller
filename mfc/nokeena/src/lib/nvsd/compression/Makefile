# Filename: lib/nvsd/compression/Makefile

include ${PROD_TREE_ROOT}/src/mk/common.mk

LIB=nvsd_compression
LIB_BUILD_DIR=${CUST_LIB_DIR}/nvsd/compression

SRCS= \
	nkn_compressmgr.c      \


CFLAGS += -fPIC
CFLAGS += -D_GNU_SOURCE
CFLAGS += -include nkn_no_alloc.h
CFLAGS += -I/usr/lib64/glib-2.0/include -I/usr/include/glib-2.0 -I${PROD_CUSTOMER_ROOT}/nokeena/src/include/mfp -I/usr/lib/glib-2.0/include
CFLAGS += ${BUILD_EXTRA_CFLAGS}

ifeq (${BUILD_GCOV}, 1)
  CFLAGS += -fprofile-arcs -ftest-coverage
endif

ifneq (${PROD_FEATURE_I18N_SUPPORT}, 0)
	GETTEXT_PACKAGE=lib${LIB}
	DEFINES+=-DGETTEXT_DOMAIN=\"${GETTEXT_PACKAGE}\"
	SUBMAKEFILES+=Makefile.i18n
endif

LDADD= \
 ${PROD_BUILD_OUTPUT_DIR}/${INSTALL_TREE}/${LIB_DIR}/libcommon/libcommon.a \
 ${PROD_BUILD_OUTPUT_DIR}/${INSTALL_TREE}/${CUST_LIB_DIR}/mfp/libmfp_kms.a \
 ${BUILD_EXTRA_LDADD}

include ${PROD_TREE_ROOT}/src/mk/lib.mk
