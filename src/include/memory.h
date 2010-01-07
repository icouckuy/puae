 /*
  * UAE - The Un*x Amiga Emulator
  *
  * memory management
  *
  * Copyright 1995 Bernd Schmidt
  */

extern void memory_reset (void);
extern void a1000_reset (void);

#ifdef JIT
extern int special_mem;
#define SPECIAL_MEM_READ 1
#define SPECIAL_MEM_WRITE 2
#define S_READ 1
#define S_WRITE 2

extern void *cache_alloc (int);
extern void cache_free (void*);

extern int canbang, candirect;
void init_shm (void);
void preinit_shm (void);
#endif

#ifdef ADDRESS_SPACE_24BIT
# define MEMORY_BANKS		256
# define MEMORY_RANGE_MASK	((1<<24)-1)
#else
# define MEMORY_BANKS		65536
# define MEMORY_RANGE_MASK	(~0)
#endif

typedef uae_u32 (REGPARAM3 *mem_get_func)(uaecptr) REGPARAM;
typedef void (REGPARAM3 *mem_put_func)(uaecptr, uae_u32) REGPARAM;
typedef uae_u8 *(REGPARAM3 *xlate_func)(uaecptr) REGPARAM;
typedef int (REGPARAM3 *check_func)(uaecptr, uae_u32) REGPARAM;

extern uae_u8 *address_space, *good_address_map;
extern uae_u8 *chipmemory;

extern uae_u32 allocated_chipmem;
extern uae_u32 allocated_fastmem;
extern uae_u32 allocated_bogomem;
extern uae_u32 allocated_gfxmem;
extern uae_u32 allocated_z3fastmem, allocated_z3fastmem2, max_z3fastmem;
extern uae_u32 allocated_a3000mem;
extern uae_u32 allocated_cardmem;

extern uae_u32 wait_cpu_cycle_read (uaecptr addr, int mode);
extern uae_u32 wait_cpu_cycle_read_cycles (uaecptr addr, int mode, int *cycles);
extern void wait_cpu_cycle_write (uaecptr addr, int mode, uae_u32 v);
extern uae_u32 wait_cpu_cycle_read_ce020 (uaecptr addr, int mode);
extern void wait_cpu_cycle_write_ce020 (uaecptr addr, int mode, uae_u32 v);

#undef DIRECT_MEMFUNCS_SUCCESSFUL
#include "machdep/maccess.h"
#include "osdep/memory.h"

#define chipmem_start 0x00000000
#define bogomem_start 0x00C00000
#define cardmem_start 0x00E00000
#define kickmem_start 0x00F80000
extern uaecptr z3fastmem_start, z3fastmem2_start;
extern uaecptr p96ram_start;
extern uaecptr fastmem_start;
extern uaecptr a3000lmem_start, a3000hmem_start;

extern int ersatzkickfile;
extern int cloanto_rom, kickstart_rom;
extern uae_u16 kickstart_version;
extern int uae_boot_rom, uae_boot_rom_size;
extern uaecptr rtarea_base;

extern uae_u8* baseaddr[];

enum { ABFLAG_UNK = 0, ABFLAG_RAM = 1, ABFLAG_ROM = 2, ABFLAG_ROMIN = 4, ABFLAG_IO = 8, ABFLAG_NONE = 16, ABFLAG_SAFE = 32 };
typedef struct {
    /* These ones should be self-explanatory... */
    mem_get_func lget, wget, bget;
    mem_put_func lput, wput, bput;
    /* Use xlateaddr to translate an Amiga address to a uae_u8 * that can
     * be used to address memory without calling the wget/wput functions.
     * This doesn't work for all memory banks, so this function may call
     * abort(). */
    xlate_func xlateaddr;
    /* To prevent calls to abort(), use check before calling xlateaddr.
     * It checks not only that the memory bank can do xlateaddr, but also
     * that the pointer points to an area of at least the specified size.
     * This is used for example to translate bitplane pointers in custom.c */
    check_func check;
    /* For those banks that refer to real memory, we can save the whole trouble
       of going through function calls, and instead simply grab the memory
       ourselves. This holds the memory address where the start of memory is
       for this particular bank. */
    uae_u8 *baseaddr;
	TCHAR *name;
    /* for instruction opcode/operand fetches */
    mem_get_func lgeti, wgeti;
    int flags;
} addrbank;

#define CE_MEMBANK_FAST 0
#define CE_MEMBANK_CHIP 1
#define CE_MEMBANK_CIA 2
#define CE_MEMBANK_FAST16BIT 3
extern uae_u8 ce_banktype[65536];

extern uae_u8 *filesysory;
extern uae_u8 *rtarea;

extern addrbank chipmem_bank;
extern addrbank chipmem_agnus_bank;
extern addrbank chipmem_bank_ce2;
extern addrbank kickmem_bank;
extern addrbank custom_bank;
extern addrbank clock_bank;
extern addrbank cia_bank;
extern addrbank rtarea_bank;
extern addrbank expamem_bank;
extern addrbank fastmem_bank;
extern addrbank gfxmem_bank, gfxmem_bankx;
extern addrbank gayle_bank;
extern addrbank gayle2_bank;
extern addrbank mbres_bank;
extern addrbank akiko_bank;
extern addrbank cardmem_bank;

extern void rtarea_init (void);
extern void rtarea_init_mem (void);
extern void rtarea_setup (void);
extern void expamem_init (void);
extern void expamem_reset (void);
extern void expamem_next (void);

extern uae_u32 gfxmem_start;
extern uae_u8 *gfxmemory;
extern uae_u32 gfxmem_mask;
extern int address_space_24;
extern uae_u16 last_custom_value1;

/* Default memory access functions */

extern int REGPARAM3 default_check(uaecptr addr, uae_u32 size) REGPARAM;
extern uae_u8 *REGPARAM3 default_xlate(uaecptr addr) REGPARAM;
/* 680x0 opcode fetches */
extern uae_u32 REGPARAM3 dummy_lgeti (uaecptr addr) REGPARAM;
extern uae_u32 REGPARAM3 dummy_wgeti (uaecptr addr) REGPARAM;

#define bankindex(addr) (((uaecptr)(addr)) >> 16)

extern addrbank *mem_banks[MEMORY_BANKS];

#ifdef JIT
extern uae_u8 *baseaddr[MEMORY_BANKS];
#endif

#define get_mem_bank(addr) (*mem_banks[bankindex(addr)])

#ifdef JIT
#define put_mem_bank(addr, b, realstart) do { \
    (mem_banks[bankindex(addr)] = (b)); \
    if ((b)->baseaddr) \
	baseaddr[bankindex(addr)] = (b)->baseaddr - (realstart); \
    else \
	baseaddr[bankindex(addr)] = (uae_u8*)(((uae_u8*)b)+1); \
} while (0)
#else
#define put_mem_bank(addr, b, realstart) \
    (mem_banks[bankindex(addr)] = (b));
#endif

extern void memory_init (void);
extern void memory_cleanup (void);
extern void map_banks (addrbank *bank, int first, int count, int realsize);
extern void map_overlay (int chip);
extern void memory_hardreset (void);
extern void free_fastmemory (void);

#define longget(addr) (call_mem_get_func(get_mem_bank(addr).lget, addr))
#define wordget(addr) (call_mem_get_func(get_mem_bank(addr).wget, addr))
#define byteget(addr) (call_mem_get_func(get_mem_bank(addr).bget, addr))
#define longgeti(addr) (call_mem_get_func(get_mem_bank(addr).lgeti, addr))
#define wordgeti(addr) (call_mem_get_func(get_mem_bank(addr).wgeti, addr))
#define longput(addr,l) (call_mem_put_func(get_mem_bank(addr).lput, addr, l))
#define wordput(addr,w) (call_mem_put_func(get_mem_bank(addr).wput, addr, w))
#define byteput(addr,b) (call_mem_put_func(get_mem_bank(addr).bput, addr, b))

STATIC_INLINE uae_u32 get_long (uaecptr addr)
{
    return longget(addr);
}
STATIC_INLINE uae_u32 get_word (uaecptr addr)
{
    return wordget(addr);
}
STATIC_INLINE uae_u32 get_byte (uaecptr addr)
{
    return byteget(addr);
}
STATIC_INLINE uae_u32 get_longi(uaecptr addr)
{
    return longgeti(addr);
}
STATIC_INLINE uae_u32 get_wordi(uaecptr addr)
{
    return wordgeti(addr);
}

/*
 * Read a host pointer from addr
 */
#if SIZEOF_VOID_P == 4
# define get_pointer(addr) ((void *)get_long (addr))
#else
# if SIZEOF_VOID_P == 8
STATIC_INLINE void *get_pointer (uaecptr addr)
{
    const unsigned int n = SIZEOF_VOID_P / 4;
    union {
	void    *ptr;
	uae_u32  longs[SIZEOF_VOID_P / 4];
    } p;
    unsigned int i;

    for (i = 0; i < n; i++) {
#ifdef WORDS_BIGENDIAN
	p.longs[i]     = get_long (addr + i * 4);
#else
	p.longs[n - 1 - i] = get_long (addr + i * 4);
#endif
    }
    return p.ptr;
}
# else
#  error "Unknown or unsupported pointer size."
# endif
#endif

STATIC_INLINE void put_long (uaecptr addr, uae_u32 l)
{
    longput(addr, l);
}
STATIC_INLINE void put_word (uaecptr addr, uae_u32 w)
{
    wordput(addr, w);
}
STATIC_INLINE void put_byte (uaecptr addr, uae_u32 b)
{
    byteput(addr, b);
}

extern void put_long_slow (uaecptr addr, uae_u32 v);
extern void put_word_slow (uaecptr addr, uae_u32 v);
extern void put_byte_slow (uaecptr addr, uae_u32 v);
extern uae_u32 get_long_slow (uaecptr addr);
extern uae_u32 get_word_slow (uaecptr addr);
extern uae_u32 get_byte_slow (uaecptr addr);


/*
 * Store host pointer v at addr
 */
#if SIZEOF_VOID_P == 4
# define put_pointer(addr, p) (put_long ((addr), (uae_u32)(p)))
#else
# if SIZEOF_VOID_P == 8
STATIC_INLINE void put_pointer (uaecptr addr, void *v)
{
    const unsigned int n = SIZEOF_VOID_P / 4;
    union {
	void    *ptr;
	uae_u32  longs[SIZEOF_VOID_P / 4];
    } p;
    unsigned int i;

    p.ptr = v;

    for (i = 0; i < n; i++) {
#ifdef WORDS_BIGENDIAN
	put_long (addr + i * 4, p.longs[i]);
#else
	put_long (addr + i * 4, p.longs[n - 1 - i]);
#endif
    }
}
# endif
#endif

STATIC_INLINE uae_u8 *get_real_address (uaecptr addr)
{
    return get_mem_bank(addr).xlateaddr(addr);
}

STATIC_INLINE int valid_address(uaecptr addr, uae_u32 size)
{
    return get_mem_bank(addr).check(addr, size);
}

extern int addr_valid (TCHAR*, uaecptr,uae_u32);

/* For faster access in custom chip emulation.  */
extern uae_u32 REGPARAM3 chipmem_lget (uaecptr) REGPARAM;
extern uae_u32 REGPARAM3 chipmem_wget (uaecptr) REGPARAM;
extern uae_u32 REGPARAM3 chipmem_bget (uaecptr) REGPARAM;
extern void REGPARAM3 chipmem_lput (uaecptr, uae_u32) REGPARAM;
extern void REGPARAM3 chipmem_wput (uaecptr, uae_u32) REGPARAM;
extern void REGPARAM3 chipmem_bput (uaecptr, uae_u32) REGPARAM;

extern uae_u32 REGPARAM3 chipmem_agnus_lget (uaecptr) REGPARAM;
extern uae_u32 REGPARAM3 chipmem_agnus_wget (uaecptr) REGPARAM;
extern uae_u32 REGPARAM3 chipmem_agnus_bget (uaecptr) REGPARAM;
extern void REGPARAM3 chipmem_agnus_lput (uaecptr, uae_u32) REGPARAM;
extern void REGPARAM3 chipmem_agnus_wput (uaecptr, uae_u32) REGPARAM;
extern void REGPARAM3 chipmem_agnus_bput (uaecptr, uae_u32) REGPARAM;

extern uae_u32 chipmem_mask, kickmem_mask;
extern uae_u8 *kickmemory;
extern int kickmem_size;
extern addrbank dummy_bank;

/* 68020+ Chip RAM DMA contention emulation */
extern uae_u32 REGPARAM3 chipmem_lget_ce2 (uaecptr) REGPARAM;
extern uae_u32 REGPARAM3 chipmem_wget_ce2 (uaecptr) REGPARAM;
extern uae_u32 REGPARAM3 chipmem_bget_ce2 (uaecptr) REGPARAM;
extern void REGPARAM3 chipmem_lput_ce2 (uaecptr, uae_u32) REGPARAM;
extern void REGPARAM3 chipmem_wput_ce2 (uaecptr, uae_u32) REGPARAM;
extern void REGPARAM3 chipmem_bput_ce2 (uaecptr, uae_u32) REGPARAM;

#ifdef NATMEM_OFFSET

typedef struct shmpiece_reg {
    uae_u8 *native_address;
    int id;
    uae_u32 size;
    struct shmpiece_reg *next;
    struct shmpiece_reg *prev;
} shmpiece;

extern shmpiece *shm_start;

#endif

extern uae_u8 *mapped_malloc (size_t, const TCHAR*);
extern void mapped_free (uae_u8 *);
extern void clearexec (void);
extern void mapkick (void);
extern int decode_cloanto_rom_do (uae_u8 *mem, int size, int real_size);
extern void a3000_fakekick (int);

#define ROMTYPE_KICK	    0x000001
#define ROMTYPE_KICKCD32    0x000002
#define ROMTYPE_EXTCD32	    0x000004
#define ROMTYPE_EXTCDTV	    0x000008
#define ROMTYPE_A2091BOOT   0x000010
#define ROMTYPE_A4091BOOT   0x000020
#define ROMTYPE_AR	    0x000040
#define ROMTYPE_SUPERIV	    0x000080
#define ROMTYPE_KEY	    0x000100
#define ROMTYPE_ARCADIABIOS 0x000200
#define ROMTYPE_ARCADIAGAME 0x000400
#define ROMTYPE_HRTMON	    0x000800
#define ROMTYPE_NORDIC	    0x001000
#define ROMTYPE_XPOWER	    0x002000
#define ROMTYPE_CD32CART    0x004000
#define ROMTYPE_SPECIALKICK 0x008000
#define ROMTYPE_MASK	    0x01ffff
#define ROMTYPE_EVEN	    0x020000
#define ROMTYPE_ODD	    0x040000
#define ROMTYPE_8BIT	    0x080000
#define ROMTYPE_BYTESWAP    0x100000
#define ROMTYPE_CD32	    0x200000
#define ROMTYPE_SCRAMBLED   0x400000

struct romheader {
	TCHAR *name;
    int id;
};

struct romdata {
	TCHAR *name;
    int ver, rev;
    int subver, subrev;
	TCHAR *model;
    uae_u32 size;
    int id;
    int cpu;
    int cloanto;
    int type;
    int group;
    int title;
	TCHAR *partnumber;
    uae_u32 crc32;
    uae_u32 sha1[5];
	TCHAR *configname;
};

struct romlist {
	TCHAR *path;
    struct romdata *rd;
};

extern struct romdata *getromdatabypath (TCHAR *path);
extern struct romdata *getromdatabycrc (uae_u32 crc32);
extern struct romdata *getromdatabydata (uae_u8 *rom, int size);
extern struct romdata *getromdatabyid (int id);
extern struct romdata *getromdatabyidgroup (int id, int group, int subitem);
extern struct romdata *getromdatabyzfile (FILE *f);
extern struct romlist **getarcadiaroms (void);
extern struct romdata *getarcadiarombyname (TCHAR *name);
extern struct romlist **getromlistbyident (int ver, int rev, int subver, int subrev, TCHAR *model, int all);
extern void getromname (struct romdata*, TCHAR*);
extern struct romdata *getromdatabyname (TCHAR*);
extern struct romlist *getromlistbyids (int *ids);
extern void romwarning(int *ids);
extern struct romlist *getromlistbyromdata (struct romdata *rd);
extern void romlist_add (TCHAR *path, struct romdata *rd);
extern TCHAR *romlist_get (struct romdata *rd);
extern void romlist_clear (void);
extern struct zfile *read_rom (struct romdata **rd);
extern struct zfile *read_rom_name (const TCHAR *filename);

extern int load_keyring (struct uae_prefs *p, TCHAR *path);
extern uae_u8 *target_load_keyfile (struct uae_prefs *p, TCHAR *path, int *size, TCHAR *name);
extern void free_keyring (void);
extern int get_keyring (void);

extern uaecptr strcpyha_safe (uaecptr dst, const uae_char *src);
extern uae_char *strcpyah_safe (uae_u8 *dst, uaecptr src, int maxsize);
extern void memcpyha_safe (uaecptr dst, const uae_u8 *src, int size);
//extern void memcpyha (uaecptr dst, const uae_u8 *src, int size);
extern void memcpyah_safe (uae_u8 *dst, uaecptr src, int size);
extern void memcpyah (uae_u8 *dst, uaecptr src, int size);

