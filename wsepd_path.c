/* wsepd_path.c
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
 * Linked list containing x and y coordinates.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ert_log.h>

#include "wsepd_path.h"

#define FOREACH_COORDINATE_IN_PATH(_PATH)	\
    for (size_t _N = 0; _N < _PATH->length; ++_N)

struct Coordinate {
    size_t x;
    size_t y;
    struct Coordinate *Next;
    struct Coordinate *Prev;
};

struct Path {
    size_t xmax;		
    size_t ymax;		
    size_t length;
    struct Coordinate *Head;
    struct Coordinate *Tail;
};

struct Path *
Path_create(size_t width, size_t height)
{
    struct Path * List = malloc(sizeof *List);
    if (NULL == List) {
	log_err("Memory error");
	return NULL;
    } else {
	log_debug("Allocated new Path List.");
    }

    List->xmax = width - 1;
    List->ymax = height - 1;
    List->length = 0;
    List->Tail = NULL;
    List->Head = NULL;

    return List;
}

/* Adds Cooridinate to the end of the Path list. */
int
Path_append_coordinate(struct Path *List, size_t x, size_t y)
{
    if (x > List->xmax || y > List->ymax) {
	log_err("Coordinates exceed maximum dimensions.");
	return 1;
    }

    struct Coordinate *New = malloc(sizeof *New);
    if (NULL == New) {
	log_err("Memory Error");
	return 1;
    } else {
	log_debug("Allocated new coordiante (%zu,%zu).",
		  New->x, New->y);
    }

    New->x = x;
    New->y = y;
    New->Next = NULL;

    if (0 == List->length) {
	log_debug("Path List is empty, appending first Coordinate.");
	New->Prev = NULL;
	List->Head = New;
	List->Tail = New;
    } else {
	log_debug("Appending Cooridinate to end of Path list.");
	List->Tail->Next = New;
	New->Prev = List->Tail;
	List->Tail = New;
    }
	
    ++List->length;

    return 0;
}

/* Frees each Coordinate in the List and then frees the Path itself */
void
Path_destroy(struct Path *List)
{
    FOREACH_COORDINATE_IN_PATH(List) {
	List->Head = List->Head->Next;

	if (List->Head->Prev) {
	    free(List->Head->Prev);
	    List->Head->Prev = NULL;
	}

	--List->length;
    }
    
    if (List->length != 0)
	log_warn("Memory leak!");

    free(List);
    List = NULL;

    return;
}
