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

char *new_dir_location(DList_of_lists *main_location, DList_of_lists *new_location, DList *insert_to)
{
	extern struct options_menu options;

	int size1, size2, size3;
	char *dirname, *dir_location, *new_dir_location;
	mode_t perm;
	long size;
	char *final_path;
	time_t atime, mtime;

	atime = 0;
	mtime = 0;

	dirname = main_location->dirname;
	perm = main_location->st_mode;
	size = main_location->complete_dir_size;
	// location of a B directory to which an A directory from a main location will be copied
	dir_location = main_location->dir_location;
	new_dir_location = new_location->dir_location;
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

	if (options.time_mods == 0) {
		dlist_ins_next(insert_to,insert_to->tail,dirname,perm,size,dir_location,0,final_path,main_location->atime,main_location->mtime,main_location);
		insert_to->tail->tree_position = main_location;
	}
	else if (options.time_mods == 1) {
		if (options.preserve_a_time == 1)
			atime = main_location->atime;
		if (options.preserve_m_time == 1)
			mtime = main_location->mtime;
		dlist_ins_next(insert_to,insert_to->tail,dirname,perm,size,dir_location,0,final_path,main_location->atime,main_location->mtime,main_location);
		insert_to->tail->tree_position = main_location;
	}

	return final_path;
}
