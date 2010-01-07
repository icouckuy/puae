/*
 * A collection of ugly and random junk brought in from Win32
 * which desparately needs to be tidied up
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "events.h"
#include "uae.h"
#include "autoconf.h"
#include "traps.h"
#include "enforcer.h"
#include "picasso96.h"
#include "driveclick.h"

static int tablet;
static int axmax, aymax, azmax;
static int xmax, ymax, zmax;
static int xres, yres;
static int maxpres;
static TCHAR *tabletname;
static int tablet_x, tablet_y, tablet_z, tablet_pressure, tablet_buttons, tablet_proximity;
static int tablet_ax, tablet_ay, tablet_az, tablet_flags;

struct winuae_currentmode {
        unsigned int flags;
        int native_width, native_height, native_depth, pitch;
        int current_width, current_height, current_depth;
        int amiga_width, amiga_height;
        int frequency;
        int initdone;
        int fullfill;
        int vsync;
};

static struct winuae_currentmode currentmodestruct;
static struct winuae_currentmode *currentmode = &currentmodestruct;

static int serial_period_hsyncs, serial_period_hsync_counter;
static int data_in_serdatr; /* new data received */

static uae_u32 REGPARAM2 gfxmem_lgetx (uaecptr addr)
{
	uae_u32 *m;
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	m = (uae_u32 *)(gfxmemory + addr);
	return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 gfxmem_wgetx (uaecptr addr)
{
	uae_u16 *m;
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	m = (uae_u16 *)(gfxmemory + addr);
	return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 gfxmem_bgetx (uaecptr addr)
{
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	return gfxmemory[addr];
}

static void REGPARAM2 gfxmem_lputx (uaecptr addr, uae_u32 l)
{
	uae_u32 *m;
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	m = (uae_u32 *)(gfxmemory + addr);
	do_put_mem_long (m, l);
}

static void REGPARAM2 gfxmem_wputx (uaecptr addr, uae_u32 w)
{
	uae_u16 *m;
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	m = (uae_u16 *)(gfxmemory + addr);
	do_put_mem_word (m, (uae_u16)w);
}

static void REGPARAM2 gfxmem_bputx (uaecptr addr, uae_u32 b)
{
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	gfxmemory[addr] = b;
}

static int REGPARAM2 gfxmem_check (uaecptr addr, uae_u32 size)
{
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	return (addr + size) < allocated_gfxmem;
}

static uae_u8 *REGPARAM2 gfxmem_xlate (uaecptr addr)
{
	addr -= gfxmem_start & gfxmem_mask;
	addr &= gfxmem_mask;
	return gfxmemory + addr;
}


addrbank gfxmem_bankx = {
	gfxmem_lgetx, gfxmem_wgetx, gfxmem_bgetx,
	gfxmem_lputx, gfxmem_wputx, gfxmem_bputx,
	gfxmem_xlate, gfxmem_check, NULL, "RTG RAM",
	dummy_lgeti, dummy_wgeti, ABFLAG_RAM
};

void getgfxoffset (int *dxp, int *dyp, int *mxp, int *myp)
{
*dxp = 0;
*dyp = 0;
*mxp = 0;
*myp = 0;
/*
        int dx, dy;

        getfilteroffset (&dx, &dy, mxp, myp);
        *dxp = dx;
        *dyp = dy;
        if (picasso_on) {
                dx = picasso_offset_x;
                dy = picasso_offset_y;
                *mxp = picasso_offset_mx;
                *myp = picasso_offset_my;
        }
        *dxp = dx;
        *dyp = dy;
        if (currentmode->flags & DM_W_FULLSCREEN) {
                if (scalepicasso && screen_is_picasso)
                        return;
                if (usedfilter && !screen_is_picasso)
                        return;
                if (currentmode->fullfill && (currentmode->current_width > currentmode->native_width || currentmode->current_height > currentmode->native_height))
                        return;
                dx += (currentmode->native_width - currentmode->current_width) / 2;
                dy += (currentmode->native_height - currentmode->current_height) / 2;
        }
        *dxp = dx;
        *dyp = dy;
*/
}


int is_tablet (void)
{
        return tablet ? 1 : 0;
}

int vsync_switchmode (int hz, int oldhz)
{
        static int tempvsync;
        int w = currentmode->native_width;
        int h = currentmode->native_height;
        int d = currentmode->native_depth / 8;
//        struct MultiDisplay *md = getdisplay (&currprefs);
        struct PicassoResolution *found;

        int newh, i, cnt;
        int dbl = getvsyncrate (currprefs.chipset_refreshrate) != currprefs.chipset_refreshrate ? 2 : 1;

        if (hz < 0)
                return tempvsync;

        newh = h * oldhz / hz;
        hz = hz * dbl;

        found = NULL;
/*        for (i = 0; md->DisplayModes[i].depth >= 0 && !found; i++) {
                struct PicassoResolution *r = &md->DisplayModes[i];
                if (r->res.width == w && r->res.height == h && r->depth == d) {
                        int j;
                        for (j = 0; r->refresh[j] > 0; j++) {
                                if (r->refresh[j] == oldhz) {
                                        found = r;
                                        break;
                                }
                        }
                }
        }*/
        if (found == NULL) {
                write_log (L"refresh rate changed to %d but original rate was not found\n", hz);
                return 0;
        }

        found = NULL;
/*        for (cnt = 0; cnt <= abs (newh - h) + 1 && !found; cnt++) {
                for (i = 0; md->DisplayModes[i].depth >= 0 && !found; i++) {
                        struct PicassoResolution *r = &md->DisplayModes[i];
                        if (r->res.width == w && (r->res.height == newh + cnt || r->res.height == newh - cnt) && r->depth == d) {
                                int j;
                                for (j = 0; r->refresh[j] > 0; j++) {
                                        if (r->refresh[j] == hz) {
                                                found = r;
                                                break;
                                        }
                                }
                        }
                }
        }*/
        if (!found) {
                tempvsync = currprefs.gfx_avsync;
                changed_prefs.gfx_avsync = 0;
                write_log ("refresh rate changed to %d but no matching screenmode found, vsync disabled\n", hz);
        } else {
                newh = found->res.height;
                changed_prefs.gfx_size_fs.height = newh;
                changed_prefs.gfx_refreshrate = hz;
                write_log ("refresh rate changed to %d, new screenmode %dx%d\n", hz, w, newh);
        }
/*
        reopen (1);
*/
        return 0;
}

void serial_check_irq (void)
{
        if (data_in_serdatr)
                INTREQ_0 (0x8000 | 0x0800);
}

void serial_uartbreak (int v)
{
#ifdef SERIAL_PORT
        serialuartbreak (v);
#endif
}

void serial_hsynchandler (void)
{
#ifdef AHI
        extern void hsyncstuff(void);
        hsyncstuff();
#endif
/*
        if (serial_period_hsyncs == 0)
                return;
        serial_period_hsync_counter++;
        if (serial_period_hsyncs == 1 || (serial_period_hsync_counter % (serial_period_hsyncs - 1)) == 0) {
                checkreceive_serial (0);
                checkreceive_enet (0);
        }
        if ((serial_period_hsync_counter % serial_period_hsyncs) == 0)
                checksend (0);
*/
}

/*
static int drvsampleres[] = {
        IDR_DRIVE_CLICK_A500_1, DS_CLICK,
        IDR_DRIVE_SPIN_A500_1, DS_SPIN,
        IDR_DRIVE_SPINND_A500_1, DS_SPINND,
        IDR_DRIVE_STARTUP_A500_1, DS_START,
        IDR_DRIVE_SNATCH_A500_1, DS_SNATCH,
        -1
};
*/
int driveclick_loadresource (struct drvsample *sp, int drivetype)
{
/*
        int i, ok;

        ok = 1;
        for (i = 0; drvsampleres[i] >= 0; i += 2) {
                struct drvsample *s = sp + drvsampleres[i + 1];
                HRSRC res = FindResource (NULL, MAKEINTRESOURCE (drvsampleres[i + 0]), L"WAVE");
                if (res != 0) {
                        HANDLE h = LoadResource (NULL, res);
                        int len = SizeofResource (NULL, res);
                        uae_u8 *p = LockResource (h);
                        s->p = decodewav (p, &len);
                        s->len = len;
                } else {
                        ok = 0;
                }
        }
        return ok;
*/
}

void driveclick_fdrawcmd_close(int drive)
{
/*
        if (h[drive] != INVALID_HANDLE_VALUE)
                CloseHandle(h[drive]);
        h[drive] = INVALID_HANDLE_VALUE;
        motors[drive] = 0;
*/
}

static int driveclick_fdrawcmd_open_2(int drive)
{
/*
        TCHAR s[32];

        driveclick_fdrawcmd_close(drive);
        _stprintf (s, L"\\\\.\\fdraw%d", drive);
        h[drive] = CreateFile(s, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (h[drive] == INVALID_HANDLE_VALUE)
                return 0;
        return 1;
*/
}

int driveclick_fdrawcmd_open(int drive)
{
/*
        if (!driveclick_fdrawcmd_open_2(drive))
                return 0;
        driveclick_fdrawcmd_init(drive);
        return 1;
*/
}

void driveclick_fdrawcmd_detect(void)
{
/*
        static int detected;
        if (detected)
                return;
        detected = 1;
        if (driveclick_fdrawcmd_open_2(0))
                driveclick_pcdrivemask |= 1;
        driveclick_fdrawcmd_close(0);
        if (driveclick_fdrawcmd_open_2(1))
                driveclick_pcdrivemask |= 2;
        driveclick_fdrawcmd_close(1);
*/
}

void driveclick_fdrawcmd_seek(int drive, int cyl)
{
//        write_comm_pipe_int (dc_pipe, (drive << 8) | cyl, 1);
}
void driveclick_fdrawcmd_motor (int drive, int running)
{
//        write_comm_pipe_int (dc_pipe, 0x8000 | (drive << 8) | (running ? 1 : 0), 1);
}

void driveclick_fdrawcmd_vsync(void)
{
/*
        int i;
        for (i = 0; i < 2; i++) {
                if (motors[i] > 0) {
                        motors[i]--;
                        if (motors[i] == 0)
                                CmdMotor(h[i], 0);
                }
        }
*/
}

static int driveclick_fdrawcmd_init(int drive)
{
/*
        static int thread_ok;

        if (h[drive] == INVALID_HANDLE_VALUE)
                return 0;
        motors[drive] = 0;
        SetDataRate(h[drive], 3);
        CmdSpecify(h[drive], 0xd, 0xf, 0x1, 0);
        SetMotorDelay(h[drive], 0);
        CmdMotor(h[drive], 0);
        if (thread_ok)
                return 1;
        thread_ok = 1;
        init_comm_pipe (dc_pipe, DC_PIPE_SIZE, 3);
        uae_start_thread (L"fdrawcmd_win32", driveclick_thread, NULL, NULL);
        return 1;
*/
}

void misc_hsync_stuff (void)
{
#ifdef FILESYS
    static int misc_demux_installed;

#ifdef AHI
    {
	static int count;
	if (ahi_on) {
	    count++;
	    //15625/count freebuffer check
	    if (count > 20) {
		ahi_updatesound (1);
		count = 0;
	    }
	}
    }
#endif

#endif
}
