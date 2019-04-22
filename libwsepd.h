/* libwsepd.h
 *
 * This file is part of libwsepd.
 *
 * Copyright (C) 2019 Ellis Rhys Thomas
 * 
 * libwsepd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libwsepd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with libwsepd.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Description:
 * Library for Waveshare 2.9" e-ink screen using the wiringPi
 * backend. Lookup Tables and Derived from the Waveshare example code,
 * which can be found on github below:
 * 
 * https://github.com/waveshare/e-Paper
 *
 * Plans to include more waveshare displays in the future.
 */

#ifndef LIBWSEPD_H
#define LIBWSEPD_H

#include <stdint.h>

/* Screen display setting constants */
enum ROTATION { NONE = 0, CLOCKWISE = 90, FLIP = 180, ANTICLOCKWISE = 270 };
enum DISPLAY_COLOUR { BLACK = 0x00, WHITE = 0xFF };
enum MIRROR { MIRROR_FALSE, MIRROR_TRUE };
enum CLEAR { CLEAR_FLASE, CLEAR_TRUE };

typedef struct Epd *EPD;

/* Electrionic Paper Display object */
EPD EPD_create(size_t width, size_t height);
void EPD_destroy(EPD Display);
void EPD_sleep(EPD Display);

/* Manipulate E-paper display object properties */
void EPD_set_rotation(EPD Display, enum ROTATION value);
enum ROTATION EPD_get_rotation(EPD Display);

void EPD_set_fgcolour(EPD Display, enum DISPLAY_COLOUR value);
enum DISPLAY_COLOUR EPD_get_colour(EPD Display);

void EPD_set_mirror(EPD Display, enum MIRROR value);
enum MIRROR EPD_get_mirror(EPD Display);

/* Image display and manipulation */
uint8_t *EPD_get_bmp(EPD Display);
void EPD_draw_point(EPD Display, uint16_t x, uint16_t y);
void EPD_print_bmp(EPD Display);
int EPD_refresh(EPD Display);
void EPD_clear(EPD Display);
/* Send e-paer display module into deep sleep mode */

#endif /* LIBWSEPD_H */
