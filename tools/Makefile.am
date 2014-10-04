INCLUDES = @SDL_CFLAGS@ -I$(top_srcdir)/src
LIBS = @LIBS@ @SDL_LIBS@

EXTRA_DIST = README.tools vwing.lev wings.lev mkthumbnail.sh

noinst_PROGRAMS = ldat lcmap importlev

ldat_SOURCES = ldatar.c archive.c archive.h \
			$(top_srcdir)/src/ldat.c $(top_srcdir)/src/parser.c \
			$(top_srcdir)/src/list.c

lcmap_SOURCES = lcmaptool.c $(top_srcdir)/src/lcmap.c

lcmap_LDADD = -lSDL_image

importlev_SOURCES = importlev.c importlev.h \
					$(top_srcdir)/src/ldat.c \
					$(top_srcdir)/src/lcmap.c \
					thumbnail.c thumbnail.h \
					im_vwing.c im_vwing.h \
					im_wings.c im_wings.h \
					im_tou.c im_tou.h
