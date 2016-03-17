lib_LTLIBRARIES = libwfdcommon.la

includelibwfdcommondir = $(includedir)/mmf

libwfdcommon_la_SOURCES = mm_wfd_attrs.c mm_wfd_sink_ini.c

libwfdcommon_la_CFLAGS =  -I$(srcdir)/include \
        $(GLIB_CFLAGS) \
        $(TZPLATFORM_CONFIG_CFLAGS) \
        $(MMCOMMON_CFLAGS)

libwfdcommon_la_LIBADD = $(GLIB_LIBS) \
        $(MMCOMMON_LIBS) \
        $(TZPLATFORM_CONFIG_LIBS) \
        $(INIPARSER_LIBS)
