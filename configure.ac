AC_PREREQ(2.52)
AC_INIT(matchbox-desktop, 2.2)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR(libtaku/taku-tile.c)

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

AC_PROG_CPP
AC_PROG_CC
AC_PROG_RANLIB

WARN_CFLAGS="-Wall -Wextra -Wno-unused-parameter"
AC_SUBST(WARN_CFLAGS)

AC_CHECK_HEADERS([sys/inotify.h], inotify_support=yes, inotify_support=no)
AM_CONDITIONAL(HAVE_INOTIFY, [test "$inotify_support" = "yes"])
if test x$inotify_support = xyes; then
        AC_DEFINE(WITH_INOTIFY, [1], [If inotify is enabled])
fi

PKG_CHECK_MODULES(GTK, [glib-2.0 >= 2.14 gtk+-3.0 x11])

AC_ARG_ENABLE(startup_notification,
        AC_HELP_STRING([--disable-startup-notification], [disable startup notification support]),
        enable_startup_notification=$enableval, enable_startup_notification=yes)

if test x$enable_startup_notification != xno; then
  PKG_CHECK_MODULES(SN, libstartup-notification-1.0, ,
    AC_MSG_ERROR([*** Required Startup Notification library not installed ***]))
  AC_DEFINE(USE_LIBSN, [1], [Has Startup Notification Support])
fi

AC_ARG_WITH(dbus,
        AC_HELP_STRING([--with-dbus], [use DBus to send a 'loaded' signal]),
        with_dbus=$withval, with_dbus=no)
if test x$with_dbus != xno; then
    PKG_CHECK_MODULES(DBUS, dbus-1, ,
      AC_MSG_ERROR([libdbus-1 not installed (disable with --without-dbus)]))
    AC_DEFINE(WITH_DBUS, [1], [Use DBus])
fi   

AC_OUTPUT([
Makefile
libtaku/Makefile
src/Makefile
])
