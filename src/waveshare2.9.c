/* waveshare2.9.c
 *
 * This file is part of libwsepd and contains works derived from the
 * waveshare example code avaliable at:
 * 
 * https://github.com/waveshare/e-Paper
 *
 * The right to sublicense this under the GPLv3 has been exercised,
 * the original permission and copyright notice is included below.
 *
 * Copyright (C) 2019 Ellis Rhys Thomas
 * Copyright (C) 2017 Waveshare
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * Implementation of device specific methods for the waveshare 2.9"
 * e-Paper display.
 * 
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ert_log.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

#include "waveshare2.9.h"
#include "libwsepd.h"

/**
   SPI Communication Methods
**/

/* Sends hex data buffer to e-paper module over the SPI
   interface. Returns non-zero on failure. Data in buffer will be
   overwritten by previous EPD buffer, if the module supports data
   output. */
int
spi_comms(int channel, uint8_t *out_buf, int len)
{
    if (SPILOG) {
	fprintf(stdout, "%02x",out_buf[0]);
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
int
send_command_byte(enum EPD_COMMANDS command)
{
    if (SPILOG) {
	fprintf(stdout, "\n[SPI] 0x");
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
int
send_data_byte(uint8_t data)
{
    digitalWrite(DC_PIN, GPIO_HIGH);
    digitalWrite(CS_PIN, GPIO_LOW);
    int rc = spi_comms(PI_CHANNEL, &data, 1);
    digitalWrite(CS_PIN, GPIO_HIGH);

    return rc;
}

/**
   Device Commands
**/

/* Initialise the EPD display using the waveshare EPD hex
   commands. This code is specific to the 2.9" HAT module */
int
init_epd(EPD Display)
{
    reset_epd();

    int rc = send_command_byte(DRIVER_OUTPUT_CONTROL);
    if (rc) goto out;
    rc = send_data_byte((EPD_get_height(Display) - 1) & 0xFF);
    if (rc) goto out;
    rc = send_data_byte(((EPD_get_height(Display) - 1) >> 8) & 0xFF);
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
void
set_display_window(EPD Display, size_t *sizes)
{
    uint16_t xmin, xmax, ymin, ymax;

    if (sizes == NULL) {		/* use default, max area */
	xmin = 0;
	xmax = EPD_get_width(Display);
	ymin = 0;
	ymax = EPD_get_height(Display);
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
void
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
int
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
int
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
