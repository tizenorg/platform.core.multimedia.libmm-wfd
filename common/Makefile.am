lib_LTLIBRARIES = libwfdcommon.la

includelibwfdcommondir = $(includedir)/mmf

libwfdcommon_la_SOURCES = mm_wfd_attrs.c mm_wfd_ini.c

libwfdcommon_la_CFLAGS =  -I$(srcdir)/include \
		-I$(top_builddir)/src/include \
		$(GST_CFLAGS) \
        $(GLIB_CFLAGS) \
        $(MMCOMMON_CFLAGS) \
		$(MMTA_CFLAGS)

libwfdcommon_la_LIBADD = $(GLIB_LIBS) \
        $(MMCOMMON_LIBS) \
        $(INIPARSER_LIBS)