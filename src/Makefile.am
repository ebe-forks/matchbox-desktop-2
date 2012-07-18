AM_CPPFLAGS = \
	$(GTK_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(SN_CFLAGS) \
	-I$(top_srcdir) \
	-DPKGDATADIR=\"$(pkgdatadir)\"
AM_CFLAGS = $(WARN_CFLAGS)

bin_PROGRAMS = matchbox-desktop
matchbox_desktop_SOURCES = \
	main.c \
	taku-category-bar.c taku-category-bar.h \
	desktop.c desktop.h

matchbox_desktop_LDADD = \
	$(top_builddir)/libtaku/libtaku.a \
	$(GTK_LIBS) \
	$(DBUS_LIBS) \
	$(SN_LIBS)

if HAVE_INOTIFY
matchbox_desktop_LDADD += $(top_builddir)/libtaku/libinotify.a
endif

-include $(top_srcdir)/git.mk
