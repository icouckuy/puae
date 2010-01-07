cmake_minimum_required(VERSION 2.6)

FIND_PACKAGE(SDL REQUIRED)

include_directories(${SDL_INCLUDE_DIR} )
include_directories(${CMAKE_SOURCE_DIR}/src/include ${CMAKE_SOURCE_DIR}/src )

########### next target ###############

SET(gfxdep_STAT_SRCS
   sdlgfx.c
   sdlkeys.c
   rawkeys.c
)

add_library(gfxdep STATIC ${gfxdep_STAT_SRCS})
target_link_libraries(gfxdep ${SDL_LIBRARY})

########### install files ###############

#original Makefile.am contents follow:

#INCLUDES =  -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_srcdir)
#
#noinst_LIBRARIES = libgfxdep.a
#
#libgfxdep_a_SOURCES = sdlgfx.c sdlkeys.c rawkeys.c
#
#noinst_HEADERS = gfx.h sdlgfx.h
