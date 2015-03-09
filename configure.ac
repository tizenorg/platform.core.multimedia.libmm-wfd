#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([libmm-wfd],[0.0.1])
AC_CONFIG_SRCDIR([sink/mm_wfd_sink_priv.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
m4_pattern_allow([AM_PROG_AR])
AM_PROG_AR
AC_PROG_CC
AC_PROG_LIBTOOL

AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_HEADER_STDBOOL
AC_HEADER_STDC
AC_HEADER_TIME
AC_PROG_GCC_TRADITIONAL

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_INT64_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_STAT
AC_CHECK_FUNCS([gettimeofday memset strchr strstr])

PKG_CHECK_MODULES(MMCOMMON,mm-common)
AC_SUBST(MMCOMMON_CFLAGS)
AC_SUBST(MMCOMMON_LIBS)

PKG_CHECK_MODULES(GLIB, glib-2.0)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(GIO, gio-2.0)
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GIO_LIBS)

PKG_CHECK_MODULES(GST, gstreamer-1.0 >= 1.0)
AC_SUBST(GST_CFLAGS)
AC_SUBST(GST_LIBS)

PKG_CHECK_MODULES(GST_PLUGIN_BASE, gstreamer-plugins-base-1.0 >= 1.0)
AC_SUBST(GST_PLUGIN_BASE_CFLAGS)
AC_SUBST(GST_PLUGIN_BASE_LIBS)

PKG_CHECK_MODULES(GST_VIDEO, gstreamer-video-1.0 >= 1.0)
AC_SUBST(GST_VIDEO_CFLAGS)
AC_SUBST(GST_VIDEO_LIBS)

PKG_CHECK_MODULES(GST_APP, gstreamer-app-1.0 >= 1.0)
AC_SUBST(GST_APP_CFLAGS)
AC_SUBST(GST_APP_LIBS)

PKG_CHECK_MODULES(INIPARSER, iniparser)
AC_SUBST(INIPARSER_CFLAGS)
AC_SUBST(INIPARSER_LIBS)

PKG_CHECK_MODULES(WIFI_DIRECT, capi-network-wifi-direct)
AC_SUBST(WIFI_DIRECT_CFLAGS)
AC_SUBST(WIFI_DIRECT_LIBS)

PKG_CHECK_MODULES(MM_WFD_COMMON, mm-scmirroring-common)
AC_SUBST(MM_WFD_COMMON_CFLAGS)
AC_SUBST(MM_WFD_COMMON_LIBS)

# for testsuite

AC_ARG_ENABLE(sdk, AC_HELP_STRING([--enable-sdk], [sdk build]),
	 [
		case "${enableval}" in
		yes) IS_SDK=yes ;;
		no)  IS_SDK=no ;;
		  *)   AC_MSG_ERROR(bad value ${enableval} for --enable-sdk) ;;
		esac
	],
[IS_SDK=no])
AM_CONDITIONAL([IS_SDK], [test "x$IS_SDK" = "xyes"])

AC_CONFIG_FILES([Makefile
		 common/Makefile
		 sink/Makefile
		 sink_test/Makefile
		 mm-wfd.pc
])
AC_OUTPUT
