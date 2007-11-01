bin_PROGRAMS = matchbox-desktop

PKGDATADIR = $(datadir)/matchbox

AM_CPPFLAGS = $(GTK_CFLAGS) $(SN_CFLAGS) -DPKGDATADIR=\"$(PKGDATADIR)\"
AM_CFLAGS = -Wall
LDADD = $(GTK_LIBS) $(SN_LIBS) $(top_builddir)/libtaku/libtaku.a
if HAVE_INOTIFY
LDADD += $(top_builddir)/libtaku/libinotify.a
endif

matchbox_desktop_SOURCES = main.c
