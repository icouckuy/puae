cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_SOURCE_DIR}/src/include ${CMAKE_SOURCE_DIR}/src )


########### next target ###############

SET(keymap_STAT_SRCS
   keymap.c
   x11pc_rawkeys.c
   quartz_rawkeys.c
   beos_rawkeys.c
   amiga_rawkeys.c
)

add_library(keymap STATIC ${keymap_STAT_SRCS})

########### install files ###############


#original Makefile.am contents follow:

#INCLUDES =  -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_srcdir)
#
#noinst_LIBRARIES = libkeymap.a
#
#libkeymap_a_SOURCES = keymap.c x11pc_rawkeys.c quartz_rawkeys.c beos_rawkeys.c \
#		      amiga_rawkeys.c
#
#noinst_HEADERS = keymap.h x11pc_rawkeys.h quartz_rawkeys.h beos_rawkeys.h \
#	         amiga_rawkeys.h keymap_common.h keymap_all.h hotkeys_common.h
