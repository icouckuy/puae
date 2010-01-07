cmake_minimum_required(VERSION 2.6)

FIND_PACKAGE(GTK2 COMPONENTS gtk REQUIRED)

include_directories(${GTK2_INCLUDE_DIRS} )
include_directories(${CMAKE_SOURCE_DIR}/src/include ${CMAKE_SOURCE_DIR}/src )

########### next target ###############

SET(guidep_STAT_SRCS
   gtkui.c
   util.c
   chooserwidget.c
   cputypepanel.c
   cpuspeedpanel.c
   floppyfileentry.c
   led.c
   chipsettypepanel.c
   chipsetspeedpanel.c
)

add_library(guidep STATIC ${guidep_STAT_SRCS})
target_link_libraries(guidep ${GTK2_LIBRARIES} )

MESSAGE(STATUS "gui-libs: " ${GTK2_LIBRARIES})

########### install files ###############

#original Makefile.am contents follow:

#INCLUDES =  -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_srcdir)
#
#AM_CFLAGS = @GTK_CFLAGS@
#
#noinst_LIBRARIES = libguidep.a
#
#libguidep_a_SOURCES = gtkui.c util.c chooserwidget.c cputypepanel.c \
#		      cpuspeedpanel.c floppyfileentry.c led.c \
#		      chipsettypepanel.c chipsetspeedpanel.c
#
#noinst_HEADERS = util.h chooserwidget.h cputypepanel.h cpuspeedpanel.h \
#		 floppyfileentry.h led.h chipsettypepanel.h chipsetspeedpanel.h
#
