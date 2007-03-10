bin_PROGRAMS = matchbox-desktop

matchbox_desktop_CPPFLAGS = \
	$(GTK_CFLAGS) $(SN_CFLAGS)
matchbox_desktop_CFLAGS = \
	-Wall
matchbox_desktop_LDADD = \
	$(GTK_LIBS) $(SN_LIBS)

matchbox_desktop_SOURCES = \
	taku-tile.h taku-tile.c \
	taku-icon-tile.h taku-icon-tile.c \
	taku-launcher-tile.h taku-launcher-tile.c \
	taku-table.h taku-table.c \
	launcher-util.h launcher-util.c \
	xutil.h xutil.c \
	eggsequence.h eggsequence.c \
	main.c

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
