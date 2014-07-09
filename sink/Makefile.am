lib_LTLIBRARIES = libmmfwfdsink.la

includelibmmfwfdsinkdir = $(includedir)/mmf

includelibmmfwfdsink_HEADERS = include/mm_wfd_sink.h

libmmfwfdsink_la_SOURCES = mm_wfd_sink.c \
		mm_wfd_sink_util.c \
		mm_wfd_sink_priv.c

libmmfwfdsink_la_CFLAGS =  -I$(srcdir)/include \
			  $(MMCOMMON_CFLAGS) \
			  $(GST_CFLAGS) \
			  $(GST_PLUGINS_BASE_CFLAGS) \
			  -I$(top_builddir)/common/include

libmmfwfdsink_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

noinst_HEADERS = include/mm_wfd_sink_priv.h \
				include/mm_wfd_sink_util.h

libmmfwfdsink_la_LIBADD = $(GST_LIBS) \
		$(GST_PLUGINS_BASE_LIBS) \
		$(GST_BASE_LIBS) \
		$(top_builddir)/common/libwfdcommon.la \
		$(MMCOMMON_LIBS)

libmmfwfdsink_la_CFLAGS += $(MMLOG_CFLAGS) -DMMF_LOG_OWNER=0x02000000 -DMMF_DEBUG_PREFIX=\"MMF-WFD-SINK\"
libmmfwfdsink_la_LIBADD += $(MMLOG_LIBS)
