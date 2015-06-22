PKGDATADIR = $(datadir)/matchbox

AM_CPPFLAGS = $(GTK_CFLAGS) $(SN_CFLAGS) -DPKGDATADIR=\"$(PKGDATADIR)\"
AM_CFLAGS = $(WARN_CFLAGS)

noinst_LIBRARIES = libtaku.a
libtaku_a_SOURCES = \
	launcher-util.c launcher-util.h \
	taku-icon-tile.c taku-icon-tile.h \
	taku-launcher-tile.c taku-launcher-tile.h \
	taku-menu.h \
	taku-menu-desktop.c \
	taku-tile.c taku-tile.h \
	xutil.c xutil.h \
	taku-queue-source.c taku-queue-source.h

if HAVE_INOTIFY
noinst_LIBRARIES += libinotify.a
libinotify_a_SOURCES = \
	inotify/inotify-kernel.c inotify/inotify-kernel.h \
	inotify/inotify-sub.c 	inotify/inotify-sub.h \
	inotify/inotify-path.c inotify/inotify-path.h \
	inotify/inotify-missing.c inotify/inotify-missing.h \
	inotify/inotify-diag.c inotify/inotify-diag.h \
	inotify/local_inotify.h inotify/local_inotify_syscalls.h
endif

-include $(top_srcdir)/git.mk
