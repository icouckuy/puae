cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_SOURCE_DIR}/src/include ${CMAKE_SOURCE_DIR}/src )


########### next target ###############

SET(dms_STAT_SRCS
   crc_csum.c
   getbits.c
   maketbl.c
   pfile.c
   tables.c
   u_deep.c
   u_heavy.c
   u_init.c
   u_medium.c
   u_quick.c
   u_rle.c
)

add_library(dms STATIC ${dms_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

#INCLUDES =  -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_srcdir)
#
#noinst_LIBRARIES = libdms.a
#
#libdms_a_SOURCES = crc_csum.c  getbits.c  maketbl.c  pfile.c  tables.c \
#		   u_deep.c  u_heavy.c  u_init.c  u_medium.c  u_quick.c \
#		   u_rle.c
#
#noinst_HEADERS   = cdata.h crc_csum.h getbits.h maketbl.h pfile.h tables.h \
#		   u_deep.h u_heavy.h u_init.h u_medium.h u_quick.h u_rle.h
