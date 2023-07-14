/*
    cps - a program that copies and synchronizes files and directories
    Copyright (C) 2023  Danilo Kovljanić

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "main.h"
#include "dlist.h"

/* dlist_init */
void dlist_init(DList *list) 
{
	list->num = 0;
	list->files_size = 0;
	list->head = NULL;
	list->tail = NULL;

	return;
}

/* dlist_destroy */
void dlist_destroy(DList *list) {
	char 		*dirname, *dir_location, *new_location;

	while (list->num > 0) {
		if (dlist_remove(list, list->tail, &dirname, &dir_location, &new_location) == 0) {
			if (dirname != NULL)
				free(dirname);
			if (dir_location != NULL)
				free(dir_location);
			if (new_location != NULL)
				free(new_location);
		}
	}

	/* no operations are allowed now, but clear the structure as a precaution */
	memset(list, 0, sizeof(DList));

	return;
}

/* dlist_ins_next */
int dlist_ins_next(DList *list, DListElmt *element, char *name, mode_t perm, long size, char *dir_location, int match, char *new_location) {

	DListElmt	*new_element;

	/* do not allow a NULL element unless the list is empty */
	if (element == NULL && list->num != 0)
		return -1;

	/* allocate storage for the element */
	if ((new_element = (DListElmt *)malloc(sizeof(DListElmt))) == NULL)
		return -1;

	/* insert the new element into the list */
	new_element->name = name;
	new_element->st_mode = perm;
	new_element->size = size;
	new_element->dir_location = dir_location;
	new_element->match = match;
	new_element->new_location = new_location;

	if (list->num == 0) {
		/* handle insertation when the list is empty */
		list->head = new_element;
		list->head->prev = NULL;
		list->head->next = NULL;
		list->tail = new_element;
	}
	else {
		/* handle insertation when the list is not empty */
		new_element->next = element->next;
		new_element->prev = element;

		if (element->next == NULL)
			list->tail = new_element;
		else
			element->next->prev = new_element;

		element->next = new_element;
	}

	/* adjust the size of the list to account for the inserted element */
	list->num++;
	list->files_size += (long) size;

	return 0;
}

/* dlist_remove */
int dlist_remove(DList *list, DListElmt *element, char **name, char **dir_location, char **new_location)
{
	/* do not allow a NULL element or removal from an empty list */
	if (element == NULL || list->num == 0)
		return -1;

	/* remove the element from the list */
	*name = element->name;
	*dir_location = element->dir_location;
	*new_location = element->new_location;

	if (element == list->head) {
		/* handle removal from the head of the list */
		list->head = element->next;
		if (list->head == NULL)
			list->tail = NULL;
		else
			element->next->prev = NULL;
	}
	else {
		/* handle removal from other than the head of the list */
		element->prev->next = element->next;
		if (element->next == NULL)
			list->tail = element->prev;
		else
			element->next->prev = element->prev;
	}

	free(element);
	list->num--;

	return 0;
}
