AC_INIT([paukn],[0.0.0])

dnl Use the m4/ directory to contain libtool macros that will be needed to 
dnl go with the ltmain.sh script (both will be created/updated via the command
dnl libtoolize --copy --force
AC_CONFIG_MACRO_DIR([m4])

#------------------------------------------------------------------------------------
# Library's "official" release version:

LIBPAUKN_RELEASE_MAJOR=0
LIBPAUKN_RELEASE_MINOR=0
LIBPAUKN_RELEASE_BUILD=0
AC_CONFIG_SRCDIR([src/paukn.cpp])
AC_CANONICAL_TARGET([])
AM_CONFIG_HEADER(src/config.h)

AC_DISABLE_SHARED dnl allows for optimizations

#------------------------------------------------------------------------------------
# The following is the libtool / shared library version. This doesn't have to
# do anything with the release version. It MUST conform to the following rules:
#
#  1. Start with version information of `0:0:0' for each libtool library.
#  2. Update the version information only immediately before a public release of
#     your software. More frequent updates are unnecessary, and only guarantee
#     that the current interface number gets larger faster.
#  3. If the library source code has changed at all since the last update, then
#     increment revision (`c:r:a' becomes `c:r+1:a').
#  4. If any interfaces have been added, removed, or changed since the last update,
#     increment current, and set revision to 0.
#  5. If any interfaces have been added since the last public release, then increment
#     age.
#  6. If any interfaces have been removed since the last public release, then set age
#     to 0.

SHARED_VERSION_INFO="0:0:0"

AC_PROG_LIBTOOL
AM_PROG_LIBTOOL

# save $CXXFLAGS etc. since AC_PROG_CXX likes to insert "-g -O2"
# if $CXXFLAGS is blank and it finds GCC
cflags_save="$CFLAGS"
cppflags_save="$CPPFLAGS"
cxxflags_save="$CXXFLAGS"
AC_LANG([C++])
AC_PROG_CXX
AC_PROG_CXXCPP

# restore those variables back
CFLAGS="$cflags_save"
CPPFLAGS="$cppflags_save"
CXXFLAGS="$cxxflags_save"

AC_C99_FUNC_LRINT()
AC_C99_FUNC_LRINTF()

AC_SUBST(SHLIB_VERSION_ARG)
AC_SUBST(SHARED_VERSION_INFO)

AC_ARG_ENABLE(static,[  --enable-static enable static build: (default: disable)],[enable_static=$enableval],[enable_static=no])

AC_ARG_ENABLE(debug,[  --enable-debug enable debug build: (default: disable)],[enable_debug=$enableval],[enable_debug=no])

AC_ARG_ENABLE(universal_binary,[  --enable-universal_binary enable universal binary build: (default: disable)],[enable_universal_binary=$enableval],[enable_universal_binary=no])

AC_ARG_ENABLE(vst,[  --enable-vst enable VST support (default: disable)],[enable_vst=$enableval],[enable_vst=no])

if test x$enable_vst = xyes; then
   AC_SUBST(VST_CFLAGS)
   AC_SUBST(VST_LIBS)
   AC_SUBST(VSTSDK_DIR)
   AC_SUBST(VSTSDKROOT_DIR)
   AC_SUBST(VSTGUI_DIR)
   AC_DEFINE(HAVE_VST,1,[Define to use libvst])
fi
AM_CONDITIONAL(ENABLE_VST, test x$enable_vst = xyes)

PAUKN_CFLAGS="-ffast-math -funroll-loops"

if test x$enable_static = xno; then
  case "$target_os" in
     darwin*)
	PAUKN_CFLAGS="$PAUKN_CFLAGS -mdynamic-no-pic"
	;;
	*)
	;;
   esac
fi

if test x$enable_debug = xyes; then
   PAUKN_CFLAGS="$PAUKN_CFLAGS -g -O3"
else
   PAUKN_CFLAGS="$PAUKN_CFLAGS -O3 -fomit-frame-pointer"
fi

if test x$enable_universal_binary = xyes; then
   case "$target_os" in
     darwin*)
        PAUKN_CFLAGS="$PAUKN_CFLAGS -mmacosx-version-min=10.4 -arch i386 -arch ppc -isysroot /Developer/SDKs/MacOSX10.4u.sdk"
	;;
     *)
	;;
   esac
fi

AC_SUBST(PAUKN_CFLAGS)
AC_SUBST(VST_CFLAGS)
AC_SUBST(VST_LIBS)
AC_SUBST(VSTSDK_DIR)
AC_SUBST(VSTSDKROOT_DIR)
AC_SUBST(VSTGUI_DIR)
AC_DEFINE(HAVE_VST,1,[Define to use libvst])

AM_INIT_AUTOMAKE([])

AM_MAINTAINER_MODE()

AC_CONFIG_FILES([Makefile src/Makefile libpaukn.spec paukn.pc])
AC_OUTPUT
