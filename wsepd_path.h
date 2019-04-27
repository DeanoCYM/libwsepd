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

typedef struct Path * Path;

/* Dynamically allocates memory for a new Path object */
Path Path_create(size_t width, size_t height);

/* Appends coordinates to end of a Path */
int  Path_append_coordinate(Path List, size_t x, size_t y);

/* Frees all memory in the Path object */
void Path_destroy(Path List);
