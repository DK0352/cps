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

char *new_dir_location(DList_of_lists *main_location, DList_of_lists *new_location, DList *insert_to)
{
	int size1, size2, size3;
	char *dirname, *dir_location, *new_dir_location;
	mode_t perm;
	long size;
	char *final_path;

	dirname = strdup(main_location->dirname);
	if (dirname == NULL) {
		printf("new_dir_location(): strdup() error 1. exiting.\n");
		exit(1);
	}
	perm = main_location->st_mode;
	size = main_location->complete_dir_size;
	// location of b directory to which a directory from main location will be concatenated
	dir_location = strdup(main_location->dir_location);
	if (dir_location == NULL) {
		printf("new_dir_location(): strdup error 2. exiting.\n");
		exit(1);
	}
	new_dir_location = strdup(new_location->dir_location);
	if (new_dir_location == NULL) {
		printf("new_dir_location(): strdup error 3. exiting.\n");
		exit(1);
	}
	size1 = strlen(new_dir_location);
	size2 = strlen(dirname);
	size3 = size1 + size2 + 2;
	final_path = malloc(size3);
	if (final_path == NULL) {
		printf("new_dir_location(): malloc error 1. exiting.\n");
		exit(1);
	}
	strcpy(final_path,new_dir_location);
	strcat(final_path,"/");
	strcat(final_path,dirname);

	dlist_ins_next(insert_to,insert_to->tail,dirname,perm,size,dir_location,0,final_path);

	return final_path;
}
