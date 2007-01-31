AC_PREREQ(2.52)
AC_INIT(matchbox-desktop, 0.0, http://o-hand.com)
AC_CONFIG_SRCDIR(taku-tile.c)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)

AM_DISABLE_STATIC
AC_PROG_CPP
AC_PROG_CC
dnl AM_PROG_LIBTOOL

PKG_CHECK_MODULES(GTK, gtk+-2.0)

AC_ARG_ENABLE(startup_notification,
        AC_HELP_STRING([--enable-startup-notification], [enable startup notification support]),
        enable_startup_notification=$enableval, enable_startup_notification=yes)

if test x$enable_startup_notification != xno; then
  PKG_CHECK_MODULES(SN, libstartup-notification-1.0, ,
    AC_MSG_ERROR([*** Required Startup Notification library not installed ***]))
  AC_DEFINE(USE_LIBSN, [1], [Has Startup Notification Support])
fi

AC_OUTPUT([
Makefile
])
