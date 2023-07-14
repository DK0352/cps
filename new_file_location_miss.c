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

// This function is used to concatenate name of a file from the main location onto a directory of a new location. It is called because the file from the one location is missing in the other location.
// It can be used to invert location in case of adding a file to a surplus list, so the file is read from the destination directory, and written to a source directory.
char *new_file_location_miss(DListElmt *main_location, DList_of_lists *new_location, DList *insert_to)
{
	int size1, size2, size3;
	char *name, *dir_location;
	mode_t st_mode;
	long size;
	char *final_path;

	size1 = size2 = size3 = 0;

	name = strdup(main_location->name);
	if (name == NULL) {
		printf("new_file_location_miss(): strdup() error 1. exiting.\n");
		exit(1);
	}
	st_mode = main_location->st_mode;
	size = main_location->size;
	dir_location = strdup(main_location->dir_location);
	if (dir_location == NULL) {
		printf("new_file_location_miss(): strdup error 2. exiting.\n");
		exit(1);
	}
	size1 = strlen(new_location->dir_location);
	size2 = strlen(main_location->name);
	size3 = size1 + size2 + 2;
	final_path = malloc(size3);
	if (final_path == NULL) {
		printf("new_file_location_miss(): malloc error 1. exiting.\n");
		exit(1);
	}
	strcpy(final_path,new_location->dir_location);
	strcat(final_path,"/");
	strcat(final_path,main_location->name);

	dlist_ins_next(insert_to,insert_to->tail,name,st_mode,size,dir_location,0,final_path);

	return final_path;
}
