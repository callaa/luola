# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([Luola], [1.3.2], [calle@luolamies.org])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([SDL_image], [IMG_Load])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit memset mkdir strcasecmp strchr strdup strrchr])

AC_ARG_ENABLE([sound],
	      [  --enable-sound	Enable sounds],
	      [sounds=$enableval],
	      [sounds=no])

if test "x$sounds" = "xyes"
then
	AC_CHECK_LIB([SDL_mixer], [main], [],
		     AC_MSG_WARN(SDL_mixer not found))
fi

AC_ARG_ENABLE([truetype],
	      [  --disable-truetype	Disable Truetype fonts],
	      [truetype=$enableval],
	      [truetype=yes])

if test "x$truetype" = "xyes"
then
	AC_CHECK_LIB([SDL_ttf], [main], [],
		     AC_MSG_WARN(SDL_ttf not found))
fi

AC_ARG_ENABLE([sdl-gfx],
	      [  --disable-sdl-gfx	Disable SDL_gfx],
	      [sdlgfx=$enableval],
	      [sdlgfx=yes])

if test "x$sdlgfx" = "xyes"
then
	AC_CHECK_LIB([SDL_gfx], [pixelColor], [],
		     AC_MSG_WARN(SDL_gfx not found))
fi

dnl Check for SDL
AM_PATH_SDL(1.1.5,,AC_MSG_ERROR([*** SDL >= 1.1.5 not installed - please install first ***]))
#AC_CHECK_LIB(pthread, main,, AC_MSG_ERROR(SDL needs pthread !))
AC_CHECK_LIB([SDL_image],[main],[],AC_MSG_ERROR(SDL_image is needed))
AC_CHECK_LIB([z],[main],[],AC_MSG_ERROR(libz is needed))

AC_CONFIG_FILES([Makefile
                 contrib/Makefile
                 data/Makefile
                 data/font/Makefile
                 data/levels/Makefile
                 src/Makefile
                 tools/Makefile])
AC_OUTPUT
