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
#include <stdlib.h>
#include <ert_log.h>

#include "libwsepd.h"
#include "wsepd_path.h"

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
	
    /* Highlight the original and terminal pixel */
    EPD_set_px(Display, 0, 0);
    EPD_set_px(Display, WIDTH-1, HEIGHT-1);

    /* Draw lines through centre of display */
    PATH Route = PATH_create(WIDTH, HEIGHT);
    PATH_append_coordinate(Route, WIDTH/2, 0);
    PATH_append_coordinate(Route, WIDTH/2, HEIGHT-1);
    EPD_draw_path(Display, Route);
    PATH_clear_coordinates(Route);
    PATH_append_coordinate(Route, 0, HEIGHT/2);
    PATH_append_coordinate(Route, WIDTH-1, HEIGHT/2);
    EPD_draw_path(Display, Route);
    PATH_clear_coordinates(Route);

    /* Draw line around outside */
    PATH_append_coordinate(Route, 0, 0);
    PATH_append_coordinate(Route, 0, HEIGHT-1);
    PATH_append_coordinate(Route, WIDTH-1, HEIGHT-1);
    PATH_append_coordinate(Route, WIDTH-1, 0);
    PATH_append_coordinate(Route, 0, 0);
    EPD_draw_path(Display, Route);
    PATH_clear_coordinates(Route);
    
    /* Draw and 8x8 box */
    PATH_append_coordinate(Route, 10, 10);
    PATH_append_coordinate(Route, 18, 10);
    PATH_append_coordinate(Route, 18, 18);
    PATH_append_coordinate(Route, 10, 18);
    PATH_append_coordinate(Route, 10, 10);
    EPD_draw_path(Display, Route);
    PATH_clear_coordinates(Route);

    /* Draw and 16x16 box */
    PATH_append_coordinate(Route, 10, 20);
    PATH_append_coordinate(Route, 10+16, 20);
    PATH_append_coordinate(Route, 10+16, 20+16);
    PATH_append_coordinate(Route, 10, 20+16);
    PATH_append_coordinate(Route, 10, 20);
    EPD_draw_path(Display, Route);

    EPD_refresh(Display);
    PATH_destroy(Route);
    EPD_destroy(Display);
    return 0;
}

