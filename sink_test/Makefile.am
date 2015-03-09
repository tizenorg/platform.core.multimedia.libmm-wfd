bin_PROGRAMS = mm_wfd_sink_testsuite

mm_wfd_sink_testsuite_SOURCES = mm_wfd_sink_testsuite2.c

mm_wfd_sink_testsuite_CFLAGS = -I$(srcdir)/../sink/include \
           $(GLIB_CFLAGS)\
           $(MMCOMMON_CFLAGS) \
           $(MM_WFD_COMMON_CFLAGS) \
           $(WIFI_DIRECT_CFLAGS)

mm_wfd_sink_testsuite_CFLAGS += -D_USE_GMAINLOOP

############################################

mm_wfd_sink_testsuite_DEPENDENCIES = $(top_builddir)/sink/libmmfwfdsink.la

mm_wfd_sink_testsuite_LDADD = $(top_builddir)/sink/libmmfwfdsink.la \
			    $(GLIB_LIBS)\
			    $(MMCOMMON_LIBS) \
			    $(MM_WFD_COMMON_LIBS) \
			    $(WIFI_DIRECT_LIBS)
