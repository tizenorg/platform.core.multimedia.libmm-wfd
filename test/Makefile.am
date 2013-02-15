bin_PROGRAMS = mm_wfd_testsuite

mm_wfd_testsuite_SOURCES = mm_wfd_testsuite.c

mm_wfd_testsuite_CFLAGS = -I$(srcdir)/../src/include \
			     $(GLIB_CFLAGS)\
			     $(INIPARSER_CFLAGS) \
			     $(MMCOMMON_CFLAGS) \
			     $(MMTA_CFLAGS)\
			     $(UTILX_CFLAGS)\
			     $(X11_CFLAGS)

mm_wfd_testsuite_CFLAGS += -D_USE_GMAINLOOP
#mm_wfd_testsuite_CFLAGS += -D_USE_EFLMAINLOOP

############################################

mm_wfd_testsuite_DEPENDENCIES = $(top_builddir)/src/libmmfwfd.la

mm_wfd_testsuite_LDADD = $(top_builddir)/src/libmmfwfd.la \
			    $(GLIB_LIBS)\
			    $(MMCOMMON_LIBS) \
			    $(MMTA_LIBS) \
			    $(INIPARSER_LIBS)\
			    $(UTILX_LIBS)\
			    $(X11_LIBS)
