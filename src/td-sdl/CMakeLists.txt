cmake_minimum_required(VERSION 2.6)

FIND_PACKAGE(SDL REQUIRED)

include_directories(${SDL_INCLUDE_DIR} )
include_directories(${CMAKE_SOURCE_DIR}/src/include ${CMAKE_SOURCE_DIR}/src )


########### next target ###############

SET(threaddep_STAT_SRCS
   thread.c
)

add_library(threaddep STATIC ${threaddep_STAT_SRCS})
target_link_libraries(threaddep ${SDL_LIBRARY} )

########### install files ###############




#original Makefile.am contents follow:

#INCLUDES =  -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_srcdir)
#
#noinst_LIBRARIES = libthreaddep.a
#
#libthreaddep_a_SOURCES = thread.c
#
#noinst_HEADERS = thread.h
