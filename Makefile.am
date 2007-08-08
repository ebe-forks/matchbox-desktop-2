bin_PROGRAMS = matchbox-desktop

PKGDATADIR = $(datadir)/matchbox

AM_CPPFLAGS = $(GTK_CFLAGS) $(SN_CFLAGS) -DPKGDATADIR=\"$(PKGDATADIR)\"
AM_CFLAGS = -Wall
LDADD = $(GTK_LIBS) $(SN_LIBS)

matchbox_desktop_SOURCES = \
	taku-tile.h taku-tile.c \
	taku-icon-tile.h taku-icon-tile.c \
	taku-launcher-tile.h taku-launcher-tile.c \
	taku-table.h taku-table.c \
	launcher-util.h launcher-util.c \
	xutil.h xutil.c \
	eggsequence.h eggsequence.c \
	main.c

# Optional inotify support
if HAVE_INOTIFY
AM_CPPFLAGS += -DWITH_INOTIFY
LDADD += libinotify.a

noinst_LIBRARIES = libinotify.a
libinotify_a_SOURCES = \
	inotify/inotify-kernel.c inotify/inotify-kernel.h \
	inotify/inotify-sub.c 	inotify/inotify-sub.h \
	inotify/inotify-path.c inotify/inotify-path.h \
	inotify/inotify-missing.c inotify/inotify-missing.h \
	inotify/inotify-diag.c inotify/inotify-diag.h \
	inotify/local_inotify.h inotify/local_inotify_syscalls.h
endif


# make maintainer-clean should remove *everything* 
MAINTAINERCLEANFILES = \
	aclocal.m4 \
	compile \
	config.guess \
	config.h.in \
	configure \
	depcomp \
	install-sh \
	Makefile.in \
	missing
