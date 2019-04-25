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
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <ert_log.h>

#include "libwsepd.h"
#include "wsepd_signal.h"
#include "waveshare2.9.h"

#define SPI_CLK_HZ 32000000	/* SPI clock speed (Hz) */
#define PI_CHANNEL 0		/* RPi has two channels */
#define RST_DELAY_MS 200	/* GPIO reset time delay (ms) */
#define BUSY_DELAY_MS 100	/* GPIO busy wait time (ms) */

/* GPIO pins in BCM numbering format, named by connected interface on
   e-paper module */
enum BCM_EPD_PINS
    { RST_PIN  = 17, DC_PIN   = 25,
      CS_PIN   =  8, BUSY_PIN = 24 };

/* GPIO output level (typically 0V low, 3.3V high)  */
enum GPIO_OUTPUT_LEVEL { GPIO_LOW, GPIO_HIGH };

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
    enum MIRROR mirror;
    enum ROTATION rotation;
    enum DISPLAY_COLOUR colour;
    enum CLEAR clear;
};

/**
 ** Static Functions
 **/

/* Epd <-> RPi communications*/
static int spi_comms(int channel, uint8_t *buf, int len);
static int send_command_byte(enum EPD_COMMANDS command);
static int send_data_byte(uint8_t data);

/* Device initialisation */
static int init_gpio(void);
static int init_epd(struct Epd *Display);

/* EPD commands */
static void set_display_window(struct Epd *Display, size_t *sizes);
static void set_cursor(uint16_t x, uint16_t y);
static int wait_while_busy(void);
static int load_display_from_ram(void);
static void reset_epd(void);

/* Bitmap manipulation and application */
static int bitmap_alloc(struct Epd *Display);
static void bitmap_write_to_ram(struct Epd *Display);
static void bitmap_clear(struct Epd *Display);

/* Sends hex data buffer to e-paper module over the SPI
   interface. Returns non-zero on failure. Data in buffer will be
   overwritten by previous EPD buffer, if the module supports data
   output. */
static int
spi_comms(int channel, uint8_t *out_buf, int len)
{
    if (LOGLEVEL == 4) {
	fprintf(stderr, "%02x",out_buf[0]);
    }

    int spi_in = wiringPiSPIDataRW(channel, out_buf, len);
    if (spi_in < 0) {
	errno = EREMOTEIO;
	log_err("SPI I/O error.");
    }
    return (spi_in < 0) ? 1 : 0;
}

/* Send command (1 Byte) to the e-paper display module, returns
   non-zero in event of an SPI write failure. */
static int
send_command_byte(enum EPD_COMMANDS command)
{
    if (LOGLEVEL == 4) {
	fprintf(stderr, "\nSPICommand: 0x");
    }

    uint8_t command_byte = command & 0xFF;

    digitalWrite(DC_PIN, GPIO_LOW);
    digitalWrite(CS_PIN, GPIO_LOW);
    int rc = spi_comms(PI_CHANNEL, &command_byte, 1);
    digitalWrite(CS_PIN, GPIO_HIGH);



    return rc;
}

/* Send one byte of data to the e-paper display module, returns
   non-zero in event of an SPI write failure */
static int
send_data_byte(uint8_t data)
{
    digitalWrite(DC_PIN, GPIO_HIGH);
    digitalWrite(CS_PIN, GPIO_LOW);
    int rc = spi_comms(PI_CHANNEL, &data, 1);
    digitalWrite(CS_PIN, GPIO_HIGH);

    return rc;
}

/* Initialise GPIO and SPI on raspberry pi */
static int
init_gpio(void)
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

/* Initialise the EPD display using the waveshare EPD hex
   commands. This code is specific to the 2.9" HAT module */
static int
init_epd(struct Epd *Display)
{
    if (Display->poweron) {
	errno = EALREADY;
	//log_warn("Attempt made to initialise active device.");
	return 0;
    } else { 
	Display->poweron = 1;
    }

    /* Interrupts need to be blocked while device is active as leaving
       the device powered on for extended periods of time can cause
       damage */
    start_signal_handler();

    reset_epd();

    int rc = send_command_byte(DRIVER_OUTPUT_CONTROL);
    if (rc) goto out;
    rc = send_data_byte((Display->height - 1) & 0xFF);
    if (rc) goto out;
    rc = send_data_byte(((Display->height - 1) >> 8) & 0xFF);
    if (rc) goto out;
    rc = send_data_byte(0x00);
    if (rc) goto out;
    
    rc = send_command_byte(BOOSTER_SOFT_START_CONTROL);
    if (rc) goto out;
    rc = send_data_byte(0xD7);
    if (rc) goto out;
    rc = send_data_byte(0xD6);
    if (rc) goto out;
    rc = send_data_byte(0x9D);
    if (rc) goto out;

    rc = send_command_byte(WRITE_VCOM_REGISTER);
    if (rc) goto out;
    rc = send_data_byte(0xA8);
    if (rc) goto out;

    rc = send_command_byte(SET_DUMMY_LINE_PERIOD);
    if (rc) goto out;
    rc = send_data_byte(0x1A);
    if (rc) goto out;

    rc = send_command_byte(SET_GATE_TIME);
    if (rc) goto out;
    rc = send_data_byte(0x08);
    if (rc) goto out;

    rc = send_command_byte(BORDER_WAVEFORM_CONTROL);
    if (rc) goto out;
    rc = send_data_byte(0x03);
    if (rc) goto out;

    rc = send_command_byte(DATA_ENTRY_MODE_SETTING);
    if (rc) goto out;
    rc = send_data_byte(0x03);
    if (rc) goto out;

    rc = send_command_byte(WRITE_LUT_REGISTER);
    if (rc) goto out;
    for (int i = 0; i < 30; ++i) {// check cant send entire buffer
	rc = send_data_byte(lut_full_update[i]);
	if (rc) goto out;
    }

    check_signal_handler(Display);
    log_info("E-paper display initialised successfully.");
    
    return 0;
 out:
    errno = EREMOTEIO;
    log_err("Failed to initialise e-paper display module.");
    return 1;
}

/* Set the e-paper display window to the provided minimum and maximum
   ranges for the x and y coordinates.

   If sizes is provided it should be a pointers to the start of an
   array containing 4 integers representing:
   sizes[0] - minimum width in pixels
   sizes[1] - maximum height in pixels
   sizes[2] - minimum width in pixels
   sizes[3] - maximum height in pixels
       
   If size is NULL, the default maximum pixels are used from the
   display structure and the minimums are set to the origin.

   Returns non-zero on failure. */
static void
set_display_window(struct Epd *Display, size_t *sizes)
{
    uint16_t xmin, xmax, ymin, ymax;

    if (sizes == NULL) {		/* use default, max area */
	xmin = 0;
	xmax = Display->width;
	ymin = 0;
	ymax = Display->height;
    } else {			/* use provided sizes */
	xmin = sizes[0];
	xmax = sizes[1];
	ymin = sizes[2];
	ymax = sizes[3];
    }
    // try to send more than one byte at a time over SPI with
    // send_data_spi.
    //
    // May need to reverse order of bytes... or find out order wiringPi
    // sends data in.
    //
    // Determine why only one byte is sent for x address and 2 for y
    // address (is it just pixel width related?).
    //
    // Also determine what the point of the bit shift on width is
    // required for.
    send_command_byte(SET_RAM_X_ADDRESS_START_END_POSITION);
    send_data_byte((xmin >> 3) & 0xFF);
    send_data_byte((xmax >> 3) & 0xFF);

    send_command_byte(SET_RAM_Y_ADDRESS_START_END_POSITION);
    send_data_byte(ymin & 0xFF);
    send_data_byte((ymin >> 8) & 0xFF);
    send_data_byte(ymax & 0xFF);
    send_data_byte((ymax >> 8) & 0xFF);

    return;
}

/* Set the cursor position (typically run prior to writing image data
   to RAM) */
static void
set_cursor(uint16_t x, uint16_t y)
{
    send_command_byte(SET_RAM_X_ADDRESS_COUNTER);
    send_data_byte((x >> 3) & 0xFF);

    send_command_byte(SET_RAM_Y_ADDRESS_COUNTER);
    send_data_byte(y & 0xFF);
    send_data_byte((y >> 8) & 0xFF);

    return;
}

/* Wait until busy pin reads low. Return wait time (in ms) or -1 if
   the wait time was greater than 100x BUSY_DELAY_MS.  */
static int
wait_while_busy(void)
{
    int t = 0;
    while (digitalRead(BUSY_PIN) == GPIO_HIGH) {

	if (t > 100) {
	    errno = EBUSY;
	    log_err("Device not leaving busy state. Is power connected?");
	    return -1;
	}

	delay(BUSY_DELAY_MS);
	++t;
    }

   return t * BUSY_DELAY_MS;
}

/* Apply the bitmap in RAM to the e-paper display, returns 1 if busy
   line is held low for too long (see wait_while_busy). */
static int
load_display_from_ram(void)
{
    send_command_byte(DISPLAY_UPDATE_CONTROL_2);
    send_data_byte(0xC4);

    send_command_byte(MASTER_ACTIVATION);
    send_command_byte(TERMINATE_FRAME_READ_WRITE);

    if (wait_while_busy() < 0) {
	errno = EBUSY;
	log_err("Failed to load display from RAM.");
	return 1;
    }
    
    return 0;
}

/* Resets the e-paper display by stepping the reset pin low for
   RST_DELAY_MS */
void
reset_epd(void)
{
    digitalWrite(RST_PIN, GPIO_HIGH);
    delay(RST_DELAY_MS);

    digitalWrite(RST_PIN, GPIO_LOW);
    delay(RST_DELAY_MS);

    digitalWrite(RST_PIN, GPIO_HIGH);
    delay(RST_DELAY_MS);    

    return;
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
    if (init_gpio())
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
    if (init_epd(Display))
	goto out2;
    if (bitmap_alloc(Display))
	goto out2;

    //EPD_sleep(Display);

    /* Set some defaults */
    EPD_set_fgcolour(Display, BLACK);
    EPD_set_mirror(Display, MIRROR_FALSE);
    EPD_set_rotation(Display, NONE);

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

/* Set and get rotation field in the epd structure */
void
EPD_set_rotation(struct Epd *Display, enum ROTATION value)
{
    Display->rotation = value;
    log_info("Screen rotation set to %d degrees", Display->rotation);

    return;
}

enum ROTATION
EPD_get_rotation(struct Epd *Display)
{
    return Display->rotation;
}


/* Set and get the display foreground colour in the epd structure */
void
EPD_set_fgcolour(struct Epd *Display, enum DISPLAY_COLOUR value)
{
    Display->colour = value;

    if(Display->colour == WHITE) {
	log_info("Foreground colour set to white");
    } else {
	log_info("Foreground colour set to black");
    }
  
    return;
}

enum DISPLAY_COLOUR
EPD_get_colour(struct Epd *Display)
{
    return Display->colour;
}

/* Set and get the screen mirroring field in the epd structure */
void
EPD_set_mirror(struct Epd *Display, enum MIRROR value)
{
    Display->mirror = value;

    if (Display->mirror == MIRROR_TRUE) {
	log_info("Display mirroring active");
    } else {
	log_info("Display mirroring disabled");
    }

    return;
}

enum MIRROR
EPD_get_mirror(struct Epd *Display)
{
    return Display->mirror;
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

/* Set the pixels at (x, y) in the bitmap to the foreground colour */
void
EPD_draw_point(struct Epd *Display, size_t x, size_t y)
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

    *point ^= 0x80 >> (x % 8);
    
    return;
}

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

/* Apply transformations (according to flags), write the bitmap to ram
   and refresh the display.  */
int
EPD_refresh(struct Epd *Display)
{
    if (init_epd(Display)) {
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
    //EPD_sleep(Display);
    
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
