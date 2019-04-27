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
 *
 * Library for Waveshare 2.9" e-ink screen using the
 * wiringPi backend.
 *
 * Plans to include more waveshare displays in the future.
 */

#ifndef LIBWSEPD_H
#define LIBWSEPD_H

#include <stdint.h>

/* Screen display setting constants */
enum FOREGROUND_COLOUR { BLACK = 0x00, WHITE = 0xFF };
enum WRITE_MODE { TOGGLEMODE, FGMODE, BGMODE };

typedef struct Epd * EPD;

/* Electrionic Paper Display object */
EPD EPD_create(size_t width, size_t height);
void EPD_destroy(EPD Display);
void EPD_sleep(EPD Display);

/* Get/Set EPD properties */
void EPD_set_fgcolour(EPD Display, enum FOREGROUND_COLOUR value);
void EPD_set_write_mode(EPD Display, enum WRITE_MODE value);

enum WRITE_MODE EPD_get_write_mode(EPD Display);
enum FOREGROUND_COLOUR EPD_get_colour(EPD Display);
int EPD_get_poweron(EPD Display);
int EPD_get_width(EPD Display);
int EPD_get_height(EPD Display);

/* Image display and manipulation */
void EPD_set_px(EPD Display, size_t x, size_t y);
int EPD_draw_line(EPD Display, size_t *from, size_t *to);
int EPD_refresh(EPD Display);
int EPD_clear(EPD Display);

/* Debugging only */
void EPD_print_bmp(EPD Display);
uint8_t *EPD_get_bmp(EPD Display);

#endif /* LIBWSEPD_H */
