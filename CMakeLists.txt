cmake_minimum_required(VERSION 2.6)

PROJECT( puae )

# configure stage
SET(PACKAGE_NAME "\"puae\"" CACHE STRING "Name")
SET(UAE_VERSION 2.0.2 CACHE STRING "Version")
SET(UAE_VERSION_MAJOR 2 CACHE STRING "Major version")
SET(UAE_VERSION_MINOR 0 CACHE STRING "Minor version")
SET(UAE_VERSION_REVISION 2 CACHE STRING "Revision")
SET(UAE_VERSION_TAG tag CACHE STRING "Tag")

# types
INCLUDE (CheckTypeSize)
CHECK_TYPE_SIZE("char" SIZEOF_CHAR)
CHECK_TYPE_SIZE("int" SIZEOF_INT)
CHECK_TYPE_SIZE("long" SIZEOF_LONG)
CHECK_TYPE_SIZE("long long" SIZEOF_LONG_LONG)
CHECK_TYPE_SIZE("short int" SIZEOF_SHORT)
CHECK_TYPE_SIZE("void *" SIZEOF_VOID_P)
CHECK_TYPE_SIZE("__int64" SIZEOF___INT64)

INCLUDE (CheckIncludeFiles)
#CHECK_INCLUDE_FILES ("sys/param.h;sys/mount.h" HAVE_SYS_MOUNT_H)
CHECK_INCLUDE_FILES (byteswap.h HAVE_BYTESWAP_H) # used in src/include/uae_endian.h
CHECK_INCLUDE_FILES (dirent.h HAVE_DIRENT_H) # used in src/include/sysdeps.h
CHECK_INCLUDE_FILES (dustat.h HAVE_DUSTAT_H) # used in src/fsusage.c
CHECK_INCLUDE_FILES (fcntl.h HAVE_FCNTL_H) # used in src/fsusage.c and src/include/sysdeps.h
CHECK_INCLUDE_FILES (libraries/cybergraphics.h HAVE_LIBRARIES_CYBERGRAPHICS_H) # used in src/gfx-amigaos/ami-win.c
CHECK_INCLUDE_FILES (machine/joystick.h HAVE_MACHINE_JOYSTICK_H) # used in src/jd-linuxold/joystick.c
CHECK_INCLUDE_FILES (machine/soundcard.h HAVE_MACHINE_SOUNDCARD_H) # used in src/sd-uss/sound.c
CHECK_INCLUDE_FILES (ncurses.h HAVE_NCURSES_H) # used in src/gfx-curses/ncurses.c and src/svgancui.c
CHECK_INCLUDE_FILES (strings.h HAVE_STRINGS_H) # used in src/include/uae_string.h
CHECK_INCLUDE_FILES (string.h HAVE_STRING_H) # used in src/include/uae_string.h
CHECK_INCLUDE_FILES (utime.h HAVE_UTIME_H) # used in src/include/sysdeps.h
#CHECK_INCLUDE_FILES (sys/stat.h HAVE_SYS_STAT_H) # used in src/filesys_unix.c and src/filesys_unix.c and src/fsusage.c
#CHECK_INCLUDE_FILES (sys/statfs.h HAVE_SYS_STATFS_H) # used in TODO
CHECK_INCLUDE_FILES (sys/statvfs.h HAVE_SYS_STATVFS_H) # used in TODO

#CHECK_INCLUDE_FILES (inttypes.h HAVE_INTTYPES_H) NOT NEEDED (grep'ed)
#CHECK_INCLUDE_FILES (cybergraphx/cybergraphics.h HAVE_CYBERGRAPHX_CYBERGRAPHICS_H)  NOT NEEDED (grep'ed)
#CHECK_INCLUDE_FILES (devices/ahi.h HAVE_DEVICES_AHI_H) NOT NEEDED (grep'ed)
#CHECK_INCLUDE_FILES (caps/capsimage.h HAVE_CAPS_CAPSIMAGE_H) NOT NEEDED (grep'ed)
#CHECK_INCLUDE_FILES (curses.h HAVE_CURSES_H) # NOT NEEDED (ncurses var is used)
#CHECK_INCLUDE_FILES (stdint.h HAVE_STDINT_H) NOT NEEDED (grep'ed)
#CHECK_INCLUDE_FILES (stdlib.h HAVE_STDLIB_H) NOT NEEDED (gre'ped)
#CHECK_INCLUDE_FILES (memory.h HAVE_MEMORY_H) NOT NEEDED (grep'ed)

INCLUDE (CheckFunctionExists)
SET(CMAKE_REQUIRED_LIBRARIES -ldl)
CHECK_FUNCTION_EXISTS (dlopen HAVE_DLOPEN) # used in src/caps/caps.c
CHECK_FUNCTION_EXISTS (gettimeofday HAVE_GETTIMEOFDAY) # used in lots of places
CHECK_FUNCTION_EXISTS (gmtime_r HAVE_GMTIME_R) # used in src/filesys.c
CHECK_FUNCTION_EXISTS (isinf HAVE_ISINF) # used in src/fpp.c
CHECK_FUNCTION_EXISTS (isnan HAVE_ISNAN) # used in src/fpp.c
CHECK_FUNCTION_EXISTS (localtime_r HAVE_LOCALTIME_R) # used in src/filesys.c
CHECK_FUNCTION_EXISTS (nanosleep HAVE_NANOSLEEP) # used in src/include/sleep.h
CHECK_FUNCTION_EXISTS (readdir_r HAVE_READDIR_R) # used in src/filesys.c
CHECK_FUNCTION_EXISTS (sigaction HAVE_SIGACTION) # used in src/od-amiga/ami-disk.c and src/od-amiga/main.c
CHECK_FUNCTION_EXISTS (strcasecmp HAVE_STRCASECMP) # used in src/include/uae_string.h
CHECK_FUNCTION_EXISTS (strcmpi HAVE_STRCMPI) # used in src/include/uae_string.h
CHECK_FUNCTION_EXISTS (strdup HAVE_STRDUP) # used in src/include/uae_string.h and src/missing.c
CHECK_FUNCTION_EXISTS (stricmp HAVE_STRICMP) # used in src/include/uae_string.h
#CHECK_FUNCTION_EXISTS (statfs HAVE_SYS_STATFS) # used in src/filesys_unix.c and src/fsusage.c
CHECK_FUNCTION_EXISTS (statvfs HAVE_SYS_STATVFS) # used in src/filesys_unix.c and src/fsusage.c
#SET(STAT_STATFS4 "1") # hardcoded
SET(STAT_STATVFS "1")

#CHECK_FUNCTION_EXISTS (setitimer HAVE_SETITIMER) # used in md-i386-gcc/support.c (clashes with utime)
CHECK_FUNCTION_EXISTS (alarm HAVE_ALARM) # used in md-i386-gcc/support.c

#cmakedefine STAT_STATVFS
#CHECK_FUNCTION_EXISTS (setitimer HAVE_SETITIMER) # clashes with utime
#CHECK_FUNCTION_EXISTS (select HAVE_SELECT) NOT NEEDED (grep'ed)
#CHECK_FUNCTION_EXISTS (sleep HAVE_SLEEP) NOT NEEDED (grep'ed)
#CHECK_FUNCTION_EXISTS (strerror HAVE_STRERROR) NOT NEEDED (grep'ed)
#CHECK_FUNCTION_EXISTS (strstr HAVE_STRSTR) NOT NEEDED (grep'ed)

INCLUDE (CheckSymbolExists)
CHECK_SYMBOL_EXISTS(DIR "ndir.h" HAVE_NDIR_H) # used in src/include/sysdeps.h

FIND_PACKAGE(ZLIB REQUIRED)

# options
ADD_DEFINITIONS(-DCPUEMU_0) # standard cpu emulation

OPTION(COMPATIBLE "Enable compatible CPU emulation" YES )
IF( COMPATIBLE )
    ADD_DEFINITIONS(-DCPUEMU_5)
ENDIF( COMPATIBLE )

OPTION(CYCLEEXACT "Enable cycle-exact CPU emulation" YES )
IF( CYCLEEXACT )
    ADD_DEFINITIONS(-DCPUEMU_6)
ENDIF( CYCLEEXACT )

OPTION(FPUEMU "Enable FPU emulation" YES)
IF (FPUEMU)
  #FindPackage(EXPAT)
  # Make sure expat was found
   #if(NOT EXPAT_FOUND)
   #   message(FATAL_ERROR "Expat was NOT found and is needed")
   #endif()
   ADD_DEFINITIONS(-DFPUEMU)
ELSE(FPUEMU)
   #add_subdirectory(${My_Expat_Directory} ${PROJECT_BINARY_DIR}/Expat)
ENDIF(FPUEMU)

OPTION(AGA "Enable AGA chipset emulation" YES )
IF( AGA )
    ADD_DEFINITIONS(-DAGA)
ENDIF( AGA )

OPTION(CD32 "Enable CD32 emulation" NO )
IF( CD32 )
    ADD_DEFINITIONS(-DCD32)
ENDIF( CD32 )

OPTION(CDTV "Enable CDTV emulation" NO )
IF( CDTV )
    ADD_DEFINITIONS(-DCDTV)
ENDIF( CDTV )

OPTION(ARCADIA "Enable CDTV emulation" NO )
IF( ARCADIA )
    ADD_DEFINITIONS(-DARCADIA)
ENDIF( ARCADIA )

OPTION(JIT "Enable JIT compiler (currently x86 only)" YES )
IF( JIT )
    ADD_DEFINITIONS(-DJIT)
ENDIF( JIT )

OPTION(NATMEM_OFFSET "Enable JIT direct memory support" YES )
IF( NATMEM_OFFSET )
    ADD_DEFINITIONS(-DNATMEM_OFFSET=0x50000000)
ENDIF( NATMEM_OFFSET )

OPTION(FDI "Enable FDI support" YES )
IF( FDI )
    ADD_DEFINITIONS(-DFDI2RAW)
ENDIF( FDI )

OPTION(SAVESTATE "Enable support for saving state snapshots" YES )
IF( SAVESTATE )
    ADD_DEFINITIONS(-DSAVESTATE)
ENDIF( SAVESTATE )

OPTION(ACTION_REPLAY "Enable Action Replay cartridge emulation" YES )
IF( ACTION_REPLAY )
    ADD_DEFINITIONS(-DACTION_REPLAY)
ENDIF( ACTION_REPLAY )

OPTION(ENFORCER "Enable ersatz Enforcer support" YES )
IF( ENFORCER )
    ADD_DEFINITIONS(-DENFORCER)
ENDIF( ENFORCER )

OPTION(DRVSND "Enable Floppy Drive Sound Emulation" YES )
IF( DRVSND )
    ADD_DEFINITIONS(-DDRIVESOUND)
ENDIF( DRVSND )

OPTION(XARCADE "Disable keymaps for X-Arcade joysticks" YES )
IF( XARCADE )
    ADD_DEFINITIONS(-DXARCADE)
ENDIF( XARCADE )

# standard ./configure defines
# -D_GNU_SOURCE=1 -D_REENTRANT  -DCPUEMU_0 -DCPUEMU_5 -DCPUEMU_6 -DFPUEMU -DUNALIGNED_PROFITABLE -DJIT -DNATMEM_OFFSET=0x50000000 -DAGA -DAUTOCONFIG -DFILESYS -DBSDSOCKET -DSUPPORT_THREADS -DUSE_SDL -DFDI2RAW -DDEBUGGER -DSAVESTATE -DENFORCER -DACTION_REPLAY -DXARCADE -DDRIVESOUND
# difference with current cmake one
#  -DNOINLINE=""
ADD_DEFINITIONS(
	-D_GNU_SOURCE=1
	-D_REENTRANT
	-DUNALIGNED_PROFITABLE
	-DAUTOCONFIG
	-DFILESYS
	-DBSDSOCKET
	-DSUPPORT_THREADS
	-DUSE_SDL
	-DDEBUGGER
#	-DRETSIGTYPE=void -> sysconfig
#	-DCPUEMU_20
#	-DNOINLINE=
#	-Wl,--noinhibit-exec
#	-Wa,--execstack
#	-fomit-frame-pointer
#	-Wall
#	-Wno-unused
#	-Wno-format
#	-W
#	-Wmissing-prototypes
#	-Wstrict-prototypes
#	-fno-strength-reduce
)

SET(MULTIPLICATION_PROFITABLE 1)
SET(HAVE_STRUCT_STAT_ST_BLOCKS 1)

#include dirs
# Filthy hack -> threads.h is included in gui-qt,src/main.c (among others), if td-sdl is used, we have to find the sdl headers
FIND_PACKAGE(SDL REQUIRED)
include_directories(${SDL_INCLUDE_DIR} )

#  CMAKE_USE_SPROC_INIT       - are we using sproc?
#  CMAKE_USE_WIN32_THREADS_INIT - using WIN32 threads?
#  CMAKE_USE_PTHREADS_INIT    - are we using pthreads
#  CMAKE_HP_PTHREADS_INIT     - are we using hp threads
FIND_PACKAGE(Threads REQUIRED)
#CMAKE_THREAD_LIBS_INIT     - the thread library
IF( CMAKE_USE_WIN32_THREADS_INIT )
	SET(THREADDEP "td-win32")
ELSE( CMAKE_USE_WIN32_THREADS_INIT )
	IF( CMAKE_USE_PTHREADS_INIT )
		SET(THREADDEP "td-posix")
	ELSE( CMAKE_USE_PTHREADS_INIT )
		SET(THREADDEP "td-sdl")
	ENDIF( CMAKE_USE_PTHREADS_INIT )
ENDIF( CMAKE_USE_WIN32_THREADS_INIT )

# TODO: make options + check for library availability
SET(GUI_DEP "gui-gtk")
SET(SND_DEP "sd-sdl")
SET(GFX_DEP "gfx-sdl")
SET(JOY_DEP "jd-sdl")

SET(MACHDEP "md-i386-gcc")
#SET(THREADDEP "td-sdl")
SET(OSDEP "od-linux")

SET(TARGETDEP "t-unix.h")
SET(MDFPP_H "fpp-ieee.h")

MESSAGE(STATUS "threads: " ${THREADDEP} )
MESSAGE(STATUS "threads library: " ${CMAKE_THREAD_LIBS_INIT} )

# options done, now create symlinks
# "${CMAKE_COMMAND} -E create_symlink"

EXECUTE_PROCESS(
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/src/targets/${TARGETDEP} ${CMAKE_SOURCE_DIR}/src/include/target.h
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/src/include/${MDFPP_H} ${CMAKE_SOURCE_DIR}/src/include/md-fpp.h

  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/src/${MACHDEP} ${CMAKE_SOURCE_DIR}/src/machdep
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/src/${OSDEP} ${CMAKE_SOURCE_DIR}/src/osdep
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/src/${THREADDEP} ${CMAKE_SOURCE_DIR}/src/threaddep
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/src/${GFX_DEP} ${CMAKE_SOURCE_DIR}/src/gfxdep
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/src/${SND_DEP} ${CMAKE_SOURCE_DIR}/src/sounddep
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/src/${JOY_DEP} ${CMAKE_SOURCE_DIR}/src/joydep
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/src/${GUI_DEP} ${CMAKE_SOURCE_DIR}/src/guidep
)

add_subdirectory(src)


########### install files ###############

install(FILES  bootstrap.sh DESTINATION misc)

#original Makefile.am contents follow:

#ACLOCAL_AMFLAGS = -I m4
#
#EXTRA_DIST = m4 amiga docs CHANGES README
#
#SUBDIRS = src
#
#dist_noinst_SCRIPTS = bootstrap.sh
#
#dist-hook:
#	rm -rf `find $(distdir) -name CVS`
#	rm -rf `find $(distdir) -name *.bak`
#	rm -rf `find $(distdir) -name *~`
#	rm -rf `find $(distdir) -name *.orig`
