bin_PROGRAMS = mm_wfd_proxy_test

mm_wfd_proxy_test_SOURCES = mm_wfd_proxy_test.c

mm_wfd_proxy_test_CFLAGS = -I$(srcdir)/../src/include \
			     $(GLIB_CFLAGS)\
			     $(INIPARSER_CFLAGS) \
			     $(MMCOMMON_CFLAGS) \
			     $(UTILX_CFLAGS)\
			     $(X11_CFLAGS)

mm_wfd_proxy_test_CFLAGS += -D_USE_GMAINLOOP
#mm_wfd_proxy_test_CFLAGS += -D_USE_EFLMAINLOOP

############################################

mm_wfd_proxy_test_DEPENDENCIES = $(top_builddir)/src/libmmfwfdproxy.la

mm_wfd_proxy_test_LDADD = $(top_builddir)/src/libmmfwfdproxy.la \
			    $(GLIB_LIBS)\
			    $(MMCOMMON_LIBS) \
			    $(INIPARSER_LIBS)\
			    $(UTILX_LIBS)\
			    $(X11_LIBS)
