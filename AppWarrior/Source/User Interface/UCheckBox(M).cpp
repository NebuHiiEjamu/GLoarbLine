#if MACINTOSH

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UCheckBox.h"
#include <QuickDraw.h>

/* -------------------------------------------------------------------------- */

typedef struct {
	Int16 height, width;
	Uint16 depth;
	Uint16 rowBytes;
	const void *data;
	const void *mask;
	const void *colortab;
} SSimplePixMap;

const Uint32 CT_checkbox_col_enabled_off[] = {
	0x00002448, 0x00000003, 0x0000FFFF, 0xFFFFFFFF, 0x0001DDDD, 0xDDDDDDDD, 0x00028888,
	0x88888888, 0x00030000, 0x00000000
};
const Uint32 CT_checkbox_col_enabled_on[] = {
	0x0000322D, 0x00000005, 0x0000FFFF, 0xFFFFFFFF, 0x0001DDDD, 0xDDDDDDDD, 0x00028888,
	0x88888888, 0x0003AAAA, 0xAAAAAAAA, 0x00047777, 0x77777777, 0x000F0000, 0x00000000
};
const Uint32 CT_checkbox_col_enabled_mixed[] = {
	0x00003485, 0x00000004, 0x0000FFFF, 0xFFFFFFFF, 0x0001DDDD, 0xDDDDDDDD, 0x00028888,
	0x88888888, 0x0003AAAA, 0xAAAAAAAA, 0x000F0000, 0x00000000
};
const Uint32 CT_checkbox_col_enabled_ticked[] = {
	0x00005262, 0x00000004, 0x0000FFFF, 0xFFFFFFFF, 0x0001DDDD, 0xDDDDDDDD, 0x0002AAAA,
	0xAAAAAAAA, 0x00038888, 0x88888888, 0x000F0000, 0x00000000
};
const Uint32 PD_checkbox_col_enabled_off[] = {
	0xFFFFFF00, 0xC0000700, 0xC5555B00, 0xC5555B00, 0xC5555B00, 0xC5555B00, 0xC5555B00,
	0xC5555B00, 0xC5555B00, 0xC5555B00, 0xDAAAAB00, 0xFFFFFF00
};
const Uint32 PD_checkbox_col_enabled_on[] = {
	0xFFFFFFFF, 0xFFFF0000, 0xF0000000, 0x001F0000, 0xF01F1111, 0xF12F0000, 0xF01FF11F,
	0xF42F0000, 0xF011FFFF, 0x432F0000, 0xF0111FF4, 0x312F0000, 0xF011FFFF, 0x112F0000,
	0xF01FF43F, 0xF12F0000, 0xF01F4311, 0xF42F0000, 0xF0113111, 0x132F0000, 0xF1222222,
	0x222F0000, 0xFFFFFFFF, 0xFFFF0000
};
const Uint32 PD_checkbox_col_enabled_mixed[] = {
	0xFFFFFFFF, 0xFFFF0000, 0xF0000000, 0x001F0000, 0xF0111111, 0x112F0000, 0xF0111111,
	0x112F0000, 0xF0111111, 0x112F0000, 0xF01FFFFF, 0xF32F0000, 0xF01FFFFF, 0xF32F0000,
	0xF0113333, 0x332F0000, 0xF0111111, 0x112F0000, 0xF0111111, 0x112F0000, 0xF1222222,
	0x222F0000, 0xFFFFFFFF, 0xFFFF0000
};
const Uint32 PD_checkbox_col_enabled_ticked[] = {
	0xFFFFFFFF, 0xFFFF0000, 0xF0000000, 0x001F0000, 0xF0111111, 0x1F3F0000, 0xF0111111,
	0xFF3F0000, 0xF011F11F, 0xF23F0000, 0xF01FF1FF, 0x213F0000, 0xF01FFFF2, 0x113F0000,
	0xF01FFF21, 0x113F0000, 0xF01FF211, 0x113F0000, 0xF0122111, 0x113F0000, 0xF1333333,
	0x333F0000, 0xFFFFFFFF, 0xFFFF0000
};
const Uint32 CT_checkbox_col_pressed_off[] = {
	0x00005242, 0x00000003, 0x00009999, 0x99999999, 0x00017777, 0x77777777, 0x00025555,
	0x55555555, 0x00030000, 0x00000000
};
const Uint32 CT_checkbox_col_pressed_on[] = {
	0x0000576B, 0x00000004, 0x00009999, 0x99999999, 0x00017777, 0x77777777, 0x00025555,
	0x55555555, 0x00034444, 0x44444444, 0x000F0000, 0x00000000
};
const Uint32 CT_checkbox_col_pressed_mixed[] = {
	0x00005C41, 0x00000003, 0x00009999, 0x99999999, 0x00017777, 0x77777777, 0x00025555,
	0x55555555, 0x00030000, 0x00000000
};
const Uint32 CT_checkbox_col_pressed_ticked[] = {
	0x00005816, 0x00000003, 0x00009999, 0x99999999, 0x00017777, 0x77777777, 0x00025555,
	0x55555555, 0x00030000, 0x00000000
};
const Uint32 PD_checkbox_col_pressed_off[] = {
	0xFFFFFF00, 0xEAAAA700, 0xE5555300, 0xE5555300, 0xE5555300, 0xE5555300, 0xE5555300,
	0xE5555300, 0xE5555300, 0xE5555300, 0xD0000300, 0xFFFFFF00
};
const Uint32 PD_checkbox_col_pressed_on[] = {
	0xFFFFFFFF, 0xFFFF0000, 0xF2222222, 0x221F0000, 0xF21F1111, 0xF10F0000, 0xF21FF11F,
	0xF30F0000, 0xF211FFFF, 0x320F0000, 0xF2111FF3, 0x210F0000, 0xF211FFFF, 0x110F0000,
	0xF21FF32F, 0xF10F0000, 0xF21F3211, 0xF30F0000, 0xF2112111, 0x120F0000, 0xF1000000,
	0x000F0000, 0xFFFFFFFF, 0xFFFF0000
};
const Uint32 PD_checkbox_col_pressed_mixed[] = {
	0xFFFFFF00, 0xEAAAA700, 0xE5555300, 0xE5555300, 0xE5555300, 0xE7FFE300, 0xE7FFE300,
	0xE5AAA300, 0xE5555300, 0xE5555300, 0xD0000300, 0xFFFFFF00
};
const Uint32 PD_checkbox_col_pressed_ticked[] = {
	0xFFFFFF00, 0xEAAAA700, 0xE5557300, 0xE555F300, 0xE5D7E300, 0xE7DF9300, 0xE7FE5300,
	0xE7F95300, 0xE7E55300, 0xE6955300, 0xD0000300, 0xFFFFFF00
};
const Uint32 CT_checkbox_col_disabled_off[] = {
	0x0000620E, 0x00000003, 0x0000FFFF, 0xFFFFFFFF, 0x0001DDDD, 0xDDDDDDDD, 0x0002AAAA,
	0xAAAAAAAA, 0x00038888, 0x88888888
};
const Uint32 CT_checkbox_col_disabled_on[] = {
	0x0000660A, 0x00000003, 0x0000FFFF, 0xFFFFFFFF, 0x0001DDDD, 0xDDDDDDDD, 0x0002AAAA,
	0xAAAAAAAA, 0x00038888, 0x88888888
};
const Uint32 CT_checkbox_col_disabled_mixed[] = {
	0x00006979, 0x00000003, 0x0000FFFF, 0xFFFFFFFF, 0x0001DDDD, 0xDDDDDDDD, 0x0002AAAA,
	0xAAAAAAAA, 0x00038888, 0x88888888
};
const Uint32 CT_checkbox_col_disabled_ticked[] = {
	0x00005C3D, 0x00000003, 0x0000FFFF, 0xFFFFFFFF, 0x0001DDDD, 0xDDDDDDDD, 0x0002AAAA,
	0xAAAAAAAA, 0x00038888, 0x88888888
};
const Uint32 PD_checkbox_col_disabled_off[] = {
	0xFFFFFF00, 0xC0000700, 0xC5555B00, 0xC5555B00, 0xC5555B00, 0xC5555B00, 0xC5555B00,
	0xC5555B00, 0xC5555B00, 0xC5555B00, 0xDAAAAB00, 0xFFFFFF00
};
const Uint32 PD_checkbox_col_disabled_on[] = {
	0xFFFFFF00, 0xC0000700, 0xC755DB00, 0xC7D7EB00, 0xC5FFAB00, 0xC57E9B00, 0xC5FF5B00,
	0xC7EBDB00, 0xC7A5EB00, 0xC5956B00, 0xDAAAAB00, 0xFFFFFF00
};
const Uint32 PD_checkbox_col_disabled_mixed[] = {
	0xFFFFFF00, 0xC0000700, 0xC5555B00, 0xC5555B00, 0xC5555B00, 0xC7FFEB00, 0xC7FFEB00,
	0xC5AAAB00, 0xC5555B00, 0xC5555B00, 0xDAAAAB00, 0xFFFFFF00
};
const Uint32 PD_checkbox_col_disabled_ticked[] = {
	0xFFFFFF00, 0xC0000700, 0xC5557B00, 0xC555FB00, 0xC5D7EB00, 0xC7DF9B00, 0xC7FE5B00,
	0xC7F95B00, 0xC7E55B00, 0xC6955B00, 0xDAAAAB00, 0xFFFFFF00
};

const SSimplePixMap PM_checkbox_col_enabled_off			= { 12, 12, 2, 4, PD_checkbox_col_enabled_off, nil, CT_checkbox_col_enabled_off };
const SSimplePixMap PM_checkbox_col_enabled_on			= { 12, 12, 4, 8, PD_checkbox_col_enabled_on, nil, CT_checkbox_col_enabled_on };
const SSimplePixMap PM_checkbox_col_enabled_mixed		= { 12, 12, 4, 8, PD_checkbox_col_enabled_mixed, nil, CT_checkbox_col_enabled_mixed };
const SSimplePixMap PM_checkbox_col_enabled_ticked		= { 12, 12, 4, 8, PD_checkbox_col_enabled_ticked, nil, CT_checkbox_col_enabled_ticked };

const SSimplePixMap PM_checkbox_col_pressed_off			= { 12, 12, 2, 4, PD_checkbox_col_pressed_off, nil, CT_checkbox_col_pressed_off };
const SSimplePixMap PM_checkbox_col_pressed_on			= { 12, 12, 4, 8, PD_checkbox_col_pressed_on, nil, CT_checkbox_col_pressed_on };
const SSimplePixMap PM_checkbox_col_pressed_mixed		= { 12, 12, 2, 4, PD_checkbox_col_pressed_mixed, nil, CT_checkbox_col_pressed_mixed };
const SSimplePixMap PM_checkbox_col_pressed_ticked		= { 12, 12, 2, 4, PD_checkbox_col_pressed_ticked, nil, CT_checkbox_col_pressed_ticked };

const SSimplePixMap PM_checkbox_col_disabled_off		= { 12, 12, 2, 4, PD_checkbox_col_disabled_off, nil, CT_checkbox_col_disabled_off };
const SSimplePixMap PM_checkbox_col_disabled_on			= { 12, 12, 2, 4, PD_checkbox_col_disabled_on, nil, CT_checkbox_col_disabled_on };
const SSimplePixMap PM_checkbox_col_disabled_mixed		= { 12, 12, 2, 4, PD_checkbox_col_disabled_mixed, nil, CT_checkbox_col_disabled_mixed };
const SSimplePixMap PM_checkbox_col_disabled_ticked		= { 12, 12, 2, 4, PD_checkbox_col_disabled_ticked, nil, CT_checkbox_col_disabled_ticked };

const Uint32 PD_checkbox_bw_enabled_off[] = {
	0xFFF08010, 0x80108010, 0x80108010, 0x80108010, 0x80108010, 0x8010FFF0
};
const Uint32 PD_checkbox_bw_enabled_on[] = {
	0xFFF0C030, 0xA0509090, 0x89108610, 0x86108910, 0x9090A050, 0xC030FFF0
};
const Uint32 PD_checkbox_bw_enabled_mixed[] = {
	0xFFF08010, 0x80108010, 0x80109F90, 0x9F908010, 0x80108010, 0x8010FFF0
};
const Uint32 PD_checkbox_bw_pressed_off[] = {
	0xFFF0FFF0, 0xC030C030, 0xC030C030, 0xC030C030, 0xC030C030, 0xFFF0FFF0
};
const Uint32 PD_checkbox_bw_pressed_on[] = {
	0xFFF0FFF0, 0xE070D0B0, 0xC930C630, 0xC630C930, 0xD0B0E070, 0xFFF0FFF0
};
const Uint32 PD_checkbox_bw_pressed_mixed[] = {
	0xFFF0FFF0, 0xC030C030, 0xC030DFB0, 0xDFB0C030, 0xC030C030, 0xFFF0FFF0
};

const SSimplePixMap PM_checkbox_bw_enabled_off			= { 12, 12, 1, 2, PD_checkbox_bw_enabled_off, nil, nil };
const SSimplePixMap PM_checkbox_bw_enabled_on			= { 12, 12, 1, 2, PD_checkbox_bw_enabled_on, nil, nil };
const SSimplePixMap PM_checkbox_bw_enabled_mixed		= { 12, 12, 1, 2, PD_checkbox_bw_enabled_mixed, nil, nil };
const SSimplePixMap PM_checkbox_bw_enabled_ticked		= { 12, 12, 4, 8, PD_checkbox_col_enabled_ticked, nil, CT_checkbox_col_enabled_ticked };

const SSimplePixMap PM_checkbox_bw_pressed_off			= { 12, 12, 1, 2, PD_checkbox_bw_pressed_off, nil, nil };
const SSimplePixMap PM_checkbox_bw_pressed_on			= { 12, 12, 1, 2, PD_checkbox_bw_pressed_on, nil, nil };
const SSimplePixMap PM_checkbox_bw_pressed_mixed		= { 12, 12, 1, 2, PD_checkbox_bw_pressed_mixed, nil, nil };
const SSimplePixMap PM_checkbox_bw_pressed_ticked		= { 12, 12, 2, 4, PD_checkbox_col_pressed_ticked, nil, CT_checkbox_col_pressed_ticked };

#define PM_radio_bw_disabled_off		PM_radio_bw_enabled_off
#define PM_radio_bw_disabled_on			PM_radio_bw_enabled_on
#define PM_radio_bw_disabled_mixed		PM_radio_bw_enabled_mixed
#define PM_radio_bw_disabled_ticked		PM_radio_bw_enabled_ticked

#define PM_checkbox_bw_disabled_off		PM_checkbox_bw_enabled_off
#define PM_checkbox_bw_disabled_on		PM_checkbox_bw_enabled_on
#define PM_checkbox_bw_disabled_mixed	PM_checkbox_bw_enabled_mixed
#define PM_checkbox_bw_disabled_ticked	PM_checkbox_bw_enabled_ticked

/* -------------------------------------------------------------------------- */

const Uint32 CT_radio_col_enabled_off[] = {
	0x00009145, 0x00000008, 0x0000FFFF, 0xFFFFFFFF, 0x0001DDDD, 0xDDDDDDDD, 0x00028888,
	0x88888888, 0x0003EEEE, 0xEEEEEEEE, 0x0004BBBB, 0xBBBBBBBB, 0x0005AAAA, 0xAAAAAAAA,
	0x00065555, 0x55555555, 0x00074444, 0x44444444, 0x000F0000, 0x00000000
};
const Uint32 CT_radio_col_enabled_on[] = {
	0x0000954B, 0x0000000A, 0x0000FFFF, 0xFFFFFFFF, 0x0001DDDD, 0xDDDDDDDD, 0x00028888,
	0x88888888, 0x0003BBBB, 0xBBBBBBBB, 0x0004AAAA, 0xAAAAAAAA, 0x00055555, 0x55555555,
	0x00064444, 0x44444444, 0x00079999, 0x99999999, 0x00087777, 0x77777777, 0x00092222,
	0x22222222, 0x000F0000, 0x00000000
};
const Uint32 CT_radio_col_enabled_mixed[] = {
	0x000098F1, 0x00000008, 0x0000FFFF, 0xFFFFFFFF, 0x0001DDDD, 0xDDDDDDDD, 0x00028888,
	0x88888888, 0x0003BBBB, 0xBBBBBBBB, 0x0004AAAA, 0xAAAAAAAA, 0x00055555, 0x55555555,
	0x00064444, 0x44444444, 0x0007EEEE, 0xEEEEEEEE, 0x000F0000, 0x00000000
};
const Uint32 PD_radio_col_enabled_off[] = {
	0x00057FF7, 0x50000000, 0x00F61114, 0x7F000000, 0x0F413000, 0x12F00000, 0x56130033,
	0x14750000, 0x71300331, 0x14270000, 0xF1003311, 0x442F0000, 0xF1033114, 0x452F0000,
	0x74031144, 0x55270000, 0x56111445, 0x52750000, 0x0F244455, 0x22F00000, 0x00F62222,
	0x7F000000, 0x00057FF7, 0x50000000
};
const Uint32 PD_radio_col_enabled_on[] = {
	0x00049FF9, 0x40000000, 0x009F6555, 0x6F000000, 0x09658882, 0x27F00000, 0x4F58FFFF,
	0x77640000, 0x968FFFFF, 0xF7360000, 0xF58FFFFF, 0xF33F0000, 0xF58FFFFF, 0xF31F0000,
	0x952FFFFF, 0xF1060000, 0x4927FFFF, 0x10640000, 0x09777331, 0x00F00000, 0x00963310,
	0x6F000000, 0x00046FF6, 0x40000000
};
const Uint32 PD_radio_col_enabled_mixed[] = {
	0x00046FF6, 0x40000000, 0x00F51113, 0x6F000000, 0x0F317000, 0x12F00000, 0x45170077,
	0x13640000, 0x61700771, 0x13260000, 0xF10FFFFF, 0xF32F0000, 0xF10FFFFF, 0xF42F0000,
	0x63071133, 0x44260000, 0x45111334, 0x42640000, 0x0F233344, 0x22F00000, 0x00F52222,
	0x6F000000, 0x00046FF6, 0x40000000
};
const Uint32 CT_radio_col_pressed_off[] = {
	0x00009CE5, 0x0000000A, 0x0000FFFF, 0xFFFFFFFF, 0x00018888, 0x88888888, 0x0002BBBB,
	0xBBBBBBBB, 0x0003AAAA, 0xAAAAAAAA, 0x00045555, 0x55555555, 0x00054444, 0x44444444,
	0x00069999, 0x99999999, 0x00076666, 0x66666666, 0x00087777, 0x77777777, 0x00092222,
	0x22222222, 0x000F0000, 0x00000000
};
const Uint32 CT_radio_col_pressed_on[] = {
	0x0000A175, 0x0000000A, 0x0000FFFF, 0xFFFFFFFF, 0x00018888, 0x88888888, 0x0002BBBB,
	0xBBBBBBBB, 0x0003AAAA, 0xAAAAAAAA, 0x00045555, 0x55555555, 0x00054444, 0x44444444,
	0x00069999, 0x99999999, 0x00076666, 0x66666666, 0x00087777, 0x77777777, 0x00092222,
	0x22222222, 0x000F0000, 0x00000000
};
const Uint32 CT_radio_col_pressed_mixed[] = {
	0x0000A4A4, 0x0000000A, 0x0000FFFF, 0xFFFFFFFF, 0x00018888, 0x88888888, 0x0002BBBB,
	0xBBBBBBBB, 0x0003AAAA, 0xAAAAAAAA, 0x00045555, 0x55555555, 0x00054444, 0x44444444,
	0x00069999, 0x99999999, 0x00076666, 0x66666666, 0x00087777, 0x77777777, 0x00092222,
	0x22222222, 0x000F0000, 0x00000000
};
const Uint32 PD_radio_col_pressed_off[] = {
	0x00039FF9, 0x30000000, 0x009F5555, 0x5F000000, 0x09554477, 0x78F00000, 0x3F544778,
	0x88530000, 0x95447788, 0x81650000, 0xF5477888, 0x116F0000, 0xF5778881, 0x166F0000,
	0x95788811, 0x66250000, 0x39788116, 0x62430000, 0x05881166, 0x22F00000, 0x00956662,
	0x4F000000, 0x00035FF5, 0x30000000
};
const Uint32 PD_radio_col_pressed_on[] = {
	0x00039FF9, 0x30000000, 0x009F5555, 0x5F000000, 0x09554477, 0x78F00000, 0x3F54FFFF,
	0x88530000, 0x954FFFFF, 0xF1650000, 0xF54FFFFF, 0xF16F0000, 0xF57FFFFF, 0xF66F0000,
	0x957FFFFF, 0xF6250000, 0x3978FFFF, 0x62430000, 0x05881166, 0x22F00000, 0x00956662,
	0x4F000000, 0x00035FF5, 0x30000000
};
const Uint32 PD_radio_col_pressed_mixed[] = {
	0x00039FF9, 0x30000000, 0x009F5555, 0x5F000000, 0x09554477, 0x78F00000, 0x3F544778,
	0x88530000, 0x95447788, 0x81650000, 0xF54FFFFF, 0xF16F0000, 0xF57FFFFF, 0xF66F0000,
	0x95788811, 0x66250000, 0x39788116, 0x62430000, 0x05881166, 0x22F00000, 0x00956662,
	0x4F000000, 0x00035FF5, 0x30000000
};
const Uint32 CT_radio_col_disabled_off[] = {
	0x0000A800, 0x00000005, 0x0000FFFF, 0xFFFFFFFF, 0x00018888, 0x88888888, 0x0002BBBB,
	0xBBBBBBBB, 0x0003AAAA, 0xAAAAAAAA, 0x0004CCCC, 0xCCCCCCCC, 0x0005EEEE, 0xEEEEEEEE
};
const Uint32 CT_radio_col_disabled_on[] = {
	0x0000AADD, 0x00000007, 0x0000FFFF, 0xFFFFFFFF, 0x00018888, 0x88888888, 0x0002BBBB,
	0xBBBBBBBB, 0x0003AAAA, 0xAAAAAAAA, 0x0004CCCC, 0xCCCCCCCC, 0x0005EEEE, 0xEEEEEEEE,
	0x0006DDDD, 0xDDDDDDDD, 0x00077777, 0x77777777
};
const Uint32 CT_radio_col_disabled_mixed[] = {
	0x0000AD89, 0x00000006, 0x0000FFFF, 0xFFFFFFFF, 0x00018888, 0x88888888, 0x0002BBBB,
	0xBBBBBBBB, 0x0003AAAA, 0xAAAAAAAA, 0x0004CCCC, 0xCCCCCCCC, 0x0005EEEE, 0xEEEEEEEE,
	0x00067777, 0x77777777
};
const Uint32 PD_radio_col_disabled_off[] = {
	0x00021111, 0x20000000, 0x00115554, 0x11000000, 0x01450000, 0x53100000, 0x21500000,
	0x54120000, 0x15000005, 0x54310000, 0x15000055, 0x44310000, 0x15000554, 0x42310000,
	0x14005544, 0x22310000, 0x21555442, 0x23120000, 0x01344422, 0x33100000, 0x00113333,
	0x11000000, 0x00021111, 0x20000000
};
const Uint32 PD_radio_col_disabled_on[] = {
	0x00061111, 0x60000000, 0x00113333, 0x31000000, 0x01332224, 0x44100000, 0x61327777,
	0x44360000, 0x13277777, 0x74530000, 0x13277777, 0x75510000, 0x13277777, 0x75010000,
	0x13477777, 0x70030000, 0x63447777, 0x00360000, 0x01444550, 0x00100000, 0x00115500,
	0x31000000, 0x00063113, 0x60000000
};
const Uint32 PD_radio_col_disabled_mixed[] = {
	0x00021111, 0x20000000, 0x00115554, 0x11000000, 0x01450000, 0x53100000, 0x21500000,
	0x54120000, 0x15000005, 0x54310000, 0x15066666, 0x64310000, 0x15066666, 0x62310000,
	0x14005544, 0x22310000, 0x21555442, 0x23120000, 0x01344422, 0x33100000, 0x00113333,
	0x11000000, 0x00021111, 0x20000000
};
const Uint32 MD_radio_col[] = {
	0x1F8F3FCF, 0x7FEFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF7FEF, 0x3FCF1F8F
};

const SSimplePixMap PM_radio_col_enabled_off		= { 12, 12, 4, 8, PD_radio_col_enabled_off, MD_radio_col, CT_radio_col_enabled_off };
const SSimplePixMap PM_radio_col_enabled_on			= { 12, 12, 4, 8, PD_radio_col_enabled_on, MD_radio_col, CT_radio_col_enabled_on };
const SSimplePixMap PM_radio_col_enabled_mixed		= { 12, 12, 4, 8, PD_radio_col_enabled_mixed, MD_radio_col, CT_radio_col_enabled_mixed };

const SSimplePixMap PM_radio_col_pressed_off		= { 12, 12, 4, 8, PD_radio_col_pressed_off, MD_radio_col, CT_radio_col_pressed_off };
const SSimplePixMap PM_radio_col_pressed_on			= { 12, 12, 4, 8, PD_radio_col_pressed_on, MD_radio_col, CT_radio_col_pressed_on };
const SSimplePixMap PM_radio_col_pressed_mixed		= { 12, 12, 4, 8, PD_radio_col_pressed_mixed, MD_radio_col, CT_radio_col_pressed_mixed };

const SSimplePixMap PM_radio_col_disabled_off		= { 12, 12, 4, 8, PD_radio_col_disabled_off, MD_radio_col, CT_radio_col_disabled_off };
const SSimplePixMap PM_radio_col_disabled_on		= { 12, 12, 4, 8, PD_radio_col_disabled_on, MD_radio_col, CT_radio_col_disabled_on };
const SSimplePixMap PM_radio_col_disabled_mixed		= { 12, 12, 4, 8, PD_radio_col_disabled_mixed, MD_radio_col, CT_radio_col_disabled_mixed };

const Uint32 PD_radio_bw_enabled_off[] = {
	0x0F0030C0, 0x40204020, 0x80108010, 0x80108010, 0x40204020, 0x30C00F00
};
const Uint32 PD_radio_bw_enabled_on[] = {
	0x0F0030C0, 0x40204F20, 0x9F909F90, 0x9F909F90, 0x4F204020, 0x30C00F00
};
const Uint32 PD_radio_bw_enabled_mixed[] = {
	0x0F0030C0, 0x40204020, 0x80109F90, 0x9F908010, 0x40204020, 0x30C00F00
};
const Uint32 PD_radio_bw_pressed_off[] = {
	0x0F003FC0, 0x70E06060, 0xC030C030, 0xC030C030, 0x606070E0, 0x3FC00F00
};
const Uint32 PD_radio_bw_pressed_on[] = {
	0x0F003FC0, 0x70E06F60, 0xDFB0DFB0, 0xDFB0DFB0, 0x6F6070E0, 0x3FC00F00
};
const Uint32 PD_radio_bw_pressed_mixed[] = {
	0x0F003FC0, 0x70E06060, 0xC030DFB0, 0xDFB0C030, 0x606070E0, 0x3FC00F00
};
const Uint32 MD_radio_bw[] = {
	0x0F0F3FCF, 0x7FEF7FEF, 0xFFFFFFFF, 0xFFFFFFFF, 0x7FEF7FEF, 0x3FCF0F0F
};

const SSimplePixMap PM_radio_bw_enabled_off			= { 12, 12, 1, 2, PD_radio_bw_enabled_off, MD_radio_bw, nil };
const SSimplePixMap PM_radio_bw_enabled_on			= { 12, 12, 1, 2, PD_radio_bw_enabled_on, MD_radio_bw, nil };
const SSimplePixMap PM_radio_bw_enabled_mixed		= { 12, 12, 1, 2, PD_radio_bw_enabled_mixed, MD_radio_bw, nil };

const SSimplePixMap PM_radio_bw_pressed_off			= { 12, 12, 1, 2, PD_radio_bw_pressed_off, MD_radio_bw, nil };
const SSimplePixMap PM_radio_bw_pressed_on			= { 12, 12, 1, 2, PD_radio_bw_pressed_on, MD_radio_bw, nil };
const SSimplePixMap PM_radio_bw_pressed_mixed		= { 12, 12, 1, 2, PD_radio_bw_pressed_mixed, MD_radio_bw, nil };

/* -------------------------------------------------------------------------- */

void _DrawSimplePixMap(const SSimplePixMap& inPix, const Rect& inBounds);
static void _CalcRects(Rect& inBaseRect, Rect *outCheckRect, Rect *outTitleRect);
void _DrawVCenteredText(const void *inText, Uint32 inLength, Rect& r, Int16 offset = 0);

GrafPtr _ImageToGrafPtr(TImage inImage);

/* -------------------------------------------------------------------------- */

void UCheckBox_Draw(TImage inImage, const SRect& inBounds, const SCheckBoxInfo& inInfo)
{
	Rect bounds = { inBounds.top, inBounds.left, inBounds.bottom, inBounds.right };
	Rect checkRect, titleRect;
	const SSimplePixMap *pix;
	
	::SetPort(_ImageToGrafPtr(inImage));
	
	_CalcRects(bounds, &checkRect, &titleRect);

	if (inInfo.style == 1)
	{
		if (inInfo.options & 0x8000)	// if b/w
		{
			if (inInfo.mark == 1)
				pix = &PM_radio_bw_enabled_on;
			else if (inInfo.mark == 2)
				pix = &PM_radio_bw_enabled_mixed;
			else
				pix = &PM_radio_bw_enabled_off;
		}
		else
		{
			if (inInfo.mark == 1)
				pix = &PM_radio_col_enabled_on;
			else if (inInfo.mark == 2)
				pix = &PM_radio_col_enabled_mixed;
			else
				pix = &PM_radio_col_enabled_off;
		}
	}
	/*
	else if (inInfo.style == 0)
	{
		if (inInfo.isColor)
		{
			if (inInfo.mark == 1)
				pix = &PM_checkbox_col_enabled_on;
			else if (inInfo.mark == 2)
				pix = &PM_checkbox_col_enabled_mixed;
			else
				pix = &PM_checkbox_col_enabled_off;
		}
		else
		{
			if (inInfo.mark == 1)
				pix = &PM_checkbox_bw_enabled_on;
			else if (inInfo.mark == 2)
				pix = &PM_checkbox_bw_enabled_mixed;
			else
				pix = &PM_checkbox_bw_enabled_off;
		}
	}
	*/
	else
	{
		if (inInfo.options & 0x8000)	// if b/w
		{
			if (inInfo.mark == 1)
				pix = &PM_checkbox_bw_enabled_ticked;
			else if (inInfo.mark == 2)
				pix = &PM_checkbox_bw_enabled_mixed;
			else
				pix = &PM_checkbox_bw_enabled_off;
		}
		else
		{
			if (inInfo.mark == 1)
				pix = &PM_checkbox_col_enabled_ticked;
			else if (inInfo.mark == 2)
				pix = &PM_checkbox_col_enabled_mixed;
			else
				pix = &PM_checkbox_col_enabled_off;
		}
	}

	_DrawSimplePixMap(*pix, checkRect);
	
	if (inInfo.title)
	{
		ForeColor(blackColor);
		TextMode(srcOr);
		_DrawVCenteredText(inInfo.title, inInfo.titleSize, titleRect, titleRect.left);
	}
}

void UCheckBox_DrawFocused(TImage inImage, const SRect& inBounds, const SCheckBoxInfo& inInfo)
{
	UCheckBox_Draw(inImage, inBounds, inInfo);
}

void UCheckBox_DrawHilited(TImage inImage, const SRect& inBounds, const SCheckBoxInfo& inInfo)
{
	Rect bounds = { inBounds.top, inBounds.left, inBounds.bottom, inBounds.right };
	Rect checkRect, titleRect;
	const SSimplePixMap *pix;
	
	::SetPort(_ImageToGrafPtr(inImage));
	
	_CalcRects(bounds, &checkRect, &titleRect);

	if (inInfo.style == 1)
	{
		if (inInfo.options & 0x8000)	// if b/w
		{
			if (inInfo.mark == 1)
				pix = &PM_radio_bw_pressed_on;
			else if (inInfo.mark == 2)
				pix = &PM_radio_bw_pressed_mixed;
			else
				pix = &PM_radio_bw_pressed_off;
		}
		else
		{
			if (inInfo.mark == 1)
				pix = &PM_radio_col_pressed_on;
			else if (inInfo.mark == 2)
				pix = &PM_radio_col_pressed_mixed;
			else
				pix = &PM_radio_col_pressed_off;
		}
	}
	/*
	else if (inInfo.style == 0)
	{
		if (inInfo.isColor)
		{
			if (inInfo.mark == 1)
				pix = &PM_checkbox_col_pressed_on;
			else if (inInfo.mark == 2)
				pix = &PM_checkbox_col_pressed_mixed;
			else
				pix = &PM_checkbox_col_pressed_off;
		}
		else
		{
			if (inInfo.mark == 1)
				pix = &PM_checkbox_bw_pressed_on;
			else if (inInfo.mark == 2)
				pix = &PM_checkbox_bw_pressed_mixed;
			else
				pix = &PM_checkbox_bw_pressed_off;
		}
	}
	*/
	else
	{
		if (inInfo.options & 0x8000)	// if b/w
		{
			if (inInfo.mark == 1)
				pix = &PM_checkbox_bw_pressed_ticked;
			else if (inInfo.mark == 2)
				pix = &PM_checkbox_bw_pressed_mixed;
			else
				pix = &PM_checkbox_bw_pressed_off;
		}
		else
		{
			if (inInfo.mark == 1)
				pix = &PM_checkbox_col_pressed_ticked;
			else if (inInfo.mark == 2)
				pix = &PM_checkbox_col_pressed_mixed;
			else
				pix = &PM_checkbox_col_pressed_off;
		}
	}

	_DrawSimplePixMap(*pix, checkRect);
	
	if (inInfo.title)
	{
		ForeColor(blackColor);
		TextMode(srcOr);
		_DrawVCenteredText(inInfo.title, inInfo.titleSize, titleRect, titleRect.left);
	}
}

void UCheckBox_DrawDisabled(TImage inImage, const SRect& inBounds, const SCheckBoxInfo& inInfo)
{
	Rect bounds = { inBounds.top, inBounds.left, inBounds.bottom, inBounds.right };
	Rect checkRect, titleRect;
	const SSimplePixMap *pix;
	
	::SetPort(_ImageToGrafPtr(inImage));
	
	_CalcRects(bounds, &checkRect, &titleRect);

	if (inInfo.style == 1)
	{
		if (inInfo.options & 0x8000)	// if b/w
		{
			if (inInfo.mark == 1)
				pix = &PM_radio_bw_disabled_on;
			else if (inInfo.mark == 2)
				pix = &PM_radio_bw_disabled_mixed;
			else
				pix = &PM_radio_bw_disabled_off;
		}
		else
		{
			if (inInfo.mark == 1)
				pix = &PM_radio_col_disabled_on;
			else if (inInfo.mark == 2)
				pix = &PM_radio_col_disabled_mixed;
			else
				pix = &PM_radio_col_disabled_off;
		}
	}
	/*
	else if (inInfo.style == 0)
	{
		if (inInfo.isColor)
		{
			if (inInfo.mark == 1)
				pix = &PM_checkbox_col_disabled_on;
			else if (inInfo.mark == 2)
				pix = &PM_checkbox_col_disabled_mixed;
			else
				pix = &PM_checkbox_col_disabled_off;
		}
		else
		{
			if (inInfo.mark == 1)
				pix = &PM_checkbox_bw_disabled_on;
			else if (inInfo.mark == 2)
				pix = &PM_checkbox_bw_disabled_mixed;
			else
				pix = &PM_checkbox_bw_disabled_off;
		}
	}
	*/
	else
	{
		if (inInfo.options & 0x8000)	// if b/w
		{
			if (inInfo.mark == 1)
				pix = &PM_checkbox_bw_disabled_ticked;
			else if (inInfo.mark == 2)
				pix = &PM_checkbox_bw_disabled_mixed;
			else
				pix = &PM_checkbox_bw_disabled_off;
		}
		else
		{
			if (inInfo.mark == 1)
				pix = &PM_checkbox_col_disabled_ticked;
			else if (inInfo.mark == 2)
				pix = &PM_checkbox_col_disabled_mixed;
			else
				pix = &PM_checkbox_col_disabled_off;
		}
	}

	_DrawSimplePixMap(*pix, checkRect);
	
	if (inInfo.title)
	{
		if (inInfo.options & 0x8000)	// if b/w
		{
			TextMode(grayishTextOr);
		}
		else
		{
			const RGBColor textColor = { 0x8888, 0x8888, 0x8888 };
			RGBForeColor(&textColor);
			TextMode(srcOr);
		}
		_DrawVCenteredText(inInfo.title, inInfo.titleSize, titleRect, titleRect.left);
		TextMode(srcOr);
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void _DrawSimplePixMap(const SSimplePixMap& inPix, const Rect& inBounds)
{
	const RGBColor rgbWhiteColor 	= { 0xFFFF, 0xFFFF, 0xFFFF };
	const RGBColor rgbBlackColor	= { 0x0000, 0x0000, 0x0000 };
	const long kBlackAndWhiteCLUT[]	= { 0x0000768F, 0x00000001, 0x0000FFFF, 0xFFFFFFFF, 0x00010000, 0x00000000 };
	CTabPtr pBWColorTab = (CTabPtr)kBlackAndWhiteCLUT;
	GrafPtr curPort;
	PixMap pm;
	CTabHandle hColorTab = nil;
	CTabHandle disposeColorTab = nil;
	RGBColor saveBackColor;
	
	if (inPix.data == nil) 
		return;

	GetPort(&curPort);
	GetBackColor(&saveBackColor);
	RGBBackColor(&rgbWhiteColor);
	RGBForeColor(&rgbBlackColor);
	
	if (inPix.colortab == nil)
	{
		if (inPix.depth == 1)
			hColorTab = &pBWColorTab;
		else
		{
			hColorTab = disposeColorTab = GetCTable(inPix.depth);
			if (hColorTab == nil) goto abort;
		}
	}
	else
	{
		// I hope this doesn't break, but ATB shows CopyBits doesn't call HLock
		hColorTab = (CTabHandle)&inPix.colortab;
	}
	
	pm.baseAddr = (Ptr)inPix.data;
	pm.rowBytes = inPix.rowBytes | 0x8000;	// ((((inPix.width * inPix.depth) + 15) >> 4) << 1) | 0x8000;
	pm.bounds.top = 0;
	pm.bounds.left = 0;
	pm.bounds.bottom = inPix.height;
	pm.bounds.right = inPix.width;
	pm.pmVersion = 0;
	pm.packType = 0;
	pm.packSize = 0;
	pm.hRes = 72;
	pm.vRes = 72;
	pm.pixelType = 0;
	pm.pixelSize = inPix.depth;
	pm.cmpCount = 1;
	pm.cmpSize = inPix.depth;
#if TARGET_API_MAC_CARBON
	pm.pixelFormat = 0;
#else
	pm.planeBytes = 0;
#endif
	pm.pmTable = hColorTab;
#if TARGET_API_MAC_CARBON
	pm.pmExt = nil;
#else
	pm.pmReserved = 0;
#endif
	
	if (inPix.mask == nil)
	{
	#if TARGET_API_MAC_CARBON
		::CopyBits((BitMap *)&pm, ::GetPortBitMapForCopyBits(curPort), &pm.bounds, &inBounds, srcCopy, nil);
	#else
		::CopyBits((BitMap *)&pm, &curPort->portBits, &pm.bounds, &inBounds, srcCopy, nil);
	#endif
	}
	else
	{
		BitMap bm;
		bm.baseAddr = (Ptr)inPix.mask;
		bm.rowBytes = ((inPix.width + 15) >> 4) << 1;
		bm.bounds.top = 0;
		bm.bounds.left = 0;
		bm.bounds.bottom = inPix.height;
		bm.bounds.right = inPix.width;
	
	#if TARGET_API_MAC_CARBON
		::CopyMask((BitMap *)&pm, &bm, ::GetPortBitMapForCopyBits(curPort), &pm.bounds, &bm.bounds, &inBounds);
	#else
		::CopyMask((BitMap *)&pm, &bm, &curPort->portBits, &pm.bounds, &bm.bounds, &inBounds);	
	#endif
	}
	
abort:
	RGBBackColor(&saveBackColor);
	if (disposeColorTab)
		DisposeCTable(disposeColorTab);
}

static void _CalcRects(Rect& inBaseRect, Rect *outCheckRect, Rect *outTitleRect)
{
	const Int16 kCheckBoxSize = 12;
	
	Rect r;
	
	if (outCheckRect)
	{
		r = inBaseRect;
		r.left += 2;
		r.right = r.left + kCheckBoxSize;
		r.top = r.top + (((r.bottom - r.top)/2) - (kCheckBoxSize/2));
		r.bottom = r.top + kCheckBoxSize;
		
		*outCheckRect = r;
	}
	
	if (outTitleRect)
	{
		r = inBaseRect;
		r.left += 2 + kCheckBoxSize + 4;
		
		*outTitleRect = r;
	}
}

void _DrawVCenteredText(const void *inText, Uint32 inLength, Rect& r, Int16 offset)
{
	if (inText && inLength > 0)
	{
		FontInfo fi;
		Int16 v;
	
		GetFontInfo(&fi);
		v = (((r.top + r.bottom) - (fi.descent + fi.ascent)) / 2) + fi.ascent;
		MoveTo(offset, v);
		DrawText((Ptr)inText, 0, inLength);
	}
}






#endif /* MACINTOSH */
