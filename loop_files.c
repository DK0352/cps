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

char *new_file_location_diff(DListElmt *main_location, DListElmt *new_location, DList *insert_to);
char *new_file_location_miss(DListElmt *main_location, DList_of_lists *new_location, DList *insert_to);

#define COMPARE_L 33
#define COMPARE_S 44

/* compare the file size/number in the source and destination directories */
int loop_files(DList_of_lists *file_tree_element_a, DList_of_lists *file_tree_element_b)
{
	extern struct Data_Copy_Info data_copy_info;
	DList	 		*files_a, *files_b;
	DListElmt 		*compare_l, *compare_s, *list_l, *list_s;
	DList_of_lists 		*top_dir_pos, *hold_top;
	int 			main_mark, same_file_num, filelist_num_a, filelist_num_b; /* isti fajlovi na obe lokacije */
	long			size;
	mode_t			file1_type, file2_type;

	if (file_tree_element_a->files != NULL) {
		files_a = file_tree_element_a->files;
		filelist_num_a = files_a->num;
	}
	else if (file_tree_element_a->files == NULL) {
		filelist_num_a = 0;
	}
	if (file_tree_element_b->files != NULL) {
		files_b = file_tree_element_b->files;
		filelist_num_b = files_b->num;
	}
	else if (file_tree_element_b->files == NULL) {
		filelist_num_b = 0;
	}

	same_file_num = 0;

	/* compare_l is the larger file list, compare_s is the smaller file list. main_mark marks the source (main) directory. this is arranged so that smaller list is compared against larger, and
	the rest is simply added as the files to copy list, or the files surplus list if the same_file_num variable doesn't match the number of files in the source and destination directories. */
	if (filelist_num_a > 0 && filelist_num_b > 0) {
		if (filelist_num_a > filelist_num_b) {
			compare_l = files_a->head; // files_a is file_tree_element_a->files
			compare_s = files_b->head; // files_b is file_tree_element_b->files
			list_l = files_a->head;
			list_s = files_b->head;
			main_mark = COMPARE_L;
		}
		else if (filelist_num_b > filelist_num_a) {
			compare_l = files_b->head; // files_b is file_tree_element_b->files
			compare_s = files_a->head; // files_a is file_tree_element_a->files
			list_l = files_b->head;
			list_s = files_a->head;
			main_mark = COMPARE_S;
		}
		else if (filelist_num_a == filelist_num_b) {
			compare_l = files_a->head; // files_a is file_tree_element_a->files
			compare_s = files_b->head; // files_b is file_tree_element_b->files
			list_l = files_a->head;
			list_s = files_b->head;
			main_mark = COMPARE_L;
		}
		/* This for loop searches for the files with the same name, compares their size, type, and adds them to the appropriate list. for example: if two files with the same name are found, but their
		size doesn't match, and the file with the main mark is larger, add it to the diff_size_ml_list. (ml is short for main larger). If the file in the destination directory is larger, add it to
		the diff_size_ms_list (ms is short for main smaller). After the loop finishes, if the number of files in the source or destination doesn't match the same_file_num, the rest of the function 
		after the loop takes care whether files are added to the copy or the surplus lists. */
		for (compare_l = list_l; compare_l != NULL; compare_l = compare_l->next) {
			for (compare_s = list_s; compare_s != NULL; compare_s = compare_s->next) {
				/* Compare file names. If they are same and match isn't 1 (which means that that haven't already been compared, set match to 1, 
				increase same_file_num variable and countinue further with comparsion */
				if (compare_l->match != 1 && compare_s->match != 1 && strcmp(compare_l->name,compare_s->name) == 0) {
					compare_l->match = 1;
					compare_s->match = 1;
					same_file_num++; 
					// Found the files with the same name; compare their size, type, and add them to appropriate list if there is some difference...
					if (compare_l->size != compare_s->size) {
						if (compare_l->size > compare_s->size) {
							if (main_mark == COMPARE_L) {
								if (data_copy_info.diff_size_ml_list == NULL) {
									data_copy_info.diff_size_ml_list = malloc(sizeof(DList));
									if (data_copy_info.diff_size_ml_list != NULL)
										dlist_init(data_copy_info.diff_size_ml_list);
									else {
										printf("loop_files() malloc() error 1-2.\n");
										exit(1);
									}
								}
								new_file_location_diff(compare_l,compare_s,data_copy_info.diff_size_ml_list);

								data_copy_info.global_diff_size_ml_num++;
								data_copy_info.global_diff_size_ml_size += compare_l->size;

								break;
							}
							else if (main_mark == COMPARE_S) {
								if (data_copy_info.diff_size_ms_list == NULL) {
									data_copy_info.diff_size_ms_list = malloc(sizeof(DList));
									if (data_copy_info.diff_size_ms_list != NULL)
										dlist_init(data_copy_info.diff_size_ms_list);
									else {
										printf("loop_files() malloc() error 3-2.\n");
										exit(1);
									}
								}
								new_file_location_diff(compare_s,compare_l,data_copy_info.diff_size_ms_list);

								data_copy_info.global_diff_size_ms_num++;
								data_copy_info.global_diff_size_ms_size += compare_s->size;

								break;
							}
						} // if (compare_l->size > compare_s->size
						else if (compare_l->size < compare_s->size) {
							if (main_mark == COMPARE_L) {
								if (data_copy_info.diff_size_ms_list == NULL) {
									data_copy_info.diff_size_ms_list = malloc(sizeof(DList));
									if (data_copy_info.diff_size_ms_list != NULL)
										dlist_init(data_copy_info.diff_size_ms_list);
									else {
										printf("loop_files() malloc() error 5-2\n");
										exit(1);
									}
								}
								new_file_location_diff(compare_l,compare_s,data_copy_info.diff_size_ms_list);

								data_copy_info.global_diff_size_ms_num++;
								data_copy_info.global_diff_size_ms_size += compare_l->size;

								break;
							}
							else if (main_mark == COMPARE_S) {
								if (data_copy_info.diff_size_ml_list == NULL) {
									data_copy_info.diff_size_ml_list = malloc(sizeof(DList));
									if (data_copy_info.diff_size_ml_list != NULL)
										dlist_init(data_copy_info.diff_size_ml_list);
									else {
										printf("loop_files() malloc() error 7-2\n");
										exit(1);
									}
								}
								new_file_location_diff(compare_s,compare_l,data_copy_info.diff_size_ml_list);

								data_copy_info.global_diff_size_ml_num++;
								data_copy_info.global_diff_size_ml_size += compare_s->size;

								break;
							}
						} // if (compare_l->size < compare_s->size
					} // else if (compare_l->size != compare_s->size) {
					file1_type = 0;
					file2_type = 0;
					file1_type |= compare_l->st_mode;
					file2_type |= compare_s->st_mode;
					if ((file1_type & file2_type) == 0) {
						if (data_copy_info.diff_type_list_main == NULL) {
							data_copy_info.diff_type_list_main = malloc(sizeof(DList));
							if (data_copy_info.diff_type_list_main != NULL)
								dlist_init(data_copy_info.diff_type_list_main);
							else {
								printf("loop_files() malloc() error 1. exiting.\n");
								exit(1);
							}
						}
						if (data_copy_info.diff_type_list_secondary == NULL) {
							data_copy_info.diff_type_list_secondary = malloc(sizeof(DList));
							if (data_copy_info.diff_type_list_secondary != NULL)
								dlist_init(data_copy_info.diff_type_list_secondary);
							else {
								printf("loop_files() malloc() error 2. exiting.\n");
								exit(1);
							}
						}
						// rewrite this later
						if (main_mark == COMPARE_L) {
							new_file_location_diff(compare_l,compare_s,data_copy_info.diff_type_list_main);
							
							data_copy_info.global_diff_type_num_main++;
							data_copy_info.global_diff_type_size_main += compare_l->size;

							break;
						}
						else if (main_mark == COMPARE_S) {
							new_file_location_diff(compare_s,compare_l,data_copy_info.diff_type_list_main);

							data_copy_info.global_diff_type_num_main++;
							data_copy_info.global_diff_type_size_main += compare_s->size;

							break;
						}
					} // strcmp() dlist_type
				} // if (compare_l->match != 1 && compare_s->match != 1 && strcmp(compare_l->dirname,compare_s->dirname) == 0) {
			} // for loop 2
		} // for loop 1
	} // if (filelist_num_a > 0 && filelist_num_b > 0)

	// source has files, destination empty
	else if (filelist_num_a > 0 && filelist_num_b == 0) {
		compare_l = files_a->head;
		while (compare_l != NULL) {
			if (compare_l->match != 1) {
				if (data_copy_info.files_to_copy_list == NULL) {
					data_copy_info.files_to_copy_list = malloc(sizeof(DList));
					if (data_copy_info.files_to_copy_list != NULL)
						dlist_init(data_copy_info.files_to_copy_list);
					else {
						printf("compare_trees() malloc() error 2-3.\n");
						exit(1);
					}
				}
				new_file_location_miss(compare_l,file_tree_element_b,data_copy_info.files_to_copy_list);
				data_copy_info.global_files_to_copy_num++;
				data_copy_info.global_files_to_copy_size += compare_l->size;
				compare_l->match = 1;
				compare_l = compare_l->next;
			} 
			// ovo nema smisla ak je druga strana nula, jer onda uopće nisu uspoređivani
			else if (compare_l->match == 1)
				compare_l = compare_l->next;
		} // while() loop
		return 0;
	} // if (filelist_num_a > 0 && filelist_num_b == 0)
	// source is empty, destination has files
	else if (filelist_num_a == 0 && filelist_num_b > 0) {
		compare_s = files_b->head;
		while (compare_s != NULL) {
			if (compare_s->match != 1) {
				if (data_copy_info.files_surplus_list == NULL) {
					data_copy_info.files_surplus_list = malloc(sizeof(DList));
					if (data_copy_info.files_surplus_list != NULL)
						dlist_init(data_copy_info.files_surplus_list);
					else {
						printf("loop_files() malloc() error 2-4.\n");
						exit(1);
					}
				}
				new_file_location_miss(compare_s,file_tree_element_a,data_copy_info.files_surplus_list);
				data_copy_info.global_files_surplus_num++;
				data_copy_info.global_files_surplus_size += compare_s->size;
				compare_s->match = 1;
				compare_s = compare_s->next;
			} 
			else if (compare_s->match == 1)
				compare_s = compare_s->next;
		} // while() loop
		return 0;
	} // else if (filelist_num_a == 0 && filelist_num_b > 0)
	// both source and destination empty of files. function probably woudn't be called in the first place, but who knows?
	else if (filelist_num_a == 0 && filelist_num_b == 0)
		return 0;
	/*************************************** all files in the directory compared, but same_file_num is not matched, so find the missing/surplus files ************************************************/
	compare_l = list_l;
	compare_s = list_s;
	// source directory is compare_l
	if (main_mark == COMPARE_L) {
		// source directory has more than just equal number of files than destination directory
		if (filelist_num_a > same_file_num) {
			while (compare_l != NULL) {
				if (compare_l->match != 1) {
					if (data_copy_info.files_to_copy_list == NULL) {
						data_copy_info.files_to_copy_list = malloc(sizeof(DList));
						if (data_copy_info.files_to_copy_list != NULL)
							dlist_init(data_copy_info.files_to_copy_list);
						else {
							printf("loop_files() malloc() error 2-5.\n");
							exit(1);
						}
					}
					new_file_location_miss(compare_l,file_tree_element_b,data_copy_info.files_to_copy_list);
					data_copy_info.global_files_to_copy_num++;
					data_copy_info.global_files_to_copy_size += compare_l->size;
					compare_l->match = 1;
					compare_l = compare_l->next;
				} 
				else if (compare_l->match == 1)
					compare_l = compare_l->next;
			}  // while (compare_l != NULL)
		} // if (filelist_num_a > same_file_num) {
		compare_l = list_l;
		// destination directory has more than just equal number of files than source directory
		if (filelist_num_b > same_file_num) {
			while (compare_s != NULL) {
				if (compare_s->match != 1) {
					if (data_copy_info.files_surplus_list == NULL) {
						data_copy_info.files_surplus_list = malloc(sizeof(DList));
						if (data_copy_info.files_surplus_list != NULL)
							dlist_init(data_copy_info.files_surplus_list);
						else {
							printf("loop_files() malloc() error 2-6.\n");
							exit(1);
						}
					}
					new_file_location_miss(compare_s,file_tree_element_a,data_copy_info.files_surplus_list);
					data_copy_info.global_files_surplus_num++;
					data_copy_info.global_files_surplus_size += compare_s->size;
					compare_s->match = 1;
					compare_s = compare_s->next;
				} 
				else if (compare_s->match == 1)
					compare_s = compare_s->next;
			} // while (compare_s != NULL)
		} // if (filelist_num_b > same_file_num)
		compare_s = list_s;
	} // if (main_mark == COMPARE_L)

	// source directory is compare_s
	else if (main_mark == COMPARE_S) {
		// destination directory has more than just equal number of files than source directory
		if (filelist_num_a > same_file_num) {
			while (compare_s != NULL) {
				if (compare_s->match != 1) {
					if (data_copy_info.files_to_copy_list == NULL) {
						data_copy_info.files_to_copy_list = malloc(sizeof(DList));
						if (data_copy_info.files_to_copy_list != NULL)
							dlist_init(data_copy_info.files_to_copy_list);
						else {
							printf("loop_files() malloc() error 2-7.\n");
							exit(1);
						}
					}
					new_file_location_miss(compare_s,file_tree_element_b,data_copy_info.files_to_copy_list);
					data_copy_info.global_files_to_copy_num++;
					data_copy_info.global_files_to_copy_size += compare_s->size;
					compare_s->match = 1;
					compare_s = compare_s->next;
				}
				else if (compare_s->match == 1)
					compare_s = compare_s->next;
			} // while (compare_s != NULL)
		} // if (filelist_num_b > same_file_num)
		// source directory has more than just equal number of files than destination directory
		if (filelist_num_b > same_file_num) {
			while (compare_l != NULL) {
				if (compare_l->match != 1) {
					if (data_copy_info.files_surplus_list == NULL) {
						data_copy_info.files_surplus_list = malloc(sizeof(DList));
						if (data_copy_info.files_surplus_list != NULL)
							dlist_init(data_copy_info.files_surplus_list);
						else {
							printf("loop_files() malloc() error 2-8.\n");
							exit(1);
						}
					}
					new_file_location_miss(compare_l,file_tree_element_a,data_copy_info.files_surplus_list);
					data_copy_info.global_files_surplus_num++;
					data_copy_info.global_files_surplus_size += compare_l->size;
					compare_l->match = 1;
					compare_l = compare_l->next;
				} 
				else if (compare_l->match == 1)
					compare_l = compare_l->next;
			} // while (compare_l != NULL)
		} // if (filelist_num_a > same_file_num)
	} // else if (main_mark == COMPARE_S)
}
