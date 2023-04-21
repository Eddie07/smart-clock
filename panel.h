/*
 * linux/include/video/st7735fb.h -- FB driver for ST7735 LCD controller
 *
 * Copyright (C) 2011, Matt Porter
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 */

#define DRVNAME		"st7735fb"
#define WIDTH		160
#define HEIGHT		128
#define BPP		16


#include <asm/io.h>
#include "font48.h"
#include "font24.h"

struct BitmapFont
  {
  uint8_t *table;
  int width;
  int height; 
  }; 

const struct BitmapFont font[]={
				{font24_table, 17, 24 },
				{font48_table, 24, 48 },
};



/* Init script function */
struct st7735_function {
	uint8_t cmd;
	uint8_t data;
};

/* Init script commands */
enum st7735_cmd {
	ST7735_START,
	ST7735_END,
	ST7735_CMD,
	ST7735_DATA,
	ST7735_DELAY
};


struct st7735fb_par {
	struct spi_device *spi;
	uint8_t *spi_writebuf;
	size_t vmem_size;
        char __iomem *screen_base;	/* Virtual address */
	uint8_t rst;
	uint8_t dc;
} par;


/* colors */
const uint32_t WHITE_COLOR=0xffffffu;//white
const uint32_t RED_COLOR=0xff0000u;//red
const uint32_t GREEN_COLOR=0x008000u;//green
const uint32_t BLUE_COLOR=0x0000ffu;//blue
const uint32_t YELLOW_COLOR=0xffff00u;//yellow 


/* ST7735 Commands */
#define ST7735_NOP	0x0
#define ST7735_SWRESET	0x01
#define ST7735_RDDID	0x04
#define ST7735_RDDST	0x09
#define ST7735_SLPIN	0x10
#define ST7735_SLPOUT	0x11
#define ST7735_PTLON	0x12
#define ST7735_NORON	0x13
#define ST7735_INVOFF	0x20
#define ST7735_INVON	0x21
#define ST7735_DISPOFF	0x28
#define ST7735_DISPON	0x29
#define ST7735_CASET	0x2A
#define ST7735_RASET	0x2B
#define ST7735_RAMWR	0x2C
#define ST7735_RAMRD	0x2E
#define ST7735_COLMOD	0x3A
#define ST7735_MADCTL	0x36
#define ST7735_FRMCTR1	0xB1
#define ST7735_FRMCTR2	0xB2
#define ST7735_FRMCTR3	0xB3
#define ST7735_INVCTR	0xB4
#define ST7735_DISSET5	0xB6
#define ST7735_PWCTR1	0xC0
#define ST7735_PWCTR2	0xC1
#define ST7735_PWCTR3	0xC2
#define ST7735_PWCTR4	0xC3
#define ST7735_PWCTR5	0xC4
#define ST7735_VMCTR1	0xC5
#define ST7735_RDID1	0xDA
#define ST7735_RDID2	0xDB
#define ST7735_RDID3	0xDC
#define ST7735_RDID4	0xDD
#define ST7735_GMCTRP1	0xE0
#define ST7735_GMCTRN1	0xE1
#define ST7735_PWCTR6	0xFC
#define ST7735_MADCTL_MY 0x80
#define ST7735_MADCTL_MX 0x40
#define ST7735_MADCTL_MV 0x20
#define ST7735_MADCTL_ML 0x10
#define ST7735_MADCTL_RGB 0x00

static struct st7735_function st7735_cfg[] = {
	{ ST7735_START, ST7735_START},
	/* cfg script start */
	{ ST7735_CMD, ST7735_SWRESET},
	{ ST7735_DELAY, 150},
	{ ST7735_CMD, ST7735_SLPOUT},
	{ ST7735_DELAY, 255},
	{ ST7735_CMD, ST7735_FRMCTR1},
	{ ST7735_DATA, 0x01},
	{ ST7735_DATA, 0x2c},
	{ ST7735_DATA, 0x2d},
	{ ST7735_CMD, ST7735_FRMCTR2},
	{ ST7735_DATA, 0x01},
	{ ST7735_DATA, 0x2c},
	{ ST7735_DATA, 0x2d},
	{ ST7735_CMD, ST7735_FRMCTR3},
	{ ST7735_DATA, 0x01},
	{ ST7735_DATA, 0x2c},
	{ ST7735_DATA, 0x2d},
	{ ST7735_DATA, 0x01},
	{ ST7735_DATA, 0x2c},
	{ ST7735_DATA, 0x2d},
	{ ST7735_CMD, ST7735_INVCTR},
	{ ST7735_DATA, 0x07},
	{ ST7735_CMD, ST7735_PWCTR1},
	{ ST7735_DATA, 0xa2},
	{ ST7735_DATA, 0x02},
	{ ST7735_DATA, 0x84},
	{ ST7735_CMD, ST7735_PWCTR2},
	{ ST7735_DATA, 0xc5},
	{ ST7735_CMD, ST7735_PWCTR3},
	{ ST7735_DATA, 0x0a},
	{ ST7735_DATA, 0x00},
	{ ST7735_CMD, ST7735_PWCTR4},
	{ ST7735_DATA, 0x8a},
	{ ST7735_DATA, 0x2a},
	{ ST7735_CMD, ST7735_PWCTR5},
	{ ST7735_DATA, 0x8a},
	{ ST7735_DATA, 0xee},
	{ ST7735_CMD, ST7735_VMCTR1},
	{ ST7735_DATA, 0x0e},
	{ ST7735_CMD, ST7735_INVOFF},
	{ ST7735_CMD, ST7735_MADCTL},
	{ ST7735_DATA, 0xc0},
	{ ST7735_CMD, ST7735_COLMOD},
	{ ST7735_DATA, 0x05},
	{ ST7735_CMD, ST7735_CASET},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, WIDTH - 1 },
	{ ST7735_CMD, ST7735_RASET},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x00 },
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, HEIGHT - 1 },
	{ ST7735_CMD, ST7735_GMCTRP1},
	{ ST7735_DATA, 0x02},
	{ ST7735_DATA, 0x1c},
	{ ST7735_DATA, 0x07},
	{ ST7735_DATA, 0x12},
	{ ST7735_DATA, 0x37},
	{ ST7735_DATA, 0x32},
	{ ST7735_DATA, 0x29},
	{ ST7735_DATA, 0x2d},
	{ ST7735_DATA, 0x29},
	{ ST7735_DATA, 0x25},
	{ ST7735_DATA, 0x2b},
	{ ST7735_DATA, 0x39},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x01},
	{ ST7735_DATA, 0x03},
	{ ST7735_DATA, 0x10},
	{ ST7735_CMD, ST7735_GMCTRN1},
	{ ST7735_DATA, 0x03},
	{ ST7735_DATA, 0x1d},
	{ ST7735_DATA, 0x07},
	{ ST7735_DATA, 0x06},
	{ ST7735_DATA, 0x2e},
	{ ST7735_DATA, 0x2c},
	{ ST7735_DATA, 0x29},
	{ ST7735_DATA, 0x2d},
	{ ST7735_DATA, 0x2e},
	{ ST7735_DATA, 0x2e},
	{ ST7735_DATA, 0x37},
	{ ST7735_DATA, 0x3f},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x02},
	{ ST7735_DATA, 0x10},
	{ ST7735_CMD, ST7735_NORON},
	{ ST7735_DELAY, 10},
	/* rotate landscape (90) */	
	{ ST7735_CMD, ST7735_MADCTL},
	{ ST7735_DATA, ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_RGB},
	/* powering on the display */
	{ ST7735_CMD, ST7735_DISPON},
	/* cfg script end */
	{ ST7735_END, ST7735_END}	
};



