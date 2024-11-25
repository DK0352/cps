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
#include "data_copy_info.h"
#include "errors.h"
#include "options.h"

int loop_files(DList_of_lists *file_tree_element_a, DList_of_lists *file_tree_element_b);
int loop_dirs(DList_of_lists *file_tree_element_a, DList_of_lists *file_tree_element_b);
char *new_dir_location(DList_of_lists *main_location, DList_of_lists *new_location, DList *insert_to);

/* compares file trees of the source and destination directories */
int compare_trees(struct thread_struct *thread_data_a, struct thread_struct *thread_data_b)
{
	extern int			full_dir_write;
	extern int			no_files_and_dirs_a;
	extern int			no_files_and_dirs_b;
	extern int			no_dirs_a;
	extern int			no_dirs_b;

	extern struct Data_Copy_Info	data_copy_info;
	extern struct options_menu	options;

	DList_of_lists 			*file_tree_element_a, *file_tree_element_b, *top_location_a, *top_location_b, *location_a_down, *location_b_down;
	DList_of_lists			*start_a, *start_b;

	int				same_dir_num;
	int				dirlist_size_a, dirlist_size_b;

	/* this if/elses determine the situations in which either the source or destination directory is empty and difference in file size/number of the top directory */
	if (no_files_and_dirs_a == 1 && no_files_and_dirs_b == 1)
		return -1;
	else if (no_files_and_dirs_a == 0 && no_files_and_dirs_b == 1) {
		full_dir_write = 1;
		return 0;
	}
	else if (no_files_and_dirs_a == 1 && no_files_and_dirs_b == 0) {
		full_dir_write = 2;
		return 0;
	}

	same_dir_num = 0;
	top_location_a = thread_data_a->file_tree_top_dir;
	top_location_b = thread_data_b->file_tree_top_dir;

	if (top_location_a->directories != NULL) {
		dirlist_size_a = top_location_a->directories->num;
		file_tree_element_a = top_location_a->first_dir_in_chain;
		start_a = file_tree_element_a;
	}
	else
		dirlist_size_a = 0;

	if (top_location_b->directories != NULL) {
		dirlist_size_b = top_location_b->directories->num;
		file_tree_element_b = top_location_b->first_dir_in_chain;
		start_b = file_tree_element_b;
	}
	else
		dirlist_size_b = 0;

	if (options.ignore_symlinks != 1) {
		if ((top_location_a->file_num != 0 && top_location_b->file_num != 0 || top_location_a->files_size != top_location_b->files_size) ||
		top_location_a->sym_links_num != 0 && top_location_b->sym_links_num != 0 || top_location_a->sym_links_size != top_location_b->sym_links_size ) {
			loop_files(top_location_a, top_location_b);
		}
	}
	else if (options.ignore_symlinks == 1) {
		if (top_location_a->file_num != 0 && top_location_b->file_num != 0 || top_location_a->files_size != top_location_b->files_size)
			loop_files(top_location_a, top_location_b);
	}
	/* This for loop compares just the top directories, and if there are differences in size, it calls the loop_files() function to compare the difference in file size/number 
	 * or modification time, and loop_dirs for differences in subdirectory size/number. It increments sam_dir_num variable for each directory with the same name, and if it matches 
	 * the number of directories in each directory, the function returns. If not, function does additional comparsion and determines which directories are missing in the destination,
	 * or which directories are extraneous. */
	if (no_dirs_a != 1 && no_dirs_b != 1) {
		for (file_tree_element_a = top_location_a->first_dir_in_chain; file_tree_element_a != NULL; file_tree_element_a = file_tree_element_a->next) {
			for (file_tree_element_b = top_location_b->first_dir_in_chain; file_tree_element_b != NULL; file_tree_element_b = file_tree_element_b->next) {
				if (strcmp(file_tree_element_a->dirname,file_tree_element_b->dirname) == 0) {
					++same_dir_num;
					file_tree_element_a->found_dir_match = 1;
					file_tree_element_b->found_dir_match = 1;
					if (options.ignore_symlinks != 1) {
						if ((file_tree_element_a->file_num != 0 || file_tree_element_b->file_num != 0) || 
						(file_tree_element_a->sym_links_num != 0 || file_tree_element_a->sym_links_num != 0))
							loop_files(file_tree_element_a, file_tree_element_b);
					}
					else if (options.ignore_symlinks == 1) {
						if (file_tree_element_a->file_num != 0 || file_tree_element_b->file_num != 0)
							loop_files(file_tree_element_a, file_tree_element_b);
					}
					loop_dirs(file_tree_element_a, file_tree_element_b);
				} // if strcmp(dirname,dirname)
			} // for loop b
		} // for loop a
	
		/* determine missing or extraneous directories within the top directory */
	
		if (top_location_a->first_dir_in_chain != NULL)
			file_tree_element_a = top_location_a->first_dir_in_chain;
		else
			file_tree_element_a = top_location_a;
	
		if (top_location_b->first_dir_in_chain != NULL)
			file_tree_element_b = top_location_b->first_dir_in_chain;
		else
			file_tree_element_b = top_location_b;
	
		if (dirlist_size_a > same_dir_num && dirlist_size_b == same_dir_num) {
			while (file_tree_element_a != NULL) {
				if (file_tree_element_a->found_dir_match != 1) {
					if (data_copy_info.dirs_to_copy_list == NULL) {
						data_copy_info.dirs_to_copy_list = malloc(sizeof(DList));
						if (data_copy_info.dirs_to_copy_list != NULL)
							dlist_init(data_copy_info.dirs_to_copy_list);
						else {
							printf("compare_trees(): malloc() error 2-1.\n");
							exit(1);
						}
					}
					new_dir_location(file_tree_element_a,top_location_b,data_copy_info.dirs_to_copy_list);
	
					data_copy_info.global_dirs_to_copy_num++;
					//data_copy_info.global_files_within_dirs_to_copy_num += file_tree_element_a->complete_file_num;
					//data_copy_info.global_symlinks_within_dirs_to_copy_num += file_tree_element_a->complete_sym_links_num;
					//data_copy_info.global_subdir_files_to_copy_num += file_tree_element_a->subdir_file_num;
					//data_copy_info.global_subdir_symlinks_to_copy_num += file_tree_element_a->subdir_sym_links_num;
					data_copy_info.global_dirs_to_copy_size += file_tree_element_a->complete_dir_size;
					//data_copy_info.global_subdirs_to_copy_num += file_tree_element_a->subdirs_num;
					//data_copy_info.global_subdirs_to_copy_size += file_tree_element_a->subdirs_size;

					file_tree_element_a->found_dir_match = 1;
					file_tree_element_a = file_tree_element_a->next;
				}
				else
					file_tree_element_a = file_tree_element_a->next;
			}
			return 0;
		}
		else if (dirlist_size_a == same_dir_num && dirlist_size_b > same_dir_num) {
			while (file_tree_element_b != NULL) {
				if (file_tree_element_b->found_dir_match != 1) {
					if (data_copy_info.dirs_extraneous_list == NULL) {
						data_copy_info.dirs_extraneous_list = malloc(sizeof(DList));
						if (data_copy_info.dirs_extraneous_list != NULL)
							dlist_init(data_copy_info.dirs_extraneous_list);
						else {
							printf("compare_trees(): malloc() error 2-2.\n");
							exit(1);
						}
					}
					new_dir_location(file_tree_element_b,top_location_a,data_copy_info.dirs_extraneous_list);
	
					data_copy_info.global_dirs_extraneous_num++;
					//data_copy_info.global_subdirs_extraneous_num += file_tree_element_b->subdirs_num;
					//data_copy_info.global_files_within_dirs_extraneous_num += file_tree_element_b->complete_file_num;
					//data_copy_info.global_symlinks_within_dirs_extraneous_num += file_tree_element_b->complete_sym_links_num;
					//data_copy_info.global_files_extraneous_num += file_tree_element_b->complete_file_num;
					data_copy_info.global_dirs_extraneous_size += file_tree_element_b->complete_dir_size;
					//data_copy_info.global_subdirs_extraneous_size += file_tree_element_b->subdirs_size;
	
					file_tree_element_b->found_dir_match = 1;
					file_tree_element_b = file_tree_element_b->next;
				}
				else
					file_tree_element_b = file_tree_element_b->next;
			}
			return 0;
		}
		else if (dirlist_size_a > same_dir_num && dirlist_size_b > same_dir_num) {
			while (file_tree_element_a != NULL) {
				if (file_tree_element_a->found_dir_match != 1) {
					if (data_copy_info.dirs_to_copy_list == NULL) {
						data_copy_info.dirs_to_copy_list = malloc(sizeof(DList));
						if (data_copy_info.dirs_to_copy_list != NULL)
							dlist_init(data_copy_info.dirs_to_copy_list);
						else {
							printf("compare_trees(): malloc() error 2-3.\n");
							exit(1);
						}
					}
					new_dir_location(file_tree_element_a,top_location_b,data_copy_info.dirs_to_copy_list);
	
					data_copy_info.global_dirs_to_copy_num++;
					//data_copy_info.global_subdirs_to_copy_num += file_tree_element_a->subdir_num;
					//data_copy_info.global_files_within_dirs_to_copy_num += file_tree_element_a->complete_file_num;
					//data_copy_info.global_symlinks_within_dirs_to_copy_num += file_tree_element_a->complete_sym_links_num;
					//data_copy_info.global_files_to_copy_num += file_tree_element_a->complete_file_num;
					data_copy_info.global_dirs_to_copy_size += file_tree_element_a->complete_dir_size;
					//data_copy_info.global_subdirs_to_copy_size += file_tree_element_a->subdirs_size;
	
					file_tree_element_a->found_dir_match = 1;
					file_tree_element_a = file_tree_element_a->next;
				}
				else
					file_tree_element_a = file_tree_element_a->next;
			}
			file_tree_element_a = top_location_a;
			while (file_tree_element_b != NULL) {
				if (file_tree_element_b->found_dir_match != 1) {
					if (data_copy_info.dirs_extraneous_list == NULL) {
						data_copy_info.dirs_extraneous_list = malloc(sizeof(DList));
						if (data_copy_info.dirs_extraneous_list != NULL)
							dlist_init(data_copy_info.dirs_extraneous_list);
						else {
							printf("compare_trees(): malloc() error 2-4.\n");
							exit(1);
						}
					}
					new_dir_location(file_tree_element_b,top_location_a,data_copy_info.dirs_extraneous_list);
	
					data_copy_info.global_dirs_extraneous_num++;
					//data_copy_info.global_subdirs_extraneous_num += file_tree_element_b->subdir_num;
					//data_copy_info.global_files_within_dirs_extraneous_num += file_tree_element_b->complete_file_num;
					//data_copy_info.global_symlinks_within_dirs_extraneous_num += file_tree_element_b->complete_sym_links_num;
					//data_copy_info.global_files_extraneous_num += file_tree_element_b->complete_file_num;
					data_copy_info.global_dirs_extraneous_size += file_tree_element_b->complete_dir_size;
					//data_copy_info.global_subdirs_extraneous_size += file_tree_element_b->subdirs_size;
	
					file_tree_element_b->found_dir_match = 1;
					file_tree_element_b = file_tree_element_b->next;
				}
				else
					file_tree_element_b = file_tree_element_b->next;
			}
			return 0;
		}
		else if (dirlist_size_a == same_dir_num && dirlist_size_b == same_dir_num)
			return 0;
	} // if (thread_data_a->no_dirs_a != 1 && thread_data_b->no_dirs_b != 1)
	else if (no_dirs_a == 0 && no_dirs_b == 1) {
		while (file_tree_element_a != NULL) {
			if (data_copy_info.dirs_to_copy_list == NULL) {
				data_copy_info.dirs_to_copy_list = malloc(sizeof(DList));
				if (data_copy_info.dirs_to_copy_list != NULL)
					dlist_init(data_copy_info.dirs_to_copy_list);
				else {
					printf("compare_trees(): malloc() error 2-1.\n");
					exit(1);
				}
			}
			new_dir_location(file_tree_element_a,top_location_b,data_copy_info.dirs_to_copy_list);

			data_copy_info.global_dirs_to_copy_num++;
			//data_copy_info.global_subdirs_to_copy_num += file_tree_element_a->subdir_num;
			//data_copy_info.global_files_within_dirs_to_copy_num += file_tree_element_a->complete_file_num;
			//data_copy_info.global_symlinks_within_dirs_to_copy_num += file_tree_element_a->complete_sym_links_num;
			//data_copy_info.global_files_to_copy_num += file_tree_element_a->complete_file_num;
			data_copy_info.global_dirs_to_copy_size += file_tree_element_a->complete_dir_size;
			//data_copy_info.global_subdirs_to_copy_size += file_tree_element_a->subdirs_size;

			file_tree_element_a = file_tree_element_a->next;
		}
		return 0;
	}
	else if (no_dirs_a == 1 && no_dirs_b == 0) {
		while (file_tree_element_b != NULL) {
			if (data_copy_info.dirs_extraneous_list == NULL) {
				data_copy_info.dirs_extraneous_list = malloc(sizeof(DList));
				if (data_copy_info.dirs_extraneous_list != NULL)
					dlist_init(data_copy_info.dirs_extraneous_list);
				else {
					printf("compare_trees(): malloc() error 2-2.\n");
					exit(1);
				}
			}
			new_dir_location(file_tree_element_b,top_location_a,data_copy_info.dirs_extraneous_list);

			data_copy_info.global_dirs_extraneous_num++;
			//data_copy_info.global_subdirs_extraneous_num += file_tree_element_b->subdir_num;
			//data_copy_info.global_files_within_dirs_extraneous_num += file_tree_element_b->complete_file_num;
			//data_copy_info.global_symlinks_within_dirs_extraneous_num += file_tree_element_b->complete_sym_links_num;
			//data_copy_info.global_files_extraneous_num += file_tree_element_b->complete_file_num;
			data_copy_info.global_dirs_extraneous_size += file_tree_element_b->complete_dir_size;
			//data_copy_info.global_subdirs_extraneous_size += file_tree_element_b->subdirs_size;

			file_tree_element_b = file_tree_element_b->next;
		}
		return 0;
	}
	else if (no_dirs_a == 1 && no_dirs_b == 1)
		return 0;
} // compare_trees()
