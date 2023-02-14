# Process this file with autoconf to create configure.

AC_PREREQ([2.65])

# ====================
# Version informations
# ====================
AC_INIT([braille-printer-app], [2.0b1], [https://github.com/OpenPrinting/braille-printer-app/issues], [braille-printer-app], [https://github.com/OpenPrinting/braille-printer-app/])
braille_printer_app_version="AC_PACKAGE_VERSION"
braille_printer_app_version_major="`echo AC_PACKAGE_VERSION | awk -F. '{print $1}'`"
braille_printer_app_version_major="`echo AC_PACKAGE_VERSION | awk -F. '{printf("%d\n",$2);}'`"

# =============
# Automake init
# =============
AC_CONFIG_MACRO_DIR([m4])
m4_include([m4/ac_define_dir.m4])
m4_include([m4/ax_compare_version.m4])
m4_include([m4/basic-directories.m4])
AM_INIT_AUTOMAKE([1.11 gnu dist-xz dist-bzip2 subdir-objects foreign])
AM_SILENT_RULES([yes])
AC_LANG([C])
AC_CONFIG_HEADERS([config.h])
# Extra defines for the config.h
AH_BOTTOM([
#ifdef HAVE_LONG_LONG
#  define CUPS_LLFMT   "%lld"
#  define CUPS_LLCAST  (long long)
#else
#  define CUPS_LLFMT   "%ld"
#  define CUPS_LLCAST  (long)
#endif /* HAVE_LONG_LONG */

#ifdef HAVE_ARC4RANDOM
#  define CUPS_RAND() arc4random()
#  define CUPS_SRAND(v) arc4random_stir()
#elif defined(HAVE_RANDOM)
#  define CUPS_RAND() random()
#  define CUPS_SRAND(v) srandom(v)
#elif defined(HAVE_LRAND48)
#  define CUPS_RAND() lrand48()
#  define CUPS_SRAND(v) srand48(v)
#else
#  define CUPS_RAND() rand()
#  define CUPS_SRAND(v) srand(v)
#endif /* HAVE_ARC4RANDOM */
])

# ===========================
# Find required base packages
# ===========================
AC_PROG_CC
AM_PROG_CC_C_O
AM_ICONV
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
LT_INIT
PKG_PROG_PKG_CONFIG([0.20])

AM_GNU_GETTEXT_VERSION([0.18.3])
AM_GNU_GETTEXT([external])

# ================================
# Find CUPS internals (no pc file)
# ================================
AC_ARG_WITH([cups-config],
	[AS_HELP_STRING([--with-cups-config=path], [Specify path to cups-config executable.])],
	[with_cups_config="$withval"],
	[with_cups_config=system]
)

AS_IF([test "x$with_cups_config" != "xsystem"], [
	CUPSCONFIG=$with_cups_config
], [
	AC_PATH_TOOL(CUPSCONFIG, [cups-config])
	AS_IF([test -z "$CUPSCONFIG"], [
		AC_MSG_ERROR([Required cups-config is missing. Please install CUPS developer packages.])
	])
])
CUPS_CFLAGS=`$CUPSCONFIG --cflags`
CUPS_LIBS=`$CUPSCONFIG --image --libs`
CUPS_VERSION=`$CUPSCONFIG --version`
AC_SUBST(CUPS_CFLAGS)
AC_SUBST(CUPS_LIBS)

CUPS_DATADIR="`$CUPSCONFIG --datadir`"
AC_DEFINE_UNQUOTED(CUPS_DATADIR, "$CUPS_DATADIR", [CUPS datadir])
AC_SUBST(CUPS_DATADIR)

CUPS_SERVERROOT="`$CUPSCONFIG --serverroot`"
AC_DEFINE_UNQUOTED(CUPS_SERVERROOT, "$CUPS_SERVERROOT", [CUPS serverroot])
AC_SUBST(CUPS_SERVERROOT)

CUPS_FONTPATH="$CUPS_DATADIR/fonts"
AC_DEFINE_UNQUOTED(CUPS_FONTPATH, "$CUPS_FONTPATH", [Path to CUPS fonts dir])
AC_SUBST(CUPS_FONTPATH)

CUPS_SERVERBIN="`$CUPSCONFIG --serverbin`"
AC_DEFINE_UNQUOTED(CUPS_SERVERBIN, "$CUPS_SERVERBIN", [Path to CUPS binaries dir])
AC_SUBST(CUPS_SERVERBIN)

# ======================
# Check system functions
# ======================
AC_CHECK_FUNCS(sigaction)
AC_CHECK_FUNCS(waitpid wait3)
AC_CHECK_FUNCS(strtoll)
AC_CHECK_FUNCS(open_memstream)
AC_CHECK_FUNCS(getline,[],AC_SUBST([GETLINE],['bannertopdf-getline.$(OBJEXT)']))
AC_CHECK_FUNCS(strcasestr,[],AC_SUBST([STRCASESTR],['pdftops-strcasestr.$(OBJEXT)']))
AC_SEARCH_LIBS(pow, m)
dnl Checks for string functions.
AC_CHECK_FUNCS(strdup strlcat strlcpy)
if test "$host_os_name" = "hp-ux" -a "$host_os_version" = "1020"; then
        echo Forcing snprintf emulation for HP-UX.
else
        AC_CHECK_FUNCS(snprintf vsnprintf)
fi

# ========================
# Check for system headers
# ========================
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([sys/stat.h])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([zlib.h])
AC_CHECK_HEADERS([endian.h])
AC_CHECK_HEADERS([dirent.h])
AC_CHECK_HEADERS([sys/ioctl.h])
AC_CHECK_HEADER(string.h,AC_DEFINE(HAVE_STRING_H))
AC_CHECK_HEADER(strings.h,AC_DEFINE(HAVE_STRINGS_H))

# ===================================
# Check for large files and long long
# ===================================
AC_SYS_LARGEFILE
LARGEFILE=""
AS_IF([test x"$enable_largefile" != "xno"], [
	LARGEFILE="-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE"
	AS_IF([test x"$ac_cv_sys_large_files" = "x1"], [LARGEFILE="$LARGEFILE -D_LARGE_FILES"])
	AS_IF([test x"$ac_cv_sys_file_offset_bits" = "x64"], [LARGEFILE="$LARGEFILE -D_FILE_OFFSET_BITS=64"])
])
AC_SUBST(LARGEFILE)

AC_CHECK_TYPE(long long, [long_long_found=yes], [long_long_found=no])
AS_IF([test x"$long_long_found" = "xyes"], [
	AC_DEFINE([HAVE_LONG_LONG], [], [Platform supports long long type])
])

# ================
# Check for CFLAGS
# ================
AC_ARG_ENABLE([werror],
	[AS_HELP_STRING([--enable-werror], [Treat all warnings as errors, useful for development.])],
	[enable_werror="$enableval"],
	[enable_werror=no]
)
AS_IF([test x"$enable_werror" = "xyes"], [
	CFLAGS="$CFLAGS -Werror"
])
AS_IF([test x"$GCC" = "xyes"], [
	# Be tough with warnings and produce less careless code
	CFLAGS="$CFLAGS -Wall -std=gnu11"
	CXXFLAGS="$CXXFLAGS -Wall " # -Weffc++" # TODO: enable when it does not print 1MB of warnings
])
CFLAGS="$CFLAGS -D_GNU_SOURCE"
CXXFLAGS="$CXXFLAGS -D_GNU_SOURCE"

# ==========================
# Braille embossing/liblouis
# ==========================
AC_ARG_ENABLE(braille, AS_HELP_STRING([--enable-braille],[enable Braille embosing filters, requires liblouis]),
	      enable_braille=$enableval,enable_braille=yes)
AC_MSG_CHECKING(for liblouis)
PKG_CHECK_EXISTS([liblouis], [
	AC_MSG_RESULT(yes)
	if test "x$enable_braille" = xyes; then
		TABLESDIR=`$PKG_CONFIG --variable=tablesdir liblouis`
	else
		TABLESDIR=/usr/share/liblouis/tables
	fi
], [
	AC_MSG_RESULT(no)
	TABLESDIR=/usr/share/liblouis/tables
])
AM_CONDITIONAL(ENABLE_BRAILLE, test "x$enable_braille" = xyes)
AC_SUBST(TABLESDIR)

# =====================
# Prepare all .in files
# =====================
AC_CONFIG_FILES([
	Makefile
	driver/index/indexv4.sh
	driver/index/indexv3.sh
	driver/index/index.sh
	driver/index/textbrftoindexv3
	driver/index/imageubrltoindexv3
	driver/index/imageubrltoindexv4
	driver/generic/brftoembosser
	filter/cups-braille.sh
	filter/imagetobrf
	filter/texttobrf
	filter/brftopagedbrf
	filter/vectortopdf
	filter/vectortobrf
	filter/musicxmltobrf
	filter/liblouis1.defs.gen
])
AC_CONFIG_COMMANDS([executable-scripts], [
	chmod +x filter/liblouis1.defs.gen
])
AC_OUTPUT

# ==============================================
# Display final informations about configuration
# ==============================================
AC_MSG_NOTICE([
==============================================================================
Environment settings:
	CFLAGS:                                    ${CFLAGS}
	CXXFLAGS:                                  ${CXXFLAGS}
	LDFLAGS:                                   ${LDFLAGS}
Build configuration:
	cups-config:     ${with_cups_config}
	braille:	 ${enable_braille}
	braille tables:  ${TABLESDIR}
	werror:          ${enable_werror}
==============================================================================
])
