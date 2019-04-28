/* wsepd_path.h
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
 * Provides a 'Path' object containing a list of x and y coordinates.
 *
 */

#ifndef WSEPD_PATH_H
#define WSEPD_PATH_H

struct Coordinate {
    size_t x;
    size_t y;
};

typedef struct Path * PATH;

/* Dynamically allocates memory for a new Path object */
PATH PATH_create(size_t width, size_t height);

/* Appends coordinates to end of a Path */
int  PATH_append_coordinate(PATH List, size_t x, size_t y);

/* Clear all coordinates in the Path list */
void PATH_clear_coordinates(PATH List);

/* Remove a specific co-ordinate (1 based numbering) */
void PATH_remove_coordinate(PATH List, size_t N);
size_t PATH_get_length(PATH List);
size_t PATH_get_position(PATH List);
struct Coordinate *PATH_get_next_coordinate(PATH List);

/* Frees all memory in the Path object */
void PATH_destroy(PATH List);

#endif /* WSEPD_PATH_H */
