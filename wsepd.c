/* wsepd.c
 * 
 * This file is part of libwsepd.
 *
 * Copyright (C) 2019 Ellis Rhys Thomas
 * 
 * libwsepd is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libwsepd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with libwsepd.  If not, see <https://www.gnu.org/licenses/>.
 * 
 * Description:
 *
 * Implementation of libwsepd.a interface. See libwsepd.h for
 * interface documentation.
 * 
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <ert_log.h>
#include <math.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

#include "libwsepd.h"
#include "wsepd_signal.h"
#include "waveshare2.9.h"

/* A bitmap representing the e-paper dispay screen */
struct bitmap {
    uint8_t *buf;		      /* Pointer to 1D image bitmap buffer */
    size_t buflen;	      /* Total Length in bytes of 1D array  */
    size_t width;		      /* Theoretical width in bytes if 2D */
    /* Theoretical height is one byte per pixel (height in struct Epd) */
};

/* E-paper display object */
struct Epd {
    size_t width;
    size_t height;
    int poweron;
    struct bitmap bmp;
    enum FOREGROUND_COLOUR colour;
    enum WRITE_MODE write_mode;
};

/**
 ** Static Functions
 **/

/* Device initialisation */
static int initialise_gpio(void);
static int initialise_epd(struct Epd *Display);

/* Bitmap manipulation and application */
static int bitmap_alloc(struct Epd *Display);
static void bitmap_write_to_ram(struct Epd *Display);
static void bitmap_set_px(uint8_t *byte, uint8_t n);
static void bitmap_unset_px(uint8_t *byte, uint8_t n);
static void bitmap_flip_px(uint8_t *byte, uint8_t n);
static void bitmap_clear(struct Epd *Display);

/* Initialise GPIO and SPI on raspberry pi */
static int
initialise_gpio(void)
{
    wiringPiSetupGpio();	/* fatal on failure */
    switch (errno) {
    case EACCES:
    /* warning resets errno set by wiringPi */
	log_warn("Running without root privileges,\n\t"
		 "this may work dependant on hardware configuration.");
    case 0:
	break;
    default:
	log_warn("GPIO error");
    }
    
    /* GPIO operating modes (see page 9/26 in waveshare epd manual) */
    pinMode(RST_PIN, OUTPUT);
    pinMode(DC_PIN, OUTPUT);
    pinMode(CS_PIN, OUTPUT);
    pinMode(BUSY_PIN, INPUT);

    if (wiringPiSPISetup(PI_CHANNEL, SPI_CLK_HZ) == -1) {
	log_err("Failed to initialise SPI comms.");
	return 1;
    }

    log_info("GPIO initialised.");
    
    return 0;
}

/* Initialise the e-Paper display, blocking appropriate signals so
   that the device cannot be left in a dangerous powered state. */
static int
initialise_epd(struct Epd *Display)
{
    if (Display->poweron) {
	errno = EALREADY;
	log_warn("Attempt made to initialise powered device!");
	return 1;
    }

    /* Interrupts need to be blocked while device is active as leaving
       the device powered on for extended periods of time can cause
       damage */
    start_signal_handler();
    Display->poweron = 1;
    int rc = init_epd(Display);
    check_signal_handler(Display);

    return rc;
}

/* Stores image buffer large enough to store binary data for each
   pixel in the e-paper display in the e-paper display object. Returns
   0 on success or 1 on memory error. */
static int
bitmap_alloc(struct Epd *Display)
{
    /* A bit can characterise one pixel, so one byte can represent 8
       pixels across the width (x axis). This is multiplied by the
       number of pixels in the height (y axis) to determine the number of
       bytes required to describe the entire e-paper display area.  */

    Display->bmp.width = (Display->width % 8 == 0)
	? Display->width / 8
	: Display->width / 8 + 1;

    Display->bmp.buflen = Display->bmp.width * Display->height;

    Display->bmp.buf = calloc(Display->bmp.buflen, sizeof *Display->bmp.buf);
    if (Display->bmp.buf == NULL) {
	log_err("Memory error.");
	return 1;
    }
    log_debug("Allocated %zuB for bitmap buffer.",
	      (sizeof *Display->bmp.buf) * Display->bmp.buflen);

    return 0;
}

/* Write bitmap to e-paper RAM and refresh screen */
static void
bitmap_write_to_ram(struct Epd *Display)
{

    size_t addr = 0;		/* 1D array index (calc from 2D) */

    for (size_t y = 0; y < Display->height; ++y) {

	/* Set cursor at start of each new row */
	set_cursor(0, y);
	send_command_byte(WRITE_RAM);

	/* Send one row of byte data */
	for (size_t x = 0; x < Display->bmp.width; ++x) {
	    addr = (y * Display->bmp.width) + x; 
	    send_data_byte(Display->bmp.buf[addr]);
	}
    }

    if (LOGLEVEL == 3) {
	EPD_print_bmp(Display);
    }
  
    return;
}

/* Set specified bit number to 0 */
static void
bitmap_set_px(uint8_t *byte, uint8_t n)
{
    *byte &= 0x80 >> n;
    return;
}

/* Set sepecified bit number n to 0 */
static void
bitmap_unset_px(uint8_t *byte, uint8_t n)
{
    *byte &= ~(0x80 >> n);
    return;
}

/* Flip specified bit number */
static void
bitmap_flip_px(uint8_t *byte, uint8_t n)
{
    *byte ^= 0x80 >> n;
    return;
}

/* Set each bit in the bitmap to the background colour (background
   colour is the inverse of the draw colour stored in E-paper display
   object */
static void
bitmap_clear(struct Epd *Display)
{
    size_t i;

    for (i = 0; i < Display->bmp.buflen; ++i) 
	Display->bmp.buf[i] = (~Display->colour) & 0xFF;

    log_debug("Buffer cleared (%zuB of %zuB).",
	      i, Display->bmp.buflen);

    return;
}

/**
 ** Interface functions
 **/

/* Create an object representing the e-paper display */
struct Epd *
EPD_create(size_t width, size_t height)
{
    if (initialise_gpio())
	goto out1;

    struct Epd *Display = malloc(sizeof *Display);
    if (!Display) {
	log_err("Memory error.");
	goto out1;
    } 
    log_debug("Allocated %zuB for EPD object", sizeof *Display);


    Display->width = width;
    Display->height = height;
    Display->poweron = 0;

    if (create_signal_handler())
	goto out2;
    if (initialise_epd(Display))
	goto out2;
    if (bitmap_alloc(Display))
	goto out2;

    EPD_sleep(Display);

    /* Set some defaults */
    EPD_set_fgcolour(Display, BLACK);
    EPD_set_write_mode(Display, FGMODE);

    if (EPD_clear(Display))
	goto out3;

    delay(500);			/* wiringPi delay */

    return Display;
 out3:
    free(Display->bmp.buf);
 out2:
    free(Display);
 out1:
    errno = ECANCELED;
    log_err("Failed to create e-paper display object");
    return NULL;
}

/* Free all memory in e-paper display object and power down the
   device */
void
EPD_destroy(struct Epd *Display)
{
    if (!Display) {
	log_warn("Attempted to destroy invalid object");
	return;
    }

    EPD_sleep(Display);
    
    if (Display->bmp.buf != NULL) {
	free(Display->bmp.buf);
	Display->bmp.buf = NULL;
    } else {
	log_debug("No bitmap buffer to free");
    }

    if (Display) {
	free(Display);
	Display = NULL;
    } else {
	log_debug("No display object to free");
    }

    log_debug("Display object cleanup complete");
    
    return;
}
  
/* Send device into deep sleep */
void
EPD_sleep(struct Epd *Display)
{
    if (0 == Display->poweron) {
	log_debug("Display is already asleep, doing nothing.");
	return;
    }

    if (wait_while_busy() < 0) {
	errno = EBUSY;
	log_err("Failed to sleep device");
	return;
    }

    send_command_byte(DEEP_SLEEP_MODE);
    send_data_byte(0x01);

    check_signal_handler(Display);
    stop_signal_handler();

    Display->poweron = 0;
    log_info("E-paper display sleeping");

    return;
}

/* Toggle the pixel colour at (x, y) in the bitmap */
void
EPD_set_px(struct Epd *Display, size_t x, size_t y)
{
    if (x >= Display->width || y >= Display->height) {
	errno = EINVAL;
	log_err("Invalid coordinates, must be within %zupxW x %zupxH.",
		Display->width, Display->height);
	return;
    }

    /* Convert 2D coordinates into flat array index and obtain byte of
       interest (each byte contains the bitmap data for 8 pixels
       across the width). */
    size_t byte_addr = (Display->bmp.width * y) + (x / 8);
    uint8_t *point = Display->bmp.buf + byte_addr;

    switch (Display->write_mode) {

    case TOGGLEMODE:
	bitmap_flip_px(point, (x % 8) & 0xFF);
	break;

    case FGMODE:
	if (Display->colour == WHITE)
	    bitmap_set_px(point, (x % 8) & 0xFF);
	else
	    bitmap_unset_px(point, (x % 8) & 0xFF);
	break;

    case BGMODE:
	if (Display->colour == BLACK)
	    bitmap_set_px(point, (x % 8) & 0xFF);
	else
	    bitmap_unset_px(point, (x % 8) & 0xFF);
	break;

    default:			/* should not reach */
	errno = EINVAL;
	log_err("Invalid WRITE_MODE enum value in object!");
    }
    
    return;
}

/* Toggles all pixels in a straight line from (x,y) to (x,y) */
int
EPD_draw_line(struct Epd *Display, size_t *from, size_t *to)
{
    log_debug("Drawing line from (%zu,%zu) to (%zu,%zu).",
	      from[0], from[1], to[0], to[1]);

    if (from[0] >= Display->width     ||
	to[0]   >= Display->width     ||
	from[1] >= Display->height    ||
	to[1]   >= Display->height )     {

	errno = EINVAL;
	log_err("Coordinates too large for %zux%zu display.\n\t"
		"Note: origin is at (0,0) so maximum Npx -1.",
		  Display->width, Display->height);
	return 1;
    }

    float dx = (float)to[0] - from[0];
    float dy = (float)to[1] - from[1];
    log_debug("dx == %0.2f, dy == %0.2f", dx, dy);

    float m = dy / dx;
    float c = to[1] - (m * to[0]);
    log_debug("m == %0.2f, c == %0.2f", m, c);
    
    /* For maximum resolution, line should be calculated with respect
       to the greatest dimension */
    size_t x, y;
    if (Display->width > Display->height) {
	for (x =  fminf(from[0], to[0]);
	     x <= fmaxf(from[0], to[0]);
	     ++x) {
	    y = (size_t)roundf((m * x) + c);
	    EPD_set_px(Display, x, y);
	}
    } else {
	for (y =  fminf(from[1], to[1]);
	     y <= fmaxf(from[1], to[1]);
	     ++y) {
	    x = (size_t)roundf((y - c) / m );
	    EPD_set_px(Display, x, y);
	}
    }

    return 0;
}

/* Apply transformations (according to flags), write the bitmap to ram
   and refresh the display.  */
int
EPD_refresh(struct Epd *Display)
{
    if (initialise_epd(Display)) {
	errno = EREMOTEIO;
	goto out;
    }
	
    set_display_window(Display, NULL);
    bitmap_write_to_ram(Display);

    if (load_display_from_ram()) {
	errno = EBUSY;
	goto out;
    }

    delay(500);
    log_info("Display refreshed.");
    EPD_sleep(Display);
    
    return 0;
 out:
    log_err("Failed to refresh display.");
    return 1;
}

/* Wipe the bitmap and apply the background colour (inverse of
   fgcolour) to the display. Returns non zero if there is a problem
   refreshing the display.  */
int
EPD_clear(struct Epd *Display)
{
    /* For full screen usage, window display set from origin to furthest
       possible co-ordinate */

    bitmap_clear(Display);
    return EPD_refresh(Display);
}

/**
   Get and set methods
**/

/* Set and get the display foreground colour in the epd structure */
void
EPD_set_fgcolour(struct Epd *Display, enum FOREGROUND_COLOUR value)
{
    Display->colour = value;

    switch (Display->colour) {
    case BLACK: log_info("Foreground colour set to black");
	break;
    case WHITE: log_info("Foreground colour set to white");
	break;
    default:
	errno = EINVAL;
	log_err("Invalid display colour provided");
    }
  
    return;
}

enum FOREGROUND_COLOUR
EPD_get_colour(struct Epd *Display)
{
    return Display->colour;
}

/* Set and get the mode of writing to the bitmap */
void
EPD_set_write_mode(struct Epd *Display, enum WRITE_MODE value)
{
    Display->write_mode = value;
    
    switch (Display->write_mode) {
    case TOGGLEMODE: log_info("Write set to toggle.");
	break;
    case FGMODE: log_info("Write set to foreground colour.");
	break;
    case BGMODE: log_info("Write set to background colour.");
	break;
    default:
	errno = EINVAL;
	log_err("Invalid write mode provided.");
    }
    
    return;
}

enum WRITE_MODE
EPD_get_write_mode(struct Epd *Display)
{
    return Display->write_mode;
}

int
EPD_get_poweron(struct Epd *Display)
{
    return Display->poweron;
}

int
EPD_get_width(struct Epd *Display)
{
    return Display->width;
}

int
EPD_get_height(struct Epd *Display)
{
    return Display->height;
}

/**
   Debugging methods
 **/

/* Hex dump of image bitmap buffer to stdout */
void
EPD_print_bmp(struct Epd *Display)
{
    log_debug("Printing bitmap (%zu px W x %zu px H):",
	     Display->width, Display->height);
    
    size_t addr = 0;		/* 1D array index (calc from 2D) */

    for (size_t y = 0; y < Display->height; ++y) {

	/* Print column headers */
	if (0 == y) {
	    printf("Byte -> ");
	    for (size_t x = 0; x < Display->bmp.width; ++x)
		printf("%02zu ", x);
	    printf("\n");
	}

	/* Print row numbers */
	printf("%04zu 0x ", y);	

	/* Print row byte data */
	for (size_t x = 0; x < Display->bmp.width; ++x) {
	    addr = (y * Display->bmp.width) + x; 
	    printf("%02X ", Display->bmp.buf[addr]);
	}

	printf("\n");		/* end of row */
    }

    return;
}

/* Returns a pointer to the image bitmap, or null if one is not
   initialised. */
uint8_t *
EPD_get_bmp(struct Epd *Display)
{
    if (!Display->bmp.buf) {
	log_warn("Image bitmap does not appear to be initialised.");
    }

    return Display->bmp.buf;
}

