bin_PROGRAMS = miracast_server

miracast_serverdir = $(includedir)/mmf

miracast_server_SOURCES = miracast_server.c

miracast_server_HEADERS = wfd-structure.h

miracast_server_CFLAGS = -I$(srcdir)/../src/include \
			     $(GLIB_CFLAGS)\
			     $(INIPARSER_CFLAGS) \
			     $(DBUS_CFLAGS) \
			     $(DBUS_GLIB_CFLAGS) \
			     $(MMCOMMON_CFLAGS) \
			     $(VCONF_CFLAGS)\
			     $(MMTA_CFLAGS)

miracast_server_CFLAGS += -D_USE_GMAINLOOP
#miracast_server_CFLAGS += -D_USE_EFLMAINLOOP

############################################

miracast_server_DEPENDENCIES = $(top_builddir)/src/libmmfwfd.la

miracast_server_LDADD = $(top_builddir)/src/libmmfwfd.la \
			    $(GLIB_LIBS)\
			    $(DBUS_LIBS) \
			    $(DBUS_GLIB_LIBS) \
			    $(MMCOMMON_LIBS) \
			    $(MMTA_LIBS) \
			    $(VCONF_LIBS)\
			    $(INIPARSER_LIBS)
