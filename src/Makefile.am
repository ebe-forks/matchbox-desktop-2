AM_CPPFLAGS = $(GTK_CFLAGS) $(SN_CFLAGS) -I$(top_srcdir)
AM_CFLAGS = -Wall

bin_PROGRAMS = matchbox-desktop
matchbox_desktop_SOURCES = \
	main.c \
	taku-category-bar.c taku-category-bar.h \
	desktop.c desktop.h

matchbox_desktop_LDADD = \
	$(GTK_LIBS) \
	$(SN_LIBS) \
	$(top_builddir)/libtaku/libtaku.a

if HAVE_INOTIFY
matchbox_desktop_LDADD += $(top_builddir)/libtaku/libinotify.a
endif
