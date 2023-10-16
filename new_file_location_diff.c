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
#include "options.h"
extern struct options_menu options;

// This function is used when two files with the same name are found, but different size or type. It just adds a file to a list to copy. It can be used to invert location in case of adding the file
// to a surplus list, so the file is read from the destionation directory and written to the source directory
char *new_file_location_diff(DListElmt *main_location, DListElmt *new_location, DList *insert_to)
{
	int size1, size2, size3;
	char *name, *dir_location_1, *dir_location_2;
	mode_t st_mode;
	long size;
	time_t atime, mtime;

	atime = 0;
	mtime = 0;

	size1 = size2 = size3 = 0;

	name = strdup(main_location->name);
	if (name == NULL) {
		printf("new_file_location(): strdup() error 1. exiting.\n");
		exit(1);
	}
	st_mode = main_location->st_mode;
	size = main_location->size;

	dir_location_1 = strdup(main_location->dir_location);
	if (dir_location_1 == NULL) {
		printf("new_file_location(): strdup error 2. exiting.\n");
		exit(1);
	}
	dir_location_2 = strdup(new_location->dir_location);
	if (dir_location_2 == NULL) {
		printf("new_file_location(): strdup error 2. exiting.\n");
		exit(1);
	}
	if (options.time_mods == 0)
		dlist_ins_next(insert_to,insert_to->tail,name,st_mode,size,dir_location_1,0,dir_location_2,main_location->atime,main_location->mtime);
	else if (options.time_mods == 1) {
		if (options.preserve_a_time == 1)
			atime = main_location->atime;
		if (options.preserve_m_time == 1)
			mtime = main_location->mtime;
		dlist_ins_next(insert_to,insert_to->tail,name,st_mode,size,dir_location_1,0,dir_location_2,main_location->atime,main_location->mtime);
	}
	
	return dir_location_2;
}
