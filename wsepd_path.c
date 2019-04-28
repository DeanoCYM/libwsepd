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
#include <assert.h>

#include "wsepd_path.h"

struct Node {
    struct Coordinate *px;
    struct Node *Next;
    struct Node *Prev;
};

struct Path {
    size_t xmax;		
    size_t ymax;		
    size_t length;
    size_t journey_position;
    struct Node *Head;
    struct Node *Tail;
    struct Node *JourneyHead;
};

/**
   Static Functions
**/

static void Node_destroy(struct Node *Node);

static void
Node_destroy(struct Node *Node)
{
    assert(Node && Node->px);

    free(Node->px); Node->px = NULL;
    free(Node); Node = NULL;
    return;
}
    

/**
   Interface Functions
**/

/* Dynamically allocates memory for an empty Path structure */
struct Path *
PATH_create(size_t width, size_t height)
{
    struct Path * List = malloc(sizeof *List);
    if (NULL == List) {
	log_err("Memory error");
	return NULL;
    }

    List->xmax = width - 1;
    List->ymax = height - 1;
    List->length = 0;
    List->journey_position = 0;
    List->Tail = NULL;
    List->Head = NULL;
    List->JourneyHead = NULL;

    return List;
}

/* Frees each Node in the List and then frees the Path structure
   itself */
void
PATH_destroy(struct Path *List)
{
    assert(List);

    log_debug("Destroying path list with %zu coordinate node(s).",
	      List->length);

    if (List->length > 0)
	PATH_clear_coordinates(List);

    List->Head = NULL;
    List->Tail = NULL;
    free(List);
    List = NULL;
	
    return;
}

/* Adds a Cooridinate structure to the end of the Path list. */
int
PATH_append_coordinate(struct Path *List, size_t x, size_t y)
{
    assert(List);

    if (x > List->xmax || y > List->ymax) {
	log_err("Nodes exceed maximum dimensions.");
	return 1;
    }

    struct Node *New = malloc(sizeof *New);
    if (NULL == New) {
	log_err("Memory Error");
	return 1;
    }

    New->px = malloc(sizeof *New->px);
    if (NULL == New->px) {
	log_err("Memory Error");
	return 1;
    }    

    New->px->x = x;
    New->px->y = y;
    New->Next = NULL;

    if (0 == List->length) {
	New->Prev = NULL;
	List->Head = New;
	List->Tail = New;
	List->JourneyHead = New;
    } else {
	List->Tail->Next = New;
	New->Prev = List->Tail;
	List->Tail = New;
    }
	
    ++List->length;
    log_debug("Appended coordinate (%zu,%zu).",
	      List->Tail->px->x, List->Tail->px->y);

    return 0;
}

/* Deletes the Nth node, repairing the linked list appropriately. */
void
PATH_remove_coordinate(struct Path *List, size_t N)
{
    assert(List && List->Head);

    if (N >= List->length) {
	errno = EINVAL;
	log_err("Cant remove node at position %zu of %zu nodes.\n\t",
		N, List->length);
    }
    
    size_t n;
    struct Node *Target = List->Head;
    for (n = 1; n < N; ++n) {
	Target = Target->Next;
    }

    Target->Prev->Next = Target->Next;
    Target->Next->Prev = Target->Prev;
    Node_destroy(Target);

    return;
}

/* Removes all Coordinate nodes, and frees associated memory, within
   the linked list structure.  */
void
PATH_clear_coordinates(struct Path *List)
{
    assert(List);

    if (NULL == List->Head) {
	errno = ECANCELED;
	log_warn("Path list already empty");
	return;
    }

    struct Node *Temp;
    while(List->length > 0) {
	Temp = List->Head->Next;
	Node_destroy(List->Head);
	List->Head = Temp;
	--List->length;
    }

    List->Head = NULL;
    List->Tail = NULL;
    List->JourneyHead = NULL;
    List->journey_position = 0;

    if (List->length > 0) {
	log_warn("Memory leak! %zu coordinate nodes not freed.",
		 List->length);
    }

    return;
}

/* Returns the number of nodes in the list */
size_t
PATH_get_length(struct Path *List)
{
    assert(List);
    return List->length;
}

/* Returns the current position when traversing the list */
size_t
PATH_get_position(struct Path *List)
{
    assert(List);
    return List->journey_position;
}

/* Traverses the linked list one node and returns the associated
   coordinate. The jouney position field is incremented to keep track
   of the current location */
struct Coordinate *
PATH_get_next_coordinate(struct Path *List)
{
    assert(List);

    if (List->journey_position > List->length) {
	errno = EINVAL;
	log_err("End of path.");
	return NULL;
    }

    struct Coordinate *px = List->JourneyHead->px;
    List->JourneyHead = List->JourneyHead->Next;
    ++List->journey_position;

    return px;
}
