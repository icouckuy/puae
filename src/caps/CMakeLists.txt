cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_SOURCE_DIR}/src/include ${CMAKE_SOURCE_DIR}/src )


########### next target ###############

SET(caps_STAT_SRCS
   caps.c
)

add_library(caps STATIC ${caps_STAT_SRCS})


########### install files ###############

#original Makefile.am contents follow:

#INCLUDES =  -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_srcdir)
#
#noinst_LIBRARIES = libcaps.a
#
#libcaps_a_SOURCES = caps.c
