lib_LTLIBRARIES = libmmfwfdsink.la

includelibmmfwfdsinkdir = $(includedir)/mmf

includelibmmfwfdsink_HEADERS = include/mm_wfd_sink.h

libmmfwfdsink_la_SOURCES = mm_wfd_sink_util.c \
				mm_wfd_sink.c \
				mm_wfd_sink_manager.c \
				mm_wfd_sink_priv.c \
				mm_wfd_sink_wayland.c


libmmfwfdsink_la_CFLAGS =  -I$(srcdir)/include \
			$(MMCOMMON_CFLAGS) \
			$(GST_CFLAGS) \
			$(EVAS_CFLAGS) \
			$(ELEMENTARY_CFLAGS) \
			$(GST_PLUGINS_BASE_CFLAGS) \
			$(GST_VIDEO_CFLAGS) \
			$(MM_WFD_COMMON_CFLAGS) \
			$(AUDIOSESSIONMGR_CFLAGS) \
			-I$(top_builddir)/common/include


libmmfwfdsink_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

noinst_HEADERS = include/mm_wfd_sink_priv.h \
			include/mm_wfd_sink_util.h \
			include/mm_wfd_sink_manager.h \
			include/mm_wfd_sink_wayland.h

libmmfwfdsink_la_LIBADD = $(GST_LIBS) \
			$(GST_PLUGINS_BASE_LIBS) \
			$(GST_BASE_LIBS) \
			$(ELEMENTARY_LIBS) \
			$(EVAS_LIBS) \
			$(top_builddir)/common/libwfdcommon.la \
			$(MMCOMMON_LIBS) \
			$(MM_WFD_COMMON_LIBS) \
			$(AUDIOSESSIONMGR_LIBS) \
			$(GST_VIDEO_LIBS)


libmmfwfdsink_la_CFLAGS += $(MMLOG_CFLAGS) -DMMF_LOG_OWNER=0x02000000 -DMMF_DEBUG_PREFIX=\"MMF-WFD-SINK\"
libmmfwfdsink_la_LIBADD += $(MMLOG_LIBS)
