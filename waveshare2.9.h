/* waveshare2.9.h
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
 */

/* Waveshare EPD module commands */
enum EPD_COMMANDS
    { DRIVER_OUTPUT_CONTROL                  = 0x01,
      BOOSTER_SOFT_START_CONTROL             = 0x0C,
      GATE_SCAN_START_POSITION               = 0x0F,
      DEEP_SLEEP_MODE                        = 0x10,
      DATA_ENTRY_MODE_SETTING                = 0x11,
      SW_RESET                               = 0x12,
      TEMPERATURE_SENSOR_CONTROL             = 0x1A,
      MASTER_ACTIVATION                      = 0x20,
      DISPLAY_UPDATE_CONTROL_1               = 0x21,
      DISPLAY_UPDATE_CONTROL_2               = 0x22,
      WRITE_RAM                              = 0x24,
      WRITE_VCOM_REGISTER                    = 0x2C,
      WRITE_LUT_REGISTER                     = 0x32,
      SET_DUMMY_LINE_PERIOD                  = 0x3A,
      SET_GATE_TIME                          = 0x3B,
      BORDER_WAVEFORM_CONTROL                = 0x3C,
      SET_RAM_X_ADDRESS_START_END_POSITION   = 0x44,
      SET_RAM_Y_ADDRESS_START_END_POSITION   = 0x45,
      SET_RAM_X_ADDRESS_COUNTER              = 0x4E,
      SET_RAM_Y_ADDRESS_COUNTER              = 0x4F,
      TERMINATE_FRAME_READ_WRITE             = 0xFF };

/* Waveshare look up tables for module register */
static const uint8_t lut_full_update[] =
    { 0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22,
      0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88,
      0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51,
      0x35, 0x51, 0x51, 0x19, 0x01, 0x00 };

/* static const uint8_t lut_partial_update[] = */
/*   { 0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, */
/*     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, */
/*     0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12, */
/*     0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; */


