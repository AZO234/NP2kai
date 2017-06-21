dnl
dnl Configure for Xnp2
dnl

AC_PREREQ(2.69)

AC_INIT(Xnp2,0.86,nonakap@gmail.com,xnp2)
AM_INIT_AUTOMAKE([no-define no-dist no-installinfo subdir-objects])
AC_CONFIG_SRCDIR([../np2ver.h])
AC_CONFIG_HEADERS(config.h)
AC_CANONICAL_HOST

dnl
dnl Checks for programs.
dnl
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_GCC_TRADITIONAL
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S

dnl
dnl Checks for header files.
dnl
AC_STDC_HEADERS
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h malloc.h stddef.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h termios.h unistd.h])

dnl
dnl Checks for typedefs, structures, and compiler characteristics.
dnl
AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_C_VOLATILE
AC_CHECK_TYPES([ptrdiff_t])
AC_SYS_LARGEFILE
AC_C_CHAR_UNSIGNED

dnl
dnl Check GCC
dnl
AM_CONDITIONAL(HAVE_GCC,test x"$GCC" = "xyes")

dnl
dnl Checks for library functions.
dnl
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([gettimeofday memset mkdir strcasecmp strdup strerror strtol])

dnl
dnl Checks for math library functions.
dnl
AC_CHECK_FUNCS(floor,,AC_CHECK_LIB(m,floor))
AC_CHECK_FUNCS(pow,,AC_CHECK_LIB(m,pow))
AC_CHECK_FUNCS(sqrt,,AC_CHECK_LIB(m,sqrt))

dnl
dnl Checks for pthread
dnl
ACX_PTHREAD
LIBS="$LIBS $PTHREAD_LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"

dnl
dnl Checks for X
dnl
AC_PATH_X
AC_PATH_XTRA

dnl
dnl Xnp2 configure options.
dnl
AC_ARG_ENABLE(gtk3,
  AC_HELP_STRING([--enable-gtk3],[Use GTK+-3 library [default=no]]),,
  [enable_gtk3="no"])

AC_ARG_ENABLE(sdl,
  AC_HELP_STRING([--enable-sdl],[Use SDL 1.2 library [default=yes]]),,
  [enable_sdl="yes"])
AC_ARG_ENABLE(sdl2,
  AC_HELP_STRING([--enable-sdl2],[Use SDL 2.0 library [default=yes]]),,
  [enable_sdl2="yes"])
AC_ARG_ENABLE(sdlmixer,
  AC_HELP_STRING([--enable-sdlmixer],[Use SDL_mixer library [default=yes]]),,
  [enable_sdlmixer="yes"])
AC_ARG_ENABLE(libusb1,
  AC_HELP_STRING([--enable-libusb1],[Use libusb-1.0 library [default=yes]]),,
  [enable_libusb1="yes"])
AC_ARG_ENABLE(xf86vidmode,
  AC_HELP_STRING([--enable-xf86vidmode],[Use XF86VidMode extension [default=yes]]),,
  [enable_xf86vidmode="yes"])
AC_ARG_ENABLE(ia32,
  AC_HELP_STRING([--enable-ia32],[Use IA-32 emulation [default=no]]),,
  [enable_ia32="no"])
AC_ARG_ENABLE(build-all,
  AC_HELP_STRING([--enable-build-all],[Build xnp2(i286 emulation) and xnp21(IA-32 emulation) programs [default=no]]),,
  [enable_build_all="no"])
AC_ARG_ENABLE(debug,
  AC_HELP_STRING([--enable-debug],[Enable debugging [default=no]]),,
  [enable_debug="no"])
AC_ARG_ENABLE(warning,
  AC_HELP_STRING([--enable-warning],[Enable warning [default=no]]),,
  [enable_warning="no"])
AC_ARG_ENABLE(gtk-deprecated-api,
  AC_HELP_STRING([--enable-gtk-deprecated-api],[Enable GTK+ deprecated API [default=yes]]),,
  [enable_gtk_deprecated_api="yes"])

dnl
dnl Checks for X11
dnl
PKG_CHECK_MODULES([X11],[x11],,[AC_MSG_ERROR(Xnp2 needs X11)])
AC_SUBST(X11_CFLAGS)
AC_SUBST(X11_LIBS)
PKG_CHECK_MODULES([XEXT],[xext])
AC_SUBST(XEXT_CFLAGS)
AC_SUBST(XEXT_LIBS)

dnl
dnl Checks for GTK+
dnl
AS_IF([test x"$enable_gtk3" = "xyes"],
  [
    PKG_CHECK_MODULES([GTK], [gtk+-3.0],
      [
        AC_DEFINE(USE_GTK2,1,[Define to 1 if you have the GTK+-2 library.])
        AC_DEFINE(USE_GTK3,1,[Define to 1 if you have the GTK+-3 library.])
      ],
      enable_gtk3=no
    )
  ]
)
AS_IF([test x"$enable_gtk3" != "xyes"],
  [
    PKG_CHECK_MODULES([GTK], [gtk+-2.0],
      [AC_DEFINE(USE_GTK2,1,[Define to 1 if you have the GTK+-2 library.])]
    )
  ]
)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

dnl
dnl Checks for SDL and SDL_mixer
dnl
no_sdl=yes
if test x"$enable_sdl2" = "xyes"; then
  AM_PATH_SDL2(2.0.0)
  if test x"$no_sdl" != "xyes"; then
    AC_DEFINE(USE_SDLAUDIO,,[Define if you have the SDL 1.2/2.0 library.])
    if test x"$enable_sdlmixer" = "xyes"; then
      AC_CHECK_LIB(SDL2_mixer,Mix_OpenAudio,
        AC_DEFINE(USE_SDLMIXER,,[Define if you have the SDL_mixer library.])
        SDL_LIBS="$SDL_LIBS -lSDL2_mixer",,$SDL_LIBS)
    fi
  fi
fi
if test x"$no_sdl" = "xyes" -a x"$enable_sdl" = "xyes"; then
  AM_PATH_SDL(1.2.0)
  if test x"$no_sdl" != "xyes"; then
    AC_DEFINE(USE_SDLAUDIO,,[Define if you have the SDL 1.2/2.0 library.])
    if test x"$enable_sdlmixer" = "xyes"; then
      AC_CHECK_LIB(SDL_mixer,Mix_OpenAudio,
        AC_DEFINE(USE_SDLMIXER,,[Define if you have the SDL_mixer library.])
        SDL_LIBS="$SDL_LIBS -lSDL_mixer",,$SDL_LIBS)
    fi
  fi
fi

dnl
dnl Checks for libusb-1.0
dnl
if test x"$enable_libusb1" = "xyes"; then
  PKG_CHECK_MODULES([LIBUSB1], [libusb-1.0],
    [AC_DEFINE(USE_LIBUSB1,1,[Define to 1 if you have the libusb-1.0 library.])]
  )
fi
AC_SUBST(LIBUSB1_CFLAGS)
AC_SUBST(LIBUSB1_LIBS)

dnl
dnl Checks for XF86VidMode extension
dnl
if test x"$enable_xf86vidmode" = "xyes"; then
  ac_cv_save_cflags=$CFLAGS
  CFLAGS="$CFLAGS $X_CFLAGS"
  AC_CHECK_LIB(Xext,XextCreateExtension,
    XLIB="-lXext $XLIB",,[$X_LIBS $X_PRE_LIBS $XLIB $X_EXTRA_LIBS])
  AC_CHECK_HEADERS(X11/extensions/xf86vmode.h,
    [AC_CHECK_LIB(Xxf86vm,XF86VidModeQueryExtension,
      [X_PRE_LIBS="$X_PRE_LIBS -lXxf86vm"
      AC_DEFINE(HAVE_XF86VIDMODE,1,[Define to 1 if you have the libXxf86vm library.])],,
      [$X_LIBS $X_PRE_LIBS $XLIB $X_EXTRA_LIBS])],
    ,
    [#include <X11/Xlib.h>])
  CFLAGS=$ac_cv_save_cflags
fi

dnl
dnl Checks for nasm
dnl
AC_CHECK_PROGS(nasm,[nasm nasmw],no)

dnl
dnl Checks for build all programs
dnl
AC_MSG_CHECKING([whether to build all programs])
AM_CONDITIONAL(BUILD_ALL,test x"$enable_build_all" = "xyes")
if test x"$enable_build_all" = "xyes"; then
  if test x"$enable_ia32" != "xyes"; then
    enable_ia32="yes"
  fi
fi
AC_MSG_RESULT($enable_build_all)

dnl
dnl Checks for IA-32 CPU emulation
dnl
AC_MSG_CHECKING([whether to use IA-32 emulation])
AM_CONDITIONAL(CPUCORE_IA32,test x"$enable_ia32" = "xyes")
AC_MSG_RESULT($enable_ia32)

dnl
dnl Checks for debug mode
dnl
AC_MSG_CHECKING([whether to enable debugging])
if test x"$enable_debug" = "xyes"; then
  CPPFLAGS="$CPPFLAGS -g -DDEBUG -DTRACE"
  CFLAGS="$CFLAGS -Wstack-protector -fstack-protector --param ssp-buffer-size=1"
  CXXFLAGS="$CXXFLAGS -Wstack-protector -fstack-protector --param ssp-buffer-size=1"
  enable_warning=yes
else
  CPPFLAGS="$CPPFLAGS -DNDEBUG"
fi
AC_MSG_RESULT($enable_debug)

dnl
dnl Checks for warning
dnl
AC_MSG_CHECKING([whether to enable warning])
if test x"$enable_warning" = "xyes"; then
  CFLAGS="$CFLAGS -Wall -Wextra"
  CFLAGS="$CFLAGS -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith"
  CFLAGS="$CFLAGS -Wreturn-type -Wswitch -Wshadow"
  CFLAGS="$CFLAGS -Wcast-qual -Wwrite-strings"
  CFLAGS="$CFLAGS -Wno-unused-parameter -Wno-missing-field-initializers"
  CFLAGS="$CFLAGS -Wformat=2"
  CXXFLAGS="$CXXFLAGS -Wall -Wextra"
  CXXFLAGS="$CXXFLAGS -Wpointer-arith"
  CXXFLAGS="$CXXFLAGS -Wreturn-type -Wswitch -Wshadow"
  CXXFLAGS="$CXXFLAGS -Wcast-qual -Wwrite-strings"
  CXXFLAGS="$CXXFLAGS -Wno-unused-parameter -Wno-missing-field-initializers"
  CXXFLAGS="$CXXFLAGS -Wformat=2"
fi
AC_MSG_RESULT($enable_warning)

dnl
dnl Checks for gtk-deprecated-api
dnl
AC_MSG_CHECKING([whether to enable GTK+ deprecated API])
if test x"$enable_gtk_deprecated_api" != "xyes"; then
  GTK_CFLAGS="$GTK_CFLAGS -DGTK_DISABLE_DEPRECATED"
  GTK_CFLAGS="$GTK_CFLAGS -DGDK_DISABLE_DEPRECATED"
  GTK_CFLAGS="$GTK_CFLAGS -DGLIB_DISABLE_DEPRECATED"
  CPPFLAGS="$CPPFLAGS -Werror=deprecated-declarations"
fi
AC_MSG_RESULT($enable_gtk_deprecated_api)

dnl
dnl Checks for Xnp2 version
dnl
NP2VER_X11=`echo $PACKAGE_VERSION | $AWK 'BEGIN {FS="-"} NF==2 {print $2}'`
if test x"$NP2VER_X11" != "x"; then
  NP2VER_X11="\"-$NP2VER_X11\""
  AC_DEFINE_UNQUOTED(NP2VER_X11,$NP2VER_X11,[Define if Xnp2 version is available])
fi

dnl
dnl Output Makefiles
dnl
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
