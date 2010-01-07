cmake_minimum_required(VERSION 2.6)

# generate sysconfig.h
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/sysconfig.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/sysconfig.h)

# include dirs
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/include ${CMAKE_SOURCE_DIR}/src/tools ${CMAKE_SOURCE_DIR}/src )

SET(genblitter_SRCS
        genblitter.c
        blitops.c
        writelog.c
)

SET(genlinetoscr_SRCS
        genlinetoscr.c
)

SET(build68k_SRCS
        build68k.c
        writelog.c
)

SET(gencpu_SRCS
        gencpu.c
        ${CMAKE_SOURCE_DIR}/src/cpudefs.c
        readcpu.c
        missing.c
        writelog.c
)

SET(gencomp_SRCS
        gencomp.c
        readcpu.c
        missing.c
        ${CMAKE_SOURCE_DIR}/src/cpudefs.c
        writelog.c
)

ADD_EXECUTABLE(genblitter ${genblitter_SRCS})
ADD_EXECUTABLE(genlinetoscr ${genlinetoscr_SRCS})
ADD_EXECUTABLE(build68k ${build68k_SRCS})
ADD_EXECUTABLE(gencomp ${gencomp_SRCS})
ADD_EXECUTABLE(gencpu ${gencpu_SRCS})

# subdirectories
ADD_SUBDIRECTORY(tools)
ADD_SUBDIRECTORY(${MACHDEP})
ADD_SUBDIRECTORY(${THREADDEP})
ADD_SUBDIRECTORY(${GFX_DEP})
ADD_SUBDIRECTORY(${SND_DEP})
ADD_SUBDIRECTORY(${JOY_DEP})
ADD_SUBDIRECTORY(${GUI_DEP})
ADD_SUBDIRECTORY(${OSDEP})
ADD_SUBDIRECTORY(keymap)
ADD_SUBDIRECTORY(dms)
ADD_SUBDIRECTORY(caps)

SET(cpuemu_SRCS
        cpuemu_0.c
        cpuemu_5.c
        cpuemu_6.c
        fpp.c

	cputbl.h
)

SET(readdisk_SRCS
   readdisk.c
   missing.c
)

ADD_CUSTOM_COMMAND(
       OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/blitfunc.c ${CMAKE_CURRENT_SOURCE_DIR}/blitfunc.h ${CMAKE_CURRENT_SOURCE_DIR}/blit.h ${CMAKE_CURRENT_SOURCE_DIR}/blittable.c
       COMMAND genblitter f >${CMAKE_CURRENT_SOURCE_DIR}/blitfunc.c
       COMMAND genblitter h >${CMAKE_CURRENT_SOURCE_DIR}/blitfunc.h
       COMMAND genblitter t >${CMAKE_CURRENT_SOURCE_DIR}/blittable.c
       COMMAND genblitter i >${CMAKE_CURRENT_SOURCE_DIR}/blit.h
       DEPENDS genblitter
)

ADD_CUSTOM_COMMAND(
      OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/cpudefs.c
      COMMAND build68k < ${CMAKE_CURRENT_SOURCE_DIR}/table68k > ${CMAKE_CURRENT_SOURCE_DIR}/cpudefs.c
      DEPENDS build68k
)

ADD_CUSTOM_COMMAND(
      OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/compstbl.c ${CMAKE_CURRENT_SOURCE_DIR}/comptbl.h
      OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/compemu.c
      COMMAND gencomp
      DEPENDS gencomp
)

ADD_CUSTOM_COMMAND(
      OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/cpustbl.c ${CMAKE_CURRENT_SOURCE_DIR}/cputbl.h
      OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/cpuemu_0.c
      OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/cpuemu_5.c
      OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/cpuemu_6.c 
      COMMAND gencpu --optimized-flags
      DEPENDS gencpu
)

ADD_CUSTOM_COMMAND(
      TARGET uae
      PRE_LINK
      #OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/linetoscr.c
      COMMAND genlinetoscr  >${CMAKE_CURRENT_SOURCE_DIR}/linetoscr.c
      #DEPENDS genlinetoscr
)

########### next target ###############

SET(uae_SRCS
        main.c
        newcpu.c
        memory.c
        events.c
        custom.c
        serial.c
        cia.c
        blitter.c
        autoconf.c
        traps.c
        ersatz.c
        keybuf.c
        expansion.c
        zfile.c
        cfgfile.c
        picasso96.c
        inputdevice.c
        gfxutil.c
        audio.c
        sinctable.c
        drawing.c
        native2amiga.c
        disk.c
        crc32.c
        savestate.c
        unzip.c
        uaeexe.c
        uaelib.c
        fdi2raw.c
        hotkeys.c
        ar.c
        driveclick.c
        enforcer.c
        misc.c
        missing.c
        readcpu.c
        blitfunc.c
        blittable.c
        cpustbl.c
        cpudefs.c
        writelog.c
        fsdb.c
        fsusage.c
        filesys_unix.c
        filesys.c
        fsdb_unix.c
        hardfile_unix.c
        bsdsocket-posix-new.c
        bsdsocket.c
        scsi-none.c
        debug.c
        identify.c
	compemu.c
        compemu_support.c
#        compemu_raw_x86.c # included in compemu_support.c
#	compemu_optimizer_x86.c included in compemu_support.c
	compemu_fpp.c
        compstbl.c
        hardfile.c
#	akiko.c
#	blkdev.c
#	blkdev-libscg.c
#	cd32_fmv.c
#	cdtv.c
	dongle.c
	driveclick.c
)

ADD_LIBRARY(cpuemu STATIC ${cpuemu_SRCS})

ADD_EXECUTABLE(readdisk ${readdisk_SRCS})

ADD_EXECUTABLE(uae ${uae_SRCS})
TARGET_LINK_LIBRARIES(
      uae
      machdep
      joydep
      snddep
      gfxdep
      guidep
      threaddep
      osdep
      keymap
      dms
      caps
      cpuemu
)

install(TARGETS uae DESTINATION bin)
install(TARGETS readdisk DESTINATION bin)

# Generates code, not to be installed
#install(TARGETS genblitter DESTINATION bin)
#install(TARGETS genlinetoscr DESTINATION bin)
#install(TARGETS build68k DESTINATION bin)
#install(TARGETS gencomp DESTINATION bin)
#install(TARGETS gencpu DESTINATION bin)


########### install files ###############
install(FILES table68k inputevents.def filesys.asm filesys.sh install_libscg DESTINATION misc)

#original Makefile.am contents follow:

#INCLUDES = \
#	-I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src -I$(top_srcdir)
#
#SUBDIRS = \
#	tools @MACHDEP@ @THREADDEP@ @GFX_DEP@ @SND_DEP@ @JOY_DEP@ @GUI_DEP@ \
#	@OSDEP@ keymap dms caps
#
#DIST_SUBDIRS = \
#	md-generic md-68k md-i386-gcc md-ppc md-ppc-gcc md-amd64-gcc \
#	gfx-amigaos gfx-beos gfx-x11 gfx-sdl gfx-curses gfx-svga \
#	sd-none sd-alsa sd-amigaos sd-beos sd-sdl sd-solaris sd-uss \
#	jd-none jd-amigainput jd-amigaos jd-beos jd-linuxold jd-sdl \
#	gui-none gui-beos gui-cocoa gui-gtk gui-muirexx \
#	od-generic od-amiga od-beos od-linux od-macosx od-win32 \
#	td-none td-amigaos td-beos td-posix td-sdl td-win32 \
#	keymap \
#	caps dms
#
#BUILT_SOURCES = \
#	blit.h blitfunc.h blitfunc.c blittable.c \
#	linetoscr.c \
#	cpudefs.c \
#	cpuemu_0.c cpuemu_5.c cpuemu_6.c \
#	cpustbl.c cputbl.h \
#	compemu.c \
#	compstbl.c comptbl.h
#
#CLEANFILES = \
#	blit.h blitfunc.h blitfunc.c blittable.c \
#	cpudefs.c \
#	cpuemu_0.c cpuemu_5.c cpuemu_6.c \
#	cpustbl.c cputbl.h \
#	compemu.c \
#	compstbl.c comptbl.h \
#	linetoscr.c
#
#DISTCLEANFILES = \
#	machdep osdep threaddep gfxdep sounddep joydep guidep \
#	target.h md-fpp.h
#
#bin_PROGRAMS  = \
#	uae readdisk
#
#if BUILD_MACOSX_BUNDLE
#bundle = @PACKAGE_NAME@.app
#bundle_contents = $(bundle)/Contents
#endif
#
#if TARGET_BIGENDIAN
#genlinetoscr_args = -b
#endif
#
#
#uae$(EXEEXT):	$(uae_OBJECTS) $(uae_DEPENDENCIES)
#		@rm -f uae$(EXEEXT)
#		$(LINK) $(uae_LDFLAGS) $(uae_OBJECTS) $(uae_LDADD) $(LIBS)
#if TARGET_BEOS
#		xres -o $@ @UAE_RSRCFILE@
#		mimeset -f $@
#endif
#if BUILD_MACOSX_BUNDLE
#		rm -fr $(bundle)
#		mkdir -p $(bundle_contents)/MacOS
#		mkdir -p $(bundle_contents)/Resources
#		$(INSTALL_DATA) od-macosx/Info.plist $(bundle_contents)
#		$(INSTALL_PROGRAM) uae$(EXEEXT) $(bundle_contents)/MacOS/
#		$(INSTALL_DATA) $(srcdir)/od-macosx/euae.icns $(bundle_contents)/Resources/
#endif
#
#noinst_LIBRARIES = \
#	libcpuemu.a
#
#dist_noinst_DATA = \
#	table68k inputevents.def filesys.asm
#
#noinst_HEADERS = 	 		 	include/caps.h		 	include/driveclick.h	 \
#		include/execlib.h include/akiko.h		include/ar.h		 \
#	include/audio.h		include/autoconf.h	 include/blitter.h	include/blkdev.h	 include/bsdsocket.h \
#	include/catweasel.h include/cdrom.h include/cia.h		 include/commpipe.h	include/compemu.h	 \
#	include/cpu_prefetch.h include/crc32.h					 include/custom.h	 include/custom_private.h \
#	include/debug.h		include/disk.h include/drawing.h include/enforcer.h	include/ersatz.h	 include/events.h \
#	include/fdi2raw.h	include/filesys.h	 include/fpp-ieee.h	include/fpp-unknown.h	 \
#	include/fsdb.h		include/fsusage.h	 include/genblitter.h	include/gensound.h	 \
#	include/gfxfilter.h	include/gui.h		 include/hotkeys.h				 include/hrtimer.h	include/identify.h	 \
#	include/inputdevice.h	include/joystick.h	 include/keyboard.h	include/keybuf.h	 include/memory.h	 \
#	include/native2amiga.h	include/newcpu.h	 include/noflags.h	include/options.h	 \
#	include/osemu.h		include/picasso96.h	 include/readcpu.h	include/savestate.h	 \
#	include/scsidev.h	include/serial.h	 include/sinctable.h				 include/sleep.h		include/sysdeps.h	 \
#	include/traps.h include/tui.h		include/uae.h		 include/uae_endian.h \
#	include/uae_malloc.h include/uae_string.h include/uae_types.h include/uaeexe.h \
#	include/unzip.h		 include/version.h include/writelog.h include/xwin.h		include/zfile.h		 \
#	targets/t-amiga.h	targets/t-beos.h targets/t-unix.h	targets/t-win32.h
#
#dist_noinst_SCRIPTS = \
#	filesys.sh install_libscg
#
#EXTRA_DIST = \
#	tools/configure.in tools/configure tools/sysconfig.h.in \
#	tools/target.h tools/Makefile.in \
#	test/test_optflag.c test/Makefile.in test/Makefile.am
#
#uae_SOURCES = ar.c audio.c autoconf.c blitter.c cfgfile.c cia.c crc32.c \
#	custom.c disk.c drawing.c driveclick.c enforcer.c ersatz.c events.c expansion.c \
#	fdi2raw.c fpp.c gfxutil.c hotkeys.c inputdevice.c keybuf.c main.c \
#	memory.c misc.c missing.c native2amiga.c newcpu.c picasso96.c readcpu.c \
#	savestate.c serial.c sinctable.c traps.c uaeexe.c uaelib.c unzip.c zfile.c
#
#EXTRA_uae_SOURCES = \
#	bsdsocket.c bsdsocket-posix-new.c build68k.c catweasel.c cdrom.c \
#	compemu_fpp.c compemu_raw_x86.c compemu_support.c \
#	debug.c identify.c filesys.c filesys_bootrom.c fsdb.c fsdb_unix.c fsusage.c genblitter.c \
#	gencpu.c gengenblitter.c gencomp.c genlinetoscr.c hardfile.c \
#	hardfile_unix.c scsi-none.c \
#	scsiemul.c svgancui.c tui.c \
#	blkdev-libscg.c filesys_unix.c \
#	akiko.c ar.c blkdev.c blitops.c \
#	writelog.c
#
#uae_DEPENDENCIES = \
#	@MACHDEP@/libmachdep.a		\
#	@JOY_DEP@/libjoydep.a		\
#	@SND_DEP@/libsnddep.a		\
#	@GFX_DEP@/libgfxdep.a		\
#	@GUI_DEP@/libguidep.a		\
#	keymap/libkeymap.a		\
#	dms/libdms.a			\
#	caps/libcaps.a			\
#	blitfunc.o blittable.o		\
#	cpustbl.o cpudefs.o		\
#	libcpuemu.a			\
#	@EXTRAOBJS@			\
#	@FILESYSOBJS@ 			\
#	@BSDSOCKOBJS@			\
#	@SCSIOBJS@			\
#	@CDOBJS@			\
#	@DEBUGOBJS@			\
#	@ASMOBJS@			\
#	@THREADDEP@/libthreaddep.a	\
#	@OSDEP@/libosdep.a
#
#uae_LDADD = \
#	@MACHDEP@/libmachdep.a		\
#	@JOY_DEP@/libjoydep.a		\
#	@SND_DEP@/libsnddep.a		\
#	@GFX_DEP@/libgfxdep.a		\
#	@GUI_DEP@/libguidep.a		\
#	keymap/libkeymap.a		\
#	dms/libdms.a			\
#	caps/libcaps.a			\
#	blitfunc.o blittable.o		\
#	cpustbl.o cpudefs.o		\
#	libcpuemu.a			\
#	@EXTRAOBJS@			\
#	@FILESYSOBJS@			\
#	@BSDSOCKOBJS@			\
#	@SCSIOBJS@			\
#	@CDOBJS@			\
#	@DEBUGOBJS@			\
#	@ASMOBJS@			\
#	@THREADDEP@/libthreaddep.a	\
#	@OSDEP@/libosdep.a
#
#
#readdisk_SOURCES = \
#	readdisk.c missing.c
#
#libcpuemu_a_SOURCES =
#libcpuemu_a_LIBADD =		@CPUOBJS@ @JITOBJS@
#libcpuemu_a_DEPENDENCIES =	@CPUOBJS@ @JITOBJS@
#
#cpuemu_0.c:	tools/gencpu
#		./tools/gencpu @GENCPUOPTS@
#
#cpustbl.c:	cpuemu_0.c
#
#cputbl.h:	cpuemu_0.c
#
#cpuemu_5.c:	cpuemu_0.c
#
#cpuemu_6.c:	cpuemu_0.c
#
#compemu.c:	tools/gencomp
#		./tools/gencomp
#
#compstbl.c:	compemu.c
#
#comptbl.h:	compemu.c
#
#cpuemu_nf_%.o: cpuemu_%.c
#	$(CC) $(INCLUDES) -c $(CPPFLAGS) $(CFLAGS) $(DEBUGFLAGS) $(NO_SCHED_CFLAGS) -DNOFLAGS $< -o $@
#
#cpuemu_%.o: cpuemu_%.c
#	$(CC) $(INCLUDES) -c $(CPPFLAGS) $(CFLAGS) $(DEBUGFLAGS) $(NO_SCHED_CFLAGS) $< -o $@
#
#cpustbl_nf.o: cpustbl.c
#	$(CC) $(INCLUDES) -c $(CPPFLAGS) $(CFLAGS) $(DEBUGFLAGS) $(NO_SCHED_CFLAGS) -DNOFLAGS cpustbl.c -o $@
#
#
#blit.h:		tools/genblitter
#		./tools/genblitter i >blit.h
#
#blitfunc.c: 	tools/genblitter blitfunc.h
#		./tools/genblitter f >blitfunc.c
#
#blitfunc.h: 	tools/genblitter
#		./tools/genblitter h >blitfunc.h
#
#blittable.c: 	tools/genblitter blitfunc.h
#		./tools/genblitter t >blittable.c
#
#linetoscr.c:    tools/genlinetoscr
#		./tools/genlinetoscr $(genlinetoscr_args) >linetoscr.c
#
#tools/genblitter:
#	$(MAKE) -C tools genblitter
#tools/build68k:
#	$(MAKE) -C tools build68k
#tools/gencpu:
#	$(MAKE) -C tools gencpu
#tools/gencomp:
#	$(MAKE) -C tools gencomp
#tools/genlinetoscr:
#	$(MAKE) -C tools genlinetoscr
#
#cpudefs.c:	tools/build68k @top_srcdir@/src/table68k
#		./tools/build68k <@top_srcdir@/src/table68k >cpudefs.c
#
#machdep/libmachdep.a:
#	$(MAKE) -C @MACHDEP@ libmachdep.a
#
#gfxdep/libgfxdep.a:
#	$(MAKE) -C @GFX_DEP@ libgfxdep.a
#
#sounddep/libsnddep.a:
#	$(MAKE) -C @SND_DEP@ libsnddep.a
#
#joydep/libjoydep.a:
#	$(MAKE)	-C @JOY_DEP@ libjoydep.a
#
#guidep/libguidep.a:
#	$(MAKE) -C @GUI_DEP@ libguidep.a
#
#osdep/libosdep.a:
#	$(MAKE) -C @OSDEP@ libosdep.a
#
#osdep/libthreaddep.a:
#	$(MAKE) -C @THREADDEP@ libthreaddep.a
#
#keymap/libkeymap.a:
#	$(MAKE) -C keymap libkeymap.a
#
#clean-local:
#	$(MAKE) -C tools clean
#
#distclean-local:
#	$(MAKE) -C tools distclean
