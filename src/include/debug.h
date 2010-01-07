 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Debugger
  *
  * (c) 1995 Bernd Schmidt
  *
  */

#ifdef DEBUGGER

#define	MAX_HIST 500
#define MAX_LINEWIDTH 100

extern int debugging;
extern int exception_debugging;
extern int debug_copper;
extern int debug_dma;
extern int debug_sprite_mask;
extern int debug_bpl_mask, debug_bpl_mask_one;
extern int debugger_active;

extern void debug (void);
extern void debugger_change (int mode);
extern void activate_debugger (void);
extern void deactivate_debugger (void);

extern void record_copper (uaecptr addr, int hpos, int vpos);
extern void record_copper_reset (void);

extern void dump_aga_custom (void);
extern void memory_map_dump (void);

struct dma_rec
{
    uae_u16 reg;
    uae_u16 dat;
    uae_u32 addr;
    uae_u16 evt;
    int type;
};

#define DMA_EVENT_BLITIRQ 1
#define DMA_EVENT_BLITNASTY 2
#define DMA_EVENT_BLITFINISHED 4
#define DMA_EVENT_BPLFETCHUPDATE 8
#define DMA_EVENT_COPPERWAKE 16

#define DMARECORD_REFRESH 1
#define DMARECORD_CPU 2
#define DMARECORD_COPPER 3
#define DMARECORD_AUDIO 4
#define DMARECORD_BLITTER 5
#define DMARECORD_BLITTER_LINE 6
#define DMARECORD_BITPLANE 7
#define DMARECORD_SPRITE 8
#define DMARECORD_DISK 9
#define DMARECORD_MAX 10

extern struct dma_rec *record_dma (uae_u16 reg, uae_u16 dat, uae_u32 addr, int hpos, int vpos, int type);
extern void record_dma_reset (void);
extern void record_dma_event (int evt, int hpos, int vpos);
extern void debug_draw_cycles (uae_u8 *buf, int bpp, int line, int width, int height, uae_u32 *xredcolors, uae_u32 *xgreencolors, uae_u32 *xbluescolors);

#endif
