cmake_minimum_required(VERSION 2.6)

FIND_PACKAGE(SDL REQUIRED)
FIND_PACKAGE(SDL_sound REQUIRED)

MESSAGE(STATUS "SDL lib: " ${SDL_LIBRARY} ${SDL_SOUND_LIBRARIES} )

include_directories(${SDL_INCLUDE_DIR} ${SDL_SOUND_INCLUDE_DIR} )
include_directories(${CMAKE_SOURCE_DIR}/src/include ${CMAKE_SOURCE_DIR}/src )


########### next target ###############

SET(snddep_STAT_SRCS
   sound.c
)

add_library(snddep STATIC ${snddep_STAT_SRCS})
#target_link_libraries(snddep ${SDL_LIBRARY} ${SDL_SOUND_LIBRARIES} )

########### install files ###############




#original Makefile.am contents follow:

#INCLUDES =  -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_srcdir)
#
#noinst_LIBRARIES = libsnddep.a
#
#libsnddep_a_SOURCES = sound.c
#
#noinst_HEADERS = sound.h
