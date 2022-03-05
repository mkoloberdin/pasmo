# configure.ac for Pasmo
# Process this file with autoconf to produce a configure script.

#AC_PREREQ(2.57)

AC_INIT(pasmo, 0.5.5, julian.notfound@gmail.com)

AM_INIT_AUTOMAKE([foreign])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AC_REQUIRE_AUX_FILE([tap-driver.sh])

########################################################################
#		Options
########################################################################

AC_ARG_ENABLE(debug, AC_HELP_STRING(
	[--enable-debug], [Debug version [[no]]] ) )

if test "$enable_debug" = "yes" ; then
	AC_MSG_RESULT([Configured for debug version])
else
	AC_MSG_RESULT([Configured for release version])
	AC_DEFINE([NDEBUG], [], [Using debug mode])
fi

########################################################################

AC_PROG_CXX

########################################################################
# Generate files

AC_CONFIG_FILES([Makefile])

AC_OUTPUT

# End of configure.ac for Pasmo
