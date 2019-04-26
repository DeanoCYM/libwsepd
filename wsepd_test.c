/* wsepd_test.c
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
 * Functions that test the interface of libwsepd.a
 * 
 */

#include <stdio.h>
#include <unistd.h>
#include <ert_log.h>
#include "libwsepd.h"

#define WIDTH  128
#define HEIGHT 296

int
main(int argc, char *argv[])
{
    log_debug("Testing %s (argc == %d).", argv[0], argc);

    EPD Display = EPD_create(WIDTH, HEIGHT);
    if (Display == NULL) {
    	log_debug("Failed to create object");
    	return 1;
    }
	
    /* Highlight the origin pixel */
    EPD_set_px(Display, 0, 0);
    EPD_refresh(Display);

    /* Draw lines through centre of display */
    for (size_t x = WIDTH/2,  y = 0; y < HEIGHT; ++y)
    	EPD_set_px(Display, x, y);
    for (size_t x = 0,  y = HEIGHT/2; x < WIDTH; ++x)
    	EPD_set_px(Display, x, y);
    EPD_refresh(Display);

    /* Draw a diagonal lines through centre */
    size_t from1[2] = { 0, 0 };
    size_t to1[2] = {WIDTH-1, HEIGHT-1};
    EPD_draw_line(Display, from1, to1);

    size_t from2[2] = { 0, HEIGHT-1 };
    size_t to2[2] = { WIDTH-1, 0 };
    EPD_draw_line(Display, from2, to2);

    EPD_refresh(Display);


    EPD_destroy(Display);
    return 0;
}

