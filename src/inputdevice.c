 /*
  * UAE - The Un*x Amiga Emulator
  *
  * joystick/mouse emulation
  *
  * Copyright 2001-2008 Toni Wilen
  *
  * new fetures:
  * - very configurable (and very complex to configure :)
  * - supports multiple native input devices (joysticks and mice)
  * - supports mapping joystick/mouse buttons to keys and vice versa
  * - joystick mouse emulation (supports both ports)
  * - supports parallel port joystick adapter
  * - full cd32 pad support (supports both ports)
  * - fully backward compatible with old joystick/mouse configuration
  *
  */

//#define DONGLE_DEBUG

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "keyboard.h"
#include "keybuf.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "memory.h"
#include "events.h"
#include "newcpu.h"
#include "autoconf.h"
#include "inputdevice.h"
#include "uae.h"
#include "picasso96.h"
#include "catweasel.h"
#include "debug.h"
#include "ar.h"
#include "gui.h"
#include "disk.h"
#include "audio.h"
#include "savestate.h"
#include "dongle.h"

#include <ctype.h>

extern int bootrom_header, bootrom_items;

// 01 = host events
// 02 = joystick
// 04 = cia buttons
// 16 = potgo
// 32 = vsync

int inputdevice_logging = 0;

#define DIR_LEFT 1
#define DIR_RIGHT 2
#define DIR_UP 4
#define DIR_DOWN 8

#define IE_INVERT 0x80
#define IE_CDTV 0x100

struct inputevent {
	const TCHAR *confname;
	const TCHAR *name;
    int allow_mask;
    int type;
    int unit;
    int data;
};


#define JOYBUTTON_1 0 /* fire/left mousebutton */
#define JOYBUTTON_2 1 /* 2nd/right mousebutton */
#define JOYBUTTON_3 2 /* 3rd/middle mousebutton */
#define JOYBUTTON_CD32_PLAY 3
#define JOYBUTTON_CD32_RWD 4
#define JOYBUTTON_CD32_FFW 5
#define JOYBUTTON_CD32_GREEN 6
#define JOYBUTTON_CD32_YELLOW 7
#define JOYBUTTON_CD32_RED 8
#define JOYBUTTON_CD32_BLUE 9

#define INPUTEVENT_JOY1_CD32_FIRST INPUTEVENT_JOY1_CD32_PLAY
#define INPUTEVENT_JOY2_CD32_FIRST INPUTEVENT_JOY2_CD32_PLAY
#define INPUTEVENT_JOY1_CD32_LAST INPUTEVENT_JOY1_CD32_BLUE
#define INPUTEVENT_JOY2_CD32_LAST INPUTEVENT_JOY2_CD32_BLUE

/* event masks */
#define AM_KEY 1 /* keyboard allowed */
#define AM_JOY_BUT 2 /* joystick buttons allowed */
#define AM_JOY_AXIS 4 /* joystick axis allowed */
#define AM_MOUSE_BUT 8 /* mouse buttons allowed */
#define AM_MOUSE_AXIS 16 /* mouse direction allowed */
#define AM_AF 32 /* supports autofire */
#define AM_INFO 64 /* information data for gui */
#define AM_DUMMY 128 /* placeholder */
#define AM_CUSTOM 256 /* custom event */
#define AM_K (AM_KEY|AM_JOY_BUT|AM_MOUSE_BUT|AM_AF) /* generic button/switch */

#define JOYMOUSE_CDTV 8

/* event flags */
#define ID_FLAG_AUTOFIRE 1
#define ID_FLAG_TOGGLE 2
#define ID_FLAG_SAVE_MASK 0xff
#define ID_FLAG_TOGGLED 0x100

#define DEFEVENT(A, B, C, D, E, F) {#A, B, C, D, E, F },
const struct inputevent events[] = {
{0, 0, AM_K, 0, 0, 0},
#include "inputevents.def"
{0, 0, 0, 0, 0, 0}
};
#undef DEFEVENT

static struct inputdevice_functions idev[3];

static int sublevdir[2][MAX_INPUT_SUB_EVENT];

struct uae_input_device2 {
    uae_u32 buttonmask;
    int states[MAX_INPUT_DEVICE_EVENTS / 2];
};

static struct uae_input_device2 joysticks2[MAX_INPUT_DEVICES];
static struct uae_input_device2 mice2[MAX_INPUT_DEVICES];

static int input_acquired;
static int testmode;

static uae_u8 *inprec_buffer, *inprec_p;
static struct zfile *inprec_zf;
static int inprec_size;
int input_recording = 0;
static uae_u8 *inprec_plast, *inprec_plastptr;
static int inprec_div;

static uae_u32 oldbuttons[4];
static uae_u16 oldjoy[2];

static uae_u8 mouse_settings_reset[MAX_INPUT_SETTINGS][MAX_INPUT_DEVICES];
static uae_u8 joystick_settings_reset[MAX_INPUT_SETTINGS][MAX_INPUT_DEVICES];

static int isdevice (const struct uae_input_device *id)
{
    int i, j;
    for (i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
	for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
	    if (id->eventid[i][j] > 0)
		return 1;
	}
    }
    return 0;
}

int inputdevice_uaelib (TCHAR *s, TCHAR *parm)
{
    int i;

    for (i = 1; events[i].name; i++) {
		if (!_tcscmp (s, events[i].confname)) {
			handle_input_event (i, _tstol (parm), 1, 0);
	    return 1;
	}
    }
    return 0;
}

static struct uae_input_device *joysticks;
static struct uae_input_device *mice;
static struct uae_input_device *keyboards;
static struct uae_input_device_kbr_default *keyboard_default;

static int mouse_axis[MAX_INPUT_DEVICES][MAX_INPUT_DEVICE_EVENTS];
static int oldm_axis[MAX_INPUT_DEVICES][MAX_INPUT_DEVICE_EVENTS];

static int mouse_x[MAX_INPUT_DEVICES], mouse_y[MAX_INPUT_DEVICE_EVENTS];
static int mouse_delta[MAX_INPUT_DEVICES][MAX_INPUT_DEVICE_EVENTS];
static int mouse_deltanoreset[MAX_INPUT_DEVICES][MAX_INPUT_DEVICE_EVENTS];
static int joybutton[MAX_INPUT_DEVICES];
static unsigned int joydir[MAX_INPUT_DEVICE_EVENTS];
static int joydirpot[MAX_INPUT_DEVICE_EVENTS][2];
static int mouse_frame_x[2], mouse_frame_y[2];

static int mouse_port[2];
static int cd32_shifter[2];
static int cd32_pad_enabled[2];
static int parport_joystick_enabled;
static int oldmx[4], oldmy[4];
static int oleft[4], oright[4], otop[4], obot[4];

uae_u16 potgo_value;
static int pot_cap[2][2];
static uae_u8 pot_dat[2][2];
static int pot_dat_act[2][2];
static int analog_port[2][2];
static int digital_port[2][2];
#define POTDAT_DELAY_PAL 8
#define POTDAT_DELAY_NTSC 7

static int use_joysticks[MAX_INPUT_DEVICES];
static int use_mice[MAX_INPUT_DEVICES];
static int use_keyboards[MAX_INPUT_DEVICES];

#define INPUT_QUEUE_SIZE 16
struct input_queue_struct {
    int event, storedstate, state, max, framecnt, nextframecnt;
};
static struct input_queue_struct input_queue[INPUT_QUEUE_SIZE];

static void out_config (FILE *f, int id, int num, const char *s1, const char *s2)
{
    cfgfile_write (f, "input.%d.%s%d=%s\n", id, s1, num, s2);
}

static void write_config2 (FILE *f, int idnum, int i, int offset, TCHAR *tmp1, struct uae_input_device *id)
{
	TCHAR tmp2[200], tmp3[200], *p;
	int evt, got, j, k;
	TCHAR *custom;

    p = tmp2;
    got = 0;
    for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
		evt = id->eventid[i + offset][j];
	custom = id->custom[i + offset][j];
		if (custom == NULL && evt <= 0) {
	    for (k = j + 1; k < MAX_INPUT_SUB_EVENT; k++) {
				if (id->eventid[i + offset][k] > 0 || id->custom[i + offset][k] != NULL)
					break;
	    }
	    if (k == MAX_INPUT_SUB_EVENT)
		break;
	}
	if (p > tmp2) {
	    *p++ = ',';
	    *p = 0;
	}
		if (custom && _tcslen (custom) > 0)
			_stprintf (p, L"'%s'.%d", custom, id->flags[i + offset][j] & ID_FLAG_SAVE_MASK);
		else if (evt <= 0)
			_stprintf (p, L"NULL");
	else
			_stprintf (p, L"%s.%d", events[evt].confname, id->flags[i + offset][j]);
		p += _tcslen (p);
    }
    if (p > tmp2)
	out_config (f, idnum, i, tmp1, tmp2);
}

static void write_config (FILE *f, int idnum, int devnum, const char *name, const struct uae_input_device *id, const struct uae_input_device2 *id2)
{
	TCHAR tmp1[MAX_DPATH], tmp2[MAX_DPATH], *s;
    int i;

    if (!isdevice (id))
	return;
    cfgfile_write (f, "input.%d.%s.%d.disabled=%d\n", idnum, name, devnum, id->enabled ? 0 : 1);
    sprintf (tmp1, "%s.%d.axis.", name, devnum);
    for (i = 0; i < ID_AXIS_TOTAL; i++)
	write_config2 (f, idnum, i, ID_AXIS_OFFSET, tmp1, id);
	_stprintf (tmp1, L"%s.%d.button." ,name, devnum);
    for (i = 0; i < ID_BUTTON_TOTAL; i++)
	write_config2 (f, idnum, i, ID_BUTTON_OFFSET, tmp1, id);
}

static void kbrlabel (TCHAR *s)
{
    while (*s) {
		*s = _totupper (*s);
		if (*s == ' ')
			*s = '_';
	s++;
    }
}

static void write_kbr_config (FILE *f, int idnum, int devnum, struct uae_input_device *kbr)
{
	TCHAR tmp1[200], tmp2[200], tmp3[200], tmp4[200], *p;
    int i, j, k, evt, skip;

    if (!keyboard_default)
	return;
    i = 0;
    while (i < MAX_INPUT_DEVICE_EVENTS && kbr->extra[i][0] >= 0) {
	skip = 0;
	k = 0;
	while (keyboard_default[k].scancode >= 0) {
	    if (keyboard_default[k].scancode == kbr->extra[i][0]) {
		skip = 1;
		for (j = 1; j < MAX_INPUT_SUB_EVENT; j++) {
		    if (kbr->flags[i][j] || kbr->eventid[i][j] > 0)
			skip = 0;
		}
		if (keyboard_default[k].event != kbr->eventid[i][0] || kbr->flags[i][0] != 0)
		    skip = 0;
		break;
	    }
	    k++;
	}
	if (kbr->eventid[i][0] == 0 && kbr->flags[i][0] == 0 && keyboard_default[k].scancode < 0)
	    skip = 1;
	if (skip) {
	    i++;
	    continue;
	}
	p = tmp2;
	p[0] = 0;
	for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
			TCHAR *custom = kbr->custom[i][j];
	    evt = kbr->eventid[i][j];
	    if (custom == NULL && evt <= 0) {
		for (k = j + 1; k < MAX_INPUT_SUB_EVENT; k++) {
					if (kbr->eventid[i][k] > 0 || kbr->custom[i][k] != NULL)
						break;
		}
		if (k == MAX_INPUT_SUB_EVENT)
		    break;
	    }
	    if (p > tmp2) {
		*p++ = ',';
		*p = 0;
	    }
			if (custom && _tcslen (custom) > 0)
				_stprintf (p, L"'%s'.%d", custom, kbr->flags[i][j] & 0xff);
	    else if (evt > 0)
				_stprintf (p, L"%s.%d", events[evt].confname, kbr->flags[i][j]);
	    else
				_tcscat (p, L"NULL");
			p += _tcslen(p);
	}
		_stprintf (tmp3, L"%d", kbr->extra[i][0]);
	kbrlabel (tmp3);
	sprintf (tmp1, "keyboard.%d.button.%s", devnum, tmp3);
	cfgfile_write (f, "input.%d.%s=%s\n", idnum, tmp1, tmp2[0] ? tmp2 : "NULL");
	i++;
    }
}

void write_inputdevice_config (struct uae_prefs *p, FILE *f)
{
    int i, id;

    cfgfile_write (f, "input.config=%d\n", p->input_selected_setting);
    cfgfile_write (f, "input.joymouse_speed_analog=%d\n", p->input_joymouse_multiplier);
    cfgfile_write (f, "input.joymouse_speed_digital=%d\n", p->input_joymouse_speed);
    cfgfile_write (f, "input.joymouse_deadzone=%d\n", p->input_joymouse_deadzone);
    cfgfile_write (f, "input.joystick_deadzone=%d\n", p->input_joystick_deadzone);
    cfgfile_write (f, "input.mouse_speed=%d\n", p->input_mouse_speed);
    cfgfile_write (f, "input.autofire=%d\n", p->input_autofire_framecnt);
    for (id = 1; id <= MAX_INPUT_SETTINGS; id++) {
	for (i = 0; i < MAX_INPUT_DEVICES; i++)
	    write_config (f, id, i, "joystick", &p->joystick_settings[id][i], &joysticks2[i]);
	for (i = 0; i < MAX_INPUT_DEVICES; i++)
	    write_config (f, id, i, "mouse", &p->mouse_settings[id][i], &mice2[i]);
	for (i = 0; i < MAX_INPUT_DEVICES; i++)
	    write_kbr_config (f, id, i, &p->keyboard_settings[id][i]);
    }
}

static int getnum (const TCHAR **pp)
{
	const TCHAR *p = *pp;
	int v = _tstol (p);

	while (*p != 0 && *p !='.' && *p != ',')
		p++;
	if (*p == '.' || *p == ',')
		p++;
    *pp = p;
    return v;
}
static TCHAR *getstring (const TCHAR **pp)
{
    int i;
	static TCHAR str[1000];
	const TCHAR *p = *pp;

    if (*p == 0)
	return 0;
    i = 0;
    while (*p != 0 && *p !='.' && *p != ',')
	str[i++] = *p++;
    if (*p == '.' || *p == ',')
	p++;
    str[i] = 0;
    *pp = p;
    return str;
}

void reset_inputdevice_config (struct uae_prefs *pr)
{
    memset (joystick_settings_reset, 0, sizeof (joystick_settings_reset));
    memset (mouse_settings_reset, 0, sizeof (mouse_settings_reset));
}

static void clear_id (struct uae_input_device *id)
{
#ifndef _DEBUG
    int i, j;
    for (i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
	for (j = 0; j < MAX_INPUT_SUB_EVENT; j++)
	    free (id->custom[i][j]);
    }
#endif
    memset (id, 0, sizeof (struct uae_input_device));
    id->enabled = 1;
}

void read_inputdevice_config (struct uae_prefs *pr, TCHAR *option, TCHAR *value)
{
    struct uae_input_device *id = 0;
	struct inputevent *ie;
	int devnum, num, button, joystick, flags, i, subnum, idnum, keynum;
    int mask;
	TCHAR *p, *p2, *custom;

    option += 6; /* "input." */
    p = getstring (&option);
	if (!strcasecmp (p, L"config"))
		pr->input_selected_setting = _tstol (value);
	if (!strcasecmp (p, L"joymouse_speed_analog"))
		pr->input_joymouse_multiplier = _tstol (value);
	if (!strcasecmp (p, L"joymouse_speed_digital"))
		pr->input_joymouse_speed = _tstol (value);
	if (!strcasecmp (p, L"joystick_deadzone"))
		pr->input_joystick_deadzone = _tstol (value);
	if (!strcasecmp (p, L"joymouse_deadzone"))
		pr->input_joymouse_deadzone = _tstol (value);
	if (!strcasecmp (p, L"mouse_speed"))
		pr->input_mouse_speed = _tstol (value);
	if (!strcasecmp (p, L"autofire"))
		pr->input_autofire_framecnt = _tstol (value);
	if (!strcasecmp (p, L"analog_joystick_multiplier"))
		pr->input_analog_joystick_mult = _tstol (value);
	if (!strcasecmp (p, L"analog_joystick_offset"))
		pr->input_analog_joystick_offset = _tstol (value);

	idnum = _tstol (p);
    if (idnum <= 0 || idnum > MAX_INPUT_SETTINGS)
	return;

	if (_tcsncmp (option, L"mouse.", 6) == 0) {
	p = option + 6;
	} else if (_tcsncmp (option, L"joystick.", 9) == 0) {
		p = option + 9;
	} else if (_tcsncmp (option, L"keyboard.", 9) == 0) {
		p = option + 9;
	} else
		return;

	devnum = getnum (&p);
	if (devnum < 0 || devnum >= MAX_INPUT_DEVICES)
	    return;

    p2 = getstring (&p);
    if (!p2)
	return;

	if (_tcsncmp (option, L"mouse.", 6) == 0) {
		id = &pr->mouse_settings[idnum][devnum];
		joystick = 0;
	} else if (_tcsncmp (option, L"joystick.", 9) == 0) {
		id = &pr->joystick_settings[idnum][devnum];
		joystick = 1;
	} else if (_tcsncmp (option, L"keyboard.", 9) == 0) {
		id = &pr->keyboard_settings[idnum][devnum];
		joystick = -1;
	}
	if (!id)
		return;

	if (!_tcscmp (p2, L"name")) {
		xfree (id->configname);
		id->configname = my_strdup (value);
		return;
	}
	if (!_tcscmp (p2, L"friendlyname")) {
		xfree (id->name);
		id->name = my_strdup (value);
		return;
	}

	if (!_tcscmp (p2, L"empty")) {
		clear_id (id);
		id->enabled = 1;
		return;
	}

	if (!_tcscmp (p2, L"disabled")) {
	int disabled;
	p = value;
	disabled = getnum (&p);
	id->enabled = disabled == 0 ? 1 : 0;
	return;
    }

    if (joystick < 0) {
	num = getnum (&p);
	keynum = 0;
	while (id->extra[keynum][0] >= 0) {
	    if (id->extra[keynum][0] == num)
		break;
	    keynum++;
	}
	if (id->extra[keynum][0] < 0)
	    return;
    } else {
	button = -1;
		if (!_tcscmp (p2, L"axis"))
	    button = 0;
		else if(!_tcscmp (p2, L"button"))
	    button = 1;
	if (button < 0)
	    return;
	num = getnum (&p);
    }
    p = value;

    custom = NULL;
    for (subnum = 0; subnum < MAX_INPUT_SUB_EVENT; subnum++) {
	free (custom);
	custom = NULL;
	p2 = getstring (&p);
	if (!p2)
	    break;
	i = 1;
	while (events[i].name) {
			if (!_tcscmp (events[i].confname, p2))
		break;
	    i++;
	}
	ie = &events[i];
	if (!ie->name) {
	    ie = &events[0];
			if (_tcslen (p2) > 2 && p2[0] == '\'' && p2[_tcslen (p2) - 1] == '\'') {
		custom = my_strdup (p2 + 1);
				custom[_tcslen (custom) - 1] = 0;
	    }
	}
		flags = 0;
		if (p[-1] == '.')
	flags = getnum (&p);
	if (custom == NULL && ie->name == NULL) {
			if (!_tcscmp(p2, L"NULL")) {
				if (joystick < 0) {
		id->eventid[keynum][subnum] = 0;
		id->flags[keynum][subnum] = 0;
				} else if (button) {
					id->eventid[num + ID_BUTTON_OFFSET][subnum] = 0;
					id->flags[num + ID_BUTTON_OFFSET][subnum] = 0;
				} else {
					id->eventid[num + ID_AXIS_OFFSET][subnum] = 0;
					id->flags[num + ID_AXIS_OFFSET][subnum] = 0;
				}
	    }
	    continue;
	}
	if (joystick < 0) {
	    if (!(ie->allow_mask & AM_K))
		continue;
	    id->eventid[keynum][subnum] = ie - events;
	    id->flags[keynum][subnum] = flags;
	    free (id->custom[keynum][subnum]);
	    id->custom[keynum][subnum] = custom;
	} else  if (button) {
	    if (joystick)
		mask = AM_JOY_BUT;
	    else
		mask = AM_MOUSE_BUT;
	    if (!(ie->allow_mask & mask))
		continue;
	    id->eventid[num + ID_BUTTON_OFFSET][subnum] = ie - events;
	    id->flags[num + ID_BUTTON_OFFSET][subnum] = flags;
	    free (id->custom[num + ID_BUTTON_OFFSET][subnum]);
	    id->custom[num + ID_BUTTON_OFFSET][subnum] = custom;
	} else {
	    if (joystick)
		mask = AM_JOY_AXIS;
	    else
		mask = AM_MOUSE_AXIS;
	    if (!(ie->allow_mask & mask))
		continue;
	    id->eventid[num + ID_AXIS_OFFSET][subnum] = ie - events;
	    id->flags[num + ID_AXIS_OFFSET][subnum] = flags;
	    free (id->custom[num + ID_AXIS_OFFSET][subnum]);
	    id->custom[num + ID_AXIS_OFFSET][subnum] = custom;
	}
	custom = NULL;
    }
    free (custom);
}

static int mouseedge_alive, mousehack_alive_cnt;
static int lastmx, lastmy;
static uaecptr magicmouse_ibase, magicmouse_gfxbase;
static int dimensioninfo_width, dimensioninfo_height, dimensioninfo_dbl;
static int vp_xoffset, vp_yoffset, mouseoffset_x, mouseoffset_y;
static int tablet_maxx, tablet_maxy, tablet_data;

int mousehack_alive (void)
{
	return mousehack_alive_cnt > 0 ? mousehack_alive_cnt : 0;
}

#define MH_E 0
#define MH_CNT 2
#define MH_MAXX 4
#define MH_MAXY 6
#define MH_MAXZ 8
#define MH_X 10
#define MH_Y 12
#define MH_Z 14
#define MH_RESX 16
#define MH_RESY 18
#define MH_MAXAX 20
#define MH_MAXAY 22
#define MH_MAXAZ 24
#define MH_AX 26
#define MH_AY 28
#define MH_AZ 30
#define MH_PRESSURE 32
#define MH_BUTTONBITS 34
#define MH_INPROXIMITY 38
#define MH_ABSX 40
#define MH_ABSY 42

#define MH_END 44
#define MH_START 4

int inputdevice_is_tablet (void)
{
	int v;
	if (!uae_boot_rom)
		return 0;
	if (currprefs.input_tablet == TABLET_OFF)
		return 0;
	if (currprefs.input_tablet == TABLET_MOUSEHACK)
		return -1;
	v = is_tablet ();
	if (!v)
		return 0;
	if (kickstart_version < 37)
		return v ? -1 : 0;
	return v ? 1 : 0;
}

static int getmhoffset (void)
{
	if (!uae_boot_rom)
		return 0;
	return get_long (rtarea_base + bootrom_header + 7 * 4) + bootrom_header;
}

static void mousehack_enable (void)
{
#ifdef FILESYS /* Internal mousehack depends on filesys boot-rom */
	int off, mode;

	if (!uae_boot_rom || currprefs.input_tablet == TABLET_OFF)
		return;
	off = getmhoffset ();
	if (rtarea[off + MH_E])
		return;
	mode = 0x80;
	if (currprefs.input_tablet == TABLET_MOUSEHACK)
		mode |= 1;
	if (inputdevice_is_tablet () > 0)
		mode |= 2;
	write_log (L"Tablet driver enabled (%s)\n", ((mode & 3) == 3 ? "tablet+mousehack" : ((mode & 3) == 2) ? "tablet" : "mousehack"));
	rtarea[off + MH_E] = 0x80;
#endif
}

void get_custom_mouse_limits (int *w, int *h, int *dx, int *dy, int dbl);

void inputdevice_tablet_strobe (void)
{
    uae_u8 *p;
	uae_u32 off;

	mousehack_enable ();
	if (!uae_boot_rom)
	return;
	if (!tablet_data)
		return;
	off = getmhoffset ();
	p = rtarea + off;
	p[MH_CNT]++;
}

void inputdevice_tablet (int x, int y, int z, int pressure, uae_u32 buttonbits, int inproximity, int ax, int ay, int az)
{
	uae_u8 *p;
	uae_u8 tmp[MH_END];
	uae_u32 off;

	mousehack_enable ();
	if (inputdevice_is_tablet () <= 0)
		return;
	//write_log (L"%d %d %d %d %08X %d %d %d %d\n", x, y, z, pressure, buttonbits, inproximity, ax, ay, az);
	off = getmhoffset ();
	p = rtarea + off;

	memcpy (tmp, p + MH_START, MH_END - MH_START); 
#if 0
	if (currprefs.input_magic_mouse) {
		int maxx, maxy, diffx, diffy;
		int dw, dh, ax, ay, aw, ah;
		float xmult, ymult;
		float fx, fy;

		fx = (float)x;
		fy = (float)y;
		desktop_coords (&dw, &dh, &ax, &ay, &aw, &ah);
		xmult = (float)tablet_maxx / dw;
		ymult = (float)tablet_maxy / dh;

		diffx = 0;
		diffy = 0;
		if (picasso_on) {
			maxx = gfxvidinfo.width;
			maxy = gfxvidinfo.height;
		} else {
			get_custom_mouse_limits (&maxx, &maxy, &diffx, &diffy);
		}
		diffx += ax;
		diffy += ah;

		fx -= diffx * xmult;
		if (fx < 0)
			fx = 0;
		if (fx >= aw * xmult)
			fx = aw * xmult - 1;
		fy -= diffy * ymult;
		if (fy < 0)
			fy = 0;
		if (fy >= ah * ymult)
			fy = ah * ymult - 1;

		x = (int)(fx * (aw * xmult) / tablet_maxx + 0.5);
		y = (int)(fy * (ah * ymult) / tablet_maxy + 0.5);

	}
#endif
	p[MH_X] = x >> 8;
	p[MH_X + 1] = x;
	p[MH_Y] = y >> 8;
	p[MH_Y + 1] = y;
	p[MH_Z] = z >> 8;
	p[MH_Z + 1] = z;

	p[MH_AX] = ax >> 8;
	p[MH_AX + 1] = ax;
	p[MH_AY] = ay >> 8;
	p[MH_AY + 1] = ay;
	p[MH_AZ] = az >> 8;
	p[MH_AZ + 1] = az;

	p[MH_MAXX] = tablet_maxx >> 8;
	p[MH_MAXX + 1] = tablet_maxx;
	p[MH_MAXY] = tablet_maxy >> 8;
	p[MH_MAXY + 1] = tablet_maxy;

	p[MH_PRESSURE] = pressure >> 8;
	p[MH_PRESSURE + 1] = pressure;

	p[MH_BUTTONBITS + 0] = buttonbits >> 24;
	p[MH_BUTTONBITS + 1] = buttonbits >> 16;
	p[MH_BUTTONBITS + 2] = buttonbits >>  8;
	p[MH_BUTTONBITS + 3] = buttonbits >>  0;

	if (inproximity < 0) {
		p[MH_INPROXIMITY] = p[MH_INPROXIMITY + 1] = 0xff;
	} else {
		p[MH_INPROXIMITY] = 0;
		p[MH_INPROXIMITY + 1] = inproximity ? 1 : 0;
	}

	if (!memcmp (tmp, p + MH_START, MH_END - MH_START))
		return;
	rtarea[off + MH_E] = 0xc0 | 2;
	p[MH_CNT]++;
}

void inputdevice_tablet_info (int maxx, int maxy, int maxz, int maxax, int maxay, int maxaz, int xres, int yres)
{
	uae_u8 *p;

	if (!uae_boot_rom)
		return;
	p = rtarea + getmhoffset ();

	tablet_maxx = maxx;
	tablet_maxy = maxy;
	p[MH_MAXX] = maxx >> 8;
	p[MH_MAXX + 1] = maxx;
	p[MH_MAXY] = maxy >> 8;
	p[MH_MAXY + 1] = maxy;
	p[MH_MAXZ] = maxz >> 8;
	p[MH_MAXZ + 1] = maxz;

	p[MH_RESX] = xres >> 8;
	p[MH_RESX + 1] = xres;
	p[MH_RESY] = yres >> 8;
	p[MH_RESY + 1] = yres;

	p[MH_MAXAX] = maxax >> 8;
	p[MH_MAXAX + 1] = maxax;
	p[MH_MAXAY] = maxay >> 8;
	p[MH_MAXAY + 1] = maxay;
	p[MH_MAXAZ] = maxaz >> 8;
	p[MH_MAXAZ + 1] = maxaz;
}


void getgfxoffset (int *dx, int *dy, int*,int*);

static void inputdevice_mh_abs (int x, int y)
{
	uae_u8 *p;
	uae_u8 tmp[4];
	uae_u32 off;

	mousehack_enable ();
	off = getmhoffset ();
	p = rtarea + off;

	memcpy (tmp, p + MH_ABSX, 4);

	x -= mouseoffset_x + 1;
	y -= mouseoffset_y + 2;

	p[MH_ABSX] = x >> 8;
	p[MH_ABSX + 1] = x;
	p[MH_ABSY] = y >> 8;
	p[MH_ABSY + 1] = y;

	if (!memcmp (tmp, p + MH_ABSX, 4))
		return;
	rtarea[off + MH_E] = 0xc0 | 1;
	p[MH_CNT]++;
	tablet_data = 1;
}

#if 0
static void inputdevice_mh_abs_v36 (int x, int y)
{
	uae_u8 *p;
	uae_u8 tmp[MH_END];
	uae_u32 off;
	int maxx, maxy, diffx, diffy;
	int fdy, fdx, fmx, fmy;

	mousehack_enable ();
	off = getmhoffset ();
	p = rtarea + off;

	memcpy (tmp, p + MH_START, MH_END - MH_START); 

	getgfxoffset (&fdx, &fdy, &fmx, &fmy);
	x -= fdx;
	y -= fdy;
	x += vp_xoffset;
	y += vp_yoffset;

	diffx = diffy = 0;
	maxx = maxy = 0;

	if (picasso_on) {
		maxx = picasso96_state.Width;
		maxy = picasso96_state.Height;
	} else if (dimensioninfo_width > 0 && dimensioninfo_height > 0) {
		maxx = dimensioninfo_width;
		maxy = dimensioninfo_height;
		get_custom_mouse_limits (&maxx, &maxy, &diffx, &diffy, dimensioninfo_dbl);
	} else {
		uaecptr gb = get_gfxbase ();
		maxx = 0; maxy = 0;
		if (gb) {
			maxy = get_word (gb + 216);
			maxx = get_word (gb + 218);
		}
		get_custom_mouse_limits (&maxx, &maxy, &diffx, &diffy, 0);
	}
#if 0
	{
		uaecptr gb = get_intuitionbase ();
		maxy = get_word (gb + 1344 + 2);
		maxx = get_word (gb + 1348 + 2);
		write_log (L"%d %d\n", maxx, maxy);
	}
#endif
#if 1
	{
		uaecptr gb = get_gfxbase ();
		uaecptr view = get_long (gb + 34);
		if (view) {
			uaecptr vp = get_long (view);
			if (vp) {
				int w, h, dw, dh;
				w = get_word (vp + 24);
				h = get_word (vp + 26) * 2;
				dw = get_word (vp + 28);
				dh = get_word (vp + 30);
				//write_log (L"%d %d %d %d\n", w, h, dw, dh);
				if (w < maxx)
					maxx = w;
				if (h < maxy)
					maxy = h;
				x -= dw;
				y -= dh;
			}
		}
		//write_log (L"* %d %d\n", get_word (gb + 218), get_word (gb + 216));
	}
	//write_log (L"%d %d\n", maxx, maxy);
#endif

	maxx = maxx * 1000 / fmx;
	maxy = maxy * 1000 / fmy;

	if (maxx <= 0)
		maxx = 1;
	if (maxy <= 0)
		maxy = 1;

	x -= diffx;
	if (x < 0)
		x = 0;
	if (x >= maxx)
		x = maxx - 1;

	y -= diffy;
	if (y < 0)
		y = 0;
	if (y >= maxy)
		y = maxy - 1;

	//write_log (L"%d %d %d %d\n", x, y, maxx, maxy);

	p[MH_X] = x >> 8;
	p[MH_X + 1] = x;
	p[MH_Y] = y >> 8;
	p[MH_Y + 1] = y;
	p[MH_MAXX] = maxx >> 8;
	p[MH_MAXX + 1] = maxx;
	p[MH_MAXY] = maxy >> 8;
	p[MH_MAXY + 1] = maxy;

	p[MH_Z] = p[MH_Z + 1] = 0;
	p[MH_MAXZ] = p[MH_MAXZ + 1] = 0;
	p[MH_AX] = p[MH_AX + 1] = 0;
	p[MH_AY] = p[MH_AY + 1] = 0;
	p[MH_AZ] = p[MH_AZ + 1] = 0;
	p[MH_PRESSURE] = p[MH_PRESSURE + 1] = 0;
	p[MH_INPROXIMITY] = p[MH_INPROXIMITY + 1] = 0xff;

	if (!memcmp (tmp, p + MH_START, MH_END - MH_START))
		return;
	p[MH_CNT]++;
	tablet_data = 1;
}
#endif

static void mousehack_helper (void)
{
#ifdef FILESYS /* Internal mousehack depends on filesys boot-rom */
	int x, y;
	int fdy, fdx, fmx, fmy;

	if (currprefs.input_magic_mouse == 0 || currprefs.input_tablet < TABLET_MOUSEHACK)
		return;
#if 0
	if (kickstart_version >= 36) {
		inputdevice_mh_abs_v36 (lastmx, lastmy);
		return;
	}
#endif
	x = lastmx;
	y = lastmy;
	getgfxoffset (&fdx, &fdy, &fmx, &fmy);


#ifdef PICASSO96
	if (picasso_on) {
		x -= picasso96_state.XOffset;
		y -= picasso96_state.YOffset;
		x = x * fmx / 1000;
		y = y * fmy / 1000;
		x -= fdx * fmx / 1000;
		y -= fdy * fmy / 1000;
	} else
#endif
    {
		x = x * fmx / 1000;
		y = y * fmy / 1000;
		x -= fdx * fmx / 1000 - 1;
		y -= fdy * fmy / 1000 - 2;
		x = coord_native_to_amiga_x (x);
		y = coord_native_to_amiga_y (y) << 1;
    }
	inputdevice_mh_abs (x, y);
#endif
}

static int mouseedge_x, mouseedge_y, mouseedge_time;
#define MOUSEEDGE_RANGE 100
#define MOUSEEDGE_TIME 2

extern void setmouseactivexy (int,int,int);

STATIC_INLINE int adjust (int val)
{
    if (val > 127)
	return 127;
    else if (val < -127)
	return -127;
    return val;
}

int getbuttonstate (int joy, int button)
{
    return joybutton[joy] & (1 << button);
}

static int getvelocity (int num, int subnum, int pct)
{
	int val;
	int v;

	if (pct > 1000)
		pct = 1000;
	val = mouse_delta[num][subnum];
	v = val * pct / 1000;
	if (!v) {
		if (val < -maxvpos / 2)
			v = -2;
		else if (val < 0)
			v = -1;
		else if (val > maxvpos / 2)
			v = 2;
		else if (val > 0)
			v = 1;
	}
	if (!mouse_deltanoreset[num][subnum])
		mouse_delta[num][subnum] -= v;
	return v;
}

static void mouseupdate (int pct, int vsync)
{
    int v, i;
	int max = 120;
	static int mxd, myd;


    for (i = 0; i < 2; i++) {

		v = getvelocity (i, 0, pct);
		mxd += v;
		mouse_x[i] += v;

		v = getvelocity (i, 1, pct);
		myd += v;
		mouse_y[i] += v;

		v = getvelocity (i, 2, pct);
	if (v > 0)
	    record_key (0x7a << 1);
	else if (v < 0)
	    record_key (0x7b << 1);
	if (!mouse_deltanoreset[i][2])
	    mouse_delta[i][2] = 0;

		if (mouse_frame_x[i] - mouse_x[i] > max)
			mouse_x[i] = mouse_frame_x[i] - max;
		if (mouse_frame_x[i] - mouse_x[i] < -max)
			mouse_x[i] = mouse_frame_x[i] + max;

		if (mouse_frame_y[i] - mouse_y[i] > max)
			mouse_y[i] = mouse_frame_y[i] - max;
		if (mouse_frame_y[i] - mouse_y[i] < -max)
			mouse_y[i] = mouse_frame_y[i] + max;

		if (!vsync) {
		    mouse_frame_x[i] = mouse_x[i];
		    mouse_frame_y[i] = mouse_y[i];
		}
    }
}

static int input_vpos, input_frame;
extern int vpos;
static void readinput (void)
{
	uae_u32 totalvpos;
	int diff;

	totalvpos = input_frame * maxvpos + vpos;
	diff = totalvpos - input_vpos;
	if (diff > 0) {
		if (diff < 10) {
			mouseupdate (0, 0);
		} else {
			mouseupdate (diff * 1000 / maxvpos, 0);
    }
    }
	input_vpos = totalvpos;

}

int getjoystate (int joy)
{
	int left = 1, right = 1, top = 1, bot = 1;
	uae_u16 v;

	if (inputdevice_logging & 2)
		write_log (L"JOY%dDAT %08x\n", joy, M68K_GETPC);
    readinput ();
    if (joydir[joy] & DIR_LEFT)
		left = 0;
    if (joydir[joy] & DIR_RIGHT)
		right = 0;
    if (joydir[joy] & DIR_UP)
		top = 0;
    if (joydir[joy] & DIR_DOWN)
		bot = 0;
    v = (uae_u8)mouse_x[joy] | (mouse_y[joy] << 8);
	if (!left || !right || !top || !bot || !mouse_port[joy]) {
		int b9, b8, b1, b0;
		mouse_x[joy] &= ~3;
		mouse_y[joy] &= ~3;
		b0 = bot ^ right;
		b1 = right ^ 1;
		b8 = top ^ left;
		b9 = left ^ 1;
	v &= ~0x0303;
		v |= (b0 << 0) | (b1 << 1) | (b8 << 8) | (b9 << 9);
    }
#ifdef DONGLE_DEBUG
    if (notinrom ())
		write_log (L"JOY%dDAT %04X %s\n", joy, v, debuginfo (0));
#endif
    return v;
}

uae_u16 JOY0DAT (void)
{
	uae_u16 v;
	v = getjoystate (0);
	v = dongle_joydat (0, v);
	return v;
}

uae_u16 JOY1DAT (void)
{
	uae_u16 v;
	v = getjoystate (1);
	v = dongle_joydat (1, v);
	return v;
}

void JOYTEST (uae_u16 v)
{
    mouse_x[0] &= 3;
    mouse_y[0] &= 3;
    mouse_x[1] &= 3;
    mouse_y[1] &= 3;
    mouse_x[0] |= v & 0xFC;
    mouse_x[1] |= v & 0xFC;
    mouse_y[0] |= (v >> 8) & 0xFC;
    mouse_y[1] |= (v >> 8) & 0xFC;
    mouse_frame_x[0] = mouse_x[0];
    mouse_frame_y[0] = mouse_y[0];
    mouse_frame_x[1] = mouse_x[1];
    mouse_frame_y[1] = mouse_y[1];
	dongle_joytest (v);
	if (inputdevice_logging & 2)
		write_log ("JOYTEST: %04X PC=%x\n", v , M68K_GETPC);
}

static uae_u8 parconvert (uae_u8 v, int jd, int shift)
{
    if (jd & DIR_UP)
	v &= ~(1 << shift);
    if (jd & DIR_DOWN)
	v &= ~(2 << shift);
    if (jd & DIR_LEFT)
	v &= ~(4 << shift);
    if (jd & DIR_RIGHT)
	v &= ~(8 << shift);
    return v;
}

/* io-pins floating: dir=1 -> return data, dir=0 -> always return 1 */
uae_u8 handle_parport_joystick (int port, uae_u8 pra, uae_u8 dra)
{
    uae_u8 v;
    switch (port)
    {
	case 0:
	v = (pra & dra) | (dra ^ 0xff);
	if (parport_joystick_enabled) {
	    v = parconvert (v, joydir[2], 0);
	    v = parconvert (v, joydir[3], 4);
	}
	return v;
	case 1:
	v = ((pra & dra) | (dra ^ 0xff)) & 0x7;
	if (parport_joystick_enabled) {
	    if (getbuttonstate (2, 0)) v &= ~4;
	    if (getbuttonstate (3, 0)) v &= ~1;
	}
	return v;
	default:
	abort ();
	return 0;
    }
}

static void charge_cap (int joy, int idx, int charge)
{
	if (charge < -1 || charge > 1)
		charge = charge * 80;
	pot_cap[joy][idx] += charge;
	if (pot_cap[joy][idx] < 0)
		pot_cap[joy][idx] = 0;
	if (pot_cap[joy][idx] > 511)
		pot_cap[joy][idx] = 511;
}

static void cap_check (void)
{
	int joy, i;

	for (joy = 0; joy < 2; joy++) {
		for (i = 0; i < 2; i++) {
			int charge = 0, dong, joypot;
			uae_u16 pdir = 0x0200 << (joy * 4 + i * 2); /* output enable */
			uae_u16 pdat = 0x0100 << (joy * 4 + i * 2); /* data */
			int isbutton = getbuttonstate (joy, i == 0 ? JOYBUTTON_3 : JOYBUTTON_2);

			if (cd32_pad_enabled[joy])
				continue;
			dong = dongle_analogjoy (joy, i);
			if (dong >= 0) {
				isbutton = 0;
				joypot = dong;
				if (pot_cap[joy][i] < joypot)
					charge = 1; // slow charge via dongle resistor
			} else {
				joypot = joydirpot[joy][i];
				if (analog_port[joy][i] && pot_cap[joy][i] < joypot)
					charge = 1; // slow charge via pot variable resistor
				if ((digital_port[joy][i] || mouse_port[joy]))
					charge = 1; // slow charge via pull-up resistor
			}
			if (!(potgo_value & pdir)) { // input?
				if (pot_dat_act[joy][i])
					pot_dat[joy][i]++;
				/* first 8 lines after potgo has been started = discharge cap */
				if (pot_dat_act[joy][i] == 1) {
					if (pot_dat[joy][i] < (currprefs.ntscmode ? POTDAT_DELAY_NTSC : POTDAT_DELAY_PAL)) {
						charge = -2; /* fast discharge delay */
					} else {
						pot_dat_act[joy][i] = 2;
						pot_dat[joy][i] = 0;
					}
				}
				if (dong >= 0) {
					if (pot_dat_act[joy][i] == 2 && pot_cap[joy][i] >= joypot)
						pot_dat_act[joy][i] = 0;
				} else {
					if (analog_port[joy][i] && pot_dat_act[joy][i] == 2 && pot_cap[joy][i] >= joypot)
						pot_dat_act[joy][i] = 0;
					if ((digital_port[joy][i] || mouse_port[joy]) && pot_dat_act[joy][i] == 2) {
						if (pot_cap[joy][i] >= 10 && !isbutton)
							pot_dat_act[joy][i] = 0;
					}
				}
			} else { // output?
				charge = (potgo_value & pdat) ? 2 : -2; /* fast (dis)charge if output */
				if (potgo_value & pdat)
					pot_dat_act[joy][i] = 0; // instant stop if output+high
				if (isbutton)
					pot_dat[joy][i]++; // "free running" if output+low
			}

			if (isbutton)
				charge = -2; // button press overrides everything

			if (currprefs.cs_cdtvcd) {
				/* CDTV P9 is not floating */
				if (!(potgo_value & pdir) && i == 1 && charge == 0)
					charge = 2;
			}
			/* official Commodore mouse has pull-up resistors in button lines
			* NOTE: 3rd party mice may not have pullups! */
			if (dong < 0 && mouse_port[joy] && charge == 0)
				charge = 2;
			/* emulate pullup resistor if button mapped because there too many broken
			* programs that read second button in input-mode (and most 2+ button pads have
			* pullups)
			*/
			if (dong < 0 && digital_port[joy][i] && charge == 0)
				charge = 2;

			charge_cap (joy, i, charge);
		}
	}
}

uae_u8 handle_joystick_buttons (uae_u8 dra)
{
    uae_u8 but = 0;
    int i;

	cap_check ();
    for (i = 0; i < 2; i++) {
	if (cd32_pad_enabled[i]) {
	    uae_u16 p5dir = 0x0200 << (i * 4);
	    uae_u16 p5dat = 0x0100 << (i * 4);
	    but |= 0x40 << i;
	    /* Red button is connected to fire when p5 is 1 or floating */
	    if (!(potgo_value & p5dir) || ((potgo_value & p5dat) && (potgo_value & p5dir))) {
		if (getbuttonstate (i, JOYBUTTON_CD32_RED))
		    but &= ~(0x40 << i);
	    }
	} else if (!getbuttonstate (i, JOYBUTTON_1)) {
	    but |= 0x40 << i;
	}
    }
	if (inputdevice_logging & 4)
		write_log ("BFE001: %02X:%02X %x\n", dra, but, M68K_GETPC);
    return but;
}

/* joystick 1 button 1 is used as a output for incrementing shift register */
void handle_cd32_joystick_cia (uae_u8 pra, uae_u8 dra)
{
    static int oldstate[2];
    int i;

	cap_check ();
    for (i = 0; i < 2; i++) {
	uae_u8 but = 0x40 << i;
	uae_u16 p5dir = 0x0200 << (i * 4); /* output enable P5 */
	uae_u16 p5dat = 0x0100 << (i * 4); /* data P5 */
	if (!(potgo_value & p5dir) || !(potgo_value & p5dat)) {
	    if ((dra & but) && (pra & but) != oldstate[i]) {
		if (!(pra & but)) {
		    cd32_shifter[i]--;
		    if (cd32_shifter[i] < 0)
			cd32_shifter[i] = 0;
		}
	    }
	}
	oldstate[i] = pra & but;
    }
}

/* joystick port 1 button 2 is input for button state */
static uae_u16 handle_joystick_potgor (uae_u16 potgor)
{
    int i;

	cap_check ();
    for (i = 0; i < 2; i++) {
	uae_u16 p9dir = 0x0800 << (i * 4); /* output enable P9 */
	uae_u16 p9dat = 0x0400 << (i * 4); /* data P9 */
	uae_u16 p5dir = 0x0200 << (i * 4); /* output enable P5 */
	uae_u16 p5dat = 0x0100 << (i * 4); /* data P5 */

	if (cd32_pad_enabled[i]) {

	    /* p5 is floating in input-mode */
	    potgor &= ~p5dat;
	    potgor |= potgo_value & p5dat;
	    if (!(potgo_value & p9dir))
		potgor |= p9dat;
	    /* P5 output and 1 -> shift register is kept reset (Blue button) */
	    if ((potgo_value & p5dir) && (potgo_value & p5dat))
		cd32_shifter[i] = 8;
	    /* shift at 1 == return one, >1 = return button states */
	    if (cd32_shifter[i] == 0)
		potgor &= ~p9dat; /* shift at zero == return zero */
	    if (cd32_shifter[i] >= 2 && (joybutton[i] & ((1 << JOYBUTTON_CD32_PLAY) << (cd32_shifter[i] - 2))))
		potgor &= ~p9dat;

	} else {

		potgor &= ~p5dat;
			if (pot_cap[i][0] > 100)
				potgor |= p5dat;

		potgor &= ~p9dat;
			if (pot_cap[i][1] > 100)
				potgor |= p9dat;

	}
    }
    return potgor;
}


static int inputdelay;

void inputdevice_hsync (void)
{
	static int cnt;
	cap_check ();

#ifdef CATWEASEL
    catweasel_hsync ();
#endif
	if ((++cnt & 63) == 63) {
		idev[IDTYPE_MOUSE].read ();
		idev[IDTYPE_JOYSTICK].read ();
		idev[IDTYPE_KEYBOARD].read ();
	}
    if (inputdelay > 0) {
	inputdelay--;
	if (inputdelay == 0) {
	    idev[IDTYPE_JOYSTICK].read ();
	    idev[IDTYPE_KEYBOARD].read ();
	}
    }
}

static uae_u16 POTDAT (int joy)
{
	uae_u16 v = (pot_dat[joy][1] << 8) | pot_dat[joy][0];
	if (inputdevice_logging & 16)
		write_log (L"POTDAT%d: %04X %08X\n", joy, v, M68K_GETPC);
	return v;
}

uae_u16 POT0DAT (void)
{
	return POTDAT (0);
}
uae_u16 POT1DAT (void)
{
	return POTDAT (1);
}

/* direction=input, data pin floating, last connected logic level or previous status
 *                  written when direction was ouput
 *                  otherwise it is currently connected logic level.
 * direction=output, data pin is current value, forced to zero if joystick button is pressed
 * it takes some tens of microseconds before data pin changes state
 */

void POTGO (uae_u16 v)
{
	int i, j;

	if (inputdevice_logging & 16)
		write_log (L"POTGO_W: %04X %08X\n", v, M68K_GETPC);
#ifdef DONGLE_DEBUG
    if (notinrom ())
		write_log (L"POTGO %04X %s\n", v, debuginfo(0));
#endif
	dongle_potgo (v);
    potgo_value = potgo_value & 0x5500; /* keep state of data bits */
    potgo_value |= v & 0xaa00; /* get new direction bits */
    for (i = 0; i < 8; i += 2) {
	uae_u16 dir = 0x0200 << i;
	if (v & dir) {
	    uae_u16 data = 0x0100 << i;
	    potgo_value &= ~data;
	    potgo_value |= v & data;
	}
    }
    for (i = 0; i < 2; i++) {
	if (cd32_pad_enabled[i]) {
	    uae_u16 p5dir = 0x0200 << (i * 4); /* output enable P5 */
	    uae_u16 p5dat = 0x0100 << (i * 4); /* data P5 */
	    if ((potgo_value & p5dir) && (potgo_value & p5dat))
		cd32_shifter[i] = 8;
	}
    }
    if (v & 1) {
		for (i = 0; i < 2; i++) {
			for (j = 0; j < 2; j++) {
				pot_dat_act[i][j] = 1;
				pot_dat[i][j] = 0;
			}
		}
    }
}

uae_u16 POTGOR (void)
{
	uae_u16 v;

	v = handle_joystick_potgor (potgo_value) & 0x5500;
	v = dongle_potgor (v);
#ifdef DONGLE_DEBUG
    if (notinrom ())
	write_log ("POTGOR %04.4X %s\n", v, debuginfo(0));
#endif
	if (inputdevice_logging & 16)
		write_log (L"POTGO_R: %04X %08X %d\n", v, M68K_GETPC, cd32_shifter[1]);
    return v;
}

static int check_input_queue (int event)
{
	struct input_queue_struct *iq;
    int i;
    for (i = 0; i < INPUT_QUEUE_SIZE; i++) {
	iq = &input_queue[i];
	if (iq->event == event)
	    return i;
    }
    return -1;
}

static void queue_input_event (int event, int state, int max, int framecnt, int autofire)
{
    struct input_queue_struct *iq;
    int i = check_input_queue (event);

    if (event <= 0)
		return;
    if (state < 0 && i >= 0) {
		iq = &input_queue[i];
		iq->nextframecnt = -1;
		iq->framecnt = -1;
		iq->event = 0;
		if (iq->state == 0)
		    handle_input_event (event, 0, 1, 0);
    } else if (i < 0) {
		for (i = 0; i < INPUT_QUEUE_SIZE; i++) {
		    iq = &input_queue[i];
			if (iq->framecnt < 0)
				break;
	}
	if (i == INPUT_QUEUE_SIZE) {
	    write_log ("input queue overflow\n");
	    return;
	}
	iq->event = event;
	iq->state = iq->storedstate = state;
	iq->max = max;
	iq->framecnt = framecnt;
	iq->nextframecnt = autofire > 0 ? framecnt : -1;
    }
}

static uae_u8 keybuf[256];
static int inputcode_pending, inputcode_pending_state;

void inputdevice_release_all_keys (void)
{
   int i;

   for (i = 0; i < 0x80; i++) {
	if (keybuf[i] != 0) {
	    keybuf[i] = 0;
	    record_key (i << 1|1);
	}
   }
}


void inputdevice_add_inputcode (int code, int state)
{
    inputcode_pending = code;
    inputcode_pending_state = state;
}

void inputdevice_do_keyboard (int code, int state)
{
    if (code < 0x80) {
		uae_u8 key = code | (state ? 0x00 : 0x80);
		keybuf[key & 0x7f] = (key & 0x80) ? 0 : 1;
		if (key == AK_RESETWARNING) {
			resetwarning_do (0);
			return;
		} else if ((keybuf[AK_CTRL] || keybuf[AK_RCTRL]) && keybuf[AK_LAMI] && keybuf[AK_RAMI]) {
		    int r = keybuf[AK_LALT] | keybuf[AK_RALT];
			if (!r && currprefs.cs_resetwarning && resetwarning_do (1))
				return;
		    memset (keybuf, 0, sizeof (keybuf));
		    uae_reset (r);
		}
		if (record_key ((uae_u8)((key << 1) | (key >> 7)))) {
			if (inputdevice_logging & 1)
				write_log ("Amiga key %02X %d\n", key & 0x7f, key >> 7);
		}
		return;
    }
    inputdevice_add_inputcode (code, state);
}

void inputdevice_handle_inputcode (void)
{
	static int swapperslot;
    int code = inputcode_pending;
    int state = inputcode_pending_state;

    inputcode_pending = 0;
    if (code == 0)
		return;
    if (vpos != 0)
		write_log ("inputcode=%d but vpos = %d", code, vpos);

#ifdef ARCADIA
    switch (code)
    {
    case AKS_ARCADIADIAGNOSTICS:
		arcadia_flag &= ~1;
		arcadia_flag |= state ? 1 : 0;
		break;
    case AKS_ARCADIAPLY1:
		arcadia_flag &= ~4;
		arcadia_flag |= state ? 4 : 0;
		break;
    case AKS_ARCADIAPLY2:
		arcadia_flag &= ~2;
		arcadia_flag |= state ? 2 : 0;
		break;
    case AKS_ARCADIACOIN1:
		if (state)
	    arcadia_coin[0]++;
		break;
    case AKS_ARCADIACOIN2:
		if (state)
	    arcadia_coin[1]++;
		break;
    }
#endif

    if (!state)
		return;
    switch (code)
    {
    case AKS_ENTERGUI:
		gui_display (-1);
		break;
    case AKS_SCREENSHOT:
		screenshot (1);
		break;
#ifdef ACTION_REPLAY
    case AKS_FREEZEBUTTON:
		action_replay_freeze ();
		break;
#endif
    case AKS_FLOPPY0:
		gui_display (0);
		break;
    case AKS_FLOPPY1:
		gui_display (1);
		break;
    case AKS_FLOPPY2:
		gui_display (2);
		break;
    case AKS_FLOPPY3:
		gui_display (3);
		break;
    case AKS_EFLOPPY0:
		disk_eject (0);
		break;
    case AKS_EFLOPPY1:
		disk_eject (1);
		break;
    case AKS_EFLOPPY2:
		disk_eject (2);
		break;
    case AKS_EFLOPPY3:
		disk_eject (3);
		break;
    case AKS_IRQ7:
		NMI_delayed ();
		break;
    case AKS_PAUSE:
		if (uae_get_state () == UAE_STATE_PAUSED)
		    uae_resume ();
		else
		    uae_pause ();
		break;
    case AKS_WARP:
		warpmode (-1);
		break;
    case AKS_INHIBITSCREEN:
		toggle_inhibit_frame (IHF_SCROLLLOCK);
		break;
#ifdef SAVESTATE
    case AKS_STATEREWIND:
		savestate_dorewind(1);
#endif
		break;
    case AKS_VOLDOWN:
		audio_volume (-1);
		break;
    case AKS_VOLUP:
		audio_volume (1);
		break;
    case AKS_VOLMUTE:
		audio_volume (0);
		break;
    case AKS_QUIT:
		uae_quit ();
		break;
    case AKS_SOFTRESET:
		uae_reset (0);
		break;
    case AKS_HARDRESET:
		uae_reset (1);
		break;
#ifdef SAVESTATE
    case AKS_STATESAVEQUICK:
    case AKS_STATESAVEQUICK1:
    case AKS_STATESAVEQUICK2:
    case AKS_STATESAVEQUICK3:
    case AKS_STATESAVEQUICK4:
    case AKS_STATESAVEQUICK5:
    case AKS_STATESAVEQUICK6:
    case AKS_STATESAVEQUICK7:
    case AKS_STATESAVEQUICK8:
    case AKS_STATESAVEQUICK9:
		savestate_quick ((code - AKS_STATESAVEQUICK) / 2, 1);
		break;
    case AKS_STATERESTOREQUICK:
    case AKS_STATERESTOREQUICK1:
    case AKS_STATERESTOREQUICK2:
    case AKS_STATERESTOREQUICK3:
    case AKS_STATERESTOREQUICK4:
    case AKS_STATERESTOREQUICK5:
    case AKS_STATERESTOREQUICK6:
    case AKS_STATERESTOREQUICK7:
    case AKS_STATERESTOREQUICK8:
    case AKS_STATERESTOREQUICK9:
		savestate_quick ((code - AKS_STATERESTOREQUICK) / 2, 0);
		break;
#endif
    case AKS_TOGGLEFULLSCREEN:
		toggle_fullscreen ();
		break;
    case AKS_TOGGLEMOUSEGRAB:
		toggle_mousegrab ();
		break;
#ifdef DEBUGGER
    case AKS_ENTERDEBUGGER:
		activate_debugger ();
		break;
#endif
    case AKS_STATESAVEDIALOG:
		gui_display (5);
		break;
	case AKS_STATERESTOREDIALOG:
		gui_display (4);
		break;
	case AKS_DECREASEREFRESHRATE:
	case AKS_INCREASEREFRESHRATE:
		{
			int dir = code == AKS_INCREASEREFRESHRATE ? 5 : -5;
			if (currprefs.chipset_refreshrate == 0)
				currprefs.chipset_refreshrate = currprefs.ntscmode ? 60 : 50;
			changed_prefs.chipset_refreshrate = currprefs.chipset_refreshrate + dir;
			if (changed_prefs.chipset_refreshrate < 10)
				changed_prefs.chipset_refreshrate = 10;
			if (changed_prefs.chipset_refreshrate > 900)
				changed_prefs.chipset_refreshrate = 900;
		}
		break;
	case AKS_DISKSWAPPER_NEXT:
		swapperslot++;
		if (swapperslot >= MAX_SPARE_DRIVES || currprefs.dfxlist[swapperslot][0] == 0)
			swapperslot = 0;
		break;
	case AKS_DISKSWAPPER_PREV:
		swapperslot--;
		if (swapperslot < 0)
			swapperslot = MAX_SPARE_DRIVES - 1;
		while (swapperslot > 0) {
			if (currprefs.dfxlist[swapperslot][0])
				break;
			swapperslot--;
		}
		break;
	case AKS_DISKSWAPPER_INSERT0:
	case AKS_DISKSWAPPER_INSERT1:
	case AKS_DISKSWAPPER_INSERT2:
	case AKS_DISKSWAPPER_INSERT3:
		_tcscpy (changed_prefs.df[code - AKS_DISKSWAPPER_INSERT0], currprefs.dfxlist[swapperslot]);
		break;

		break;
	case AKS_INPUT_CONFIG_1:
	case AKS_INPUT_CONFIG_2:
	case AKS_INPUT_CONFIG_3:
	case AKS_INPUT_CONFIG_4:
		changed_prefs.input_selected_setting = currprefs.input_selected_setting = code - AKS_INPUT_CONFIG_1 + 1;
		inputdevice_updateconfig (&currprefs);
		break;
    }
}

int handle_custom_event (TCHAR *custom)
{
	TCHAR *p, *buf, *nextp;

	if (custom == NULL)
		return 0;
	p = buf = my_strdup (custom);
	while (p && *p) {
		TCHAR *p2;
		if (*p != '\"')
			break;
		p++;
		p2 = p;
		while (*p2 != '\"' && *p2 != 0)
			p2++;
		if (*p2 == '\"') {
			*p2++ = 0;
			nextp = p2 + 1;
			while (*nextp == ' ')
				nextp++;
		}
		cfgfile_parse_line (&changed_prefs, p, 0);
		p = nextp;
	}
	xfree(buf);
	return 0;
}

int handle_input_event (int nr, int state, int max, int autofire)
{
	struct inputevent *ie;
    int joy;

    if (nr <= 0)
	return 0;
    ie = &events[nr];
	if (inputdevice_logging & 1)
		write_log (L"'%s' %d %d\n", ie->name, state, max);
    if (autofire) {
	if (state)
	    queue_input_event (nr, state, max, currprefs.input_autofire_framecnt, 1);
	else
	    queue_input_event (nr, -1, 0, 0, 1);
    }
    switch (ie->unit)
    {
	case 5: /* lightpen/gun */
		{
			if (lightpen_x < 0 && lightpen_y < 0) {
				lightpen_x = gfxvidinfo.width / 2;
				lightpen_y = gfxvidinfo.height / 2;
			}
			if (ie->type == 0) {
				int delta = 0;
				if (max == 0)
					delta = state * currprefs.input_mouse_speed / 100;
				else if (state > 0)
					delta = currprefs.input_joymouse_speed;
				else if (state < 0)
					delta = -currprefs.input_joymouse_speed;
				if (ie->data)
					lightpen_y += delta;
				else
					lightpen_x += delta;
			} else {
				int delta = currprefs.input_joymouse_speed;
				if (ie->data & DIR_LEFT)
					lightpen_x -= delta;
				if (ie->data & DIR_RIGHT)
					lightpen_x += delta;
				if (ie->data & DIR_UP)
					lightpen_y -= delta;
				if (ie->data & DIR_DOWN)
					lightpen_y += delta;
			}
		}
	break;
	case 1: /* ->JOY1 */
	case 2: /* ->JOY2 */
	case 3: /* ->Parallel port joystick adapter port #1 */
	case 4: /* ->Parallel port joystick adapter port #2 */
	    joy = ie->unit - 1;
	    if (ie->type & 4) {

		if (state)
		    joybutton[joy] |= 1 << ie->data;
		else
		    joybutton[joy] &= ~(1 << ie->data);

	    } else if (ie->type & 8) {

		/* real mouse / analog stick mouse emulation */
		int delta;
		int deadzone = currprefs.input_joymouse_deadzone * max / 100;
			int unit = ie->data & 0x7f;

		if (max) {
				if (state <= deadzone && state >= -deadzone) {
			state = 0;
					mouse_deltanoreset[joy][unit] = 0;
		    } else if (state < 0) {
			state += deadzone;
					mouse_deltanoreset[joy][unit] = 1;
		    } else {
			state -= deadzone;
					mouse_deltanoreset[joy][unit] = 1;
		    }
		    max -= deadzone;
		    delta = state * currprefs.input_joymouse_multiplier / max;
		} else {
		    delta = state;
				mouse_deltanoreset[joy][unit] = 0;
		}
			if (ie->data & IE_CDTV) {
				delta = 0;
				if (state > 0)
					delta = JOYMOUSE_CDTV;
				else if (state < 0)
					delta = -JOYMOUSE_CDTV;
			}

			mouse_delta[joy][unit] += delta * ((ie->data & IE_INVERT) ? -1 : 1);

	    } else if (ie->type & 32) { /* button mouse emulation vertical */

			int speed = (ie->data & IE_CDTV) ? JOYMOUSE_CDTV : currprefs.input_joymouse_speed;

		if (state && (ie->data & DIR_UP)) {
		    mouse_delta[joy][1] = -speed;
		    mouse_deltanoreset[joy][1] = 1;
		} else if (state && (ie->data & DIR_DOWN)) {
		    mouse_delta[joy][1] = speed;
		    mouse_deltanoreset[joy][1] = 1;
		} else
		    mouse_deltanoreset[joy][1] = 0;

	    } else if (ie->type & 64) { /* button mouse emulation horizontal */

			int speed = (ie->data & IE_CDTV) ? JOYMOUSE_CDTV : currprefs.input_joymouse_speed;

		if (state && (ie->data & DIR_LEFT)) {
		    mouse_delta[joy][0] = -speed;
		    mouse_deltanoreset[joy][0] = 1;
		} else if (state && (ie->data & DIR_RIGHT)) {
		    mouse_delta[joy][0] = speed;
		    mouse_deltanoreset[joy][0] = 1;
		} else
		    mouse_deltanoreset[joy][0] = 0;

		} else if (ie->type & 128) { /* analog joystick / paddle */

		int deadzone = currprefs.input_joymouse_deadzone * max / 100;
			int unit = ie->data & 0x7f;
		if (max) {
				if (state <= deadzone && state >= -deadzone) {
			state = 0;
		    } else if (state < 0) {
			state += deadzone;
		    } else {
			state -= deadzone;
		    }
		    state = state * max / (max - deadzone);
		}
			if (ie->data & IE_INVERT)
				state = -state;
			state = state * currprefs.input_analog_joystick_mult / max;
			state += (128 * currprefs.input_analog_joystick_mult / 100) + currprefs.input_analog_joystick_offset;
			if (state < 0)
				state = 0;
			if (state > 255)
				state = 255;
			joydirpot[joy][unit] = state;

	    } else {

		int left = oleft[joy], right = oright[joy], top = otop[joy], bot = obot[joy];
		if (ie->type & 16) {
		    /* button to axis mapping */
				if (ie->data & DIR_LEFT)
					left = oleft[joy] = state ? 1 : 0;
				if (ie->data & DIR_RIGHT)
					right = oright[joy] = state ? 1 : 0;
				if (ie->data & DIR_UP)
					top = otop[joy] = state ? 1 : 0;
				if (ie->data & DIR_DOWN)
					bot = obot[joy] = state ? 1 : 0;
		} else {
		    /* "normal" joystick axis */
		    int deadzone = currprefs.input_joystick_deadzone * max / 100;
		    int neg, pos;
		    if (state < deadzone && state > -deadzone)
			state = 0;
		    neg = state < 0 ? 1 : 0;
		    pos = state > 0 ? 1 : 0;
				if (ie->data & DIR_LEFT)
					left = oleft[joy] = neg;
				if (ie->data & DIR_RIGHT)
					right = oright[joy] = pos;
				if (ie->data & DIR_UP)
					top = otop[joy] = neg;
				if (ie->data & DIR_DOWN)
					bot = obot[joy] = pos;
		}
		joydir[joy] = 0;
			if (left)
				joydir[joy] |= DIR_LEFT;
			if (right)
				joydir[joy] |= DIR_RIGHT;
			if (top)
				joydir[joy] |= DIR_UP;
			if (bot)
				joydir[joy] |= DIR_DOWN;

	    }
	break;
	case 0: /* ->KEY */
	    inputdevice_do_keyboard (ie->data, state);
	break;
    }
    return 1;
}

void inputdevice_vsync (void)
{
    struct input_queue_struct *iq;
    int i;

	if (inputdevice_logging & 32)
		write_log ("*\n");

    for (i = 0; i < INPUT_QUEUE_SIZE; i++) {
	iq = &input_queue[i];
	if (iq->framecnt > 0) {
	    iq->framecnt--;
	    if (iq->framecnt == 0) {
				if (iq->state)
					iq->state = 0;
				else
					iq->state = iq->storedstate;
		handle_input_event (iq->event, iq->state, iq->max, 0);
		iq->framecnt = iq->nextframecnt;
	    }
	}
    }

	input_frame++;
	mouseupdate (0, 1);
    idev[IDTYPE_MOUSE].read ();
	idev[IDTYPE_JOYSTICK].read ();

	inputdelay = rand () % (maxvpos <= 1 ? 1 : maxvpos - 1);
    inputdevice_handle_inputcode ();
#ifdef ARCADIA
	if (arcadia_bios)
	arcadia_vsync ();
#endif
}

void inputdevice_reset (void)
{
}

static int switchdevice (struct uae_input_device *id, int num, int button)
{
}

static void process_custom_event (struct uae_input_device *id, int offset, int state)
{
	int idx = -1;
	int custompos = (id->flags[offset][0] >> 15) & 1;
	TCHAR *custom;

	if (state < 0) {
		idx = 0;
		custompos = 0;
	} else {
		idx = state > 0 ? 0 : 1;
		if (custompos)
			idx += 2;
		if (state == 0)
			custompos ^= 1;
	}
	custom = id->custom[offset][idx];
	if (custom == NULL) {
		if (idx >= 2)
			custom = id->custom[offset][idx - 2];
	}
	handle_custom_event (custom);
	id->flags[offset][0] &= ~0x8000;
	id->flags[offset][0] |= custompos << 15;
}

static void setbuttonstateall (struct uae_input_device *id, struct uae_input_device2 *id2, int button, int state)
{
	int i;
    uae_u32 mask = 1 << button;
    uae_u32 omask = id2->buttonmask & mask;
    uae_u32 nmask = (state ? 1 : 0) << button;

	if (!id->enabled) {
		if (state)
			switchdevice (id, button, 1);
		return;
	}
    if (button >= ID_BUTTON_TOTAL)
	return;

    for (i = 0; i < MAX_INPUT_SUB_EVENT; i++) {
		int evt = evt = id->eventid[ID_BUTTON_OFFSET + button][sublevdir[state <= 0 ? 1 : 0][i]];
		int autofire = (id->flags[ID_BUTTON_OFFSET + button][sublevdir[state <= 0 ? 1 : 0][i]] & ID_FLAG_AUTOFIRE) ? 1 : 0;
		int toggle = (id->flags[ID_BUTTON_OFFSET + button][sublevdir[state <= 0 ? 1 : 0][i]] & ID_FLAG_TOGGLE) ? 1 : 0;

	if (state < 0) {
			handle_input_event (evt, 1, 1, 0);
			queue_input_event (evt, 0, 1, 1, 0); /* send release event next frame */
			if (i == 0)
				process_custom_event (id, ID_BUTTON_OFFSET + button, state);
		} else if (toggle) {
			int toggled;
			if (!state)
				continue;
			if (omask & mask)
				continue;
			id->flags[ID_BUTTON_OFFSET + button][sublevdir[state <= 0 ? 1 : 0][i]] ^= ID_FLAG_TOGGLED;
			toggled = (id->flags[ID_BUTTON_OFFSET + button][sublevdir[state <= 0 ? 1 : 0][i]] & ID_FLAG_TOGGLED) ? 1 : 0;
			handle_input_event (evt, toggled, 1, autofire);
			if (i == 0)
				process_custom_event (id, ID_BUTTON_OFFSET + button, toggled);
	} else {
			if ((omask ^ nmask) & mask) {
				handle_input_event (evt, state, 1, autofire);
				if (i == 0)
					process_custom_event (id, ID_BUTTON_OFFSET + button, state);
	}
    }
	}

    if ((omask ^ nmask) & mask) {
	if (state)
	    id2->buttonmask |= mask;
	else
	    id2->buttonmask &= ~mask;
    }
}


/* - detect required number of joysticks and mice from configuration data
 * - detect if CD32 pad emulation is needed
 * - detect device type in ports (mouse or joystick)
 */

static int iscd32 (int ei)
{
    if (ei >= INPUTEVENT_JOY1_CD32_FIRST && ei <= INPUTEVENT_JOY1_CD32_LAST) {
	cd32_pad_enabled[0] = 1;
	return 1;
    }
    if (ei >= INPUTEVENT_JOY2_CD32_FIRST && ei <= INPUTEVENT_JOY2_CD32_LAST) {
	cd32_pad_enabled[1] = 1;
	return 2;
    }
    return 0;
}

static int isparport (int ei)
{
    if (ei > INPUTEVENT_PAR_JOY1_START && ei < INPUTEVENT_PAR_JOY_END) {
	parport_joystick_enabled = 1;
	return 1;
    }
    return 0;
}

static int ismouse (int ei)
{
    if (ei >= INPUTEVENT_MOUSE1_FIRST && ei <= INPUTEVENT_MOUSE1_LAST) {
	mouse_port[0] = 1;
	return 1;
    }
    if (ei >= INPUTEVENT_MOUSE2_FIRST && ei <= INPUTEVENT_MOUSE2_LAST) {
	mouse_port[1] = 1;
	return 2;
    }
    return 0;
}

static int isanalog (int ei)
{
	if (ei == INPUTEVENT_JOY1_HORIZ_POT || ei == INPUTEVENT_JOY1_HORIZ_POT_INV) {
		analog_port[0][0] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY1_VERT_POT || ei == INPUTEVENT_JOY1_VERT_POT_INV) {
		analog_port[0][1] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY2_HORIZ_POT || ei == INPUTEVENT_JOY2_HORIZ_POT_INV) {
		analog_port[1][0] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY2_VERT_POT || ei == INPUTEVENT_JOY2_VERT_POT_INV) {
		analog_port[1][1] = 1;
		return 1;
	}
	return 0;
}

static int isdigitalbutton (int ei)
{
	if (ei == INPUTEVENT_JOY1_2ND_BUTTON) {
		digital_port[0][0] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY1_3RD_BUTTON) {
		digital_port[0][1] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY2_2ND_BUTTON) {
		digital_port[1][0] = 1;
		return 1;
	}
	if (ei == INPUTEVENT_JOY2_3RD_BUTTON) {
		digital_port[1][1] = 1;
		return 1;
	}
	return 0;
}

static void scanevents (struct uae_prefs *p)
{
	int i, j, k, ei;
    const struct inputevent *e;
	int n_joy = idev[IDTYPE_JOYSTICK].get_num();
	int n_mouse = idev[IDTYPE_MOUSE].get_num();

    cd32_pad_enabled[0] = cd32_pad_enabled[1] = 0;
    parport_joystick_enabled = 0;
    mouse_port[0] = mouse_port[1] = 0;

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			digital_port[i][j] = 0;
			analog_port[i][j] = 0;
			joydirpot[i][j] = 128 / (312 * 100 / currprefs.input_analog_joystick_mult) + (128 * currprefs.input_analog_joystick_mult / 100) + currprefs.input_analog_joystick_offset;
		}
	}

    for (i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++)
	joydir[i] = 0;

    for (i = 0; i < MAX_INPUT_DEVICES; i++) {
	use_joysticks[i] = 0;
	use_mice[i] = 0;
	for (k = 0; k < MAX_INPUT_SUB_EVENT; k++) {
	    for (j = 0; j < ID_BUTTON_TOTAL; j++) {

				if ((joysticks[i].enabled && i < n_joy) || joysticks[i].enabled < 0) {
		    ei = joysticks[i].eventid[ID_BUTTON_OFFSET + j][k];
		    e = &events[ei];
		    iscd32 (ei);
		    isparport (ei);
		    ismouse (ei);
					isdigitalbutton (ei);
		    if (joysticks[i].eventid[ID_BUTTON_OFFSET + j][k] > 0)
			use_joysticks[i] = 1;
		}
				if ((mice[i].enabled && i < n_mouse) || mice[i].enabled < 0) {
		    ei = mice[i].eventid[ID_BUTTON_OFFSET + j][k];
		    e = &events[ei];
		    iscd32 (ei);
		    isparport (ei);
		    ismouse (ei);
					isdigitalbutton (ei);
		    if (mice[i].eventid[ID_BUTTON_OFFSET + j][k] > 0)
			use_mice[i] = 1;
		}

	    }

	    for (j = 0; j < ID_AXIS_TOTAL; j++) {

				if ((joysticks[i].enabled && i < n_joy) || joysticks[i].enabled < 0) {
		    ei = joysticks[i].eventid[ID_AXIS_OFFSET + j][k];
		    iscd32 (ei);
		    isparport (ei);
		    ismouse (ei);
					isanalog (ei);
					isdigitalbutton (ei);
		    if (ei > 0)
			use_joysticks[i] = 1;
		}
				if ((mice[i].enabled && i < n_mouse) || mice[i].enabled < 0) {
		    ei = mice[i].eventid[ID_AXIS_OFFSET + j][k];
		    iscd32 (ei);
		    isparport (ei);
		    ismouse (ei);
					isanalog (ei);
					isdigitalbutton (ei);
		    if (ei > 0)
			use_mice[i] = 1;
		}
	    }
	}
    }
    for (i = 0; i < MAX_INPUT_DEVICES; i++) {
	use_keyboards[i] = 0;
	if (keyboards[i].enabled && i < idev[IDTYPE_KEYBOARD].get_num()) {
	    j = 0;
	    while (keyboards[i].extra[j][0] >= 0) {
		use_keyboards[i] = 1;
		for (k = 0; k < MAX_INPUT_SUB_EVENT; k++) {
		    ei = keyboards[i].eventid[j][k];
		    iscd32 (ei);
		    isparport (ei);
		    ismouse (ei);
					isdigitalbutton (ei);
		}
		j++;
	    }
	}
    }
}

#ifdef CD32
static void setcd32 (int joy, int n)
{
    joysticks[joy].eventid[ID_BUTTON_OFFSET + 0][0] = n ? INPUTEVENT_JOY2_CD32_RED : INPUTEVENT_JOY1_CD32_RED;
    joysticks[joy].eventid[ID_BUTTON_OFFSET + 1][0] = n ? INPUTEVENT_JOY2_CD32_BLUE : INPUTEVENT_JOY1_CD32_BLUE;
    joysticks[joy].eventid[ID_BUTTON_OFFSET + 2][0] = n ? INPUTEVENT_JOY2_CD32_YELLOW : INPUTEVENT_JOY1_CD32_YELLOW;
    joysticks[joy].eventid[ID_BUTTON_OFFSET + 3][0] = n ? INPUTEVENT_JOY2_CD32_GREEN : INPUTEVENT_JOY1_CD32_GREEN;
    joysticks[joy].eventid[ID_BUTTON_OFFSET + 4][0] = n ? INPUTEVENT_JOY2_CD32_FFW : INPUTEVENT_JOY1_CD32_FFW;
    joysticks[joy].eventid[ID_BUTTON_OFFSET + 5][0] = n ? INPUTEVENT_JOY2_CD32_RWD : INPUTEVENT_JOY1_CD32_RWD;
    joysticks[joy].eventid[ID_BUTTON_OFFSET + 6][0] = n ? INPUTEVENT_JOY2_CD32_PLAY :  INPUTEVENT_JOY1_CD32_PLAY;
}
#endif

int compatibility_device[2];

static void compatibility_mode (struct uae_prefs *prefs)
{
    int joy, i;
    int used[4] = { 0, 0, 0, 0};

    compatibility_device[0] = -1;
    compatibility_device[1] = -1;
	compatibility_device[2] = -1;
	compatibility_device[3] = -1;
    for (i = 0; i < MAX_INPUT_DEVICES; i++) {
	memset (&mice[i], 0, sizeof (*mice));
	memset (&joysticks[i], 0, sizeof (*joysticks));
    }

    if ((joy = jsem_ismouse (0, prefs)) >= 0) {
	mice[joy].eventid[ID_AXIS_OFFSET + 0][0] = INPUTEVENT_MOUSE1_HORIZ;
	mice[joy].eventid[ID_AXIS_OFFSET + 1][0] = INPUTEVENT_MOUSE1_VERT;
	mice[joy].eventid[ID_AXIS_OFFSET + 2][0] = INPUTEVENT_MOUSE1_WHEEL;
	mice[joy].eventid[ID_BUTTON_OFFSET + 0][0] = INPUTEVENT_JOY1_FIRE_BUTTON;
	mice[joy].eventid[ID_BUTTON_OFFSET + 1][0] = INPUTEVENT_JOY1_2ND_BUTTON;
	mice[joy].eventid[ID_BUTTON_OFFSET + 2][0] = INPUTEVENT_JOY1_3RD_BUTTON;
	mice[joy].eventid[ID_BUTTON_OFFSET + 3][0] = INPUTEVENT_KEY_ALT_LEFT;
	mice[joy].eventid[ID_BUTTON_OFFSET + 3][1] = INPUTEVENT_KEY_CURSOR_LEFT;
	mice[joy].eventid[ID_BUTTON_OFFSET + 4][0] = INPUTEVENT_KEY_ALT_LEFT;
	mice[joy].eventid[ID_BUTTON_OFFSET + 4][1] = INPUTEVENT_KEY_CURSOR_RIGHT;
	mice[joy].enabled = 1;
    }
    if ((joy = jsem_ismouse (1, prefs)) >= 0) {
	mice[joy].eventid[ID_AXIS_OFFSET + 0][0] = INPUTEVENT_MOUSE2_HORIZ;
	mice[joy].eventid[ID_AXIS_OFFSET + 1][0] = INPUTEVENT_MOUSE2_VERT;
	mice[joy].eventid[ID_BUTTON_OFFSET + 0][0] = INPUTEVENT_JOY2_FIRE_BUTTON;
	mice[joy].eventid[ID_BUTTON_OFFSET + 1][0] = INPUTEVENT_JOY2_2ND_BUTTON;
	mice[joy].eventid[ID_BUTTON_OFFSET + 2][0] = INPUTEVENT_JOY2_3RD_BUTTON;
	mice[joy].enabled = 1;
    }

    joy = jsem_isjoy (1, prefs);
    if (joy >= 0) {
	joysticks[joy].eventid[ID_AXIS_OFFSET + 0][0] = INPUTEVENT_JOY2_HORIZ;
	joysticks[joy].eventid[ID_AXIS_OFFSET + 1][0] = INPUTEVENT_JOY2_VERT;
	used[joy] = 1;
	joysticks[joy].eventid[ID_BUTTON_OFFSET + 0][0] = INPUTEVENT_JOY2_FIRE_BUTTON;
	joysticks[joy].eventid[ID_BUTTON_OFFSET + 1][0] = INPUTEVENT_JOY2_2ND_BUTTON;
	joysticks[joy].eventid[ID_BUTTON_OFFSET + 2][0] = INPUTEVENT_JOY2_3RD_BUTTON;
#ifdef CD32
	if (cd32_enabled)
	    setcd32 (joy, 1);
#endif
	joysticks[joy].enabled = 1;
    }

    joy = jsem_isjoy (0, prefs);
    if (joy >= 0) {
	used[joy] = 1;
	joysticks[joy].eventid[ID_AXIS_OFFSET + 0][0] = INPUTEVENT_JOY1_HORIZ;
	joysticks[joy].eventid[ID_AXIS_OFFSET + 1][0] = INPUTEVENT_JOY1_VERT;
	joysticks[joy].eventid[ID_BUTTON_OFFSET + 0][0] = INPUTEVENT_JOY1_FIRE_BUTTON;
	joysticks[joy].eventid[ID_BUTTON_OFFSET + 1][0] = INPUTEVENT_JOY1_2ND_BUTTON;
	joysticks[joy].eventid[ID_BUTTON_OFFSET + 2][0] = INPUTEVENT_JOY1_3RD_BUTTON;
	joysticks[joy].enabled = 1;
    }

    for (joy = 0; used[joy]; joy++);
    if (JSEM_ISANYKBD (0, prefs)) {
	joysticks[joy].eventid[ID_AXIS_OFFSET +  0][0] = INPUTEVENT_JOY1_HORIZ;
	joysticks[joy].eventid[ID_AXIS_OFFSET +  1][0] = INPUTEVENT_JOY1_VERT;
	joysticks[joy].eventid[ID_BUTTON_OFFSET + 0][0] = INPUTEVENT_JOY1_FIRE_BUTTON;
	joysticks[joy].eventid[ID_BUTTON_OFFSET + 1][0] = INPUTEVENT_JOY1_2ND_BUTTON;
	joysticks[joy].eventid[ID_BUTTON_OFFSET + 2][0] = INPUTEVENT_JOY1_3RD_BUTTON;
	joysticks[joy].enabled = 1;
	used[joy] = 1;
	compatibility_device[0] = joy;
    }
    for (joy = 0; used[joy]; joy++);
    if (JSEM_ISANYKBD (1, prefs)) {
	joysticks[joy].eventid[ID_AXIS_OFFSET +  0][0] = INPUTEVENT_JOY2_HORIZ;
	joysticks[joy].eventid[ID_AXIS_OFFSET +  1][0] = INPUTEVENT_JOY2_VERT;
	joysticks[joy].eventid[ID_BUTTON_OFFSET + 0][0] = INPUTEVENT_JOY2_FIRE_BUTTON;
	joysticks[joy].eventid[ID_BUTTON_OFFSET + 1][0] = INPUTEVENT_JOY2_2ND_BUTTON;
	joysticks[joy].eventid[ID_BUTTON_OFFSET + 2][0] = INPUTEVENT_JOY2_3RD_BUTTON;
#ifdef CD32
	if (cd32_enabled)
	    setcd32 (joy, 1);
#endif
	joysticks[joy].enabled = 1;
	used[joy] = 1;
	compatibility_device[1] = joy;
    }
}

void inputdevice_updateconfig (struct uae_prefs *prefs)
{
    int i;

    if (currprefs.jport0 != changed_prefs.jport0
	|| currprefs.jport1 != changed_prefs.jport1) {
	currprefs.jport0 = changed_prefs.jport0;
	currprefs.jport1 = changed_prefs.jport1;
    }
    joybutton[0] = joybutton[1] = 0;
    joydir[0] = joydir[1] = 0;
    oldmx[0] = oldmx[1] = -1;
    oldmy[0] = oldmy[1] = -1;
    cd32_shifter[0] = cd32_shifter[1] = 8;
    oleft[0] = oleft[1] = 0;
    oright[0] = oright[1] = 0;
    otop[0] = otop[1] = 0;
    obot[0] = obot[1] = 0;
    for (i = 0; i < 2; i++) {
	mouse_deltanoreset[i][0] = 0;
	mouse_delta[i][0] = 0;
	mouse_deltanoreset[i][1] = 0;
	mouse_delta[i][1] = 0;
	mouse_deltanoreset[i][2] = 0;
	mouse_delta[i][2] = 0;
    }
    memset (keybuf, 0, sizeof (keybuf));

    for (i = 0; i < INPUT_QUEUE_SIZE; i++)
	input_queue[i].framecnt = input_queue[i].nextframecnt = -1;

    for (i = 0; i < MAX_INPUT_SUB_EVENT; i++) {
	sublevdir[0][i] = i;
	sublevdir[1][i] = MAX_INPUT_SUB_EVENT - i - 1;
    }

    joysticks = prefs->joystick_settings[prefs->input_selected_setting];
    mice = prefs->mouse_settings[prefs->input_selected_setting];
    keyboards = prefs->keyboard_settings[prefs->input_selected_setting];

    memset (joysticks2, 0, sizeof (joysticks2));
    memset (mice2, 0, sizeof (mice2));
    if (prefs->input_selected_setting == 0)
	compatibility_mode (prefs);

    joystick_setting_changed ();

    scanevents (prefs);

#if 0
#ifdef CD32
	if (currprefs.input_selected_setting == 0 && currprefs.cs_cd32cd)
	cd32_pad_enabled[1] = 1;
#endif
#endif
}

static void set_kbr_default (struct uae_prefs *p, int index, int num)
{
	int i, j, k, l;
    struct uae_input_device_kbr_default *trans = keyboard_default;
    struct uae_input_device *kbr;
    struct inputdevice_functions *id = &idev[IDTYPE_KEYBOARD];
    uae_u32 scancode;

    if (!trans)
	return;
    for (j = 0; j < MAX_INPUT_DEVICES; j++) {
	kbr = &p->keyboard_settings[index][j];
	for (i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
	    memset (kbr, 0, sizeof (struct uae_input_device));
	    kbr->extra[i][0] = -1;
	}
	if (j < id->get_num ()) {
	    if (j == 0)
		kbr->enabled = 1;
	    for (i = 0; i < id->get_widget_num (num); i++) {
		id->get_widget_type (num, i, 0, &scancode);
		kbr->extra[i][0] = scancode;
		l = 0;
		while (trans[l].scancode >= 0) {
		    if (kbr->extra[i][0] == trans[l].scancode) {
			for (k = 0; k < MAX_INPUT_SUB_EVENT; k++) {
			    if (kbr->eventid[i][k] == 0) break;
			}
			if (k == MAX_INPUT_SUB_EVENT) {
			    write_log ("corrupt default keyboard mappings\n");
			    return;
			}
			kbr->eventid[i][k] = trans[l].event;
			break;
		    }
		    l++;
		}
	    }
	}
    }
}

void inputdevice_default_prefs (struct uae_prefs *p)
{
	int i, j;

    inputdevice_init ();
    p->input_joymouse_multiplier = 20;
    p->input_joymouse_deadzone = 33;
    p->input_joystick_deadzone = 33;
    p->input_joymouse_speed = 10;
	p->input_analog_joystick_mult = 15;
	p->input_analog_joystick_offset = -1;
    p->input_mouse_speed = 100;
    p->input_autofire_framecnt = 10;
    for (i = 0; i <= MAX_INPUT_SETTINGS; i++) {
	set_kbr_default (p, i, 0);
	input_get_default_mouse (p->mouse_settings[i]);
	input_get_default_joystick (p->joystick_settings[i]);
    }
}

void inputdevice_setkeytranslation (struct uae_input_device_kbr_default *trans)
{
    keyboard_default = trans;
}

static int inputdevice_translatekeycode_2 (int keyboard, int scancode, int state)
{
    struct uae_input_device *na = &keyboards[keyboard];
    int j, k;
	int handled = 0;

    if (!keyboards || scancode < 0)
		return handled;
    j = 0;
    while (na->extra[j][0] >= 0) {
	if (na->extra[j][0] == scancode) {
	    for (k = 0; k < MAX_INPUT_SUB_EVENT; k++) {/* send key release events in reverse order */
		int autofire = (na->flags[j][sublevdir[state == 0 ? 1 : 0][k]] & ID_FLAG_AUTOFIRE) ? 1 : 0;
				int toggle = (na->flags[j][sublevdir[state == 0 ? 1 : 0][k]] & ID_FLAG_TOGGLE) ? 1 : 0;
				int evt = na->eventid[j][sublevdir[state == 0 ? 1 : 0][k]];
				int toggled;

				if (toggle) {
					if (!state)
						continue;
					na->flags[j][sublevdir[state == 0 ? 1 : 0][k]] ^= ID_FLAG_TOGGLED;
					toggled = (na->flags[j][sublevdir[state == 0 ? 1 : 0][k]] & ID_FLAG_TOGGLED) ? 1 : 0;
					handled |= handle_input_event (evt, toggled, 1, autofire);
				} else {
					handled |= handle_input_event (evt, state, 1, autofire);
	    }
			}
			process_custom_event (na, j, state);
			return handled;
	}
	j++;
    }
	return handled;
}

#define IECODE_UP_PREFIX 0x80
#define RAW_STEALTH 0x68
#define STEALTHF_E0KEY 0x08
#define STEALTHF_UPSTROKE 0x04
#define STEALTHF_SPECIAL 0x02
#define STEALTHF_E1KEY 0x01

static void sendmmcodes(int code, int newstate)
{
	uae_u8 b;

	b = RAW_STEALTH | IECODE_UP_PREFIX;
	record_key(((b << 1) | (b >> 7)) & 0xff);
	b = IECODE_UP_PREFIX;
	if ((code >> 8) == 0x01)
		b |= STEALTHF_E0KEY;
	if ((code >> 8) == 0x02)
		b |= STEALTHF_E1KEY;
	if (!newstate)
		b |= STEALTHF_UPSTROKE;
	record_key(((b << 1) | (b >> 7)) & 0xff);
	b = ((code >> 4) & 0x0f) | IECODE_UP_PREFIX;
	record_key(((b << 1) | (b >> 7)) & 0xff);
	b = (code & 0x0f) | IECODE_UP_PREFIX;
	record_key(((b << 1) | (b >> 7)) & 0xff);
}

int inputdevice_translatekeycode (int keyboard, int scancode, int state)
{
	if (inputdevice_translatekeycode_2 (keyboard, scancode, state))
		return 1;
	if (currprefs.mmkeyboard)
		sendmmcodes (scancode, state);
    return 0;
}

void inputdevice_init (void)
{
    idev[IDTYPE_JOYSTICK] = inputdevicefunc_joystick;
    idev[IDTYPE_JOYSTICK].init ();
    idev[IDTYPE_MOUSE] = inputdevicefunc_mouse;
    idev[IDTYPE_MOUSE].init ();
    idev[IDTYPE_KEYBOARD] = inputdevicefunc_keyboard;
    idev[IDTYPE_KEYBOARD].init ();
}

void inputdevice_close (void)
{
    idev[IDTYPE_JOYSTICK].close ();
    idev[IDTYPE_MOUSE].close ();
    idev[IDTYPE_KEYBOARD].close ();
}

static struct uae_input_device *get_uid (const struct inputdevice_functions *id, int devnum)
{
    struct uae_input_device *uid = 0;
    if (id == &idev[IDTYPE_JOYSTICK]) {
	uid = &joysticks[devnum];
    } else if (id == &idev[IDTYPE_MOUSE]) {
	uid = &mice[devnum];
    } else if (id == &idev[IDTYPE_KEYBOARD]) {
	uid = &keyboards[devnum];
    }
    return uid;
}

static int get_event_data (const struct inputdevice_functions *id, int devnum, int num, int *eventid, TCHAR **custom, int *flags, int sub)
{
    const struct uae_input_device *uid = get_uid (id, devnum);
    int type = id->get_widget_type (devnum, num, 0, 0);
    int i;
    if (type == IDEV_WIDGET_BUTTON) {
	i = num - id->get_widget_first (devnum, type) + ID_BUTTON_OFFSET;
	*eventid = uid->eventid[i][sub];
	*flags = uid->flags[i][sub];
	*custom = uid->custom[i][sub];
	return i;
    } else if (type == IDEV_WIDGET_AXIS) {
	i = num - id->get_widget_first (devnum, type) + ID_AXIS_OFFSET;
	*eventid = uid->eventid[i][sub];
	*flags = uid->flags[i][sub];
	*custom = uid->custom[i][sub];
	return i;
    } else if (type == IDEV_WIDGET_KEY) {
	i = num - id->get_widget_first (devnum, type);
	*eventid = uid->eventid[i][sub];
	*flags = uid->flags[i][sub];
	*custom = uid->custom[i][sub];
	return i;
    }
    return -1;
}

static int put_event_data (const struct inputdevice_functions *id, int devnum, int num, int eventid, TCHAR *custom, int flags, int sub)
{
    struct uae_input_device *uid = get_uid (id, devnum);
    int type = id->get_widget_type (devnum, num, 0, 0);
    int i;
    if (type == IDEV_WIDGET_BUTTON) {
	i = num - id->get_widget_first (devnum, type) + ID_BUTTON_OFFSET;
	uid->eventid[i][sub] = eventid;
	uid->flags[i][sub] = flags;
	free (uid->custom[i][sub]);
		uid->custom[i][sub] = custom && _tcslen (custom) > 0 ? my_strdup (custom) : NULL;
	return i;
    } else if (type == IDEV_WIDGET_AXIS) {
	i = num - id->get_widget_first (devnum, type) + ID_AXIS_OFFSET;
	uid->eventid[i][sub] = eventid;
	uid->flags[i][sub] = flags;
	free (uid->custom[i][sub]);
		uid->custom[i][sub] = custom && _tcslen (custom) > 0 ? my_strdup (custom) : NULL;
	return i;
    } else if (type == IDEV_WIDGET_KEY) {
	i = num - id->get_widget_first (devnum, type);
	uid->eventid[i][sub] = eventid;
	uid->flags[i][sub] = flags;
	free (uid->custom[i][sub]);
		uid->custom[i][sub] = custom && _tcslen (custom) > 0 ? my_strdup (custom) : NULL;
	return i;
    }
    return -1;
}

static int is_event_used (const struct inputdevice_functions *id, int devnum, int isnum, int isevent)
{
	struct uae_input_device *uid = get_uid (id, devnum);
	int num, evt, flag, sub;
	TCHAR *custom;

    for (num = 0; num < id->get_widget_num (devnum); num++) {
	for (sub = 0; sub < MAX_INPUT_SUB_EVENT; sub++) {
			if (get_event_data (id, devnum, num, &evt, &custom, &flag, sub) >= 0) {
				if (evt == isevent && isnum != num)
		    return 1;
	    }
	}
    }
    return 0;
}

int inputdevice_get_device_index (int devnum)
{
    if (devnum < idev[IDTYPE_JOYSTICK].get_num())
	return devnum;
    else if (devnum < idev[IDTYPE_JOYSTICK].get_num() + idev[IDTYPE_MOUSE].get_num())
	return devnum - idev[IDTYPE_JOYSTICK].get_num();
    else if (devnum < idev[IDTYPE_JOYSTICK].get_num() + idev[IDTYPE_MOUSE].get_num() + idev[IDTYPE_KEYBOARD].get_num())
	return devnum - idev[IDTYPE_JOYSTICK].get_num() - idev[IDTYPE_MOUSE].get_num();
    else
	return -1;
}

static int gettype (int devnum)
{
    if (devnum < idev[IDTYPE_JOYSTICK].get_num())
	return IDTYPE_JOYSTICK;
    else if (devnum < idev[IDTYPE_JOYSTICK].get_num() + idev[IDTYPE_MOUSE].get_num())
	return IDTYPE_MOUSE;
    else if (devnum < idev[IDTYPE_JOYSTICK].get_num() + idev[IDTYPE_MOUSE].get_num() + idev[IDTYPE_KEYBOARD].get_num())
	return IDTYPE_KEYBOARD;
    else
	return -1;
}

static struct inputdevice_functions *getidf (int devnum)
{
	int type = gettype (devnum);
	if (type < 0)
		return NULL;
	return &idev[type];
}


/* returns number of devices of type "type" */
int inputdevice_get_device_total (int type)
{
    return idev[type].get_num ();
}
/* returns the name of device */
TCHAR *inputdevice_get_device_name (int type, int devnum)
{
	return idev[type].get_friendlyname (devnum);
}
/* returns machine readable name of device */
TCHAR *inputdevice_get_device_unique_name (int type, int devnum)
{
	return idev[type].get_uniquename (devnum);
}
/* returns state (enabled/disabled) */
int inputdevice_get_device_status (int devnum)
{
    const struct inputdevice_functions *idf = getidf (devnum);
    struct uae_input_device *uid = get_uid (idf, inputdevice_get_device_index (devnum));
    return uid->enabled;
}

/* set state (enabled/disabled) */
void inputdevice_set_device_status (int devnum, int enabled)
{
    const struct inputdevice_functions *idf = getidf (devnum);
    struct uae_input_device *uid = get_uid (idf, inputdevice_get_device_index (devnum));
    uid->enabled = enabled;
}

/* returns number of axis/buttons and keys from selected device */
int inputdevice_get_widget_num (int devnum)
{
    const struct inputdevice_functions *idf = getidf (devnum);
    return idf->get_widget_num (inputdevice_get_device_index (devnum));
}

static void get_ename (const struct inputevent *ie, TCHAR *out)
{
    if (!out)
	return;
    if (ie->allow_mask == AM_K)
		_stprintf (out, L"%s (0x%02X)", ie->name, ie->data);
    else
		_tcscpy (out, ie->name);
}

int inputdevice_iterate (int devnum, int num, TCHAR *name, int *af)
{
    const struct inputdevice_functions *idf = getidf (devnum);
    static int id_iterator;
	struct inputevent *ie;
    int mask, data, flags, type;
    int devindex = inputdevice_get_device_index (devnum);
	TCHAR *custom;

    *af = 0;
    *name = 0;
    for (;;) {
	ie = &events[++id_iterator];
	if (!ie->confname) {
	    id_iterator = 0;
	    return 0;
	}
	mask = 0;
	type = idf->get_widget_type (devindex, num, 0, 0);
	if (type == IDEV_WIDGET_BUTTON) {
	    if (idf == &idev[IDTYPE_JOYSTICK]) {
		mask |= AM_JOY_BUT;
	    } else {
		mask |= AM_MOUSE_BUT;
	    }
	} else if (type == IDEV_WIDGET_AXIS) {
	    if (idf == &idev[IDTYPE_JOYSTICK]) {
		mask |= AM_JOY_AXIS;
	    } else {
		mask |= AM_MOUSE_AXIS;
	    }
	} else if (type == IDEV_WIDGET_KEY) {
	    mask |= AM_K;
	}
	if (ie->allow_mask & AM_INFO) {
			struct inputevent *ie2 = ie + 1;
	    while (!(ie2->allow_mask & AM_INFO)) {
		if (is_event_used (idf, devindex, ie2 - ie, -1)) {
		    ie2++;
		    continue;
		}
				if (ie2->allow_mask & mask)
					break;
		ie2++;
	    }
	    if (!(ie2->allow_mask & AM_INFO))
		mask |= AM_INFO;
	}
	if (!(ie->allow_mask & mask))
	    continue;
	get_event_data (idf, devindex, num, &data, &custom, &flags, 0);
	get_ename (ie, name);
	*af = (flags & ID_FLAG_AUTOFIRE) ? 1 : 0;
	return 1;
    }
}

int inputdevice_get_mapped_name (int devnum, int num, int *pflags, TCHAR *name, TCHAR *custom, int sub)
{
    const struct inputdevice_functions *idf = getidf (devnum);
    const struct uae_input_device *uid = get_uid (idf, inputdevice_get_device_index (devnum));
    int flags = 0, flag, data;
    int devindex = inputdevice_get_device_index (devnum);
	TCHAR *customp = NULL;

    if (name)
		_tcscpy (name, L"<none>");
    if (custom)
	custom[0] = 0;
    if (pflags)
	*pflags = 0;
    if (uid == 0 || num < 0)
	return 0;
    if (get_event_data (idf, devindex, num, &data, &customp, &flag, sub) < 0)
	return 0;
    if (customp && custom)
		_tcscpy (custom, customp);
    if (flag & ID_FLAG_AUTOFIRE)
	flags |= IDEV_MAPPED_AUTOFIRE_SET;
	if (flag & ID_FLAG_TOGGLE)
		flags |= IDEV_MAPPED_TOGGLE;
    if (!data)
	return 0;
    if (events[data].allow_mask & AM_AF)
	flags |= IDEV_MAPPED_AUTOFIRE_POSSIBLE;
    if (pflags)
	*pflags = flags;
    get_ename (&events[data], name);
    return data;
}

int inputdevice_set_mapping (int devnum, int num, TCHAR *name, TCHAR *custom, int flags, int sub)
{
    const struct inputdevice_functions *idf = getidf (devnum);
    const struct uae_input_device *uid = get_uid (idf, inputdevice_get_device_index (devnum));
    int eid, data, flag, amask;
	TCHAR ename[256];
    int devindex = inputdevice_get_device_index (devnum);
	TCHAR *customp = NULL;

    if (uid == 0 || num < 0)
	return 0;
    if (name) {
	eid = 1;
	while (events[eid].name) {
	    get_ename (&events[eid], ename);
			if (!_tcscmp(ename, name)) break;
	    eid++;
	}
	if (!events[eid].name)
	    return 0;
	if (events[eid].allow_mask & AM_INFO)
	    return 0;
    } else {
	eid = 0;
    }
	if (get_event_data (idf, devindex, num, &data, &customp, &flag, sub) < 0)
	return 0;
    if (data >= 0) {
	amask = events[eid].allow_mask;
		flag &= ~(ID_FLAG_AUTOFIRE | ID_FLAG_TOGGLE);
		if (amask & AM_AF) {
			flag |= (flags & IDEV_MAPPED_AUTOFIRE_SET) ? ID_FLAG_AUTOFIRE : 0;
			flag |= (flags & IDEV_MAPPED_TOGGLE) ? ID_FLAG_TOGGLE : 0;
		}
	put_event_data (idf, devindex, num, eid, custom, flag, sub);
	return 1;
    }
    return 0;
}

int inputdevice_get_widget_type (int devnum, int num, TCHAR *name)
{
    const struct inputdevice_functions *idf = getidf (devnum);
    return idf->get_widget_type (inputdevice_get_device_index (devnum), num, name, 0);
}

static int config_change;

void inputdevice_config_change (void)
{
    config_change = 1;
}

int inputdevice_config_change_test (void)
{
    int v = config_change;
    config_change = 0;
    return v;
}

void inputdevice_copyconfig (const struct uae_prefs *src, struct uae_prefs *dst)
{
    int i, j;

    dst->input_selected_setting = src->input_selected_setting;
    dst->input_joymouse_multiplier = src->input_joymouse_multiplier;
    dst->input_joymouse_deadzone = src->input_joymouse_deadzone;
    dst->input_joystick_deadzone = src->input_joystick_deadzone;
    dst->input_joymouse_speed = src->input_joymouse_speed;
    dst->input_mouse_speed = src->input_mouse_speed;
    dst->input_autofire_framecnt = src->input_autofire_framecnt;
    dst->jport0 = src->jport0;
    dst->jport1 = src->jport1;

    for (i = 0; i < MAX_INPUT_SETTINGS + 1; i++) {
	for (j = 0; j < MAX_INPUT_DEVICES; j++) {
	    memcpy (&dst->joystick_settings[i][j], &src->joystick_settings[i][j], sizeof (struct uae_input_device));
	    memcpy (&dst->mouse_settings[i][j], &src->mouse_settings[i][j], sizeof (struct uae_input_device));
	    memcpy (&dst->keyboard_settings[i][j], &src->keyboard_settings[i][j], sizeof (struct uae_input_device));
	}
    }

    inputdevice_updateconfig (dst);
}

void inputdevice_swap_ports (struct uae_prefs *p, int devnum)
{
    const struct inputdevice_functions *idf = getidf (devnum);
    struct uae_input_device *uid = get_uid (idf, inputdevice_get_device_index (devnum));
    int i, j, k, event, unit;
    const struct inputevent *ie, *ie2;

    for (i = 0; i < MAX_INPUT_DEVICE_EVENTS; i++) {
	for (j = 0; j < MAX_INPUT_SUB_EVENT; j++) {
	    event = uid->eventid[i][j];
	    if (event <= 0)
		continue;
	    ie = &events[event];
	    if (ie->unit <= 0)
		continue;
	    unit = ie->unit;
	    k = 1;
	    while (events[k].confname) {
		ie2 = &events[k];
		if (ie2->type == ie->type && ie2->data == ie->data && ie2->unit - 1 == ((ie->unit - 1) ^ 1) && ie2->allow_mask == ie->allow_mask) {
		    uid->eventid[i][j] = k;
		    break;
		}
		k++;
	    }
	}
    }
}

void inputdevice_copy_single_config (struct uae_prefs *p, int src, int dst, int devnum)
{
    if (src == dst)
	return;
    if (devnum < 0 || gettype (devnum) == IDTYPE_JOYSTICK)
	memcpy (p->joystick_settings[dst], p->joystick_settings[src], sizeof (struct uae_input_device) * MAX_INPUT_DEVICES);
    if (devnum < 0 || gettype (devnum) == IDTYPE_MOUSE)
	memcpy (p->mouse_settings[dst], p->mouse_settings[src], sizeof (struct uae_input_device) * MAX_INPUT_DEVICES);
    if (devnum < 0 || gettype (devnum) == IDTYPE_KEYBOARD)
	memcpy (p->keyboard_settings[dst], p->keyboard_settings[src], sizeof (struct uae_input_device) * MAX_INPUT_DEVICES);
}

void inputdevice_acquire (int allmode)
{
    int i;

    inputdevice_unacquire ();
    for (i = 0; i < MAX_INPUT_DEVICES; i++) {
		if (use_joysticks[i] || (allmode && !idev[IDTYPE_JOYSTICK].get_flags (i)))
	    idev[IDTYPE_JOYSTICK].acquire (i, 0);
    }
    for (i = 0; i < MAX_INPUT_DEVICES; i++) {
		if (use_mice[i] || (allmode && !idev[IDTYPE_MOUSE].get_flags (i)))
	    idev[IDTYPE_MOUSE].acquire (i, 0);
    }
    for (i = 0; i < MAX_INPUT_DEVICES; i++) {
	if (use_keyboards[i])
	    idev[IDTYPE_KEYBOARD].acquire (i, 0);
    }
	//    if (!input_acquired)
	//	write_log (L"input devices acquired (%s)\n", allmode ? "all" : "selected only");
	input_acquired = 1;
}

void inputdevice_unacquire (void)
{
    int i;

	//    if (input_acquired)
	//	write_log (L"input devices unacquired\n");
	input_acquired = 0;
    for (i = 0; i < MAX_INPUT_DEVICES; i++)
	idev[IDTYPE_JOYSTICK].unacquire (i);
    for (i = 0; i < MAX_INPUT_DEVICES; i++)
	idev[IDTYPE_MOUSE].unacquire (i);
    for (i = 0; i < MAX_INPUT_DEVICES; i++)
	idev[IDTYPE_KEYBOARD].unacquire (i);
}

static void testrecord (int type, int num, int wtype, int wnum, int state)
{
	TCHAR tmp[2000];
	tmp[0] = 0;
	wnum += idev[type].get_widget_first (num, wtype);
	idev[type].get_widget_type (num, wnum, tmp, NULL);
	write_log (L"%s: %s %d\n", idev[type].get_friendlyname (num), tmp, state);
}

int inputdevice_istest (void)
{
	return testmode;
}

int inputdevice_testread (TCHAR *name)
{
	testmode = 1;
	idev[IDTYPE_KEYBOARD].read ();
	idev[IDTYPE_JOYSTICK].read ();
	idev[IDTYPE_MOUSE].read ();
	testmode = 0;
	return 0;
}

static int ignoreoldinput (int joy)
{
	if (!use_joysticks[joy])
		return 0;
	if (currprefs.input_selected_setting == 0) {
		if (jsem_isjoy (0, &currprefs) != joy && jsem_isjoy (1, &currprefs) != joy &&
			jsem_isjoy (2, &currprefs) != joy && jsem_isjoy (3, &currprefs) != joy)
			return 1;
	}
	return 0;
}

void do_fake_joystick (int nr, int *fake)
{
	struct uae_input_device *id1;
	struct uae_input_device2 *id2;
	int state, i, axis;

	nr = compatibility_device[nr];
	id1 = &joysticks[nr];
	id2 = &joysticks2[nr];

	axis = 0;
	state = fake[1] ? -1 : (fake[2] ? 1 : 0);
	for (i = 0; i < MAX_INPUT_SUB_EVENT; i++)
		handle_input_event (id1->eventid[ID_AXIS_OFFSET + axis][i], state, 1,
		id1->flags[ID_AXIS_OFFSET + axis][i]);
	id2->states[axis] = state;

	axis = 1;
	state = fake[0] ? -1 : (fake[3] ? 1 : 0);
	for (i = 0; i < MAX_INPUT_SUB_EVENT; i++)
		handle_input_event (id1->eventid[ID_AXIS_OFFSET + axis][i], state, 1,
		id1->flags[ID_AXIS_OFFSET + axis][i]);
	id2->states[axis] = state;

	setbuttonstateall (id1, id2, 0, fake[4] ? 1 : 0);
	setbuttonstateall (id1, id2, 1, fake[5] ? 1 : 0);
	setbuttonstateall (id1, id2, 2, fake[6] ? 1 : 0);
}

/* Call this function when host machine's joystick/joypad/etc button state changes
 * This function translates button events to Amiga joybutton/joyaxis/keyboard events
 */

/* button states:
 * state = -1 -> mouse wheel turned or similar (button without release)
 * state = 1 -> button pressed
 * state = 0 -> button released
 */

void setjoybuttonstate (int joy, int button, int state)
{
	if (ignoreoldinput (joy)) {
		if (state)
			switchdevice (&joysticks[joy], button, 1);
	return;
	}
    setbuttonstateall (&joysticks[joy], &joysticks2[joy], button, state ? 1 : 0);
}

/* buttonmask = 1 = normal toggle button, 0 = mouse wheel turn or similar
 */
void setjoybuttonstateall (int joy, uae_u32 buttonbits, uae_u32 buttonmask)
{
    int i;

	if (ignoreoldinput (joy))
	return;
    for (i = 0; i < ID_BUTTON_TOTAL; i++) {
	if (buttonmask & (1 << i))
	    setbuttonstateall (&joysticks[joy], &joysticks2[joy], i, (buttonbits & (1 << i)) ? 1 : 0);
	else if (buttonbits & (1 << i))
	    setbuttonstateall (&joysticks[joy], &joysticks2[joy], i, -1);
    }
}
/* mouse buttons (just like joystick buttons)
 */
void setmousebuttonstateall (int mouse, uae_u32 buttonbits, uae_u32 buttonmask)
{
    int i;

    for (i = 0; i < ID_BUTTON_TOTAL; i++) {
	if (buttonmask & (1 << i))
	    setbuttonstateall (&mice[mouse], &mice2[mouse], i, (buttonbits & (1 << i)) ? 1 : 0);
	else if (buttonbits & (1 << i))
	    setbuttonstateall (&mice[mouse], &mice2[mouse], i, -1);
    }
}

void setmousebuttonstate (int mouse, int button, int state)
{
	if (testmode) {
		testrecord (IDTYPE_MOUSE, mouse, IDEV_WIDGET_BUTTON, button, state);
		return;
	}
    setbuttonstateall (&mice[mouse], &mice2[mouse], button, state);
}

/* same for joystick axis (analog or digital)
 * (0 = center, -max = full left/top, max = full right/bottom)
 */
void setjoystickstate (int joy, int axis, int state, int max)
{
	struct uae_input_device *id = &joysticks[joy];
	struct uae_input_device2 *id2 = &joysticks2[joy];
	int deadzone = currprefs.input_joymouse_deadzone * max / 100;
	int i, v1, v2;

	if (testmode) {
		testrecord (IDTYPE_JOYSTICK, joy, IDEV_WIDGET_AXIS, axis, state);
		return;
	}
	v1 = state;
	v2 = id2->states[axis];
	if (v1 < deadzone && v1 > -deadzone)
		v1 = 0;
	if (v2 < deadzone && v2 > -deadzone)
		v2 = 0;
	if (v1 == v2)
		return;
	if (!joysticks[joy].enabled || ignoreoldinput(joy)) {
		if (v1)
			switchdevice (&joysticks[joy], axis * 2 + (v1 < 0 ? 0 : 1), 0);
		return;
	}
	for (i = 0; i < MAX_INPUT_SUB_EVENT; i++)
		handle_input_event (id->eventid[ID_AXIS_OFFSET + axis][i], state, max,
		id->flags[ID_AXIS_OFFSET + axis][i]);
	id2->states[axis] = state;
}
int getjoystickstate(int joy)
{
	if (testmode)
		return 1;
	return joysticks[joy].enabled;
}

void setmousestate (int mouse, int axis, int data, int isabs)
{
	int i, v, diff;
	int *mouse_p, *oldm_p;
	double d;
    struct uae_input_device *id = &mice[mouse];
	static double fract[MAX_INPUT_DEVICES][MAX_INPUT_DEVICE_EVENTS];

	if (testmode) {
		testrecord (IDTYPE_MOUSE, mouse, IDEV_WIDGET_AXIS, axis, data);
		return;
	}
	if (!mice[mouse].enabled) {
		if (isabs && currprefs.input_tablet > 0) {
			if (axis == 0)
				lastmx = data;
			else
				lastmy = data;
			if (axis)
				mousehack_helper ();
		}
		return;
	}
    d = 0;
    mouse_p = &mouse_axis[mouse][axis];
    oldm_p = &oldm_axis[mouse][axis];
    if (!isabs) {
		*oldm_p = *mouse_p;
		*mouse_p += data;
		d = (*mouse_p - *oldm_p) * currprefs.input_mouse_speed / 100.0;
    } else {
		d = data - *oldm_p;
		*oldm_p = data;
		*mouse_p += d;
		if (axis == 0)
		    lastmx = data;
		else
		    lastmy = data;
		if (axis)
			mousehack_helper ();
		if (currprefs.input_tablet == TABLET_MOUSEHACK && mousehack_alive ())
			return;
    }
	v = (int)d;
	fract[mouse][axis] += d - v;
	diff = (int)fract[mouse][axis];
	v += diff;
	fract[mouse][axis] -= diff;
    for (i = 0; i < MAX_INPUT_SUB_EVENT; i++)
		handle_input_event (id->eventid[ID_AXIS_OFFSET + axis][i], v, 0, 0);
}

int getmousestate (int joy)
{
	if (testmode)
		return 1;
	return mice[joy].enabled;
}

void warpmode (int mode)
{
    int fr, fr2;
    
    fr = currprefs.gfx_framerate;
    if (fr == 0)
	fr = -1;
    fr2 = currprefs.turbo_emulation;
    if (fr2 == -1)
	fr2 = 0;

    if (mode < 0) {
	if (currprefs.turbo_emulation) {
	    changed_prefs.gfx_framerate = currprefs.gfx_framerate = fr2;
	    currprefs.turbo_emulation = 0;
	} else {
	    currprefs.turbo_emulation = fr;
	}
    } else if (mode == 0 && currprefs.turbo_emulation) {
	if (currprefs.turbo_emulation > 0)
	    changed_prefs.gfx_framerate = currprefs.gfx_framerate = fr2;
	currprefs.turbo_emulation = 0;
    } else if (mode > 0 && !currprefs.turbo_emulation) {
	currprefs.turbo_emulation = fr;
    }
    if (currprefs.turbo_emulation) {
	if (!currprefs.cpu_cycle_exact && !currprefs.blitter_cycle_exact)
	    changed_prefs.gfx_framerate = currprefs.gfx_framerate = 10;
	audio_pause ();
    } else {
	audio_resume ();
    }
    compute_vsynctime ();
#ifdef RETROPLATFORM
	rp_turbo (currprefs.turbo_emulation);
#endif
}

void pausemode (int mode)
{
	if (mode < 0)
		pause_emulation = pause_emulation ? 0 : 9;
	else
		pause_emulation = mode;
}

int jsem_isjoy (int port, const struct uae_prefs *p)
{
    int v = JSEM_DECODEVAL (port, p);
    if (v < JSEM_JOYS)
	return -1;
    v -= JSEM_JOYS;
    if (v >= inputdevice_get_device_total (IDTYPE_JOYSTICK))
	return -1;
    return v;
}

int jsem_ismouse (int port, const struct uae_prefs *p)
{
    int v = JSEM_DECODEVAL (port, p);
    if (v < JSEM_MICE)
	return -1;
    v -= JSEM_MICE;
    if (v >= inputdevice_get_device_total (IDTYPE_MOUSE))
	return -1;
    return v;
}

int jsem_iskbdjoy (int port, const struct uae_prefs *p)
{
    int v = JSEM_DECODEVAL (port, p);
    if (v < JSEM_KBDLAYOUT)
	return -1;
    v -= JSEM_KBDLAYOUT;
    if (v >= JSEM_LASTKBD)
	return -1;
    return v;
}
