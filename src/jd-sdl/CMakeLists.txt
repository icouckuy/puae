cmake_minimum_required(VERSION 2.6)

FIND_PACKAGE(SDL REQUIRED)

include_directories(${SDL_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/include ${CMAKE_SOURCE_DIR}/src )


########### next target ###############

SET(joydep_STAT_SRCS
   joystick.c
)

add_library(joydep STATIC ${joydep_STAT_SRCS})
target_link_libraries(joydep ${SDL_LIBRARY} )


########### install files ###############

#original Makefile.am contents follow:

#INCLUDES =  -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_srcdir)
#
#noinst_LIBRARIES = libjoydep.a
#
#libjoydep_a_SOURCES = joystick.c
#
