cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_SOURCE_DIR}/src/include ${CMAKE_SOURCE_DIR}/src )


########### next target ###############

SET(osdep_STAT_SRCS
   main.c
   memory.c
   blkdev-linux.c
)

add_library(osdep STATIC ${osdep_STAT_SRCS})

########### install files ###############

#original Makefile.am contents follow:

#INCLUDES =  -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_srcdir)
#
#noinst_LIBRARIES = libosdep.a
#
#libosdep_a_SOURCES = main.c memory.c blkdev-linux.c
#
#noinst_HEADERS = memory.h hrtimer.h
