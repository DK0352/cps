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
#include "data_copy_info.h" // benchmark

void init_to_zero(DList_of_lists *file_tree_element);

/* create top directories by using linked list od directories created using open_dirs() function, which is called from the main() for the top directories, and then by build_tree() 
 * function continously. build_tree() calls create_top_dirs just for the top directories, and then onwards create_dirs(). */
DList_of_lists *create_top_dirs(DList *list, DList_of_lists *file_tree_element)
{
	DList_of_lists 		*save_old_position, *alloc, *save_first_dir, *save_file_tree_top_dir;
	DListElmt		*dirlist_pointer;
	int a, limit;
	int top_dir_num;	// broj top direktorija
	extern struct Data_Copy_Info data_copy_info; // benchmark

	limit = list->num;
	dirlist_pointer = list->head;
	top_dir_num = 0;

	file_tree_element->down = NULL;
	save_file_tree_top_dir = file_tree_element;
	save_old_position = file_tree_element;

	for (a = 1; a <= limit; a++, dirlist_pointer = dirlist_pointer->next) {
		// first directory
		if (a == 1) {
			alloc = malloc(sizeof(DList_of_lists));
			if (alloc == NULL) {
				printf("error allocating memory - create_top_dirs(1)\n");
				exit(1);
			}
			data_copy_info.dlist_of_lists_num++;
			file_tree_element->next = alloc;
			file_tree_element->first_dir_in_chain = alloc;
			file_tree_element = file_tree_element->next;
			// file_tree_element is now the first directory 
			save_first_dir = file_tree_element;
			init_to_zero(file_tree_element);
			file_tree_element->up = NULL;
			file_tree_element->prev = save_old_position;
			file_tree_element->one_of_the_top_dirs = file_tree_element;
			file_tree_element->one_of_the_top_dirs_num = ++top_dir_num;
			file_tree_element->first_dir_in_chain = save_first_dir;
			file_tree_element->file_tree_top_dir = save_file_tree_top_dir;
			file_tree_element->this_is_top_dir = 1;
			file_tree_element->dirname = dirlist_pointer->name; /* using directory list created using open_dirs() function and creating dlist_of_lists file tree */
			file_tree_element->dir_location = dirlist_pointer->dir_location; /* just using pointers as all the allocated data will be destroyed by using dlist_destroy function in process of destroying dlist_of_lists */
			file_tree_element->st_mode = dirlist_pointer->st_mode;
			file_tree_element->atime = dirlist_pointer->atime;
			file_tree_element->mtime = dirlist_pointer->mtime;
			dirlist_pointer->tree_position = file_tree_element;
		}
		// just one directory
		if (limit == 1) {
			file_tree_element->last_dir = 1;
			file_tree_element->first_dir_in_chain->last_dir_in_chain = file_tree_element;
			file_tree_element->next = NULL;

			return file_tree_element;
		}
		else if (a < limit && a != 1) {
			save_old_position = file_tree_element;
			alloc = malloc(sizeof(DList_of_lists));
			if (alloc == NULL) {
				printf("error allocating memory - create_top_dirs(2)\n");
				exit(1);
			}
			data_copy_info.dlist_of_lists_num++;
			file_tree_element->next = alloc;
			file_tree_element = file_tree_element->next;
			init_to_zero(file_tree_element);
			file_tree_element->up = NULL;
			file_tree_element->prev = save_old_position;
			file_tree_element->one_of_the_top_dirs = file_tree_element;
			file_tree_element->one_of_the_top_dirs_num = ++top_dir_num;
			file_tree_element->file_tree_top_dir = save_file_tree_top_dir;
			file_tree_element->first_dir_in_chain = save_first_dir;
			file_tree_element->this_is_top_dir = 1;
			file_tree_element->dirname = dirlist_pointer->name;
			file_tree_element->dir_location = dirlist_pointer->dir_location;
			file_tree_element->st_mode = dirlist_pointer->st_mode;
			file_tree_element->atime = dirlist_pointer->atime;
			file_tree_element->mtime = dirlist_pointer->mtime;
			dirlist_pointer->tree_position = file_tree_element;

		}
		else if (a == limit && limit != 1) {
			save_old_position = file_tree_element;
			alloc = malloc(sizeof(DList_of_lists));
			if (alloc == NULL) {
				printf("error allocating memory - create_top_dirs(3)\n");
				exit(1);
			}
			data_copy_info.dlist_of_lists_num++;
			file_tree_element->next = alloc;
			file_tree_element = file_tree_element->next;
			init_to_zero(file_tree_element);
			file_tree_element->up = NULL;
			file_tree_element->next = NULL;
			file_tree_element->prev = save_old_position;
			file_tree_element->one_of_the_top_dirs = file_tree_element;
			file_tree_element->one_of_the_top_dirs_num = ++top_dir_num;
			file_tree_element->file_tree_top_dir = save_file_tree_top_dir;
			file_tree_element->first_dir_in_chain = save_first_dir;
			file_tree_element->this_is_top_dir = 1;
			file_tree_element->last_dir = 1;
			file_tree_element->dirname = dirlist_pointer->name;
			file_tree_element->dir_location = dirlist_pointer->dir_location;
			file_tree_element->st_mode = dirlist_pointer->st_mode;
			file_tree_element->atime = dirlist_pointer->atime;
			file_tree_element->mtime = dirlist_pointer->mtime;
			dirlist_pointer->tree_position = file_tree_element;
			save_old_position = file_tree_element;
			file_tree_element = file_tree_element->first_dir_in_chain;
			file_tree_element->last_dir_in_chain = save_old_position;
			file_tree_element->file_tree_top_dir->last_dir_in_chain = save_old_position;

			return file_tree_element;
		}
	} /* for loop */
}
/* size je dlist_num(directories), ubaci iza file_tree_element-a, dirlist_pointer je dlist_head(directories) */
DList_of_lists *create_dirs(DList *list, DList_of_lists *file_tree_element, DList_of_lists *file_tree_top_dir)
{
	DList_of_lists 		*save_old_position, *save_up_position, *alloc, *save_first_dir, *save_one_of_top_dirs;
	DListElmt		*dirlist_pointer;
	int a, limit;
	extern struct Data_Copy_Info data_copy_info; // benchmark

	if (list != NULL) {
		limit = list->num;
		dirlist_pointer = list->head;
		save_up_position = file_tree_element->up;
		save_first_dir = file_tree_element;
		file_tree_element->first_dir_in_chain = file_tree_element;
		
	}
	else {
		printf("create_dirs error: list == NULL!\n");
		exit(1);
	}
	for (a = 1; a <= limit; a++, dirlist_pointer = dirlist_pointer->next) {
		if (a == 1) {
			file_tree_element->one_of_the_top_dirs = save_up_position->one_of_the_top_dirs;
			file_tree_element->dirname = dirlist_pointer->name; 
			file_tree_element->dir_location = dirlist_pointer->dir_location;
			file_tree_element->st_mode = dirlist_pointer->st_mode;
			file_tree_element->atime = dirlist_pointer->atime;
			file_tree_element->mtime = dirlist_pointer->mtime;
			dirlist_pointer->tree_position = file_tree_element;
			if (limit == 1) {
				file_tree_element->last_dir = 1;
				file_tree_element->first_dir_in_chain->last_dir_in_chain = file_tree_element;
				file_tree_element->next = NULL;
				return file_tree_element;
			}
		}
		else if (a < limit && a != 1) {
			save_old_position = file_tree_element;
			alloc = malloc(sizeof(DList_of_lists));
			if (alloc == NULL) {
				printf("error allocating memory - create_dirs(2)\n");
				exit(1);
			}
			data_copy_info.dlist_of_lists_num++;
			file_tree_element->next = alloc;
			file_tree_element = file_tree_element->next;
			init_to_zero(file_tree_element);
			file_tree_element->up = save_up_position;
			file_tree_element->prev = save_old_position;
			file_tree_element->one_of_the_top_dirs = save_up_position->one_of_the_top_dirs;
			file_tree_element->first_dir_in_chain = save_first_dir;
			file_tree_element->dirname = dirlist_pointer->name;
			file_tree_element->dir_location = dirlist_pointer->dir_location;
			file_tree_element->st_mode = dirlist_pointer->st_mode;
			file_tree_element->atime = dirlist_pointer->atime;
			file_tree_element->mtime = dirlist_pointer->mtime;
			dirlist_pointer->tree_position = file_tree_element;
		}

		else if (a == limit && limit != 1) {
			save_old_position = file_tree_element;
			alloc = malloc(sizeof(DList_of_lists));
			if (alloc == NULL) {
				printf("error allocating memory - create_dirs(3)\n");
				exit(1);
			}
			data_copy_info.dlist_of_lists_num++;
			file_tree_element->next = alloc;
			file_tree_element = file_tree_element->next;
			init_to_zero(file_tree_element);
			file_tree_element->next = NULL;
			file_tree_element->up = save_up_position;
			file_tree_element->prev = save_old_position;
			file_tree_element->one_of_the_top_dirs = save_up_position->one_of_the_top_dirs;
			file_tree_element->first_dir_in_chain = save_first_dir;
			file_tree_element->last_dir = 1;
			file_tree_element->dirname = dirlist_pointer->name;
			file_tree_element->dir_location = dirlist_pointer->dir_location;
			file_tree_element->st_mode = dirlist_pointer->st_mode;
			file_tree_element->atime = dirlist_pointer->atime;
			file_tree_element->mtime = dirlist_pointer->mtime;
			dirlist_pointer->tree_position = file_tree_element;
			save_old_position = file_tree_element;
			file_tree_element = file_tree_element->first_dir_in_chain;
			file_tree_element->last_dir_in_chain = save_old_position;
			return file_tree_element;
		}
	} /* for loop */
}
