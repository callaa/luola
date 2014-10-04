INCLUDES = @SDL_CFLAGS@ -I$(top_srcdir)/src
LIBS = @LIBS@ @SDL_LIBS@

EXTRA_DIST = README.tools vwing.lev wings.lev

noinst_PROGRAMS = mkecompact ldat lcmap importlev

mkecompact_SOURCES = mkecompact.c lconf2bin.c lconf2bin.h \
					$(top_srcdir)/src/ldat.c $(top_srcdir)/src/lconf.c \
					$(top_srcdir)/src/stringutil.c $(top_srcdir)/src/sbmp.c

mkecompact_LDADD = -lSDL_image

ldat_SOURCES = ldatar.c archive.c archive.h \
			$(top_srcdir)/src/ldat.c $(top_srcdir)/src/stringutil.c

lcmap_SOURCES = lcmaptool.c $(top_srcdir)/src/lcmap.c

lcmap_LDADD = -lSDL_image

importlev_SOURCES = importlev.c importlev.h lconf2bin.c lconf2bin.h \
					$(top_srcdir)/src/ldat.c $(top_srcdir)/src/lconf.c \
					$(top_srcdir)/src/stringutil.c $(top_srcdir)/src/sbmp.c \
					$(top_srcdir)/src/lcmap.c \
					im_vwing.c im_vwing.h \
					im_wings.c im_wings.h \
					im_tou.c im_tou.h
