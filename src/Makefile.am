lib_LTLIBRARIES = libmmfwfdproxy.la

includelibmmfwfdproxydir = $(includedir)/mmf

includelibmmfwfdproxy_HEADERS = include/mm_wfd_proxy.h \
        include/wfd-stub.h

libmmfwfdproxy_la_SOURCES = mm_wfd_proxy.c

libmmfwfdproxy_la_CFLAGS =  -I$(srcdir)/include \
				$(GLIB_CFLAGS) \
				$(GST_CFLAGS) \
        $(DBUS_CFLAGS) \
        $(DBUS_GLIB_CFLAGS) \
				$(GST_INTERFACE_CFLAGS) \
				$(MMCOMMON_CFLAGS) \
				$(GST_APP_CFLAGS) \
				$(GST_PLUGINS_BASE_CFLAGS)

#noinst_HEADERS = include/mm_wfd_proxy.h

libmmfwfdproxy_la_LIBADD = $(GLIB_LIBS) \
				$(GST_LIBS) \
				$(GST_PLUGINS_BASE_LIBS) \
				$(GST_BASE_LIBS) \
        $(DBUS_LIBS) \
        $(DBUS_GLIB_LIBS) \
				$(GST_INTERFACE_LIBS) \
				$(GST_APP_LIBS) \
				$(MMCOMMON_LIBS)

lib_LTLIBRARIES += libmmfwfd.la

includelibmmfwfddir = $(includedir)/mmf

includelibmmfwfd_HEADERS = include/mm_wfd.h

libmmfwfd_la_SOURCES = mm_wfd.c \
		mm_wfd_priv.c

libmmfwfd_la_CFLAGS =  -I$(srcdir)/include \
			  $(MMCOMMON_CFLAGS) \
			  $(MMTA_CFLAGS) \
			  $(GST_CFLAGS) \
			  -I$(top_builddir)/common/include

libmmfwfd_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

noinst_HEADERS = include/mm_wfd_priv.h
		 
libmmfwfd_la_LIBADD = $(GST_LIBS) \
		$(MMTA_LIBS) \
		$(MMCOMMON_LIBS) \
		$(GST_APP_LIBS) \
    -lmmfwfd_rtsp_server \
		$(top_builddir)/common/libwfdcommon.la \
		$(INIPARSER_LIBS)

libmmfwfd_la_CFLAGS += $(MMLOG_CFLAGS) -DMMF_LOG_OWNER=0x02000000 -DMMF_DEBUG_PREFIX=\"MMF-WFD\" -D_INTERNAL_SESSION_MANAGER_
libmmfwfd_la_LIBADD += $(MMLOG_LIBS)
