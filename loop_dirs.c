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
#include "options.h"
#include "errors.h"

int loop_files(DList_of_lists *file_tree_element_a, DList_of_lists *file_tree_element_b);
char *new_dir_location(DList_of_lists *main_location, DList_of_lists *new_location, DList *insert_to);

#define COMPARE_L 33
#define COMPARE_S 44

/* Called from each top directory if there is some difference in number/size of subdirectories. It is called recursively until the exact 
   spot in which there is difference is found. Also calls loop_files() if it detects difference in file size/number for certain directory. */
int loop_dirs(DList_of_lists *file_tree_element_a, DList_of_lists *file_tree_element_b) 
{
	extern struct Data_Copy_Info data_copy_info;
	extern struct options_menu options;
	DList_of_lists *start_a, *start_b, *location_a_down, *location_b_down, *compare_l, *compare_s, *list_l, *list_s;
	int same_dir_num, main_mark;
	int dirlist_size_a;
	int dirlist_size_b;

	same_dir_num = 0;

	start_a = file_tree_element_a;
	if (file_tree_element_a->down != NULL) {
		file_tree_element_a = file_tree_element_a->down;
		dirlist_size_a = file_tree_element_a->up->directories->num;
		start_a = file_tree_element_a;
	}
	else {
		dirlist_size_a = 0;
	}
		
	start_b = file_tree_element_b;
	if (file_tree_element_b->down != NULL) {
		file_tree_element_b = file_tree_element_b->down;
		dirlist_size_b = file_tree_element_b->up->directories->num;
		start_b = file_tree_element_b;
	}
	else {
		dirlist_size_b = 0;
	}

	if (dirlist_size_a != 0 && dirlist_size_b != 0) {
		if (dirlist_size_a > dirlist_size_b) {
			compare_l = file_tree_element_a;
			compare_s = file_tree_element_b;
			list_l = file_tree_element_a;
			list_s = file_tree_element_b;
			main_mark = COMPARE_L;
		}
		if (dirlist_size_a < dirlist_size_b) {
			compare_l = file_tree_element_b;
			compare_s = file_tree_element_a;
			list_l = file_tree_element_b;
			list_s = file_tree_element_a;
			main_mark = COMPARE_S;
		}
		if (dirlist_size_a == dirlist_size_b) {
			compare_l = file_tree_element_a;
			compare_s = file_tree_element_b;
			list_l = file_tree_element_a;
			list_s = file_tree_element_b;
			main_mark = COMPARE_L;
		}
		for ( ; list_l != NULL; list_l = list_l->next) {
			for (list_s = start_b; list_s != NULL; list_s = list_s->next) {
				if (list_l->found_dir_match != 1 && list_s->found_dir_match != 1) {
					if (strcmp(list_l->dirname,list_s->dirname) == 0) {
						list_l->found_dir_match = 1;
						list_s->found_dir_match = 1;
						same_dir_num++;
						if (options.naive_mode == 0) {
							if (list_l->file_num != 0 || list_s->file_num != 0)
								loop_files(list_l,list_s);
							loop_dirs(list_l,list_s);
						}
						else if (options.naive_mode == 1) {
							if (list_l->files_size != list_s->files_size || list_l->file_num != list_s->file_num) {
								loop_files(list_l,list_s);
							}
							if (list_l->subdirs_size != list_s->subdirs_size || 
								list_l->subdir_num != list_s->subdir_num ||
								list_l->subdir_file_num != list_s->subdir_file_num) {
								loop_dirs(list_l,list_s);
							}
						}	
					} // if (strcmp(list_l->dirname,list_s->dirname) == 0)
				}
			} // for_loop 2
		} // for_loop 1

		file_tree_element_a = start_a;
		file_tree_element_b = start_b;

		// More directories in the source directory than just those equal with the destination directory. Add them to the copy list.
		if (dirlist_size_a > same_dir_num && dirlist_size_b == same_dir_num) {
			// this_directory: to get actual directory pathname of directory in case it is empty. equal result would be element_b->up, which may happen in the future.
			while (file_tree_element_a != NULL) {
				if (file_tree_element_a->found_dir_match != 1) {
					if (data_copy_info.dirs_to_copy_list == NULL) {
						data_copy_info.dirs_to_copy_list = malloc(sizeof(DList)); 
						if (data_copy_info.dirs_to_copy_list != NULL)
							dlist_init(data_copy_info.dirs_to_copy_list);
						else {
							printf("loop_dirs(): malloc() error 2-1.\n");
							exit(1);
						}
					}
					new_dir_location(file_tree_element_a,file_tree_element_b->up,data_copy_info.dirs_to_copy_list);

					data_copy_info.global_dirs_to_copy_num++;
					data_copy_info.global_dirs_to_copy_num += file_tree_element_a->complete_dir_num;
					data_copy_info.global_files_to_copy_num += file_tree_element_a->complete_file_num;
					data_copy_info.global_dirs_to_copy_size += file_tree_element_a->complete_dir_size;

					file_tree_element_a->found_dir_match = 1;
					file_tree_element_a = file_tree_element_a->next;
				}
				else
					file_tree_element_a = file_tree_element_a->next;
			}
		} // if (dirlist_size_a > same_dir_num && dirlist_size_b == same_dir_num) {
		// More directories in the destination directory than just those equal with the source directory. Add them to the surplus list.
		else if (dirlist_size_a == same_dir_num && dirlist_size_b > same_dir_num) {
			//file_tree_element_b = file_tree_element_b->first_dir_in_chain;
			//file_tree_element_a = file_tree_element_a->this_directory;
			while (file_tree_element_b != NULL) {
				if (file_tree_element_b->found_dir_match != 1) {
					if (data_copy_info.dirs_surplus_list == NULL) {
						data_copy_info.dirs_surplus_list = malloc(sizeof(DList));
						if (data_copy_info.dirs_surplus_list != NULL) 
							dlist_init(data_copy_info.dirs_surplus_list);
						else {
							printf("loop_dirs() malloc() error 3-2.\n");
							exit(1);
						}
					}
					new_dir_location(file_tree_element_b,file_tree_element_a->up,data_copy_info.dirs_surplus_list);

					data_copy_info.global_dirs_surplus_num++;
					data_copy_info.global_dirs_surplus_num += file_tree_element_b->complete_dir_num;
					data_copy_info.global_files_surplus_num += file_tree_element_b->complete_file_num;
					data_copy_info.global_dirs_surplus_size += file_tree_element_b->complete_dir_size;

					file_tree_element_b->found_dir_match = 1;
					file_tree_element_b = file_tree_element_b->next;
				}
				else
					file_tree_element_b = file_tree_element_b->next;
			}
		} // else if (dirlist_size_a == same_dir_num && dirlist_size_b > same_dir_num)
		// More directories in both the source and destination directory than just those equal. Add them to the appropriate lists.
		else if (dirlist_size_a > same_dir_num && dirlist_size_b > same_dir_num) {
			//file_tree_element_a = file_tree_element_a->first_dir_in_chain;
			//file_tree_element_b = file_tree_element_b->this_directory;
			while (file_tree_element_a != NULL) {
				if (file_tree_element_a->found_dir_match != 1) {
					if (data_copy_info.dirs_to_copy_list == NULL) {
						data_copy_info.dirs_to_copy_list = malloc(sizeof(DList));
						if (data_copy_info.dirs_to_copy_list != NULL)
							dlist_init(data_copy_info.dirs_to_copy_list);
						else {
							printf("loop_dirs() malloc() error 2-4.\n");
							exit(1);
						}
					}
					new_dir_location(file_tree_element_a,file_tree_element_b->up,data_copy_info.dirs_to_copy_list);

					data_copy_info.global_dirs_to_copy_num++;
					data_copy_info.global_dirs_to_copy_num += file_tree_element_a->complete_dir_num;
					data_copy_info.global_files_to_copy_num += file_tree_element_a->complete_file_num;
					data_copy_info.global_dirs_to_copy_size += file_tree_element_a->complete_dir_size;

					file_tree_element_a->found_dir_match = 1;
					file_tree_element_a = file_tree_element_a->next;
				}
				else
					file_tree_element_a = file_tree_element_a->next;
			}
			file_tree_element_a = start_a;
			while (file_tree_element_b != NULL) {
				if (file_tree_element_b->found_dir_match != 1) {
					if (data_copy_info.dirs_surplus_list == NULL) {
						data_copy_info.dirs_surplus_list = malloc(sizeof(DList));	
						if (data_copy_info.dirs_surplus_list != NULL)
							dlist_init(data_copy_info.dirs_surplus_list);
						else {
							printf("loop_dirs() malloc() error 3-5.\n");
							exit(1);
						}
					}
					new_dir_location(file_tree_element_b,file_tree_element_a->up,data_copy_info.dirs_surplus_list);

					data_copy_info.global_dirs_surplus_num++;
					data_copy_info.global_dirs_surplus_num += file_tree_element_b->complete_dir_num;
					data_copy_info.global_files_surplus_num += file_tree_element_b->complete_file_num;
					data_copy_info.global_dirs_surplus_size += file_tree_element_b->complete_dir_size;

					file_tree_element_b->found_dir_match = 1;
					file_tree_element_b = file_tree_element_b->next;
				}
				else
					file_tree_element_b = file_tree_element_b->next;
			}
		} // else if (dirlist_size_a > same_dir_num && dirlist_size_b > same_dir_num) {
	} // if (directories_a->size != 0 && directories_b->size != 0) {

	// Source directory has some directories, destination is empty
	else if (dirlist_size_a > 0 && dirlist_size_b == 0) {
		//file_tree_element_a = file_tree_element_a->first_dir_in_chain;
		//file_tree_element_b = file_tree_element_b->this_directory;
		while (file_tree_element_a != NULL) {
			if (file_tree_element_a->found_dir_match != 1) {
				if (data_copy_info.dirs_to_copy_list == NULL) {
					data_copy_info.dirs_to_copy_list = malloc(sizeof(DList));
					if (data_copy_info.dirs_to_copy_list != NULL)
						dlist_init(data_copy_info.dirs_to_copy_list);
					else {
						printf("loop_dirs() malloc() error 2-6.\n");
						exit(1);
					}
				}
				new_dir_location(file_tree_element_a,file_tree_element_b->up,data_copy_info.dirs_to_copy_list);

				data_copy_info.global_dirs_to_copy_num++;
				data_copy_info.global_dirs_to_copy_num += file_tree_element_a->complete_dir_num;
				data_copy_info.global_files_to_copy_num += file_tree_element_a->complete_file_num;
				data_copy_info.global_dirs_to_copy_size += file_tree_element_a->complete_dir_size;

				file_tree_element_a->found_dir_match = 1;
				file_tree_element_a = file_tree_element_a->next;
			}
			else
				file_tree_element_a = file_tree_element_a->next;
		} // while (file_tree_element_a != NULL) {
	} // else if (directories_a->size > 0 && directories_b->size == 0) {

	// Source directory is empty, destination has some directories
	else if (dirlist_size_a == 0 && dirlist_size_b > 0) {
		//file_tree_element_b = file_tree_element_b->first_dir_in_chain;
		//file_tree_element_a = file_tree_element_a->this_directory;
		while (file_tree_element_b != NULL) {
			if (file_tree_element_b->found_dir_match != 1) {
				if (data_copy_info.dirs_surplus_list == NULL) {
					data_copy_info.dirs_surplus_list = malloc(sizeof(DList));	
					if (data_copy_info.dirs_surplus_list != NULL)
						dlist_init(data_copy_info.dirs_surplus_list);
					else {
						printf("loop_dirs() malloc() error 2-7.\n");
						exit(1);
					}
				}
				new_dir_location(file_tree_element_b,file_tree_element_a->up,data_copy_info.dirs_surplus_list);

				data_copy_info.global_dirs_surplus_num++;
				data_copy_info.global_dirs_surplus_num += file_tree_element_b->complete_dir_num;
				data_copy_info.global_files_surplus_num += file_tree_element_b->complete_file_num;
				data_copy_info.global_dirs_surplus_size += file_tree_element_b->complete_dir_size;

				file_tree_element_b->found_dir_match = 1;
				file_tree_element_b = file_tree_element_b->next;
			}
			else
				file_tree_element_b = file_tree_element_b->next;
		} // while (file_tree_element_b != NULL) {
	} // else if (directories_a->size == 0 && directories_b->size > 0) {
	else if (dirlist_size_a == 0 && dirlist_size_b == 0)
		return 0;
	
	return 0;
} // int loop_dirs()
