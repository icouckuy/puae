cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_SOURCE_DIR}/src/include ${CMAKE_SOURCE_DIR}/src )


########### next target ###############

SET(machdep_STAT_SRCS
   support.c
)

add_library(machdep STATIC ${machdep_STAT_SRCS})


########### install files ###############




#original Makefile.am contents follow:

#INCLUDES =  -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_srcdir)
#
#AM_CCASFLAGS = $(INCLUDES)
#
#noinst_LIBRARIES = libmachdep.a
#
#libmachdep_a_SOURCES = support.c
#
#noinst_HEADERS = machdep.h m68k.h m68kops.h maccess.h rpt.h
