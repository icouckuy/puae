#ifndef SYSCONFIG_H
#define SYSCONFIG_H
/*TODO*/

/* Define if building universal (internal helper macro) */
#cmakedefine AC_APPLE_UNIVERSAL_BUILD

/* Define to 1 if you have the `alarm' function. */
#cmakedefine HAVE_ALARM 1

/* Define to 1 if you have the 'bswap_16' function. */
#cmakedefine HAVE_BSWAP_16 1

/* Define to 1 if you have the 'bswap_32' function. */
#cmakedefine HAVE_BSWAP_32 1

/* DONE */

/* Define to 1 if you have the <byteswap.h> header file. */
#cmakedefine HAVE_BYTESWAP_H 1

/* Define to 1 if you have the <caps/capsimage.h> header file. */
#cmakedefine HAVE_CAPS_CAPSIMAGE_H 1

/* Define to 1 if you have the <curses.h> header file. */
#cmakedefine HAVE_CURSES_H 1

/* Define to 1 if you have the <cybergraphx/cybergraphics.h> header file. */
#cmakedefine HAVE_CYBERGRAPHX_CYBERGRAPHICS_H 1

/* Define to 1 if you have the <devices/ahi.h> header file. */
#cmakedefine HAVE_DEVICES_AHI_H 1

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#cmakedefine HAVE_DIRENT_H 1

/* Define to 1 if you have 'dlopen' function */
#cmakedefine HAVE_DLOPEN 1

/* Define to 1 if you have the <dustat.h> header file. */
#cmakedefine HAVE_DUSTAT_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#cmakedefine HAVE_FCNTL_H 1

/* Define to 1 if you have the CAPS framework. */
#cmakedefine HAVE_FRAMEWORK_CAPSIMAGE 1

/* Define to 1 if you have the `gettimeofday' function. */
#cmakedefine HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the `gmtime_r' function. */
#cmakedefine HAVE_GMTIME_R 1

/* Define to 1 if you have the `isinf' function. */
#cmakedefine HAVE_ISINF 1

/* Define to 1 if you have the `isnan' function. */
#cmakedefine HAVE_ISNAN 1

/* Define to 1 if you have the <libraries/cybergraphics.h> header file. */
#cmakedefine HAVE_LIBRARIES_CYBERGRAPHICS_H 1

/* Define to 1 if you have the `localtime_r' function. */
#cmakedefine HAVE_LOCALTIME_R 1

/* Define to 1 if you have the <machine/joystick.h> header file. */
#cmakedefine HAVE_MACHINE_JOYSTICK_H 1

/* Define to 1 if you have the <machine/soundcard.h> header file. */
#cmakedefine HAVE_MACHINE_SOUNDCARD_H 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* CPU supports MMX. */
#cmakedefine HAVE_MMX 1

/* Define to 1 if you have the `nanosleep' function. */
#ifndef HAVE_NANOSLEEP
#cmakedefine HAVE_NANOSLEEP 1
#endif

/* Define to 1 if you have the <ncurses.h> header file. */
#cmakedefine HAVE_NCURSES_H 1

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
#cmakedefine HAVE_NDIR_H 1

/* TODO*/

/* Define if you have POSIX threads libraries and header files. */
#cmakedefine HAVE_PTHREAD

/* DONE */

/* Define to 1 if you have the `readdir_r' function. */
#cmakedefine HAVE_READDIR_R 1

/* Define to 1 if you have the `select' function. */
#cmakedefine HAVE_SELECT 1

/* Define to 1 if you have the `setitimer' function. */
#cmakedefine HAVE_SETITIMER 1

/* Define to 1 if you have the `sigaction' function. */
#cmakedefine HAVE_SIGACTION 1

/* Define to 1 if you have the `sleep' function. */
#cmakedefine HAVE_SLEEP 1

/* CPU supports SSE . */
#cmakedefine HAVE_SSE 1

/* CPU supports SSE2 . */
#cmakedefine HAVE_SSE2 1

/* CPU supports SSE4_1 . */
#cmakedefine HAVE_SSE4_1 1

/* CPU supports SSE3 . */
#cmakedefine HAVE_SSE3 1

/* TODO */

/* Define to 1 if stdbool.h conforms to C99. */
#cmakedefine HAVE_STDBOOL_H 1

/* DONE */

/* Define to 1 if you have the <stdint.h> header file. */
#ifndef HAVE_STDINT_H
#cmakedefine HAVE_STDINT_H 1
#endif

/*TODO */

/* Define if <stdint.h> exists, doesn't clash with <sys/types.h>, and declares
   uintmax_t. */
#cmakedefine HAVE_STDINT_H_WITH_UINTMAX

/* DONE */

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#cmakedefine HAVE_STRCASECMP 1

/* Define to 1 if you have the `strcmpi' function. */
#cmakedefine HAVE_STRCMPI 1

/* Define to 1 if you have the `strdup' function. */
#cmakedefine HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#cmakedefine HAVE_STRERROR 1

/* Define to 1 if you have the `stricmp' function. */
#cmakedefine HAVE_STRICMP 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the `strstr' function. */
#cmakedefine HAVE_STRSTR 1

/* TODO */
/* Define to 1 if `st_blocks' is member of `struct stat'. */
#cmakedefine HAVE_STRUCT_STAT_ST_BLOCKS 1

/* Define to 1 if your `struct stat' has `st_blocks'. Deprecated, use
   `HAVE_STRUCT_STAT_ST_BLOCKS' instead. */
#cmakedefine HAVE_ST_BLOCKS 1

/* TODO: tired */

/* Define to 1 if you have the <sun/audioio.h> header file. */
#cmakedefine HAVE_SUN_AUDIOIO_H 1

/* Define to 1 if you have the `sync' function. */
#cmakedefine HAVE_SYNC 1

/* Define to 1 if you have the <sys/audioio.h> header file. */
#cmakedefine HAVE_SYS_AUDIOIO_H 1

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
#cmakedefine HAVE_SYS_DIR_H 1

/* Define to 1 if you have the <sys/filio.h> header file. */
#cmakedefine HAVE_SYS_FILIO_H 1

/* Define to 1 if you have the <sys/filsys.h> header file. */
#cmakedefine HAVE_SYS_FILSYS_H 1

/* Define to 1 if you have the <sys/fs/s5param.h> header file. */
#cmakedefine HAVE_SYS_FS_S5PARAM_H 1

/* Define to 1 if you have the <sys/fs_types.h> header file. */
#cmakedefine HAVE_SYS_FS_TYPES_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#cmakedefine HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/ipc.h> header file. */
#cmakedefine HAVE_SYS_IPC_H 1

/* Define to 1 if you have the <sys/mman.h> header file. */
#cmakedefine HAVE_SYS_MMAN_H 1

/* Define to 1 if you have the <sys/mount.h> header file. */
#cmakedefine HAVE_SYS_MOUNT_H 1

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
#cmakedefine HAVE_SYS_NDIR_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#cmakedefine HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/shm.h> header file. */
#cmakedefine HAVE_SYS_SHM_H 1

/* Define to 1 if you have the <sys/soundcard.h> header file. */
#cmakedefine HAVE_SYS_SOUNDCARD_H 1

/* Define to 1 if you have the <sys/statfs.h> header file. */
#cmakedefine HAVE_SYS_STATFS_H 1

/* Define to 1 if you have the <sys/statvfs.h> header file. */
#cmakedefine HAVE_SYS_STATVFS_H 1

/* Define to 1 if you have the <sys/stat.h> header file. DONE */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/termios.h> header file. */
#cmakedefine HAVE_SYS_TERMIOS_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#cmakedefine HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/vfs.h> header file. */
#cmakedefine HAVE_SYS_VFS_H 1

/* Define to 1 if you have the `tcgetattr' function. */
#cmakedefine HAVE_TCGETATTR 1

/* Define to 1 if you have the `timegm' function. */
#cmakedefine HAVE_TIMEGM 1

/* Define if you have the 'uintmax_t' type in <stdint.h> or <inttypes.h>. */
#cmakedefine HAVE_UINTMAX_T

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define if you have the 'unsigned long long' type. */
#cmakedefine HAVE_UNSIGNED_LONG_LONG

/* Define to 1 if you have the `usleep' function. */
#cmakedefine HAVE_USLEEP 1

/* Define to 1 if you have the <utime.h> header file. DONE */ 
#cmakedefine HAVE_UTIME_H 1

/* Define to 1 if `utime(file, NULL)' sets file's timestamp to the present. */
#cmakedefine HAVE_UTIME_NULL 1

/* Define to 1 if you have the <values.h> header file. */
#cmakedefine HAVE_VALUES_H 1

/* Define to 1 if you have the `vfprintf' function. */
#cmakedefine HAVE_VFPRINTF 1

/* Define to 1 if you have the `vprintf' function. */
#cmakedefine HAVE_VPRINTF 1

/* Define to 1 if you have the `vsprintf' function. */
#cmakedefine HAVE_VSPRINTF 1

/* Define to 1 if the system has the type `_Bool'. */
#cmakedefine HAVE__BOOL 1

/* Define to 1 if your CPU profitably supports multiplication. */
#cmakedefine MULTIPLICATION_PROFITABLE 1

/* Name of package */
#cmakedefine PACKAGE ${PACKAGE}

/* Define to the address where bug reports for this package should be sent. */
#cmakedefine PACKAGE_BUGREPORT ${PACKAGE_BUGREPORT}

/* Define to the full name of this package. */
#cmakedefine PACKAGE_NAME ${PACKAGE_NAME}

/* Define to the full name and version of this package. */
#cmakedefine PACKAGE_STRING ${PACKAGE_STRING}

/* Define to the one symbol short name of this package. */
#cmakedefine PACKAGE_TARNAME ${PACKAGE_TARNAME}

/* Define to the version of this package. */
#cmakedefine PACKAGE_VERSION ${PACKAGE_VERSION}

/* Define to the necessary symbol if this constant uses a non-standard name on
   your system. */
#cmakedefine PTHREAD_CREATE_JOINABLE

/* Define as the return type of signal handlers (`int' or `void'). TODO */
#define RETSIGTYPE void

/* The size of `char', as computed by sizeof. */
#cmakedefine SIZEOF_CHAR ${SIZEOF_CHAR}

/* The size of `int', as computed by sizeof. */
#cmakedefine SIZEOF_INT ${SIZEOF_INT}

/* The size of `long', as computed by sizeof. */
#cmakedefine SIZEOF_LONG ${SIZEOF_LONG}

/* The size of `long long', as computed by sizeof. */
#cmakedefine SIZEOF_LONG_LONG ${SIZEOF_LONG_LONG}

/* The size of `short', as computed by sizeof. */
#cmakedefine SIZEOF_SHORT ${SIZEOF_SHORT}

/* The size of `void *', as computed by sizeof. */
#cmakedefine SIZEOF_VOID_P ${SIZEOF_VOID_P}

/* The size of `__int64', as computed by sizeof. */
#cmakedefine SIZEOF___INT64 ${SIZEOF___INT64}

/* Define if the block counts reported by statfs may be truncated to 2GB and
   the correct values may be stored in the f_spare array. (SunOS 4.1.2, 4.1.3,
   and 4.1.3_U1 are reported to have this problem. SunOS 4.1.1 seems not to be
   affected.) */
#cmakedefine STATFS_TRUNCATES_BLOCK_COUNTS

/* Define if there is no specific function for reading filesystems usage
   information and you have the <sys/filsys.h> header file. (SVR2) */
#cmakedefine STAT_READ_FILSYS

/* Define if statfs takes 2 args and struct statfs has a field named f_bsize.
   (4.3BSD, SunOS 4, HP-UX, AIX PS/2) */
#cmakedefine STAT_STATFS2_BSIZE

/* Define if statfs takes 2 args and struct statfs has a field named f_fsize.
   (4.4BSD, NetBSD) */
#cmakedefine STAT_STATFS2_FSIZE

/* Define if statfs takes 2 args and the second argument has type struct
   fs_data. (Ultrix) */
#cmakedefine STAT_STATFS2_FS_DATA

/* Define if statfs takes 3 args. (DEC Alpha running OSF/1) */
#cmakedefine STAT_STATFS3_OSF1

/* Define if statfs takes 4 args. (SVR3, Dynix, Irix, Dolphin) */
#cmakedefine STAT_STATFS4

/* Define if there is a function named statvfs. (SVR4) */
#cmakedefine STAT_STATVFS

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#cmakedefine TIME_WITH_SYS_TIME 1

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
#cmakedefine TM_IN_SYS_TIME 1

/* Enable extensions on AIX 3, Interix.  */
#cmakedefine _ALL_SOURCE

/* Enable GNU extensions on systems that have them.  */
#cmakedefine _GNU_SOURCE

/* Enable threading extensions on Solaris.  */
#cmakedefine _POSIX_PTHREAD_SEMANTICS

/* Enable extensions on HP NonStop.  */
#cmakedefine _TANDEM_SOURCE

/* Enable general extensions on Solaris.  */
#cmakedefine __EXTENSIONS__

/* Version number of package */
#cmakedefine VERSION

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
#define WORDS_BIGENDIAN
# endif
#endif

/* Define to 1 if the X Window System is missing or not being used. */
#cmakedefine X_DISPLAY_MISSING 1

/* Number of bits in a file offset, on hosts where this is settable. */
#cmakedefine _FILE_OFFSET_BITS

/* Define for large files, on AIX-style hosts. */
#cmakedefine _LARGE_FILES

/* Define to 1 if on MINIX. */
#cmakedefine _MINIX 1

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
#cmakedefine _POSIX_1_SOURCE 2

/* Define to 1 if you need to in order for `stat' and other things to work. */
#cmakedefine _POSIX_SOURCE 1

/* Define to empty if `const' does not conform to ANSI C. */
#cmakedefine const

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#cmakedefine inline
#endif

/* Define to `int' if <sys/types.h> does not define. */
#cmakedefine mode_t

/* Define to `long int' if <sys/types.h> does not define. */
#cmakedefine off_t

/* Define to `unsigned int' if <sys/types.h> does not define. */
#cmakedefine size_t

/* Substitute for socklen_t */
#cmakedefine socklen_t

/* Define to unsigned long or unsigned long long if <stdint.h> and
   <inttypes.h> don't define. */
#cmakedefine uintmax_t

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
#cmakedefine volatile

#endif
